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
 *  Scott MacGregor <mscott@netscape.com>
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

var gCurrentHeight = 0;
var gFinalHeight = 50;
var gWidth = 260;
var gSlideIncrement = 1;
var gSlideTime = 10;
var gOpenTime = 3000; // total time the alert should stay up once we are done animating.

var gAlertListener = null;
var gAlertTextClickable = false;
var gAlertCookie = "";

function onAlertLoad()
{
  // unwrap all the args....
  // arguments[0] --> the image src url
  // arguments[1] --> the alert title
  // arguments[2] --> the alert text
  // arguments[3] --> is the text clickable? 
  // arguments[4] --> the alert cookie to be passed back to the listener

  document.getElementById('alertImage').setAttribute('src', window.arguments[0]);
  document.getElementById('alertTitleLabel').firstChild.nodeValue = window.arguments[1];
  document.getElementById('alertTextLabel').firstChild.nodeValue = window.arguments[2];
  gAlertTextClickable = window.arguments[3];
  gAlertCookie = window.arguments[4];
 
  window.outerWidth = 40 + window.arguments[2].length * 7; // icon is 26 pixels

  if (gAlertTextClickable)
    document.getElementById('alertTextLabel').setAttribute('clickable', true);
  
  // the 5th argument is optional
  if (window.arguments[5])
   gAlertListener = window.arguments[5].QueryInterface(Components.interfaces.nsIAlertListener);

  try 
  {
    var prefService = Components.classes["@mozilla.org/preferences-service;1"].getService();
    prefService = prefService.QueryInterface(Components.interfaces.nsIPrefService);
    var prefBranch = prefService.getBranch(null);
    gSlideIncrement = prefBranch.getIntPref("alerts.slideIncrement");
    gSlideTime = prefBranch.getIntPref("alerts.slideIncrementTime");
    gOpenTime = prefBranch.getIntPref("alerts.totalOpenTime");
    gFinalHeight = prefBranch.getIntPref("alerts.height");
  } catch (ex) {}

  gWidth = window.outerWidth + 10;

  window.moveTo(screen.availWidth, screen.availHeight); // move it offscreen initially
  setTimeout('window.resizeTo(gWidth - 10, 1);', 1);   
  setTimeout('animateAlert();', 10);
}

function animateAlert()
{
  if (gCurrentHeight < gFinalHeight)
  {
    gCurrentHeight += gSlideIncrement;
    window.outerHeight = gCurrentHeight;
    window.moveTo((screen.availWidth - gWidth), screen.availHeight - gCurrentHeight); 
    setTimeout('animateAlert();', gSlideTime);
  }
  else
   setTimeout('closeAlert();', gOpenTime);  
}

function closeAlert()
{
  if (gCurrentHeight)
  {
    gCurrentHeight -= gSlideIncrement;
    window.outerHeight = gCurrentHeight;
    window.moveTo((screen.availWidth - gWidth), screen.availHeight - gCurrentHeight); 
    setTimeout('closeAlert();', gSlideTime);
  }
  else
  {
    if (gAlertListener)
      gAlertListener.onAlertFinished(gAlertCookie); 
    window.close(); 
  }
}

function onAlertClick()
{
  if (gAlertListener && gAlertTextClickable)
    gAlertListener.onAlertClickCallback(gAlertCookie);
}
