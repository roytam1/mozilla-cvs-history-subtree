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

##
## Command Line Flags Supported:
##
##  -l | --is-lesstif:				Prints True/False if usign lesstif.
##
##  -v | --print-version:			Prints XmVERSION.
##  -r | --print-revision:			Prints XmREVISION.
##  -u | --print-update-level:		Prints XmUPDATE_LEVEL.
##  -s | --print-version-string:	Prints XmVERSION_STRING.
##
##  -id | --print-include-dir:		Prints dir dir of motif includes.
##  -sd | --print-static-dir:		Prints dir dir of motif static libs.
##  -dd | --print-dynamic-dir:		Prints dir dir of motif dynamic libs.
##
##  -if | --print-include-flags:	Prints cc flags needed to build motif apps.
##  -sf | --print-static-flags:		Prints ld flags for linking statically.
##  -df | --print-dynamic-flags:	Prints ld flags for linking dynamically.
##
##  -de | --dynamic-ext:			Set extension used on dynamic libs.
##  -se | --static-ext:				Set extension used on static libs.
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
XM_PROG=./test_motif
XM_SRC=test_motif.c

##
## Option defaults
##
XM_DYNAMIC_EXT=so
XM_STATIC_EXT=a

XM_IS_LESSTIF=False

XM_PRINT_VERSION=False
XM_PRINT_REVISION=False
XM_PRINT_UPDATE_LEVEL=False

XM_PRINT_INCLUDE_DIR=False
XM_PRINT_STATIC_DIR=False
XM_PRINT_DYNAMIC_DIR=False

XM_PRINT_INCLUDE_FLAGS=False
XM_PRINT_STATIC_FLAGS=False
XM_PRINT_DYNAMIC_FLAGS=False

XM_PRINT_EVERYTHING=False

##
## Stuff we need to figure out
##
XM_VERSION_RESULT=
XM_REVISION_RESULT=
XM_UPDATE_RESULT=
XM_VERSION_REVISION_RESULT=
XM_VERSION_REVISION_UPDATE_RESULT=
XM_VERSION_STRING_RESULT=
XM_IS_LESSTIF_RESULT=

XM_INCLUDE_DIR=
XM_LIB_DIR=

XM_STATIC_LIB=
XM_DYNAMIC_LIB=

XM_STATIC_DIR=
XM_DYNAMIC_DIR=

XM_INCLUDE_FLAGS=not_found
XM_STATIC_FLAGS=not_found
XM_DYNAMIC_FLAGS=not_found

function test_motif_usage()
{
echo
echo "Usage:   `basename $0` [options]"
echo
echo "  -l,  --is-lesstif:            Prints {True,False} if using lesstif."
echo
echo "  -v,  --print-version:         Prints XmVERSION."
echo "  -r,  --print-revision:        Prints XmREVISION."
echo "  -u,  --print-update-level:    Prints XmUPDATE_LEVEL."
echo "  -s,  --print-version-string:  Prints XmVERSION_STRING."
echo
echo "  -id, --print-include-dir:     Prints dir of motif includes."
echo "  -sd, --print-static-dir:      Prints dir of motif static libs."
echo "  -dd, --print-dynamic-dir:     Prints dir of motif dynamic libs."
echo
echo "  -if, --print-include-flags:   Prints cc flags needed to compile."
echo "  -sf, --print-static-flags:    Prints ld flags for linking statically."
echo "  -df, --print-dynamic-flags:   Prints ld flags for linking dynamically."
echo
echo "  -e, --print-everything:       Prints everything that is known."
echo
echo "  -de, --dynamic-ext:           Set extension used on dynamic libs."
echo "  -se, --static-ext:            Set extension used on static libs."
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
            test_motif_usage
			exit 0
            ;;

        -l | --is-lesstif)
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
XM_DYNAMIC_LIB_NAME=libXm.$XM_DYNAMIC_EXT
XM_STATIC_LIB_NAME=libXm.$XM_STATIC_EXT

##
## Cleanup the dummy test source/program
##
function xm_cleanup()
{
	rm -f $XM_PROG
	rm -f $XM_SRC
}

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
XM_INCLUDE_FLAGS=-I$XM_INCLUDE_DIR

##
## Compile the dummy test program
##
cc $XM_INCLUDE_FLAGS -o $XM_PROG $XM_SRC

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
## -l | --is-lesstif
##
if [ "$XM_IS_LESSTIF" = "True" ]
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
	echo "XM_VERSION:            $XM_VERSION_RESULT"
	echo "XM_REVISION:           $XM_REVISION_RESULT"
	echo "XM_UPDATE:             $XM_UPDATE_RESULT"
	echo "XM_VERSION_STRING:     $XM_VERSION_STRING_RESULT"
	echo "XM_IS_LESSTIF:         $XM_IS_LESSTIF_RESULT"
	echo "XM_INCLUDE_DIR:        $XM_INCLUDE_DIR"
	echo "XM_LIB_DIR:            $XM_LIB_DIR"
	echo "XM_STATIC_LIB:         $XM_STATIC_LIB"
	echo "XM_DYNAMIC_LIB:        $XM_DYNAMIC_LIB"
	echo "XM_STATIC_DIR:         $XM_STATIC_DIR"
	echo "XM_DYNAMIC_DIR:        $XM_DYNAMIC_DIR"
	echo "XM_INCLUDE_FLAGS:      $XM_INCLUDE_FLAGS"
	echo "XM_STATIC_FLAGS:       $XM_STATIC_FLAGS"
	echo "XM_DYNAMIC_FLAGS:      $XM_DYNAMIC_FLAGS"
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
