/* 
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
 * The Original Code is Mozilla Session Roaming code.
 * 
 * The Initial Developer of the Original Code is
 * Ben Bucksch <http://www.bucksch.org> of
 * Beonex <http://www.beonex.com>
 * Portions created by Ben Bucksch are Copyright (C) 2002 Ben Bucksch.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 */

/* See all.js */

const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var _elementIDs = []; // no prefs (nsIPref) needed, see above

function Startup()
{
  FileLabels();
  SetFiles();
  // dataManager.pageData doesn't work, because it needs to work on both panes
  if (!parent.roaming)
    parent.roaming = new RoamingPrefs();
  DataToUI();
}


// Logic

// some files have a non-static name. set these filenames here.
function SetFiles()
{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService);
  var prefs = prefService.getBranch(null);
  try
  {
    SetFile("filePassword", prefs.getCharPref("signon.SignonFileName"));
    SetFile("fileWallet", prefs.getCharPref("wallet.SchemaValueFileName"));
  } catch (e) {}

  // disable the nodes which are still invalid
  var children = E("filesList").childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var checkbox = children[i];
    if (!("getAttribute" in checkbox) ||
        checkbox.getAttribute("type") != "checkbox")
      continue;
    if (checkbox.getAttribute("filename") == "")
      checkbox.disabled = true;
  }
}

function SetFile(elementID, filename)
{
  var listitem = document.getElementById(elementID);
  listitem.setAttribute("filename", filename);
}


// UI

// write data to widgets
function DataToUI()
{
  var data = parent.roaming;
  var filesList = E("filesList");

  EnableTree(data.Enabled, filesList);

  // first, disable all by default
  var children = filesList.childNodes;
  for (var i = 0, l = children.length; i < l; i++)
  {
    var checkbox = children[i];
    if (!("getAttribute" in checkbox) ||
        checkbox.getAttribute("type") != "checkbox")
      // Somebody adds unwanted nodes as children to listbox :-(
      continue;
    checkbox.checked = false;
  }
  // then check for each file in the list, if it's in the checkboxes.
  // enabled it, if so, otherwise create and enable it (for files added by
  // the user).
  for (i = 0, l = data.Files.length; i < l; i++)
  {
    var file = data.Files[i];
    var found = false;
    for (var i2 = 0, l2 = children.length; i2 < l2; i2++)
    {
      var checkbox = children[i2];
      if ("getAttribute" in checkbox
          && checkbox.getAttribute("type") == "checkbox"
          // Somebody adds unwanted nodes as children to listbox :-(
          && checkbox.getAttribute("filename") == file)
      {
        checkbox.checked = true;
        found = true;
      }
    }
    if (!found)
    {
      var li = document.createElementNS(kXULNS, "listitem");
      if (li)
      {
        li.setAttribute("type", "checkbox");
        li.setAttribute("id", file);
        li.setAttribute("filename", file);
        li.setAttribute("label", file);
        li.setAttribute("checked", "true");
        filesList.appendChild(li);
      }
    }
  }
}

// write widget content to data
function UIToData()
{
  var data = parent.roaming;
  data.Files = new Array(); // clear list
  var children = E("filesList").childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var checkbox = children.item(i);
    if (checkbox.checked)
      data.Files.push(checkbox.getAttribute("filename"));
  }
  data.changed = true; // excessive
}

/* read human-readable names for profile files from filedescr.properties
   and use them as labels */
function FileLabels()
{
  ddump("filelabels()");
  var children = E("filesList").childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var checkbox = children[i];
    if (!("getAttribute" in checkbox) ||
        checkbox.getAttribute("type") != "checkbox")
      continue;

    checkbox.setAttribute("label",
                          GetFileDescription(checkbox.getAttribute("filename"),
                                             checkbox.getAttribute("id")));
  }
}
