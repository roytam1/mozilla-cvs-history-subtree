#//------------------------------------------------------------------------
#//
#// Makefile to build OJI
#//
#//------------------------------------------------------------------------

DEPTH        = ../../..

include $(DEPTH)/config/config.mk

MODULE		= ojiimpl

ifdef MOZ_OJI
LIBRARY_NAME	= oji

CPPSRCS         = jvmmgr.cpp \
                  scd.cpp  \
                  nsJVMManager.cpp	 \
                  nsJVMPluginTagInfo.cpp \
                  ProxyJNI.cpp \
                  nsCSecurityContext.cpp
                  lcglue.cpp 

REQUIRES	= img java js lay layer plugin plugimpl pref style util xpcom raptor oji caps
LCFLAGS += -DJSJDLL=\"$(JSJDLL)\" 
INCLUDES += 		-I$(DIST)/include/private

endif # MOZ_OJI

EXPORTS         = jvmmgr.h	 \
		  nsJVMManager.h \
		  nsJVMPluginTagInfo.h

include $(DEPTH)/config/rules.mk

