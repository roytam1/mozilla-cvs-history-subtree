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
# The Original Code is Mozilla.org Code.
# 
# The Initial Developer of the Original Code is
# Doron Rosenberg.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@bengoodger.com> (v2.0) 
#   Blake Ross <blakeross@telocity.com>
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


const kObserverServiceProgID = "@mozilla.org/observer-service;1";
const NC_NS = "http://home.netscape.com/NC-rdf#";

var gDownloadManager;
var gDownloadListener;
var gDownloadsView;

function fireEventForElement(aElement, aEventType)
{
  var e = document.createEvent("Events");
  e.initEvent("download-" + aEventType, false, true);
  
  aElement.dispatchEvent(e);
}

function onDownloadCancel(aEvent)
{
  gDownloadManager.cancelDownload(aEvent.target.id);

  setRDFProperty(aEvent.target.id, "DownloadAnimated", "false");

  gDownloadViewController.onCommandUpdate();
}

function onDownloadPause(aEvent)
{
  gDownloadManager.pauseDownload(aEvent.target.id);
  
  aEvent.target.setAttribute("status", aEvent.target.getAttribute("status-internal"));
}

function onDownloadResume(aEvent)
{
  gDownloadManager.resumeDownload(aEvent.target.id);
}

function onDownloadRemove(aEvent)
{
  if (canRemoveDownload(aEvent.target)) {
    gDownloadManager.removeDownload(aEvent.target.id);
    
    gDownloadViewController.onCommandUpdate();
  }
}

function onDownloadShow(aEvent)
{
  var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
  f.initWithPath(aEvent.target.id);

  if (f.exists()) {
#ifdef XP_UNIX
    // on unix, open a browser window rooted at the parent
    var parent = f.parent;
    if (parent) {
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
      var browserURL = pref.copyCharPref("browser.chromeURL");                          
      window.openDialog(browserURL, "_blank", "chrome,all,dialog=no", parent.path);
    }
#else
    f.reveal();
#endif
  }
}

function onDownloadOpen(aEvent)
{
  if (aEvent.target.localName == "download") {
    var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
    f.initWithPath(aEvent.target.id);

    if (f.exists()) {
      // XXXben security check!  
      f.launch();
    }
  }
}

function onDownloadOpenWith(aEvent)
{
  
}

function onDownloadProperties(aEvent)
{
  window.openDialog("chrome://mozapps/content/downloads/downloadProperties.xul",
                    "_blank", "modal,centerscreen,chrome,resizable=no", aEvent.target.id);
}

function setRDFProperty(aID, aProperty, aValue)
{
  var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

  var db = gDownloadManager.datasource;
  var propertyArc = rdf.GetResource(NC_NS + aProperty);
  
  var res = rdf.GetResource(aID);
  var node = db.GetTarget(res, propertyArc, true);
  if (node)
    db.Change(res, propertyArc, node, rdf.GetLiteral(aValue));
  else
    db.Assert(res, propertyArc, rdf.GetLiteral(aValue), true);
}

function onDownloadAnimated(aEvent)
{
  gDownloadViewController.onCommandUpdate();    

  setRDFProperty(aEvent.target.id, "DownloadAnimated", "true");
}

function onDownloadRetry(aEvent)
{

  gDownloadViewController.onCommandUpdate();
}

function Startup() 
{
  gDownloadsView = document.getElementById("downloadView");

  const dlmgrContractID = "@mozilla.org/download-manager;1";
  const dlmgrIID = Components.interfaces.nsIDownloadManager;
  gDownloadManager = Components.classes[dlmgrContractID].getService(dlmgrIID);

  gDownloadsView.addEventListener("download-cancel",  onDownloadCancel,   false);
  gDownloadsView.addEventListener("download-pause",   onDownloadPause,    false);
  gDownloadsView.addEventListener("download-resume",  onDownloadResume,   false);
  gDownloadsView.addEventListener("download-remove",  onDownloadRemove,   false);
  gDownloadsView.addEventListener("download-show",    onDownloadShow,     false);
  gDownloadsView.addEventListener("download-open",    onDownloadOpen,     false);
  gDownloadsView.addEventListener("download-retry",   onDownloadRetry,    false);
  gDownloadsView.addEventListener("download-animated",onDownloadAnimated, false);
  gDownloadsView.addEventListener("dblclick",         onDownloadOpen,     false);
  
  gDownloadsView.controllers.appendController(gDownloadViewController);

  var ds = gDownloadManager.datasource;

  gDownloadsView.database.AddDataSource(ds);
  gDownloadsView.builder.rebuild();
  
  gDownloadsView.focus();
  
  var downloadStrings = document.getElementById("downloadStrings");
  gDownloadListener = new DownloadProgressListener(document, downloadStrings);
  gDownloadManager.listener = gDownloadListener;
}

function Shutdown() 
{
  gDownloadManager.listener = null;

  // Save the status messages.
  saveStatusMessages();
  
  // Assert the current progress for all the downloads in case the window is reopened
  gDownloadManager.saveState();
}

function saveStatusMessages()
{
  gDownloadManager.startBatchUpdate();
  
  var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

  var db = gDownloadManager.datasource;
  
  var statusArc = rdf.GetResource(NC_NS + "DownloadStatus");
  
  for (var i = gDownloadsView.childNodes.length - 1; i >= 0; --i) {
    var currItem = gDownloadsView.childNodes[i];
    if (currItem.localName == "download" && 
        currItem.getAttribute("state") == "4")
      setRDFProperty(currItem.id, "DownloadStatus", 
                     currItem.getAttribute("status-internal"));
  }
 
  gDownloadManager.endBatchUpdate();
}

var gContextMenus = [ 
  ["menuitem_pause", "menuitem_cancel", "menuseparator_properties", "menuitem_properties"],
  ["menuitem_open", "menuitem_openWith", "menuitem_show", "menuseparator_properties", "menuitem_properties"],
  ["menuitem_retry", "menuitem_remove", "menuseparator_properties", "menuitem_properties"],
  ["menuitem_retry", "menuitem_remove", "menuseparator_properties", "menuitem_properties"],
  ["menuitem_resume", "menuitem_cancel", "menuseparator_properties", "menuitem_properties"]
];

function buildContextMenu(aEvent)
{
  if (aEvent.target.id != "downloadContextMenu")
    return;
    
  var popup = document.getElementById("downloadContextMenu");
  while (popup.hasChildNodes())
    popup.removeChild(popup.firstChild);
  
  if (gDownloadsView.selected) {
    var idx = parseInt(gDownloadsView.selected.getAttribute("state"));
    if (idx < 0)
      idx = 0;
    
    var menus = gContextMenus[idx];
    for (var i = 0; i < menus.length; ++i)
      popup.appendChild(document.getElementById(menus[i]).cloneNode(true));
    
    return true;
  }
  
  return false;
}

var gDownloadDNDObserver =
{
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    aDragSession.canDrop = true;
  },
  
  onDrop: function(aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  //do nothing, not a valid URL
      var name = split[1];
      saveURL(url, name, null, true, true);
    }
  },
  _flavourSet: null,  
  getSupportedFlavours: function ()
  {
    if (!this._flavourSet) {
      this._flavourSet = new FlavourSet();
      this._flavourSet.appendFlavour("text/x-moz-url");
      this._flavourSet.appendFlavour("text/unicode");
    }
    return this._flavourSet;
  }
}

function onSelect(aEvent) {
  window.updateCommands("list-select");
}

var gDownloadViewController = {
  supportsCommand: function dVC_supportsCommand (aCommand)
  {
    switch (aCommand) {
    case "cmd_cleanUp":
      return true;
    }
    return false;
  },
  
  isCommandEnabled: function dVC_isCommandEnabled (aCommand)
  {
    switch (aCommand) {
    case "cmd_cleanUp": 
      var canCleanUp = false;
      for (var i = 0; i < gDownloadsView.childNodes.length; ++i) {
        var currDownload = gDownloadsView.childNodes[i];
        if (currDownload.localName == "download" &&
            (currDownload.getAttribute("state") != "0" && 
             currDownload.getAttribute("state") != "-1" && 
             currDownload.getAttribute("state") != "4"))
          canCleanUp = true;
      }
      
      return canCleanUp;
    }
    return false;
  },
  
  doCommand: function dVC_doCommand (aCommand)
  {
    switch (aCommand) {
    case "cmd_cleanUp":
      if (this.isCommandEnabled(aCommand))
        cleanUpDownloadsList();
      break;
    }
  },  
  
  onCommandUpdate: function dVC_onCommandUpdate ()
  {
    var command = "cmd_cleanUp";
    var enabled = this.isCommandEnabled(command);
    goSetCommandEnabled(command, enabled);
  }
};

function getFileForItem(aElement)
{
  return createLocalFile(aElement.id);
}

function createLocalFile(aFilePath) 
{
  var lfContractID = "@mozilla.org/file/local;1";
  var lfIID = Components.interfaces.nsILocalFile;
  var lf = Components.classes[lfContractID].createInstance(lfIID);
  lf.initWithPath(aFilePath);
  return lf;
}

function cleanUpDownloadsList()
{
  gDownloadManager.startBatchUpdate();

  for (var i = gDownloadsView.childNodes.length - 1; i >= 0; --i) {
    var currItem = gDownloadsView.childNodes[i];
    if (currItem.localName == "download" && canRemoveDownload(currItem))
      gDownloadManager.removeDownload(currItem.id);
  }
  
  gDownloadManager.endBatchUpdate();

  gDownloadViewController.onCommandUpdate();
}

function canRemoveDownload(aDownload)
{
  return aDownload.getAttribute("state") == "1" ||
         aDownload.getAttribute("state") == "2" ||
         aDownload.getAttribute("state") == "3" ||
         aDownload.getAttribute("state") == "4";
}

