var dirTree = 0;
var gResultsOutliner = 0;
var gAbView = null;
var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

// functions needed from abMainWindow and abSelectAddresses

// Controller object for Results Pane
var ResultsPaneController =
{
  supportsCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
      case "cmd_delete":
      case "button_delete":
      case "button_edit":
        return true;
      default:
        return false;
    }
  },

  isCommandEnabled: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        return true;

      case "cmd_delete":
      case "button_delete":
        var numSelected;
        if (gAbView && gAbView.selection)
          numSelected = gAbView.selection.count;
        else 
          numSelected = 0;

        // fix me, don't update on isCommandEnabled
        if (command == "cmd_delete") {
          if (numSelected < 2)
            goSetMenuValue(command, "valueCard");
          else
            goSetMenuValue(command, "valueCards");
        }
        return (numSelected > 0);
      case "button_edit":
        return (GetSelectedCardIndex() != -1);
      default:
        return false;
    }
  },

  doCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        if (gAbView)
          gAbView.selectAll();
        break;
      case "cmd_delete":
      case "button_delete":
        AbDelete();
        break;
      case "button_edit":
        AbEditSelectedCard();
        break;
    }
  },

  onEvent: function(event)
  {
    // on blur events set the menu item texts back to the normal values
    if (event == "blur")
      goSetMenuValue("cmd_delete", "valueDefault");
  }
};


// Controller object for Dir Pane
var DirPaneController =
{
  supportsCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
      case "cmd_delete":
      case "button_delete":
      case "button_edit":
        return true;
      default:
        return false;
    }
  },

  isCommandEnabled: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        return true;
      case "cmd_delete":
      case "button_delete":
        if (command == "cmd_delete")
          goSetMenuValue(command, "valueAddressBook");
        if (dirTree && dirTree.selectedItems)
          return true;
        else
          return false;
      case "button_edit":
        var selectedItems = dirTree.selectedItems;
        if (selectedItems.length == 1) {
          var mailingListUri = selectedItems[0].getAttribute('id');
          var directory = rdf.GetResource(mailingListUri).QueryInterface(Components.interfaces.nsIAbDirectory);
          if (directory.isMailList)
             return true;
        }
        return false;
      default:
        return false;
    }
  },

  doCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        if (dirTree)
          dirTree.selectAll();
        break;

      case "cmd_delete":
      case "button_delete":
        if (dirTree)
          AbDeleteDirectory();
//        top.addressbook.deleteAddressBooks(dirTree.database, dirTree, dirTree.selectedItems);
        break;
      case "button_edit":
        AbEditSelectedDirectory();
        break;
    }
  },

  onEvent: function(event)
  {
    // on blur events set the menu item texts back to the normal values
    if (event == "blur")
      goSetMenuValue("cmd_delete", "valueDefault");
  }
};

function AbEditSelectedDirectory()
{
  var selectedItems = dirTree.selectedItems;
  if (selectedItems.length == 1) {
    var mailingListUri = selectedItems[0].getAttribute('id');
    var directory = rdf.GetResource(mailingListUri).QueryInterface(Components.interfaces.nsIAbDirectory);
    if (directory.isMailList) {
      var parentURI = selectedItems[0].parentNode.parentNode.getAttribute('id');
      goEditListDialog(parentURI, null, mailingListUri, UpdateCardView);
    }
  }
}
        
function InitCommonJS()
{
  dirTree = document.getElementById("dirTree");
  gResultsOutliner = document.getElementById("abResultsOutliner");
}

function SetupCommandUpdateHandlers()
{
  // dir pane
  if (dirTree)
    dirTree.controllers.appendController(DirPaneController);

  // results pane
  if (gResultsOutliner)
    gResultsOutliner.controllers.appendController(ResultsPaneController);
}

function AbDelete()
{
  if (gAbView) 
    gAbView.deleteSelectedCards();
}

function AbNewCard(abListItem)
{
  var selectedAB = GetSelectedAddressBookDirID(abListItem);

  goNewCardDialog(selectedAB);
}

// NOTE, will return -1 if more than one card selected, or no cards selected.
function GetSelectedCardIndex()
{
  if (!gAbView)
    return -1;

  var outlinerSelection = gAbView.selection;
  if (outlinerSelection.getRangeCount() == 1) {
    var start = new Object;
    var end = new Object;
    outlinerSelection.getRangeAt(0,start,end);
    if (start.value == end.value)
      return start.value;
    }

  return -1;
}

// NOTE, returns the card if exactly one card is selected, null otherwise
function GetSelectedCard()
{
  var index = GetSelectedCardIndex();
  if (index == -1)
    return null;
  else 
    return gAbView.getCardFromRow(index);
}

function AbEditSelectedCard()
{
  AbEditCard(GetSelectedCard());
}

function AbEditCard(card)
{
  if (!card)
    return;

    if (card.isMailList) {
    goEditListDialog(gAbView.URI, card, card.mailListURI, UpdateCardView);
    }
    else {
    goEditCardDialog(gAbView.URI, card, UpdateCardView);
  }
}

function AbNewMessage()
{
  var msgComposeType = Components.interfaces.nsIMsgCompType;
  var msgComposFormat = Components.interfaces.nsIMsgCompFormat;
  var msgComposeService = Components.classes["@mozilla.org/messengercompose;1"].getService();
  msgComposeService = msgComposeService.QueryInterface(Components.interfaces.nsIMsgComposeService);

  var params = Components.classes["@mozilla.org/messengercompose/composeparams;1"].createInstance(Components.interfaces.nsIMsgComposeParams);
  if (params)
  {
    params.type = msgComposeType.New;
    params.format = msgComposFormat.Default;
    var composeFields = Components.classes["@mozilla.org/messengercompose/composefields;1"].createInstance(Components.interfaces.nsIMsgCompFields);
    if (composeFields)
    {
      composeFields.to = GetSelectedAddresses();
      params.composeFields = composeFields;
      msgComposeService.OpenComposeWindowWithParams(null, params);
    }
  }
}

function GetOneOrMoreCardsSelected()
{
  if (!gAbView)
    return false;

  return (gAbView.selection.getRangeCount() > 0);
}

// XXX move into utilityOverlay.js?
function goToggleSplitter( id, elementID )
{
  var splitter = document.getElementById( id );
  var element = document.getElementById( elementID );
  if ( splitter )
  {
    var attribValue = splitter.getAttribute("state") ;

    if ( attribValue == "collapsed" )
    {
      splitter.setAttribute("state", "open" );
      if ( element )
        element.setAttribute("checked","true")
    }
    else
    {
      splitter.setAttribute("state", "collapsed");
      if ( element )
        element.setAttribute("checked","false")
    }
    document.persist(id, 'state');
    document.persist(elementID, 'checked');
  }
}

function GetSelectedAddresses()
{
  var selectedAddresses = "";

  var cards = GetSelectedAbCards();
  var count = cards.length;

  for (var i = 0; i < count; i++) { 
    var generatedAddress = GenerateAddressFromCard(cards[i]);

    if (generatedAddress.length) {
      if (i != 0) {
        selectedAddresses += ",";
      }
      selectedAddresses += generatedAddress;
    }
  }

  return selectedAddresses;
}

function GetNumSelectedCards()
{
 try {
   var outlinerSelection = gAbView.selection;
   return outlinerSelection.count;
 }
 catch (ex) {
   return 0;
 }
}

// XXX fix me to be GetSelectedRanges()
function GetSelectedRows()
{
  var selectedRows = "";

  if (!gAbView)
    return selectedRows;

  var outlinerSelection = gAbView.selection;

  var cards = new Array(outlinerSelection.count);
  var i,j;
  var count = outlinerSelection.getRangeCount();
  
  var current = 0;

  for (i=0; i < count; i++) {
    var start = new Object;
    var end = new Object;
    outlinerSelection.getRangeAt(i,start,end);
    for (j=start.value;j<=end.value;j++) {
      if (selectedRows != "")
        selectedRows += ",";
      selectedRows += j;
    }
  }
  return selectedRows;
}

function GetSelectedAbCards()
{  
  if (!gAbView)
    return null;

  var outlinerSelection = gAbView.selection;

  var cards = new Array(outlinerSelection.count);
  var i,j;
  var count = outlinerSelection.getRangeCount();
  
  var current = 0;

  for (i=0; i < count; i++) {
    var start = new Object;
    var end = new Object;
    outlinerSelection.getRangeAt(i,start,end);
    for (j=start.value;j<=end.value;j++) {
      cards[current] = gAbView.getCardFromRow(j);
      current++;
    }
  }

  return cards;
}


function SelectFirstAddressBook()
{
  var children = dirTree.childNodes;

  var treechildren;
  for (var i=0;i<children.length; i++) {
    if (children[i].localName == "treechildren") {
      treechildren = children[i];
      break;
    }
  }

  children = treechildren.childNodes;
  for (i=0; i<children.length; i++) {
    if (children[i].localName == "treeitem") {
      dirTree.selectItem(children[i]);
      ChangeDirectoryByDOMNode(children[i]);
      return;
    }
  }
}

function SelectFirstCard()
{
  if (gAbView && gAbView.selection) {
    gAbView.selection.select(0);
  }
}

function DirPaneDoubleClick()
{
  if (dirTree && dirTree.selectedItems && (dirTree.selectedItems.length == 1))
    AbEditSelectedDirectory();
}

function DirPaneSelectionChange()
{
  if (dirTree && dirTree.selectedItems && (dirTree.selectedItems.length == 1))
    ChangeDirectoryByDOMNode(dirTree.selectedItems[0]);
  }

function GetAbResultsBoxObject()
{
  var outliner = GetAbResultsOutliner();
  return outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject);
}

var gAbResultsOutliner = null;

function GetAbResultsOutliner()
{
  if (gAbResultsOutliner) 
    return gAbResultsOutliner;

	gAbResultsOutliner = document.getElementById('abResultsOutliner');
	return gAbResultsOutliner;
}

function CloseAbView()
{
  var boxObject = GetAbResultsBoxObject();
  boxObject.view = null;

  if (gAbView) {
    gAbView.close();
    gAbView = null;
  }
}

function SetAbView(uri, sortColumn, sortDirection)
{
  CloseAbView();

  gAbView = Components.classes["@mozilla.org/addressbook/abview;1"].createInstance(Components.interfaces.nsIAbView);

  var actualSortColumn = gAbView.init(uri, GetAbViewListener(), sortColumn, sortDirection);
  
  var boxObject = GetAbResultsBoxObject();
  boxObject.view = gAbView.QueryInterface(Components.interfaces.nsIOutlinerView);
  return actualSortColumn;
}

const kDefaultSortColumn = "GeneratedName";
const kDefaultAscending = "ascending";
const kDefaultDescending = "descending";

function GetAbViewURI()
{
  if (gAbView)
    return gAbView.URI;
  else 
    return null;
}

function ChangeDirectoryByDOMNode(dirNode)
{
  var uri = dirNode.getAttribute("id");
  if (gAbView && gAbView.URI == uri)
    return;
  
  var sortColumn = dirNode.getAttribute("sortColumn");
  var sortDirection = dirNode.getAttribute("sortDirection");
  
  if (!sortColumn)
    sortColumn = kDefaultSortColumn;

  if (!sortDirection)
    sortDirection = kDefaultAscending;

  var actualSortColumn = SetAbView(uri, sortColumn, sortDirection);

  UpdateSortIndicators(actualSortColumn, sortDirection);

  setTimeout('SelectFirstCard();',0);
  
  return;
}

function AbSortAscending()
{
  var sortColumn = kDefaultSortColumn;

  if (gAbView) {
    var node = document.getElementById(gAbView.URI);
    sortColumn = node.getAttribute("sortColumn");
  }

  SortAndUpdateIndicators(sortColumn, kDefaultAscending);
}

function AbSortDescending()
{
  var sortColumn = kDefaultSortColumn;

  if (gAbView) {
    var node = document.getElementById(gAbView.URI);
    sortColumn = node.getAttribute("sortColumn");
  }

  SortAndUpdateIndicators(sortColumn, kDefaultDescending);
}

function SortResultPane(sortColumn)
{
  var sortDirection = kDefaultAscending;

  if (gAbView) {
     sortDirection = gAbView.sortDirection;
  }

  SortAndUpdateIndicators(sortColumn, sortDirection);
}

function SortAndUpdateIndicators(sortColumn, sortDirection)
{
  UpdateSortIndicators(sortColumn, sortDirection);

  if (gAbView)
    gAbView.sortBy(sortColumn, sortDirection);

  SaveSortSetting(sortColumn, sortDirection);
}

function SaveSortSetting(column, direction)
{
  if (dirTree && gAbView) {
    var node = document.getElementById(gAbView.URI);
  if (node) {
      node.setAttribute("sortColumn", column);
      node.setAttribute("sortDirection", direction);
    }
      }
}

function UpdateSortIndicators(colID, sortDirection)
{
  var sortedColumn;
  // set the sort indicator on the column we are sorted by
  if (colID) {
    sortedColumn = document.getElementById(colID);
    if (sortedColumn) {
      sortedColumn.setAttribute("sortDirection",sortDirection);
        }
      }

  // remove the sort indicator from all the columns
  // except the one we are sorted by
  var currCol = GetAbResultsOutliner().firstChild;
  while (currCol) {
    while (currCol && currCol.localName != "outlinercol")
      currCol = currCol.nextSibling;
    if (currCol && (currCol != sortedColumn)) {
      currCol.removeAttribute("sortDirection");
    }
    if (currCol) 
      currCol = currCol.nextSibling;
  }
}

function InvalidateResultsPane()
{
  var outliner = GetAbResultsOutliner();
  outliner.boxObject.invalidate();
}

function AbNewList(abListItem)
{
  var selectedAB = GetSelectedAddressBookDirID(abListItem);

  goNewListDialog(selectedAB);
}

function GetSelectedAddressBookDirID(abListItem)
{
  var selectedAB = 0;
  var abDirEntries = document.getElementById(abListItem);

  if (abDirEntries && abDirEntries.selectedItems && (abDirEntries.selectedItems.length == 1))
    selectedAB = abDirEntries.selectedItems[0].getAttribute("id");

  // request could be coming from the context menu of addressbook panel in sidebar
  // addressbook dirs are listed as menu item. So, get the selected item id.
  if (!selectedAB && abDirEntries && abDirEntries.selectedItem) {
    selectedAB = abDirEntries.selectedItem.getAttribute("id");
  }

  return selectedAB;
}

function goNewListDialog(selectedAB)
{
  window.openDialog("chrome://messenger/content/addressbook/abMailListDialog.xul",
                    "",
                    "chrome,titlebar,centerscreen,resizable=no",
                    {selectedAB:selectedAB});
}

function goEditListDialog(abURI, abCard, listURI, okCallback)
{
  window.openDialog("chrome://messenger/content/addressbook/abEditListDialog.xul",
                    "",
                    "chrome,titlebar,resizable=no",
                    {abURI:abURI, abCard:abCard, listURI:listURI, okCallback:okCallback});
}

function goNewCardDialog(selectedAB)
{
  window.openDialog("chrome://messenger/content/addressbook/abNewCardDialog.xul",
                    "",
                    "chrome,resizable=no,titlebar,modal,centerscreen",
                    {selectedAB:selectedAB});
}

function goEditCardDialog(abURI, card, okCallback)
{
  window.openDialog("chrome://messenger/content/addressbook/abEditCardDialog.xul",
					  "",
					  "chrome,resizable=no,modal,titlebar,centerscreen",
					  {abURI:abURI, card:card, okCallback:okCallback});
}


function setSortByMenuItemCheckState(id, value)
{
    var menuitem = document.getElementById(id);
    if (menuitem) {
      menuitem.setAttribute("checked", value);
    }
}

function InitViewSortByMenu()
{
    var sortColumn = kDefaultSortColumn;
    var sortDirection = kDefaultAscending;

    if (gAbView) {
      sortColumn = gAbView.sortColumn;
      sortDirection = gAbView.sortDirection;
    }

    // this approach is necessary to support generic columns that get overlayed.
    var elements = document.getElementsByAttribute("name","sortas");
    for (var i=0; i<elements.length; i++) {
        var cmd = elements[i].getAttribute("id");
        var columnForCmd = cmd.split("cmd_SortBy")[1];
        setSortByMenuItemCheckState(cmd, (sortColumn == columnForCmd));
    }

    setSortByMenuItemCheckState("sortAscending", (sortDirection == kDefaultAscending));
    setSortByMenuItemCheckState("sortDescending", (sortDirection == kDefaultDescending));
}

function GenerateAddressFromCard(card)
{
  var address = "";
  var name = card.displayName
  if (name.length) {
    address += "\"" + name + "\" ";
  }

  if (card.isMailList) {
    if (name.length) {
      address += "<" + name + ">";
    }
  }
  else {
    var email = card.primaryEmail;
    if (email.length) {
      address += "<" + email + ">";
    }
  }
  return address;
}
