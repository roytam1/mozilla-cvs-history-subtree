const kObserverServiceProgID = "@mozilla.org/observer-service;1";
const NC_NS = "http://home.netscape.com/NC-rdf#";

var gDownloadManager;
var gDownloadListener;

function onDownloadCancel(aEvent)
{
  gDownloadManager.cancelDownload(aEvent.target.id);
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
}

function onDownloadShow(aEvent)
{
  var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
  f.initWithPath(aEvent.target.id);

  if (f.exists())
    f.reveal();
}

function onDownloadOpen(aEvent)
{
  var f = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
  f.initWithPath(aEvent.target.id);

  if (f.exists()) {
    // XXXben security check!  
    f.launch();
  }
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
  setRDFProperty(aEvent.target.id, "DownloadAnimated", "true");
}

function onDownloadRetry(aEvent)
{

}

function Startup() 
{
  var downloadView = document.getElementById("downloadView");

  const dlmgrContractID = "@mozilla.org/download-manager;1";
  const dlmgrIID = Components.interfaces.nsIDownloadManager;
  gDownloadManager = Components.classes[dlmgrContractID].getService(dlmgrIID);

  downloadView.addEventListener("download-cancel",  onDownloadCancel,   false);
  downloadView.addEventListener("download-pause",   onDownloadPause,    false);
  downloadView.addEventListener("download-resume",  onDownloadResume,   false);
  downloadView.addEventListener("download-remove",  onDownloadRemove,   false);
  downloadView.addEventListener("download-show",    onDownloadShow,     false);
  downloadView.addEventListener("download-retry",   onDownloadRetry,    false);
  downloadView.addEventListener("download-animated",onDownloadAnimated, false);
  downloadView.addEventListener("dblclick",         onDownloadOpen,     false);

  var ds = gDownloadManager.datasource;

  downloadView.database.AddDataSource(ds);
  downloadView.builder.rebuild();
  
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

  var downloadView = document.getElementById("downloadView");
  var db = gDownloadManager.datasource;
  
  var statusArc = rdf.GetResource(NC_NS + "DownloadStatus");
  
  for (var i = downloadView.childNodes.length - 1; i >= 0; --i) {
    var currItem = downloadView.childNodes[i];
    if (currItem.getAttribute("state") == "4")
      setRDFProperty(currItem.id, "DownloadStatus", 
                     currItem.getAttribute("status-internal"));
  }
 
  gDownloadManager.endBatchUpdate();
}

var downloadDNDObserver =
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
  
var downloadViewController = {
  supportsCommand: function dVC_supportsCommand (aCommand)
  {
    switch (aCommand) {
    case "cmd_properties":
    case "cmd_remove":
    case "cmd_openfile":
    case "cmd_showinshell":
    case "cmd_selectAll":
      return true;
    }
    return false;
  },
  
  isCommandEnabled: function dVC_isCommandEnabled (aCommand)
  {    
    // XXXben
    return false;
    if (!selectionCount) return false;

    var selectedItem = gDownloadHistoryView.selectedItem;
    switch (aCommand) {
    case "cmd_openfile":
    case "cmd_showinshell":
    case "cmd_properties":
      return selectionCount == 1;
    case "cmd_remove":
      return selectionCount;
    case "cmd_selectAll":
      return gDownloadHistoryView.getRowCount() != selectionCount;
    default:
      return false;
    }
  },
  
  doCommand: function dVC_doCommand (aCommand)
  {
    var selectedItem, selectedItems, file, i;

    switch (aCommand) {
    case "cmd_openfile":
      selectedItem = gDownloadHistoryView.selectedItem;
      file = getFileForItem(selectedItem);
      file.launch();
      break;
    case "cmd_showinshell":
      selectedItem = gDownloadHistoryView.selectedItem;
      file = getFileForItem(selectedItem);
      
#ifdef XP_UNIX
      // on unix, open a browser window rooted at the parent
      file = file.QueryInterface(Components.interfaces.nsIFile);
      var parent = file.parent;
      if (parent) {
        //XXXBlake use chromeUrlForTask pref here
        const browserURL = "chrome://browser/content/browser.xul";
        window.openDialog(browserURL, "_blank", "chrome,all,dialog=no", parent.path);
      }
#else
      file.reveal();
#endif
      break;
    case "cmd_properties":
      selectedItem = gDownloadHistoryView.selectedItem;
      window.openDialog("chrome://browser/content/downloads/downloadProperties.xul",
                        "_blank", "modal,centerscreen,chrome,resizable=no", selectedItem.id);
      break;
    case "cmd_remove":
      selectedItems = gDownloadHistoryView.selectedItems;
      var selectedIndex = gDownloadHistoryView.selectedIndex;
      gDownloadManager.startBatchUpdate();
      
      // Notify the datasource that we're about to begin a batch operation
      gDownloadManager.datasource.beginUpdateBatch();
      for (i = 0; i <= selectedItems.length - 1; ++i) {
        gDownloadManager.removeDownload(selectedItems[i].id);
      }
      gDownloadManager.datasource.endUpdateBatch();

      gDownloadManager.endBatchUpdate();
      var rowCount = gDownloadHistoryView.getRowCount();
      if (selectedIndex > ( rowCount- 1))
        selectedIndex = rowCount - 1;

      gDownloadHistoryView.selectedIndex = selectedIndex;
      break;
    case "cmd_selectAll":
      gDownloadHistoryView.selectAll();
      break;
    default:
    }
  },  
  
  onEvent: function dVC_onEvent (aEvent)
  {
    switch (aEvent) {
    case "list-select":
      this.onCommandUpdate();
    }
  },

  onCommandUpdate: function dVC_onCommandUpdate ()
  {
    var cmds = ["cmd_properties", "cmd_remove",
                "cmd_openfile", "cmd_showinshell"];
    for (var command in cmds)
      goUpdateCommand(cmds[command]);
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

function buildContextMenu()
{
  var selectionCount = gDownloadHistoryView.selectedCount;
  if (!selectionCount)
    return false;

  var launchItem = document.getElementById("menuitem_launch");
  var launchSep = document.getElementById("menuseparator_launch");
  var removeItem = document.getElementById("menuitem_remove");
  var showItem = document.getElementById("menuitem_show");
  var propsItem = document.getElementById("menuitem_properties");
  var propsSep = document.getElementById("menuseparator_properties");
  showItem.hidden = selectionCount != 1;
  launchItem.hidden = selectionCount != 1;
  launchSep.hidden = selectionCount != 1;
  propsItem.hidden = selectionCount != 1;
  propsSep.hidden = selectionCount != 1;
  return true;
}
    
function cleanUpDownloads()
{
  gDownloadManager.startBatchUpdate();

  var downloadView = document.getElementById("downloadView");
  for (var i = downloadView.childNodes.length - 1; i >= 0; --i) {
    var currItem = downloadView.childNodes[i];
    if (currItem.getAttribute("state") == "1" ||
        currItem.getAttribute("state") == "2" ||
        currItem.getAttribute("state") == "3" ||
        currItem.getAttribute("state") == "4" )
      gDownloadManager.removeDownload(currItem.id);
  }
  
  gDownloadManager.endBatchUpdate();
}


