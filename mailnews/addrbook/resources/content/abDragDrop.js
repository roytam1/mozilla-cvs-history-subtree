/* -*- Mode: Java; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

var nsIDragService = Components.interfaces.nsIDragService;

function debugDump(msg)
{
  // uncomment for noise
  // dump(msg+"\n");
}

function GetDragService()
{
	var dragService = Components.classes["@mozilla.org/widget/dragservice;1"].getService();
	if (dragService) 
		dragService = dragService.QueryInterface(nsIDragService);

	return dragService;
}

function DragOverTree(event)
{
	var validFlavor = false;
	var dragSession = null;
	var retVal = true;

	var dragService = GetDragService();
	if ( !dragService )	return(false);

	dragSession = dragService.getCurrentSession();
	if ( !dragSession )	return(false);

	if ( dragSession.isDataFlavorSupported("text/x-moz-address"))	
    validFlavor = true;

	// touch the attribute on the rowgroup to trigger the repaint with the drop feedback.
	if ( validFlavor )
	{
		//XXX this is really slow and likes to refresh N times per second.
		var treeItem = event.target.parentNode.parentNode;
		treeItem.setAttribute ( "dd-triggerrepaint", 0 );
		dragSession.canDrop = true;
		// necessary??
		retVal = false; // do not propagate message
	}
	return(retVal);
}

function BeginDragAbResultsPane(event)
{
	var dragStarted = false;
    var outliner = GetAbResultsOutliner();
	var dragService = GetDragService();

	var transArray = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
	if ( !transArray ) return(false); 

    // let's build the drag region
    var region = null;
    try {
      region = Components.classesByID["{da5b130a-1dd1-11b2-ad47-f455b1814a78}"].createInstance(Components.interfaces.nsIScriptableRegion);
      region.init();
      var obo = outliner.outlinerBoxObject;
      var bo = obo.outlinerBody.boxObject;
      var obosel= obo.selection;

      var rowX = bo.x;
      var rowY = bo.y;
      var rowHeight = obo.rowHeight;
      var rowWidth = bo.width;

      //add a rectangle for each visible selected row
      for (var i = obo.getFirstVisibleRow(); i <= obo.getLastVisibleRow(); i ++)
      {
        if (obosel.isSelected(i))
          region.unionRect(rowX, rowY, rowWidth, rowHeight);
        rowY = rowY + rowHeight;
      }
      
      //and finally, clip the result to be sure we don't spill over...
      region.intersectRect(bo.x, bo.y, bo.width, bo.height);
    } catch(ex) {
      dump("Error while building selection region: " + ex + "\n");
      region = null;
    }
    
    var selArray = GetSelectedAbCards();

	var count = selArray.length;
	debugDump("selArray.length = " + count + "\n");
    for (i = 0; i < count; i++ ) {
		var trans = Components.classes["@mozilla.org/widget/transferable;1"].createInstance(Components.interfaces.nsITransferable);
		if ( !trans )		return(false);

		var genTextData = Components.classes["@mozilla.org/supports-wstring;1"].createInstance(Components.interfaces.nsISupportsWString);
		if (!genTextData)	return(false);

        trans.addDataFlavor("text/x-moz-address");
        
        var address = GenerateAddressFromCard(selArray[i]);
        genTextData.data = address;
        debugDump("address #" + i + " = " + address + "\n");

        trans.setTransferData ( "text/x-moz-address", genTextData, address.length * 2 );  // doublebyte byte data

		// put it into the transferable as an |nsISupports|
		var genTrans = trans.QueryInterface(Components.interfaces.nsISupports);
		transArray.AppendElement(genTrans);
	}

    dragService.invokeDragSession ( event.target, transArray, region, nsIDragService.DRAGDROP_ACTION_COPY +
		nsIDragService.DRAGDROP_ACTION_MOVE );

	dragStarted = true;

	return(!dragStarted);  // don't propagate the event if a drag has begun
}

function DropOnDirectoryTree(event)
{
	debugDump("DropOnTree\n");

    if (event.target.localName != "treecell" &&
        event.target.localName != "treeitem")
        return false;

  var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService().QueryInterface(Components.interfaces.nsIRDFService);

	var treeRoot = dirTree;
	if (!treeRoot)	return(false);
	var treeDatabase = treeRoot.database;
	if (!treeDatabase)
    return(false);

	// target is the <treecell>, and "id" is on the <treeitem> two levels above
	var treeItem = event.target.parentNode.parentNode;
	if (!treeItem)
    return(false);

	// drop action is always "on" not "before" or "after"
	// get drop hint attributes
	var dropBefore = treeItem.getAttribute("dd-droplocation");
	var dropOn = treeItem.getAttribute("dd-dropon");

	var dropAction;
	if (dropOn == "true") 
		dropAction = "on";
	else
		return(false);

	var targetID = treeItem.getAttribute("id");
	if (!targetID)
    return(false);

	debugDump("***targetID = " + targetID + "\n");

	var dragService = GetDragService();
	if ( !dragService )	
    return(false);
	
	var dragSession = dragService.getCurrentSession();
	if ( !dragSession )	
    return(false);

	var trans = Components.classes["@mozilla.org/widget/transferable;1"].createInstance(Components.interfaces.nsITransferable);
	if ( !trans ) 
    return(false);
	trans.addDataFlavor("text/x-moz-address");

	for ( var i = 0; i < dragSession.numDropItems; ++i )
	{
		dragSession.getData ( trans, i );
		var dataObj = new Object();
		var bestFlavor = new Object();
		var len = new Object();
		trans.getAnyTransferData ( bestFlavor, dataObj, len );
		if ( dataObj )
      dataObj = dataObj.value.QueryInterface(Components.interfaces.nsISupportsWString);
		if ( !dataObj )	
      continue;

		var address = dataObj.data.substring(0, len.value);
		if (!address)	
      continue;

		debugDump("drop address #" + i + ": drop '" + address + "' " + dropAction + " '" + targetID + "'\n");

/*
		var sourceNode = rdf.GetResource(sourceID, true);
		if (!sourceNode)
			continue;
		
		var targetNode = rdf.GetResource(targetID, true);
		if (!targetNode) 
			continue;

		// Prevent dropping of a node before, after, or on itself
		if (sourceNode == targetNode)	
      continue;

		if (sourceID.substring(0,targetID.length) != targetID)
		{
			var cardResource = rdf.GetResource(sourceID);
			var card = cardResource.QueryInterface(Components.interfaces.nsIAbCard);
			if (!card.isMailList)
				card.dropCardToDatabase(targetID);
		}
*/
	}

	return(false);
}
