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

#
# Configuration common to all (supported) versions of OS/2
#
# OS_CFLAGS is the command line options for the compiler when
#   building the .DLL object files.
# OS_EXE_CFLAGS is the command line options for the compiler
#   when building the .EXE object files; this is for the test
#   programs.
# the macro OS_CFLAGS is set to OS_EXE_CFLAGS inside of the
#   makefile for the pr/tests directory. ... Hack.

# Specify toolset.  Default to EMX.
ifeq ($(MOZ_OS2_TOOLS),VACPP)
XP_OS2_VACPP = 1
else
MOZ_OS2_TOOLS = EMX
XP_OS2_EMX   = 1
endif

#
# On OS/2 we proudly support gbash...
#
SHELL = GBASH.EXE

CC			= icc -q -DXP_OS2 -N10
CCC			= icc -q -DXP_OS2 -DOS2=4 -N10
LINK			= ilink
AR			= ilib /noignorecase /nologo $(subst /,\\,$@)
RANLIB 			= @echo RANLIB
BSDECHO 		= @echo BSDECHO
NSINSTALL 		= nsinstall
INSTALL			= $(NSINSTALL)
MAKE_OBJDIR 		= if test ! -d $(OBJDIR); then mkdir $(OBJDIR); fi
IMPLIB 			= implib -nologo -noignorecase
FILTER 			= cppfilt -b -p -q
RC 			= rc.exe

GARBAGE =

XP_DEFINE 		= -DXP_PC
LIB_SUFFIX 		= lib
DLL_SUFFIX 		= dll

OS_CFLAGS     		= -I. -W3 -gm -gd+ -sd- -su4 -ge-
OS_EXE_CFLAGS 		= -I. -W3 -gm -gd+ -sd- -su4 
AR_EXTRA_ARGS 		= ,,

ifdef BUILD_OPT
OPTIMIZER		= -O+ -Oi 
DEFINES 		= -UDEBUG -U_DEBUG -DNDEBUG
DLLFLAGS		= -DLL -OUT:$@ -MAP:$(@:.dll=.map)
EXEFLAGS    		= -PMTYPE:VIO -OUT:$@ -MAP:$(@:.exe=.map) -nologo -NOE
OBJDIR_TAG 		= _OPT
else
OPTIMIZER		= -Ti+
DEFINES 		= -DDEBUG -D_DEBUG -UNDEBUG
DLLFLAGS		= -DEBUG -DLL -OUT:$@ -MAP:$(@:.dll=.map)
EXEFLAGS    		= -DEBUG -PMTYPE:VIO -OUT:$@ -MAP:$(@:.exe=.map) -nologo -NOE
OBJDIR_TAG 		= _DBG
LDFLAGS 		= -DEBUG 
endif

DEFINES += -DOS2=4 -DBSD_SELECT
DEFINES += -D_X86_
DEFINES += -D_PR_GLOBAL_THREADS_ONLY

# Name of the binary code directories
ifdef MOZ_LITE
OBJDIR_NAME 		= $(subst OS2,NAV,$(OS_CONFIG))_$(MOZ_OS2_TOOLS)$(OBJDIR_TAG).OBJ
else
OBJDIR_NAME 		= $(OS_CONFIG)_$(MOZ_OS2_TOOLS)$(OBJDIR_TAG).OBJ
endif

OS_DLLFLAGS 		= -nologo -DLL -FREE -NOE

ifdef XP_OS2_VACPP

OS_LIBS 		= so32dll.lib tcp32dll.lib

DEFINES += -DXP_OS2_VACPP

else
CC			= gcc
CCC			= gcc
LINK			= gcc
RC 			= rc.exe
FILTER  		= emxexp
IMPLIB  		= emximp -o

# Determine which object format to use.  Two choices:
# a.out and omf.  We default to a.out.
ifeq ($(MOZ_OS2_EMX_OBJECTFORMAT), OMF)
OMF_FLAG 	= -Zomf
AR		= emxomfar r $@
LIB_SUFFIX	= lib
else
AR      	= ar -q $@
LIB_SUFFIX	= a
endif

OS_LIBS     		= -lsocket -lemxio

DEFINES += -DXP_OS2_EMX

OS_CFLAGS     		= $(OMF_FLAG) -I. -Wall -Zmt
OS_EXE_CFLAGS 		= $(OMF_FLAG) -I. -Wall -Zmt
OS_DLLFLAGS 		= $(OMF_FLAG) -Zmt -Zdll -Zcrtdll -o $@

ifdef BUILD_OPT
OPTIMIZER		= -O3
DLLFLAGS		= 
EXEFLAGS    		= -Zmt -o $@
else
OPTIMIZER		= -g
DLLFLAGS		= -g
EXEFLAGS		= $(OMF_FLAG) -g -Zmt -L$(DIST)/lib -o $@
endif

AR_EXTRA_ARGS 		=
endif


