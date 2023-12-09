#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>

#define JAILDIR "/tmp/simplejail.root"
#define SHELL "/bin/bash"

// gcc simplejail.c -O2 -o /usr/bin/simplejail && chmod +s /usr/bin/simplejail

// https://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix
static int mkdir_p(const char *path, int mode) {
    char *buf = strdup(path);
    char *p = buf;
    int ret = 0;

    if (buf == NULL) {
        return -1;
    }

    do {
        p = strchr(p + 1, '/');
        if (p) {
            *p = '\0';
        }
        if (mkdir(buf, mode) != 0) {
            if (errno != EEXIST) {
                ret = errno;
                break;
            }
        }
        if (p) {
            *p = '/';
        }
    } while (p);

    free(buf);

    return ret;
}

#define MOUNT_FLAGS_SYS (MS_RDONLY | MS_NOSUID)

#define mount_bind_mkdirfunc(src, flags, mkdirfunc) { \
    if (access(src, F_OK) == 0) { \
        mkdirfunc(src); \
        if (mount(src, src + 1, NULL, MS_SILENT | MS_BIND | MS_PRIVATE, NULL)) { \
            perror("mount"); \
            return 1; \
        } \
        if (mount("none", src + 1, NULL, MS_SILENT | MS_BIND | MS_PRIVATE | MS_REMOUNT | flags, NULL)) { \
            perror("remount"); \
            return 1; \
        } \
    } \
}
#define mount_bind(src, flags) mount_bind_mkdirfunc(src, flags, mkdir_check)
#define mount_bind_mkdirp(src, flags) mount_bind_mkdirfunc(src, flags, mkdirp_check)

#define mkdir_chmod_check(src, mode) { \
    if (mkdir(src + 1, mode)) { \
        perror("mkdir_p"); \
        return 1; \
    } \
}
#define mkdir_check(src) mkdir_chmod_check(src, 0755)

#define mkdirp_chmod_check(src, mode) { \
    if (mkdir_p(src + 1, mode)) { \
        perror("mkdir_p"); \
        return 1; \
    } \
}
#define mkdirp_check(src) mkdirp_chmod_check(src, 0755)

int main(int argc, char** argv) {
    int uid = getuid();
    int gid = getgid();
    if (uid == 0 || gid == 0) {
        printf("Don't run this as root!\n");
        return 1;
    }

    char cwdbuf[4096];
    char* cwd = getcwd(cwdbuf, 4096);

    mkdir(JAILDIR, 0755);

    if (unshare(CLONE_NEWNS | CLONE_FILES | CLONE_FS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWUTS)) {
        perror("unshare");
        return 1;
    }

    if (mount("none", "/", NULL, MS_SILENT | MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("mount_private_rec");
        return 1;
    }

    if (mount("none", JAILDIR, "tmpfs", MS_SILENT, NULL)) {
        perror("mount_tmp");
        return 1;
    }

    if (chmod(JAILDIR, 0755)) {
        perror("chmod_jail");
        return 1;
    }

    if (chdir(JAILDIR)) {
        perror("chdir_jail");
        return 1;
    }

    struct passwd *pw = getpwuid(uid);

    mount_bind("/bin", MOUNT_FLAGS_SYS);
    mount_bind("/sbin", MOUNT_FLAGS_SYS);
    mount_bind("/lib", MOUNT_FLAGS_SYS);
    mount_bind("/lib64", MOUNT_FLAGS_SYS);
    mount_bind("/usr", MOUNT_FLAGS_SYS);

    mount_bind("/etc", MOUNT_FLAGS_SYS);

    mode_t old_umask = umask(0);
    mkdir_chmod_check("/tmp", 01777);
    umask(old_umask);

    mkdir_check("/home");

    mount_bind("/proc", 0);
    mount_bind("/sys", 0);
    mount_bind("/dev", 0);

    mount_bind_mkdirp(pw->pw_dir, MS_NOSUID);
    mount_bind_mkdirp("/mnt/zhdd/nas", MOUNT_FLAGS_SYS);

    if (mount(NULL, JAILDIR, NULL, MS_SILENT | MS_REMOUNT | MOUNT_FLAGS_SYS, NULL)) {
        perror("remount_tmp");
        return 1;
    }

    if (chroot(".")) {
        perror("chroot");
        return 1;
    }

    if (chdir(cwd) && chdir("/")) {
        perror("chdir_root");
        return 1;
    }

    if (setresgid(gid, gid, gid)) {
        perror("setresgid");
        return 1;
    }
    if (setresuid(uid, uid, uid)) {
        perror("setresuid");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return status;
    }

    execv(SHELL, argv);
    perror("execv");
    return 1;
}
