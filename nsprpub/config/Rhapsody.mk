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
# Config stuff for Rhapsody5.0
#

include $(MOD_DEPTH)/config/UNIX.mk

#
# The default implementation strategy for Rhapsody is classic nspr.
#
ifeq ($(USE_CTHREADS),1)
IMPL_STRATEGY = _CTH
DEFINES += -D_PR_GLOBAL_THREADS_ONLY -D_PR_NO_CTHREAD_KEY_T
else
DEFINES += -D_PR_LOCAL_THREADS_ONLY
endif

DEFINES			+= -D_PR_NEED_FAKE_POLL


CC			= cc
CCC			= cc++
RANLIB			= ranlib

OS_REL_CFLAGS		= -Dppc
CPU_ARCH		= ppc

#OS_REL_CFLAGS		= -mno-486 -Di386
#CPU_ARCH		= x86

OS_CFLAGS		= $(DSO_CFLAGS) $(OS_REL_CFLAGS) -pipe -DRHAPSODY -DHAVE_STRERROR -DHAVE_BSD_FLOCK

ARCH			= rhapsody

#DSO_CFLAGS		= -fPIC
#DSO_LDOPTS		= -Bshareable
#DSO_LDFLAGS		=

MKSHLIB			= $(CC) -arch ppc -dynamiclib -compatibility_version 1 -current_version 1
DLL_SUFFIX		= dylib
