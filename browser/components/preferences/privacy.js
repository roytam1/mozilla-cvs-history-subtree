# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
# The Original Code is the Firefox Preferences System.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
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

var gPrivacyPane = {
  _sanitizer: null,
  init: function ()
  {
    var itemList = document.getElementById("itemList");
    var lastSelected = 0;
    if (itemList.hasAttribute("lastSelected"))
      lastSelected = parseInt(itemList.getAttribute("lastSelected"));
    itemList.selectedIndex = lastSelected;
    
    // Update the clear buttons
    if (!this._sanitizer)
      this._sanitizer = new Sanitizer();
    this._updateClearButtons();
    
    window.addEventListenre("unload", this.uninit, false);
  },
  
  uninit: function ()
  {
    if (this._updateInterval != -1)
      clearInterval(this._updateInterval);
  },
  
  _updateInterval: -1,
  _updateClearButtons: function ()
  {
    var buttons = document.getElementsByAttribute("item", "*");
    var buttonCount = buttons.length;
    for (var i = 0; i < buttonCount; ++i)
      buttons[i].disabled = !this._sanitizer.canClearItem(buttons[i].getAttribute("item"));
  },
    
  onItemSelect: function ()
  {
    var itemList = document.getElementById("itemList");
    var itemPreferences = document.getElementById("itemPreferences");
    itemPreferences.setAttribute("selectedIndex", itemList.selectedIndex);
    itemList.setAttribute("lastSelected", itemList.selectedIndex);
    document.persist("itemList", "lastSelected");
  },
  
  showSanitizeSettings: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/sanitize.xul",
                                           "", null);
  },

  _enableRestrictedWasChecked: false,
  writeEnableCookiesPref: function ()
  { 
    var enableCookies = document.getElementById("enableCookies");
    if (enableCookies.checked) {
      var enableRestricted = document.getElementById("enableCookiesForOriginating");
      this._enableRestrictedWasChecked = enableRestricted.checked;
      return this._enableRestrictedWasChecked ? 1 : 0;
    }
    return 2;
  },

  readEnableCookiesPref: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    var enableRestricted = document.getElementById("enableCookiesForOriginating");    
    enableRestricted.disabled = pref.value == 2;
    this.enableCookiesChanged();
    return (pref.value == 0 || pref.value == 1);
  },
  
  readEnableRestrictedPref: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    return pref.value != 2 ? pref.value : this._enableRestrictedWasChecked;
  },

  enableCookiesChanged: function ()
  {
    var preference = document.getElementById("network.cookie.cookieBehavior");
    var denyPref = document.getElementById("network.cookie.denyRemovedCookies");
    denyPref.disabled = preference.value == 2;
    var lifetimePref = document.getElementById("network.cookie.lifetimePolicy");
    lifetimePref.disabled = preference.value == 2;
  },
  
  readCacheSizePref: function ()
  {
    var preference = document.getElementById("browser.cache.disk.capacity");
    return preference.value / 1000;
  },
  
  writeCacheSizePref: function ()
  {
    var cacheSize = document.getElementById("cacheSize");
    return cacheSize * 1000;
  },
  
  _sanitizer: null,
  clear: function (aButton) 
  {
    var category = aButton.getAttribute("item");
    this._sanitizer.clearItem(category);
    aButton.disabled = !this._sanitizer.canClearItem(category);
    if (this._updateInterval == -1)
      this._updateInterval = setInterval("gPrivacyPane._updateClearButtons()", 10000);
  },
  
  viewCookies: function (aCategory) 
  {
    document.documentElement.openSubDialog("chrome://browser/content/cookieviewer/CookieViewer.xul",
                                           "resizable", "cookieManager");
  },
  viewDownloads: function (aCategory) 
  {
    document.documentElement.openWindow("Download:Manager", 
                                        "chrome://mozapps/content/downloads/downloads.xul",
                                        "dialog=no,resizable", null);
  },
  viewPasswords: function (aCategory) 
  {
    document.documentElement.openSubDialog("chrome://passwordmgr/content/passwordManager.xul",
                                           "resizable", "8");
  },
};

