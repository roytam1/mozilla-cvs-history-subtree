#!/usr/bin/python
# vim:set ts=4 sts=4 sw=4 noet:
#
# This tool generates full update packages for the update system.
# Author: Darin Fisher
#

import sys;
import os;
import shutil;

#-----------------------------------------------------------------------------

def print_usage():
	bn = os.path.basename(sys.argv[0]);
	print "Usage: %s [OPTIONS] ARCHIVE DIRECTORY" % (bn);

def enum_dir(dir):
	result = [];
	for f in os.listdir(dir):
		if dir == os.curdir:
			path = f;
		else:
			path = dir + os.sep + f;
		if os.path.isfile(path):
			result.append(path);
		elif os.path.isdir(path):
			result.extend(enum_dir(path));
	return result;

def ensure_dir(dir):
	if not(os.path.isdir(dir)):
		os.makedirs(dir);

#-----------------------------------------------------------------------------
	
if (len(sys.argv) < 2):
	print_usage();
	sys.exit(1);

if (sys.argv[1] == "-h"):
	print_usage();
	print ""
	print "The contents of DIRECTORY will be stored in ARCHIVE."
	print ""
	print "Options:"
	print "  -h  show this help text"
	print ""
	sys.exit(0);

if (len(sys.argv) < 3):
	print_usage();
	sys.exit(1);

archive = sys.argv[1];
targetdir = sys.argv[2];
workdir = targetdir + ".work";

# remember the current working directory so we can restore it later
cwd = os.getcwd();

os.chdir(targetdir);
targetfiles = enum_dir(os.curdir);
os.chdir(cwd);

ensure_dir(workdir);

manifest_path = workdir + os.sep + "update.manifest";
manifest = open(manifest_path, "wt");

for f in targetfiles:
	print "  processing %s" % (f);
	manifest.write("add \"" + f + "\"\n");
	src = targetdir + os.sep + f;
	dest = workdir + os.sep + f;
	ensure_dir(os.path.dirname(dest));
	# bzip2 compress the target file
	cmd = "bzip2 -cz9 \"" + src + "\" > \"" + dest + "\"";
	os.system(cmd);
	# fixup permissions
	if os.access(src, os.X_OK):
		os.chmod(dest, 0755);
	else:
		os.chmod(dest, 0644);

manifest.close();

# compress the manifest file
cmd = "bzip2 -z9 " + manifest_path;
os.system(cmd);
os.rename(manifest_path + ".bz2", manifest_path);

os.chdir(workdir);
cmd = "mar -c output.mar update.manifest";
for f in targetfiles:
	cmd += " \"" + f + "\"";
os.system(cmd);
os.chdir(cwd);
os.rename(workdir + os.sep + "output.mar", archive);

# cleanup
shutil.rmtree(workdir);
