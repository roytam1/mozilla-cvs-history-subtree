# 
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code is the Netscape Portable Runtime (NSPR).
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are 
# Copyright (C) 1998-2000 Netscape Communications Corporation.  All
# Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
# 

#
# Configuration common to all versions of Windows NT
# and Windows 95.
#

#
# Client build: make sure we use the shmsdos.exe under $(MOZ_TOOLS).
# $(MOZ_TOOLS_FLIPPED) is $(MOZ_TOOLS) with all the backslashes
# flipped, so that gmake won't interpret them as escape characters.
#
ifdef PR_CLIENT_BUILD_WINDOWS
SHELL := $(MOZ_TOOLS_FLIPPED)/bin/shmsdos.exe
endif

CC = cl
CCC = cl
LINK = link
AR = lib -NOLOGO -OUT:"$@"
RANLIB = echo
BSDECHO = echo
STRIP = echo
NSINSTALL = nsinstall
INSTALL	= $(NSINSTALL)
define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); $(NSINSTALL) -D $(@D); fi
endef
RC = rc.exe

GARBAGE = $(OBJDIR)/vc20.pdb $(OBJDIR)/vc40.pdb

XP_DEFINE = -DXP_PC
OBJ_SUFFIX = obj
LIB_SUFFIX = lib
DLL_SUFFIX = dll

OS_CFLAGS = -W3 -nologo -GF -Gy

ifdef BUILD_OPT
ifeq ($(OS_TARGET),WINCE)
OS_CFLAGS += -Zl
else
OS_CFLAGS += -MD
endif
OPTIMIZER = -O2
DEFINES = -UDEBUG -U_DEBUG -DNDEBUG
DLLFLAGS = -OUT:"$@"
OBJDIR_TAG = _OPT

# Add symbolic information for use by a profiler
ifdef MOZ_PROFILE
OPTIMIZER += -Z7
DLLFLAGS += -DEBUG -DEBUGTYPE:CV
LDFLAGS += -DEBUG -DEBUGTYPE:CV
endif

else
#
# Define USE_DEBUG_RTL if you want to use the debug runtime library
# (RTL) in the debug build
#
ifeq ($(OS_TARGET),WINCE)
OS_CFLAGS += -Zl
else
ifdef USE_DEBUG_RTL
OS_CFLAGS += -MDd
else
OS_CFLAGS += -MD
endif
endif
OPTIMIZER = -Od -Z7
#OPTIMIZER = -Zi -Fd$(OBJDIR)/ -Od
DEFINES = -DDEBUG -D_DEBUG -UNDEBUG

DLLFLAGS = -DEBUG -DEBUGTYPE:CV -OUT:"$@"
ifdef GLOWCODE
DLLFLAGS = -DEBUG -DEBUGTYPE:both -INCLUDE:_GlowCode -OUT:"$@"
endif

OBJDIR_TAG = _DBG
LDFLAGS = -DEBUG -DEBUGTYPE:CV
#
# When PROFILE=1 is defined, set the compile and link options
# to build targets for use by the ms-win32 profiler
#
ifdef PROFILE
LDFLAGS += -PROFILE -MAP
DLLFLAGS += -PROFILE -MAP
endif
endif

DEFINES += -DWIN32

#
# On Win95, we use the TlsXXX() interface by default because that
# allows us to load the NSPR DLL dynamically at run time.
# If you want to use static thread-local storage (TLS) for better
# performance, build the NSPR library with USE_STATIC_TLS=1.
#
ifeq ($(USE_STATIC_TLS),1)
DEFINES += -D_PR_USE_STATIC_TLS
endif

#
# NSPR uses both fibers and static thread-local storage
# (i.e., __declspec(thread) variables) on NT.  We need the -GT
# flag to turn off certain compiler optimizations so that fibers
# can use static TLS safely.
#
# Also, we optimize for Pentium (-G5) on NT.
#
ifeq ($(OS_TARGET),WINNT)
OS_CFLAGS += -GT
ifeq ($(CPU_ARCH),x86)
OS_CFLAGS += -G5
endif
DEFINES += -DWINNT
else
DEFINES += -D_PR_GLOBAL_THREADS_ONLY
ifeq ($(OS_TARGET),WINCE)
DEFINES += -DWINCE -D$(CPU_ARCH) -DUNICODE -D_UNICODE -DUNDER_CE=300 -D_WIN32_WCE=300
ifeq ($(CPU_ARCH),x86)
DEFINES += -D_WIN32_WCE_CEPC
endif
else
DEFINES += -DWIN95
endif
endif

ifeq ($(CPU_ARCH),x86)
DEFINES += -D_X86_
else
ifeq ($(CPU_ARCH),MIPS)
DEFINES += -D_MIPS_
else
ifeq ($(CPU_ARCH),ALPHA)
DEFINES += -D_ALPHA_=1
else
ifeq ($(CPU_ARCH),ARM)
DEFINES += -D_ARM_=1
else
CPU_ARCH = processor_is_undefined
endif
endif
endif
endif

# Name of the binary code directories

ifeq ($(CPU_ARCH),x86)
CPU_ARCH_TAG =
else
CPU_ARCH_TAG = $(CPU_ARCH)
endif

ifdef USE_DEBUG_RTL
OBJDIR_SUFFIX = OBJD
else
OBJDIR_SUFFIX = OBJ
endif

OBJDIR_NAME = $(OS_CONFIG)$(CPU_ARCH_TAG)$(OBJDIR_TAG).$(OBJDIR_SUFFIX)

ifeq ($(OS_TARGET),WINCE)
ifeq ($(TARGETCPU),X86)
OS_DLLFLAGS = -nologo -DLL -SUBSYSTEM:WINDOWSCE,3.00 -MACHINE:IX86 -ENTRY:_DllMainCRTStartup -STACK:0x10000,0x1000 -NODEFAULTLIB:OLDNAMES.lib
else
OS_DLLFLAGS = -nologo -DLL -SUBSYSTEM:WINDOWSCE,3.00 -MACHINE:ARM -ENTRY:_DllMainCRTStartup -STACK:0x10000,0x1000 -NODEFAULTLIB:OLDNAMES.lib
endif
else
OS_DLLFLAGS = -nologo -DLL -SUBSYSTEM:WINDOWS -PDB:NONE
endif