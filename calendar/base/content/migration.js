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
 * The Original Code is Calendar migration code
 *
 * The Initial Developer of the Original Code is
 *   Joey Minta <jminta@gmail.com>
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

const Cc = Components.classes;
const Ci = Components.interfaces;

//
// The front-end wizard bits.
//
var gMigrateWizard = {
    loadMigrators: function gmw_load() {
        var listbox = document.getElementById("datasource-list");
        LOG("migrators:"+window.arguments.length);
        for each (var migrator in window.arguments[0]) {
            var listItem = document.createElement("listitem");
            var checkCell = document.createElement("listcell");
            checkCell.setAttribute("type", "checkbox");

            checkCell.setAttribute("checked", true);
            listItem.appendChild(checkCell);

            var nameCell = document.createElement("listcell");
            nameCell.setAttribute("label", migrator.title);
            listItem.appendChild(nameCell);

            listItem.migrator = migrator;
            listbox.appendChild(listItem);
        }
    },

    migrateChecked: function gmw_migrate() {
        var migrators = [];
        var listbox = document.getElementById("datasource-list");
        for (var i = listbox.childNodes.length-1; i >= 0; i--) {
            LOG("Checking child node"+listbox.childNodes[i].firstChild);
            if (listbox.childNodes[i].firstChild.getAttribute("checked")) {
                migrators.push(listbox.childNodes[i].migrator);
            }
        }
        if (migrators.length == 0) {
            window.close();
        }
        var wizard = document.getElementById('migration-wizard');
        wizard.canAdvance = false;
        wizard.canRewind = false;

        var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                  .getService(Ci.nsIStringBundleService);
        var props = sbs.createBundle("chrome://calendar/locale/calendar.properties");
        var label = document.getElementById("progress-label");
        var meter = document.getElementById("migrate-progressmeter");

        var i = 0;
        function getNextMigrator() {
            if (migrators[i]) {
                LOG("starting migrator:"+migrators[i].title);
                label.value = props.formatStringFromName("migrateMsg",
                                                         [migrators[i].title],
                                                         1);
                meter.value = i/migrators.length*100;
                migrators[i].args.push(getNextMigrator);
                try {
                    migrators[i].migrate.apply(migrators[i], migrators[i].args);
                } catch(ex) {
                    LOG("Failed to migrate:"+migrators[i].title);
                    LOG(ex);
                    i++;
                    getNextMigrator();
                }
            } else {
                LOG("migration done");
                wizard.canAdvance = true;
                label.value = calGetString("calendar", "finished");
                meter.value = 100;
            }
            i++;
        }
        getNextMigrator();
   },

    setCanRewindFalse: function gmw_finish() {
        document.getElementById('migration-wizard').canRewind = false;
    }
};

//
// The more back-end data detection bits
//
function dataMigrator(aTitle, aMigrateFunction, aArguments) {
    this.title = aTitle;
    this.migrate = aMigrateFunction;
    this.args = aArguments;
}

var gDataMigrator = {
    _isLightning: false,
    /**
     * Call to do a general data migration (for a clean profile)
     */
    checkAndMigrate: function gdm_migrate() {
        var appInfo = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULAppInfo);
        if (appInfo.name == "Thunderbird") {
            this._isLightning = true;
        }
        LOG("_isLightning is:"+this._isLightning);

        var DMs = [];
        var migrators = [this.checkCalExt, this.checkIcal, this.checkEvolution,
                 this.checkOutlook, this.checkMozApps];
        //XXX also define a category and an interface here for pluggability
        for each (var migrator in migrators) {
            var migs = migrator();
            for each (var dm in migs) {
                DMs.push(dm);
            }
        }

        if (DMs.length == 0) {
            // No migration available
            return;
        }
        LOG("DMs:"+DMs.length);
        openDialog("chrome://calendar/content/migrationWizard.xul", "migrate",
                   "chrome,titlebar,modal,resizable", DMs);
    },

    /**
     * Checks to see if the old calendar extension was installed in Firefox
     * or Thunderbird.  If so, it offers to move that data into our new storage 
     * format. Also, if we're Lightning, it will disable the old calendar 
     * extension, since it conflicts with us.
     */
    checkCalExt: function gdm_calext() {
        LOG("checking the old cal extension");

        function extMigrator(aProfileDir, aCallback) {
            // Get the old datasource
            var dataSource = aProfileDir.clone();
            dataSource.append("CalendarManager.rdf");
            if (!dataSource.exists()) {
                return;
            }

            // Let this be a lesson to anyone designed APIs.  The RDF API is so
            // impossibly confusing that it's actually simpler/cleaner/shorter
            // to simply parse as XML and use the better DOM APIs.
            var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                      .createInstance(Ci.nsIXMLHttpRequest);
            req.open('GET', "file://"+dataSource.path, true);
            req.onreadystatechange = function calext_onreadychange() {
                if (req.readyState == 4) {
                    LOG(req.responseText);
                    parseAndMigrate(req.responseXML, aCallback)
                }
            };
            req.send(null);
       }

       function parseAndMigrate(aDoc, aCallback) {
            // For duplicate detection
            var calManager = getCalendarManager();
            var uris = [];
            for each (var oldCal in calManager.getCalendars({})) {
                uris.push(oldCal.uri);
            }

            function getRDFAttr(aNode, aAttr) {
                return aNode.getAttributeNS("http://home.netscape.com/NC-rdf#",
                                            aAttr);
            }

            const RDFNS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
            var nodes = aDoc.getElementsByTagNameNS(RDFNS, "Description");
            LOG("nodes:"+nodes.length);
            for (var i = 0; i < nodes.length; i++) {
                LOG("Beginning cal node");
                var cal;
                var node = nodes[i];
                if (getRDFAttr(node, "remote") == "false") {
                    LOG("not remote");
                    var localFile = Cc["@mozilla.org/file/local;1"]
                                   .createInstance(Ci.nsILocalFile);
                    localFile.initWithPath(getRDFAttr(node, "path"));
                    cal = gDataMigrator._importICSToStorage(localFile);
                } else {
                    // Remote subscription
                    //XXX check for duplicates
                    var url = makeURL(getRDFAttr(node, "remotePath"));
                    cal = calManager.createCalendar("ics", url);
                }
                cal.name = getRDFAttr(node, "name");
                calManager.setCalendarPref(cal, "color", 
                                           getRDFAttr(node, "color"));
            }
            aCallback();
        }

        var migrators = [];

        var dirService = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties);
        var profileDir = dirService.get("ProfD", Ci.nsILocalFile);
        profileDir.append("Calendar");
        if (profileDir.exists()) {
            LOG("Found old extension directory");
            var title;
            if (this._isLightning) {
                title = "Mozilla Calendar";
            } else {
                title = "Sunbird 0.2";
            }
            migrators.push(new dataMigrator(title, extMigrator, [profileDir]));
        }

        // Get the Firefox profile.  For Sunbird, we go up 1 level and then into 
        // "Firefox"  For Thunderbird, it's harder
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
        var ffSpec = ioService.newFileURI(profileDir).path;

        if (!this._isLightning) {
            var diverge = ffSpec.indexOf("Sunbird");
            ffSpec = ffSpec.substr(0, diverge);
            var localFile = Cc["@mozilla.org/file/local;1"]
                            .createInstance(Ci.nsILocalFile);
            localFile.initWithPath(ffSpec);
        } else {
            //XXX
            // on MacOSX see the ical migration.  On other platforms, we need
            // to do other things.  Thunderbird sucks.
            return migrators;
        }

        localFile.append("Firefox");
        localFile.append("Profiles");

        if (!localFile.exists()) {
            // No firefox
            return migrators;
        }
        var dirEnum = localFile.directoryEntries;
        while (dirEnum.hasMoreElements()) {
            var profile = dirs.getNext().QueryInterface(Ci.nsIFile);
            if (profile.isFile()) {
                continue;
            } else {
                break;
            }
        }

        profile.append("Calendar");
        if (profile.exists()) {
            LOG("Found old extension directory");
            var title = "Firefox Calendar";
            migrators.push(new dataMigrator(title, extMigrator, [profile]));
        }

        return migrators;
    },

    /**
     * Checks to see if Apple's iCal is installed and offers to migrate any data
     * the user has created in it.
     */
    checkIcal: function gdm_ical() {
        LOG("Checking for ical data");

        function icalMigrate(aDataDir, aCallback) {
            aDataDir.append("Sources");
            var dirs = aDataDir.directoryEntries;
            var calManager = getCalendarManager();

            var i = 1;
            while(dirs.hasMoreElements()) {
                var dataDir = dirs.getNext().QueryInterface(Ci.nsIFile);
                var dataStore = dataDir.clone();
                dataStore.append("corestorage.ics");
                if (!dataStore.exists()) {
                    continue;
                }

                //XXX This won't work if there aren't timezones in the file. :-(
                var cal = gDataMigrator._importICSToStorage(dataStore);
                //XXX
                cal.name = "iCalendar"+i;
                i++;
            }
            aCallback();
        }
        var dirService = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties);
        var profileDir = dirService.get("ProfD", Ci.nsILocalFile);
        var icalSpec = profileDir.path;
        var icalFile;
        if (!this._isLightning) {
            var diverge = icalSpec.indexOf("Sunbird");
            if (diverge == -1) {
                return [];
            }
            icalSpec = icalSpec.substr(0, diverge);
            icalFile = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
            icalFile.initWithPath(icalSpec);
        } else {
            var diverge = icalSpec.indexOf("Thunderbird");
            if (diverge == -1) {
                return [];
            }
            icalSpec = icalSpec.substr(0, diverge);
            icalFile = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
            icalFile.initWithPath(icalSpec);
            icalFile.append("Application Support");
        }
        icalFile.append("iCal");
        if (icalFile.exists()) {
            return [new dataMigrator("Apple iCal", icalMigrate, [icalFile])];
        }

        return [];
    },

    /**
     * Checks to see if Evolution is installed and offers to migrate any data
     * stored there.
     */
    checkEvolution: function gdm_evolution() {
        LOG("Checking for evolution data");

        function evoMigrate(aDataDir, aCallback) {
            aDataDir.append("Sources");
            var dirs = aDataDir.directoryEntries;
            var calManager = getCalendarManager();

            var i = 1;
            while(dirs.hasMoreElements()) {
                var dataDir = dirs.getNext().QueryInterface(Ci.nsIFile);
                var dataStore = dataDir.clone();
                dataStore.append("calendar.ics");
                if (!dataStore.exists()) {
                    continue;
                }

                var cal = gDataMigrator._importICSToStorage(dataStore);
                //XXX
                cal.name = "Evolution"+i;
                i++;
            }
            aCallback();
        }

        var dirService = Cc["@mozilla.org/file/directory_service;1"]
                         .getService(Ci.nsIProperties);
        var profileDir = dirService.get("ProfD", Ci.nsILocalFile);
        var evoSpec = profileDir.path;
        var evoFile;
        if (!this._isLightning) {
            var diverge = evoSpec.indexOf(".mozilla");
            if (diverge == -1) {
                return [];
            }
            evoSpec = evoSpec.substr(0, diverge);
            evoFile = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
            evoFile.initWithPath(icalSpec);
        } else {
            var diverge = icalSpec.indexOf(".thunderbird");
            if (diverge == -1) {
                return [];
            }
            evoSpec = evoSpec.substr(0, diverge);
            evoFile = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
            evoFile.initWithPath(evoSpec);
        }
        evoFile.append(".evolution");
        evoFile.append("calendar");
        evoFile.append("local");
        evoFile.append("system");
        if (evoFile.exists()) {
            return [new dataMigrator("Evolution", evoMigrate, [evoFile])];
        }
        return [];
    },

    /**
     * Checks to see if Outlook is installed and offers to migrate any data
     * stored in it.
     */
    checkOutlook: function gdm_outlook() {
        LOG("Checking for outlook data");
        return [];
    },

     /**
      * Checks to see if there is calendar data from another mozilla app.
      * ie. We're Lightning and Sunbird was previously installed.
      */
    checkMozApps: function gdm_mozApps() {
        LOG("Checking for other mozilla calendaring applications");
        //XXX This is all pretty easy except for the fact that Thunderbird had
        //    to go put its profile in a weird place, that varies by platform.
        return [];
    },

    checkVistaCal: function gdm_vista() {
    },

    _importICSToStorage: function migrateIcsStorage(icsFile) {
        var calManager = getCalendarManager();
        var uris = [];
        for each (var oldCal in calManager.getCalendars({})) {
            uris.push(oldCal.uri.spec);
        }
        var uri = 'moz-profile-calendar://?id=';
        var i = 1;
        while (uris.indexOf(uri+i) != -1) {
            i++;
        }

        var cal = calManager.createCalendar("storage", makeURL(uri+i));
        var icsImporter = Cc["@mozilla.org/calendar/import;1?type=ics"]
                          .getService(Ci.calIImporter);

        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                          .createInstance(Ci.nsIFileInputStream);
        inputStream.init(icsFile, MODE_RDONLY, 0444, {} );
        // Yeah, this is really cheating with scope.  gItems is defined in
        // import-export.js, and putItemsIntoCal relies on it
        gItems = icsImporter.importFromStream(inputStream, {});
        putItemsIntoCal(cal);

        calManager.registerCalendar(cal);
        return cal;
    }
};

function LOG(aString) {
    if (!getPrefSafe("calendar.migration.log", false)) {
        return;
    }
    var consoleService = Cc["@mozilla.org/consoleservice;1"]
                         .getService(Ci.nsIConsoleService);
    consoleService.logStringMessage(aString);
    dump(aString+'\n');
}
