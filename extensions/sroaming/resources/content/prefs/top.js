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

/* The backend needs the settings in the Mozilla app registry
   (via nsIRegistry), not in the prefs system (nsIPref;
   it is not yet running when needed). This file implements
   the saving of the settings (from the dialog) to the registry.

   Oh Shit. When the user switches panes, the dialog loses all state. Nothing
   restores it. But we can't save it to the registry either, the user might
   cancel later. So, we have to store all values in our own, internal data
   structure and later save it to the registry, if the user clicked OK.

   To unify matters, I share a data structure between top.xul and files.js.
*/

//from mozSRoaming*.cpp
const kRegTreeProfile = "Profiles";
const kRegTreeRoaming = "Roaming";
const kRegKeyEnabled = "Enabled";
const kRegKeyProtocol = "Protocol";
const kRegKeyFiles = "Files";
const kRegValProtocolStream = "stream";
const kRegValProtocolCopy = "copy";
const kRegTreeStream = "Stream";
const kRegKeyStreamURL = "BaseURL";
const kRegKeyStreamUsername = "Username";
const kRegKeyStreamPassword = "Password";
const kRegKeyStreamSavePW = "SavePassword";
const kRegTreeCopy = "Copy";
const kRegKeyCopyDir = "RemoteDir";
// Registry layout:
// (root) (tree)
// + Profile (tree)
//   + (current profile) (tree)
//     + Roaming (tree)
//       + Enabled (int)
//       + Protocol (string, really enum: stream or copy)
//       + Files (string, comma-separated list)
//       + Stream (tree)
//       | + BaseURL (string)
//       | + Username (string)
//       | + SavePW (int, really bool)
//       | + Password (string, encrypted)
//       + Copy (tree)
//         + RemoteDir (string)

const kXULNS ="http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var _elementIDs = []; // no prefs (nsIPref) needed, see above
var registry;
var regkeyProf; // The registry branch with for the current profile.

function Startup()
{
  dump("startup top\n");
  if (regkeyProf) // if we switched panes and back, don't overwrite changes
    return;
  parent.hPrefWindow.registerOKCallbackFunc(Shutdown);
  ReadFromRegistry();
  InitUI();
}

function Shutdown()
{
  dump("shutdown top\n");
  SaveToRegistry();
}


// Registry

function ReadFromRegistry()
{
  if (!parent.roaming)
    parent.roaming = Object();
	if (parent.roaming.loadedTop == true)
    // prevent overwriting of user changes after pane switch back/forth
		return;

	// set default values, matching the hardcoded ones, in case we fail below
	parent.roaming.enabled = false;
	E("enabled").checked = false;
	E("protocolSelect").value = 0;
	E("streamSavePW").checked = false;

	try {
		/* to understand the structure of the registry,
       see comment at the const defs above */
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
		E("enabled").checked = value == 1 ? true : false;

		// protocol (in the sense of the roaming implementation) selection
		value = registry.getString(regkey, kRegKeyProtocol);
		if (value == kRegValProtocolStream)
			E("protocolSelect").value = 0;
		else if (value == kRegValProtocolCopy)
			E("protocolSelect").value = 1;
		else
			E("protocolSelect").value = 0;

		// stream
		var regkeyRoaming = regkey; // save for use in "copy"
		regkey = registry.getKey(regkeyRoaming, kRegTreeStream);
		E("streamURL").value = registry.getString(regkey, kRegKeyStreamURL);
		E("streamUsername").value = registry.getString(regkey,
	                                                 kRegKeyStreamUsername);
		E("streamPassword").value = registry.getString(regkey,
		                                             kRegKeyStreamPassword);
		value = registry.getInt(regkey, kRegKeyStreamSavePW);
		E("streamSavePW").checked = value == 0 ? false : true;

		// copy
		regkey = registry.getKey(regkeyRoaming, kRegTreeCopy);
		E("copyDir").value = registry.getString(regkey, kRegKeyCopyDir);

		// Note that read and write all values. We want to remember them,
		// even if not used at the moment.
	} catch (e) {
		if (!regkeyProf)
			dump("error during registry read for Roaming prefs: " + e + "\n");
		// later errors are expected, if we never wrote roaming prefs before,
		// fatal otherwise, but we don't know the difference.
		else
      dump(e);
	}
  parent.roaming.loadedTop = true;
       /* we might have failed above, but in this case, there is probably no
          sense trying it again, so treat that the same as loaded. */
}

function SaveToRegistry()
{
	if (!regkeyProf)
		// arg, we had a fatal error during read, so bail out
		return;
	try {
		var regkey = SaveRegBranch(regkeyProf, kRegTreeRoaming);

		// enabled
		registry.setInt(regkey, kRegKeyEnabled, E("enabled").checked ? 1 : 0);

		// protocol (in the sense of the roaming implementation) selection
		if (E("protocolSelect").value == 0)
			value = kRegValProtocolStream;
		else if (E("protocolSelect").value == 1)
			value = kRegValProtocolCopy;
		else // huh??
			value = kRegValProtocolStream;
		registry.setString(regkey, kRegKeyProtocol, value);

		// stream
		var regkeyRoaming = regkey; // save for use in "copy"
		regkey = SaveRegBranch(regkeyRoaming, kRegTreeStream);
		registry.setString(regkey, kRegKeyStreamURL, E("streamURL").value);
		registry.setString(regkey, kRegKeyStreamUsername,
		                                         E("streamUsername").value);
		registry.setString(regkey, kRegKeyStreamPassword,
                E("streamSavePW").checked ? E("streamPassword").value : "");
		registry.setInt(regkey, kRegKeyStreamSavePW,
		                                 E("streamSavePW").checked ? 1 : 0);

		// copy
		regkey = SaveRegBranch(regkeyRoaming, kRegTreeCopy);
		registry.setString(regkey, kRegKeyCopyDir, E("copyDir").value);

    dump("saved top\n");
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
		try { //XXX does that work? try in catch?
			return registry.addKey(baseregkey, branchname);
		} catch (e2) {
			throw e2;
		}
	}
	// throw unexpected errors to caller
}


// UI

function SwitchDeck(page, deckElementID)
{
  document.getElementById(deckElementID).setAttribute("selectedIndex", page);
}

function EnableElement(enabled, triggerElement, elementID)
{
  if (document.getElementById(elementID).getAttribute("checked") == "false")
    /* prevent that savepassword enables password, although roaming is
       disabled */
    return;

  var element = document.getElementById(elementID);
  if(!enabled)
    element.setAttribute("disabled", "true");
  else
    element.removeAttribute("disabled");
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

/* Some widgets change other widgets based on their own state.
   trigger the oncommand handlers, so that the initial state is consistent. */
function InitUI()
{
  InitElement("streamSavePW"); // do this before "enabled", because in some
                               // case might overwrite "enabled" otherwise,
                               // but "enabled" must take precedence.
  InitElement("enabled");
  InitElement("protocolSelect");
}

function InitElement(elementID)
{
  var e = document.getElementById(elementID);
  eval(e.getAttribute("oncommand").replace(/this/g, "e")); //hackish, but WFM
}

function E(elementID)
{
  return document.getElementById(elementID);
}
