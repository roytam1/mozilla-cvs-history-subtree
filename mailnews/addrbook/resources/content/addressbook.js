/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Original Author:
 *   Paul Hangas <hangas@netscape.com>
 *
 * Contributors:
 *   Seth Spitzer <sspitzer@netscape.com>
 */

var cvPrefs = 0;
var addressbook = 0;
var gAddressBookBundle;

var gTotalCardsElement = null;

// Constants that correspond to choices
// in Address Book->View -->Show Name as
const kDisplayName = 0;
const kLastNameFirst = 1;
const kFirstNameFirst = 2;

function OnUnloadAddressBook()
  {
  RemovePrefObservers();
  CloseAbView();
}

var gAddressBookAbViewListener = {
	onSelectionChanged: function() {
    ResultsPaneSelectionChanged();
  },
  onCountChanged: function(total) {
    SetTotalCardStatus(gAbView.directory.dirName, total);
  }
};

function GetAbViewListener()
{
  return gAddressBookAbViewListener;
}

const kPrefMailAddrBookLastNameFirst = "mail.addr_book.lastnamefirst";

var gMailAddrBookLastNameFirstObserver = {
  observe: function(subject, topic, value) {
    if (topic == "nsPref:changed" && value == kPrefMailAddrBookLastNameFirst) {
      UpdateCardView();
    }
}
}

function AddPrefObservers()
	{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null).QueryInterface(Components.interfaces.nsIPrefBranchInternal);
  prefBranch.addObserver(kPrefMailAddrBookLastNameFirst, gMailAddrBookLastNameFirstObserver, false);
	}

function RemovePrefObservers()
	{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null).QueryInterface(Components.interfaces.nsIPrefBranchInternal);
  prefBranch.removeObserver(kPrefMailAddrBookLastNameFirst, gMailAddrBookLastNameFirstObserver);
}

function OnLoadAddressBook()
{
	gAddressBookBundle = document.getElementById("bundle_addressBook");
	verifyAccounts(null); 	// this will do migration, if we need to.

	top.addressbook = Components.classes["@mozilla.org/addressbook;1"].createInstance(Components.interfaces.nsIAddressBook);

	InitCommonJS();
	GetCurrentPrefs();

  AddPrefObservers();

	// FIX ME - later we will be able to use onload from the overlay
	OnLoadCardView();
	
	SetupCommandUpdateHandlers();

	//workaround - add setTimeout to make sure dynamic overlays get loaded first
	setTimeout('SelectFirstAddressBook()',0);
}

function GetCurrentPrefs()
{
	// prefs
	if ( cvPrefs == 0 )
		cvPrefs = new Object;

	var prefs = Components.classes["@mozilla.org/preferences-service;1"];
	if ( prefs )
	{
		prefs = prefs.getService();
		if ( prefs )
			prefs = prefs.QueryInterface(Components.interfaces.nsIPrefBranch);
			
			cvPrefs.prefs = prefs;
		}
	
	// check "Show Name As" menu item based on pref
	var menuitemID;
	switch (prefs.getIntPref("mail.addr_book.lastnamefirst"))
	{
		case kFirstNameFirst:
			menuitemID = 'firstLastCmd';
			break;
		case kLastNameFirst:
			menuitemID = 'lastFirstCmd';
			break;
		case kDisplayName:
		default:
			menuitemID = 'displayNameCmd';
			break;
	}

	var menuitem = top.document.getElementById(menuitemID);
	if ( menuitem )
		menuitem.setAttribute('checked', 'true');
}


function SetNameColumn(cmd)
{
	var prefValue;
	
	switch ( cmd )
	{
		case 'firstLastCmd':
			prefValue = kFirstNameFirst;
			break;
		case 'lastFirstCmd':
			prefValue = kLastNameFirst;
			break;
		case 'displayNameCmd':
			prefValue = kDisplayName;
			break;
	}
	
	cvPrefs.prefs.setIntPref("mail.addr_book.lastnamefirst", prefValue);
}

function CommandUpdate_AddressBook()
{
	goUpdateCommand('button_delete');
}

function ResultsPaneSelectionChanged()
{
  UpdateCardView();
}

function UpdateCardView()
{
  var card = GetSelectedCard();

  // display the selected card, if exactly one card is selected.
  // either no cards, or more than one card is selected, clear the pane.
 
  if (card)
    DisplayCardViewPane(card);
	else
		ClearCardViewPane();
}


function OnClickedCard(card)
{
  if (card)
    DisplayCardViewPane(card);
	else
		ClearCardViewPane();
}

function AbClose()
{
	top.close();
}

function AbNewAddressBook()
{
	var dialog = window.openDialog("chrome://messenger/content/addressbook/abAddressBookNameDialog.xul",
								   "",
								   "chrome,titlebar",
								   {okCallback:AbCreateNewAddressBook});
}

function AbCreateNewAddressBook(name)
{
	var prefsAttr = new Array;
	var prefsValue = new Array;
	prefsAttr[0]  = "description";
	prefsValue[0]  = name;  
	top.addressbook.newAddressBook(dirTree.database, 1, prefsAttr, prefsValue);
}

function AbPrintCard()
{
	var selectedItems = GetSelectedAbCards();
	var numSelected = selectedItems.length;

	if (numSelected == 0)
    return;

  var addressbook = Components.classes["@mozilla.org/addressbook;1"].createInstance(Components.interfaces.nsIAddressBook);
  var uri = GetAbViewURI();
  if (!uri)
    return;

  var escapedDirName = escape(addressbook.getAbDatabaseFromURI(uri).directoryName);

	var statusFeedback;
	statusFeedback = Components.classes["@mozilla.org/messenger/statusfeedback;1"].createInstance();
	statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

	var selectionArray = new Array(numSelected);
	var totalCard = 0;

	for(var i = 0; i < numSelected; i++)
	{
		var card = selectedItems[i];
    var printCardUrl = CreatePrintCardUrl(escapedDirName, card);
		if (printCardUrl.length)
		{
			selectionArray[totalCard++] = printCardUrl;
		}
	}

	printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
										"",
										"chrome,dialog=no,all",
										totalCard, selectionArray, statusFeedback);

	return;
}

function CreatePrintCardUrl(escapedDirName, card)
{
  var url = "";

  var email = card.primaryEmail;
  if (email.length) {
  	url = "addbook:printone?email=" + email + "&folder=" + escapedDirName;
    dump("XXX url = " + url + "\n");
  } 
  return url;
}

function AbPrintAddressBook()
{
        dump("print address book \n");
        try {
                addressbook.PrintAddressbook();
        }
        catch (ex) {
                dump("failed to print address book\n");
        }
}

function AbImport()
{
	try {
		addressbook.importAddressBook();
	}
	catch (ex) {
		alert("failed to import the addressbook.\n");
		dump("import failed:  " + ex + "\n");
	}
}

function AbDeleteDirectory()
{
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
    promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);

    var selArray = dirTree.selectedItems;
    var count = selArray.length;
    if (!count)
        return;

    var isPersonalOrCollectedAbsSelectedForDeletion = false;
    var parentArray = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
    if (!parentArray) 
      return; 

    for ( var i = 0; i < count; ++i )
    {
        // check to see if personal or collected address books is selected for deletion.
        // if yes, prompt the user an appropriate message saying these cannot be deleted
        // if no, mark the selected items for deletion
        if ((selArray[i].getAttribute("id") != "moz-abmdbdirectory://history.mab") &&
             (selArray[i].getAttribute("id") != "moz-abmdbdirectory://abook.mab"))
        {
            var parent = selArray[i].parentNode.parentNode;
            if (parent)
            {
                var parentId;
                if (parent == dirTree)
                    parentId = "moz-abdirectory://";
                else	
                    parentId = parent.getAttribute("id");

                var dirResource = rdf.GetResource(parentId);
                var parentDir = dirResource.QueryInterface(Components.interfaces.nsIAbDirectory);
                parentArray.AppendElement(parentDir);
            }
        }
        else 
        {
            if (promptService)
            {
                promptService.alert(window,
                    gAddressBookBundle.getString("cannotDeleteTitle"), 
                    gAddressBookBundle.getString("cannotDeleteMessage"));
            }

            isPersonalOrCollectedAbsSelectedForDeletion = true;
            break;
        }
    }

    if (!isPersonalOrCollectedAbsSelectedForDeletion) {
        var confirmDeleteAddressbook =
            gAddressBookBundle.getString("confirmDeleteAddressbook");

        if(!window.confirm(confirmDeleteAddressbook))
            return;

        top.addressbook.deleteAddressBooks(dirTree.database, parentArray, dirTree.selectedItems);
    }
}

function GetTotalCardCountElement()
{
  if (gTotalCardsElement) 
    return gTotalCardsElement;

  var totalCardCountElement = document.getElementById('statusText');
  gTotalCardsElement = totalCardCountElement;
  return gTotalCardsElement;
}

function SetTotalCardStatus(name, total)
{
  var totalElement = GetTotalCardCountElement();
  if (totalElement)
  {
    try 
    {
      var numTotal = gAddressBookBundle.getFormattedString("totalCardStatus", [name, total]);   
      totalElement.setAttribute("label", numTotal);
    }
    catch(ex)
    {
      dump("Fail to set total cards in status\n");
    }
  }
  else
      dump("Can't find status bar\n");
}

function AbResultsPaneDoubleClick(card)
{
  AbEditCard(card);
}
