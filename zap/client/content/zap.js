// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "getWindows()[0]" -*-
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
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

////////////////////////////////////////////////////////////////////////
// imports

Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/RDFUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/FileUtils.js");

////////////////////////////////////////////////////////////////////////
// globals

var wPromptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
var wRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
var wRDFContainerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"].createInstance(Components.interfaces.nsIRDFContainerUtils);

var wInteractPane;
var wURLField;

var wSidebarTree;
var wSidebarDS;

var wPrefs;
var wPrefsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("prefs.rdf"));

var wLocationsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("locations.rdf"));
var wLocationsContainer;
var wCurrentLocation;

var wAccountsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("accounts.rdf"));
var wAccountsContainer;

var wContactsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("contacts.rdf"));
// all contacts live in the urn:mozilla:zap:contacts sequence:
var wContactsContainer;
// ... those displayed in the sidebar are also pointed to from the
// urn:mozilla:zap:friends sequence:
var wFriendsContainer;

var wCallsTree;
var wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));
var wCallsContainer;
var wRecentCallsContainer;

// an application-global ds for storing emphemeral state, e.g. whether
// a registration is active or not:
var wGlobalEphemeralDS = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].createInstance(Components.interfaces.nsIRDFDataSource);

var wSipStack;
var wSdpService;

var wErrorLog;

////////////////////////////////////////////////////////////////////////
// Initialization:

function windowInit() {
  dump("Initializing zap main window...\n");

  // set up error logging:
  // XXX this should be configurable (enable/disable)
  wErrorLog = openFileForWriting(getProfileFile("zap.log"), true);
  ErrorReporterSink.reporterFunction = function (mes) {
    if (wErrorLog)
      wErrorLog.write(mes,mes.length);
    else
      dump(mes);
  };
  
  wInteractPane = document.getElementById("interactpane");
  wURLField = document.getElementById("url_field");

  initPrefs();
  initLocations();
  initAccounts();
  initContacts();
  initCalls();
  initSidebar();
  wMediaPipeline.init();
  initSipStack();

  // set the current location:
  var locationsList = document.getElementById("locations_list_popup");
  locationsList.database.AddDataSource(wLocationsDS);
  locationsList.builder.rebuild();
  // select correct location in locations list:
  document.getElementById("locations_list").selectedItem = document.getElementById(wPrefs["urn:mozilla:zap:location"]);
  // make sure out stack is configured for this location profile:
  wCurrentLocation = Location.instantiate();
  wCurrentLocation.initWithResource(wRDF.GetResource(wPrefs["urn:mozilla:zap:location"]));
  currentLocationUpdated();
  
  dump("... Done initializing zap main window\n");
}

function windowClose() {
  cleanupSipStack();
  wMediaPipeline.cleanup();
  
  // close error log:
  wErrorLog.close();
  wErrorLog = null;
}

////////////////////////////////////////////////////////////////////////
// UI Commands:

function cmdAbout() {
  window.openDialog("chrome://zap/content/about.xul",
                    "about",
                    "chrome,modal,alwaysRaised",
                    null);
}

function cmdExit() {
  goQuitApplication();
}

function cmdGo() {
  loadPage("chrome://zap/content/make-call.xul", true);
}

function cmdGenericRequest() {
  loadPage("chrome://zap/content/generic-request.xul", false);
}

function cmdRegisterRequest() {
  loadPage("chrome://zap/content/register-request.xul", false);
}
  
////////////////////////////////////////////////////////////////////////
// Interaction pane:

function loadPage(url, force)
{
  clearSidebarSelection();
  if (wInteractPane.getAttribute("src") != url)
    wInteractPane.setAttribute("src", url);
  else if (force)
    reloadPage();
}

function reloadPage()
{
  wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// Prefs:

function initPrefs() {
  wPrefs = Prefs.instantiate();
  wPrefs.initWithResource(wRDF.GetResource("urn:mozilla:zap:prefs"));
}

var Prefs = makeClass("Prefs", PersistentRDFObject);

Prefs.prototype.datasources["default"] = wPrefsDS;

Prefs.rdfResourceAttrib("urn:mozilla:zap:location",
                        "urn:mozilla:zap:initial_location");
Prefs.rdfLiteralAttrib("urn:mozilla:zap:max_recent_calls", "10");
Prefs.rdfLiteralAttrib("urn:mozilla:zap:dnd_code", "480"); // Temporarily unavail.
Prefs.rdfLiteralAttrib("urn:mozilla:zap:dnd_headers", ""); // additional headers for DND response

////////////////////////////////////////////////////////////////////////
// Locations:

function initLocations() {
  wLocationsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wLocationsContainer.Init(wLocationsDS,
                           wRDF.GetResource("urn:mozilla:zap:locations"));
}

// called when the user selects a different location in the locations_list:
function locationChange() {
  var l = document.getElementById("locations_list").selectedItem.getAttribute("id");
  if (wPrefs["urn:mozilla:zap:location"] == l)
    return;
  else {
    wPrefs["urn:mozilla:zap:location"] = l;
    wPrefs.flush();
  }
  wCurrentLocation = Location.instantiate();
  wCurrentLocation.initWithResource(wRDF.GetResource(wPrefs["urn:mozilla:zap:location"]));
  currentLocationUpdated();
}

// we call this function every time the current location profile is
// updated or a new one is selected so that we can reconfigure the
// stack from here. Alternatively we could listen in on the location
// and prefs datasources, which we should probably do at some point.
function currentLocationUpdated() {
  // XXX these try/catch blocks will be redundant once we hook up
  // syntax checking to PersistentRDFObject
  try {
    wSipStack.FromAddress = wSipStack.syntaxFactory.deserializeAddress(wCurrentLocation["urn:mozilla:zap:from_address"]);
  }
  catch(e) {
    alert("The From-Address in the currently selected location has invalid syntax");
  }
  try {
    var routeSet = wSipStack.syntaxFactory.deserializeRouteSet(wCurrentLocation["urn:mozilla:zap:route_set"], {});
    wSipStack.setDefaultRoute(routeSet, routeSet.length);
  }
  catch(e) {
    alert("The Route Set in the currently selected location has invalid syntax");
  }

  currentAccountsUpdated();
}

//----------------------------------------------------------------------
// Location class

var Location = makeClass("Location", PersistentRDFObject, SupportsImpl);
Location.addInterfaces(Components.interfaces.zapISipCredentialsProvider);

Location.prototype.datasources["default"] = wLocationsDS;

Location.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                           "urn:mozilla:zap:locations");
Location.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                          "chrome://zap/content/location.xul");
Location.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Location.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "location");
Location.rdfLiteralAttrib("urn:mozilla:zap:from_address",
                          "Anonymous <sip:thisis@anonymous.invalid>");
Location.rdfLiteralAttrib("urn:mozilla:zap:route_set", "");

Location.spec(
  function createFromDocument(doc) {
    this._Location_createFromDocument(doc);
    // append to locations:
    wLocationsContainer.AppendElement(this.resource);
  });

// zapISipCredentialsProvider methods:
Location.fun(
  function getCredentialsForRealm(realm, username, password) {
    // try our active accounts first:
    var accounts = wLocationsDS.GetTargets(this.resource,
                                           wRDF.GetResource("urn:mozilla:zap:account"),
                                           true);
    while (accounts.hasMoreElements()) {
      var ar = accounts.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      var account = getAccount(ar);
      if (account["urn:mozilla:zap:authentication_realm"] == realm) {
        username.value = account["urn:mozilla:zap:authentication_username"];
        password.value = account["urn:mozilla:zap:authentication_password"];
        return true;
      }
    }
    // our accounts don't have the credentials; prompt instead:
    var rv = wPromptService.promptUsernameAndPassword(null, "Enter credentials",
                                                      "Enter credentials for realm "+
                                                      realm+":",
                                                      username,
                                                      password,
                                                      null,
                                                      {});
    return rv;
  });

////////////////////////////////////////////////////////////////////////
// Accounts:


function initAccounts() {
  wAccountsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wAccountsContainer.Init(wAccountsDS,
                          wRDF.GetResource("urn:mozilla:zap:accounts"));
}

var Account = makeClass("Account", PersistentRDFObject);

Account.prototype.datasources["default"] = wAccountsDS;
Account.prototype.datasources["locations"] = wLocationsDS;
Account.prototype.datasources["global-ephemeral"] = wGlobalEphemeralDS;
Account.addInMemoryDS("ephemeral");

Account.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                          "urn:mozilla:zap:accounts");
Account.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                         "chrome://zap/content/account.xul");
Account.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "account");
Account.rdfLiteralAttrib("urn:mozilla:zap:authentication_realm", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:authentication_username", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:authentication_password", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:automatic_registration", "false");
Account.rdfLiteralAttrib("urn:mozilla:zap:registrar_server", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:address_of_record", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:suggested_registration_interval", "60");

// A pointer so that we can use the current account as a parameter in
// a template rule triple. See account-template.xul
Account.rdfPointerAttrib("urn:mozilla:zap:root",
                         "urn:mozilla:zap:current-account",
                         "ephemeral");

Account.spec(
  function createFromDocument(doc) {
    this._Account_createFromDocument(doc);
    // append to accounts:
    wAccountsContainer.AppendElement(this.resource);
    this.updateLocationProfiles(doc);
  });

Account.spec(
  function updateFromDocument(doc) {
    this._Account_updateFromDocument(doc);
    this.updateLocationProfiles(doc);
  });

// helper to update resource links from location profiles to this
// account:
Account.fun(
  function updateLocationProfiles(doc) {
    // iterate through locations container and add/clear
    // (location,account,this) triples:
    var locations = wLocationsContainer.GetElements();
    var resourceAccount = wRDF.GetResource("urn:mozilla:zap:account");
    while (locations.hasMoreElements()) {
      var location = locations.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      var elem = doc.getElementById(location.Value);
      if (!elem) continue;
      var hasTriple = wLocationsDS.HasAssertion(location,
                                                resourceAccount,
                                                this.resource, true);
      if (elem.checked) {
        // add a triple if there isn't one already:
        if (!hasTriple)
          wLocationsDS.Assert(location, resourceAccount, this.resource, true);
      }
      else {
        // remove triple if there is one:
        if (hasTriple)
          wLocationsDS.Unassert(location, resourceAccount, this.resource);
      }
    }
  });

Account.spec(
  function fillDocument(doc) {
    this._Account_fillDocument(doc);
    // initialize locations template:
    var ltemplate = doc.getElementById("locations_template");
    if (!ltemplate) return;

    ltemplate.database.AddDataSource(wLocationsDS);
    ltemplate.database.AddDataSource(this.datasources["ephemeral"]);
    ltemplate.builder.rebuild();
  });

//----------------------------------------------------------------------
// ActiveAccount

var ActiveAccount = makeClass("ActiveAccount", Account, SupportsImpl);
ActiveAccount.addInterfaces(Components.interfaces.zapISipNonInviteRCListener,
                            Components.interfaces.nsITimerCallback);

// true if this account is currently successfully registered with a server:
ActiveAccount.rdfLiteralAttrib("urn:mozilla:zap:is_registered", "false", "global-ephemeral");
// true if this account has been activated:
ActiveAccount.rdfLiteralAttrib("urn:mozilla:zap:is_active", "false", "global-ephemeral");
// current registration interval as returned by the server
ActiveAccount.rdfLiteralAttrib("urn:mozilla:zap:registration_interval", "", "ephemeral");

ActiveAccount.fun(
  function activate() {
    this["urn:mozilla:zap:is_active"] = "true";
    // remember the credentials context, so that we can correctly
    // authenticate on deactivation when the location profile might
    // have changed:
    this.credentials = wCurrentLocation;

    if (this["urn:mozilla:zap:automatic_registration"] != "true")
      return;
    var server = wSipStack.syntaxFactory.deserializeURI(this["urn:mozilla:zap:registrar_server"]);
    this.aor = wSipStack.syntaxFactory.deserializeAddress(this["urn:mozilla:zap:address_of_record"]);
    var interval = this["urn:mozilla:zap:suggested_registration_interval"];
    this.registrationRC =
      wSipStack.createRegisterRequestClient(server, this.aor, interval);

    this.registrationRC.listener = this;
    this.registrationRC.sendRequest();
  });

ActiveAccount.fun(
  function deactivate() {
    this["urn:mozilla:zap:is_active"] = "false";
    if (this.registrationRC) {
      this.clearRefresh();
      this.deactivating = true;
      var contact = this.registrationRC.request.getTopContactHeader();
      if (contact) {
        contact = contact.QueryInterface(Components.interfaces.zapISipContactHeader);
        contact.setParameter("expires", "0");
        this.registrationRC.request.removeHeaders("Authorization");
        this.registrationRC.listener = this;
        try {
          this.registrationRC.sendRequest();
        } catch(e) {
          // the rc might still be busy with the last request.
          // XXX we should have a 'state' member on rc's
        }
      }
      delete this.registrationRC;
    }
  });

// zapISipNonInviteRCListener methods:
ActiveAccount.fun(
  function notifyResponseReceived(rc, dialog, response) {
    this._dump("response: "+response.statusCode);
    if (response.statusCode[0] == "1")
      return; // just a provisional response
    rc.listener = null;
    if ((response.statusCode == "401" ||
         response.statusCode == "407") &&
        wSipStack.authentication.
          addAuthorizationHeaders(this.credentials,
                                  response,
                                  rc.request)) {
      // retry with credentials:
      rc.listener = this;
      rc.sendRequest();
      return;
    }
    else if (response.statusCode[0] == "2") {
      // find our registration among the returned contact headers:
      var contacts = response.getHeaders("Contact", {});
      var contact;
      var myURI = rc.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader).address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
      for (var i=0,l=contacts.length; i<l; ++i) {
        var c = contacts[i].QueryInterface(Components.interfaces.zapISipContactHeader);
        try {
          var uri = c.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
          if (uri.equals(myURI)) {
            // we've found a match
            contact = c;
            break;
          }
          else {
            this._dump(myURI.serialize()+" != "+uri.serialize());
          }
        }
        catch(e) {
          // there was an error parsing the uri in the contact header. ignore
          this._dump("exception during registration response parsing: "+e);
        }
      }
      if (contact) {
        // looks like we're registered ok. let's see when the registration
        // expires (RFC3261 10.2.4):
        var expires = contact.getParameter("expires");
        if (!expires) {
          var expiresHeader = response.getTopHeader("Expires");
          if (expiresHeader) {
            expires = expiresHeader.QueryInterface(Components.interfaces.zapISipExpiresHeader).deltaSeconds;
          }
        }
        else {
          // the 'expires' uri parameter is a string. parse it:
          expires = parseInt(expires);
        }
        if (expires != null && !isNaN(expires) && expires!=0) {
          // we've got a valid registration!
          // update our state:
          this["urn:mozilla:zap:registration_interval"] = expires.toString();
          this["urn:mozilla:zap:is_registered"] = "true";
          // set up a refresh timer:
          this.scheduleRefresh(expires*1000*0.9);
          return;
        }
      }
    }
    // failure (or success if we are unregistering). update state:
    this["urn:mozilla:zap:is_registered"] = "false";
    if (!this.deactivating) {
      this._dump("Registration failure for "+this.aor.serialize()+". Retrying in 1 minute");
      // try again in a minute:
      this.scheduleRefresh(60000);
    }
  });

ActiveAccount.fun(
  function scheduleRefresh(delay_ms) {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
    }
    else {
      this.refreshTimer =
        Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    }
    this.refreshTimer.initWithCallback(this, delay_ms,
                                       Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  });

ActiveAccount.fun(
  function clearRefresh() {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
      delete this.refreshTimer;
    }
  });

// nsITimerCallback methods:
ActiveAccount.fun(
  function notify(timer) {
    this.registrationRC.listener = this;
    try {
      this.registrationRC.sendRequest();
    } catch(e) {
      // the rc might still be busy with the last request.
      // XXX we should have a 'state' member on rc's
    }
  });

//----------------------------------------------------------------------
// active account management

// hash of currently active accounts:
var wActiveAccounts = {};

// Get the Account object for the given resource. This will either be
// a new Account instance or a cached active account object:
function getAccount(resource) {
  var account = wActiveAccounts[resource.Value];
  if (!account) {
    account = Account.instantiate();
    account.initWithResource(resource);
  }
  return account;
}

// This will be called whenever the current location changes and we
// might need to add/remove accounts to/from wActiveAccounts:
function currentAccountsUpdated() {
  // walk through all accounts:
  var accounts = wAccountsContainer.GetElements();
  var resourceAccount = wRDF.GetResource("urn:mozilla:zap:account");
  while (accounts.hasMoreElements()) {
    var account = accounts.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
    // check if this account is active in the current location
    // profile:
    var active = wLocationsDS.HasAssertion(wCurrentLocation.resource,
                                           resourceAccount,
                                           account,
                                           true);
    if (active) {
      if (wActiveAccounts[account.Value]) {
        // Account is already active. Activate it again (route set or
        // contact might have changed:
        dump("Reactivating account "+account.Value+"\n");
        wActiveAccounts[account.Value].activate();
      }
      else {
        // activate account:
        dump("Activating account "+account.Value+"\n");
        var aa = ActiveAccount.instantiate();
        aa.initWithResource(account);
        wActiveAccounts[account.Value] = aa;
        aa.activate();
      }
    }
    else {
      var aa = wActiveAccounts[account.Value];
      if (aa) {
        dump("Deactivating account "+account.Value+"\n");
        aa.deactivate();
        delete wActiveAccounts[account.Value];
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Contacts:

function initContacts() {
  wContactsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wContactsContainer.Init(wContactsDS,
                          wRDF.GetResource("urn:mozilla:zap:contacts"));
  wFriendsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wFriendsContainer.Init(wContactsDS,
                         wRDF.GetResource("urn:mozilla:zap:friends"));
}

var Contact = makeClass("Contact", PersistentRDFObject);

Contact.prototype.datasources["default"] = wContactsDS;

Contact.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                          "urn:mozilla:zap:friends");
Contact.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                         "chrome://zap/content/contacts.xul");
Contact.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "contact");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip1", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip2", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:organization", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:email", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:homepage", "");
// if isfriend is true, the contact will be displayed in the sidebar:
Contact.rdfLiteralAttrib("urn:mozilla:zap:isfriend", "true");

Contact.spec(
  function createFromDocument(doc) {
    this._Contact_createFromDocument(doc);
    // append to contacts:
    wContactsContainer.AppendElement(this.resource);
    // ... and if it's a friend, to the friends list as well:
    if (this["urn:mozilla:zap:isfriend"] == "true")
      wFriendsContainer.AppendElement(this.resource);
  });

 Contact.spec(
   function updateFromDocument(doc) {
     this._Contact_updateFromDocument(doc);
     // add/remove from friends container:
     var index = wFriendsContainer.IndexOf(this.resource);
     if (this["urn:mozilla:zap:isfriend"] == "true") {
       if (index == -1)
         wFriendsContainer.AppendElement(this.resource);
     }
     else {
       if (index != -1)
         wFriendsContainer.RemoveElementAt(index, true);
     }
   });

////////////////////////////////////////////////////////////////////////
// Sidebar:

/*
  Sidebar RDF datasource:
                           zap:
                     sidebarparent _n
                            __     __
                           |  |   |  |
                           V  |   |  V
 +------------+         +---------------+    zap:     +----------+
 |  resource: |   _n    |   resource:   | chromepage  | literal: |
 | zap:sidebar|-------->|  sidebarnode  |------------>|   url    |
 +------------+         +---------------+             +----------+
                          |           |
                          | NC:Name   | zap:nodetype
                          V           V
                   +----------+  +----------+
                   | literal: |  | literal: |
                   |   name   |  |  type    |
                   +----------+  +----------+
*/

// initialize wSidebarDS and wSidebarTree:
function initSidebar() {
  wSidebarDS = wRDF.GetDataSourceBlocking(getProfileFileURL("sidebar.rdf"));
  wSidebarTree = document.getElementById("sidebar");
  wSidebarTree.database.AddDataSource(wSidebarDS);
  wSidebarTree.database.AddDataSource(wLocationsDS);
  wSidebarTree.database.AddDataSource(wAccountsDS);
  wSidebarTree.database.AddDataSource(wContactsDS);
  wSidebarTree.database.AddDataSource(wGlobalEphemeralDS);

  wSidebarTree.builder.rebuild();
  selectSidebarNode("urn:mozilla:zap:home");
}

function getSelectedSidebarResource()
{
  return getSelectedResource(wSidebarTree);
}

function getSidebarResourceAttrib(source, attrib) {
  var property = wRDF.GetResource(attrib);
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  return target;
}

function getSidebarResourceParent(source) {
  var property = wRDF.GetResource("urn:mozilla:zap:sidebarparent");
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFResource);
  return target;
}

function clearSidebarSelection()
{
  var selection = wSidebarTree.view.selection;
  if (selection)
    selection.clearSelection();
}

function ensureSidebarNode(uri) {
  var resource = wRDF.GetResource(uri);
  var index = wSidebarTree.builder.getIndexOfResource(resource);
  if (index == -1) {
    // The node isn't in the tree. Try recursively expanding ancestors:
    var parent = getSidebarResourceParent(resource);
    if (parent) {
      var parentIndex = ensureSidebarNode(parent.Value);
      wSidebarTree.builder.toggleOpenState(parentIndex);
      index = wSidebarTree.builder.getIndexOfResource(resource);
    }
  }
  return index;
}

function selectSidebarNode(uri)
{
  var index = ensureSidebarNode(uri);
  if (index < 0) return;
  wSidebarTree.view.selection.select(index);
  wSidebarTree.treeBoxObject.ensureRowIsVisible(index);
}

// called by the sidebar whenever its selection changes:
function sidebarSelectionChange()
{
  var page;
  var selection = getSelectedSidebarResource();
  if (!selection) return;

  clearCallsSelection();
  page = getSidebarResourceAttrib(selection, "urn:mozilla:zap:chromepage");
  if (!page) page = "about:blank";

  if (page != wInteractPane.getAttribute("src"))
    wInteractPane.setAttribute("src", page);
  else
    wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// MediaPipeline

var PB = makePropertyBag;

var wMediaPipeline = {
  init : function() {
    try {
      // We use getService() instead of createInstance() to get a globally
      // well-known media graph instance:
      this.mediagraph = Components.classes["@mozilla.org/zap/mediagraph;1"].getService(Components.interfaces.zapIMediaGraph);

      // 8000Hz, 20ms, 1 channel audio pipe:      
      this.audioin = this.mediagraph.addNode("audioin", null);
      this.asplitter = this.mediagraph.addNode("splitter", null);
      this.audioout = this.mediagraph.addNode("audioout", null);
      this.amixer = this.mediagraph.addNode("audio-mixer", null);
      this.A = this.mediagraph.connect(this.audioin, null,
                                       this.asplitter, null);
      this.B = this.mediagraph.connect(this.amixer, null,
                                       this.audioout, null);
      this.mediagraph.setAlias("ain", this.asplitter);
      this.mediagraph.setAlias("aout", this.amixer);
    }
    catch(e) {
      dump("Error during media pipeline initialization: "+e+"\n");
    }
  },
  
  cleanup: function() {
    try {
      this.mediagraph.shutdown();
    }
    catch(e) {}
    delete this.mediagraph;
  }
  
}

////////////////////////////////////////////////////////////////////////
// SipStack


function initSipStack() {
  wSdpService = Components.classes["@mozilla.org/zap/sdpservice;1"].
    getService(Components.interfaces.zapISdpService);
  
  wSipStack = Components.classes["@mozilla.org/zap/sipstack;1"].createInstance(Components.interfaces.zapISipUAStack);
  wSipStack.init(wUAHandler,
                 makePropertyBag({$port_base:5060,
                                  $methods: "OPTIONS,INVITE,ACK,CANCEL,BYE"}));
  document.getElementById("status_text").label = "Listening for UDP/TCP SIP traffic on port "+wSipStack.listeningPort;

  // set up logging to profile_dir/zap.sbox:
  wSipTrafficLogger.logOutputStream = openFileForWriting(getProfileFile("zap.sbox"),
                                                         true);
  wSipStack.transport.transceiver.trafficMonitor = wSipTrafficLogger;
}

function cleanupSipStack() {
  wSipTrafficLogger.logOutputStream.close();
  wSipTrafficLogger.logOutputStream = null;
}

// traffic logger for logging the raw sip traffic to profile_dir/zap.sbox
var wSipTrafficLogger = {
  logOutputStream : null,
  notifyPacket: function(data, protocol, localAddress, localPort,
                         remoteAddress, remotePort, outbound) {
    if (!this.logOutputStream) return;
    var entry = "- From "+localAddress+":"+localPort;
    if (outbound)
      entry += "-->";
    else
      entry += "<--";
    entry += remoteAddress+":" + remotePort + " " + protocol + " ";
    entry += data.length + "bytes " + (new Date()).toUTCString() + "\n";
    entry += data + "\n\n";

    this.logOutputStream.write(entry, entry.length);
  }
};


// our UA request handler:
var wUAHandler = {
  handleNonInviteRequest: function(rs) {
    return false;
  },

  handleInviteRequest: function(rs) {
    var ic = InboundCall.instantiate();
    ic.createNew(true);
    ic.receiveCall(rs);
    return true;
  }
};

////////////////////////////////////////////////////////////////////////
// Calls

// initialize wCallsDS, wCallsTree, wCallsContainer and
// wRecentCallsContainer:
function initCalls() {
  wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));
  wCallsTree = document.getElementById("calls");
  wCallsTree.database.AddDataSource(wCallsDS);
  wCallsTree.builder.rebuild();

  wCallsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wCallsContainer.Init(wCallsDS,
                              wRDF.GetResource("urn:mozilla:zap:calls"));
  wRecentCallsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wRecentCallsContainer.Init(wCallsDS,
                              wRDF.GetResource("urn:mozilla:zap:recent_calls"));

  // Check database for any active call zombies left over from
  // unclean shutdown:
  var recent = wRecentCallsContainer.GetElements();
  var resourceActive = wRDF.GetResource("urn:mozilla:zap:active");
  var resourceStatus = wRDF.GetResource("urn:mozilla:zap:status");
  while (recent.hasMoreElements()) {
    var resource = recent.getNext();
    var active = wCallsDS.GetTarget(resource, resourceActive, true).QueryInterface(Components.interfaces.nsIRDFLiteral);
    if (active.Value == "true") {
      // aha, a zombie!
      wCallsDS.Change(resource, resourceActive, active, wRDF.GetLiteral("false"), true);
      var old_status = wCallsDS.GetTarget(resource, resourceStatus, true);
      wCallsDS.Change(resource, resourceStatus, old_status, wRDF.GetLiteral("zombied"), true);
    }
  }
}

function clearCallsSelection() {
  var selection = wCallsTree.view.selection;
  if (selection)
    selection.clearSelection();
}

function selectCall(resource) {
  selectResource(wCallsTree, resource);
}

var wSuppressSelectionChangeNotifications = false;

function callsSelectionChange() {
  if (wSuppressSelectionChangeNotifications) return;
  
  var selection = getSelectedResource(wCallsTree);
  if (!selection) return;
  
  clearSidebarSelection();
  if (wInteractPane.getAttribute("src") != "chrome://zap/content/call.xul")
    wInteractPane.setAttribute("src", "chrome://zap/content/call.xul");
  else
    wInteractPane.webNavigation.reload(0);
}

function getSelectedCallResource()
{
  return getSelectedResource(wCallsTree);
}

// hash of active calls, indexed by resource name:
var wActiveCalls = {};

// get the Call object for the given resource. This will either be a
// new Call instance or the cached instance in wActiveCalls if the
// call is currently active:
function getCall(resource) {
  var call = wActiveCalls[resource.Value];
  if (!call) {
    call = Call.instantiate();
    call.initWithResource(resource);
  }
  return call;
}

//----------------------------------------------------------------------
// Call class

var Call = makeClass("Call", PersistentRDFObject);

Call.prototype.datasources["default"] = wCallsDS;
Call.addInMemoryDS("ephemeral");

// persistent attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:active", "true");
Call.rdfLiteralAttrib("urn:mozilla:zap:status", "");
Call.rdfLiteralAttrib("urn:mozilla:zap:remote", "");
Call.rdfLiteralAttrib("urn:mozilla:zap:subject", "");

// ephemeral attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:session-running", "false", "ephemeral");

// This is to provide an entrypoint for template recursion. see
// e.g. call.xul for usage and comments in
// RDFUtils.js::rdfPointerAttrib:
Call.rdfPointerAttrib("urn:mozilla:zap:root",
                      "urn:mozilla:zap:current-call",
                      "ephemeral");


// Add this call to the recent calls container (so that it displays
// in the call list). Maybe remove old items if the list is getting too large:
Call.fun(
  function addToRecentCalls() {
    wSuppressSelectionChangeNotifications = true;
    
    this._assert(this.resource, "can't add noninitialized call object");
    wRecentCallsContainer.InsertElementAt(this.resource, 1, true);
    // XXX renumber etc.

    // check if we need to remove items from the recent calls list:
    var last_call_index = wRecentCallsContainer.GetCount();
    var overflow = last_call_index - wPrefs["urn:mozilla:zap:max_recent_calls"];
    while (overflow > 0 && last_call_index >= 1) {
      this._dump("looking at "+last_call_index);
      var target = wCallsDS.GetTarget(wRDF.GetResource("urn:mozilla:zap:recent_calls"),
                                        wRDFContainerUtils.IndexToOrdinalResource(last_call_index), true);

      //this._assert(target, "recent calls ds broken at "+last_call_index);
      if (!target) {
        --last_call_index;
        continue;
      }
      
      // if this is an inactive call, remove it:
      var active = wCallsDS.GetTarget(target, wRDF.GetResource("urn:mozilla:zap:active"), true).QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      if (active == "true") {
        --last_call_index;
        continue;
      }
      
      wRecentCallsContainer.RemoveElementAt(last_call_index, true);
      --last_call_index;
      --overflow;
    }
    wSuppressSelectionChangeNotifications = false;
  });

//----------------------------------------------------------------------
// Active call

var ActiveCall = makeClass("ActiveCall", Call, SupportsImpl);
ActiveCall.addInterfaces(Components.interfaces.zapISipDialogListener);

// Mediasession and dialog will only be valid for our subclasses
// OutboundCall and InboundCall while a call is in progress:
ActiveCall.obj("mediasession", null);
ActiveCall.obj("dialog", null);

ActiveCall.spec(
  function createFromDocument(doc) {
    this._ActiveCall_createFromDocument(doc);
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);
    
    // ... and to recent calls as well:
    this.addToRecentCalls();

    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;
  });

ActiveCall.spec(
  function createNew(addAssertions) {
    this._ActiveCall_createNew(addAssertions);
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);

    // ... and to recent calls as well:
    this.addToRecentCalls();

    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;
  });

// clean up after call has terminated
ActiveCall.fun(
  function terminated() { 
    this["urn:mozilla:zap:active"] = "false";
    delete wActiveCalls[this.resource.Value];
    if (this.mediasession) {
      this.mediasession.shutdown();
      this.mediasession = null;
    }
    this.dialog = null;
    this.flush();
  });

// zapISipDialogListener methods:
ActiveCall.fun(
  function notifyDialogTerminated(dialog) {
    this["urn:mozilla:zap:status"] = "Terminated";
    this.terminated();
  });

//----------------------------------------------------------------------
// OutboundCall class

var OutboundCall = makeClass("OutboundCall", ActiveCall);

OutboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "outbound");

OutboundCall.fun(
  function makeCall(toAddress) {
    this.callHandler = OutboundCallHandler.instantiate();
    this.callHandler.call = this;

    var rc = wSipStack.createInviteRequestClient(toAddress);
    rc.listener = this.callHandler;
    this.callHandler.rc = rc;

    var subject = this["urn:mozilla:zap:subject"];
    if (subject) {
      try {
        var s = wSipStack.syntaxFactory.createHeader("Subject").QueryInterface(Components.interfaces.zapISipSubjectHeader);
        s.subject = subject;
        rc.request.appendHeader(s);
      }
      catch(e) {
        this._warning("Exception during Subject header construction: "+e);
      }
    }
    
    var offer = this.mediasession.generateSDPOffer();

    rc.request.setContent("application", "sdp", offer.serialize());
    rc.sendInvite(this.callHandler);
  });

//----------------------------------------------------------------------
// OutboundCallHandler
// CALLING -> 2XX_RECEIVED -> TERMINATED

var OutboundCallHandler = makeClass("OutboundCallHandler",
                                    SupportsImpl, StateMachine);
OutboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteResponseHandler,
                                  Components.interfaces.zapISipInviteRCListener);

// zapISipInviteResponseHandler methods:
OutboundCallHandler.fun(
  function handle2XXResponse(rc, dialog, response, ackTemplate) {
    if (this.call.dialog) {
      this._dump("received additional dialog for outbound call");
      // we already have a dialog. this must be a subsequent dialog
      // from a forking proxy. send a bye immediately:
      // XXX probably not a good idea to do this before sending the ack
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // try to parse answer:
    var sd;
    try {
      sd = wSdpService.deserializeSessionDescription(response.body);
    }
    catch(e) {
      this["urn:mozilla:zap:status"] = "Can't parse other party's session description";
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // we've got a parsed answer; process it:
    try {
      this.call.mediasession.processSDPAnswer(sd);
    }
    catch(e) {
      // XXX add verbose error
      this["urn:mozilla:zap:status"] = "Session negotiation failed";
      
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // finally. we've cleared all the hurdles:
    this.call.mediasession.startSession();
    this.call.dialog = dialog;
    this.call.dialog.listener = this.call;
    this.call["urn:mozilla:zap:session-running"] = "true";
    return ackTemplate;
  });

// zapISipInviteRCListener methods:
OutboundCallHandler.fun(
  function notifyResponseReceived(rc, dialog, response) {
    this._dump(response.statusCode+" response for call="+this.call);
    if (response.statusCode[0] == "1")
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
    else if (response.statusCode[0] == "2") {
      this.call["urn:mozilla:zap:status"] = "Connected";
      this.changeState("2XX_RECEIVED");
      // we'll terminate from notifyTerminated now
    }
    else if (response.statusCode == "401" ||
             response.statusCode == "407") {
      if (wSipStack.authentication.
          addAuthorizationHeaders(wCurrentLocation,
                                  response,
                                  rc.request)) {
        // we've got new credentials -> retry
        rc.sendInvite(this);
      }
    }
//     else if (response.statusCode == "301" ||
//              ...) {
//       // retry with new destination
//     }
    else {
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
      this.terminate();
    }
  });

OutboundCallHandler.statefun(
  "2XX_RECEIVED",
  function notifyTerminated(rc) {
    this._dump("terminated in 2XX_RECEIVED state. call="+this.call);
    this.terminate();
  });

OutboundCallHandler.statefun(
  "*",
  function notifyTerminated(rc) {
    this._dump("ignoring responsehandler termination.");
  });

OutboundCallHandler.fun(
  function terminate() {
    this.changeState("TERMINATED");
    if (!this.call.dialog) {
      this.call.terminated();
    }
    delete this.call.callHandler;
    delete this.call;
    delete this.rc;
  });

//----------------------------------------------------------------------
// InboundCall

var InboundCall = makeClass("InboundCall", ActiveCall);

InboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "inbound");

InboundCall.fun(
  function receiveCall(rs) {
    this["urn:mozilla:zap:remote"] = rs.request.getFromHeader().address.serialize();
    var subjectHeader = rs.request.getTopHeader("Subject");
    if (subjectHeader) {
      this["urn:mozilla:zap:subject"] = subjectHeader.QueryInterface(Compontents.interfaces.zapISipSubjectHeader).subject;
    }
    this.callHandler = InboundCallHandler.instantiate();
    this.callHandler.handleCall(this, rs);
  });

//----------------------------------------------------------------------
// InboundCallHandler

var InboundCallHandler = makeClass("InboundCallHandler", SupportsImpl);
InboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteRSListener);

InboundCallHandler.fun(
  function handleCall(call, rs) {
    this.call = call;
    this.rs = rs;
    rs.listener = this;
    // reject based on busy settings:
    if (document.getElementById("button_dnd").checked) {
      this._respond(wPrefs["urn:mozilla:zap:dnd_code"],
                    wPrefs["urn:mozilla:zap:dnd_headers"]);
      return;
    }

    // check if there is a media offer in the request:
    // XXXif (){}
    
    // parse offer; check if it's acceptable:
    try {
      var offer = wSdpService.deserializeSessionDescription(rs.request.body);
      this.call.mediasession = Components.classes["@mozilla.org/zap/mediasession;1"]
        .createInstance(Components.interfaces.zapIMediaSession);
      this.call.mediasession.init("zap",
                                  wSipStack.hostAddress,
                                  wSipStack.hostAddress);
      this.answer = this.call.mediasession.processSDPOffer(offer);
    }
    catch(e) {
      // Session negotiation failed.
      // Send 488 (Not acceptable) (RFC3261 13.3.1.3)
      // XXX set a Warning header
      this._respond("488");
      return;
    }

    // the offer is acceptable; we have an answer at hand. ring the user:
    this._respond("180");
  });

InboundCallHandler.fun(
  function acceptCall() {
    // formulate accepting response:
    var resp = this.rs.formulateResponse("200");
    // append session description:
    resp.setContent("application", "sdp", this.answer.serialize());
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    
    this.call.mediasession.startSession();
    this.call.dialog = this.rs.sendResponse(resp);
    this.call.dialog.listener = this.call;    
    this.call["urn:mozilla:zap:session-running"] = "true";
  });

InboundCallHandler.fun(
  function rejectCall() {
    this._respond("603");
  });

InboundCallHandler.fun(
  function _respond(code, extra_headers) {
    var resp = this.rs.formulateResponse(code);

    if (extra_headers) {
      // add in extra header, converting line endings appropriately:
      var responseTxt = resp.serialize();
      responseTxt = responseTxt.substring(0, responseTxt.length-2); // strip last CRLF
      responseTxt += extra_headers;
      responseTxt = responseTxt.replace(/\r/g, "");
      responseTxt = responseTxt.replace(/\n/g, "\r\n");
      responseTxt += "\r\n\r\n";
      resp = wSipStack.syntaxFactory.deserializeMessage(responseTxt).QueryInterface(Components.interfaces.zapISipResponse);
    }
    
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    return this.rs.sendResponse(resp);
  });
  
// zapISipInviteRSListener methods:
InboundCallHandler.fun(
  function notifyCancelled(rs) {
    this.call["urn:mozilla:zap:status"] = "Missed";
  });

InboundCallHandler.fun(
  function notifyACKReceived(rs, ack) {
    this.call["urn:mozilla:zap:status"] = "Confirmed";
  });

InboundCallHandler.fun(
  function notifyTerminated(rs) {
    if (!this.call.dialog) {
      this.call.terminated();
    }
    this.call.callHandler = null;
    this.call = null;
    this.rs = null;
  });

