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

CSDKVERSION = 5.05
FROMDIR = /share/builds/components/ldapsdk50/v$(CSDKVERSION)
TODIR	= /u/svbld/mhein/ldapcsdk-$(CSDKVERSION)
BUILDDIRS = AIX4.3_DBG.OBJ AIX4.3_OPT.OBJ HP-UXB.11.00_64_DBG.OBJ \
	    HP-UXB.11.00_64_OPT.OBJ HP-UXB.11.00_DBG.OBJ HP-UXB.11.00_OPT.OBJ \
	    Linux2.2_x86_glibc_PTH_DBG.OBJ Linux2.2_x86_glibc_PTH_OPT.OBJ \
	    OSF1V4.0D_DBG.OBJ OSF1V4.0D_OPT.OBJ SunOS5.6_DBG.OBJ \
	    SunOS5.6_OPT.OBJ SunOS5.8_64_DBG.OBJ SunOS5.8_64_OPT.OBJ \
	    SunOS5.8_DBG.OBJ SunOS5.8_OPT.OBJ SunOS5.8_i86pc_DBG.OBJ \
	    SunOS5.8_i86pc_OPT.OBJ WIN954.0_DBG.OBJ WIN954.0_OPT.OBJ \
	    WINNT4.0_DBG.OBJ WINNT4.0_OPT.OBJ

all::   FORCE
	@for i in $(BUILDDIRS); do \
	  cd $(FROMDIR)/$$i ; \
	  zip -r $(TODIR)/ldapcsdk$(CSDKVERSION)-$$i.zip * ; \
	  zip -d $(TODIR)/ldapcsdk$(CSDKVERSION)-$$i.zip include-private/liblber/* ; \
	  zip -d $(TODIR)/ldapcsdk$(CSDKVERSION)-$$i.zip include-private/* ; \
	  zip -d $(TODIR)/ldapcsdk$(CSDKVERSION)-$$i.zip include-private ; \
	done

clean:: FORCE
	@echo "Cleaning up old package"
	rm -rf $(TODIR)/*

FORCE::
