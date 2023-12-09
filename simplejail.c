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

#define mount_bind_mkdirfunc(src, mkdirfunc) { \
    if (access(src, F_OK) == 0) { \
        mkdirfunc(src); \
        if (mount(src, src + 1, NULL, MS_SILENT | MS_BIND, NULL)) { \
            perror("mount"); \
            return 1; \
        } \
    } \
}
#define mount_bind(src) mount_bind_mkdirfunc(src, mkdir_check)
#define mount_bind_mkdirp(src) mount_bind_mkdirfunc(src, mkdirp_check)

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

    if (unshare(CLONE_NEWNS | CLONE_FS | CLONE_FILES)) {
        perror("unshare");
        return 1;
    }

    mkdir(JAILDIR, 0755);

    if (mount(NULL, JAILDIR, "tmpfs", 0, NULL)) {
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

    mount_bind("/bin");
    mount_bind("/sbin");
    mount_bind("/lib");
    mount_bind("/lib64");
    mount_bind("/usr");

    mount_bind("/etc");

    mode_t old_umask = umask(0);
    mkdir_chmod_check("/tmp", 01777);
    umask(old_umask);

    mkdir_check("/home");

    mount_bind("/proc");
    mount_bind("/sys");
    mount_bind("/dev");

    mount_bind_mkdirp(pw->pw_dir);

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

    execv(SHELL, argv);
    perror("execv");
    return 1;
}
