#!/usr/bin/env python3

from sys import argv, stderr
from os import stat
from time import time

mtime_metrics = {}
data_metrics = {}

def check_file(fname: str, get_contents: bool = False):
    info = stat(fname)

    mtime_metrics[fname] = info.st_mtime
    if get_contents:
        with open(fname) as file:
            data_metrics[fname] = float(file.read().strip())

def collect():
    mtime_metrics.clear()
    data_metrics.clear()

    for fname in argv[1:]:
        get_contents = False
        if fname[0] == "@":
            fname = fname[1:]
            get_contents = True

        try:
            check_file(fname, get_contents)
        except Exception as e:
            stderr.write(f'Error checking file {fname}: {e}\n')
            stderr.flush()
            mtime_metrics[fname] = -1
            if get_contents:
                data_metrics.pop(fname, None)

    output = """# HELP filemon_age_seconds The age of the file in seconds
# TYPE filemon_age_seconds gauge
"""
    ctime = time()
    for fname, mtime in mtime_metrics.items():
        if mtime < 0:
            continue
        age = ctime - mtime
        output += f'filemon_age_seconds{{file="{fname}"}} {int(age)}\n'

    output += """# HELP filemon_mtime_seconds The mtime of the file in seconds since epoch
# TYPE filemon_mtime_seconds gauge
"""
    for fname, mtime in mtime_metrics.items():
        output += f'filemon_mtime_seconds{{file="{fname}"}} {int(mtime)}\n'

    output += """# HELP filemon_data_number The data in the file
# TYPE filemon_data_number gauge
"""
    for fname, data in data_metrics.items():
        output += f'filemon_data_number{{file="{fname}"}} {data}\n'

    print(output)

def main():
    collect()

if __name__ == '__main__':
    main()
