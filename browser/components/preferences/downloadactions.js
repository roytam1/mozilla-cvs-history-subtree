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
# The Original Code is the Download Actions Manager.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are 
# Copyright (C) 2000, 2001, 2003, 2005
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

const kPluginHandlerContractID = "@mozilla.org/content/plugin/document-loader-factory;1";
const kDisabledPluginTypesPref = "plugin.disable_full_page_plugin_for_types";
const kShowPluginsInList = "browser.download.show_plugins_in_list";
const kHideTypesWithoutExtensions = "browser.download.hide_plugins_without_extensions";
const kRootTypePrefix = "urn:mimetype:";

///////////////////////////////////////////////////////////////////////////////
// MIME Types Datasource RDF Utils
function NC_URI(aProperty)
{
  return "http://home.netscape.com/NC-rdf#" + aProperty;
}

function MIME_URI(aType)
{
  return "urn:mimetype:" + aType;
}

function HANDLER_URI(aHandler)
{
  return "urn:mimetype:handler:" + aHandler;
}

function APP_URI(aType)
{
  return "urn:mimetype:externalApplication:" + aType;
}

var gDownloadActionsWindow = {  
  _tree         : null,
  _editButton   : null,
  _removeButton : null,
  _actions      : [],
  _plugins      : {},
  _bundle       : null,
  _pref         : Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch),
  _mimeSvc      : Components.classes["@mozilla.org/uriloader/external-helper-app-service;1"]
                            .getService(Components.interfaces.nsIMIMEService),
  _excludingPlugins           : false,
  _excludingMissingExtensions : false,
  
  init: function ()
  {
    (this._editButton = document.getElementById("editFileHandler")).disabled = true;
    (this._removeButton = document.getElementById("removeFileHandler")).disabled = true;    

    var pbi = this._pref.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
    pbi.addObserver(kShowPluginsInList, this, false);
    pbi.addObserver(kHideTypesWithoutExtensions, this, false);
    
    // Initialize the File Type list
    this._bundle = document.getElementById("bundlePreferences");
    this._tree = document.getElementById("fileHandlersList");
    this._loadPluginData();
    this._loadMIMERegistryData();
    this._view._rowCount = this._actions.length;

    // Determine any exclusions being applied - e.g. don't show types for which
    // only a plugin handler exists, don't show types lacking extensions, etc. 
    this._updateExclusions();    
    this._tree.treeBoxObject.view = this._view;    
    
    var indexToSelect = parseInt(this._tree.getAttribute("lastSelected"));
    if (indexToSelect < this._tree.view.rowCount)
      this._tree.view.selection.select(indexToSelect);
    this._tree.focus();    
  },
  
  uninit: function ()
  {
    var pbi = this._pref.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
    pbi.removeObserver(kShowPluginsInList, this);
    pbi.removeObserver(kHideTypesWithoutExtensions, this);
  },
  
  observe: function (aSubject, aTopic, aData)
  {
    if (aTopic == "nsPref:changed" &&
        (aData == kShowPluginsInList || aData == kHideTypesWithoutExtensions)) {
      this._updateExclusions();
      this._tree.treeBoxObject.invalidate();    
    }
  },
  
  _updateExclusions: function ()
  {
    this._excludingPlugins = !this._pref.getBoolPref(kShowPluginsInList);
    this._excludingMissingExtensions = this._pref.getBoolPref(kHideTypesWithoutExtensions);    
    this._view._exclusionSet = [].concat(this._actions);
    var usingExclusionSet = false;
    if (this._excludingMissingExtensions) {
      usingExclusionSet = true;
      for (var i = 0; i < this._view._exclusionSet.length;) {
        if (!this._view._exclusionSet[i].hasExtension)
          this._view._exclusionSet.splice(i, 1);
        else
          ++i;
      }
    }
    if (this._excludingPlugins) {
      usingExclusionSet = true;
      for (i = 0; i < this._view._exclusionSet.length;) {
        if (this._view._exclusionSet[i].handledOnlyByPlugin)
          this._view._exclusionSet.splice(i, 1);
        else
          ++i        
      }      
    }
    this._view._rowCount = 0;
    this._tree.treeBoxObject.rowCountChanged(0, -this._view._rowCount);
    if (usingExclusionSet) {
      this._view._usingExclusionSet = true;
      this._view._rowCount = this._view._exclusionSet.length;
    }
    else {
      this._view._usingExclusionSet = false;
      this._view._rowCount = this._view._filtered ? this._view._filterSet.length 
                                                  : this._actions.length;
    }
    this._tree.treeBoxObject.rowCountChanged(0, this._view._rowCount);
  },
  
  _loadPluginData: function ()
  {
    // Read enabled plugin type information from the category manager
    var disabled = "";
    if (this._pref.prefHasUserValue(kDisabledPluginTypesPref)) 
      disabled = this._pref.getCharPref(kDisabledPluginTypesPref);
    
    for (var i = 0; i < navigator.plugins.length; ++i) {
      var plugin = navigator.plugins[i];
      for (var j = 0; j < plugin.length; ++j) {
        var actionName = this._bundle.getFormattedString("openWith", [plugin.name])
        var type = plugin[j].type;
        var action = this._createAction(type, actionName, true, 
                                        FILEACTION_OPEN_PLUGIN, null,
                                        true, disabled.indexOf(type) == -1);
        this._plugins[action.type] = action;
        action.handledOnlyByPlugin = true;
      }
    }
  },

  _createAction: function (aMIMEType, aActionName, 
                           aIsEditable, aHandleMode, aCustomHandler,
                           aPluginAvailable, aPluginEnabled)
  {
    var newAction = !(aMIMEType in this._plugins);
    var action = newAction ? new FileAction() : this._plugins[aMIMEType];
    action.type = aMIMEType;
    var info = this._mimeSvc.getFromTypeAndExtension(action.type, null);
    
    // File Extension
    try {
      action.extension = info.primaryExtension;
    }
    catch (e) {
      action.extension = this._bundle.getString("extensionNone");
      action.hasExtension = false;
    }
    
    // Large and Small Icon
    try {
      action.smallIcon = "moz-icon://goat." + info.primaryExtension + "?size=16";
      action.bigIcon = "moz-icon://goat." + info.primaryExtension + "?size=32";
    }
    catch (e) {
      action.smallIcon = "moz-icon://goat?size=16&contentType=" + info.MIMEType;
      action.bigIcon = "moz-icon://goat?contentType=" + info.MIMEType + "&size=32";
    }

    // Pretty Type Name
    if (info.description == "") {
      try {
        action.typeName = this._bundle.getFormattedString("fileEnding", [info.primaryExtension.toUpperCase()]);
      }
      catch (e) { 
        // Wow, this sucks, just show the MIME type as a last ditch effort to display
        // the type of file that this is. 
        action.typeName = info.MIMEType;
      }
    }
    else
      action.typeName = info.description;

    // Pretty Action Name
    action.action = aActionName;

    action.id = MIME_URI(action.type);
    action.pluginAvailable = aPluginAvailable;
    action.pluginEnabled = aPluginEnabled;
    action.editable = aIsEditable;
    action.handleMode = aHandleMode;
    action.customHandler = aCustomHandler;
    action.mimeInfo = info;
    
    if (newAction)
      this._actions.push(action);
    return action;
  },
  
  _loadMIMEDS: function ()
  {
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    
    var file = fileLocator.get("UMimTyp", Components.interfaces.nsIFile);

    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
    var fileHandler = ioService.getProtocolHandler("file")
                               .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    this._mimeDS = this._rdf.GetDataSourceBlocking(fileHandler.getURLSpecFromFile(file));
  },
  
  _getLiteralValue: function (aResource, aProperty)
  {
    var property = this._rdf.GetResource(NC_URI(aProperty));
    var value = this._mimeDS.GetTarget(aResource, property, true);
    if (value)
      return value.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    return "";
  },
  
  _getChildResource: function (aResource, aProperty)
  {
    var property = this._rdf.GetResource(NC_URI(aProperty));
    return this._mimeDS.GetTarget(aResource, property, true);
  },
  
  _getDisplayNameForFile: function (aFile)
  {
#ifdef XP_WIN
    var lfw = aFile.QueryInterface(Components.interfaces.nsILocalFileWin);
    return lfw.getVersionInfoField("FileDescription"); 
#else
    // XXXben - Read the bundle name on OS X.
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var url = ios.newFileURI(aFile).QueryInterface(Components.interfaces.nsIURL);
    return url.fileName;
#endif
  },  
  
  _loadMIMERegistryData: function ()
  {
    this._rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                          .getService(Components.interfaces.nsIRDFService);
    this._loadMIMEDS();                          
                          
    var root = this._rdf.GetResource("urn:mimetypes:root");
    var container = Components.classes["@mozilla.org/rdf/container;1"]
                              .createInstance(Components.interfaces.nsIRDFContainer);
    container.Init(this._mimeDS, root);
    
    var elements = container.GetElements();
    while (elements.hasMoreElements()) {
      var type = elements.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      var editable = this._getLiteralValue(type, "editable") == "true";
      if (!editable)
        continue;
      
      var handler = this._getChildResource(type, "handlerProp");
      var alwaysAsk = this._getLiteralValue(handler, "alwaysAsk") == "true";
      if (alwaysAsk)
        continue;
      var saveToDisk        = this._getLiteralValue(handler, "saveToDisk") == "true";
      var useSystemDefault  = this._getLiteralValue(handler, "useSystemDefault") == "true";
      var editable          = this._getLiteralValue(handler, "editable") == "true";
      var handledInternally = this._getLiteralValue(handler, "handleInternal") == "true";
      var externalApp       = this._getChildResource(handler, "externalApplication");
      var externalAppPath   = this._getLiteralValue(externalApp, "path");
      var customHandler = Components.classes["@mozilla.org/file/local;1"]
                                    .createInstance(Components.interfaces.nsILocalFile);
      customHandler.initWithPath(externalAppPath);
      
      var mimeType = this._getLiteralValue(type, "value");
      var typeInfo = this._mimeSvc.getFromTypeAndExtension(mimeType, null);

      // Determine the pretty name of the associated action.
      var actionName = "";
      var handleMode = 0;
      if (saveToDisk) {
        // Save the file to disk
        actionName = this._bundle.getString("saveToDisk");
        handleMode = FILEACTION_SAVE_TO_DISK;
      }
      else if (useSystemDefault) {
        // Use the System Default handler
        actionName = this._bundle.getFormattedString("openWith", [typeInfo.defaultDescription]);
        handleMode = FILEACTION_OPEN_DEFAULT;
      }
      else {
        // Custom Handler
        actionName = this._bundle.getFormattedString("openWith", [this._getDisplayNameForFile(file)]);
        handleMode = FILEACTION_OPEN_CUSTOM;
      }

      if (handleInternally)
        handleMode = FILEACTION_OPEN_INTERNALLY;
      
      var pluginAvailable = mimeType in this._plugins && this._plugins[mimeType].pluginAvailable;
      var pluginEnabled = pluginAvailable && this._plugins[mimeType].pluginEnabled;
      var action = this._createAction(mimeType, actionName, editable, handleMode, 
                                      customHandler, pluginAvailable, pluginEnabled);
      action.handledOnlyByPlugin = false;
    }
  },
  
  _view: {
    _filtered           : false,
    _filterSet          : [],
    _usingExclusionSet  : false,
    _exclusionSet       : [],
    _filterValue        : "",

    _rowCount: 0,
    get rowCount() 
    { 
      return this._rowCount; 
    },
    
    get activeCollection ()
    {
      return this._filtered ? this._filterSet 
                            : this._usingExclusionSet ? this._exclusionSet 
                                                      : gDownloadActionsWindow._actions;
    },

    getItemAtIndex: function (aIndex)
    {
      return this.activeCollection[aIndex];
    },
    
    getCellText: function (aIndex, aColumn)
    {
      switch (aColumn.id) {
      case "fileExtension":
        return this.getItemAtIndex(aIndex).extension.toUpperCase();
      case "fileType":
        return this.getItemAtIndex(aIndex).typeName;
      case "fileMIMEType":
        return this.getItemAtIndex(aIndex).type;
      case "fileHandler":
        return this.getItemAtIndex(aIndex).action;
      }
      return "";
    },
    getImageSrc: function (aIndex, aColumn) 
    {
      if (aColumn.id == "fileExtension") 
        return this.getItemAtIndex(aIndex).smallIcon;
      return "";
    },
    _selection: null, 
    get selection () { return this._selection; },
    set selection (val) { this._selection = val; return val; },
    getRowProperties: function (aIndex, aProperties) {},
    getCellProperties: function (aIndex, aColumn, aProperties) {},
    getColumnProperties: function (aColumn, aProperties) {},
    isContainer: function (aIndex) { return false; },
    isContainerOpen: function (aIndex) { return false; },
    isContainerEmpty: function (aIndex) { return false; },
    isSeparator: function (aIndex) { return false; },
    isSorted: function (aIndex) { return false; },
    canDrop: function (aIndex, aOrientation) { return false; },
    drop: function (aIndex, aOrientation) {},
    getParentIndex: function (aIndex) { return -1; },
    hasNextSibling: function (aParentIndex, aIndex) { return false; },
    getLevel: function (aIndex) { return 0; },
    getProgressMode: function (aIndex, aColumn) {},    
    getCellValue: function (aIndex, aColumn) {},
    setTree: function (aTree) {},    
    toggleOpenState: function (aIndex) { },
    cycleHeader: function (aColumn) {},    
    selectionChanged: function () {},    
    cycleCell: function (aIndex, aColumn) {},    
    isEditable: function (aIndex, aColumn) { return false; },
    setCellValue: function (aIndex, aColumn, aValue) {},    
    setCellText: function (aIndex, aColumn, aValue) {},    
    performAction: function (aAction) {},  
    performActionOnRow: function (aAction, aIndex) {},    
    performActionOnCell: function (aAction, aindex, aColumn) {}
  },

  removeFileHandler: function ()
  {
  },
  
  editFileHandler: function ()
  {
    var selection = this._tree.view.selection; 
    if (selection.count != 1)
      return;

    var item = this._view.getItemAtIndex(selection.currentIndex);
    openDialog("chrome://browser/content/preferences/changeaction.xul", 
               "_blank", "modal,centerscreen", item);
  },
  
  onSelectionChanged: function ()
  {
    if (this._tree.view.rowCount == 0)
      return;
      
    var selection = this._tree.view.selection; 
    var selected = selection.count;
    this._removeButton.disabled = selected == 0;
    this._editButton.disabled = selected != 1;
    this._removeButton.label = this._bundle.getString(selected > 1 ? "removeActions" : "removeAction");
    
    var canRemove = true;
    
    var rangeCount = selection.getRangeCount();
    var min = { }, max = { };
    var setLastSelected = false;
    for (var i = 0; i < rangeCount; ++i) {
      selection.getRangeAt(i, min, max);
      
      for (var j = min.value; j <= max.value; ++j) {
        if (!setLastSelected) {
          // Set the last selected index to the first item in the selection
          this._tree.setAttribute("lastSelected", j);
          setLastSelected = true;
        }

        var item = this._view.getItemAtIndex(j);
        if (!item.editable || item.handleMode == FILEACTION_OPEN_INTERNALLY)
          canRemove = false;
      }
    }
    
    if (!canRemove) {
      this._removeButton.disabled = true;
      this._editButton.disabled = true;
    }
  },
  
  _lastSortProperty : "",
  _lastSortAscending: false,
  sort: function (aProperty) 
  {
    var ascending = (aProperty == this._lastSortProperty) ? !this._lastSortAscending : true;
    function sortByProperty(a, b) 
    {
      return a[aProperty].toLowerCase().localeCompare(b[aProperty].toLowerCase());
    }
    function sortByExtension(a, b)
    {
      if (!a.hasExtension && b.hasExtension)
        return 1;
      if (!b.hasExtension && a.hasExtension)
        return -1;
      return a.extension.toLowerCase().localeCompare(b.extension.toLowerCase());
    }
    // Sort the Filtered List, if in Filtered mode
    if (!this._view._filtered) { 
      this._view.activeCollection.sort(aProperty == "extension" ? sortByExtension : sortByProperty);
      if (!ascending)
        this._view.activeCollection.reverse();
    }

    this._view.selection.clearSelection();
    this._view.selection.select(0);
    this._tree.treeBoxObject.invalidate();
    this._tree.treeBoxObject.ensureRowIsVisible(0);

    this._lastSortAscending = ascending;
    this._lastSortProperty = aProperty;
  }  
};

