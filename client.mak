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

DEPTH=.

!if !defined(MOZ_TOP)
#enable builds from changed top level directories
MOZ_TOP=mozilla
!endif

MOZ_SRC_FLIPPED = $(MOZ_SRC:\=/)
MOZ_DIST_FLIPPED = $(MOZ_SRC_FLIPPED)/mozilla/dist
!ifdef MOZ_DEBUG
MOZ_OBJDIR = WIN32_D.OBJ
!else
MOZ_OBJDIR = WIN32_O.OBJ
!endif

#
# Command macro defines
#

#//------------------------------------------------------------------------
#// Figure out how to do the pull.
#//------------------------------------------------------------------------
# uncomment these, modify branch tag, and check in to branch for milestones

# pull the svg mini-branch:
MOZ_BRANCH=SVG_20010721_BRANCH
# we pull the head revision if a file is not tagged: 
!ifndef MOZ_CO_FLAGS
MOZ_CO_FLAGS=-f
!else
MOZ_CO_FLAGS= $(MOZ_CO_FLAGS) -f
!endif

# list of all tagged files on the svg mini-branch. we use it for the
# branch maintenance targets below
SVG_BRANCH_FILES = \
	content/svg \
	layout/svg \
	dom/public/idl/svg \
	\
	aclocal.m4 \
	allmakefiles.sh \
	build/autoconf/libart.m4 \
	build/mac/build_scripts/MozillaBuildList.pm \
	client.mk \
	client.mak \
	config/autoconf.mk.in \
	configure \
	configure.in \
	content/Makefile.in \
	content/base/public/nsIDocument.h \
	content/base/public/nsIElementFactory.h \
	content/base/src/nsStyleContext.cpp \
	content/build/Makefile.in \
	content/build/makefile.win \
	content/build/nsContentCID.h \
	content/build/nsContentDLF.cpp \
	content/build/nsContentModule.cpp \
	content/html/document/src/nsHTMLDocument.cpp \
	content/html/style/public/nsIRuleNode.h \
	content/html/style/src/nsCSSDeclaration.cpp \
	content/html/style/src/nsCSSParser.cpp \
	content/html/style/src/nsCSSStyleRule.cpp \
	content/html/style/src/nsICSSDeclaration.h \
	content/html/style/src/nsRuleNode.cpp \
	content/html/style/src/nsRuleNode.h \
	content/makefile.win \
	content/shared/public/MANIFEST \
	content/shared/public/Makefile.in \
	content/shared/public/makefile.win \
	content/shared/public/nsCSSKeywordList.h \
	content/shared/public/nsCSSPropList.h \
	content/shared/public/nsCSSProps.h \
	content/shared/public/nsSVGAtomList.h \
	content/shared/public/nsSVGAtoms.h \
	content/shared/public/nsStyleStruct.h \
	content/shared/src/Makefile.in \
	content/shared/src/makefile.win \
	content/shared/src/nsCSSProps.cpp \
	content/shared/src/nsSVGAtoms.cpp \
	content/shared/src/nsStyleStruct.cpp \
	content/shared/src/nsStyleUtil.cpp \
	content/xml/document/src/nsXMLDocument.cpp \
	content/xml/document/src/nsXMLDocument.h \
	dom/public/idl/Makefile.in \
	dom/public/idl/makefile.win \
	dom/public/nsIDOMClassInfo.h \
	dom/src/base/nsDOMClassInfo.cpp \
	gfx/public/nsTransform2D.h \
	htmlparser/public/nsIParser.h \
	htmlparser/src/nsExpatTokenizer.cpp \
	htmlparser/src/nsViewSourceHTML.cpp \
	layout/base/public/nsStyleConsts.h \
	layout/build/Makefile.in \
	layout/build/makefile.win \
	layout/html/style/src/Makefile.in \
	layout/html/style/src/makefile.win \
	layout/html/style/src/nsCSSFrameConstructor.cpp \
	layout/html/style/src/nsCSSFrameConstructor.h \
	layout/html/tests/makefile.win \
	netwerk/mime/src/nsXMLMIMEDataSource.cpp \
	uriloader/exthandler/nsExternalHelperAppService.cpp \
	xpfe/browser/src/nsBrowserInstance.cpp


#NSPR_CO_TAG=SeaMonkey_M17_BRANCH
#PSM_CO_TAG=SeaMonkey_M17_BRANCH
#NSS_CO_TAG=SeaMonkey_M17_BRANCH
#LDAPCSDK_CO_TAG=SeaMonkey_M17_BRANCH
#ACCESSIBLE_CO_TAG=SeaMonkey_M17_BRANCH
#IMGLIB2_CO_TAG=SeaMonkey_M17_BRANCH
#GFX2_CO_TAG=SeaMonkey_M17_BRANCH


!ifdef MOZ_BRANCH
CVS_BRANCH=-r $(MOZ_BRANCH)
HAVE_BRANCH=1
!else
HAVE_BRANCH=0
!endif

!ifdef MOZ_DATE
CVS_BRANCH=-D "$(MOZ_DATE)"
HAVE_DATE=1
!else
HAVE_DATE=0
!endif

!if $(HAVE_DATE) && $(HAVE_BRANCH)
ERR_MESSAGE=$(ERR_MESSAGE)^
Cannot specify both MOZ_BRANCH and MOZ_DATE
!endif

# default pull is "quiet" but it can be overridden with MOZ_CVS_VERBOSE
!ifndef MOZ_CVS_VERBOSE
CVS_FLAGS=-q
!endif

# honor any user-defined CVS flags
!ifdef MOZ_CVS_FLAGS
CVS_FLAGS=$(CVS_FLAGS) $(MOZ_CVS_FLAGS)
!endif

# let's be explicit about CVSROOT... some windows cvs clients
# are too stupid to correctly work without the -d option 
#
#  if they are too stupid, they should fail.  I am
#  commenting this out because this does not work
#  under 4nt.  (%'s are evaluted differently)
#
#  If it breaks you, mail dougt@netscape.com
#  and leaf@mozilla.org
#
!if 0
!if defined(CVSROOT)
CVS_FLAGS=$(CVS_FLAGS) -d "$(CVSROOT)"
!endif
!endif

!ifndef MOZ_CO_FLAGS
MOZ_CO_FLAGS = -P
!endif

CVSCO = cvs $(CVS_FLAGS) co $(MOZ_CO_FLAGS) $(CVS_BRANCH) $(CVS_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull NSPR.
#// If no NSPR_CO_TAG is specified, use the default static tag
#//------------------------------------------------------------------------

!ifndef NSPR_CO_FLAGS
NSPR_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(NSPR_CO_TAG)" != ""
NSPR_CO_FLAGS=$(NSPR_CO_FLAGS) -r $(NSPR_CO_TAG)
!else
NSPR_CO_FLAGS=$(NSPR_CO_FLAGS) -r NSPRPUB_PRE_4_2_CLIENT_BRANCH
!endif

CVSCO_NSPR = cvs $(CVS_FLAGS) co $(NSPR_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull NSS and PSM libs.
#// If no NSS_CO_TAG or PSM_CO_TAG is specified, use the default static tag
#//------------------------------------------------------------------------

!ifndef NSS_CO_FLAGS
NSS_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(NSS_CO_TAG)" != ""
NSS_CO_FLAGS=$(NSS_CO_FLAGS) -r $(NSS_CO_TAG)
!else
NSS_CO_FLAGS=$(NSS_CO_FLAGS) -r NSS_CLIENT_TAG
!endif

CVSCO_NSS = cvs $(CVS_FLAGS) co $(NSS_CO_FLAGS)

!ifndef PSM_CO_FLAGS
PSM_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(PSM_CO_TAG)" != ""
PSM_CO_FLAGS=$(PSM_CO_FLAGS) -r $(PSM_CO_TAG)
!else
PSM_CO_FLAGS=$(PSM_CO_FLAGS) $(CVS_BRANCH)
!endif

CVSCO_PSM = cvs $(CVS_FLAGS) co $(PSM_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull LDAP C SDK client libs.
#// If no LDAPCSDK_CO_TAG is specified, use the default tag
#//------------------------------------------------------------------------


!ifndef LDAPCSDK_CO_FLAGS
LDAPCSDK_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(LDAPCSDK_CO_TAG)" != ""
LDAPCSDK_CO_FLAGS=$(LDAPCSDK_CO_FLAGS) -r $(LDAPCSDK_CO_TAG)
!else
LDAPCSDK_CO_FLAGS=$(LDAPCSDK_CO_FLAGS) -r LDAPCSDK_40_BRANCH
!endif

CVSCO_LDAPCSDK = cvs $(CVS_FLAGS) co $(LDAPCSDK_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull accessibility libs.
#// If no ACCESSIBLE_CO_TAG is specified, use the default tag
#//------------------------------------------------------------------------

!ifndef ACCESSIBLE_CO_FLAGS
ACCESSIBLE_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(ACCESSIBLE_CO_TAG)" != ""
ACCESSIBLE_CO_FLAGS=$(ACCESSIBLE_CO_FLAGS) -r $(ACCESSIBLE_CO_TAG)
!else
ACCESSIBLE_CO_FLAGS=$(ACCESSIBLE_CO_FLAGS) $(CVS_BRANCH)
!endif

CVSCO_ACCESSIBLE = cvs $(CVS_FLAGS) co $(ACCESSIBLE_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull new image library.
#// If no IMGLIB2_CO_TAG is specified, use the default tag
#//------------------------------------------------------------------------

!ifndef IMGLIB2_CO_FLAGS
IMGLIB2_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(IMGLIB2_CO_TAG)" != ""
IMGLIB2_CO_FLAGS=$(IMGLIB2_CO_FLAGS) -r $(IMGLIB2_CO_TAG)
!else
IMGLIB2_CO_FLAGS=$(IMGLIB2_CO_FLAGS) $(CVS_BRANCH)
!endif

CVSCO_IMGLIB2 = cvs $(CVS_FLAGS) co $(IMGLIB2_CO_FLAGS)

#//------------------------------------------------------------------------
#// Figure out how to pull new image library.
#// If no GFX2_CO_TAG is specified, use the default tag
#//------------------------------------------------------------------------

!ifndef GFX2_CO_FLAGS
GFX2_CO_FLAGS=$(MOZ_CO_FLAGS)
!endif

!if "$(GFX2_CO_TAG)" != ""
GFX2_CO_FLAGS=$(GFX2_CO_FLAGS) -r $(GFX2_CO_TAG)
!else
GFX2_CO_FLAGS=$(GFX2_CO_FLAGS) $(CVS_BRANCH)
!endif

CVSCO_GFX2 = cvs $(CVS_FLAGS) co $(GFX2_CO_FLAGS)


## The master target
############################################################

pull_and_build_all: pull_all depend build_all


## Rules for pulling the source from the cvs repository
############################################################

pull_clobber_and_build_all: pull_all clobber_all build_all

pull_all: pull_nspr pull_psm pull_ldapcsdk pull_accessible pull_gfx2 pull_imglib2 pull_seamonkey

pull_nspr: pull_clientmak
      cd $(MOZ_SRC)\.
      $(CVSCO_NSPR) mozilla/nsprpub

pull_nss:
	cd $(MOZ_SRC)\.
	$(CVSCO_NSS) mozilla/security/coreconf
	$(CVSCO_NSS) mozilla/security/nss

pull_psm: pull_nss
	cd $(MOZ_SRC)\.
	$(CVSCO_PSM) mozilla/security/manager
	$(CVSCO_PSM) mozilla/security/makefile.win

pull_ldapcsdk:
	cd $(MOZ_SRC)\.
	$(CVSCO_LDAPCSDK) mozilla/directory/c-sdk

pull_accessible:
	cd $(MOZ_SRC)\.
	$(CVSCO_ACCESSIBLE) mozilla/accessible

pull_gfx2:
  cd $(MOZ_SRC)\.
  $(CVSCO_GFX2) mozilla/gfx2

pull_imglib2:
  cd $(MOZ_SRC)\.
  $(CVSCO_IMGLIB2) mozilla/modules/libpr0n

pull_xpconnect: pull_nspr
	cd $(MOZ_SRC)\.
	$(CVSCO) mozilla/include
	$(CVSCO) mozilla/config
	$(CVSCO) -l mozilla/js
	$(CVSCO) -l mozilla/js/src
	$(CVSCO) mozilla/js/src/fdlibm
	$(CVSCO) mozilla/js/src/xpconnect
	$(CVSCO) mozilla/modules/libreg
	$(CVSCO) mozilla/xpcom
	$(CVSCO) mozilla/string

# pull either layout only or seamonkey the browser
pull_layout:
	cd $(MOZ_SRC)\.
	$(CVSCO) RaptorWin

pull_seamonkey: pull_clientmak
	cd $(MOZ_SRC)\.
	$(CVSCO) SeaMonkeyAll

pull_clientmak:
    cd $(MOZ_SRC)\.
    $(CVSCO) mozilla/client.mak

############################################################

# nmake has to be hardcoded, or we have to depend on mozilla/config
# being pulled already to figure out what $(NMAKE) should be.

clobber_all: clobber_nspr clobber_psm clobber_seamonkey

build_all: build_nspr build_seamonkey

distclean: 
	@cd $(MOZ_SRC)\$(MOZ_TOP)\nsprpub
	gmake -f gmakefile.win distclean MOZ_SRC_FLIPPED=$(MOZ_SRC_FLIPPED)
	@cd $(MOZ_SRC)\$(MOZ_TOP)
	nmake /f client.mak clobber_psm
	nmake /f client.mak clobber_seamonkey

clobber_nspr: 
	@cd $(MOZ_SRC)\$(MOZ_TOP)\nsprpub
	gmake -f gmakefile.win clobber_all MOZ_SRC_FLIPPED=$(MOZ_SRC_FLIPPED)

clobber_psm:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\security
	nmake -f makefile.win clobber_all

clobber_xpconnect:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	-rd /s /q dist
	set DIST_DIRS=1
	@cd $(MOZ_SRC)\$(MOZ_TOP)\nsprpub
	gmake -f gmakefile.win clobber_all  MOZ_SRC_FLIPPED=$(MOZ_SRC_FLIPPED)
	@cd $(MOZ_SRC)\$(MOZ_TOP)\include
	nmake -f makefile.win clobber_all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\modules\libreg
	nmake -f makefile.win clobber_all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\string
	nmake -f makefile.win clobber_all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\xpcom
	nmake -f makefile.win clobber_all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\js
	nmake -f makefile.win clobber_all

clobber_seamonkey:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	-rd /s /q dist
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake -f makefile.win clobber_all 

depend: export
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake -f makefile.win depend 

depend_xpconnect:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\include
	nmake -f makefile.win depend
	@cd $(MOZ_SRC)\$(MOZ_TOP)\modules\libreg
	nmake -f makefile.win depend
	@cd $(MOZ_SRC)\$(MOZ_TOP)\string
	nmake -f makefile.win depend
	@cd $(MOZ_SRC)\$(MOZ_TOP)\xpcom
	nmake -f makefile.win depend
	@cd $(MOZ_SRC)\$(MOZ_TOP)\js\src
	nmake -f makefile.win depend

build_nspr: 
	@cd $(MOZ_SRC)\$(MOZ_TOP)\nsprpub
	gmake -f gmakefile.win MOZ_SRC_FLIPPED=$(MOZ_SRC_FLIPPED)

build_psm:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\security
	nmake -f makefile.win 

build_xpconnect: build_nspr
	@cd $(MOZ_SRC)\$(MOZ_TOP)\include
	nmake -f makefile.win all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\modules\libreg
	nmake -f makefile.win all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\xpcom
	nmake -f makefile.win export
	@cd $(MOZ_SRC)\$(MOZ_TOP)\string
	nmake -f makefile.win all
	@cd $(MOZ_SRC)\$(MOZ_TOP)\xpcom
	nmake -f makefile.win install
	@cd $(MOZ_SRC)\$(MOZ_TOP)\js\src
	nmake -f makefile.win all

build_seamonkey:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake -f makefile.win all

build_client:
	@cd $(MOZ_SRC)\mozilla\.
	set CLIENT_DIRS=1
	nmake -f makefile.win all

build_layout:
	@cd $(MOZ_SRC)\mozilla\.
	set LAYOUT_DIRS=1
	nmake -f makefile.win all

build_dist:
	@cd $(MOZ_SRC)\mozilla\.
	set DIST_DIRS=1
	nmake -f makefile.win all

install:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake -f makefile.win install

export:
	@cd $(MOZ_SRC)\$(MOZ_TOP)\nsprpub
	gmake -f gmakefile.win MOZ_SRC_FLIPPED=$(MOZ_SRC_FLIPPED)
	@cd $(MOZ_SRC)\$(MOZ_TOP)\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake -f makefile.win export

clobber_dist:
	@cd $(MOZ_SRC)\mozilla\.
	set DIST_DIRS=1
	nmake -f makefile.win clobber_all

clobber_client:
	@cd $(MOZ_SRC)\mozilla\.
	set CLIENT_DIRS=1
	nmake -f makefile.win clobber_all

clobber_layout:
	@cd $(MOZ_SRC)\mozilla\.
	set LAYOUT_DIRS=1
	nmake -f makefile.win clobber_all

browse_info::
	cd $(MOZ_SRC)\$(MOZ_TOP)
	-dir /s /b *.sbr > sbrlist.tmp
	-bscmake /Es /o mozilla.bsc @sbrlist.tmp
	-rm sbrlist.tmp

regchrome::
	@cd $(MOZ_SRC)\mozilla\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake /f makefile.win regchrome

deliver::
	@cd $(MOZ_SRC)\mozilla\.
	set DIST_DIRS=1
	set LAYOUT_DIRS=1
	set CLIENT_DIRS=1
	nmake /f makefile.win splitsymbols


# svg mini-branch maintenance targets:
merge:
	cvs -z3 up -dP -jSVG_20010721_TAG -jHEAD $(SVG_BRANCH_FILES)

statictag:
	cvs -z3 tag -F -rHEAD SVG_20010721_TAG $(SVG_BRANCH_FILES)

commitmerge:
	cvs -z3 ci $(SVG_BRANCH_FILES)

diffsvg:
	cvs -z3 diff -u $(SVG_BRANCH_FILES)

#//------------------------------------------------------------------------
#// Utility stuff...
#//------------------------------------------------------------------------

#//------------------------------------------------------------------------
# Verify that MOZ_SRC is set correctly
#//------------------------------------------------------------------------

# Check to see if it is set at all
!if "$(MOZ_SRC)"==""
MOZ_SRC = $(MAKEDIR)\..
!endif

#
# create a temp file at the root and make sure it is visible from MOZ_SRC
#
!if [copy $(MAKEDIR)\client.mak $(MAKEDIR)\xyzzy.tmp > NUL] == 0
!endif

!if !EXIST( $(MOZ_SRC)\mozilla\xyzzy.tmp )
ERR_MESSAGE=$(ERR_MESSAGE)^
MOZ_SRC isn't set correctly: [$(MOZ_SRC)\mozilla]!=[$(MAKEDIR)]
!endif

!if [del $(MAKEDIR)\xyzzy.tmp]
!endif

#//------------------------------------------------------------------------
# Verify that MOZ_BITS is set
#//------------------------------------------------------------------------
!if !defined(MOZ_BITS)
ERR_MESSAGE=$(ERR_MESSAGE)^
Environment variable MOZ_BITS isn't set.
!endif

!if !defined(MOZ_TOOLS)
ERR_MESSAGE=$(ERR_MESSAGE)^
Environment variable MOZ_TOOLS isn't set.
!endif


#//------------------------------------------------------------------------
#// Display error 
#//------------------------------------------------------------------------

!if "$(ERR_MESSAGE)" != ""
ERR_MESSAGE = ^
client.mak:  ^
$(ERR_MESSAGE) ^
^
client.mak: usage^
^
nmake -f client.mak [MOZ_BRANCH=<cvs_branch_name>] ^
		    [MOZ_DATE=<cvs_date>]^
		    [pull_and_build_all]^
		    [pull_all]^
		    [build_all]^
^
Environment variables:^
^
MOZ_BITS    set to 32^
MOZ_TOOLS   set to the directory containing the needed tools ^

!ERROR $(ERR_MESSAGE)

!endif
