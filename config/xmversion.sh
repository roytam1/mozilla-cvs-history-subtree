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
## Name:		xmversion.sh - Get motif lib location, version and other info.
##
## Description:	Artificial intelligence to figure out:
##
##              + Where the motif headers/lib are located.
##              + Whether lesstif is being used.
##              + The version of motif being used.
##              + The compile and link flags needed to build motif apps.
##
## Author:		Ramiro Estrugo <ramiro@netscape.com>
##
##############################################################################


## This script looks in the space sepeared list of directories (XM_SEARCH_PATH)
## for motif headers and libraries.
##
## It also generates and builds a program 'get_motif_info-OBJECT_NAME'
## which prints out info on the motif detected on the system.
##
## This information is munged into useful strings that can be printed
## through the various command line flags decribed bellow.  This script
## can be invoked from Makefiles or other scripts in order to set
## flags needed to build motif program.
##
## The get 'get_motif_info-OBJECT_NAME' program is generated/built only
## once and reused.  Because of the OBJECT_NAME suffix, it will work on
## multiple platforms at the same time (same mozilla source tree)
##

##
## Command Line Flags Supported:
##
##  -l | --is-lesstif:				Print True/False if usign lesstif.
##
##  -v | --print-version:			Print XmVERSION.
##  -r | --print-revision:			Print XmREVISION.
##  -u | --print-update-level:		Print XmUPDATE_LEVEL.
##  -s | --print-version-string:	Print XmVERSION_STRING.
##
##  -id | --print-include-dir:		Print dir of motif includes.
##  -sd | --print-static-dir:		Print dir of motif static libs.
##  -dd | --print-dynamic-dir:		Print dir of motif dynamic libs.
##
##  -sl | --print-static-lib:		Print static lib.
##  -dl | --print-dynamic-lib:		Print dynamic lib.
##
##  -if | --print-include-flags:	Print cc flags needed to build motif apps.
##  -sf | --print-static-flags:		Print ld flags for linking statically.
##  -df | --print-dynamic-flags:	Print ld flags for linking dynamically.
##
##  -de | --dynamic-ext:			Set extension used on dynamic libs.
##  -se | --static-ext:				Set extension used on static libs.
##
##  -o  | --set-object-name:		Set object name for current system.
##  -cc | --set-compiler:			Set compiler for building test program.
##

##
## Look for motif stuff in the following places:
##
XM_SEARCH_PATH="\
/usr/lesstif \
/usr/local \
/usr/dt \
/usr/X11R6 \
/usr \
"

##
## Constants
##
XM_PROG_PREFIX=./get_motif_info

##
## Defaults
##
XM_DYNAMIC_EXT=so
XM_STATIC_EXT=a

XM_PRINT_IS_LESSTIF=False

XM_PRINT_VERSION=False
XM_PRINT_REVISION=False
XM_PRINT_UPDATE_LEVEL=False

XM_PRINT_INCLUDE_DIR=False
XM_PRINT_STATIC_DIR=False
XM_PRINT_DYNAMIC_DIR=False

XM_PRINT_STATIC_LIB=False
XM_PRINT_DYNAMIC_LIB=False

XM_PRINT_INCLUDE_FLAGS=False
XM_PRINT_STATIC_FLAGS=False
XM_PRINT_DYNAMIC_FLAGS=False

XM_PRINT_EVERYTHING=False

XM_OBJECT_NAME=`uname`-`uname -r`
XM_CC=cc

##
## Stuff we need to figure out
##
XM_VERSION_RESULT=unknown
XM_REVISION_RESULT=unknown
XM_UPDATE_RESULT=unknown
XM_VERSION_REVISION_RESULT=unknown
XM_VERSION_REVISION_UPDATE_RESULT=unknown
XM_VERSION_STRING_RESULT=unknown
XM_IS_LESSTIF_RESULT=unknown

XM_INCLUDE_DIR=unknown
XM_STATIC_LIB=unknown
XM_DYNAMIC_LIB=unknown

XM_STATIC_DIR=unknown
XM_DYNAMIC_DIR=unknown

XM_INCLUDE_FLAGS=unknown
XM_STATIC_FLAGS=unknown
XM_DYNAMIC_FLAGS=unknown

function xm_usage()
{
echo
echo "Usage:   `basename $0` [options]"
echo
echo "  -l,  --is-lesstif:            Print {True,False} if using lesstif."
echo
echo "  -v,  --print-version:         Print XmVERSION."
echo "  -r,  --print-revision:        Print XmREVISION."
echo "  -u,  --print-update-level:    Print XmUPDATE_LEVEL."
echo "  -s,  --print-version-string:  Print XmVERSION_STRING."
echo
echo "  -id, --print-include-dir:     Print dir of motif includes."
echo "  -sd, --print-static-dir:      Print dir of motif static libs."
echo "  -dd, --print-dynamic-dir:     Print dir of motif dynamic libs."
echo
echo "  -sl, --print-static-lib:      Print static lib."
echo "  -dl  --print-dynamic-lib:     Print dynamic lib."
echo
echo "  -if, --print-include-flags:   Print cc flags needed to compile."
echo "  -sf, --print-static-flags:    Print ld flags for linking statically."
echo "  -df, --print-dynamic-flags:   Print ld flags for linking dynamically."
echo
echo "  -e,  --print-everything:      Print everything that is known."
echo
echo "  -de, --dynamic-ext:           Set extension used on dynamic libs."
echo "  -se, --static-ext:            Set extension used on static libs."
echo
echo "  -o,  --set-object-name:       Set object name for current system."
echo "  -cc, --set-compiler:          Set compiler for building test program."
echo
echo "  -h,  --help:                  Print this blurb."
echo
echo "The default is '-v -r' if no options are given."
echo
}

##
## Parse the command line
##
while [ "$*" ]; do
    case $1 in
        -h | --help)
            shift
            xm_usage
			exit 0
            ;;

        -l | --is-lesstif)
            shift
            XM_PRINT_IS_LESSTIF=True
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

        -id | --print-include-dir)
            shift
            XM_PRINT_INCLUDE_DIR=True
            ;;

        -sd | --print-static-dir)
            shift
            XM_PRINT_STATIC_DIR=True
            ;;

        -dd | --print-dynamic-dir)
            shift
            XM_PRINT_DYNAMIC_DIR=True
            ;;

        -sl | --print-static-lib)
            shift
            XM_PRINT_STATIC_LIB=True
            ;;

        -dl | --print-dynamic-lib)
            shift
            XM_PRINT_DYNAMIC_LIB=True
            ;;

        -if | --print-include-flags)
            shift
            XM_PRINT_INCLUDE_FLAGS=True
            ;;

        -sf | --print-static-flags)
            shift
            XM_PRINT_STATIC_FLAGS=True
            ;;

        -df | --print-dynamic-flags)
            shift
            XM_PRINT_DYNAMIC_FLAGS=True
            ;;

        -e | --print-everything)
            shift
            XM_PRINT_EVERYTHING=True
            ;;

        -de | --dynamic-ext)
            shift
            XM_DYNAMIC_EXT="$1"
            shift
            ;;

        -se | --static-ext)
            shift
            XM_STATIC_EXT="$1"
            shift
            ;;

        -o | --set-object-name)
            shift
            XM_OBJECT_NAME="$1"
            shift
            ;;

        -cc | --set-compiler)
            shift
            XM_CC="$1"
            shift
            ;;

        -*)
            echo "`basename $0`: invalid option '$1'"
            shift
            xm_usage
			exit 0
            ;;
    esac
done

##
## Motif info program name
##
XM_PROG="$XM_PROG_PREFIX"_"$XM_OBJECT_NAME"
XM_SRC="$XM_PROG_PREFIX"_"$XM_OBJECT_NAME.c"

##
## The library names
##
XM_DYNAMIC_LIB_NAME=libXm.$XM_DYNAMIC_EXT
XM_STATIC_LIB_NAME=libXm.$XM_STATIC_EXT

##
## Cleanup the dummy test source/program
##
function xm_cleanup()
{
	true

#	rm -f $XM_PROG
#	rm -f $XM_SRC

}

xm_cleanup

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
## Generate the dummy test program if needed
##
if [ ! -f $XM_SRC ]
then
cat << EOF > $XM_SRC
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
fi

##
## Make sure code was created
##
if [ ! -f $XM_SRC ]
then
	echo
	echo "Could not create or read test program source $XM_SRC."
	echo

	xm_cleanup

	exit 1
fi

##
## Set flags needed to Compile the dummy test program
##
XM_INCLUDE_FLAGS=-I$XM_INCLUDE_DIR

##
## Compile the dummy test program if needed
##
if [ ! -x $XM_PROG ]
then
	$XM_CC $XM_INCLUDE_FLAGS -o $XM_PROG $XM_SRC
fi

##
## Make sure it compiled
##
if [ ! -x $XM_PROG ]
then
	echo
	echo "Could not create or execute test program $XM_PROG."
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
XM_VERSION_RESULT=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $1; }'`
XM_REVISION_RESULT=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $2; }'`
XM_UPDATE_RESULT=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $3; }'`
XM_VERSION_REVISION_RESULT=$XM_VERSION_RESULT.$XM_REVISION_RESULT
XM_VERSION_REVISION_UPDATE_RESULT=$XM_VERSION_REVISION_RESULT.$XM_UPDATE_RESULT
XM_VERSION_STRING_RESULT=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $4; }'`
XM_IS_LESSTIF_RESULT=`echo $XM_PROG_OUTPUT | awk -F":" '{ print $5; }'`

##
## There could be up to 4 dyanmic libs and/or links.
##
## libXm.so
## libXm.so.1
## libXm.so.1.2
## libXm.so.1.2.4
##
XM_DYNAMIC_SEARCH_PATH="\
$XM_DYNAMIC_LIB_NAME \
$XM_DYNAMIC_LIB_NAME.$XM_VERSION_RESULT \
$XM_DYNAMIC_LIB_NAME.$XM_VERSION_REVISION_RESULT \
$XM_DYNAMIC_LIB_NAME.$XM_VERSION_REVISION_UPDATE_RESULT \
"

##
## Look for static library
##
for d in $XM_SEARCH_PATH
do
	if [ -f $d/lib/$XM_STATIC_LIB_NAME ]
	then
		XM_STATIC_DIR=$d/lib

		XM_STATIC_LIB=$XM_STATIC_DIR/$XM_STATIC_LIB_NAME

		XM_STATIC_FLAGS=$XM_STATIC_LIB

		break
	fi
done


##
## Look for dyanmic libraries
##
for d in $XM_SEARCH_PATH
do
	for l in $XM_DYNAMIC_SEARCH_PATH
	do
		if [ -x $d/lib/$l ]
		then
			XM_DYNAMIC_DIR=$d/lib

			XM_DYNAMIC_LIB=$d/lib/$l

			XM_DYNAMIC_FLAGS="-L$XM_DYNAMIC_DIR -lXm"

			break 2
		fi
	done
done

##
## If the static library directory is different than the dynamic one, it 
## is possible that the system contains two incompatible installations of
## motif/lesstif.  For example, lesstif could be installed in /usr/lesstif
## and the real motif could be installed in /usr/X11R6.  This would cause
## outofhackage later in the build.
##
## Need to handle this one.  Maybe we should just ignore the motif static
## libs and just use the lesstif ones ?  This is probably what the "user"
## wants anyway.  For instance, a "user" could be testing whether mozilla
## works with lesstif without erasing the real motif libs.
##
## Also, by default the lesstif build system only creates dynamic libraries.
## So this problem will always exist when both motif and lesstif are installed
## in the system.



##
## -l | --is-lesstif
##
if [ "$XM_PRINT_IS_LESSTIF" = "True" ]
then
	echo $XM_IS_LESSTIF_RESULT

	xm_cleanup

	exit 0
fi

##
## -e | --print-everything
##
if [ "$XM_PRINT_EVERYTHING" = "True" ]
then
	echo
	echo "XmVERSION:          $XM_VERSION_RESULT"
	echo "XmREVISION:         $XM_REVISION_RESULT"
	echo "XmUPDATE_LEVEL:     $XM_UPDATE_RESULT"
	echo "XmVERSION_STRING:   $XM_VERSION_STRING_RESULT"
	echo
	echo "Lesstif ?:          $XM_IS_LESSTIF_RESULT"
	echo
	echo "Include Dir:        $XM_INCLUDE_DIR"
	echo "Static Lib Dir:     $XM_STATIC_DIR"
	echo "Dynamic Lib Dir:    $XM_DYNAMIC_DIR"
	echo
	echo "Static Lib:         $XM_STATIC_LIB"
	echo "Synamic Lib:        $XM_DYNAMIC_LIB"
	echo
	echo "Include Flags:      $XM_INCLUDE_FLAGS"
	echo "Static Flags:       $XM_STATIC_FLAGS"
	echo "dynamic Flags:      $XM_DYNAMIC_FLAGS"
	echo
	echo "OBJECT_NAME:        $XM_OBJECT_NAME"
	echo "Test Program:       $XM_PROG"
	echo

	xm_cleanup

	exit 0
fi

##
## -id | --print-include-dir
##
if [ "$XM_PRINT_INCLUDE_DIR" = "True" ]
then
	echo $XM_INCLUDE_DIR

	xm_cleanup

	exit 0
fi

##
## -dd | --print-dynamic-dir
##
if [ "$XM_PRINT_DYNAMIC_DIR" = "True" ]
then
	echo $XM_DYNAMIC_DIR

	xm_cleanup

	exit 0
fi

##
## -sd | --print-static-dir
##
if [ "$XM_PRINT_STATIC_DIR" = "True" ]
then
	echo $XM_STATIC_DIR

	xm_cleanup

	exit 0
fi

##
## -dl | --print-dynamic-lib
##
if [ "$XM_PRINT_DYNAMIC_LIB" = "True" ]
then
	echo $XM_DYNAMIC_LIB

	xm_cleanup

	exit 0
fi

##
## -sl | --print-static-lib
##
if [ "$XM_PRINT_STATIC_LIB" = "True" ]
then
	echo $XM_STATIC_LIB

	xm_cleanup

	exit 0
fi

##
## -if | --print-include-flags
##
if [ "$XM_PRINT_INCLUDE_FLAGS" = "True" ]
then
	echo $XM_INCLUDE_FLAGS

	xm_cleanup

	exit 0
fi

##
## -df | --print-dynamic-flags
##
if [ "$XM_PRINT_DYNAMIC_FLAGS" = "True" ]
then
	echo $XM_DYNAMIC_FLAGS

	xm_cleanup

	exit 0
fi

##
## -sf | --print-static-flags
##
if [ "$XM_PRINT_STATIC_FLAGS" = "True" ]
then
	echo $XM_STATIC_FLAGS

	xm_cleanup

	exit 0
fi


num=

##
## -v | --print-version
##
if [ "$XM_PRINT_VERSION" = "True" ]
then
	num=$XM_VERSION_RESULT

	##
	## -r | --print-revision
	##
	if [ "$XM_PRINT_REVISION" = "True" ]
	then
		num="$num".
		num="$num"$XM_REVISION_RESULT

		##
		## -u | --print-update-level
		##
		if [ "$XM_PRINT_UPDATE_LEVEL" = "True" ]
		then
			num="$num".
			num="$num"$XM_UPDATE_RESULT
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
	echo $XM_VERSION_STRING_RESULT

	xm_cleanup

	exit 0
fi

##
## Default: Print XmVERSION.XmREVISION
##
echo $XM_VERSION_RESULT.$XM_REVISION_RESULT

xm_cleanup

exit 0
