#
# GNU Makefile for libprldap
#

LDAP_SRC = ../..
MCOM_ROOT = ../../../..

NOSTDCLEAN=true # don't let nsconfig.mk define target clean
NOSTDSTRIP=true # don't let nsconfig.mk define target strip
NSPR20=true	# probably should be defined somewhere else (not sure where)

OBJDEST = $(OBJDIR)/lib/libprldap
LIBDIR = $(LDAP_LIBDIR)

include $(MCOM_ROOT)/c-sdk/nsdefs.mk
include $(MCOM_ROOT)/c-sdk/nsconfig.mk
include $(LDAP_SRC)/nsldap.mk

# variable definitions for exported symbols
ifeq ($(ARCH), WINNT)
	PRLDAP_EXPORT_DEFS=../msdos/winsock/nsldappr32.def
endif

ifeq ($(USE_DLL_EXPORTS_FILE), 1)
	PRLDAP_EXPORT_DEFS=$(OBJDEST)/libprldap.exp
endif 
PRLDAP_EXPORT_FLAGS=$(addprefix $(DLLEXPORTS_PREFIX), $(PRLDAP_EXPORT_DEFS))


# recursive gmake rule to create exported symbols file
ifeq ($(USE_DLL_EXPORTS_FILE), 1)
$(OBJDEST)/libprldap.exp:	libprldap.ex $(OTHERDEFDEPS)
	$(GENEXPORTS) $(ARCH) Standard $< $(GENEXPARGS) > $@
endif


CFLAGS +=	$(SLCFLAGS)

PROBJDEST = $(OBJDIR)/lib/libprldap

LIBPRLDAP_OBJS=	ldappr-dns.o ldappr-error.o ldappr-public.o \
			ldappr-io.o ldappr-threads.o

ifeq ($(ARCH), AIX)
    EXTRA_LIBS += $(LIBNSPR) $(LDAP_SDK_LIBLDAP_DLL)
else
ifneq ($(ARCH), OSF1)
ifeq ($(ARCH), WINNT)
    EXTRA_LIBS = $(LIBNSPR)  $(LDAP_SDK_LIBLDAP_DLL)
else
    EXTRA_LIBS = $(DYNAMICNSPR) $(LDAP_SDK_LIBLDAP_DLL)
endif
endif
endif


OBJS = $(addprefix $(PROBJDEST)/, $(LIBPRLDAP_OBJS))
ifeq ($(ARCH), WINNT)
    EXTRA_LIBS += wsock32.lib
endif


ifndef INCLUDE_SSL
# do not build library if INCLUDE_SSL is defined (because the prldap
#	objects are included in libldapssl when INCLUDE_SSL is defined).
#
LIBPRLDAP=	$(addprefix $(LIBDIR)/, $(LIBPRLDAP_LIB).$(DLL_SUFFIX))
endif

clientSDK: all

all:	$(OBJDEST) $(LIBDIR) $(OBJS) $(LIBPRLDAP)

$(LIBDIR):
	$(MKDIR) $(LIBDIR)

$(LIBPRLDAP): $(OBJS) $(PRLDAP_EXPORT_DEFS)
	@echo ======= making $(LIBPRLDAP)
ifdef SO_FILES_TO_REMOVE
	-$(RM) $(SO_FILES_TO_REMOVE)
endif
	$(LINK_DLL) $(PRLDAP_EXPORT_FLAGS) $(EXTRA_LIBS)

veryclean: clean

clean:
	$(RM) $(OBJS) $(LIBPRLDAP)

$(OBJDEST):
	$(MKDIR) $(OBJDEST)

FORCE:
