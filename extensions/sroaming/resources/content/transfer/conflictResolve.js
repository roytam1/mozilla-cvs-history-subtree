/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Roaming code.
 *
 * The Initial Developer of the Original Code is 
 * Ben Bucksch <http://www.bucksch.org> of
 * Beonex <http://www.beonex.com>
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

var params = window.arguments[0].QueryInterface(
                                   Components.interfaces.nsIDialogParamBlock);
var gIsDownload = true; // otherwise upload
var gCount = 0; // number of files

function StartUp()
{
  centerWindowOnScreen();
  LoadElements();
  ClearParam();
}

function AddItem(filename, fileid)
{
  var filenames = document.getElementById("filenames");
  var radios = document.getElementById("radios");

  var label = document.createElement("text");
  label.setAttribute("value", filename);

  var radiogroup = document.createElement("radiogroup");
  radiogroup.setAttribute("orient", "horizontal");
  radiogroup.setAttribute("id", fileid);

  var radioServer = document.createElement("radio");
  radioServer.setAttribute("value", "1");
  var radioLocal = document.createElement("radio");
  radioLocal.setAttribute("value", "2");

  radiogroup.setAttribute("value", gIsDownload ? "2" : "1");

  radiogroup.appendChild(radioServer);
  radiogroup.appendChild(radioLocal);

  filenames.appendChild(label);
  radios.appendChild(radiogroup);
}

function LoadElements()
{
  //For definition of meaning of params, see mozSRoaming.cpp::ConflictResolveUI

  // download
  var direction = params.GetInt(0);
dump("Passing in: Int 0 (direction) is " + direction + "\n");
  if (direction != 1 && direction != 2)
    dump("Error: Bad direction param");
  gIsDownload = direction == 1;

  // count
  gCount = params.GetInt(1);
dump("Passing in: Int 1 (count) is " + gCount + "\n");
  if (gCount < 1)
    dump("Error: Bad count param");

  // filenames
  for (var i = 0; i < gCount; i++)
  {
    var filename = params.GetString(i);
dump("Passing in: String " + i + " is " + filename + "\n");
    AddItem(filename, "file" + i);
  }

  var descr_deck = document.getElementById("intro");
  descr_deck.setAttribute("selectedIndex", gIsDownload ? 0 : 1);
}

/* Because the param block has a different meaning on open and close, there
   is a risk of misinterpreting the input as output. Thus, clear it to avoid
   that. (0 values are intentionally not defined as legal.) */
function ClearParam()
{
  params.SetInt(0, 0);
  params.SetInt(1, 0);
}

function onExit()
{
dump("ShutDown()\n");
  for (var i = 0; i < gCount; i++)
  {
    var radiogroup = document.getElementById("file" + i);
    var choice = radiogroup.value == "1";
    if (choice != "1" && choice != "2")
      dump("Error: Bad radiogroup value: -" + choice + "-\n");
    var isServer = choice == "1";
    filename = params.SetInt(i, isServer ? 1 : 2);
dump("Passing back: Int " + i + " is " + isServer ? 1 : 2 + "\n");
  }
  return true;
}
