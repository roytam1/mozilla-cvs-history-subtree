#! gmake
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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

#
# 
#
# You need to have checked out the LDAP tree at the same level as ns
# in order to build LDAP.
#

LDAP_DEPTH	= .
NSPR_TREE	= ../nsprpub
MOD_DEPTH	= ../nsprpub
DEPTH		= ..

include $(NSPR_TREE)/config/rules.mk

all export::	FORCE
	@if [ -d $(NSPR_TREE)/config ]; then \
		cd $(NSPR_TREE)/config; \
		$(MAKE); \
	else \
		echo "No NSPR directory found"; \
		exit 0; \
	fi
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) export; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi

libs install::	FORCE
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) install; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi

clean clobber::	FORCE
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) clean; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi
	rm -rf ../dist 

realclean clobber_all::	FORCE
	@if [ -d $(LDAP_DEPTH)/c-sdk/ldap ]; then \
		cd $(LDAP_DEPTH)/c-sdk/ldap; \
		$(MAKE) -f Makefile.client $(MFLAGS) realclean; \
	else \
		echo "No LDAP directory -- skipping"; \
		exit 0; \
	fi

FORCE:
