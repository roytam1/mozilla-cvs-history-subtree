#!gmake
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

DEPTH		= ../..

MODULE		= plug
LIBRARY_NAME	= plug

CSRCS		= npassoc.c

CPPSRCS		= nsplugin.cpp

EXPORTS		= nppg.h npupp.h nsplugin.h nsjvm.h npglue.h nppriv.h

REQUIRES	= lay layer js style applet nspr dbm img util java pref xpcom

ifeq ($(subst /,,$(shell uname -s)),OS2) 
# This doesn't compile yet! CPPSRCS		= npwplat.cpp
REQUIRES	+= xp libfont os2font jtools
endif

CSRCS		+= npglue.c

include $(DEPTH)/config/rules.mk

$(LIBRARY): $(OBJS)
