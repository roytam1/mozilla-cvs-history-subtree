#!/bin/sh
#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "NPL"); you may not use this file except in
# compliance with the NPL.  You may obtain a copy of the NPL at
# http://www.mozilla.org/NPL/
#
# Software distributed under the NPL is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
# for the specific language governing rights and limitations under the
# NPL.
#
# The Initial Developer of this code under the NPL is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation.  All Rights
# Reserved.
#

##############################################################################
##
## Name:		glibcversion.sh - a fast way to get a motif lib's version
##
## Description:	Print the major and minor version numbers for motif libs on
##				the system that executes the script.  Could be tweaked
##				to output more info.
##
##              More checks need to be added for more platforms.  
##              Currently this script is only useful in Linux Universe
##              where there are a many versions of motif.
##
## Author:		Ramiro Estrugo <ramiro@netscape.com>
##
##############################################################################


##
## To be done: Handle startup flags:
##
##  --is-lesstif:		Prints True/False if using lesstif.
##  --major-only:		Prints only the major version number.
##  --minor-only:		Prints only the minor version number.
##  --patch-only:		Prints only the patch level number.
##  --include-dir:		Prints the location of the motif headers.
##  --lib-dir:			Prints the location of the motif libs.
##
##  Plus other magic that will be needed as more hacks are added.
##

TEST_MOTIF_PROG=./test_motif
TEST_MOTIF_SRC=test_motif.c
TEST_MOTIF_DIR=
TEST_MOTIF_FLAGS=

rm -f $TEST_MOTIF_PROG
rm -f $TEST_MOTIF_SRC

POSSIBLE_MOTIF_DIRS="\
/usr/lesstif/include \
/usr/local/include \
/usr/X11R6/include \
/usr/include/Xm \
"

for d in $POSSIBLE_MOTIF_DIRS
do
	if [ -d $d/Xm ]
	then
		TEST_MOTIF_DIR=$d
		break;
	fi
	
done

if [ -z $TEST_MOTIF_DIR ]
then
	echo
	echo "Could not find motif/lesstif headers."
	echo

	exit
fi

TEST_MOTIF_FLAGS=-I$TEST_MOTIF_DIR

cat << EOF > test_motif.c
#include <stdio.h>
#include <Xm/Xm.h>

int
main(int argc,char ** argv)
{
	fprintf(stdout,"%d.%d\n",XmVERSION,XmREVISION);

	return 0;
}
EOF

if [ ! -f $TEST_MOTIF_SRC ]
then
	echo
	echo "Could not create test program source $TEST_MOTIF_SRC."
	echo

	exit
fi

cc $TEST_MOTIF_FLAGS -o $TEST_MOTIF_PROG $TEST_MOTIF_SRC

if [ ! -x $TEST_MOTIF_PROG ]
then
	echo
	echo "Could not create test program $TEST_MOTIF_PROG."
	echo

	exit
fi

$TEST_MOTIF_PROG

rm -f $TEST_MOTIF_PROG
rm -f $TEST_MOTIF_SRC

