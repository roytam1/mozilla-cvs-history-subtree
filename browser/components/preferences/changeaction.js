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

var gChangeActionDialog = {
  _item     : null,
  _bundle   : null,
  _lastSelectedModeMode : null,
  _lastSelectedModeSave : null,

  init: function ()
  {
    this._item = window.arguments[0];
    this._bundle = document.getElementById("bundlePreferences");
    dump("*** ir = " + this._item.toSource() + "\n");
    
    var typeField = document.getElementById("typeField");
    typeField.value = this._item.typeName;
    
    var extensionField = document.getElementById("extensionField");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var ext = "." + this._item.extension.toLowerCase();
    var contentType = this._item.type;
    extensionField.value = this._bundle.getFormattedString("extensionStringFormat", [ext, contentType]);
    
    var typeIcon = document.getElementById("typeIcon");
    typeIcon.src = this._item.bigIcon;

    // Custom App Handler Path - this must be set before we set the selected
    // radio button because the selection event handler for the radio group
    // requires the extapp handler field to be non-empty for the extapp radio
    // button to be selected. 
    var customApp = document.getElementById("customApp");
    if (this._item.customHandler) {
      customApp.file = this._item.customHandler;
      dump("*** goat = "+ this._item.customHandler.path+"\n");
      customApp.label = this._getDisplayNameForFile(customApp.file);
      customApp.image = this._getIconURLForFile(customApp.file);
    }
    else {
      var bundlePreferences = document.getElementById("bundlePreferences");
      customApp.label = bundlePreferences.getString("downloadHelperNoneSelected");
    }

    var defaultApp = document.getElementById("defaultApp");
    defaultApp.label = this._item.mimeInfo.defaultDescription;
    defaultApp.image = this._getIconURLForFile(this._item.mimeInfo.defaultApplicationHandler); 
      
    var pluginName = document.getElementById("pluginName");
    var foundPlugin = false;
    for (var i = 0; i < navigator.plugins.length; ++i) {
      var plugin = navigator.plugins[i];
      for (var j = 0; j < plugin.length; ++j) {
        if (contentType == plugin[j].type) {
          pluginName.label = plugin.name;
          pluginName.image = "moz-icon://goat.goat?contentType=" + contentType + "&size=16";
          foundPlugin = true;
        }
      }
    }
    if (!foundPlugin) {
      pluginName.label = bundlePreferences.getString("pluginHelperNoneAvailable");
      document.getElementById("plugin").disabled = true;
    }
      
    // Selected Action Radiogroup
    var handlerGroup = document.getElementById("handlerGroup");
    if (this._item._handleMode == FILEACTION_OPEN_PLUGIN && this._item.pluginEnabled)
      handlerGroup.selectedItem = document.getElementById("plugin");
    else {
      if (this._item.handleMode == FILEACTION_OPEN_DEFAULT)
        handlerGroup.selectedItem = document.getElementById("openDefault");
      else if (this._item.handleMode == FILEACTION_SAVE_TO_DISK)
        handlerGroup.selectedItem = document.getElementById("saveToDisk");
      else
        handlerGroup.selectedItem = document.getElementById("openApplication");
    }
    this._lastSelectedMode = handlerGroup.selectedItem;
    
    // Figure out the last selected Save As mode
    var saveToOptions = document.getElementById("saveToOptions");
    this._lastSelectedSave = saveToOptions.selectedItem;

    // We don't let users open .exe files or random binary data directly 
    // from the browser at the moment because of security concerns. 
    var mimeType = mimeInfo.MIMEType;
    if (mimeType == "application/object-stream" ||
        mimeType == "application/x-msdownload") {
      document.getElementById("openApplication").disabled = true;
      document.getElementById("openDefault").disabled = true;
      handlerGroup.selectedItem = document.getElementById("saveToDisk");
    }
  },
  
  _setLiteralValue: function (aResource, aProperty, aValue)
  {
    var prop = this._rdf.GetResource(this._ncURI(aProperty));
    var val = this._rdf.GetLiteral(aValue);
    var oldVal = this._helperApps.GetTarget(aResource, prop, true);
    if (oldVal)
      this._helperApps.Change(aResource, prop, oldVal, val);
    else
      this._helperApps.Assert(aResource, prop, val, true);
  },
  
  _disableType: function (aContentType)
  {
    
    if (this._item.pluginAvailable) {
      // Since we're disabling the full page plugin for this content type, 
      // we must add it to the disabled list if it's not in there already.
      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch);
      var disabled = aContentType;
      if (prefs.prefHasUserValue(kDisabledPluginTypesPref)) {
        disabled = prefs.getCharPref(kDisabledPluginTypesPref);
        if (disabled.indexOf(aContentType) == -1) 
          disabled += "," + aContentType;
      }
      prefs.setCharPref(kDisabledPluginTypesPref, disabled);   
      
      // Also, we update the category manager so that existing browser windows
      // update.
      var catman = Components.classes["@mozilla.org/categorymanager;1"]
                             .getService(Components.interfaces.nsICategoryManager);
      catman.deleteCategoryEntry("Gecko-Content-Viewers", aContentType, false);     
    }    
  },
  
  _enableType: function (aContentType)
  {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
    // Since we're enabling the full page plugin for this content type, we must
    // look at the disabled types list and ensure that this type isn't in it.
    if (prefs.prefHasUserValue(kDisabledPluginTypesPref)) {
      var disabledList = prefs.getCharPref(kDisabledPluginTypesPref);
      if (disabledList == aContentType)
        prefs.clearUserPref(kDisabledPluginTypesPref);
      else {
        var disabledTypes = disabledList.split(",");
        var disabled = "";
        for (var i = 0; i < disabledTypes.length; ++i) {
          if (aContentType != disabledTypes[i])
            disabled += disabledTypes[i] + (i == disabledTypes.length - 1 ? "" : ",");
        }
        prefs.setCharPref(kDisabledPluginTypesPref, disabled);
      }
    }

    // Also, we update the category manager so that existing browser windows
    // update.
    var catman = Components.classes["@mozilla.org/categorymanager;1"]
                           .getService(Components.interfaces.nsICategoryManager);
    catman.addCategoryEntry("Gecko-Content-Viewers", aContentType,
                            kPluginHandlerContractID, false, true);
  },
  
  onAccept: function ()
  {
    var oldValue, newValue;

    var mimeInfo = this._helperApps.getMIMEInfo(this._itemRes);
    var handlerGroup = document.getElementById("handlerGroup");
    if (handlerGroup.selectedItem.value == "plugin") {
      this._enableType(mimeInfo.MIMEType);

      this._helperApps.enableFullPagePluginForType(mimeInfo.MIMEType, true);
      // We need to force the view in the parent window to refresh, since we've
      // effectively made a change to the datasource (even though we haven't
      // changed the actual data stored in the RDF datasource). 
      var pluginName = document.getElementById("pluginName");
      var openWith = bundleUCT.getFormattedString("openWith", [pluginName.label]);
      newValue = this._rdf.GetLiteral(openWith);
      oldValue = this._helperApps.GetTarget(this._itemRes, this._fileHandlerArc, true);
      this._helperApps.onChange(this._helperApps, this._itemRes, this._fileHandlerArc, oldValue, newValue);
      return true;
    }
    
    this._disableType(mimeInfo.MIMEType);
    
    var dirty = false;
    
    var fileHandlerString = "";
    if (this._handlerRes) {
      switch (handlerGroup.selectedItem.value) {
      case "system":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "false");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "true");
        fileHandlerString = bundleUCT.getFormattedString("openWith", [mimeInfo.defaultDescription]);
        break;
      case "app":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "false");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "false");
        if (this._extAppRes) {
          var customApp = document.getElementById("customApp");
          if (customApp.file) {
            this._setLiteralValue(this._extAppRes, "path", customApp.file.path);
            this._setLiteralValue(this._extAppRes, "prettyName", customApp.label);
            fileHandlerString = bundleUCT.getFormattedString("openWith", [customApp.label]);        
          }
        }        
        break;  
      case "save":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "true");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "false");
        fileHandlerString = bundleUCT.getString("saveToDisk");
        break;  
      }
      
      this._helperApps.flush();
     
      // Save selection
      var fileHandlersList = window.opener.document.getElementById("fileHandlersList");
      var selection = fileHandlersList.view.selection;
      var rangeCount = selection.getRangeCount();
      var ranges = [];
      for (var i = 0; i < rangeCount; ++i) {
        var min = { }; var max = { };
        selection.getRangeAt(i, min, max);
        ranges.push({ min: min.value, max: max.value });
      }
      
      // Get the template builder to refresh the display by announcing our imaginary
      // "FileHandler" property has been updated. (NC:FileHandler is a property
      // that our wrapper DS uses, its value is built dynamically based on real
      // values of other properties.
      this._helperApps.enableFullPagePluginForType(mimeInfo.MIMEType, false);
      var bundleUCT = document.getElementById("bundleUCT");
      newValue = this._rdf.GetLiteral(fileHandlerString);
      oldValue = this._helperApps.GetTarget(this._itemRes, this._fileHandlerArc, true);
      this._helperApps.onChange(this._helperApps, this._itemRes, this._fileHandlerArc, oldValue, newValue);

      // Restore selection
      for (i = 0; i < ranges.length; ++i)
        selection.rangedSelect(ranges[i].min, ranges[i].max, true);
    }
      
    return true;
  },
  
  doEnabling: function (aSelectedItem)
  {
    var defaultApp            = document.getElementById("defaultApp");
    var saveToDefault         = document.getElementById("saveToDefault");
    var saveToCustom          = document.getElementById("saveToCustom");
    var customDownloadFolder  = document.getElementById("customDownloadFolder");
    var chooseCustomDownloadFolder = document.getElementById("chooseCustomDownloadFolder");
    var saveToAskMe           = document.getElementById("saveToAskMe");
    var pluginName            = document.getElementById("pluginName");
    var changeApp             = document.getElementById("changeApp");
    var customApp             = document.getElementById("customApp");
    
    switch (aSelectedItem.id) {
    case "openDefault":
      changeApp.disabled = customApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = pluginName.disabled = true;
      defaultApp.disabled = false;
      break;
    case "openApplication":
      defaultApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = pluginName.disabled = true;
      changeApp.disabled = customApp.disabled = false;
      if (!customApp.file && !this.changeApp()) {
        this._lastSelectedMode.click();
        return;
      }
      break;
    case "saveToDisk":
      changeApp.disabled = customApp.disabled = defaultApp.disabled = pluginName.disabled = true;
      var saveToOptions = document.getElementById("saveToOptions");
      customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = !(saveToOptions.selectedItem.id == "saveToCustom");
      saveToDefault.disabled = saveToCustom.disabled = saveToAskMe.disabled = false;
      break;
    case "plugin":
      changeApp.disabled = customApp.disabled = defaultApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = true;
      pluginName.disabled = false;
      break;
    }
    this._lastSelectedMode = aSelectedItem;
  },
  
  doSaveToDiskEnabling: function (aSelectedItem)
  {
    var isSaveToCustom = aSelectedItem.id == "saveToCustom";
    var customDownloadFolder = document.getElementById("customDownloadFolder");
    var chooseCustomDownloadFolder = document.getElementById("chooseCustomDownloadFolder");
    chooseCustomDownloadFolder.disabled = customDownloadFolder.disabled = !isSaveToCustom;
    
    if (isSaveToCustom && 
        !customDownloadFolder.file && !this.changeCustomFolder()) {
      this._lastSelectedSave.click();
      return;
    }
    this._lastSelectedSave = aSelectedItem;
  },
  
  changeApp: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);

    // extract the window title
    var bundlePreferences = document.getElementById("bundlePreferences");
    var winTitle = bundlePreferences.getString("fpTitleChooseApp");
    fp.init(window, winTitle, nsIFilePicker.modeOpen);
    
    fp.appendFilters(nsIFilePicker.filterApps);
    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      var customApp = document.getElementById("customApp");
      customApp.label = this._getDisplayNameForFile(fp.file);
      dump("*** customApp.label = " + customApp.label + " = " + this._getDisplayNameForFile(fp.file) + "\n");
      customApp.file = fp.file;
      customApp.image = this._getIconURLForFile(fp.file);

      var mimeInfo = this._helperApps.getMIMEInfo(this._itemRes);
      this._extAppRes = this._rdf.GetResource("urn:mimetype:externalApplication:" + mimeInfo.MIMEType);
      
      return true;
    }
    return false;
  },
  
  changeCustomFolder: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);

    // extract the window title
    var bundlePreferences = document.getElementById("bundlePreferences");
    var winTitle = bundlePreferences.getString("fpTitleChooseDL");
    fp.init(window, winTitle, nsIFilePicker.modeGetFolder);
    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      var customDownloadFolder = document.getElementById("customDownloadFolder");
      customDownloadFolder.label = fp.file.path;
      customDownloadFolder.file = fp.file;
      customDownloadFolder.image = this._getIconURLForFile(fp.file);
      return true;
    }
    return false;
  },
  
  _getDisplayNameForFile: function (aFile)
  {
#ifdef XP_WIN
    var lfw = aFile.QueryInterface(Components.interfaces.nsILocalFileWin);
    dump("*** boaty = |" + lfw.getVersionInfoField("FileDescription") + "|\n");
    return lfw.getVersionInfoField("FileDescription"); 
#else
    // XXXben - Read the bundle name on OS X.
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var url = ios.newFileURI(aFile).QueryInterface(Components.interfaces.nsIURL);
    return url.fileName;
#endif
  },
  
  _getIconURLForFile: function (aFile)
  {
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    var urlspec = fph.getURLSpecFromFile(aFile);
    return "moz-icon://" + urlspec + "?size=16";  
  },
};

