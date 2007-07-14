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
 * The Original Code is Content Preferences (cpref).
 *
 * The Initial Developer of the Original Code is Mozilla.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Myk Melez <myk@mozilla.org>
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

function run_test() {
  // It doesn't matter whether or not this executable exists or is executable,
  // only that it'll QI to nsIFile and has a path attribute, which the service
  // expects.
  var executable = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  executable.initWithPath("/usr/bin/test");
  
  var localHandler = {
    name: "Local Handler",
    executable: executable,
    interfaces: [Ci.nsIHandlerApp, Ci.nsILocalHandlerApp, Ci.nsISupports],
    QueryInterface: function(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    }
  };
  
  var webHandler = {
    name: "Web Handler",
    uriTemplate: "http://www.example.com/?%s",
    interfaces: [Ci.nsIHandlerApp, Ci.nsIWebHandlerApp, Ci.nsISupports],
    QueryInterface: function(iid) {
      if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    }
  };
  
  var hamSvc = Cc["@mozilla.org/uriloader/hamburger-helper-service;1"].
               getService(Ci.nsIHamburgerHelperService);

  var mimeSvc = Cc["@mozilla.org/uriloader/external-helper-app-service;1"].
                getService(Ci.nsIMIMEService);

  //**************************************************************************//
  // Round-Trip Data Integrity

  // Test round-trip data integrity by getting a handler info, setting its
  // properties, and then getting it again and making sure the changes have
  // taken effect.

  // Get a handler info and make sure the initial properties are as we expect.
  var handlerInfo = mimeSvc.getFromTypeAndExtension("text/test", null);
  do_check_eq(handlerInfo.MIMEType, "text/test");
  // There shouldn't be a preferred handler yet.
  do_check_eq(handlerInfo.preferredApplicationHandler, null);
  // The default preferred action should be to use the system default.
  do_check_eq(handlerInfo.preferredAction, Ci.nsIHandlerInfo.useSystemDefault);
  // FIXME: Check the always ask flag.
  
  // Set the preferred handler.
  hamSvc.setPreferredHandler(handlerInfo, localHandler);
  // FIXME: Set the preferred action.
  // FIXME: Set the always ask flag.
  
  // Retrieve the handler info again and make sure the changes are in effect.
  handlerInfo = mimeSvc.getFromTypeAndExtension("text/test", null);
  var preferredHandler = handlerInfo.preferredApplicationHandler;
  do_check_eq(typeof preferredHandler, "object");
  do_check_eq(preferredHandler.name, "Local Handler");
  var localHandler = preferredHandler.QueryInterface(Ci.nsILocalHandlerApp);
  do_check_eq(localHandler.executable.path, "/usr/bin/test");
  // FIXME: Check the preferred action.
  // FIXME: Check the always ask flag.
  
  // FIXME: test round trip integrity for a protocol.
  // FIXME: test round trip integrity for a web handler.
}
