/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributors:
 *   ddrinan@netscape.com
 */

var gIncomingServer;
var gServerType;
var gIdentity;
var gPref = null;
var gLockedPref = null;

function onInit() 
{
    onLockPreference();	

    // init values here
    document.getElementById("encryption.ifPossibleEncryptMessage").checked =  gIdentity.ifPossibleEncryptMessage;
    document.getElementById("encryption.alwaysEncryptMessage").checked =  gIdentity.alwaysEncryptMessage;
    if (gIdentity.encryptionCertName) {
        document.getElementById("encryption.certificateName").setAttribute("value", gIdentity.encryptionCertName);
    } else {
        document.getElementById("encryption.certificateName").setAttribute("value", "");
        document.getElementById("encryption.alwaysEncryptMessage").setAttribute("disabled", "true");
    }
    document.getElementById("signing.signMessage").checked =  gIdentity.signMessage;
    if (gIdentity.signingCertName) {
        document.getElementById("signing.certificateName").setAttribute("value", gIdentity.signingCertName);
    } else {
        document.getElementById("signing.certificateName").setAttribute("value", "");
        document.getElementById("signing.signMessage").setAttribute("disabled", "true");
    }

    // Disable the encrypt if possibe check for the moment
    document.getElementById("encryption.ifPossibleEncryptMessage").setAttribute("disabled", "true");
}

function onPreInit(account, accountValues)
{
    gServerType = getAccountValue(account, accountValues, "server", "type");
    hideShowControls(gServerType);
    gIdentity = account.defaultIdentity;
}

function hideShowControls(type)
{
    
    var controls = document.getElementsByAttribute("hidable", "true");
    var len = controls.length;

    for (var i=0; i<len; i++) {
        var control = controls[i];

        var hideFor = control.getAttribute("hidefor");

        if (!hideFor)
            throw "this should not happen, things that are hidable should have hidefor set";

        var box = getEnclosingContainer(control);

        if (!box)
            throw "this should not happen, things that are hidable should be in a box";

        // hide unsupported server type
        // adding support for hiding multiple server types using hideFor="server1,server2"
        var hideForBool = false;
        var hideForTokens = hideFor.split(",");
        for (var j = 0; j < hideForTokens.length; j++) {
            if (hideForTokens[j] == type) {
                hideForBool = true;
                break;
            }
        }

        if (hideForBool) {
            box.setAttribute("hidden", "true");
        }
        else {
            box.removeAttribute("hidden");
        }
    }
}

function getEnclosingContainer(startNode) {

    var parent = startNode;
    var box;

    while (parent && parent != document) {

    var isContainer = (parent.getAttribute("iscontrolcontainer") == "true");

    if (!box || isContainer)
        box=parent;

    // break out with a controlcontainer
    if (isContainer)
        break;
    parent = parent.parentNode;
    }

    return box;
}

function onSave()
{

    gIdentity.ifPossibleEncryptMessage = document.getElementById("encryption.ifPossibleEncryptMessage").checked;
    gIdentity.alwaysEncryptMessage = document.getElementById("encryption.alwaysEncryptMessage").checked;
    if (document.getElementById("encryption.certificateName").value) {
        gIdentity.encryptionCertName = document.getElementById("encryption.certificateName").value;
    }
    gIdentity.signMessage = document.getElementById("signing.signMessage").checked;
    if (document.getElementById("signing.certificateName").value) {
        gIdentity.signingCertName = document.getElementById("signing.certificateName").value;
    }
}

// Does the work of disabling an element given the array which contains xul id/prefstring pairs.
// Also saves the id/locked state in an array so that other areas of the code can avoid
// stomping on the disabled state indiscriminately.
function disableIfLocked( prefstrArray )
{
    if (!gLockedPref)
      gLockedPref = new Array;

    for (i=0; i<prefstrArray.length; i++) {
        var id = prefstrArray[i].id;
        var element = document.getElementById(id);
        if (gPref.prefIsLocked(prefstrArray[i].prefstring)) {
            element.disabled = true;
            gLockedPref[id] = true;
        } else {
            element.removeAttribute("disabled");
            gLockedPref[id] = false;
        }
    }
}

// Disables xul elements that have associated preferences locked.
function onLockPreference()
{
    var isDownloadLocked = false;
    var isGetNewLocked = false;
    var initPrefString = "mail.server"; 
    var finalPrefString; 

    var prefService = Components.classes["@mozilla.org/preferences-service;1"];
    prefService = prefService.getService();
    prefService = prefService.QueryInterface(Components.interfaces.nsIPrefService);

    // This panel does not use the code in AccountManager.js to handle
    // the load/unload/disable.  keep in mind new prefstrings and changes
    // to code in AccountManager, and update these as well.
    var allPrefElements = [
      { prefstring:"encrypt_if_possible", id:"encryption.ifPossibleEncryptMessage"},
      { prefstring:"encrypt_always", id:"encryption.alwaysEncryptMessage"},
      { prefstring:"encryption_cert_name", id:"encryption.certificateName"},
      { prefstring:"sign", id:"signing.signMessage"},
      { prefstring:"signing_cert_name", id:"signing.certificateName"}
    ];

    finalPrefString = initPrefString + "." + gIdentity.key + ".";
    gPref = prefService.getBranch(finalPrefString);

    disableIfLocked( allPrefElements );
} 

function smimeSelectCert(smime_cert)
{
  var picker = Components.classes["@mozilla.org/user_cert_picker;1"]
               .getService(Components.interfaces.nsIUserCertPicker);
  var canceled = new Object;
  var x509cert = 0;
  var certUsage;
  var prefBundle = document.getElementById("bundle_prefs");

  if (smime_cert == "encryption.certificateName") {
    certUsage = 5;
  } else if (smime_cert == "signing.certificateName") {
    certUsage = 4;
  }

  try {
    x509cert = picker.pickByUsage(window,
      "X",
      "Y",
      certUsage, // this is from enum SECCertUsage
      false, false, canceled);
  } catch(e) {
    // XXX display error message in the future
  }

  if (!canceled.value && x509cert) {
    var certInfo = document.getElementById(smime_cert);
    if (certInfo) {
      certInfo.setAttribute("disabled", "false");
      certInfo.value = x509cert.nickname;

      if (smime_cert == "encryption.certificateName") {
        document.getElementById("encryption.alwaysEncryptMessage").removeAttribute("disabled");
      } else {
        document.getElementById("signing.signMessage").removeAttribute("disabled");
      }
	  }
  }
}
