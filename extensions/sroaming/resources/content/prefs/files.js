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

// This file is mainly copied from sroamingPrefs.js

//from mozSRoaming*.cpp
const kRegTreeProfile = "Profiles";
const kRegTreeRoaming = "Roaming";
const kRegKeyEnabled = "Enabled";
const kRegKeyProtocol = "Protocol";
const kRegKeyFiles = "Files";

const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var _elementIDs = []; // no prefs (nsIPref) needed, see above
var registry;
var regkeyProf; // The registry branch with for the current profile.
                // The Roaming branch hangs directly below that.

function Startup()
{
  dump("startup files\n");
  parent.hPrefWindow.registerOKCallbackFunc(Shutdown);
  SetFiles();
  ReadFromRegistry()
  dump("parent.roaming.enabled" + parent.roaming.enabled + "\n");
  if (parent.roaming.enabled == false)
    EnableTree(false, E("filesList"));
}

function Shutdown()
{
  dump("shutdown files\n");
  SaveToRegistry();
}


// Logic

// some files have a non-static name. set these filenames here.
function SetFiles()
{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService);
  var prefs = prefService.getBranch(null);
  try {
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
        // Somebody adds unwanted nodes as children to listbox :-(
      continue;
    if (checkbox.getAttribute("filename") == "")
	  checkbox.disabled = true;
  }
}

function SetFile(elementID, filename)
{
  dump ("setfile " + elementID + ": " + filename + "\n");
  var listitem = document.getElementById(elementID);
  listitem.setAttribute("filename", filename);
}


// Registry

function ReadFromRegistry()
{
  if (!parent.roaming)
    parent.roaming = Object();
	if (parent.roaming.loadedFiles == true)
    // prevent overwriting of user changes after pane switch back/forth
		return;

	try {
		// to understand the structure, see comment at the const defs above
		// get the Roaming reg branch
		registry = Components.classes["@mozilla.org/registry;1"]
                         .createInstance(Components.interfaces.nsIRegistry);
		registry.openWellKnownRegistry(registry.ApplicationRegistry);
		var profMan = Components.classes["@mozilla.org/profile/manager;1"]
		                 .getService(Components.interfaces.nsIProfile);
		var regkey = registry.getKey(registry.Common, kRegTreeProfile);
		regkeyProf = registry.getKey(regkey, profMan.currentProfile);
		regkey = registry.getKey(regkeyProf, kRegTreeRoaming);

		// enabled
		var value = registry.getInt(regkey, kRegKeyEnabled);

		// files
		value = registry.getString(regkey, kRegKeyFiles);
		var files = value.split(",");
		// first, disable all defaults
		var children = E("filesList").childNodes;
		for (var i = 0, l = children.length; i < l; i++)
		{
			var checkbox = children[i];
			if (!("getAttribute" in checkbox) ||
				checkbox.getAttribute("type") != "checkbox")
				// Somebody adds unwanted nodes as children to listbox :-(
				continue;
			checkbox.checked = false;
		}
		// then check for each file in the list, if it's in the checkboxes
		// enabled it, if so, otherwise create and enable it
		for (i = 0; i < files.length; i++)
		{
			var file = files[i];
			var found = false;
			for (var i2 = 0, l = children.length; i2 < l; i2++)
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
					E("filesList").appendChild(li);
				}
			}	        		
		}
	} catch (e) {
		if (!regkeyProf)
			dump("error during registry read for Roaming prefs: " + e + "\n");
		// later errors are expected, if we never wrote roaming prefs before,
		// fatal otherwise, but we don't know the difference.
		//else dump(e);
	}
	parent.roaming.loadedFiles = true;
}

function SaveToRegistry()
{
	if (!regkeyProf)
		// arg, we had a fatal error during read, so bail out
		return;
	try {
		var regkey = SaveRegBranch(regkeyProf, kRegTreeRoaming);
		
		// files
		var value = "";
		var children = E("filesList").childNodes;
		for (var i = 0; i < children.length; i++)
    {
      var checkbox = children.item(i);
      if (checkbox.checked)
      {
        var filename = checkbox.getAttribute("filename");
        if (filename)
          value += filename + ",";
      }
    }
    // remove last ","
    if (value.length > 0)
      value = value.substring(0, value.length - 1);
    dump("files: " + value + "\n");
		registry.setString(regkey, kRegKeyFiles, value);
    dump("saved files\n");
	} catch (e) {
		dump("can't write registry entries for Roaming: " + e + "\n");
		return;
	}
}

// gets or creates registry branch, i.e. drop-in replacement for getKey/addKey
// returns either the regkey for branch or nothing, if failed
function SaveRegBranch(baseregkey, branchname)
{
	try {
		return registry.getKey(baseregkey, branchname);
	} catch (e) { // XXX catch selectively
    dump(e.result + "-" + e.name + "-" + e + "\n");
    return registry.addKey(baseregkey, branchname);
    // throw errors to caller
	}
}


// UI

function E(elementID)
{
  return document.getElementById(elementID);
}

function EnableTree(enabled, element)
{
  if(!enabled)
    element.setAttribute("disabled", "true");
  else
    element.removeAttribute("disabled");

  // EnableTree direct children (recursive)
  var children = element.childNodes;
  for (var i = 0; i < children.length; i++)
    EnableTree(enabled, children.item(i));
}
