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

LDAP_DEPTH	= .
NSPR_TREE	= ../nsprpub
NSS_TREE	= ../security
MOD_DEPTH	= ../nsprpub
CORECONFDIR	= ../coreconf
NSPR_VERSION	= 4.1
NSS_VERSION	= 3.2
DEPTH		= ..
CONFIGDIR	= c-sdk/config

all:	FORCE
	@echo
	@echo The following are build targets that you can choose from:
	@echo
	@echo "   gmake buildAndPkgLdapSDK"
	@echo "   gmake buildLdapClientSDK"
	@echo "   gmake pkgLdapSDK"
	@echo "   gmake clean"
	@echo
	@echo Allowed options:
	@echo 
	@echo "   BUILD_OPT=1		for optimized build"
	@echo "   USE_PTHREADS=1	for using pthreads"
	@echo "   NS_USE_GCC=1		for using gcc"
	@echo "   BUILD_CLU=1		build command line utilities"
	@echo 
	@echo iPlanet internal options
	@echo
	@echo "   HAVE_LIBNLS=1	for building/linking with LIBNLS"
	@echo "   HAVE_CCONF=1		for building with components"
	@echo "   HAVE_SVRCORE=1	for building with svrcore"
	@echo

buildLdapClientSDK export: pullcomponents
	@echo
	@echo ==== Starting iPlanet LDAP Client SDK ==========
	@echo
ifneq ($(HAVE_CCONF), 1)
	@if [ -d $(NSPR_TREE)/ ]; then \
		echo "NSPR $(NSPR_VERSION) directory found"; \
	else \
		echo "No NSPR $(NSPR_VERSION) directory found"; \
	fi
	@if [  -d $(NSS_TREE)/ ]; then \
		echo "NSS $(NSS_VERSION) directory found"; \
	else \
		echo "No NSS $(NSS_VERSION) directory found"; \
	fi
endif
	@if [ -d $(CONFIGDIR)/ ]; then \
		cd $(CONFIGDIR); \
		gmake; \
	else \
		echo "No NSS $(CONFIGDIR) directory found"; \
	fi
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) export; \
		$(MAKE) -f Makefile.client $(MFLAGS) install; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi

pkgLdapSDK: FORCE
	@echo
	@echo "Still working on"
	@echo


buildAndPkgLdapSDK: buildLdapClientSDK pkgLdapSDK

pullcomponents: FORCE
ifeq ($(HAVE_CCONF), 1)
	@if [  -d $(CORECONFDIR)/ ]; then \
		echo "coreconf directory found"; \
	else \
		echo "No coreconf directory found"; \
		exit 0; \
	fi
  ifeq ($(ARCH), WINNT)
	cd $(CORECONFDIR)/dbm; \
	$(MAKE) VERSION=$(DBM_RELEASE_TAG)     $(MFLAGS) USE_FTP=YES import
	cd $(CORECONFDIR)/nspr20; \
	$(MAKE) VERSION=$(NSPR_RELEASE_TAG)    $(MFLAGS) USE_FTP=YES import
	cd $(CORECONFDIR)/security; \
	$(MAKE) VERSION=$(NSS_RELEASE_TAG)     $(MFLAGS) USE_FTP=YES import
	cd $(CORECONFDIR)/svrcore; \
	$(MAKE) VERSION=$(SVRCORE_RELEASE_TAG) $(MFLAGS) USE_FTP=YES import
  else
	cd $(CORECONFDIR)/dbm; \
	$(MAKE) VERSION=$(DBM_RELEASE_TAG)     $(MFLAGS) import
	cd $(CORECONFDIR)/nspr20; \
	$(MAKE) VERSION=$(NSPR_RELEASE_TAG)    $(MFLAGS) import
	cd $(CORECONFDIR)/security; \
	$(MAKE) VERSION=$(NSS_RELEASE_TAG)     $(MFLAGS) import
	cd $(CORECONFDIR)/svrcore; \
	$(MAKE) VERSION=$(SVRCORE_RELEASE_TAG) $(MFLAGS) import
  endif
endif

clean clobber:	FORCE
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) clean; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi

FORCE:
