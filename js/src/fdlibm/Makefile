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
# The Initial Developer of this code under the NPL is Sun Microsystems,
# Inc.  Portions created by Netscape are
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

DEPTH           = ../../..

include $(DEPTH)/config/config.mk

#
# Default IEEE libm
#
CFLAGS          += -D_IEEE_LIBM

LIBRARY_NAME	= fdm

MODULE		= js

CSRCS		=	\
	k_standard.c 	\
	k_rem_pio2.c 	\
	k_cos.c		\
	k_sin.c		\
	k_tan.c 	\
	e_acos.c	\
	e_acosh.c	\
	e_asin.c	\
	e_atan2.c	\
	e_atanh.c	\
	e_cosh.c	\
	e_exp.c		\
	e_fmod.c	\
	e_gamma.c	\
	e_gamma_r.c	\
	e_hypot.c	\
	e_j0.c		\
	e_j1.c		\
	e_jn.c		\
	e_lgamma.c	\
	e_lgamma_r.c	\
	e_log.c		\
	e_log10.c	\
	e_pow.c		\
	e_rem_pio2.c	\
	e_remainder.c	\
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

EXPORTS	= fdlibm.h

include $(DEPTH)/config/rules.mk

# from mozilla/js/src/Makefile
ifeq ($(CPU_ARCH),sparc)

ifndef JS_NO_ULTRA
ULTRA_OPTIONS := -xarch=v8plus,-DULTRA_SPARC
ULTRA_OPTIONSCC := -DULTRA_SPARC
else
ULTRA_OPTIONS := -xarch=v8
ULTRA_OPTIONSCC :=
endif

ifeq ($(shell uname -m),sun4u)
ASFLAGS	+= -Wa,$(ULTRA_OPTIONS),-P,-L,-D_ASM,-D__STDC__=0 $(ULTRA_OPTIONSCC)
else
ASFLAGS	+= -Wa,-xarch=v8,-P,-L,-D_ASM,-D__STDC__=0
endif

endif # sparc

ifeq ($(OS_ARCH), OSF1)
LDFLAGS += -lc_r
endif
ifeq ($(OS_ARCH), SunOS)
LDFLAGS += -lposix4 -ldl -lnsl -lsocket
endif
ifeq ($(OS_ARCH), Linux)
LDFLAGS += -ldl
endif
# end from mozilla/js/src/Makefile
