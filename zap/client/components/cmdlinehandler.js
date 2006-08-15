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
 * The Initial Developer of the Original Code is Allan Beaufour
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Allan Beaufour <allan@beaufour.dk> (original author)
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

Components.utils.importModule("gre:ComponentUtils.jsm");

function ZAPCmdLineHandler() {
}

ZAPCmdLineHandler.prototype = {
  handle : function(commandline) {
    debug("ZAPCmdLineHandler: checking for startup options\n");

    // Handle -dial
    var dial;
    try {
      dial = commandline.handleFlagWithParam("dial", false);
    } catch (e) {
      dump("ERROR: -dial needs an argument!!\n\n");
    }
    if (dial) {
      this.dialURL(dial);
    }

    // Handle URL given on the command line
    var count = commandline.length;
    for (var i=0; i<count; ++i) {
      var curarg = commandline.getArgument(i);
      if (curarg.match(/^sips?:/)) {
        this.dialURL(curarg);
      }
    }
  },
  
  helpInfo : "  -dial <url>           Dial the given URL\n"+
             "  [<url>]               Dial the given URL",

  dialURL : function(url) {
    var mainwin = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                            .getService(Components.interfaces.nsIWindowMediator)
                            .getMostRecentWindow('zap_mainwin');
    if (!mainwin) {
      dump("ERROR: Main window not loaded, cannot handle dial command\n");
      return;
    }
    mainwin.wURLField.value = url;
    mainwin.cmdGo();
  }    

};

NSGetModule = ComponentUtils.generateNSGetModule(
  [
    {
      className  : "ZAPCmdLineHandler",
      cid        : Components.ID("fce37800-ad8d-40fe-87e3-3bc9d3c7457a"),
      contractID : "@mozilla.org/zapcmdlinehandler;1",
      factory    : ComponentUtils.generateFactory(function(){ return new ZAPCmdLineHandler();},
                                                  [Components.interfaces.nsICommandLineHandler])
    }
  ],

  function(mgr,file,arr) {
    ComponentUtils.categoryManager.addCategoryEntry("command-line-handler",
                                                    "z-zap-cmd",
                                                    arr[0].contractID,
                                                    true, true);
  },
  function(mgr,file,arr) {
    ComponentUtils.categoryManager.deleteCategoryEntry("command-line-handler",
                                                       "z-zap-cmd", true);
  }
);
