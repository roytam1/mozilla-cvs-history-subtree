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

var _elementIDs = []; // no prefs (nsIPref) needed, see above

function Startup()
{
  // dataManager.pageData doesn't work, because it needs to work on both panes
  if (!parent.roaming)
    parent.roaming = new RoamingPrefs();
  DataToUI();
}


// UI

// write gData to widgets
function DataToUI()
{
  var data = parent.roaming;
  E("enabled").checked = data.Enabled;
  E("methodSelect").value = data.Method;
  E("streamURL").value = data.Stream.BaseURL;
  E("streamUsername").value = data.Stream.Username;
  E("streamPassword").value = data.Stream.Password;
  E("streamSavePW").checked = data.Stream.SavePW;
  E("copyDir").value = data.Copy.RemoteDir;

  SwitchDeck(data.Method, E("methodSettingsDeck"));
  EnableTree(data.Enabled, E("method"));
  E("streamPassword").disabled = !data.Stream.SavePW || !data.Enabled;
}

// write widget content to data
function UIToData()
{
  var data = parent.roaming;
  data.Enabled = E("enabled").checked;
  data.Method = E("methodSelect").value;
  data.Stream.BaseURL = E("streamURL").value;
  data.Stream.Username = E("streamUsername").value;
  data.Stream.Password = E("streamPassword").value;
  data.Stream.SavePW = E("streamSavePW").checked;
  data.Copy.RemoteDir = E("copyDir").value;
  data.changed = true; // excessive
}

// updates UI (esp. en/disabling of widgets) to match new user input
function SwitchChanged()
{
  UIToData();
  DataToUI();
}
