const nsIBPM = Components.interfaces.nsIBrowserProfileMigrator;

function MigrationItem(aID, aKey)
{
  this._id = aID;
  this._key = aKey;
}

var MigrationWizard = {
  _items:    [new MigrationItem(nsIBPM.SETTINGS,  "settings"),
              new MigrationItem(nsIBPM.COOKIES,   "cookies"),
              new MigrationItem(nsIBPM.HISTORY,   "history"),
              new MigrationItem(nsIBPM.FORMDATA,  "formdata"),
              new MigrationItem(nsIBPM.PASSWORDS, "passwords"),
              new MigrationItem(nsIBPM.BOOKMARKS, "bookmarks"),
              new MigrationItem(nsIBPM.DOWNLOADS, "downloads")],

  _dataSources: { 
    "ie":       [0, 1, 2, 3, 4, 5], 
    "opera":    [0, 1, 2, 5, 6],
    "dogbert":  [0, 1, 5],
    "seamonkey":[0, 1, 2, 4, 5, 6]
  },
  
  _source: "",
  _itemsFlags: 0,
  _selectedIndices: [],
  _selectedProfile: null,
  _wiz: null,
  _migrator: null,
  _autoMigrate: false,

  init: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "Migration:Started", false);
    os.addObserver(this, "Migration:ItemBeforeMigrate", false);
    os.addObserver(this, "Migration:ItemAfterMigrate", false);
    os.addObserver(this, "Migration:Ended", false);
    
    if ("arguments" in window) {
      this._migrator = window.arguments[0].QueryInterface(Components.interfaces.nsIBrowserProfileMigrator);
      this._automigrate = true;
      this._wiz.currentPage = "selectProfile";
    }
    
    this._wiz = document.documentElement;
  },
  
  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "Migration:Started");
    os.removeObserver(this, "Migration:ItemBeforeMigrate");
    os.removeObserver(this, "Migration:ItemAfterMigrate");
    os.removeObserver(this, "Migration:Ended");
  },
  
  // 1 - Import Source
  onImportSourcePageShow: function ()
  {
    var importSourceGroup = document.getElementById("importSourceGroup");
    importSourceGroup.selectedItem = document.getElementById(this._source == "" ? "ie" : this._source);
  },
  
  onImportSourcePageAdvanced: function ()
  {
    this._source = document.getElementById("importSourceGroup").selectedItem.id;

    // Create the migrator for the selected source.
    var contractID = "@mozilla.org/profile/migrator;1?app=browser&type=" + this._source;
    this._migrator = Components.classes[contractID].createInstance(nsIBPM);

    if (this._source == "opera") {
      // check for more than one Opera profile
      this._wiz.currentPage.next = this._migrator.sourceHasMultipleProfiles ? "selectProfile" : "importItems";
    }
    else {
      // Don't show the Select Profile page for sources that don't support
      // multiple profiles
      this._wiz.currentPage.next = "importItems";
    }
  },
  
  // 2 - [Profile Selection]
  onSelectProfilePageShow: function ()
  {
    var profiles = document.getElementById("profiles");
    while (profiles.hasChildNodes()) 
      profiles.removeChild(profiles.firstChild);
    
    var profiles = this._migrator.sourceProfiles;
    var count = profiles.Count();
    for (var i = 0; i < count; ++i) {
      var item = document.createElement("radio");
      var str = profiles.QueryElementAt(i, Components.interfaces.nsISupportsString);
      item.id = str.data;
      item.setAttribute("label", str.data);
      profiles.appendChild(item);
    }
    
    profiles.selectedItem = profiles.firstChild;
  },
  
  onSelectProfilePageAdvanced: function ()
  {
    var profiles = document.getElementById("profiles");
    this._selectedProfile = profiles.selectedItem.id;
    
    // If we're automigrating, don't show the item selection page, just grab everything.
    if (this._automigrate) {
      this._params = nsIBPM.ALL;
      this._wiz.next = "migrating";
    }
  },
  
  // 3 - ImportItems
  onImportItemsPageShow: function ()
  {
    var dataSources = document.getElementById("dataSources");
    while (dataSources.hasChildNodes())
      dataSources.removeChild(dataSources.firstChild);
    
    var bundle = document.getElementById("bundle");
    
    var ds = this._dataSources[this._source];
    for (var i = 0; i < ds.length; ++i) {
      var item = this._items[ds[i]];
      var checkbox = document.createElement("checkbox");
      checkbox.id = item._id;
      checkbox.setAttribute("label", bundle.getString(item._key));
      dataSources.appendChild(checkbox);
      checkbox.checked = true;
    }
  },

  onImportItemsPageAdvanced: function ()
  {
    var dataSources = document.getElementById("dataSources");
    var params = 0;
    this._selectedIndices = [];
    for (var i = 0; i < dataSources.childNodes.length; ++i) {
      var checkbox = dataSources.childNodes[i];
      if (checkbox.localName == "checkbox") {
        if (checkbox.checked) {
          params |= parseInt(checkbox.id);
          this._selectedIndices.push(i);
        }
      }
    }
    this._itemsFlags = params;
  },
  
  // 4 - Migrating
  onMigratingPageShow: function ()
  {
    this._migrator.migrate(this._itemsFlags, true, this._selectedProfile);
  },

  observe: function (aSubject, aTopic, aData)
  {
    var itemToIndex = { "settings": 0, "cookies": 1, "history": 2, "formdata": 3, "passwords": 4, "bookmarks": 5, "downloads": 6 };
    switch (aTopic) {
    case "Migration:Started":
      dump("*** migration started\n");
      var items = document.getElementById("items");
      while (items.hasChildNodes())
        items.removeChild(items.firstChild);
      
      var bundle = document.getElementById("bundle");
      for (var i = 0; i < this._selectedIndices.length; ++i) {
        var index = this._selectedIndices[i];
        var label = document.createElement("label");
        label.id = this._items[index]._key;
        label.setAttribute("value", bundle.getString(this._items[index]._key));
        items.appendChild(label);
      } 
      break;
    case "Migration:ItemBeforeMigrate":
      dump("*** item about to be migrated...\n");
      var index = itemToIndex[aData];
      var item = this._items[index];
      var checkbox = document.getElementById(item._key);
      if (checkbox)
        checkbox.setAttribute("style", "font-weight: bold");
      break;
    case "Migration:ItemAfterMigrate":
      dump("*** item was migrated...\n");
      var index = itemToIndex[aData];
      var item = this._items[index];
      var checkbox = document.getElementById(item._key);
      if (checkbox)
        checkbox.removeAttribute("style");
      break;
    case "Migration:Ended":
      dump("*** migration ended\n");
      break;
    }
  }
};

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
# The Original Code is The Browser Profile Migrator.
#
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@bengoodger.com>
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

