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

var wSipStack;
var wSdpService;

////////////////////////////////////////////////////////////////////////
// Initialization:

function windowInit() {
  dump("Initializing zap main window...\n");

  wInteractPane = document.getElementById("interactpane");
  wURLField = document.getElementById("url_field");

  initPrefs();
  initAccounts();
  initContacts();
  initCalls();
  initSidebar();
  
  // init other chrome bits:
  var accountslist = document.getElementById("outbound_list_popup");
  accountslist.database.AddDataSource(wAccountsDS);
  accountslist.builder.rebuild();

  // initialize sip stack:
  initSipStack();
  
  dump("... Done initializing zap main window\n");
}

function windowClose() {
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
  loadPage("chrome://zap/content/make-call.xul", 1);
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

Prefs.rdfLiteralAttrib("urn:mozilla:zap:max_recent_calls", "10");
Prefs.rdfLiteralAttrib("urn:mozilla:zap:dnd_code", "480"); // Temporarily unavail.

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

Account.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                          "urn:mozilla:zap:accounts");
Account.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                         "chrome://zap/content/account.xul");
Account.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "account");
Account.rdfLiteralAttrib("urn:mozilla:zap:sip_server", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:outbound_proxy", "");
Account.rdfLiteralAttrib("urn:mozilla:zap:display_name", "");

Account.spec(
  function createFromDocument(doc) {
    this._Account_createFromDocument(doc);
    // append to accounts:
    wAccountsContainer.AppendElement(this.resource);
  });

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
    // ... and if its a friend, to the friends list as well:
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
  wSidebarTree.database.AddDataSource(wAccountsDS);
  wSidebarTree.database.AddDataSource(wContactsDS);

  wSidebarTree.builder.rebuild();
  selectSidebarNode("urn:mozilla:zap:accounts");
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
// SipStack


function initSipStack() {
  wSdpService = Components.classes["@mozilla.org/zap/sdpservice;1"].
    getService(Components.interfaces.zapISdpService);
  
  wSipStack = Components.classes["@mozilla.org/zap/sipstack;1"].createInstance(Components.interfaces.zapISipUAStack);
  wSipStack.init(wUAHandler,
                 makePropertyBag({$port_base:5060,
                                  $methods: "OPTIONS,INVITE,ACK,CANCEL,BYE"}));
  document.getElementById("status_text").label = "Listening for UDP/TCP SIP traffic on port "+wSipStack.listeningPort;
}

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
Call.rdfLiteralAttrib("urn:mozilla:zap:subject", "no subject");

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
    
    var offer = this.mediasession.generateSDPOffer();
    
    var request = rc.formulateInvite();
    request.setContent("application", "sdp", offer.serialize());
    rc.sendInvite(request, this.callHandler);
  });

//----------------------------------------------------------------------
// OutboundCallHandler

var OutboundCallHandler = makeClass("OutboundCallHandler", SupportsImpl);
OutboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteResponseHandler,
                                  Components.interfaces.zapISipInviteRCListener);

// zapISipInviteResponseHandler methods:
OutboundCallHandler.fun(
  function handle2XXResponse(rc, dialog, response, ackTemplate) {
    if (this.call.dialog) {
      // we already have a dialog. this must be a subsequent dialog
      // from a forking proxy. send a bye immediately:
      // XXX probably not a good idea to do this before sending the ack
      var rc = dialog.createNonInviteRequestClient();
      rc.sendRequest(rc.formulateRequest("BYE"));
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
      var rc = dialog.createNonInviteRequestClient();
      rc.sendRequest(rc.formulateRequest("BYE"));
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
      var rc = dialog.createNonInviteRequestClient();
      rc.sendRequest(rc.formulateRequest("BYE"));
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
    if (response.statusCode[0] == "1")
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
    else if (response.statusCode[0] == "2")
      this.call["urn:mozilla:zap:status"] = "Connected";
    else {
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
    }
  });

OutboundCallHandler.fun(
  function notifyTerminated(rc) {
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
      this._respond(wPrefs["urn:mozilla:zap:dnd_code"]);
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
  function _respond(code) {
    var resp = this.rs.formulateResponse(code);
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

