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

var gChangeActionDialog = {
  _rdf            : null,
  _itemRes        : null,
  _helperApps     : null,
  _ncURI          : null,
  _handlerPropArc : null,
  _externalAppArc : null,
  _fileHandlerArc : null,
  _handlerRes     : null,
  _extAppRes      : null,
  _lastSelected   : null,

  init: function ()
  {
    this._rdf = window.opener.gRDF;
    this._itemRes = window.arguments[0];
    
    this._helperApps = window.opener.gDownloadActionsDialog._helperApps;
    this._handlerPropArc = this._helperApps._handlerPropArc;
    this._externalAppArc = this._helperApps._externalAppArc;
    this._fileHandlerArc = this._helperApps._fileHandlerArc;
    
    this._ncURI = window.opener.NC_URI;
    
    var typeField = document.getElementById("typeField");
    typeField.value = this._helperApps.getLiteralValue(this._itemRes.Value, "FileType");
    
    var extensionField = document.getElementById("extensionField");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var ext = "." + this._helperApps.getLiteralValue(this._itemRes.Value, "FileExtension").toLowerCase();
    var contentType = this._helperApps.getLiteralValue(this._itemRes.Value, "FileMIMEType");
    extensionField.value = bundlePreferences.getFormattedString("extensionStringFormat", [ext, contentType]);
    
    var typeIcon = document.getElementById("typeIcon");
    typeIcon.src = this._helperApps.getLiteralValue(this._itemRes.Value, "LargeFileIcon");

    var handlerGroup = document.getElementById("handlerGroup");
    
    this._handlerRes = this._helperApps.GetTarget(this._itemRes, this._handlerPropArc, true);
    if (this._handlerRes) {
      this._handlerRes = this._handlerRes.QueryInterface(Components.interfaces.nsIRDFResource);

      // Custom App Handler Path - this must be set before we set the selected
      // radio button because the selection event handler for the radio group
      // requires the extapp handler field to be non-empty for the extapp radio
      // button to be selected. 
      this._extAppRes = this._helperApps.GetTarget(this._handlerRes, this._externalAppArc, true);
      if (this._extAppRes) {
        this._extAppRes = this._extAppRes.QueryInterface(Components.interfaces.nsIRDFResource);

        var path = this._helperApps.getLiteralValue(this._extAppRes.Value, "path");
        var customApp = document.getElementById("customApp");
        customApp.setAttribute("path", path);
        try {
          var lf = Components.classes["@mozilla.org/file/local;1"]
                            .createInstance(Components.interfaces.nsILocalFile);
          lf.initWithPath(path);
          customApp.label = this._getDisplayNameForFile(lf);
          customApp.image = this._getIconURLForFile(lf);
        }
        catch (e) {
          customApp.label = "None Selected";
        }
      }
      
      // Selected Action Radiogroup
      var handleInternal = this._helperApps.getLiteralValue(this._handlerRes.Value, "useSystemDefault");
      var saveToDisk = this._helperApps.getLiteralValue(this._handlerRes.Value, "saveToDisk");
      if (handleInternal == "true")
        handlerGroup.selectedItem = document.getElementById("openDefault");
      else if (saveToDisk == "true")
        handlerGroup.selectedItem = document.getElementById("saveToDisk");
      else
        handlerGroup.selectedItem = document.getElementById("openApplication");
        
      this._lastSelected = handlerGroup.selectedItem;
    }
    else {
      // No Handler/ExtApp Resources for this type for some reason
      handlerGroup.selectedItem = document.getElementById("openDefault");
    }
    
    dump("*** bailed?1\n");
    var defaultApp = document.getElementById("defaultApp");
    var mimeInfo = this._helperApps.getMIMEInfo(this._itemRes);
    defaultApp.label = mimeInfo.defaultDescription;
    defaultApp.image = this._getIconURLForFile(mimeInfo.defaultApplicationHandler); 
    dump("*** bailed?\n");
    
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
  
  onAccept: function ()
  {
    var dirty = false;
    
    if (this._handlerRes) {  
      var handlerGroup = document.getElementById("handlerGroup");
      var value = handlerGroup.selectedItem.getAttribute("value");
      switch (value) {
      case "system":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "false");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "true");
        break;
      case "app":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "false");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "false");
        break;  
      case "save":
        this._setLiteralValue(this._handlerRes, "saveToDisk", "true");
        this._setLiteralValue(this._handlerRes, "useSystemDefault", "false");
        break;  
      }
      
      dirty = true;
    }
      
    if (this._extAppRes) {
      var customApp = document.getElementById("customApp");
      if (customApp.label != "") {
        this._setLiteralValue(this._extAppRes, "path", customApp.getAttribute("path"));
        this._setLiteralValue(this._extAppRes, "prettyName", customApp.label);
      }
      
      dirty = true;
    }
    
    if (dirty) {
      this._helperApps.flush();
     
      // Get the template builder to refresh the display by announcing our imaginary
      // "FileHandler" property has been updated. (NC:FileHandler is a property
      // that our wrapper DS uses, its value is built dynamically based on real
      // values of other properties.
      var newHandler = this._helperApps.GetTarget(this._itemRes, this._fileHandlerArc, true);
      this._helperApps.onChange(this._helperApps, this._itemRes, this._fileHandlerArc, newHandler);
      
      // ... this doesn't seem to work, so...
      var fileHandlersList = window.opener.document.getElementById("fileHandlersList");
      fileHandlersList.builder.rebuild();
    }
      
    return true;
  },
  
  doEnabling: function (aSelectedItem)
  {
    dump("*** goat1\n");
    if (aSelectedItem.id == "openApplication") {
      var customApp = document.getElementById("customApp")
      customApp.disabled = false;
      document.getElementById("changeApp").disabled = false;
      
      if (customApp.label == "" && !this.changeApp()) {
        this._lastSelected.click();
        return;
      }
    }
    else {
      document.getElementById("customApp").disabled = true;
      document.getElementById("changeApp").disabled = true;
    }
    
    this._lastSelected = aSelectedItem;
    dump("*** goat2\n");
  },
  
  changeApp: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);

    // extract the window title
    var winTitle = document.getElementById('changeApp').getAttribute('filepickertitle');
    fp.init(window, winTitle, nsIFilePicker.modeOpen);
    
    fp.appendFilters(nsIFilePicker.filterApps);
    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      var customApp = document.getElementById("customApp");
      customApp.label = this._getDisplayNameForFile(fp.file);
      customApp.setAttribute("path", fp.file.path);
      customApp.image = this._getIconURLForFile(fp.file);

      var mimeInfo = this._helperApps.getMIMEInfo(this._itemRes);
      this._extAppRes = this._rdf.GetResource("urn:mimetype:externalApplication:" + mimeInfo.MIMEType);
      
      return true;
    }
    return false;
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

