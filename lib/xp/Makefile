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

DEPTH		= ../..

MODULE		= xp
LIBRARY_NAME	= xp

DEFINES		+= -B1M

CSRCS		= \
		  xp_stub.c \
		  xp_error.c \
		  xp_file.c \
		  allxpstr.c \
		  $(NULL)

REQUIRES	= js nspr dbm security img util jtools layer java lay style pref rdf

include $(DEPTH)/config/rules.mk

EMACS		= lemacs

AUDIO_FILES	= $(wildcard *.au)
GIF_FILES	= $(wildcard *.gif)
JPEG_FILES	= $(wildcard *.jpg)

GEN_OUT		= $(AUDIO_FILES:.au=.h) $(GIF_FILES:.gif=.h) $(JPEG_FILES:.jpg=.h)

#
# Uncomment these to (re)generate the headers.
#
#GARBAGE		+= $(GEN_OUT) $(wildcard *~)
#
#all:: $(GEN_OUT)

$(LIBRARY): $(OBJS)

#
# Generate c header's for various binary files
#
.SUFFIXES: .au .gif .jpg

.au.h:
	@echo generating $*.h from $<...; \
	CNAME=`echo $*_au`; \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c $$CNAME $< $*.h

.gif.h:
	@echo generating $*.h from $<...; \
	CNAME=`echo $*_gif`; \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c $$CNAME $< $*.h

.jpg.h:
	@echo generating $*.h from $<...; \
	CNAME=`echo $*_jpg`; \
	$(EMACS) -batch -q -l bin-to-c.el -f batch-bin-to-c $$CNAME $< $*.h
