#! gmake
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

LIBRARY_NAME	= js

ifeq ($(subst /,_,$(shell uname -s)),OS2)
EXTRA_LIBS	= $(LIBNSPR)
MAKE_DLL	= 1
else
EXTRA_LIBS	= $(LIBNSPR) $(LIBNSJAVA) $(OBJDIR)/libjs.lib
endif

MODULE		= js

CSRCS		= prmjtime.c \
		  jsapi.c \
		  jsarray.c \
		  jsatom.c \
		  jsbool.c \
		  jscntxt.c \
		  jsdate.c \
		  jsdbgapi.c \
		  jsemit.c \
		  jsfun.c \
		  jsgc.c \
		  jsinterp.c \
		  jsmath.c \
		  jsnum.c \
		  jsobj.c \
		  jsopcode.c \
		  jsparse.c \
		  jsregexp.c \
		  jsscan.c \
		  jsscope.c \
		  jsscript.c \
		  jsstr.c \
		  jslock.c \
		  $(NULL)

EXPORTS		= jsapi.h \
		  jsarray.h \
		  jsatom.h \
		  jsbool.h \
		  jscntxt.h \
		  jscompat.h \
		  jsconfig.h \
		  jsdate.h \
		  jsdbgapi.h \
		  jsemit.h \
		  jsfun.h \
		  jsgc.h \
		  jsinterp.h \
		  jslock.h \
		  jsmath.h \
		  jsnum.h \
		  jsobj.h \
		  jsopcode.def \
		  jsopcode.h \
		  jsparse.h \
		  jsprvtd.h \
		  jspubtd.h \
		  jsregexp.h \
		  jsscan.h \
		  jsscope.h \
		  jsscript.h \
		  jsstr.h \
		  $(NULL)

include $(DEPTH)/config/rules.mk

export:: install

ifndef BUILD_OPT
MOCHAFILE	= 1
endif

ifdef JSFILE
DEFINES		+= -DJSFILE
endif

ifdef JS_THREADSAFE
DEFINES += -DJS_THREADSAFE
endif

ifdef JS_VERSION
DEFINES += -DJS_VERSION=$(JS_VERSION)
endif

INCLUDES	+= -I. 

ifdef NSPR20
INCLUDES	+= -I$(DIST)/include/nspr20/pr
else
INCLUDES	+= -I$(XPDIST)/public/nspr
endif

ifndef NSBUILDROOT
JSJAVA_STUBHEADERS = -I$(DEPTH)/sun-java/include/_gen \
		     -I$(DEPTH)/sun-java/netscape/javascript/_jri \
		     -I$(DEPTH)/sun-java/netscape/security/_jri
else
JSJAVA_STUBHEADERS = -I$(JRI_GEN_DIR) -I$(JDK_GEN_DIR)
endif
LDFLAGS		= $(DIST)/lib/libnspr.$(LIB_SUFFIX) -lm 

JSJAVA_CFLAGS	= -I$(DEPTH)/sun-java/md-include \
		  -I$(DEPTH)/sun-java/include \
		  $(JSJAVA_STUBHEADERS)

LDFLAGS		= $(DIST)/lib/libnspr.$(LIB_SUFFIX) -lm

ifeq ($(OS_ARCH), OSF1)
LDFLAGS += -lc_r
endif
ifeq ($(OS_ARCH), SunOS)
LDFLAGS += -lposix4 -ldl -lnsl -lsocket
endif

# this requires clobbering and recompiling with XCFLAGS=-DJSFILE
js:
	$(MAKE) clobber
	$(MAKE) XCFLAGS=-DJSFILE $(OBJDIR)/js$(BIN_SUFFIX)

.PHONY: js$(BIN_SUFFIX)

ifndef XP_OS2_VACPP
$(OBJDIR)/js$(BIN_SUFFIX): $(OBJDIR)/js.o $(LIBRARY)
	    @$(MAKE_OBJDIR)
	    $(CC) -o $@ $(OBJDIR)/js.o $(LIBRARY) $(LDFLAGS)
else
OS_CFLAGS += -tm-
$(OBJDIR)/js$(BIN_SUFFIX): $(OBJDIR)/js.o $(LIBRARY)
	    @$(MAKE_OBJDIR)
	    $(LINK_EXE) -OUT:$@ $(OBJDIR)/js.o  $(LIBRARIES) $(EXTRA_LIBS)
endif

# hardwire dependencies on jsopcode.def
jsopcode.h jsopcode.c: jsopcode.def


refdiff:
	@for f in `cat commfiles`; do                                         \
	    t=/tmp/refdiff.$$$$;                                              \
	    trap 'rm -f $$t' 0 1 2 15;                                        \
	    sed -f prconv.sed ../ref/$$f > $$t;                               \
	    cmp -s $$t $$f;                                                   \
	    if test $$? -ne 0; then                                           \
		echo "=== $$f";                                               \
		diff $$f $$t;                                                 \
	    fi;                                                               \
	    rm -f $$t;                                                        \
	done

refconv:
	@for f in `cat commfiles`; do                                         \
	    echo "=== $$f";                                                   \
	    sed -f prconv.sed ../ref/$$f > $$f;                               \
	done

.PHONY: refdiff refconv
