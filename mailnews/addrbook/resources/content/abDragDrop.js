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

var abResultsPaneObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      aXferData.data = new TransferData();
      var selectedRows = GetSelectedRows();
      var selectedAddresses = GetSelectedAddresses();

      aXferData.data.addDataForFlavour("moz/abcard", selectedRows);
      aXferData.data.addDataForFlavour("text/x-moz-address", selectedAddresses);
    },

  onDrop: function (aEvent, aXferData, aDragSession)
    {
    },

  onDragExit: function (aEvent, aDragSession)
    {
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
    },

  getSupportedFlavours: function ()
    {
     return null;
    }
};

var abDirTreeObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
    },

  onDrop: function (aEvent, aXferData, aDragSession)
    {
	    var xferData = aXferData.data.split("\n");
 
      // XXX do we still need this check, since we do it in onDragOver?
      if (aEvent.target.localName != "treecell") {
         return;
      }

      // target is the <treecell>, and "id" is on the <treeitem> two levels above
      var treeItem = aEvent.target.parentNode.parentNode;
      if (!treeItem)  
        return;

      var targetID = treeItem.getAttribute("id");
      var rdf = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
      var directory = rdf.GetResource(targetID).QueryInterface(Components.interfaces.nsIAbDirectory);

      var boxObject = GetAbResultsBoxObject();
      var abView = boxObject.view.QueryInterface(Components.interfaces.nsIAbView);

      var rows = xferData[0].split(",");
      var numrows = rows.length;

      for (var i=0;i<numrows;i++) {
        var card = abView.getCardFromRow(rows[i]);
        directory.dropCard(card);
      }
    },

  onDragExit: function (aEvent, aDragSession)
    {
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
      if (aEvent.target.localName != "treecell") {
         aDragSession.canDrop = false;
         return false;
      }

      // target is the <treecell>, and "id" is on the <treeitem> two levels above
      var treeItem = aEvent.target.parentNode.parentNode;
      if (!treeItem)  
        return false;

      var targetID = treeItem.getAttribute("id");
      return true;
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("moz/abcard");
      return flavourSet;
    }
};

