/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>
 *
 */

/* This file implements an nsIOutlinerView for the filepicker */

const nsILocalFile = Components.interfaces.nsILocalFile;
const nsILocalFile_CONTRACTID = "@mozilla.org/file/local;1";
const nsIFile = Components.interfaces.nsIFile;
const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
const nsIAtomService = Components.interfaces.nsIAtomService;
const nsAtomService_CONTRACTID = "@mozilla.org/atom-service;1";

var gDateService = null;

function nsFileView() {
  this.mShowHiddenFiles = false;
  this.mDirectoryFilter = false;
  this.mEntryArray = [];
  this.mFilteredList = [];
  this.mCurrentFilter = ".*";
  this.mSelectionCallback = null;
  this.mOutliner = null;

  if (!gDateService) {
    gDateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
      .getService(nsIScriptableDateFormat);
  }

  var atomService = Components.classes[nsAtomService_CONTRACTID]
                      .getService(nsIAtomService);
  this.mDirectoryAtom = atomService.getAtom("directory");
  this.mFileAtom = atomService.getAtom("file");
}

function numMatchingChars(str1, str2) {
  for (var i = 0; ((i < Math.min(str1.length, str2.length)) && (str1[i] == str2[i])); i++);
  return i;
}

nsFileView.prototype = {

  /* readonly attribute long rowCount; */
  set rowCount(c) { throw "readonly property"; },
  get rowCount() { return this.mFilteredList.length; },

  /* attribute nsIOutlinerSelection selection; */
  set selection(s) { this.mSelection = s; },
  get selection() { return this.mSelection; },

  set selectionCallback(f) { this.mSelectionCallback = f; },
  get selectionCallback() { return this.mSelectionCallback; },

  /* nsISupports methods */

  /* void QueryInterface(in nsIIDRef uuid, 
     [iid_is(uuid),retval] out nsQIResult result); */
  QueryInterface: function(iid) {
    if (!iid.equals(nsIOutlinerView) &&
        !iid.equals(nsISupports)) {
          throw Components.results.NS_ERROR_NO_INTERFACE;
        }
    return this;
  },

  /* nsIOutlinerView methods */

  /* void getRowProperties(in long index, in nsISupportsArray properties); */
  getRowProperties: function(index, properties) { },

  /* void getCellProperties(in long row, in wstring colID, in nsISupportsArray properties); */
  getCellProperties: function(row, colID, properties) {
    if (row >= this.mFilteredList.length)
      return;

    var file = this.mFilteredList[row][0];
    if (file.isDirectory())
      properties.AppendElement(this.mDirectoryAtom);
    else
      properties.AppendElement(this.mFileAtom);
},

  /* void getColumnProperties(in wstring colID, in nsIDOMElement colElt,
     in nsISupportsArray properties); */
  getColumnProperties: function(colID, colElt, properties) { },

  /* boolean isContainer(in long index); */
  isContainer: function(index) { return false; },

  /* boolean isContainerOpen(in long index); */
  isContainerOpen: function(index) { return false;},

  /* boolean isContainerEmpty(in long index); */
  isContainerEmpty: function(index) { return false; },

  /* long getParentIndex(in long rowIndex); */
  getParentIndex: function(rowIndex) { return -1; },

  /* boolean hasNextSibling(in long rowIndex, in long afterIndex); */
  hasNextSibling: function(rowIndex, afterIndex) {
    if (afterIndex < (this.rowCount -1)) {
      return true;
    } else {
      return false;
    }
  },

  /* long getLevel(in long index); */
  getLevel: function(index) { return 0; },
  
  /* wstring getCellText(in long row, in wstring colID); */
  getCellText: function(row, colID) {
    /* we cache the file size and last modified dates -
       this function must be very fast since it's called
       whenever the cell needs repainted */
    var file = this.mFilteredList[row];
    if (colID == "FilenameColumn") {
      return file[0].unicodeLeafName;
    } else if (colID == "FileSizeColumn") {
      if (!file[1]) {
        if (file[0].isFile()) {
          file[1] = String(file[0].fileSize);
        } else {
          file[1] = "";
        }
      }
      return file[1];
    } else if (colID == "LastModifiedColumn") {
      if (!file[2]) {
        // perhaps overkill, but lets get the right locale handling
        var modDate = new Date(file[0].lastModificationDate);
        file[2] = gDateService.FormatDateTime("", gDateService.dateFormatShort,
                                              gDateService.timeFormatSeconds,
                                              modDate.getFullYear(), modDate.getMonth()+1,
                                              modDate.getDate(), modDate.getHours(),
                                              modDate.getMinutes(), modDate.getSeconds());
      }
      return file[2];
    }
    return "";
  },

  /* void setOutliner(in nsIOutlinerBoxObject outliner); */
  setOutliner: function(outliner) { this.mOutliner = outliner; },

  /* void toggleOpenState(in long index); */
  toggleOpenState: function(index) { },

  /* void cycleHeader(in wstring colID, in nsIDOMElement elt); */
  cycleHeader: function(colID, elt) { },

  /* void selectionChanged(); */
  selectionChanged: function() {
    if (this.mSelectionCallback &&
        this.mSelection.currentIndex <= this.mFilteredList.length - 1) {
      var file = this.mFilteredList[this.mSelection.currentIndex][0];
      this.mSelectionCallback(file);
    }
  },

  /* void cycleCell(in long row, in wstring colID); */
  cycleCell: function(row, colID) { },

  /* boolean isEditable(in long row, in wstring colID); */
  isEditable: function(row, colID) { return false; },

  /* void setCellText(in long row, in wstring colID, in wstring value); */
  setCellText: function(row, colID, value) { },

  /* void performAction(in wstring action); */
  performAction: function(action) { },

  /* void performActionOnRow(in wstring action, in long row); */
  performActionOnRow: function(action, row) { },

  /* void performActionOnCell(in wstring action, in long row, in wstring colID); */
  performActionOnCell: function(action, row, colID) { },

  /* private methods */

  set showHiddenFiles(s) {
    this.mShowHiddenFiles = s;
    this.filterFiles();
  },

  get showHiddenFiles() { return this.mShowHiddenFiles; },

  set showOnlyDirectories(s) {
    this.mDirectoryFilter = s;
    this.filterFiles();
  },

  get showOnlyDirectories() { return this.mDirectoryFilter; },

  setDirectory: function(directory) {
    this.mDirectoryPath = directory;
    this.mEntryArray = [];

    var dir = Components.classes[nsILocalFile_CONTRACTID].createInstance(nsILocalFile);
    dir.followLinks = false;
    dir.initWithUnicodePath(directory);
    var dirEntries = dir.QueryInterface(nsIFile).directoryEntries;
    while (dirEntries.hasMoreElements()) {
      this.mEntryArray[this.mEntryArray.length] = dirEntries.getNext().QueryInterface(nsIFile);
    }

    this.filterFiles();
  },

  setFilter: function(filter) {
    // The filter may contain several components, i.e.:
    // *.html; *.htm
    // First separate it into its components
    var filterList = filter.split(/;[ ]*/);

    if (filterList.length == 0) {
      // this shouldn't happen
      return;
    }

    // Transform everything in the array to a regexp
    var tmp = filterList[0].replace(/\./g, "\\.");
    filterList[0] = tmp.replace(/\*/g, ".*");
    var shortestPrefix = filterList[0];
    
    for (var i = 1; i < filterList.length; i++) {
      // * becomes .*, and we escape all .'s with \
      tmp = filterList[i].replace(/\./g, "\\.");
      filterList[i] = tmp.replace(/\*/g, ".*");
      shortestPrefix = shortestPrefix.substr(0, numMatchingChars(shortestPrefix, filterList[i]));
    }
    
    var filterStr = shortestPrefix+"(";
    var startpos = shortestPrefix.length;
    for (i = 0; i < filterList.length; i++) {
      filterStr += filterList[i].substr(shortestPrefix.length) + "|";
    }

    this.mCurrentFilter = new RegExp(filterStr.substr(0, (filterStr.length) - 1) + ")");
    this.filterFiles();
  },

  filterFiles: function() {
    // Tell the outliner that all the old rows are going away
    if (this.mOutliner) {
      this.mOutliner.rowCountChanged(0, -this.mFilteredList.length);
    }

    this.mFilteredList = [];

    for(var i = 0; i < this.mEntryArray.length; i++) {
      var file = this.mEntryArray[i];
      var dirOrSymlink = file.isSymlink() || file.isDirectory();

      if ((this.mShowHiddenFiles || !file.isHidden()) &&
          (!this.mDirectoryFilter || dirOrSymlink) &&
          (dirOrSymlink || file.unicodeLeafName.search(this.mCurrentFilter) == 0)) {
        this.mFilteredList[this.mFilteredList.length] = [file, null, null];
      }
    }

    // Tell the outliner how many rows we just added
    if (this.mOutliner) {
      this.mOutliner.rowCountChanged(0, this.mFilteredList.length);
    }
  },

  getSelectedFile: function() {
    if (0 <= this.mSelection.currentIndex &&
        this.mSelection.currentIndex <= this.mFilteredList.length - 1) {
      return this.mFilteredList[this.mSelection.currentIndex][0];
    }

    return null;
  }
}

