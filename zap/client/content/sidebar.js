// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "getWindows()[0]" -*-
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

////////////////////////////////////////////////////////////////////////
// SidebarView: class managing the sidebar

var SidebarView = makeClass("SidebarView", SupportsImpl);
SidebarView.addInterfaces(Components.interfaces.nsITreeView);

//----------------------------------------------------------------------
// nsITreeView

//  readonly attribute long rowCount;
SidebarView.getter(
  "rowCount",
  function get_rowCount() {
    return 1;
  });

//  attribute nsITreeSelection selection;
SidebarView.obj("selection", null);

  /** 
   * An atomized list of properties for a given row.  Each property, x, that
   * the view gives back will cause the pseudoclass :moz-tree-row-x
   * to be matched on the pseudoelement ::moz-tree-row.
   */
//  void getRowProperties(in long index, in nsISupportsArray properties);
SidebarView.fun(
  function getRowProperties(index, properties) {
  });

  /**
   * An atomized list of properties for a given cell.  Each property, x, that
   * the view gives back will cause the pseudoclass :moz-tree-cell-x
   * to be matched on the ::moz-tree-cell pseudoelement.
   */
//  void getCellProperties(in long row, in nsITreeColumn col, in nsISupportsArray properties);
SidebarView.fun(
  function getCellProperties(row, col, properties) {
  });
  
  /**
   * Called to get properties to paint a column background.  For shading the sort
   * column, etc.
   */
//  void getColumnProperties(in nsITreeColumn col, in nsISupportsArray properties);
SidebarView.fun(
  function getColumnProperties(col, properties) {
  });

  /**
   * Methods that can be used to test whether or not a twisty should be drawn,
   * and if so, whether an open or closed twisty should be used.
   */
//  boolean isContainer(in long index);
SidebarView.fun(
  function isContainer(index) {
    return false;
  });

//  boolean isContainerOpen(in long index);
SidebarView.fun(
  function isContainerOpen(index) {
    return false;
  });

//  boolean isContainerEmpty(in long index);
SidebarView.fun(
  function isContainerEmpty(index) {
    return true;
  });

  /**
   * isSeparator is used to determine if the row at index is a separator.
   * A value of true will result in the tree drawing a horizontal separator.
   * The tree uses the ::moz-tree-separator pseudoclass to draw the separator.
   */
//  boolean isSeparator(in long index);
SidebarView.fun(
  function isSeparator(index) {
    return false;
  });

  /**
   * Specifies if there is currently a sort on any column. Used mostly by dragdrop
   * to affect drop feedback.
   */
//  boolean isSorted();
SidebarView.fun(
  function isSorted() {
    return false;
  });

  /**
   * Methods used by the drag feedback code to determine if a drag is allowable at
   * the current location. To get the behavior where drops are only allowed on
   * items, such as the mailNews folder pane, always return false when
   * the orientation is not DROP_ON.
   */
//  boolean canDrop(in long index, in long orientation);
SidebarView.fun(
  function canDrop(index, orientation) {
    return false;
  });

  /**
   * Called when the user drops something on this view. The |orientation| param
   * specifies before/on/after the given |row|.
   */
//  void drop(in long row, in long orientation); 
SidebarView.fun(
  function drop(row, orientation) {
    
  });

  /**
   * Methods used by the tree to draw thread lines in the tree.
   * getParentIndex is used to obtain the index of a parent row.
   * If there is no parent row, getParentIndex returns -1.
   */
//  long getParentIndex(in long rowIndex);
SidebarView.fun(
  function getParentIndex(rowIndex) {
    return -1;
  });

  /**
   * hasNextSibling is used to determine if the row at rowIndex has a nextSibling
   * that occurs *after* the index specified by afterIndex.  Code that is forced
   * to march down the view looking at levels can optimize the march by starting
   * at afterIndex+1.
   */
//  boolean hasNextSibling(in long rowIndex, in long afterIndex);
SidebarView.fun(
  function hasNextSibling(rowIndex, afterIndex) {
    return false;
  });

  /**
   * The level is an integer value that represents
   * the level of indentation.  It is multiplied by the width specified in the 
   * :moz-tree-indentation pseudoelement to compute the exact indendation.
   */
//  long getLevel(in long index);
SidebarView.fun(
  function getLevel(index) {
    return 0;
  });

  /**
   * The image path for a given cell. For defining an icon for a cell.
   * If the empty string is returned, the :moz-tree-image pseudoelement
   * will be used.
   */
//  AString getImageSrc(in long row, in nsITreeColumn col);
SidebarView.fun(
  function getImageSrc(row, col) {
    return "chrome://zap/skin/accounts.png";
  });

  /**
   * The progress mode for a given cell. This method is only called for
   * columns of type |progressmeter|.
   */
//  long getProgressMode(in long row, in nsITreeColumn col);
SidebarView.fun(
  function getProgressMode(row, col) {
    return Components.interfaces.nsITreeView.PROGRESS_NONE;
  });

  /**
   * The value for a given cell. This method is only called for columns
   * of type other than |text|.
   */
//  AString getCellValue(in long row, in nsITreeColumn col);
SidebarView.fun(
  function getCellValue(row, col) {
    return "";
  });

  /**
   * The text for a given cell.  If a column consists only of an image, then
   * the empty string is returned.  
   */
//  AString getCellText(in long row, in nsITreeColumn col);
SidebarView.fun(
  function getCellText(row, col) {
    return "Accounts";
  });

  /**
   * Called during initialization to link the view to the front end box object.
   */
//  void setTree(in nsITreeBoxObject tree);
SidebarView.fun(
  function setTree(tree) {
    this._tree = tree;
  });
  
  /**
   * Called on the view when an item is opened or closed.
   */
//  void toggleOpenState(in long index);
SidebarView.fun(
  function toggleOpenState(index) {
    
  });
  
  /**
   * Called on the view when a header is clicked.
   */
//  void cycleHeader(in nsITreeColumn col);
SidebarView.fun(
  function cycleHeader(col) {
  });
  
  /**
   * Should be called from a XUL onselect handler whenever the selection changes.
   */
//  void selectionChanged();
SidebarView.fun(
  function selectionChanged() {
  });

  /**
   * Called on the view when a cell in a non-selectable cycling column (e.g., unread/flag/etc.) is clicked.
   */
//  void cycleCell(in long row, in nsITreeColumn col);
SidebarView.fun(
  function cycleCell(row, col) {
  });

  /**
   * isEditable is called to ask the view if the cell contents are editable.
   * A value of true will result in the tree popping up a text field when 
   * the user tries to inline edit the cell.
   */
//  boolean isEditable(in long row, in nsITreeColumn col);
SidebarView.fun(
  function isEditable(row, col) {
    return false;
  });
    
  /**
   * setCellValue is called when the value of the cell has been set by the user.
   * This method is only called for columns of type other than |text|.
   */
//  void setCellValue(in long row, in nsITreeColumn col, in AString value);
SidebarView.fun(
  function setCellValue(row, col, value) {
  });

  /**
   * setCellText is called when the contents of the cell have been edited by the user.
   */   
//  void setCellText(in long row, in nsITreeColumn col, in AString value);
SidebarView.fun(
  function setCellText(row, col, value) {
  });

  /**
   * A command API that can be used to invoke commands on the selection.  The tree
   * will automatically invoke this method when certain keys are pressed.  For example,
   * when the DEL key is pressed, performAction will be called with the "delete" string.
   */
//  void performAction(in wstring action);
SidebarView.fun(
  function performAction(action) {
  });

  /**
   * A command API that can be used to invoke commands on a specific row.
   */
//  void performActionOnRow(in wstring action, in long row);
SidebarView.fun(
  function performActionOnRow(action, row) {
  });

  /**
   * A command API that can be used to invoke commands on a specific cell.
   */
//  void performActionOnCell(in wstring action, in long row, in nsITreeColumn col);
SidebarView.fun(
  function performActionOnCell(action, row, col) {
  });



