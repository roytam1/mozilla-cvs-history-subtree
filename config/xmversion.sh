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
## Name:		xmversion.sh - Get motif lib location and info.
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
##  --is-lesstif:			Prints True/False if LESSTIF_VERSION is defined.
##
##  --print-version:		Prints XmVERSION.
##  --print-revision:		Prints XmREVISION.
##  --print-update-level:	Prints XmUPDATE_LEVEL.
##  --print-version-string:	Prints XmVERSION_STRING.
##
##  --print-include-dir:	Prints the dir of the motif headers.
##  --print-lib-dir:		Prints the dir of the motif libs.
##
##  --shared-ext:			Shared externsion used by system: so sl.
##  --archive-ext:			Archive externsion used by system: a
##

##
## Look for motif stuff in the followin places:
##
XM_SEARCH_PATH="\
/usr/lesstif \
/usr/local \
/usr/X11R6 \
/usr \
"

##
## Defaults
##
XM_SHARED_EXT=so
XM_ARCHIVE_EXT=a

XM_PROG=./test_motif
XM_SRC=test_motif.c

XM_IS_LESSTIF=False

XM_PRINT_VERSION=False
XM_PRINT_REVISION=False
XM_PRINT_UPDATE_LEVEL=False
XM_PRINT_INCLUDE_DIR=False
XM_PRINT_LIB_DIR=False

XM_MAJOR_NUMBER=
XM_MINOR_NUMBER=
XM_UPDATE_NUMBER=
XM_PRINT_INCLUDE_DIR=
XM_LIB_DIR=

XM_ARCHIVE_LIB=
XM_SHARED_LIB=
XM_ARCHIVE_DIR=
XM_SHARED_DIR=

function test_motif_usage()
{
	echo
	echo FOO
	echo
}

##
## Parse the command line
##
while [ "$*" ]; do
    case $1 in
        -h | --help)
            shift
            test_motif_usage
			exit 0
            ;;
        -t | --is-lesstif)
            shift
            XM_IS_LESSTIF=True
            ;;
        -v | --print-version)
            shift
            XM_PRINT_VERSION=True
            ;;
        -r | --print-revision)
            shift
            XM_PRINT_REVISION=True
            ;;
        -u | --print-update-level)
            shift
            XM_PRINT_UPDATE_LEVEL=True
            ;;
        -s | --print-version-string)
            shift
            XM_PRINT_VERSION_STRING=True
            ;;
        -i | --print-include-dir)
            shift
            XM_PRINT_INCLUDE_DIR=True
            ;;
        -l | --print-lib-dir)
            shift
            XM_PRINT_LIB_DIR=True
            ;;
        -e | --shared-ext)
            shift
            XM_SHARED_EXT="$1"
            shift
            ;;
        -a | --archive-ext)
            shift
            XM_ARCHIVE_EXT="$1"
            shift
            ;;
        -*)
            echo "`basename $0`: invalid option '$1'"
            shift
            test_motif_usage
			exit 0
            ;;
    esac
done

##
## The library names
##
XM_SHARED_LIB_NAME=libXm.$XM_SHARED_EXT
XM_ARCHIVE_LIB_NAME=libXm.$XM_ARCHIVE_EXT

##
## Cleanup the dummy test source/program
##
function xm_cleanup()
{
	rm -f $XM_PROG
	rm -f $XM_SRC
}

#echo "XM_PROG = $XM_PROG"
#echo "XM_SRC = $XM_SRC"
#echo "XM_IS_LESSTIF = $XM_IS_LESSTIF"
#echo "XM_PRINT_VERSION = $XM_PRINT_VERSION"
#echo "XM_PRINT_REVISION = $XM_PRINT_REVISION"
#echo "XM_PRINT_UPDATE_LEVEL = $XM_PRINT_UPDATE_LEVEL"
#echo "XM_PRINT_INCLUDE_DIR = $XM_PRINT_INCLUDE_DIR"
#echo "XM_LIB_DIR = $XM_LIB_DIR"
#echo "XM_SHARED_EXT = $XM_SHARED_EXT"
#echo "XM_ARCHIVE_EXT = $XM_ARCHIVE_EXT"

##
## Cleanup the dummy test program
##
rm -f $XM_PROG
rm -f $XM_SRC

##
## Look for <Xm/Xm.h>
##
for d in $XM_SEARCH_PATH
do
	if [ -d $d/include/Xm ]
	then
		XM_INCLUDE_DIR=$d/include
		break;
	fi
done

##
## Make sure the <Xm/Xm.h> header was found.
##
if [ -z $XM_INCLUDE_DIR ]
then
	echo
	echo "Could not find motif/lesstif headers."
	echo

	xm_cleanup
	exit 1
fi

##
## Look for libraries
##
for d in $XM_SEARCH_PATH
do
	done=False

	##
	## Look for libXm.a
	##
	if [ -f $d/lib/$XM_ARCHIVE_LIB_NAME ]
	then
		done=True

		XM_ARCHIVE_DIR=$d/lib

		XM_ARCHIVE_LIB=$XM_ARCHIVE_DIR/$XM_ARCHIVE_LIB_NAME
	fi

	##
	## Look for libXm.so
	##
	if [ -x $d/lib/$XM_SHARED_LIB_NAME ]
	then
		done=True

		XM_SHARED_DIR=$d/lib

		XM_SHARED_LIB=$XM_SHARED_DIR/$XM_SHARED_LIB_NAME
	fi

	if [ "$done" = "True" ]
	then
		break
	fi
done

#echo "XM_ARCHIVE_LIB = $XM_ARCHIVE_LIB"
#echo "XM_SHARED_LIB = $XM_SHARED_LIB"
#echo "XM_ARCHIVE_DIR = $XM_ARCHIVE_DIR"
#echo "XM_SHARED_DIR = $XM_SHARED_DIR"

##
## Generate the dummy test program
##
cat << EOF > test_motif.c
#include <stdio.h>
#include <Xm/Xm.h>

int
main(int argc,char ** argv)
{
	char * lesstif =
#ifdef LESSTIF_VERSION
	"True"
#else
	"False"
#endif
	;

    /* XmVERSION:XmREVISION:XmUPDATE_LEVEL:XmVERSION_STRING:IsLesstif */
	fprintf(stdout,"%d:%d:%d:%s:%s\n",
		XmVERSION,
		XmREVISION,
		XmUPDATE_LEVEL,
		XmVERSION_STRING,
		lesstif);

	return 0;
}
EOF

##
## Make sure code was created
##
if [ ! -f $XM_SRC ]
then
	echo
	echo "Could not create test program source $XM_SRC."
	echo

	xm_cleanup
	exit 1
fi

##
## Set flags needed to Compile the dummy test program
##
XM_FLAGS=-I$XM_INCLUDE_DIR

##
## Compile the dummy test program
##
cc $XM_FLAGS -o $XM_PROG $XM_SRC

##
## Make sure it compiled
##
if [ ! -x $XM_PROG ]
then
	echo
	echo "Could not create test program $XM_PROG."
	echo

	xm_cleanup
	exit 1
fi

##
## Execute the dummy test program
##
XM_PROG_OUTPUT=`$XM_PROG`

##
## Output has the following format:
##
## 1         2          3              4                5
## XmVERSION:XmREVISION:XmUPDATE_LEVEL:XmVERSION_STRING:IsLesstif
##

##
## -t | --is-lesstif
##
if [ "$XM_IS_LESSTIF" = "True" ]
then
	echo $XM_PROG_OUTPUT | awk -F":" '{ print $5; }'

	xm_cleanup

	exit 0
fi

##
## -i | --print-include-dir
##
if [ "$XM_PRINT_INCLUDE_DIR" = "True" ]
then
	echo $XM_INCLUDE_DIR

	xm_cleanup

	exit 0
fi

##
## -l | --print-lib-dir
##
if [ "$XM_PRINT_LIB_DIR" = "True" ]
then

	##
	## Print the shared path first, archive path second.  Most of the 
	## time (always ?) these will be the same.  The shared path is 
	## checked first, because lesstif by default does not install 
	## static libs.
	##
	if [ -d $XM_SHARED_DIR ]
	then
		echo $XM_SHARED_DIR
	else
		echo $XM_ARCHIVE_DIR
	fi

	xm_cleanup

	exit 0
fi

num=

##
## -v | --print-version
##
if [ "$XM_PRINT_VERSION" = "True" ]
then
	num=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $1; }'`

	##
	## -r | --print-revision
	##
	if [ "$XM_PRINT_REVISION" = "True" ]
	then
		num="$num".
		num="$num"`echo $XM_PROG_OUTPUT | awk -F":" '{ print $2; }'`

		##
		## -u | --print-update-level
		##
		if [ "$XM_PRINT_UPDATE_LEVEL" = "True" ]
		then
			num="$num".
			num="$num"`echo $XM_PROG_OUTPUT | awk -F":" '{ print $3; }'`
		fi
	fi

	echo $num

	xm_cleanup

	exit 0
fi


##
## -s | --print-version-string
##
if [ "$XM_PRINT_VERSION_STRING" = "True" ]
then
	echo $XM_PROG_OUTPUT | awk -F":" '{ print $4; }'

	xm_cleanup

	exit 0
fi

##
## Default: Print XmVERSION.XmREVISION
##
echo `echo $XM_PROG_OUTPUT | awk -F":" '{ print $1; }'`.`echo $XM_PROG_OUTPUT | awk -F":" '{ print $2; }'`

xm_cleanup
exit 0
