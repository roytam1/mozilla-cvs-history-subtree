///////////////////////////////////////////////////////////////////////////////
// Globals

const kObserverServiceProgID = "@mozilla.org/observer-service;1";
const NC_NS = "http://home.netscape.com/NC-rdf#";

var gDownloadManager;
var gDownloadListener;
var gDownloadsView;

///////////////////////////////////////////////////////////////////////////////
// Utility Functions 
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

function getRDFProperty(aID, aProperty)
{
  var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

  var db = gDownloadManager.datasource;
  var propertyArc = rdf.GetResource(NC_NS + aProperty);
  
  var res = rdf.GetResource(aID);
  var node = db.GetTarget(res, propertyArc, true);
  try {
    node = node.QueryInterface(Components.interfaces.nsIRDFLiteral);
    return node.Value;
  }
  catch (e) {
    try {
      node = node.QueryInterface(Components.interfaces.nsIRDFInt);
      return node.Value;
    }
    catch (e) {
      node = node.QueryInterface(Components.interfaces.nsIRDFResource);
      return node.Value;
    }
  }
  return "";
}

function fireEventForElement(aElement, aEventType)
{
  var e = document.createEvent("Events");
  e.initEvent("download-" + aEventType, false, true);
  
  aElement.dispatchEvent(e);
}

///////////////////////////////////////////////////////////////////////////////
// Start/Stop Observers
var gDownloadObserver = {
  observe: function (aSubject, aTopic, aState) 
  {
    var dl = aSubject.QueryInterface(Components.interfaces.nsIDownload);
    switch (aTopic) {
    case "dl-failed":
    case "dl-done":
    case "dl-cancel":
      // Wrap this in try...catch since this can be called while shutting down... 
      // it doesn't really matter if it fails then since well.. we're shutting down
      // and there's no UI to update!
      try {
        var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
        var rdfc = Components.classes["@mozilla.org/rdf/container;1"].createInstance(Components.interfaces.nsIRDFContainer);

        var db = gDownloadManager.datasource;
        
        rdfc.Init(db, rdf.GetResource("NC:DownloadsRoot"));

        var id = dl.target.persistentDescriptor;
        var dlRes = rdf.GetUnicodeResource(id);
        rdfc.RemoveElement(dlRes, true);
        
        var elts = rdfc.GetElements();
        var insertIndex = 1;
        while (elts.hasMoreElements()) {
          var currDL = elts.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
          // This is based on the ASSumption that the dlmgr only hashes active downloads. 
          var dl = gDownloadManager.getDownload(currDL.Value);
          if (!dl) 
            break;
          ++insertIndex;
        }
        
        if (insertIndex == rdfc.GetCount() || insertIndex < 1) 
          rdfc.AppendElement(dlRes);
        else
          rdfc.InsertElementAt(dlRes, insertIndex, true);      

        if (gDownloadPercentages[id]) {
          gDownloadPercentages[id] = false;
          --gDownloadPercentagesMeta.count;
        }
        
        if (gDownloadPercentagesMeta.count == 0)
          window.title = document.documentElement.getAttribute("statictitle");    
      }
      catch (e) {
      }
      break;
    case "dl-start":
      // Add this download to the percentage average tally
      gDownloadPercentages[dl.target.persistentDescriptor] = true;
      ++gDownloadPercentagesMeta.count;
      
      break;
      
    }
  }
};

///////////////////////////////////////////////////////////////////////////////
// Download Event Handlers

function onDownloadCancel(aEvent)
{
  gDownloadManager.cancelDownload(aEvent.target.id);

  setRDFProperty(aEvent.target.id, "DownloadAnimated", "false");

  // XXXben - 
  // If we got here because we resumed the download, we weren't using a temp file
  // because we used saveURL instead. (this is because the proper download mechanism
  // employed by the helper app service isn't fully accessible yet... should be fixed...
  // talk to bz...)
  // the upshot is we have to delete the file if it exists. 
  var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
  f.initWithPath(aEvent.target.id);
  if (f.exists()) 
    f.remove(true);

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
  if (aEvent.target.removable) {
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
  var download = aEvent.target;
  if (download.localName == "download") {
    if (download.openable) {
      var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
      f.initWithPath(aEvent.target.id);

      if (f.exists()) {
        // XXXben security check!  
        f.launch();
      }
    }
    else if(download.canceledOrFailed) {
      // If the user canceled this download, double clicking tries again. 
      fireEventForElement(download, "retry")
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

function onDownloadAnimated(aEvent)
{
  gDownloadViewController.onCommandUpdate();    

  setRDFProperty(aEvent.target.id, "DownloadAnimated", "true");
}

function onDownloadRetry(aEvent)
{
  var download = aEvent.target;
  if (download.localName == "download") {
    var src = getRDFProperty(download.id, "URL");
    saveURL(src, download.getAttribute("target"), null, true, true);
  }
  
  gDownloadViewController.onCommandUpdate();
}

// This is called by the progress listener. We don't actually use the event
// system here to minimize time wastage. 
var gDownloadPercentages = {};
var gDownloadPercentagesMeta = {
  count: 0,
  lastMean: 0
};

function onUpdateProgress()
{
  var mean = 0;
  var count = 0;
  for (var download in gDownloadPercentages) {
    if (gDownloadPercentages[download]) {
      var dl = document.getElementById(download);
      if (dl) { 
        var progress = parseInt(dl.getAttribute("progress"));
        if (progress < 100) {
          ++count;
          mean += progress;
        }
      }
    }
  }
  // Refresh count. 
  gDownloadPercentagesMeta.count = count;
  
  if (count == 0)
    window.title = document.documentElement.getAttribute("statictitle");    
  else {
    mean /= gDownloadPercentagesMeta.count;
    mean = Math.round(mean);
    
    if (mean != gDownloadPercentagesMeta.lastMean) {
      gDownloadPercentagesMeta.lastMean = mean;
      var strings = document.getElementById("downloadStrings");
      
      var title = "";
      if (gDownloadPercentagesMeta.count > 1)
        title = strings.getFormattedString("downloadsTitleMultiple", [mean, gDownloadPercentagesMeta.count]);
      else
        title = strings.getFormattedString("downloadsTitle", [mean]);

      window.title = title;
      gDownloadPercentagesMeta.lastMean = mean;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Startup, Shutdown
function Startup() 
{
  gDownloadsView = document.getElementById("downloadView");

  const dlmgrContractID = "@mozilla.org/download-manager;1";
  const dlmgrIID = Components.interfaces.nsIDownloadManager;
  gDownloadManager = Components.classes[dlmgrContractID].getService(dlmgrIID);

  gDownloadsView.addEventListener("download-cancel",      onDownloadCancel,     false);
  gDownloadsView.addEventListener("download-pause",       onDownloadPause,      false);
  gDownloadsView.addEventListener("download-resume",      onDownloadResume,     false);
  gDownloadsView.addEventListener("download-remove",      onDownloadRemove,     false);
  gDownloadsView.addEventListener("download-show",        onDownloadShow,       false);
  gDownloadsView.addEventListener("download-open",        onDownloadOpen,       false);
  gDownloadsView.addEventListener("download-retry",       onDownloadRetry,      false);
  gDownloadsView.addEventListener("download-animated",    onDownloadAnimated,   false);
  gDownloadsView.addEventListener("download-properties",  onDownloadProperties, false);
  gDownloadsView.addEventListener("dblclick",             onDownloadOpen,       false);
  
  gDownloadsView.controllers.appendController(gDownloadViewController);

  var ds = gDownloadManager.datasource;

  gDownloadsView.database.AddDataSource(ds);
  gDownloadsView.builder.rebuild();
  
  gDownloadsView.focus();
  
  var downloadStrings = document.getElementById("downloadStrings");
  gDownloadListener = new DownloadProgressListener(document, downloadStrings);
  gDownloadManager.listener = gDownloadListener;
  
  var observerService = Components.classes[kObserverServiceProgID]
                                  .getService(Components.interfaces.nsIObserverService);
  observerService.addObserver(gDownloadObserver, "dl-done",   false);
  observerService.addObserver(gDownloadObserver, "dl-cancel", false);
  observerService.addObserver(gDownloadObserver, "dl-failed", false);  
  observerService.addObserver(gDownloadObserver, "dl-start", false);  
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

///////////////////////////////////////////////////////////////////////////////
// View Context Menus
var gContextMenus = [ 
  ["menuitem_pause", "menuitem_cancel", "menuseparator_properties", "menuitem_properties"],
  ["menuitem_open", "menuitem_show", "menuseparator_properties", "menuitem_properties"],
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

///////////////////////////////////////////////////////////////////////////////
// Drag and Drop

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

///////////////////////////////////////////////////////////////////////////////
// Command Updating and Command Handlers

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

function cleanUpDownloadsList()
{
  gDownloadManager.startBatchUpdate();

  for (var i = gDownloadsView.childNodes.length - 1; i >= 0; --i) {
    var currItem = gDownloadsView.childNodes[i];
    if (currItem.localName == "download" && currItem.removable)
      gDownloadManager.removeDownload(currItem.id);
  }
  
  gDownloadManager.endBatchUpdate();

  gDownloadViewController.onCommandUpdate();
}

