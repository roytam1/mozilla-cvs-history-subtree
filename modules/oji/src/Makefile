#//------------------------------------------------------------------------
#//
#// Makefile to build OJI
#//
#//------------------------------------------------------------------------

DEPTH        = ../../..

MODULE		= oji

ifdef MOZ_OJI
LIBRARY_NAME	= oji

CPPSRCS         = jvmmgr.cpp \
                  scd.cpp

REQUIRES	= img java js lay layer plug pref style util xpcom
endif # MOZ_OJI

EXPORTS         = nsIJVM.h jvmmgr.h nsscd.h

include $(DEPTH)/config/rules.mk

