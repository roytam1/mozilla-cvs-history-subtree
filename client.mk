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
# Contributor(s): Stephen Lamm

# Build the Mozilla client.
#
# This needs CVSROOT set to work, e.g.,
#   setenv CVSROOT :pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot
# or
#   setenv CVSROOT :pserver:username%somedomain.org@cvs.mozilla.org:/cvsroot
# 
# To checkout and build a tree,
#    1. cvs co mozilla/client.mk
#    2. cd mozilla
#    3. gmake -f client.mk
#
# Other targets (gmake -f client.mk [targets...]),
#    checkout
#    build
#    clean (realclean is now the same as clean)
#    distclean
#
# See http://www.mozilla.org/build/unix.html for more information.
#
# Options:
#   MOZ_OBJDIR           - Destination object directory
#   MOZ_CO_DATE          - Date tag to use for checkout (default: none)
#   MOZ_CO_MODULE        - Module to checkout (default: SeaMonkeyAll)
#   MOZ_CVS_FLAGS        - Flags to pass cvs (default: -q -z3)
#   MOZ_CO_FLAGS         - Flags to pass after 'cvs co' (default: -P)
#   MOZ_MAKE_FLAGS       - Flags to pass to $(MAKE)
#   MOZ_CO_BRANCH        - Branch tag (Deprecated. Use MOZ_CO_TAG below.)
#

#######################################################################
# Checkout Tags
#
# For branches, uncomment the MOZ_CO_TAG line with the proper tag,
# and commit this file on that tag.

# pull the SVG mini-branch:
MOZ_CO_TAG = SVG_20020806_BRANCH
# Not all files are tagged.
# By adding '-f' to CVS_CO_FLAGS, below, we pull the head revision if 
# a file is not tagged.


# lists of all tagged files on the svg mini-branch. used for the
# branch maintenance targets below.

# list of modified files
SVG_BRANCH_MODIFIED_FILES = \
	allmakefiles.sh                                       \
	client.mk                                             \
	configure.in                                          \
	configure                                             \
	config/autoconf.mk.in                                 \
	content/base/public/nsIElementFactory.h               \
	content/base/public/nsContentCID.h                    \
	content/base/src/nsRuleNode.cpp                       \
	content/base/src/nsStyleContext.cpp                   \
	content/html/style/src/nsCSSDeclaration.cpp           \
	content/html/style/src/nsCSSDeclaration.h             \
	content/html/style/src/nsCSSParser.cpp                \
	content/html/style/src/nsCSSStyleRule.cpp             \
	content/html/style/src/nsCSSStruct.cpp                \
	content/html/style/src/nsCSSStruct.h                  \
	content/shared/public/nsCSSKeywordList.h              \
	content/shared/public/nsCSSPropList.h                 \
	content/shared/public/nsCSSProps.h                    \
	content/shared/public/nsRuleNode.h                    \
	content/shared/public/nsSVGAtomList.h                 \
	content/shared/public/nsStyleStruct.h                 \
	content/shared/public/nsStyleStructList.h             \
	content/shared/src/nsCSSProps.cpp                     \
	content/shared/src/nsStyleStruct.cpp                  \
	content/svg/content/src/Makefile.in                   \
	content/svg/content/src/nsISVGValue.h                 \
	content/svg/content/src/nsSVGAnimatedLength.cpp       \
	content/svg/content/src/nsSVGAttributes.cpp           \
	content/svg/content/src/nsSVGAttributes.h             \
	content/svg/content/src/nsSVGCircleElement.cpp        \
	content/svg/content/src/nsSVGElement.cpp              \
	content/svg/content/src/nsSVGElement.h                \
	content/svg/content/src/nsSVGElementFactory.cpp       \
	content/svg/content/src/nsSVGEllipseElement.cpp       \
	content/svg/content/src/nsSVGForeignObjectElement.cpp \
	content/svg/content/src/nsSVGGElement.cpp             \
	content/svg/content/src/nsSVGGraphicElement.cpp       \
	content/svg/content/src/nsSVGGraphicElement.h         \
	content/svg/content/src/nsSVGLength.cpp               \
	content/svg/content/src/nsSVGLength.h                 \
	content/svg/content/src/nsSVGLineElement.cpp          \
	content/svg/content/src/nsSVGMatrix.cpp               \
	content/svg/content/src/nsSVGMatrix.h                 \
	content/svg/content/src/nsSVGPathDataParser.cpp       \
	content/svg/content/src/nsSVGPathElement.cpp          \
	content/svg/content/src/nsSVGPathSeg.cpp              \
	content/svg/content/src/nsSVGPathSeg.h                \
	content/svg/content/src/nsSVGPathSegList.cpp          \
	content/svg/content/src/nsSVGPointList.cpp            \
	content/svg/content/src/nsSVGRect.cpp                 \
	content/svg/content/src/nsSVGRectElement.cpp          \
	content/svg/content/src/nsSVGSVGElement.cpp           \
	content/svg/content/src/nsSVGTransform.cpp            \
	content/svg/content/src/nsSVGTransformList.cpp        \
	content/svg/content/src/nsSVGValue.cpp                \
	content/svg/content/src/nsSVGValue.h                  \
	content/svg/document/src/nsSVGDocument.cpp            \
	content/svg/document/src/nsSVGDocument.h              \
	dom/public/nsIDOMClassInfo.h                          \
	dom/public/idl/svg/Makefile.in                        \
	dom/src/base/nsDOMClassInfo.cpp                       \
	gfx/src/windows/nsDrawingSurfaceWin.cpp               \
	htmlparser/src/CParserContext.cpp                     \
	htmlparser/src/CNavDTD.cpp                            \
	htmlparser/src/nsExpatDriver.cpp                      \
	layout/base/public/nsStyleConsts.h                    \
	layout/build/Makefile.in                              \
	layout/build/nsLayoutModule.cpp                       \
	layout/html/base/src/nsFrameManager.cpp               \
	layout/html/style/src/nsCSSFrameConstructor.cpp       \
	layout/svg/Makefile.in                                \
	layout/svg/base/src/Makefile.in                       \
	layout/svg/base/src/nsSVGCircleFrame.cpp              \
	layout/svg/base/src/nsSVGEllipseFrame.cpp             \
	layout/svg/base/src/nsSVGForeignObjectFrame.cpp       \
	layout/svg/base/src/nsSVGGFrame.cpp                   \
	layout/svg/base/src/nsSVGGenericContainerFrame.cpp    \
	layout/svg/base/src/nsSVGLineFrame.cpp                \
	layout/svg/base/src/nsSVGOuterSVGFrame.cpp            \
	layout/svg/base/src/nsSVGPathFrame.cpp                \
	layout/svg/base/src/nsSVGPolygonFrame.cpp             \
	layout/svg/base/src/nsSVGPolylineFrame.cpp            \
	layout/svg/base/src/nsSVGRectFrame.cpp                \
	layout/svg/base/src/svg.css                           \
	other-licenses/libart_lgpl/Makefile.in                \
	other-licenses/libart_lgpl/ChangeLog                  \
	other-licenses/libart_lgpl/art_affine.c               \
	other-licenses/libart_lgpl/art_alphagamma.c           \
	other-licenses/libart_lgpl/art_alphagamma.h           \
	other-licenses/libart_lgpl/art_bpath.c                \
	other-licenses/libart_lgpl/art_bpath.h                \
	other-licenses/libart_lgpl/art_gray_svp.c             \
	other-licenses/libart_lgpl/art_gray_svp.h             \
	other-licenses/libart_lgpl/art_misc.c                 \
	other-licenses/libart_lgpl/art_misc.h                 \
	other-licenses/libart_lgpl/art_pathcode.h             \
	other-licenses/libart_lgpl/art_pixbuf.c               \
	other-licenses/libart_lgpl/art_pixbuf.h               \
	other-licenses/libart_lgpl/art_rect.c                 \
	other-licenses/libart_lgpl/art_rect_svp.c             \
	other-licenses/libart_lgpl/art_rect_svp.h             \
	other-licenses/libart_lgpl/art_rect_uta.c             \
	other-licenses/libart_lgpl/art_rect_uta.h             \
	other-licenses/libart_lgpl/art_render.c               \
	other-licenses/libart_lgpl/art_render.h               \
	other-licenses/libart_lgpl/art_render_gradient.c      \
	other-licenses/libart_lgpl/art_render_gradient.h      \
	other-licenses/libart_lgpl/art_render_svp.c           \
	other-licenses/libart_lgpl/art_render_svp.h           \
	other-licenses/libart_lgpl/art_rgb.c                  \
	other-licenses/libart_lgpl/art_rgb.h                  \
	other-licenses/libart_lgpl/art_rgb_a_affine.c         \
	other-licenses/libart_lgpl/art_rgb_affine.c           \
	other-licenses/libart_lgpl/art_rgb_affine_private.c   \
	other-licenses/libart_lgpl/art_rgb_bitmap_affine.c    \
	other-licenses/libart_lgpl/art_rgb_pixbuf_affine.c    \
	other-licenses/libart_lgpl/art_rgb_rgba_affine.c      \
	other-licenses/libart_lgpl/art_rgb_svp.c              \
	other-licenses/libart_lgpl/art_rgb_svp.h              \
	other-licenses/libart_lgpl/art_rgba.c                 \
	other-licenses/libart_lgpl/art_rgba.h                 \
	other-licenses/libart_lgpl/art_svp.c                  \
	other-licenses/libart_lgpl/art_svp_intersect.c        \
	other-licenses/libart_lgpl/art_svp_intersect.h        \
	other-licenses/libart_lgpl/art_svp_ops.c              \
	other-licenses/libart_lgpl/art_svp_ops.h              \
	other-licenses/libart_lgpl/art_svp_point.c            \
	other-licenses/libart_lgpl/art_svp_point.h            \
	other-licenses/libart_lgpl/art_svp_render_aa.c        \
	other-licenses/libart_lgpl/art_svp_render_aa.h        \
	other-licenses/libart_lgpl/art_svp_vpath.c            \
	other-licenses/libart_lgpl/art_svp_vpath.h            \
	other-licenses/libart_lgpl/art_svp_vpath_stroke.c     \
	other-licenses/libart_lgpl/art_svp_vpath_stroke.h     \
	other-licenses/libart_lgpl/art_svp_wind.c             \
	other-licenses/libart_lgpl/art_svp_wind.h             \
	other-licenses/libart_lgpl/art_uta.c                  \
	other-licenses/libart_lgpl/art_uta.h                  \
	other-licenses/libart_lgpl/art_uta_ops.c              \
	other-licenses/libart_lgpl/art_uta_ops.h              \
	other-licenses/libart_lgpl/art_uta_rect.c             \
	other-licenses/libart_lgpl/art_uta_rect.h             \
	other-licenses/libart_lgpl/art_uta_svp.c              \
	other-licenses/libart_lgpl/art_uta_svp.h              \
	other-licenses/libart_lgpl/art_uta_vpath.c            \
	other-licenses/libart_lgpl/art_uta_vpath.h            \
	other-licenses/libart_lgpl/art_vpath.c                \
	other-licenses/libart_lgpl/art_vpath_bpath.c          \
	other-licenses/libart_lgpl/art_vpath_bpath.h          \
	other-licenses/libart_lgpl/art_vpath_dash.c           \
	other-licenses/libart_lgpl/art_vpath_dash.h           \
	other-licenses/libart_lgpl/art_vpath_svp.c            \
	other-licenses/libart_lgpl/art_vpath_svp.h            \
	other-licenses/libart_lgpl/libart.def                 \
	other-licenses/libart_lgpl/testart.c

# list of obsolete files
SVG_BRANCH_OBSOLETE_FILES = \
	layout/svg/base/src/nsASVGGraphicSource.h             \
	layout/svg/base/src/nsASVGPathBuilder.h               \
	layout/svg/base/src/nsISVGFrame.h                     \
	layout/svg/base/src/nsSVGBPathBuilder.cpp             \
	layout/svg/base/src/nsSVGBPathBuilder.h               \
	layout/svg/base/src/nsSVGFill.cpp                     \
	layout/svg/base/src/nsSVGFill.h                       \
	layout/svg/base/src/nsSVGGraphic.cpp                  \
	layout/svg/base/src/nsSVGGraphic.h                    \
	layout/svg/base/src/nsSVGGraphicFrame.cpp             \
	layout/svg/base/src/nsSVGGraphicFrame.h               \
	layout/svg/base/src/nsSVGRenderItem.cpp               \
	layout/svg/base/src/nsSVGRenderItem.h                 \
	layout/svg/base/src/nsSVGRenderingContext.cpp         \
	layout/svg/base/src/nsSVGRenderingContext.h           \
	layout/svg/base/src/nsSVGStroke.cpp                   \
	layout/svg/base/src/nsSVGStroke.h 

# list of new files in branch
SVG_BRANCH_NEW_FILES = \
	content/svg/content/src/nsISVGContent.h               \
	content/svg/content/src/nsISVGLength.h                \
	content/svg/content/src/nsISVGLengthList.h            \
	content/svg/content/src/nsISVGSVGElement.h            \
	content/svg/content/src/nsISVGTextContentMetrics.h    \
	content/svg/content/src/nsISVGValueUtils.h            \
	content/svg/content/src/nsISVGViewportAxis.h          \
	content/svg/content/src/nsISVGViewportRect.h          \
	content/svg/content/src/nsSVGAnimatedLengthList.cpp   \
	content/svg/content/src/nsSVGAnimatedLengthList.h     \
	content/svg/content/src/nsSVGAnimatedString.cpp       \
	content/svg/content/src/nsSVGAnimatedString.h         \
	content/svg/content/src/nsSVGImageElement.cpp         \
	content/svg/content/src/nsSVGLengthList.cpp           \
	content/svg/content/src/nsSVGLengthList.h             \
	content/svg/content/src/nsSVGNumber.cpp               \
	content/svg/content/src/nsSVGNumber.h                 \
	content/svg/content/src/nsSVGTextElement.cpp          \
	content/svg/content/src/nsSVGTSpanElement.cpp         \
	content/svg/content/src/nsSVGTypeCIDs.h               \
	content/svg/content/src/nsSVGViewportRect.cpp         \
	dom/public/idl/svg/nsIDOMSVGAnimatedString.idl        \
	dom/public/idl/svg/nsIDOMSVGImageElement.idl          \
	dom/public/idl/svg/nsIDOMSVGURIReference.idl          \
	layout/svg/base/src/nsISVGChildFrame.h                \
	layout/svg/base/src/nsISVGContainerFrame.h            \
	layout/svg/base/src/nsISVGGlyphFragmentLeaf.h         \
	layout/svg/base/src/nsISVGGlyphFragmentNode.h         \
	layout/svg/base/src/nsISVGOuterSVGFrame.h             \
	layout/svg/base/src/nsISVGTextContainerFrame.h        \
	layout/svg/base/src/nsISVGTextFrame.h                 \
	layout/svg/base/src/nsSVGGlyphFrame.cpp               \
	layout/svg/base/src/nsSVGPathGeometryFrame.cpp        \
	layout/svg/base/src/nsSVGPathGeometryFrame.h          \
	layout/svg/base/src/nsSVGTextFrame.cpp                \
	layout/svg/base/src/nsSVGTSpanFrame.cpp               \
	layout/svg/base/src/resources/jar.mn                  \
	layout/svg/base/src/resources/Makefile.in             \
	layout/svg/base/src/resources/content/contents.rdf    \
	layout/svg/base/src/resources/content/svgBindings.xml \
	layout/svg/renderer/Makefile.in                       \
	layout/svg/renderer/public/Makefile.in                \
	layout/svg/renderer/public/nsISVGGeometrySource.idl   \
	layout/svg/renderer/public/nsISVGGlyphGeometrySource.idl \
	layout/svg/renderer/public/nsISVGGlyphMetricsSource.idl \
	layout/svg/renderer/public/nsISVGPathGeometrySource.idl \
	layout/svg/renderer/public/nsISVGRectangleSink.idl    \
	layout/svg/renderer/public/nsISVGRenderer.idl         \
	layout/svg/renderer/public/nsISVGRendererCanvas.idl   \
	layout/svg/renderer/public/nsISVGRendererGlyphGeometry.idl \
	layout/svg/renderer/public/nsISVGRendererGlyphMetrics.idl \
	layout/svg/renderer/public/nsISVGRendererPathBuilder.idl \
	layout/svg/renderer/public/nsISVGRendererPathGeometry.idl \
	layout/svg/renderer/public/nsISVGRendererRegion.idl   \
	layout/svg/renderer/src/Makefile.in                   \
	layout/svg/renderer/src/gdiplus/Makefile.in           \
	layout/svg/renderer/src/gdiplus/nsISVGGDIPlusCanvas.h \
	layout/svg/renderer/src/gdiplus/nsISVGGDIPlusGlyphMetrics.h \
	layout/svg/renderer/src/gdiplus/nsISVGGDIPlusRegion.h \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusCanvas.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusCanvas.h  \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusGlyphGeometry.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusGlyphGeometry.h \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusGlyphMetrics.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusGlyphMetrics.h \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusPathBuilder.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusPathBuilder.h \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusPathGeometry.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusPathGeometry.h \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusRegion.cpp \
	layout/svg/renderer/src/gdiplus/nsSVGGDIPlusRegion.h  \
	layout/svg/renderer/src/gdiplus/nsSVGRendererGDIPlus.cpp \
	layout/svg/renderer/src/libart/Makefile.in \
	layout/svg/renderer/src/libart/libart-incs.h \
	layout/svg/renderer/src/libart/nsISVGLibartBitmap.h \
	layout/svg/renderer/src/libart/nsISVGLibartCanvas.h \
	layout/svg/renderer/src/libart/nsISVGLibartRegion.h \
	layout/svg/renderer/src/libart/nsSVGFill.cpp \
	layout/svg/renderer/src/libart/nsSVGFill.h \
	layout/svg/renderer/src/libart/nsSVGLibartBitmapDefault.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartBitmapGdk.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartBPathBuilder.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartBPathBuilder.h \
	layout/svg/renderer/src/libart/nsSVGLibartCanvas.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartCanvas.h \
	layout/svg/renderer/src/libart/nsSVGLibartGlyphGeometry.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartGlyphGeometry.h \
	layout/svg/renderer/src/libart/nsSVGLibartGlyphMetrics.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartGlyphMetrics.h \
	layout/svg/renderer/src/libart/nsSVGLibartPathGeometry.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartPathGeometry.h \
	layout/svg/renderer/src/libart/nsSVGLibartRegion.cpp \
	layout/svg/renderer/src/libart/nsSVGLibartRegion.h \
	layout/svg/renderer/src/libart/nsSVGRenderItem.cpp \
	layout/svg/renderer/src/libart/nsSVGRenderItem.h \
	layout/svg/renderer/src/libart/nsSVGRendererLibart.cpp \
	layout/svg/renderer/src/libart/nsSVGStroke.cpp \
	layout/svg/renderer/src/libart/nsSVGStroke.h \
	other-licenses/libart_lgpl/Makefile.am \
	other-licenses/libart_lgpl/acconfig.h \
	other-licenses/libart_lgpl/art_render_mask.c \
	other-licenses/libart_lgpl/art_render_mask.h \
	other-licenses/libart_lgpl/art_vpath_filters.c \
	other-licenses/libart_lgpl/art_vpath_filters.h \
	other-licenses/libart_lgpl/autogen.sh \
	other-licenses/libart_lgpl/configure.in \
	other-licenses/libart_lgpl/gen_art_config.c \
	other-licenses/libart_lgpl/libart-2.0.pc.in \
	other-licenses/libart_lgpl/libart-config.in \
	other-licenses/libart_lgpl/libart-features.h.in \
	other-licenses/libart_lgpl/libartConf.sh.in \
	other-licenses/libart_lgpl/test_gradient.c 

NSPR_CO_TAG = NSPRPUB_PRE_4_2_CLIENT_BRANCH
PSM_CO_TAG = #We will now build PSM from the tip instead of a branch.
NSS_CO_TAG = NSS_CLIENT_TAG
LDAPCSDK_CO_TAG = ldapcsdk_50_client_branch
ACCESSIBLE_CO_TAG = 
IMGLIB2_CO_TAG = 
IPC_CO_TAG = IPC_BRANCH_20030304
TOOLKIT_CO_TAG =
BROWSER_CO_TAG =
MAIL_CO_TAG =
BUILD_MODULES = all

#######################################################################
# Defines
#
CVS = cvs

CWD := $(shell pwd)

ifeq "$(CWD)" "/"
CWD   := /.
endif

ifneq (, $(wildcard client.mk))
# Ran from mozilla directory
ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)
else
# Ran from mozilla/.. directory (?)
ROOTDIR   := $(CWD)
TOPSRCDIR := $(CWD)/mozilla
endif

# on os2, TOPSRCDIR may have two forward slashes in a row, which doesn't
#  work;  replace first instance with one forward slash
TOPSRCDIR := $(shell echo "$(TOPSRCDIR)" | sed -e 's%//%/%')

ifndef TOPSRCDIR_MOZ
TOPSRCDIR_MOZ=$(TOPSRCDIR)
endif

# if ROOTDIR equals only drive letter (i.e. "C:"), set to "/"
DIRNAME := $(shell echo "$(ROOTDIR)" | sed -e 's/^.://')
ifeq ($(DIRNAME),)
ROOTDIR := /.
endif

AUTOCONF := autoconf
MKDIR := mkdir
SH := /bin/sh
ifndef MAKE
MAKE := gmake
endif

CONFIG_GUESS_SCRIPT := $(wildcard $(TOPSRCDIR)/build/autoconf/config.guess)
ifdef CONFIG_GUESS_SCRIPT
  CONFIG_GUESS = $(shell $(CONFIG_GUESS_SCRIPT))
else
  _IS_FIRST_CHECKOUT := 1
endif

####################################
# CVS

# Add the CVS root to CVS_FLAGS if needed
CVS_ROOT_IN_TREE := $(shell cat $(TOPSRCDIR)/CVS/Root 2>/dev/null)
ifneq ($(CVS_ROOT_IN_TREE),)
ifneq ($(CVS_ROOT_IN_TREE),$(CVSROOT))
  CVS_FLAGS := -d $(CVS_ROOT_IN_TREE)
endif
endif

CVSCO = $(strip $(CVS) $(CVS_FLAGS) co $(CVS_CO_FLAGS))
CVSCO_LOGFILE := $(ROOTDIR)/cvsco.log
CVSCO_LOGFILE := $(shell echo $(CVSCO_LOGFILE) | sed s%//%/%)

ifdef MOZ_CO_TAG
  CVS_CO_FLAGS := -f -r $(MOZ_CO_TAG)
endif

####################################
# Load mozconfig Options

# See build pages, http://www.mozilla.org/build/unix.html, 
# for how to set up mozconfig.
MOZCONFIG_LOADER := mozilla/build/autoconf/mozconfig2client-mk
MOZCONFIG_FINDER := mozilla/build/autoconf/mozconfig-find 
MOZCONFIG_MODULES := mozilla/build/unix/modules.mk mozilla/build/unix/uniq.pl
run_for_side_effects := \
  $(shell cd $(ROOTDIR); \
     if test "$(_IS_FIRST_CHECKOUT)"; then \
        $(CVSCO) $(MOZCONFIG_FINDER) $(MOZCONFIG_LOADER) $(MOZCONFIG_MODULES); \
     else true; \
     fi; \
     $(MOZCONFIG_LOADER) $(TOPSRCDIR) mozilla/.mozconfig.mk > mozilla/.mozconfig.out)
include $(TOPSRCDIR)/.mozconfig.mk
include $(TOPSRCDIR)/build/unix/modules.mk

####################################
# Options that may come from mozconfig

# Change CVS flags if anonymous root is requested
ifdef MOZ_CO_USE_MIRROR
  CVS_FLAGS := -d :pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot
endif

# MOZ_CVS_FLAGS - Basic CVS flags
ifeq "$(origin MOZ_CVS_FLAGS)" "undefined"
  CVS_FLAGS := $(CVS_FLAGS) -q -z 3 
else
  CVS_FLAGS := $(MOZ_CVS_FLAGS)
endif

# This option is deprecated. The best way to have client.mk pull a tag
# is to set MOZ_CO_TAG (see above) and commit that change on the tag.
ifdef MOZ_CO_BRANCH
  $(warning Use MOZ_CO_TAG instead of MOZ_CO_BRANCH)
  CVS_CO_FLAGS := -r $(MOZ_CO_BRANCH)
endif

# MOZ_CO_FLAGS - Anything that we should use on all checkouts
ifeq "$(origin MOZ_CO_FLAGS)" "undefined"
  CVS_CO_FLAGS := $(CVS_CO_FLAGS) -P
else
  CVS_CO_FLAGS := $(CVS_CO_FLAGS) $(MOZ_CO_FLAGS)
endif

ifdef MOZ_CO_DATE
  CVS_CO_DATE_FLAGS := -D "$(MOZ_CO_DATE)"
endif

ifdef MOZ_OBJDIR
  OBJDIR := $(MOZ_OBJDIR)
  MOZ_MAKE := $(MAKE) $(MOZ_MAKE_FLAGS) -C $(OBJDIR)
else
  OBJDIR := $(TOPSRCDIR)
  MOZ_MAKE := $(MAKE) $(MOZ_MAKE_FLAGS)
endif

####################################
# CVS defines for PSM
#
PSM_CO_MODULE= mozilla/security/manager
PSM_CO_FLAGS := -P -A
ifdef MOZ_CO_FLAGS
  PSM_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef PSM_CO_TAG
  PSM_CO_FLAGS := $(PSM_CO_FLAGS) -r $(PSM_CO_TAG)
endif
CVSCO_PSM = $(CVS) $(CVS_FLAGS) co $(PSM_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(PSM_CO_MODULE)

####################################
# CVS defines for NSS
#
NSS_CO_MODULE = mozilla/security/nss \
		mozilla/security/coreconf \
		$(NULL)

NSS_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  NSS_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef NSS_CO_TAG
   NSS_CO_FLAGS := $(NSS_CO_FLAGS) -r $(NSS_CO_TAG)
endif
# Cannot pull static tags by date
ifeq ($(NSS_CO_TAG),NSS_CLIENT_TAG)
CVSCO_NSS = $(CVS) $(CVS_FLAGS) co $(NSS_CO_FLAGS) $(NSS_CO_MODULE)
else
CVSCO_NSS = $(CVS) $(CVS_FLAGS) co $(NSS_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(NSS_CO_MODULE)
endif

####################################
# CVS defines for NSPR
#
NSPR_CO_MODULE = mozilla/nsprpub
NSPR_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  NSPR_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef NSPR_CO_TAG
  NSPR_CO_FLAGS := $(NSPR_CO_FLAGS) -r $(NSPR_CO_TAG)
endif
# Cannot pull static tags by date
ifeq ($(NSPR_CO_TAG),NSPRPUB_CLIENT_TAG)
CVSCO_NSPR = $(CVS) $(CVS_FLAGS) co $(NSPR_CO_FLAGS) $(NSPR_CO_MODULE)
else
CVSCO_NSPR = $(CVS) $(CVS_FLAGS) co $(NSPR_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(NSPR_CO_MODULE)
endif

####################################
# CVS defines for the C LDAP SDK
#
LDAPCSDK_CO_MODULE = mozilla/directory/c-sdk
LDAPCSDK_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  LDAPCSDK_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef LDAPCSDK_CO_TAG
  LDAPCSDK_CO_FLAGS := $(LDAPCSDK_CO_FLAGS) -r $(LDAPCSDK_CO_TAG)
endif
CVSCO_LDAPCSDK = $(CVS) $(CVS_FLAGS) co $(LDAPCSDK_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(LDAPCSDK_CO_MODULE)

####################################
# CVS defines for the C LDAP SDK
#
ACCESSIBLE_CO_MODULE = mozilla/accessible
ACCESSIBLE_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  ACCESSIBLE_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef ACCESSIBLE_CO_TAG
  ACCESSIBLE_CO_FLAGS := $(ACCESSIBLE_CO_FLAGS) -r $(ACCESSIBLE_CO_TAG)
endif
CVSCO_ACCESSIBLE = $(CVS) $(CVS_FLAGS) co $(ACCESSIBLE_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(ACCESSIBLE_CO_MODULE)


####################################
# CVS defines for new image library
#
IMGLIB2_CO_MODULE = mozilla/modules/libpr0n
IMGLIB2_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  IMGLIB2_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef IMGLIB2_CO_TAG
  IMGLIB2_CO_FLAGS := $(IMGLIB2_CO_FLAGS) -r $(IMGLIB2_CO_TAG)
endif
CVSCO_IMGLIB2 = $(CVS) $(CVS_FLAGS) co $(IMGLIB2_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(IMGLIB2_CO_MODULE)

####################################
# CVS defines for ipc module
#
IPC_CO_MODULE = mozilla/ipc/ipcd
IPC_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  IPC_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef IPC_CO_TAG
  IPC_CO_FLAGS := $(IPC_CO_FLAGS) -r $(IPC_CO_TAG)
endif
CVSCO_IPC = $(CVS) $(CVS_FLAGS) co $(IPC_CO_FLAGS) $(CVS_CO_DATE_FLAGS) $(IPC_CO_MODULE)

####################################
# CVS defines for Calendar 
#
CVSCO_CALENDAR := $(CVSCO) $(CVS_CO_DATE_FLAGS) mozilla/calendar mozilla/other-licenses/libical

####################################
# CVS defines for SeaMonkey
#
ifeq ($(MOZ_CO_MODULE),)
  MOZ_CO_MODULE := SeaMonkeyAll
endif
CVSCO_SEAMONKEY := $(CVSCO) $(CVS_CO_DATE_FLAGS) $(MOZ_CO_MODULE)

####################################
# CVS defines for standalone modules
#
ifeq ($(BUILD_MODULES),all)
  CHECKOUT_STANDALONE := true
  CHECKOUT_STANDALONE_NOSUBDIRS := true
else
  STANDALONE_CO_MODULE := $(filter-out $(NSPRPUB_DIR) security directory/c-sdk, $(BUILD_MODULE_CVS))
  STANDALONE_CO_MODULE += allmakefiles.sh client.mk aclocal.m4 configure configure.in
  STANDALONE_CO_MODULE += Makefile.in
  STANDALONE_CO_MODULE := $(addprefix mozilla/, $(STANDALONE_CO_MODULE))
  CHECKOUT_STANDALONE := cvs_co $(CVSCO) $(CVS_CO_DATE_FLAGS) $(STANDALONE_CO_MODULE)

  NOSUBDIRS_MODULE := $(addprefix mozilla/, $(BUILD_MODULE_CVS_NS))
ifneq ($(NOSUBDIRS_MODULE),)
  CHECKOUT_STANDALONE_NOSUBDIRS := cvs_co $(CVSCO) -l $(CVS_CO_DATE_FLAGS) $(NOSUBDIRS_MODULE)
else
  CHECKOUT_STANDALONE_NOSUBDIRS := true
endif

CVSCO_SEAMONKEY :=
ifeq (,$(filter $(NSPRPUB_DIR), $(BUILD_MODULE_CVS)))
  CVSCO_NSPR :=
endif
ifeq (,$(filter security security/manager, $(BUILD_MODULE_CVS)))
  CVSCO_PSM :=
  CVSCO_NSS :=
endif
ifeq (,$(filter directory/c-sdk, $(BUILD_MODULE_CVS)))
  CVSCO_LDAPCSDK :=
endif
ifeq (,$(filter accessible, $(BUILD_MODULE_CVS)))
  CVSCO_ACCESSIBLE :=
endif
ifeq (,$(filter modules/libpr0n, $(BUILD_MODULE_CVS)))
  CVSCO_IMGLIB2 :=
endif
ifeq (,$(filter ipc, $(BUILD_MODULE_CVS)))
  CVSCO_IPC :=
endif
ifeq (,$(filter calendar other-licenses/libical, $(BUILD_MODULE_CVS)))
  CVSCO_CALENDAR :=
endif
endif

####################################
# CVS defined for libart (pulled and built if MOZ_INTERNAL_LIBART_LGPL is set)
#
CVSCO_LIBART := $(CVSCO) $(CVS_CO_DATE_FLAGS) mozilla/other-licenses/libart_lgpl

ifdef MOZ_INTERNAL_LIBART_LGPL
FASTUPDATE_LIBART := fast_update $(CVSCO_LIBART)
CHECKOUT_LIBART := cvs_co $(CVSCO_LIBART)
else
CHECKOUT_LIBART := true
FASTUPDATE_LIBART := true
endif

####################################
# CVS defines for Phoenix (pulled and built if MOZ_PHOENIX is set)
#
BROWSER_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  BROWSER_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef BROWSER_CO_TAG
  BROWSER_CO_FLAGS := $(BROWSER_CO_FLAGS) -r $(BROWSER_CO_TAG)
endif

CVSCO_PHOENIX := $(CVS) $(CVS_FLAGS) co $(BROWSER_CO_FLAGS) $(CVS_CO_DATE_FLAGS) mozilla/browser

ifdef MOZ_PHOENIX
FASTUPDATE_PHOENIX := fast_update $(CVSCO_PHOENIX)
CHECKOUT_PHOENIX := cvs_co $(CVSCO_PHOENIX)
MOZ_XUL_APP = 1
else
CHECKOUT_PHOENIX := true
FASTUPDATE_PHOENIX := true
endif

####################################
# CVS defines for Thunderbird (pulled and built if MOZ_THUNDERBIRD is set)
#

MAIL_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  MAIL_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef MAIL_CO_TAG
  MAIL_CO_FLAGS := $(MAIL_CO_FLAGS) -r $(MAIL_CO_TAG)
endif

CVSCO_THUNDERBIRD := $(CVS) $(CVS_FLAGS) co $(MAIL_CO_FLAGS) $(CVS_CO_DATE_FLAGS) mozilla/mail
ifdef MOZ_THUNDERBIRD
FASTUPDATE_THUNDERBIRD := fast_update $(CVSCO_THUNDERBIRD)
CHECKOUT_THUNDERBIRD := cvs_co $(CVSCO_THUNDERBIRD)
MOZ_XUL_APP = 1
else
FASTUPDATE_THUNDERBIRD := true
CHECKOUT_THUNDERBIRD := true
endif

####################################
# CVS defines for mozilla/toolkit (pulled and built if MOZ_XUL_APP is set)
#

TOOLKIT_CO_FLAGS := -P
ifdef MOZ_CO_FLAGS
  TOOLKIT_CO_FLAGS := $(MOZ_CO_FLAGS)
endif
ifdef TOOLKIT_CO_TAG
  TOOLKIT_CO_FLAGS := $(TOOLKIT_CO_FLAGS) -r $(TOOLKIT_CO_TAG)
endif

CVSCO_MOZTOOLKIT := $(CVS) $(CVS_FLAGS) co $(TOOLKIT_CO_FLAGS) $(CVS_CO_DATE_FLAGS)  mozilla/toolkit
ifdef MOZ_XUL_APP
FASTUPDATE_MOZTOOLKIT := fast_update $(CVSCO_MOZTOOLKIT)
CHECKOUT_MOZTOOLKIT := cvs_co $(CVSCO_MOZTOOLKIT)
else
FASTUPDATE_MOZTOOLKIT := true
CHECKOUT_MOZTOOLKIT := true
endif

####################################
# CVS defines for codesighs (pulled and built if MOZ_MAPINFO is set)
#
CVSCO_CODESIGHS := $(CVSCO) $(CVS_CO_DATE_FLAGS) mozilla/tools/codesighs

ifdef MOZ_MAPINFO
FASTUPDATE_CODESIGHS := fast_update $(CVSCO_CODESIGHS)
CHECKOUT_CODESIGHS := cvs_co $(CVSCO_CODESIGHS)
else
CHECKOUT_CODESIGHS := true
FASTUPDATE_CODESIGHS := true
endif

#######################################################################
# Rules
# 

# Print out any options loaded from mozconfig.
all build checkout clean depend distclean export libs install realclean::
	@if test -f .mozconfig.out; then \
	  cat .mozconfig.out; \
	  rm -f .mozconfig.out; \
	else true; \
	fi

ifdef _IS_FIRST_CHECKOUT
all:: checkout build
else
all:: checkout alldep
endif

# Windows equivalents
pull_all: checkout
build_all: build
build_all_dep: alldep
build_all_depend: alldep
clobber clobber_all: clean
pull_and_build_all: checkout alldep

# Do everything from scratch
everything: checkout clean build

####################################
# CVS checkout
#
checkout::
#	@: Backup the last checkout log.
	@if test -f $(CVSCO_LOGFILE) ; then \
	  mv $(CVSCO_LOGFILE) $(CVSCO_LOGFILE).old; \
	else true; \
	fi
ifdef RUN_AUTOCONF_LOCALLY
	@echo "Removing local configures" ; \
	cd $(ROOTDIR) && \
	$(RM) -f mozilla/configure mozilla/nsprpub/configure \
		mozilla/directory/c-sdk/configure
endif
	@echo "checkout start: "`date` | tee $(CVSCO_LOGFILE)
	@echo '$(CVSCO) $(CVS_CO_DATE_FLAGS) mozilla/client.mk $(MOZCONFIG_MODULES)'; \
        cd $(ROOTDIR) && \
	$(CVSCO) $(CVS_CO_DATE_FLAGS) mozilla/client.mk $(MOZCONFIG_MODULES)
	@cd $(ROOTDIR) && $(MAKE) -f mozilla/client.mk real_checkout

real_checkout:
#	@: Start the checkout. Split the output to the tty and a log file. \
#	 : If it fails, touch an error file because "tee" hides the error.
	@failed=.cvs-failed.tmp; rm -f $$failed*; \
	cvs_co() { echo "$$@" ; \
	  ("$$@" || touch $$failed) 2>&1 | tee -a $(CVSCO_LOGFILE) && \
	  if test -f $$failed; then false; else true; fi; }; \
	$(CHECKOUT_STANDALONE) && \
	$(CHECKOUT_STANDALONE_NOSUBDIRS) && \
	cvs_co $(CVSCO_NSPR) && \
	cvs_co $(CVSCO_NSS) && \
	cvs_co $(CVSCO_PSM) && \
	cvs_co $(CVSCO_LDAPCSDK) && \
	cvs_co $(CVSCO_ACCESSIBLE) && \
	cvs_co $(CVSCO_IMGLIB2) && \
	cvs_co $(CVSCO_IPC) && \
	cvs_co $(CVSCO_CALENDAR) && \
	$(CHECKOUT_LIBART) && \
	$(CHECKOUT_MOZTOOLKIT) && \
	$(CHECKOUT_PHOENIX) && \
	$(CHECKOUT_THUNDERBIRD) && \
	$(CHECKOUT_CODESIGHS) && \
	cvs_co $(CVSCO_SEAMONKEY)
	@echo "checkout finish: "`date` | tee -a $(CVSCO_LOGFILE)
# update the NSS checkout timestamp
	@if test `egrep -c '^(U|C) mozilla/security/(nss|coreconf)' $(CVSCO_LOGFILE) 2>/dev/null` != 0; then \
		touch $(TOPSRCDIR)/security/manager/.nss.checkout; \
	fi
#	@: Check the log for conflicts. ;
	@conflicts=`egrep "^C " $(CVSCO_LOGFILE)` ;\
	if test "$$conflicts" ; then \
	  echo "$(MAKE): *** Conflicts during checkout." ;\
	  echo "$$conflicts" ;\
	  echo "$(MAKE): Refer to $(CVSCO_LOGFILE) for full log." ;\
	  false; \
	else true; \
	fi
ifdef RUN_AUTOCONF_LOCALLY
	@echo Generating configures using $(AUTOCONF) ; \
	cd $(TOPSRCDIR) && $(AUTOCONF) && \
	cd $(TOPSRCDIR)/nsprpub && $(AUTOCONF) && \
	cd $(TOPSRCDIR)/directory/c-sdk && $(AUTOCONF)
endif

fast-update:
#	@: Backup the last checkout log.
	@if test -f $(CVSCO_LOGFILE) ; then \
	  mv $(CVSCO_LOGFILE) $(CVSCO_LOGFILE).old; \
	else true; \
	fi
ifdef RUN_AUTOCONF_LOCALLY
	@echo "Removing local configures" ; \
	cd $(ROOTDIR) && \
	$(RM) -f mozilla/configure mozilla/nsprpub/configure \
		mozilla/directory/c-sdk/configure
endif
	@echo "checkout start: "`date` | tee $(CVSCO_LOGFILE)
	@echo '$(CVSCO) mozilla/client.mk $(MOZCONFIG_MODULES)'; \
        cd $(ROOTDIR) && \
	$(CVSCO) mozilla/client.mk $(MOZCONFIG_MODULES)
	@cd $(TOPSRCDIR) && \
	$(MAKE) -f client.mk real_fast-update

real_fast-update:
#	@: Start the update. Split the output to the tty and a log file. \
#	 : If it fails, touch an error file because "tee" hides the error.
	@failed=.fast_update-failed.tmp; rm -f $$failed*; \
	fast_update() { (config/cvsco-fast-update.pl $$@ || touch $$failed) 2>&1 | tee -a $(CVSCO_LOGFILE) && \
	  if test -f $$failed; then false; else true; fi; }; \
	cvs_co() { echo "$$@" ; \
	  ("$$@" || touch $$failed) 2>&1 | tee -a $(CVSCO_LOGFILE) && \
	  if test -f $$failed; then false; else true; fi; }; \
	fast_update $(CVSCO_NSPR) && \
	cd $(ROOTDIR) && \
	failed=mozilla/.fast_update-failed.tmp && \
	cvs_co $(CVSCO_NSS) && \
	failed=.fast_update-failed.tmp && \
	cd mozilla && \
	fast_update $(CVSCO_PSM) && \
	fast_update $(CVSCO_LDAPCSDK) && \
	fast_update $(CVSCO_ACCESSIBLE) && \
	fast_update $(CVSCO_IMGLIB2) && \
	fast_update $(CVSCO_IPC) && \
	fast_update $(CVSCO_CALENDAR) && \
	$(FASTUPDATE_LIBART) && \
	$(FASTUPDATE_MOZTOOLKIT) && \
	$(FASTUPDATE_PHOENIX) && \
	$(FASTUPDATE_THUNDERBIRD) && \
	$(FASTUPDATE_CODESIGHS) && \
	fast_update $(CVSCO_SEAMONKEY)
	@echo "fast_update finish: "`date` | tee -a $(CVSCO_LOGFILE)
# update the NSS checkout timestamp
	@if test `egrep -c '^(U|C) mozilla/security/(nss|coreconf)' $(CVSCO_LOGFILE) 2>/dev/null` != 0; then \
		touch $(TOPSRCDIR)/security/manager/.nss.checkout; \
	fi
#	@: Check the log for conflicts. ;
	@conflicts=`egrep "^C " $(CVSCO_LOGFILE)` ;\
	if test "$$conflicts" ; then \
	  echo "$(MAKE): *** Conflicts during fast-update." ;\
	  echo "$$conflicts" ;\
	  echo "$(MAKE): Refer to $(CVSCO_LOGFILE) for full log." ;\
	  false; \
	else true; \
	fi
ifdef RUN_AUTOCONF_LOCALLY
	@echo Generating configures using $(AUTOCONF) ; \
	cd $(TOPSRCDIR) && $(AUTOCONF) && \
	cd $(TOPSRCDIR)/nsprpub && $(AUTOCONF) && \
	cd $(TOPSRCDIR)/directory/c-sdk && $(AUTOCONF)
endif

# svg mini-branch maintenance targets:
commit_svg:
	cvs -z3 ci $(SVG_BRANCH_MODIFIED_FILES) $(SVG_BRANCH_NEW_FILES)

merge_svg:
	cvs -z3 up -dP -jSVG_20020806_BASE -jHEAD $(SVG_BRANCH_MODIFIED_FILES)

statictag_svg:
	cvs -z3 tag -F -rHEAD SVG_20020806_BASE $(SVG_BRANCH_MODIFIED_FILES)

diff_svg:
	cvs -z3 diff -u $(SVG_BRANCH_MODIFIED_FILES) $(SVG_BRANCH_NEW_FILES)

status_svg:
	cvs -z3 status $(SVG_BRANCH_MODIFIED_FILES) $(SVG_BRANCH_NEW_FILES) | grep -A7 "File:.*Status: [^U]"

# write the file 'svg_branch_files' which can then be used in greps,
# etc.  
create_file_list_svg:
	echo $(SVG_BRANCH_MODIFIED_FILES) $(SVG_BRANCH_NEW_FILES) > svg_branch_files

#branchtag_svg:
#	cvs -z3 tag -b SVG_20020806_BRANCH $(SVG_BRANCH_MODIFIED_FILES) $(SVG_BRANCH_OBSOLETE_FILES)

#remove_obsolete_svg:
#	rm $(SVG_BRANCH_OBSOLETE_FILES)
#	cvs -z3 remove $(SVG_BRANCH_OBSOLETE_FILES)
#	cvs -z3 commit $(SVG_BRANCH_OBSOLETE_FILES)


####################################
# Web configure

WEBCONFIG_FILE  := $(HOME)/.mozconfig

MOZCONFIG2CONFIGURATOR := build/autoconf/mozconfig2configurator
webconfig:
	@cd $(TOPSRCDIR); \
	url=`$(MOZCONFIG2CONFIGURATOR) $(TOPSRCDIR)`; \
	echo Running mozilla with the following url: ;\
	echo ;\
	echo $$url ;\
	mozilla -remote "openURL($$url)" || \
	netscape -remote "openURL($$url)" || \
	mozilla $$url || \
	netscape $$url ;\
	echo ;\
	echo   1. Fill out the form on the browser. ;\
	echo   2. Save the results to $(WEBCONFIG_FILE)

#####################################################
# First Checkout

ifdef _IS_FIRST_CHECKOUT
# First time, do build target in a new process to pick up new files.
build::
	$(MAKE) -f $(TOPSRCDIR)/client.mk build
else

#####################################################
# After First Checkout


####################################
# Configure

CONFIG_STATUS := $(wildcard $(OBJDIR)/config.status)
CONFIG_CACHE  := $(wildcard $(OBJDIR)/config.cache)

ifdef RUN_AUTOCONF_LOCALLY
EXTRA_CONFIG_DEPS := \
	$(TOPSRCDIR)/aclocal.m4 \
	$(wildcard $(TOPSRCDIR)/build/autoconf/*.m4) \
	$(NULL)

$(TOPSRCDIR)/configure: $(TOPSRCDIR)/configure.in $(EXTRA_CONFIG_DEPS)
	@echo Generating $@ using autoconf
	cd $(TOPSRCDIR); $(AUTOCONF)
endif

CONFIG_STATUS_DEPS_L10N := $(wildcard $(TOPSRCDIR)/l10n/makefiles.all)

CONFIG_STATUS_DEPS := \
	$(TOPSRCDIR)/configure \
	$(TOPSRCDIR)/allmakefiles.sh \
	$(TOPSRCDIR)/.mozconfig.mk \
	$(wildcard $(TOPSRCDIR)/nsprpub/configure) \
	$(wildcard $(TOPSRCDIR)/directory/c-sdk/configure) \
	$(wildcard $(TOPSRCDIR)/mailnews/makefiles) \
	$(CONFIG_STATUS_DEPS_L10N) \
	$(wildcard $(TOPSRCDIR)/themes/makefiles) \
	$(NULL)

# configure uses the program name to determine @srcdir@. Calling it without
#   $(TOPSRCDIR) will set @srcdir@ to "."; otherwise, it is set to the full
#   path of $(TOPSRCDIR).
ifeq ($(TOPSRCDIR),$(OBJDIR))
  CONFIGURE := ./configure
else
  CONFIGURE := $(TOPSRCDIR)/configure
endif

ifdef MOZ_TOOLS
  CONFIGURE := $(TOPSRCDIR)/configure
endif

$(OBJDIR)/Makefile $(OBJDIR)/config.status: $(CONFIG_STATUS_DEPS)
	@if test ! -d $(OBJDIR); then $(MKDIR) $(OBJDIR); else true; fi
	@echo cd $(OBJDIR);
	@echo $(CONFIGURE) $(CONFIGURE_ARGS)
	@cd $(OBJDIR) && $(CONFIGURE_ENV_ARGS) $(CONFIGURE) $(CONFIGURE_ARGS) \
	  || ( echo "*** Fix above errors and then restart with\
               \"$(MAKE) -f client.mk build\"" && exit 1 )
	@touch $(OBJDIR)/Makefile

ifdef CONFIG_STATUS
$(OBJDIR)/config/autoconf.mk: $(TOPSRCDIR)/config/autoconf.mk.in
	cd $(OBJDIR); \
	  CONFIG_FILES=config/autoconf.mk ./config.status
endif


####################################
# Depend

depend:: $(OBJDIR)/Makefile $(OBJDIR)/config.status
	$(MOZ_MAKE) export && $(MOZ_MAKE) depend

####################################
# Build it

build::  $(OBJDIR)/Makefile $(OBJDIR)/config.status
	$(MOZ_MAKE)

####################################
# Other targets

# Pass these target onto the real build system
install export libs clean realclean distclean alldep:: $(OBJDIR)/Makefile $(OBJDIR)/config.status
	$(MOZ_MAKE) $@

cleansrcdir:
	@cd $(TOPSRCDIR); \
        if [ -f webshell/embed/gtk/Makefile ]; then \
          $(MAKE) -C webshell/embed/gtk distclean; \
        fi; \
	if [ -f Makefile ]; then \
	  $(MAKE) distclean ; \
	else \
	  echo "Removing object files from srcdir..."; \
	  rm -fr `find . -type d \( -name .deps -print -o -name CVS \
	          -o -exec test ! -d {}/CVS \; \) -prune \
	          -o \( -name '*.[ao]' -o -name '*.so' \) -type f -print`; \
	   build/autoconf/clean-config.sh; \
	fi;

# (! IS_FIRST_CHECKOUT)
endif

echo_objdir:
	@echo $(OBJDIR)

.PHONY: checkout real_checkout depend build export libs alldep install clean realclean distclean cleansrcdir pull_all build_all clobber clobber_all pull_and_build_all everything
