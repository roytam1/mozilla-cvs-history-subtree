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

######################################################################
# Common Config stuff for OS/2
######################################################################
SHELL	= GBASH.EXE

# Specify toolset.  Default to EMX.
ifeq ($(MOZ_OS2_TOOLS),VACPP)
XP_OS2_VACPP = 1
else
MOZ_OS2_TOOLS = EMX
XP_OS2_EMX   = 1
endif

OBJDIR_TAG := $(addprefix _$(MOZ_OS2_TOOLS), $(OBJDIR_TAG))


######################################################################
# Overrides of stuff in config.mk
######################################################################
EMPTY		:=
SLASH		:= /$(EMPTY)
BSLASH		:= \$(EMPTY)
SEMICOLON	:= ;$(EMPTY)
SPACE		:= $(EMPTY) $(EMPTY)
PATH_SEPARATOR	:= \;
XP_DEFINE	= -DXP_PC
LIB_SUFFIX	= lib
DLL_SUFFIX	= dll
MAP_SUFFIX	= map
BIN_SUFFIX	= .exe
NSINSTALL	= nsinstall
INSTALL		= $(NSINSTALL)
JAVA_PROG	= java -norestart
JAVAC_ZIP	= $(subst $(BSLASH),$(SLASH),$(JAVA_HOME))/lib/classes.zip
RANLIB		= echo
EMACS           = emacs
RM              = rm -rf
WHOAMI      = echo $(HOSTNAME)


ifdef XP_OS2_EMX
######################################################################
# These are for emx/gcc
######################################################################

CCC		= $(CC)
LINK		= $(CC)

# Determine which object format to use.  Two choices:
# a.out and omf.  We default to a.out.
ifeq ($(MOZ_OS2_EMX_OBJECTFORMAT), OMF)
OMF_FLAG 	= -Zomf
AR		= emxomfar -p64 r $@
LIB_SUFFIX	= lib
else
AR      	= ar -q $@
LIB_SUFFIX	= a
endif

PLATFORM_FLAGS	= $(OMF_FLAG) -ansi -Wall -Zmtd -DXP_OS2 -DXP_OS2_FIX -DXP_OS2_EMX -DOS2
MOVEMAIL_FLAGS	=
PORT_FLAGS		= -DNEED_GETOPT_H -DHAVE_SIGNED_CHAR

OS_CFLAGS		= $(PLATFORM_FLAGS) $(PORT_FLAGS) $(MOVEMAIL_FLAGS)
OS_LIBS     		= -lsocket -lemxio
OS_DLLFLAGS 		= $(OMF_FLAG) -Zmap -Zmt -Zdll -Zcrtdll -o $@

MKSHLIB			= $(LD) $(DSO_LDOPTS)
RC 			= rc.exe
FILTER  		= emxexp
IMPLIB  		= emximp -o

ifdef BUILD_OPT
OPTIMIZER		= -O3
DLLFLAGS		= 
else
OPTIMIZER		= -g
DLLFLAGS		= -g -L$(DIST)/lib -o $@
endif


######################################################################
# end XP_OS2_EMX
######################################################################
endif

ifdef XP_OS2_VACPP
######################################################################
# These are for VisualAge C++
######################################################################
CC		= icc
CCC		= $(CC)
LINK		= ilink

ifdef MAKE_DLL
DLL_FLAGS = -ge-
endif

ifdef BUILD_OPT
OPTIMIZER	= -O+ -Oi
else
OPTIMIZER   = -Ti+
endif

PLATFORM_FLAGS	= $(DLL_FLAGS) -q -W3 -DOS2::4 -DXP_OS2 -DXP_OS2_FIX -DXP_OS2_VACPP -N10 -D_X86_
PLATFORM_FLAGS += -Gm -Gd+ -Su4 -Ss -I. -I$(DEPTH)/config/os2
MOVEMAIL_FLAGS	=
PORT_FLAGS	= -DHAVE_SIGNED_CHAR

OS_CFLAGS	= $(PLATFORM_FLAGS) $(PORT_FLAGS) $(MOVEMAIL_FLAGS)
OS_LFLAGS	= /PM:VIO /NOLOGO
OS_DLLFLAGS	= -FREE -NOE -nologo -DLL
OS_LIBS		= $(SOCKLIB) tcp32dll.lib cppom30o.lib os2386.lib

RC		= rc$(BIN_SUFFIX)
AR		= ilib /noignorecase /nologo $(subst /,\\,$@)
IMPLIB		= implib -nologo -noignorecase
DLLFLAGS	= -OUT:$@ -MAP:$(@:.dll=.map)
LFLAGS		= -OUT:$@ $(XLFLAGS) $(DEPLIBS) $(EXTRA_LIBS) -MAP:$(@:.dll=.map) $(DEF_FILE)
AR_EXTRA_ARGS   = ,,
MKSHLIB		= ilink
FILTER		= cppfilt -q -B -P
LIB_SUFFIX      = lib

######################################################################
# End XP_OS2_VACPP
######################################################################
endif
