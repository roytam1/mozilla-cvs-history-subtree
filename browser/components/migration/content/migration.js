
var MigrationWizard = {
  init: function ()
  {
    var importSource = document.getElementById("importSource");
    importSource.selectedItem = document.getElementById("ie");

    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "Migration:Started", false);
    os.addObserver(this, "Migration:ItemBeforeMigrate", false);
    os.addObserver(this, "Migration:ItemAfterMigrate", false);
    os.addObserver(this, "Migration:Ended", false);
  },
  
  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "Migration:Started");
    os.removeObserver(this, "Migration:ItemBeforeMigrate");
    os.removeObserver(this, "Migration:ItemAfterMigrate");
    os.removeObserver(this, "Migration:Ended");
  },
  
  migrate: function ()
  {
    var contractID = "@mozilla.org/profile/migrator;1?app=browser&type=";
    var importSource = document.getElementById("importSource");
    contractID += importSource.selectedItem.id;
    
    const nsIBPM = Components.interfaces.nsIBrowserProfileMigrator;
    var bpm = Components.classes[contractID].createInstance(nsIBPM);
    bpm.migrate(nsIBPM.HISTORY, false);
  },
  
  observe: function (aSubject, aTopic, aData)
  {
    switch (aTopic) {
    case "Migration:Started":
      dump("*** migration started\n");
      break;
    case "Migration:ItemBeforeMigrate":
      dump("*** item about to be migrated...\n");
      break;
    case "Migration:ItemAfterMigrate":
      dump("*** item was migrated...\n");
      break;
    case "Migration:Ended":
      dump("*** migration ended\n");
      break;
    }
  },
};

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is The Browser Profile Migrator.
#
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@bengoodger.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

