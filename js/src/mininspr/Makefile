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
# JSRef GNUmake makefile for nini-NSPR
#

# path to js/src
SRCDEPTH = ..

# Set os+release dependent make variables
OS_ARCH         := $(subst /,_,$(shell uname -s))

# Attempt to differentiate between SunOS 5.4 and x86 5.4
OS_CPUARCH      := $(shell uname -m)
ifeq ($(OS_CPUARCH),i86pc)
OS_RELEASE      := $(shell uname -r)_$(OS_CPUARCH)
else
ifeq ($(OS_ARCH),AIX)
OS_RELEASE      := $(shell uname -v).$(shell uname -r)
else
OS_RELEASE      := $(shell uname -r)
endif
endif

# Virtually all Linux versions are identical.
# Any distinctions are handled in linux.h
ifeq ($(OS_ARCH),Linux)
OS_CONFIG      := Linux_All
else
ifeq ($(OS_ARCH),dgux)
OS_CONFIG      := dgux
else
OS_CONFIG       := $(OS_ARCH)$(OS_OBJTYPE)$(OS_RELEASE)
endif
endif

include ../config/$(OS_CONFIG).mk

DEFINES += -DMINI_NSPR -DNSPR20

ifdef BUILD_OPT
OPTIMIZER  += -O
DEFINES    += -UDEBUG -DNDEBUG -UDEBUG_$(shell whoami)
OBJDIR_TAG = _OPT
else
ifdef USE_MSVC
OPTIMIZER  = -Zi
else
OPTIMIZER  = -g
endif
DEFINES    += -DDEBUG -DDEBUG_$(shell whoami)
OBJDIR_TAG = _DBG
endif

# Name of the binary code directories
OBJDIR          = $(OS_CONFIG)$(OBJDIR_TAG).OBJ
VPATH           = $(OBJDIR)

# Automatic make dependencies file
DEPENDENCIES    = $(OBJDIR)/.md

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir $(@D); fi
endef

#
# XCFLAGS may be set in the environment or on the gmake command line
#
CFLAGS          = $(OPTIMIZER) $(OS_CFLAGS) $(DEFINES) $(INCLUDES) \
                  -DJSFILE $(XCFLAGS)
LDFLAGS		= -lm $(XLDFLAGS)

#
# NSPR file lists
#

LIB_CFILES =		\
	prdtoa.c	\
	plhash.c	\
	prlog2.c	\
	prlong.c	\
	prprf.c		\
	prstubs.c	\
	$(NULL)

ifdef USE_MSVC
LIB_OBJS  = $(addprefix $(OBJDIR)/, $(LIB_CFILES:.c=.obj))
else
LIB_OBJS  = $(addprefix $(OBJDIR)/, $(LIB_CFILES:.c=.o))
endif

CFILES = $(LIB_CFILES)
OBJS   = $(LIB_OBJS)

ifdef USE_MSVC
LIBRARY = $(OBJDIR)/mininspr.dll
else
LIBRARY = $(OBJDIR)/libmininspr.a
endif

ifdef USE_MSVC
TARGETS = $(LIBRARY)
else
TARGETS = $(LIBRARY)
endif

all: $(TARGETS)

clean:
	rm -rf $(OBJS)

clobber:
	rm -rf $(OBJS) $(TARGETS) $(DEPENDENCIES)

depend:
	gcc -MM $(CFLAGS) $(LIB_CFILES)

$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $*.c

# windows only
$(OBJDIR)/%.obj: %.c
	@$(MAKE_OBJDIR)
	$(CC) -Fo$(OBJDIR)/ -c $(CFLAGS) $*.c

ifeq ($(OS_ARCH),OS2)
$(LIBRARY): $(LIB_OBJS)
	$(AR) $@ $? $(AR_OS2_SUFFIX)
	$(RANLIB) $@
else
ifdef USE_MSVC
$(LIBRARY): $(LIB_OBJS)
	link.exe $(LIB_LINK_FLAGS) /base:0x61000000 \
	    /out:"$@" /pdb:"$(OBJDIR)/js32.pdb" /implib:"$(OBJDIR)/js32.lib" $?
else
$(LIBRARY): $(LIB_OBJS)
	$(AR) rv $@ $?
	$(RANLIB) $@
endif
endif

SUFFIXES: .i
%.i: %.c
	$(CC) -C -E $(CFLAGS) $< > $*.i

