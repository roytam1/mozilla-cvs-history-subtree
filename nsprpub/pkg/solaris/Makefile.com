#
# Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"$Id$"
#

MACH = $(shell mach)

PUBLISH_ROOT = $(DIST)
ifeq ($(MOD_DEPTH),../..)
ROOT = ROOT
else
ROOT = $(subst ../../,,$(MOD_DEPTH))/ROOT
endif

PKGARCHIVE = $(PUBLISH_ROOT)/pkgarchive
DATAFILES = copyright
FILES = $(DATAFILES) pkginfo prototype

PACKAGE = $(shell basename `pwd`)

PRODUCT_VERSION = "$(MOD_VERSION).$(MOD_MINOR).$(MOD_PATCH)$(MOD_BETA)"
LN = /usr/bin/ln

CLOBBERFILES = $(FILES)

include $(MOD_DEPTH)/config/rules.mk

# vim: ft=make
