
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


function nsBookmarksOutlinerController (aElementID) 
{
  this.id = aElementID;
}

nsBookmarksOutlinerController.prototype = 
{
  id: "",

  get element()
  {
    return document.getElementById(this.id);
  },

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

