# 
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#  
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#  
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation. Portions created by Netscape are
# Copyright (C) 1998-1999 Netscape Communications Corporation. All
# Rights Reserved.
# 
# Contributor(s): 
#

NSPR_TREE = .
MOD_DEPTH = .

include $(NSPR_TREE)/config/rules.mk
include build.mk

MMDD	= $(shell date +%m.%d)
INSTDIR = ../../dist/$(MMDD)/$(OBJDIR_NAME)
LIBDIR  = ../../dist/$(OBJDIR_NAME)/lib
INCDIR  = ../../dist/public/ldap
BINDIR  = ../../dist/$(OBJDIR_NAME)/bin
ETCDIR  = ../../dist/$(OBJDIR_NAME)/etc
EXPDIR  = ldap/examples
DOCDIR  = ldap/docs


all::   FORCE
	$(NSINSTALL) -D $(INSTDIR)

	@echo "Installing libraries"
	$(NSINSTALL) -D $(INSTDIR)/lib
ifeq ($(OS_ARCH), WINNT)
	$(NSINSTALL) $(LIBDIR)/$(LDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(SSLDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(PRLDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(LBER_LIBNAME).lib $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(LDIF_LIBNAME).lib $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(IUTIL_LIBNAME).lib $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(NSS_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(SSL_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(PLC_BASENAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(PLDS_BASENAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(NSPR_BASENAME).* $(INSTDIR)/lib
else
	$(NSINSTALL) $(LIBDIR)/lib$(LDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(SSLDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(PRLDAP_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(LBER_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(LDIF_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(IUTIL_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(NSS_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(SSL_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(PLC_BASENAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(PLDS_BASENAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/$(NSPR_BASENAME).* $(INSTDIR)/lib
ifneq ($(USE_64), 1)
ifeq ($(OS_ARCH), SunOS)
	$(NSINSTALL) $(LIBDIR)/lib$(HYBRID_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(PURE32_LIBNAME).* $(INSTDIR)/lib
endif
ifeq ($(OS_ARCH), HP-UX)
	$(NSINSTALL) $(LIBDIR)/lib$(HYBRID_LIBNAME).* $(INSTDIR)/lib
	$(NSINSTALL) $(LIBDIR)/lib$(PURE32_LIBNAME).* $(INSTDIR)/lib
endif
endif
endif
	@echo "Installing tools"
	$(NSINSTALL) -D $(INSTDIR)/tools
	$(NSINSTALL) $(BINDIR)/ldapsearch$(EXE_SUFFIX) $(INSTDIR)/tools
	$(NSINSTALL) $(BINDIR)/ldapdelete$(EXE_SUFFIX) $(INSTDIR)/tools
	$(NSINSTALL) $(BINDIR)/ldapmodify$(EXE_SUFFIX) $(INSTDIR)/tools
	$(NSINSTALL) $(BINDIR)/ldapcmp$(EXE_SUFFIX) $(INSTDIR)/tools

	@echo "Installing includes"
	$(NSINSTALL) -D $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/disptmpl.h $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/iutil.h $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/lber.h $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/ldap.h $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/ldap_ssl.h $(INSTDIR)/include
	$(NSINSTALL) $(INCDIR)/srchpref.h $(INSTDIR)/include
		
	@echo "Installing etc files"
	$(NSINSTALL) -D $(INSTDIR)/etc
	$(NSINSTALL) $(ETCDIR)/ldapfilter.conf $(INSTDIR)/etc
	$(NSINSTALL) $(ETCDIR)/ldapfriendly $(INSTDIR)/etc
	$(NSINSTALL) $(ETCDIR)/ldapsearchprefs.conf $(INSTDIR)/etc
	$(NSINSTALL) $(ETCDIR)/ldaptemplates.conf $(INSTDIR)/etc

	@echo "Installing example files"
	$(NSINSTALL) -D $(INSTDIR)/examples
	$(NSINSTALL) $(EXPDIR)/*.c $(INSTDIR)/examples
	$(NSINSTALL) $(EXPDIR)/*.h $(INSTDIR)/examples
	$(NSINSTALL) $(EXPDIR)/README $(INSTDIR)/examples
	$(NSINSTALL) $(EXPDIR)/Makefile $(INSTDIR)/examples
	$(NSINSTALL) $(EXPDIR)/xmplflt.conf $(INSTDIR)/examples

	@echo "Installing doc files"
	$(NSINSTALL) -D $(INSTDIR)/docs
	$(NSINSTALL) $(DOCDIR)/README $(INSTDIR)
	$(NSINSTALL) $(DOCDIR)/redist.txt $(INSTDIR)
	$(NSINSTALL) $(DOCDIR)/*.htm $(INSTDIR)
	$(NSINSTALL) $(DOCDIR)/*.gif $(INSTDIR)
	$(NSINSTALL) $(DOCDIR)/README $(INSTDIR)/docs
	$(NSINSTALL) $(DOCDIR)/redist.txt $(INSTDIR)/docs

ifdef BUILD_SHIP
	@echo "Copying files to $(BUILD_SHIP) directory"
	cp -r $(INSTDIR) $(BUILD_SHIP)
endif
	
clean:: FORCE
	@echo "Cleaning up old install"
	$(RM) -rf $(INSTDIR)

FORCE::
