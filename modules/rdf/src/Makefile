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

#
# 
#

DEPTH		= ../../..
MODULE		= rdf
LIBRARY_NAME	= $(LITE_PREFIX)rdf
LIBXP		= $(DIST)/lib/libxp.$(LIB_SUFFIX)

REQUIRES	= nspr 

JNI_GEN		= netscape.rdf.core.NativeRDF netscape.rdf.core.NativeRDFEnumeration

CSRCS		= \
		  vocab.c \
		  mcf.c \
		  remstore.c \
		  utils.c \
		  rdfparse.c \
		  bmk2mcf.c \
		  rdfht.c \
		  columns.c \
		  $(NULL)

LOCAL_INCLUDES	= -I$(PUBLIC)/rdf

include $(DEPTH)/config/rules.mk

$(LIBRARY): $(OBJS)
