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


function encryptMessage()
{
  var encryptionCertName = gCurrentIdentity.getUnicharAttribute("encryption_cert_name");
  if (!encryptionCertName) {
    alert(gComposeMsgsBundle.getString("chooseEncryptionCertMsg"));
    document.getElementById("menu_securityEncryptAlways").removeAttribute("checked");
    return;
  }

  var msgCompFields = msgCompose.compFields;
  if (msgCompFields) {
    if (msgCompFields.alwaysEncryptMessage) {
      msgCompFields.alwaysEncryptMessage = false;
    } else {
      msgCompFields.alwaysEncryptMessage = true;
    }
  }
}

function signMessage()
{
  var signingCertName = gCurrentIdentity.getUnicharAttribute("signing_cert_name");
  if (signingCertName == null) {
    alert(gComposeMsgsBundle.getString("chooseSigningCertMsg"));
    document.getElementById("menu_securitySign").removeAttribute("checked");
    return;
  }

  var msgCompFields = msgCompose.compFields;
  if (msgCompFields) {
    if (msgCompFields.signMessage) {
      msgCompFields.signMessage = false;
    } else {
      msgCompFields.signMessage = true;
    }
  }
}

function initSecuritySettings()
{
  var alwaysEncrypt = gCurrentIdentity.getBoolAttribute("encrypt_mail_always");
  document.getElementById("menu_securityEncryptAlways").setAttribute("checked", alwaysEncrypt);
  var alwaysSign = gCurrentIdentity.getBoolAttribute("sign_mail");
  document.getElementById("menu_securitySign").setAttribute("checked", alwaysSign);
}

function setSecuritySettings()
{
  var msgCompFields = msgCompose.compFields;
  document.getElementById("menu_securityEncryptAlways").setAttribute("checked", msgCompFields.alwaysEncryptMessage);
  document.getElementById("menu_securitySign").setAttribute("checked", msgCompFields.signMessage);
}
