
const kNC_NS = "http://home.netscape.com/NC-rdf#";

const kRDFSvcContractID = "@mozilla.org/rdf/rdf-service;1";
const kRDFSvcIID = Components.interfaces.nsIRDFService;
const kRDFSvc = Components.classes[kRDFSvcContractID].getService(kRDFSvcIID);

function nsBookmarksRDFLinerObserver()
{
}

nsBookmarksRDFLinerObserver.prototype = 
{
  onToggleOpenState: function (aItemIndex)
  {

  },

  onCycleHeader: function (aColumnID, aHeaderElement)
  {

  },

  onCycleCell: function (aItemIndex, aColumnID)
  {

  },

  onSelectionChanged: function ()
  {
    dump("*** onSelectionChanged!\n");
    // When the selection changes, we want to update commands.
    nsBookmarksOutlinerController.prototype.onCommandUpdate()
  },

  isEditable: function (aItemIndex, aColumnID)
  {

  },

  onSetCellText: function (aItemIndex, aColumnID, aValue)
  {

  },

  onPerformAction: function (aAction)
  {

  },

  onPerformActionOnRow: function (aAction, aItemIndex)
  {

  },

  onPerformActionOnCell: function (aAction, aItemIndex, aColumnID)
  {

  }
}

function nsBookmarksShell (aElementID, aBodyID)
{
  this.id = aElementID;
  this.bodyID = aBodyID;
  this.controller = new nsBookmarksOutlinerController();
  this.element.controllers.appendController(this.controller);
}

nsBookmarksShell.prototype = 
{
  get element()
  {
    return document.getElementById(this.id);
  },
  
  get bodyElement()
  {
    return document.getElementById(this.bodyID);
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // Double-click on outliner opens bookmark in window/bookmark info. 
  bodyClicked: function (aEvent)
  {
    if (aEvent.button != 0 || aEvent.detail != 2) 
      return;   // Only care about valid open events (double click, left button)
    var row = { };
    var col = { };
    var elt = { };
    this.element.boxObject.getCellAt(aEvent.clientX, aEvent.clientY, row, col, elt);
    var builder = this.bodyElement.builder.QueryInterface(Components.interfaces.nsIXULOutlinerBuilder);
    var itemRes = builder.getResourceAtIndex(row.value);
    url = this.getStringValue(itemRes, "URL");
    
    // Ignore "NC:" and empty urls.
    if (url.substring(0,3) == "NC:" || !url) return;
    
    if (aEvent.altKey)   
      // Show properties dialog 
      ;
    else if (this.openNewWindow)
      openDialog (getBrowserURL(), "_blank", "chrome,all,dialog=no", url);
    else
      openTopWin (url);

    aEvent.preventBubble();
  },
  
  getStringValue: function (aResource, aProperty)
  {
    var property = kRDFSvc.GetResource(kNC_NS + aProperty)
    var node = this.bodyElement.database.GetTarget(aResource, property, true);
    try {
      node = node.QueryInterface(Components.interfaces.nsIRDFResource);
    }
    catch (e) {
      try {
        node = node.QueryInterface(Components.interfaces.nsIRDFLiteral);
      }
      catch (e) {
        return "";
      }
    }
    return node.Value;
  }
}

function nsBookmarksOutlinerController (aElementID) 
{
}

nsBookmarksOutlinerController.prototype = 
{

  /////////////////////////////////////////////////////////////////////////////
  // nsIController
  supportsCommand: function (aCommand)
  {
    switch (aCommand) {
    case "cmd_bm_undo":
    case "cmd_bm_redo":
      return false;
    case "cmd_bm_cut":
    case "cmd_bm_copy":
    case "cmd_bm_paste":
    case "cmd_bm_delete":
    case "cmd_bm_selectAll":
    case "cmd_bm_open":
    case "cmd_bm_openfolder":
    case "cmd_bm_openfolderinnewwindow":
    case "cmd_bm_newbookmark":
    case "cmd_bm_newfolder":
    case "cmd_bm_newseparator":
    case "cmd_bm_find":
    case "cmd_bm_properties":
    case "cmd_bm_rename":
    case "cmd_bm_setnewbookmarkfolder":
    case "cmd_bm_setpersonaltoolbarfolder":
    case "cmd_bm_setnewsearchfolder":
    case "cmd_bm_import":
    case "cmd_bm_export":
    case "cmd_bm_fileBookmark":
      return true;
    default:
      return false;
    }
  },

  isCommandEnabled: function (aCommand)
  {
    switch (aCommand) {
    case "cmd_bm_undo":
    case "cmd_bm_redo":
      return false;
    case "cmd_bm_cut":
    case "cmd_bm_copy":
    case "cmd_bm_paste":
    case "cmd_bm_delete":
    case "cmd_bm_selectAll":
    case "cmd_bm_open":
    case "cmd_bm_openfolder":
    case "cmd_bm_openfolderinnewwindow":
    case "cmd_bm_newbookmark":
    case "cmd_bm_newfolder":
    case "cmd_bm_newseparator":
    case "cmd_bm_find":
    case "cmd_bm_properties":
    case "cmd_bm_rename":
    case "cmd_bm_setnewbookmarkfolder":
    case "cmd_bm_setpersonaltoolbarfolder":
    case "cmd_bm_setnewsearchfolder":
    case "cmd_bm_import":
    case "cmd_bm_export":
    case "cmd_bm_fileBookmark":
      return true;
    default:
      return false;
    }
  },

  doCommand: function (aCommand)
  {
    switch (aCommand) {
    case "cmd_bm_undo":
    case "cmd_bm_redo":
      return false;
    case "cmd_bm_cut":
    case "cmd_bm_copy":
    case "cmd_bm_paste":
    case "cmd_bm_delete":
    case "cmd_bm_selectAll":
    case "cmd_bm_open":
    case "cmd_bm_openfolder":
    case "cmd_bm_openfolderinnewwindow":
    case "cmd_bm_newbookmark":
    case "cmd_bm_newfolder":
    case "cmd_bm_newseparator":
    case "cmd_bm_find":
    case "cmd_bm_properties":
    case "cmd_bm_rename":
    case "cmd_bm_setnewbookmarkfolder":
    case "cmd_bm_setpersonaltoolbarfolder":
    case "cmd_bm_setnewsearchfolder":
    case "cmd_bm_import":
    case "cmd_bm_export":
    case "cmd_bm_fileBookmark":
      return true;
    default:
      return false;
    }
  },

  onEvent: function (aEvent)
  {

  },

  onCommandUpdate: function ()
  {

  }
}

