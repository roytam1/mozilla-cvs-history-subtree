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
    os.addObserver(this, "cookie-changed", false);
    os.addObserver(this, "perm-changed", false);
    
    this._bundle = document.getElementById("bundlePreferences");
    this._tree = document.getElementById("cookiesList");
    
    this._loadCookies();
    this._tree.treeBoxObject.view = this._view;
    // this.sort("rawHost");
    this._tree.view.selection.select(0);
    this._tree.focus();
  },
  
  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "cookie-changed");
    os.removeObserver(this, "perm-changed");
  },
  
  observe: function (aSubject, aTopic, aData) 
  {
    dump("*** observe = " + aSubject + " / " + aTopic + " / " + aData + "\n");
  },

  _view: {
    _filtered : false,
    _filterSet: [],
    _rowCount : 0,
    get rowCount() 
    { 
      return this._rowCount; 
    },
    
    _getItemAtIndex: function (aIndex)
    {
      if (this._filtered)
        return this._filterSet[aIndex];
        
      var count = 0, hostIndex = 0;
      for (var host in gCookiesWindow._hosts) {
        var currHost = gCookiesWindow._hosts[host];
        if (!currHost) continue;
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

    _removeItemAtIndex: function (aIndex, aCount)
    {
      var removeCount = aCount === undefined ? 1 : aCount;
      if (this._filtered) {
        this._filterSet.splice(aIndex, removeCount);
        return;
      }
      
      var item = this._getItemAtIndex(aIndex);
      if (!item) return;
      if (item.container)
        gCookiesWindow._hosts[item.rawHost] = null;
      else {
        var parent = this._getItemAtIndex(item.parentIndex);
        for (var i = 0; i < parent.cookies.length; ++i) {
          if (item.id == parent.cookies[i].id) 
            parent.cookies.splice(i, removeCount);
        }
      }
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
      else {
        if (aColumn.id == "domainCol")
          return this._filterSet[aIndex].rawHost;
        else if (aColumn.id == "nameCol")
          return this._filterSet[aIndex].name;
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
        return item.container;
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
        // |aParentIndex| appears to be bogus, but we can get the real
        // parent index by getting the entry for |aIndex| and reading the
        // parentIndex field. 
        // The index of the last item in this host collection is the 
        // index of the parent + the size of the host collection, and
        // aIndex has a next sibling if it is less than this value.
        var item = this._getItemAtIndex(aIndex);
        if (item) {
          if (item.container) {
            for (var i = aIndex + 1; i < this.rowCount; ++i) {
              var subsequent = this._getItemAtIndex(i);
              if (subsequent.container) 
                return true;
            }
            return false;
          }
          else {
            var parent = this._getItemAtIndex(item.parentIndex);
            if (parent && parent.container)
              return aIndex < item.parentIndex + parent.cookies.length;
          }
        }
      }
      return aIndex < this.rowCount - 1;
    },
    hasPreviousSibling: function (aIndex)
    {
      if (!this._filtered) {
        var item = this._getItemAtIndex(aIndex);
        if (!item) return false;
        var parent = this._getItemAtIndex(item.parentIndex);
        if (parent && parent.container)
          return aIndex > item.parentIndex + 1;
      }
      return aIndex > 0;
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
    this._hosts = { };
    while (e.hasMoreElements()) {
      var cookie = e.getNext().QueryInterface(Components.interfaces.nsICookie);
      if (!cookie) break;
      var host = cookie.host;
      var formattedHost = host.charAt(0) == "." ? host.substring(1, host.length) : host;
      var strippedHost = formattedHost.substring(0, 4) == "www." ? formattedHost.substring(4, formattedHost.length) : formattedHost;
      if (!(strippedHost in this._hosts) || !this._hosts[strippedHost]) {
        this._hosts[strippedHost] = { cookies   : [], 
                                      rawHost   : strippedHost,
                                      level     : 0,
                                      open      : false,
                                      container : true };
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
                level       : 1,
                container   : false };
      this._hosts[strippedHost].cookies.push(c);
    }
    
    this._view._rowCount = hostCount;
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
    var properties, item;
    var seln = this._tree.view.selection;
    if (!this._view._filtered) 
      item = this._view._getItemAtIndex(seln.currentIndex);
    else
      item = this._view._filterSet[seln.currentIndex];
      
    var ids = ["nameLabel", "name", "valueLabel", "value", "isDomain", "host", 
               "pathLabel", "path", "isSecureLabel", "isSecure", "expiresLabel", 
               "expires"];
    var someSelected = seln.count > 0;
    if (item && !item.container && someSelected) {
      properties = { name: item.name, value: item.value, host: item.host,
                     path: item.path, expires: item.expires, 
                     isDomain: item.isDomain ? this._bundle.getString("domainColon")
                                             : this._bundle.getString("hostColon"),
                     isSecure: item.isSecure ? this._bundle.getString("forSecureOnly")
                                             : this._bundle.getString("forAnyConnection") };
      for (var i = 0; i < ids.length; ++i)
        document.getElementById(ids[i]).disabled = false;
    }
    else {
      var noneSelected = this._bundle.getString("noCookieSelected");
      properties = { name: noneSelected, value: noneSelected, host: noneSelected,
                     path: noneSelected, expires: noneSelected, 
                     isSecure: noneSelected };
      for (i = 0; i < ids.length; ++i)
        document.getElementById(ids[i]).disabled = true;
    }
    for (var property in properties)
      document.getElementById(property).value = properties[property];

    var rangeCount = seln.getRangeCount();
    var selectedCookieCount = 0;
    for (i = 0; i < rangeCount; ++i) {
      var min = { }; var max = { };
      seln.getRangeAt(i, min, max);
      for (var j = min.value; j <= max.value; ++j) {
        item = this._view._getItemAtIndex(j);
        if (!item) continue;
        if (item.container && !item.open)
          selectedCookieCount += item.cookies.length;
        else if (!item.container)
          ++selectedCookieCount;
      }
    }
    var item = this._view._getItemAtIndex(seln.currentIndex);
    if (item && seln.count == 1 && item.container && item.open)
      selectedCookieCount += 2;
      
    var stringKey = selectedCookieCount == 1 ? "removeCookie" : "removeCookies";
    document.getElementById("removeCookie").label = this._bundle.getString(stringKey);
    
    document.getElementById("removeAllCookies").disabled = this._view._filtered || (seln.count == 0);
    document.getElementById("removeCookie").disabled = !someSelected;
    
  },
  
  deleteCookie: function () 
  { 
#   // Selection Notes
#   // - Selection always moves to *NEXT* adjacent item unless item
#   //   is last child at a given level in which case it moves to *PREVIOUS*
#   //   item
#   //
#   // Selection Cases (Somewhat Complicated)
#   // 
#   // 1) Single cookie selected, host has single child
#   //    v cnn.com
#   //    //// cnn.com ///////////// goksdjf@ ////
#   //    > atwola.com
#   //
#   //    Before SelectedIndex: 1   Before RowCount: 3
#   //    After  SelectedIndex: 0   After  RowCount: 1
#   //
#   // 2) Host selected, host open
#   //    v goats.com ////////////////////////////
#   //         goats.com             sldkkfjl
#   //         goat.scom             flksj133
#   //    > atwola.com
#   //
#   //    Before SelectedIndex: 0   Before RowCount: 4
#   //    After  SelectedIndex: 0   After  RowCount: 1
#   //
#   // 3) Host selected, host closed
#   //    > goats.com ////////////////////////////
#   //    > atwola.com
#   //
#   //    Before SelectedIndex: 0   Before RowCount: 2
#   //    After  SelectedIndex: 0   After  RowCount: 1
#   //
#   // 4) Single cookie selected, host has many children
#   //    v goats.com
#   //         goats.com             sldkkfjl
#   //    //// goats.com /////////// flksjl33 ////
#   //    > atwola.com
#   //
#   //    Before SelectedIndex: 2   Before RowCount: 4
#   //    After  SelectedIndex: 1   After  RowCount: 3
#   //
#   // 5) Single cookie selected, host has many children
#   //    v goats.com
#   //    //// goats.com /////////// flksjl33 ////
#   //         goats.com             sldkkfjl
#   //    > atwola.com
#   //
#   //    Before SelectedIndex: 1   Before RowCount: 4
#   //    After  SelectedIndex: 1   After  RowCount: 3
    var seln = this._view.selection;
    var tbo = this._tree.treeBoxObject;
    
    if (seln.count < 1) return;
    
    var nextSelected = 0;
    var rowCountImpact = 0;
    var deleteItems = [];
    if (!this._view._filtered) {
      var ci = seln.currentIndex;
      nextSelected = ci;
      var invalidateRow = -1;
      var item = this._view._getItemAtIndex(ci);
      if (item.container) {
        rowCountImpact -= (item.open ? item.cookies.length : 0) + 1;
        deleteItems = deleteItems.concat(item.cookies);
        if (!this._view.hasNextSibling(-1, ci)) 
          --nextSelected;
        this._view._removeItemAtIndex(ci);
      }
      else {
        var parent = this._view._getItemAtIndex(item.parentIndex);
        --rowCountImpact;
        if (parent.cookies.length == 1) {
          --rowCountImpact;
          deleteItems.push(item);
          if (!this._view.hasNextSibling(-1, ci))
            --nextSelected;
          if (!this._view.hasNextSibling(-1, item.parentIndex)) 
            --nextSelected;
          this._view._removeItemAtIndex(item.parentIndex);
          invalidateRow = item.parentIndex;
        }
        else {
          deleteItems.push(item);
          if (!this._view.hasNextSibling(-1, ci)) 
            --nextSelected;
          this._view._removeItemAtIndex(ci);
        }
      }
      this._view._rowCount += rowCountImpact;
      tbo.rowCountChanged(ci, rowCountImpact);
      if (invalidateRow != -1)
        tbo.invalidateRow(invalidateRow);
    }
    else {
      var rangeCount = seln.getRangeCount();
      for (var i = 0; i < rangeCount; ++i) {
        var min = { }; var max = { };
        seln.getRangeAt(i, min, max);
        nextSelected = min.value;        
        for (var j = min.value; j <= max.value; ++j) {
          deleteItems.push(this._view._getItemAtIndex(j));
          if (!this._view.hasNextSibling(-1, max.value))
            --nextSelected;
        }
        var delta = max.value - min.value + 1;
        this._view._removeItemAtIndex(min.value, delta);
        rowCountImpact = -1 * delta;
        this._view._rowCount += rowCountImpact;
        tbo.rowCountChanged(min.value, rowCountImpact);

      }
    }
    
    var psvc = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    var blockFutureCookies = false;
    if (psvc.prefHasUserValue("network.cookie.blockFutureCookies"))
      blockFutureCookies = psvc.getBoolPref("network.cookie.blockFutureCookies");
    for (i = 0; i < deleteItems.length; ++i) {
      var item = deleteItems[i];
      dump("*** remove = " + item.host + " / " + item.name + " / " + item.path + " / " + blockFutureCookies + "\n");
      // this._cm.remove(item.host, item.name, item.path, blockFutureCookies);
    }
    
    if (nextSelected < 0)
      seln.clearSelection();
    else {
      seln.select(nextSelected);
      this._tree.focus();
    }
  },
  
  deleteAllCookies: function ()
  {
    this._cm.removeAll();
    this._hosts = {};
    
    var oldRowCount = this._view._rowCount;
    this._view._rowCount = 0;
    this._tree.treeBoxObject.rowCountChanged(0, -this._view._rowCount);
    this._view.selection.clearSelection();
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
  },
  
  clearFilter: function ()
  {
    // Revert to single-select in the tree
    this._tree.setAttribute("seltype", "single");
    
    // Clear the Filter and the Tree Display
    document.getElementById("filter").value = "";
    this._view._filtered = false;
    this._view._rowCount = 0;
    this._tree.treeBoxObject.rowCountChanged(0, -this._view._filterSet.length);
    this._view._filterSet = [];

    // Just reload the list to make sure deletions are respected
    this._loadCookies();
    this._tree.treeBoxObject.rowCountChanged(0, this._view.rowCount);

    // Restore open state
    for (var i = 0; i < this._openIndices.length; ++i)
      this._view.toggleOpenState(this._openIndices[i]);
    this._openIndices = [];
    
    // Restore selection
    this._view.selection.clearSelection();
    for (i = 0; i < this._lastSelectedRanges.length; ++i) {
      var range = this._lastSelectedRanges[i];
      this._view.selection.rangedSelect(range.min, range.max, true);
    }
    this._lastSelectedRanges = [];

    document.getElementById("cookiesIntro").value = this._bundle.getString("cookiesAll");
  },
  
  _filterCookies: function (aFilterValue)
  {
    var cookies = [];
    for (var host in gCookiesWindow._hosts) {
      var currHost = gCookiesWindow._hosts[host];
      if (!currHost) continue;
      for (var i = 0; i < currHost.cookies.length; ++i) {
        var cookie = currHost.cookies[i];
        if (cookie.rawHost.indexOf(aFilterValue) != -1 ||
            cookie.name.indexOf(aFilterValue) != -1 || 
            cookie.value.indexOf(aFilterValue) != -1)
          cookies.push(cookie);
      }
    }
    return cookies;
  },
  
  _lastSelectedRanges: [],
  _openIndices: [],
  _saveState: function ()
  {
    // Save selection
    var seln = this._view.selection;
    this._lastSelectedRanges = [];
    var rangeCount = seln.getRangeCount();
    for (var i = 0; i < rangeCount; ++i) {
      var min = { }; var max = { };
      seln.getRangeAt(i, min, max);
      this._lastSelectedRanges.push({ min: min.value, max: max.value });
    }
  
    // Save open states
    this._openIndices = [];
    for (i = 0; i < this._view.rowCount; ++i) {
      var item = this._view._getItemAtIndex(i);
      if (item && item.container && item.open)
        this._openIndices.push(i);
    }
  },
  
  _filterTimeout: -1,
  onFilterInput: function ()
  {
    if (this._filterTimeout != -1)
      clearTimeout(this._filterTimeout);
   
    function filterCookies()
    {
      var filter = document.getElementById("filter").value;
      if (filter == "") {
        gCookiesWindow.clearFilter();
        return;
      }        
      var view = gCookiesWindow._view;
      view._filterSet = gCookiesWindow._filterCookies(filter);
      if (!view._filtered) {
        // Save Display Info for the Non-Filtered mode when we first
        // enter Filtered mode. 
        gCookiesWindow._saveState();
        view._filtered = true;
      }
      // Move to multi-select in the tree
      gCookiesWindow._tree.setAttribute("seltype", "multiple");
      
      // Clear the display
      var oldCount = view._rowCount;
      view._rowCount = 0;
      gCookiesWindow._tree.treeBoxObject.rowCountChanged(0, -oldCount);
      // Set up the filtered display
      view._rowCount = view._filterSet.length;
      gCookiesWindow._tree.treeBoxObject.rowCountChanged(0, view.rowCount);
      
      view.selection.select(0);
      document.getElementById("cookiesIntro").value = gCookiesWindow._bundle.getString("cookiesFiltered");
    }
    window.filterCookies = filterCookies;
    this._filterTimeout = setTimeout("filterCookies();", 500);
  },
  
  focusFilterBox: function ()
  {
    document.getElementById("filter").focus();
  }
};

