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

const nsICookie = Components.interfaces.nsICookie;

var gCookiesWindow = {
  _cm               : Components.classes["@mozilla.org/cookiemanager;1"]
                                .getService(Components.interfaces.nsICookieManager),
  _ds               : Components.classes["@mozilla.org/intl/scriptabledateformat;1"]
                                .getService(Components.interfaces.nsIScriptableDateFormat),
  _cookies          : [],
  _hosts            : {},
  _tree             : null,
  _bundle           : null,

  init: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "cookieChanged", false);
    os.addObserver(this, "perm-changed", false);
    
    this._bundle = document.getElementById("bundlePreferences");
    this._tree = document.getElementById("cookiesList");
    
    this._loadCookies();
    this._tree.view.selection.select(0);
  },
  
  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "cookieChanged");
    os.removeObserver(this, "perm-changed");
  },

  _view: {
    _outer: this,
    _filtered: false,
    _rowCount: 0,
    get rowCount() 
    { 
      return this._rowCount; 
    },
    
    _dumpIf: function (aIndex, aRequiredIndex, aDump)
    {
      if (aIndex == aRequiredIndex)
        dump(aDump + "\n");
    },
    
    _getItemAtIndex: function (aIndex)
    {
      var count = 0, hostIndex = 0;
      for (var host in gCookiesWindow._hosts) {
        var currHost = gCookiesWindow._hosts[host];
        if (count == aIndex)
          return currHost;
        hostIndex = count;
        if (currHost.open) {
          if (count < aIndex && aIndex <= (count + currHost.cookies.length)) {
            // We are looking for an entry within this host's children, 
            // enumerate them looking for the index. 
            ++count;
            for (var i = 0; i < currHost.cookies.length; ++i) {
              if (count == aIndex) {
                var cookie = currHost.cookies[i];
                cookie.parentIndex = hostIndex;
                return cookie;
              }
              ++count;
            }
          }
          else {
            // A host entry was open, but we weren't looking for an index
            // within that host entry's children, so skip forward over the
            // entry's children. We need to add one to increment for the
            // host value too. 
            count += currHost.cookies.length + 1;
          }
        }
        else
          ++count;
      }
      return null;
    },
    getCellText: function (aIndex, aColumn)
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) 
          return "";
        if (aColumn.id == "domainCol")
          return item.rawHost;
        else if (aColumn.id == "nameCol")
          return item.name;
      }
      return "";
    },

    _selection: null, 
    get selection () { return this._selection; },
    set selection (val) { this._selection = val; return val; },
    getRowProperties: function (aIndex, aProperties) {},
    getCellProperties: function (aIndex, aColumn, aProperties) {},
    getColumnProperties: function (aColumn, aProperties) {},
    isContainer: function (aIndex)
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return false;
        return ("cookies" in item);
      }
      return false;
    },
    isContainerOpen: function (aIndex) 
    { 
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return false;
        return item.open;
      }
      return false;
    },
    isContainerEmpty: function (aIndex) 
    { 
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return false;
        return item.cookies.length == 0;
      }
      return false;
    },
    isSeparator: function (aIndex) { return false; },    
    isSorted: function (aIndex) { return false; },    
    canDrop: function (aIndex, aOrientation) { return false; },    
    drop: function (aIndex, aOrientation) {},    
    getParentIndex: function (aIndex) 
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return -1;
        return item.parentIndex;
      }
      return -1;
    },    
    hasNextSibling: function (aParentIndex, aIndex) 
    { 
      if (!this._filtered) {
        var cookie = this._getItemAtIndex(aIndex);
        if (!cookie) return false;
        var parent = this._getItemAtIndex(cookie.parentIndex);
        if (!parent) return false;
        if ("cookies" in parent)
          return aIndex < cookie.parentIndex + parent.cookies.length;
      }
      return false;
    },
    getLevel: function (aIndex) 
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return 0;
        return item.level;
      }
      return 0;
    },
    getImageSrc: function (aIndex, aColumn) {},    
    getProgressMode: function (aIndex, aColumn) {},    
    getCellValue: function (aIndex, aColumn) {},
    setTree: function (aTree) {},    
    toggleOpenState: function (aIndex) 
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return;
        var multiplier = item.open ? -1 : 1;
        var delta = multiplier * item.cookies.length;
        this._rowCount += delta;
        item.open = !item.open;
        gCookiesWindow._tree.treeBoxObject.rowCountChanged(aIndex + 1, delta);
        gCookiesWindow._tree.treeBoxObject.invalidateRow(aIndex);
      }
    },    
    cycleHeader: function (aColumn) {},    
    selectionChanged: function () {},    
    cycleCell: function (aIndex, aColumn) {},    
    isEditable: function (aIndex, aColumn) 
    { 
      return false; 
    },
    setCellValue: function (aIndex, aColumn, aValue) {},    
    setCellText: function (aIndex, aColumn, aValue) {},    
    performAction: function (aAction) {},  
    performActionOnRow: function (aAction, aIndex) {},    
    performActionOnCell: function (aAction, aindex, aColumn) {}
  },
  
  _loadCookies: function () 
  {
    var e = this._cm.enumerator;
    var count = 0;
    var hostCount = 0;
    while (e.hasMoreElements()) {
      var cookie = e.getNext().QueryInterface(Components.interfaces.nsICookie);
      if (!cookie) break;
      var host = cookie.host;
      var formattedHost = host.charAt(0) == "." ? host.substring(1, host.length) : host;
      var strippedHost = formattedHost.substring(0, 4) == "www." ? formattedHost.substring(4, formattedHost.length) : formattedHost;
      if (!(strippedHost in this._hosts)) {
        this._hosts[strippedHost] = { cookies   : [], 
                                      rawHost   : strippedHost,
                                      level     : 0,
                                      open      : false };
        ++hostCount;
      }
      
      var c = { id          : count++,
                name        : cookie.name,
                value       : cookie.value,
                isDomain    : cookie.isDomain,
                host        : formattedHost,
                rawHost     : strippedHost,
                path        : cookie.path,
                isSecure    : cookie.isSecure,
                expires     : cookie.expires,
                level       : 1 };
      this._hosts[strippedHost].cookies.push(c);
    }
    
    this._view._rowCount = hostCount;
    this._tree.treeBoxObject.view = this._view;
    
    this.sort("rawHost");
    
    document.getElementById("removeAllCookies").disabled = count == 0;
  },
  
  formatExpiresString: function (aExpires) 
  {
    if (aExpires) {
      var date = new Date(1000 * aExpires);
      return this._ds.FormatDateTime("", this._ds.dateFormatLong,
                                     this._ds.timeFormatSeconds,
                                     date.getFullYear(),
                                     date.getMonth() + 1,
                                     date.getDate(),
                                     date.getHours(),
                                     date.getMinutes(),
                                     date.getSeconds());
    }
    return this._bundle.getString("AtEndOfSession");
  },
  
  onCookieSelected: function () 
  {
    var ci = this._tree.view.selection.currentIndex;
    if (ci == -1 || this._cookies.length == 0) {
      var properties = ["name", "value", "host", "path", "expires", "isDomain", "isSecure"];
      for (var i = 0; i < properties.length; ++i) 
        document.getElementById(properties[i]).value = "";
      return;
    }

    properties = { 
      name      : this._cookies[ci].name,
      value     : this._cookies[ci].value,
      host      : this._cookies[ci].host,
      path      : this._cookies[ci].path,
      expires   : this._cookies[ci].expires,
      isDomain  : this._cookies[ci].isDomain ? this._bundle.getString("domainColon")
                                             : this._bundle.getString("hostColon"),
      isSecure  : this._cookies[ci].isSecure ? this._bundle.getString("forSecureOnly")
                                             : this._bundle.getString("forAnyConnection"),
    };
    
    for (var property in properties)
      document.getElementById(property).value = properties[property];
  },

  deleteCookie: function () 
  { 
    if (!this._view.rowCount)
      return;
      
    var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    var blockFutureCookies = psvc.getBoolPref("network.cookie.blockFutureCookies");
    var removedCookies = [];
    gTreeUtils.deleteSelectedItems(this._tree, this._view, this._cookies, removedCookies);
    for (var i = 0; i < removedCookies.length; ++i) {
      var c = removedCookies[i];
      this._cm.remove(c.host, c.name, c.path, blockFutureCookies);
    }    
    document.getElementById("removeCookie").disabled = !this._cookies.length;
    document.getElementById("removeAllCookies").disabled = !this._cookies.length;
  },
  
  deleteAllCookies: function ()
  {
    if (!this._view.rowCount)
      return;
    this._cm.removeAll();
    this._cookies = [];
    document.getElementById("removeCookie").disabled = true;
    document.getElementById("removeAllCookies").disabled = true;  
  },
  
  onCookieKeyPress: function (aEvent)
  {
    if (aEvent.keyCode == 46)
      this.deleteCookie();
  },
  
  _lastCookieSortColumn   : "",
  _lastCookieSortAscending: false,
  
  sort: function (aColumn) 
  {
    this._lastCookieSortAscending = gTreeUtils.sort(this._tree,
                                                    this._view,
                                                    this._cookies,
                                                    aColumn,
                                                    this._lastCookieSortColumn,
                                                    this._lastCookieSortAscending);
    this._lastCookieSortColumn = aColumn;
  }
};

