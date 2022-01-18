#!/usr/bin/env python3

import os, sys
from stat import *

olduids = int(sys.argv[1])
newuids = int(sys.argv[2])

def rec(dir):
	for file in os.scandir(dir):
		stat = file.stat(follow_symlinks=False)
		cho(file.path, stat)
		if S_ISDIR(stat.st_mode):
			rec(file.path)

def inid(uid):
	return (uid >= olduids) and (uid <= (olduids + 65536))

def tf(uid):
	return (uid - olduids) + newuids

def cho(filename, stat):
	#stat = os.lstat(filename)
	uid = stat.st_uid
	gid = stat.st_gid

	if inid(uid):
		uid = tf(uid)
	if inid(gid):
		gid = tf(gid)

	if uid != stat.st_uid or gid != stat.st_gid:
		print(filename)
		os.chown(filename, uid, gid, follow_symlinks=False)

rec(sys.argv[3])
