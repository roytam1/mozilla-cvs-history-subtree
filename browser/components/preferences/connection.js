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

var gConnectionsDialog = {
  init: function ()
  {
  
  },
  
  proxyTypeChanged: function ()
  {
    var proxyTypePref = document.getElementById("network.proxy.type");
    var proxyPrefs = ["http", "ssl", "ftp", "socks", "gopher"];
    for (var i = 0; i < proxyPrefs.length; ++i) {
      var proxyServerURLPref = document.getElementById("network.proxy." + proxyPrefs[i]);
      proxyServerURLPref.disabled = proxyTypePref.value != 1;
      var proxyPortPref = document.getElementById("network.proxy." + proxyPrefs[i] + "_port");
      proxyPortPref.disabled = proxyTypePref.value != 1;
    }
    
    var shareProxiesPref = document.getElementById("network.proxy.share_proxy_settings");
    shareProxiesPref.disabled = proxyTypePref.value != 1;
    
    var socksVersionPref = document.getElementById("network.proxy.socks_version");
    socksVersionPref.disabled = proxyTypePref.value != 1;
    
    var noProxiesPref = document.getElementById("network.proxy.no_proxies_on");
    noProxiesPref.disabled = proxyTypePref.value != 1;
    
    var autoconfigURLPref = document.getElementById("network.proxy.autoconfig_url");
    autoconfigURLPref.disabled = proxyTypePref.value != 2;
    
    var disableReloadPref = document.getElementById("pref.advanced.proxies.disable_button.reload");
    disableReloadPref.disabled = proxyTypePref.value != 2;
  },
  
  readProxyType: function ()
  {
    this.proxyTypeChanged();
    return undefined;
  },
  
  shareSettingsChanged: function ()
  {
    var shareProxiesPref = document.getElementById("network.proxy.share_proxy_settings");
    shareProxiesPref.updateElements();
  },
  
  readProxyProtocolPref: function (aProtocol, aIsPort)
  {
    var shareProxiesPref = document.getElementById("network.proxy.share_proxy_settings");
    if (shareProxiesPref.value) {
      var pref = document.getElementById("network.proxy.http" + (aIsPort ? "_port" : ""));
      return pref.value;
    }
    return undefined;
  },

  reloadPAC: function ()
  {
    var autoURL = document.getElementById("networkProxyAutoconfigURL");
    var pps = Components.classesByID["{e9b301c0-e0e4-11D3-a1a8-0050041caf44}"]
                        .getService(Components.interfaces.nsIProtocolProxyService);
    pps.configureFromPAC(autoURL.value);
  },
  
  fixupURI: function ()
  {
    var autoURL = document.getElementById("networkProxyAutoconfigURL");
    var URIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                            .getService(Components.interfaces.nsIURIFixup);
    try {
      var fixedUpURI = URIFixup.createFixupURI(autoURL.value, 0);
      autoURL.value = fixedUpURI.spec;
    }
    catch(ex) {
    }
  },

  oldUrls: ["","",""],
  oldPorts: ["0","0","0"],
  
  shareProxies: function ()
  {
  
  },
};
