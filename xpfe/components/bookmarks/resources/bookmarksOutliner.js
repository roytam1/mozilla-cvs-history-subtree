
var NC_NS  = "http://home.netscape.com/NC-rdf#";
var RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const NC_NS_CMD = NC_NS + "command?cmd=";

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
  this.controller = new nsBookmarksOutlinerController(this);
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
  
  get outlinerBuilder()
  {
    return this.bodyElement.builder.QueryInterface(Components.interfaces.nsIXULOutlinerBuilder);  
  },
  
  get db()
  {
    return this.bodyElement.database;
  },

  get firstSelectedIndex()
  {
    var boxObject = this.element.outlinerBoxObject;
    var first = { };
    var firstRange = boxObject.selection.getRangeAt(0, first, { });
    return first.value;
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // Mouse down on outliner. Need to get some mouse coords for use when figuring
  // out where we clicked during context menu creation. 
  bodyMouseDown: function (aEvent)
  {
    // Cache the event coordinates, we may need them for context menu stuff.
    this._mouseClientX = aEvent.clientX;
    this._mouseClientY = aEvent.clientY;
  },  
  
  //////////////////////////////////////////////////////////////////////////////
  // Double-click on outliner opens bookmark in window/bookmark info. 
  bodyClicked: function (aEvent)
  {
    if (aEvent.button != 0 || aEvent.detail != 2) 
      return;   // Only care about valid open events (double click, left button)
    (aEvent.altKey ? this.showProperties : this.openBookmark)()
    aEvent.preventBubble();
  },
  
  bodySelect: function (aEvent)
  {
    aEvent.target.parentNode.outlinerBoxObject.view.selectionChanged();  
    this.controller.onCommandUpdate();
  },
  
  outlinerFocused: function (aEvent)
  {
    this.controller.onCommandUpdate();
  },

  showProperties: function ()
  {
    var itemRes = this.outlinerBuilder.getResourceAtIndex(this.element.outlinerBoxObject.selection.currentIndex);
    // Show properties dialog 
    openDialog("chrome://communicator/content/bookmarks/bm-props.xul",
               "BookmarkProperties", "centerscreen,chrome,dialog=no,resizable=no", 
               itemRes.Value);
  },
  
  openBookmark: function ()
  {
    // Get the current item, and load it. 
    var itemRes = this.outlinerBuilder.getResourceAtIndex(this.element.outlinerBoxObject.selection.currentIndex);
    url = this.getStringValue(itemRes, "URL");
    
    // Ignore "NC:" and empty urls.
    if (url.substring(0,3) == "NC:" || !url) return;

    if (this.openNewWindow)
      openDialog (getBrowserURL(), "_blank", "chrome,all,dialog=no", url);
    else
      openTopWin (url);
  },
  
  getStringValue: function (aResource, aProperty)
  {
    var property = kRDFSvc.GetResource(NC_NS + aProperty)
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
  },
  
  createContextMenu: function (aEvent)
  {
    // clear out the old context menu contents (if any)
    var popup = aEvent.target;
    while (popup.hasChildNodes()) 
      popup.removeChild(popup.firstChild);
    
    var bo = this.element.outlinerBoxObject;
    
    // If the user context-clicked on a selection, then we'll use it, and set
    // currentIndex to the item clicked on. Otherwise, destroy any existing 
    // selection and select the item clicked on. 
    var row = { };
    var col = { };
    var elt = { };
    bo.getCellAt(this._mouseClientX, this._mouseClientY, row, col, elt);
    
    if (!bo.selection.isSelected(row.value))
      // Not in an existing selection, deselect everything and select this. 
      bo.selection.select(row.value);
    else  
      // Clicked in an existing selection. Update the currentIndex. 
      bo.selection.currentIndex = row.value;
    
    var selection = bo.selection;
    
    // We used to have a check here to see if type was non-null. Do we still
    // need it? 
    // var currIndex = selection.currentIndex;
    // var res = this.outlinerBuilder.getResourceAtIndex(currIndex);
    
    // Iterate over the ranges in the selection, and push the selected indices
    // onto a flat array. 
    var rangeCount = selection.getRangeCount();
    var selnIndices = [];
    for (var i = 0; i < rangeCount; ++i) {
      var from = { };
      var to = { };
      selection.getRangeAt(i, from, to);
      for (var k = from.value; k <= to.value; ++k)
        selnIndices.push(k);
    }
      
    var commonCommands = [];
    for (var i = 0; i < selnIndices.length; ++i) {
      var res = this.outlinerBuilder.getResourceAtIndex(selnIndices[i]);
      var commands = this.getAllCmds(res);
      if (!commands) {
        aEvent.preventDefault();
        return;
      }
      commands = this.flattenEnumerator(commands);
      if (!commonCommands.length) commonCommands = commands;
      commonCommands = this.findCommonNodes(commands, commonCommands);
    }

    if (!commonCommands.length) {
      aEvent.preventDefault();
      return;
    }
    
    // Now that we should have generated a list of commands that is valid
    // for the entire selection, build a context menu.
    for (i = 0; i < commonCommands.length; ++i) {
      const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
      var currCommand = commonCommands[i].QueryInterface(Components.interfaces.nsIRDFResource).Value;
      var element = null;
      if (currCommand != NC_NS_CMD + "separator") {
        var commandName = this.getCommandName(currCommand);
        element = this.createMenuItem(commandName, currCommand, selection.currentIndex);
      }
      else if (i != 0 && i < commonCommands.length-1) {
        // Never append a separator as the first or last element in a context
        // menu.
        element = document.createElementNS(kXULNS, "menuseparator");
      }
      
      if (element) 
        popup.appendChild(element);
    }
    return;
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // This method constructs a menuitem for a context menu for the given command.
  // This is implemented by the client so that it can intercept menuitem naming
  // as appropriate.
  createMenuItem: function (aDisplayName, aCommandName, aItemIndex)
  {
    const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var xulElement = document.createElementNS(kXULNS, "menuitem");
    xulElement.setAttribute("cmd", aCommandName);
    xulElement.setAttribute("command", "cmd_" + aCommandName.substring(NC_NS_CMD.length));

    switch (aCommandName) {
    case NC_NS_CMD + "bm_open":
      xulElement.setAttribute("label", aDisplayName);
      xulElement.setAttribute("default", "true");
      break;
    case NC_NS_CMD + "bm_openfolder":
      var view = this.element.outlinerBoxObject.view;
      aDisplayName = view.isContainerOpen(aItemIndex) ? this.getLocaleString("cmd_bm_openfolder2") : aDisplayName;
      xulElement.setAttribute("label", aDisplayName);
      xulElement.setAttribute("default", "true");
      break;
    case NC_NS_CMD + "bm_renamebookmark":
      // XXX What the fuck.
      if (!document.popupNode.hasAttribute("type")) {
        xulElement.setAttribute("label", this.getLocaleString("cmd_bm_renamebookmark2"));
        xulElement.setAttribute("cmd", (NC_NS_CMD + "editurl"));
      }
      else
        xulElement.setAttribute("label", aDisplayName);
      break;
    default:
      xulElement.setAttribute("label", aDisplayName);
      break;
    }
    return xulElement;
  },  

  /////////////////////////////////////////////////////////////////////////////
  // Given two unique arrays, return an array that contains only the elements
  // common to both. 
  findCommonNodes: function (aNewArray, aOldArray)
  {
    var common = [];
    for (var i = 0; i < aNewArray.length; ++i) {
      for (var j = 0; j < aOldArray.length; ++j) {
        if (common.length > 0 && common[common.length-1] == aNewArray[i])
          continue;
        if (aNewArray[i] == aOldArray[j])
          common.push(aNewArray[i]);
      }
    }
    return common;
  },

  flattenEnumerator: function (aEnumerator)
  {
    if ("_index" in aEnumerator)
      return aEnumerator._inner;
    
    var temp = [];
    while (aEnumerator.hasMoreElements()) 
      temp.push(aEnumerator.getNext());
    return temp;
  },
  
  /////////////////////////////////////////////////////////////////////////////
  // Determine the rdf:type property for the given resource.
  resolveType: function (aResource)
  {
    const krType = kRDFSvc.GetResource(RDF_NS + "type");
    const type = gBookmarksShell.db.GetTarget(aResource, krType, true);
    try {
      return type.QueryInterface(Components.interfaces.nsIRDFResource).Value;
    }
    catch (e) {
      try { 
        return type.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      }
      catch (e) {
        return null;
      }
    }    
  },
 
  /////////////////////////////////////////////////////////////////////////////
  // For a given URI (a unique identifier of a resource in the graph) return 
  // an enumeration of applicable commands for that URI. 
  getAllCmds: function (aResource)
  {
    var type = this.resolveType(aResource);
    if (!type) {
      if (aResource.Value == "NC:PersonalToolbarFolder" || aResource.Value == "NC:BookmarksRoot")
        type = "http://home.netscape.com/NC-rdf#Folder";
      else
        return null;
    }
    var commands = [];
    switch (type) {
    case "http://home.netscape.com/NC-rdf#BookmarkSeparator":
      commands = ["bm_find", "bm_separator", "bm_cut", "bm_copy", "bm_paste", 
                  "bm_delete", "separator", "bm_fileBookmark", "separator", 
                  "newfolder"];
      break;
    case "http://home.netscape.com/NC-rdf#Bookmark":
      commands = ["bm_open", "bm_find", "separator", "bm_cut", 
                  "bm_copy", "bm_paste", "bm_delete", "separator", "bm_rename",
                  "separator", "bm_fileBookmark", "separator", "bm_newfolder", 
                  "separator", "bm_properties"];
      break;
    case "http://home.netscape.com/NC-rdf#Folder":
      commands = ["bm_openfolder", "bm_openfolderinnewwindow", "bm_find", "separator", 
                  "bm_cut", "bm_copy", "bm_paste", "bm_delete", "separator", "bm_rename", 
                  "separator", "bm_fileBookmark", "separator", 
                  "bm_newfolder", "separator", "bm_properties"];
      break;
    case "http://home.netscape.com/NC-rdf#IEFavoriteFolder":
      commands = ["bm_open", "bm_find", "separator", "bm_copy", "separator", "bm_rename", "separator",
                  "bm_fileBookmark", "separator", "separator", "bm_properties"];
      break;
    case "http://home.netscape.com/NC-rdf#IEFavorite":
      commands = ["bm_open", "bm_find", "separator", "bm_copy"];
      break;
    case "http://home.netscape.com/NC-rdf#FileSystemObject":
      commands = ["bm_open", "bm_find", "separator", "bm_copy"];
      break;
    default: 
      return this.db.GetAllCmds(aResource);
    }
    return new CommandArrayEnumerator(commands);
  },

  /////////////////////////////////////////////////////////////////////////////
  // Retrieve the human-readable name for a particular command. Used when 
  // manufacturing a UI to invoke commands.
  getCommandName: function (aCommand) 
  {
    var cmdName = aCommand.substring(NC_NS_CMD.length);
    try {
      // Note: this will succeed only if there's a string in the bookmarks
      //       string bundle for this command name. Otherwise, <xul:stringbundle/>
      //       will throw, we'll catch & stifle the error, and look up the command
      //       name in the datasource. 
      return this.getLocaleString ("cmd_" + cmdName);
    }
    catch (e) {
    }   
    // XXX - WORK TO DO HERE! (rjc will cry if we don't fix this) 
    // need to ask the ds for the commands for this node, however we don't
    // have the right params. This is kind of a problem. 
    dump("*** BAD! EVIL! WICKED! NO! ACK! ARGH! ORGH!, command= " + cmdName + "\n");
    const rName = kRDFSvc.GetResource(NC_NS + "Name");
    const rSource = kRDFSvc.GetResource(aNodeID);
    return this.db.GetTarget(rSource, rName, true).Value;
  },
 
  getLocaleString: function (aStringKey)
  {
    var bundle = document.getElementById("bookmarksBundle");
    return bundle.getString (aStringKey);
  }
}

function CommandArrayEnumerator (aCommandArray)
{
  this._inner = [];
  const kRDFContractID = "@mozilla.org/rdf/rdf-service;1";
  const kRDFIID = Components.interfaces.nsIRDFService;
  const RDF = Components.classes[kRDFContractID].getService(kRDFIID);
  for (var i = 0; i < aCommandArray.length; ++i)
    this._inner.push(RDF.GetResource(NC_NS_CMD + aCommandArray[i]));
    
  this._index = 0;
}

CommandArrayEnumerator.prototype = {
  getNext: function () 
  {
    return this._inner[this._index];
  },
  
  hasMoreElements: function ()
  {
    return this._index < this._inner.length;
  }
};

function nsBookmarksOutlinerController (aShell) 
{
  this.shell = aShell;
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
    case "cmd_bm_openfolder":
      // 'Expand' is only available if one item is selected and that item is a
      // container item.
      var boxObject = this.shell.element.outlinerBoxObject;
      var view = boxObject.view;
      var selection = boxObject.selection;
      return selection.count == 1 && view.isContainer(this.shell.firstSelectedIndex);
    case "cmd_bm_open":
    case "cmd_bm_properties":
      var boxObject = this.shell.element.outlinerBoxObject;
      var selection = boxObject.selection;
      
      var res = this.shell.outlinerBuilder.getResourceAtIndex(this.shell.firstSelectedIndex);      
      var type = this.shell.resolveType(res);
      if (aCommand == "cm_bm_open")
        return selection.count == 1 && type == NC_NS + "Bookmark";
      else
        return selection.count == 1 && type == NC_NS + "Folder";
    case "cmd_bm_find":
      return true;
    case "cmd_bm_cut":
    case "cmd_bm_copy":
    case "cmd_bm_paste":
    case "cmd_bm_delete":
    case "cmd_bm_selectAll":
    case "cmd_bm_openfolderinnewwindow":
    case "cmd_bm_newbookmark":
    case "cmd_bm_newfolder":
    case "cmd_bm_newseparator":
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
    case "cmd_bm_openfolder":
      var boxObject = this.shell.element.outlinerBoxObject;
      var view = boxObject.view;
      var selection = boxObject.selection;

      var first = { };
      var last = { };
      var firstRange = selection.getRangeAt(0, first, last);
      view.toggleOpenState(first.value);
      return;
    case "cmd_bm_open":
      this.shell.openBookmark();
      return;
    case "cmd_bm_properties":
      this.shell.showProperties();
      return;
    case "cmd_bm_find":
      openDialog("chrome://communicator/content/bookmarks/findBookmark.xul",
                 "FindBookmarksWindow",
                 "dialog=no,centerscreen,resizable=no,chrome,dependent");
      return;
    case "cmd_bm_cut":
    case "cmd_bm_copy":
    case "cmd_bm_paste":
    case "cmd_bm_delete":
    case "cmd_bm_selectAll":
    case "cmd_bm_openfolderinnewwindow":
    case "cmd_bm_newbookmark":
    case "cmd_bm_newfolder":
    case "cmd_bm_newseparator":
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
    var commands = ["cmd_bm_properties", "cmd_bm_rename", "cmd_bm_copy",
                    "cmd_bm_paste", "cmd_bm_cut", "cmd_bm_delete",
                    "cmd_bm_setpersonaltoolbarfolder", 
                    "cmd_bm_setnewbookmarkfolder",
                    "cmd_bm_setnewsearchfolder", "cmd_bm_fileBookmark", 
                    "cmd_bm_openfolderinnewwindow", "cmd_bm_openfolder"];
    for (var i = 0; i < commands.length; ++i)
      goUpdateCommand(commands[i]);
  }
}

