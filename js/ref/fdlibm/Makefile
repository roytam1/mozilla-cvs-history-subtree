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
#  @(#)Makefile 1.4 95/01/18 
# 
#  ====================================================
#  Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
# 
#  Developed at SunSoft, a Sun Microsystems, Inc. business.
#  Permission to use, copy, modify, and distribute this
#  software is freely granted, provided that this notice 
#  is preserved.
#  ====================================================
# 
# 

#
# There are two options in making libm at fdlibm compile time:
#       _IEEE_LIBM      --- IEEE libm; smaller, and somewhat faster
#       _MULTI_LIBM     --- Support multi-standard at runtime by 
#                           imposing wrapper functions defined in 
#                           fdlibm.h:
#                               _IEEE_MODE      -- IEEE
#                               _XOPEN_MODE     -- X/OPEN
#                               _POSIX_MODE     -- POSIX/ANSI
#                               _SVID3_MODE     -- SVID
#
# Here is how to set up CFLAGS to create the desired libm at 
# compile time:
#
#       CFLAGS = -D_IEEE_LIBM           ... IEEE libm (recommended)
#       CFLAGS = -D_SVID3_MODE  ... Multi-standard supported
#                                           libm with SVID as the 
#                                           default standard
#       CFLAGS = -D_XOPEN_MODE  ... Multi-standard supported
#                                           libm with XOPEN as the 
#                                           default standard
#       CFLAGS = -D_POSIX_MODE  ... Multi-standard supported
#                                           libm with POSIX as the 
#                                           default standard
#       CFLAGS =                        ... Multi-standard supported
#                                           libm with IEEE as the 
#                                           default standard
# 
# NOTE: if scalb's second arguement is an int, then one must
# define _SCALB_INT in CFLAGS. The default prototype of scalb
# is double scalb(double, double)
#

ifdef BUILD_OPT
OPTIMIZER  = -O
DEFINES    := -UDEBUG -DNDEBUG -UDEBUG_$(shell whoami)
OBJDIR_TAG = _OPT
else
OPTIMIZER  = -g
DEFINES    := -DDEBUG -UNDEBUG -DDEBUG_$(shell whoami)
OBJDIR_TAG = _DBG
endif

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
OS_CONFIG       := $(OS_ARCH)$(OS_OBJTYPE)$(OS_RELEASE)
endif

# Name of the binary code directories
OBJDIR          = $(OS_CONFIG)$(OBJDIR_TAG).OBJ
VPATH           = $(OBJDIR)

include config/$(OS_CONFIG).mk

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir $(@D); fi
endef

# Look in OBJDIR to find prcpucfg.h
INCLUDES   = -I$(OBJDIR)

#
# Default IEEE libm
#
CFLAGS          = -DXP_UNIX $(OPTIMIZER) $(OS_CFLAGS) $(DEFINES) $(INCLUDES) \
                  -DJSFILE $(XCFLAGS) -D_IEEE_LIBM



#CC = cc

INCFILES = fdlibm.h
.INIT: $(INCFILES)
.KEEP_STATE:
src =   k_standard.c k_rem_pio2.c \
	k_cos.c k_sin.c k_tan.c \
	e_acos.c e_acosh.c e_asin.c e_atan2.c \
	e_atanh.c e_cosh.c e_exp.c e_fmod.c \
	e_gamma.c e_gamma_r.c e_hypot.c e_j0.c \
	e_j1.c e_jn.c e_lgamma.c e_lgamma_r.c \
	e_log.c e_log10.c e_pow.c e_rem_pio2.c e_remainder.c \
	e_scalb.c e_sinh.c e_sqrt.c \
	w_acos.c w_acosh.c w_asin.c w_atan2.c \
	w_atanh.c w_cosh.c w_exp.c w_fmod.c \
	w_gamma.c w_gamma_r.c w_hypot.c w_j0.c \
	w_j1.c w_jn.c w_lgamma.c w_lgamma_r.c \
	w_log.c w_log10.c w_pow.c w_remainder.c \
	w_scalb.c w_sinh.c w_sqrt.c \
	s_asinh.c s_atan.c s_cbrt.c s_ceil.c s_copysign.c \
	s_cos.c s_erf.c s_expm1.c s_fabs.c s_finite.c s_floor.c \
	s_frexp.c s_ilogb.c s_isnan.c s_ldexp.c s_lib_version.c \
	s_log1p.c s_logb.c s_matherr.c s_modf.c s_nextafter.c \
	s_rint.c s_scalbn.c s_signgam.c s_significand.c s_sin.c \
	s_tan.c s_tanh.c

ifdef USE_MSVC
obj = $(addprefix $(OBJDIR)/, $(src:.c=.obj))
else
obj = $(addprefix $(OBJDIR)/, $(src:.c=.o))
endif

all: libfdm.a 

$(OBJDIR)/%: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ $(CFLAGS) $*.c $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $*.c

$(OBJDIR)/%.o: %.s
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) $*.s

libfdm.a : $(obj) 
	ar cru $(OBJDIR)/libfdm.a $(obj)
	ranlib $(OBJDIR)/libfdm.a

source: $(src) README

clean: 
	/bin/rm -f $(obj) $(OBJDIR)/a.out $(OBJDIR)/libfdm.a
