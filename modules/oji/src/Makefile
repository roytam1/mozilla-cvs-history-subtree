#//------------------------------------------------------------------------
#//
#// Makefile to build OJI
#//
#//------------------------------------------------------------------------

DEPTH        = ../../..

MODULE		= ojiimpl

ifdef MOZ_OJI
LIBRARY_NAME	= oji

CPPSRCS         = jvmmgr.cpp \
                  scd.cpp

REQUIRES	= img java js lay layer plug pref style util xpcom oji
endif # MOZ_OJI

EXPORTS         = jvmmgr.h

include $(DEPTH)/config/rules.mk

