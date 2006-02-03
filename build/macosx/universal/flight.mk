# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Mozilla Mac OS X Universal Binary Packaging System
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#  Mark Mentovai <mark@moxienet.com> (Original Author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

ifndef OBJDIR
OBJDIR = $(MOZ_OBJDIR)/ppc
OBJDIR_X86 = $(MOZ_OBJDIR)/i386
DIST_X86 = $(OBJDIR_X86)/dist
endif

include $(OBJDIR)/config/autoconf.mk

DIST = $(OBJDIR)/dist

# Need overrides for APPNAME = Camino when building Camino

ifeq ($(MOZ_BUILD_APP),macbrowser)
APPDIR = camino
MOZ_PKG_APPNAME = camino
APPNAME = Camino
BUILDCONFIG_JAR = Contents/MacOS/chrome/embed.jar
else
APPDIR = $(MOZ_BUILD_APP)
MOZ_PKG_APPNAME = $(MOZ_APP_NAME)
ifdef MOZ_DEBUG
APPNAME = $(MOZ_APP_DISPLAYNAME)Debug
else
APPNAME = $(MOZ_APP_DISPLAYNAME)
endif
BUILDCONFIG_JAR = Contents/MacOS/chrome/toolkit.jar
endif

postflight_all:
# Build the universal package out of only the bits that would be released.
# Call the packager to set this up.  Set MAKE_PACKAGE= to avoid building
# a dmg.  Set SIGN_NSS= to skip shlibsign.
# Revisit universal packaging later to ensure that the native shlibsign would
# be used to sign NSS libraries.
	$(MAKE) -C $(OBJDIR)/$(APPDIR)/installer MAKE_PACKAGE= SIGN_NSS=
	$(MAKE) -C $(OBJDIR_X86)/$(APPDIR)/installer MAKE_PACKAGE= SIGN_NSS=
	$(TOPSRCDIR)/build/macosx/universal/fix-buildconfig $(DIST)/$(MOZ_PKG_APPNAME)/$(APPNAME).app/$(BUILDCONFIG_JAR) $(DIST_X86)/$(MOZ_PKG_APPNAME)/$(APPNAME).app/$(BUILDCONFIG_JAR)
	mkdir -p $(DIST)/universal/$(MOZ_PKG_APPNAME)
	rm -f $(DIST_X86)/universal
	ln -s $(DIST)/universal $(DIST_X86)/universal
	rm -rf $(DIST)/universal/$(MOZ_PKG_APPNAME)/$(APPNAME).app
	$(TOPSRCDIR)/build/macosx/universal/unify $(DIST)/$(MOZ_PKG_APPNAME)/$(APPNAME).app $(DIST_X86)/$(MOZ_PKG_APPNAME)/$(APPNAME).app $(DIST)/universal/$(MOZ_PKG_APPNAME)/$(APPNAME).app
