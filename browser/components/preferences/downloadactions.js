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
# The Original Code is the Download Actions Manager.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are 
# Copyright (C) 2000, 2001, 2003, 2005
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
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

# Much of this code is inherited from other parts of the source repository. 
# For definitive histories, use bonsai to search for:
#
# mozilla/toolkit/mozapps/downloads/content/pref-downloads.js
# mozilla/toolkit/mozapps/downloads/content/pref-downloads.xul
# mozilla/toolkit/mozapps/downloads/content/helperApps.js
# mozilla/toolkit/mozapps/downloads/content/editAction.js
# mozilla/toolkit/mozapps/downloads/content/editAction.xul
#                 ( for 2003- modifications)
#
# and a similar structure under:
#
# mozilla/xpfe/components/prefwindow
#                 ( for pre 2003 modifications)
#

var gDownloadActionsDialog = {  
  _helperApps   : null,
  _handlersList : null,
  _editButton   : null,
  _removeButton : null,
  
  init: function ()
  {
    // Initialize the File Type list
    this._helperApps = new HelperApps();
    
    this._handlersList = document.getElementById("fileHandlersList");
    this._handlersList.database.AddDataSource(this._helperApps);
    this._handlersList.setAttribute("ref", "urn:mimetypes");
  
    (this._editButton = document.getElementById("editFileHandler")).disabled = true;
    (this._removeButton = document.getElementById("removeFileHandler")).disabled = true;
    
    var indexToSelect = parseInt(this._handlersList.getAttribute("lastSelected"));
    this._handlersList.view.selection.select(indexToSelect);
    
    this._handlersList.focus();
  },
  
  onSelectionChanged: function ()
  {
    var selection = this._handlersList.view.selection; 
    var selected = selection.count;
    this._removeButton.disabled = selected == 0;
    this._editButton.disabled = selected != 1;
    
    var canRemove = true;
    
    var cv = this._handlersList.contentView;
    var rangeCount = selection.getRangeCount();
    var min = { }, max = { };
    var setLastSelected = false;
    for (var i = 0; i < rangeCount; ++i) {
      selection.getRangeAt(i, min, max);
      
      for (var j = min.value; j <= max.value; ++j) {
        if (!setLastSelected) {
          // Set the last selected index to the first item in the selection
          this._handlersList.setAttribute("lastSelected", j);
          setLastSelected = true;
        }
        var item = cv.getItemAtIndex(j);
        var editable = this._helperApps.getLiteralValue(item.id, "editable") == "true";
        var handleInternal = this._helperApps.getLiteralValue(item.id, "handleInternal");
        
        if (!editable || handleInternal) 
          canRemove = false;
      }
    }
    
    if (!canRemove) {
      this._removeButton.disabled = true;
      this._editButton.disabled = true;
    }
  },

  removeFileHandler: function ()
  {
    const nsIPS = Components.interfaces.nsIPromptService;
    var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                       .getService(nsIPS);
    
    var bundleUCT = document.getElementById("bundleUCT");
    var title = bundleUCT.getString("removeActions");
    var msg = bundleUCT.getString("removeActionsMsg");

    var buttons = (nsIPS.BUTTON_TITLE_YES * nsIPS.BUTTON_POS_0) + (nsIPS.BUTTON_TITLE_NO * nsIPS.BUTTON_POS_1);

    if (ps.confirmEx(window, title, msg, buttons, "", "", "", "", { }) == 1) 
      return;
    
    var c = Components.classes["@mozilla.org/rdf/container;1"].createInstance(Components.interfaces.nsIRDFContainer);
    c.Init(this._helperApps, gRDF.GetResource("urn:mimetypes:root"));
    
    var cv = this._handlersList.contentView;
    var selection = this._handlersList.view.selection; 
    var rangeCount = selection.getRangeCount();
    var min = { }, max = { };
    
    var lastAdjacent = -1;
    for (var i = 0; i < rangeCount; ++i) {
      selection.getRangeAt(i, min, max);
      
      if (i == (rangeCount - 1)) { 
        if (min.value >= (this._handlersList.view.rowCount - selection.count)) 
          lastAdjacent = min.value - 1;
        else
          lastAdjacent = min.value;
      }
      
      for (var j = max.value; j >= min.value; --j) {
        var item = cv.getItemAtIndex(j);
        var itemResource = gRDF.GetResource(item.id);
        c.RemoveElement(itemResource, j == min.value);
        
        this._cleanResource(itemResource);
      }
    }

    if (lastAdjacent != -1) {
      selection.select(lastAdjacent);
      this._handlersList.focus();
    }
    
    this._helperApps.flush();
  },
  
  _cleanResource: function (aResource)
  {
    var handlerProp = this._helperApps.GetTarget(aResource, this._helperApps._handlerPropArc, true);
    if (handlerProp) {
      var extApp = this._helperApps.GetTarget(handlerProp, this._helperApps._externalAppArc, true);
      if (extApp)
        this._disconnect(extApp);
      this._disconnect(handlerProp);
    }
    this._disconnect(aResource);
  },

  disconnect: function (aResource)
  {
    var arcs = this._helperApps.ArcLabelsOut(aResource);
    while (arcs.hasMoreElements()) {
      var arc = arcs.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      var val = this._helperApps.GetTarget(aResource, arc, true);
      this._helperApps.Unassert(aResource, arc, val, true);
    }
  },
  
  editFileHandler: function ()
  {
    var selection = this._handlersList.view.selection; 
    
    var cv = this._handlersList.contentView;
    var item = cv.getItemAtIndex(selection.currentIndex);
    var itemResource = gRDF.GetResource(item.id);
    document.documentElement.openSubDialog("chrome://browser/content/preferences/changeaction.xul",
                                           "", itemResource);
  }
};

