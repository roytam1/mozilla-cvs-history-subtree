/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.mozIJSComponentLib).probeComponent('rel:SipUAStack.js', true)" -*- */
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


debug("*** loading SipUAStack\n");

importModule("resource:/jscodelib/JSComponentUtils.js");
importModule("resource:/jscodelib/zap/ClassUtils.js");
importModule("resource:/jscodelib/zap/ArrayUtils.js");
importModule("resource:/jscodelib/zap/StringUtils.js");
importModule("resource:/jscodelib/zap/ObjectUtils.js");
importModule("resource:/jscodelib/zap/SipUtils.js");

// name our global object:
function toString() { return "[SipUAStack.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// globals

//----------------------------------------------------------------------
// resolver

var CLASS_RESOLVER = Components.classes["@mozilla.org/zap/sipresolver;1"];
var ITF_RESOLVER = Components.interfaces.zapISipResolver;

var gResolver = CLASS_RESOLVER.getService(ITF_RESOLVER);

//----------------------------------------------------------------------
// sip transport

var CLASS_SIP_TRANSPORT = Components.classes["@mozilla.org/zap/siptransport;1"];
var ITF_SIP_TRANSPORT = Components.interfaces.zapISipTransport;

function createSipTransport() {
  // create a sip transport object via xpcom (more overhead):
  // return CLASS_SIP_TRANSPORT.createInstance().QueryInterface(ITF_SIP_TRANSPORT);

  // create a sip transport object directly from js (less overhead, no
  // type safety, access to non-xpcom interface):
  return Components.classes["@mozilla.org/moz/jsloader;1"].getService(Components.interfaces.mozIJSComponentLib).probeComponent("rel:SipTransport.js", false).SipTransport.instantiate();
}

//----------------------------------------------------------------------
// sip transaction manager

var CLASS_SIP_TRANSACTION_MANAGER = Components.classes["@mozilla.org/zap/siptransactmgr"];
var ITF_SIP_TRANSACTION_MANAGER = Components.interfaces.zapISipTransactionManager;

function createSipTransactionManager() {
  // create a sip transaction manager via xpcom (more overhead):
  // return CLASS_SIP_TRANSACTION_MANAGER.createInstance().QueryInterface(ITF_SIP_TRANSACTION_MANAGER);

  // create a sip transaction manager directly from js (less overhead,
  // no type safety, access to non-xpcom interface):
  return Components.classes["@mozilla.org/moz/jsloader;1"].getService(Components.interfaces.mozIJSComponentLib).probeComponent("rel:SipTransactions.js", false).SipTransactionManager.instantiate();
}

//----------------------------------------------------------------------
// Helpers

// Construct a dialog ID for a UAC (rfc3261 12)
function constructClientDialogID(message) {
  var callID = message.getCallIDHeader().callID;
  var localtag = message.getFromHeader().getParameter("tag");
  var remotetag = message.getToHeader().getParameter("tag");
  return callID+";"+localtag+";"+remotetag;
}

// Construct a dialog ID for a UAS (rfc3261 12)
function constructServerDialogID(message) {
  var callID = message.getCallIDHeader().callID;
  var localtag = message.getToHeader().getParameter("tag");
  var remotetag = message.getFromHeader().getParameter("tag");
  return callID+";"+localtag+";"+remotetag;
}

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP UA", level, mes);
}



////////////////////////////////////////////////////////////////////////
// SipDialog

var SipDialog = makeClass("SipDialog", SupportsImpl);
SipDialog.addInterfaces(Components.interfaces.zapISipDialog);

SipDialog.getter(
  "_name_",
  function get_name_() {
    return "Dialog "+this.id;
  });

SipDialog.fun(
  function _initState(stack, id, secure, routeSet, localSeq, remoteSeq,
                      callID, localTag, remoteTag, localAddress,
                      remoteAddress, remoteTarget) {
    this.stack = stack;
    this.id = id;
    this.secure = secure;
    this.routeSet = routeSet;
    this.localSeq = localSeq;
    this.remoteSeq = remoteSeq;
    this.callID = callID;
    this.localTag = localTag;
    this.remoteTag = remoteTag;
    this.localAddress = localAddress;
    this.remoteAddress = remoteAddress;
    this.remoteTarget = remoteTarget;
    stack.registerDialog(this);
  });

SipDialog.obj("state", null);

SipDialog.fun(
  function changeState(newState) {
    this.state = newState;
    if (newState.enterState)
      newState.enterState(this);
  });

SipDialog.fun(
  "Transition an 'Early' dialog to a confirmed state",
  function confirm(response) {
    // XXX recompute route set
    this.changeState(this.Confirmed);
    // send ACK
  });

SipDialog.fun(
  function formulateGenericRequest(method) {
    // formulate a generic request within a dialog (rfc3261 12.2.1.1):
    var m = gSyntaxFactory.createRequest();
    // method:
    m.method = method;
    // Request uri & route set:
    // XXX 
    m.requestURI = this.remoteTarget.uri;
    // To header + tag:
    var toHeader = gSyntaxFactory.createToHeader(this.remoteAddress);
    if (this.remoteTag)
      toHeader.setParameter("tag", this.remoteTag);
    m.appendHeader(toHeader);
    // From header + tag:
    var fromHeader = gSyntaxFactory.createFromHeader(this.localAddress);
    if (this.localTag)
      fromHeader.setParameter("tag", this.localTag);
    m.appendHeader(fromHeader);
    // Call-ID header:
    m.appendHeader(gSyntaxFactory.createCallIDHeader(this.callID));
    // CSeq header:
    // NOTE: CSeq sequenceNumber must be updated on sending!
    m.appendHeader(gSyntaxFactory.createCSeqHeader(method, 1));
    // Max-Forwards header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());    
    // New Via header:
    m.appendHeader(gSyntaxFactory.createViaHeader());

    return m;
  });

SipDialog.fun(
  function sendNonInviteRequest(r, TU) {
    // send a non-invite request within a dialog (rfc3261 12.2.1.1)
    var me = this;
    r.getCSeqHeader().sequenceNumber = ++this.localSeq;

    var listener = {
      resolveComplete : function(destinations, count) {
        me._assert(destinations.length > 0, "no destinations for request!");
        // just send directly to the first destination:
        // but first give the top Via a new branch id:
        r.getTopViaHeader().setParameter("branch", BRANCH_COOKIE+generateUUID());
        me.stack.transactionManager.executeNonInviteClientTransaction(r,
                                                                      destinations[0],
                                                                      TU);
      }
    };
    this.stack.resolver.resolveEndpointsAsync(this.remoteTarget.uri,
                                              /*this.routeSet,*/
                                              listener);
  });

SipDialog.fun(
  function bye() {
    var r = this.formulateGenericRequest("BYE");
    this.sendNonInviteRequest(r, null);
    this.changeState(this.Terminated);
  });

SipDialog.fun(
  function terminate() {
    this.changeState(this.Terminated);
  });

//----------------------------------------------------------------------
// zapISipDialog implementation:

// void setDialogHandler(in zapISipDialogHandler handler)
SipDialog.fun(
  function setDialogHandler(handler) {
    this.handler = handler;
  });

//----------------------------------------------------------------------
// 'Early' state

SipDialog.obj(
  "Early",
  {
    enterState : function enterState_Early(d) {
    }
  });

//----------------------------------------------------------------------
// 'Confirmed' state

SipDialog.obj(
  "Confirmed",
  {
    enterState : function enterState_Confirmed(d) {
      if (d.handler)
        d.handler.dialogConfirmed(d);
    }
  });
  
//----------------------------------------------------------------------
// 'Terminated' state

SipDialog.obj(
  "Terminated",
  {
    enterState : function enterState_Terminated(d) {
      if (d.handler) {
        d.handler.dialogTerminated(d);
        d.handler = null;
      }
      d.stack.unregisterDialog(d);
    }
  });


////////////////////////////////////////////////////////////////////////
// SipInviteClientDialog : a dialog started by an outgoing 'INVITE'

var SipInviteClientDialog = makeClass("SipInviteClientDialog",
                                      SipDialog);
// SipInviteClientDialog.addInterfaces(...

SipInviteClientDialog.fun(
  "Initialize the dialog state at the UAC end.",
  function init(stack, request, response) {
    this.initialRequest = request;
    
    // Construct dialog state (rfc3261 12.1.2):
    this._initState(stack,
                    constructClientDialogID(response),
                    false, // secure XXX
                    [], // route set XXX
                    request.getCSeqHeader().sequenceNumber,
                    null, // remote cseq
                    request.getCallIDHeader().callID,
                    request.getFromHeader().getParameter("tag"),
                    response.getToHeader().getParameter("tag"),
                    request.getFromHeader().address,
                    request.getToHeader().address,
                    response.getTopContactHeader().address);

    this.changeState(response.statusCode[0]=='1' ? this.Early : this.Confirmed);
  });

SipInviteClientDialog.fun(
  function handleResponse(response) {
    this._dump("RESPONSE "+response.statusCode);
    if (response.statusCode[0] == '2') {
      // This must be a retransmission of the response to our initial
      // invite. Quench further responses by sending an 'ACK' (rfc3261 13.2.2.4):
      this.sendACK();
    }
    else {
      this._dump("Stray response "+response.statusCode+" ("+response.reasonPhrase+"). Dropping.");
    }
  });

SipInviteClientDialog.fun(
  function handleRequest(request) {
    // XXX implement 12.2.2 procedures
    
    this._dump("REQUEST "+request.method);
    if (request.method == "BYE") {
      var m = this.stack.formulateResponse("200", "OK", request, false);
      this.stack.transactionManager.executeNonInviteServerTransaction(request,
                                                                      null,
                                                                      m);
      this.changeState(this.Terminated);      
    }
  });

SipInviteClientDialog.fun(
  function sendACK() {
    // Send an ACK for a previous 2xx response (rfc3261 13.2.2.4)
    var me = this;
    var ack = this.formulateGenericRequest("ACK");
    ack.getCSeqHeader().sequenceNumber = this.initialRequest.getCSeqHeader().sequenceNumber;
    // XXX copy authentication headers from initial request
    
    var listener = {
      resolveComplete : function(destinations, count) {
        me._assert(destinations.length > 0, "no destinations for ack!");
        // just send directly to the first destination:
        // but first give the top Via a new branch id:
        ack.getTopViaHeader().setParameter("branch", BRANCH_COOKIE+generateUUID());
        me.stack.transport.sendRequest(ack, destinations[0], null);
      }
    };
    this.stack.resolver.resolveEndpointsAsync(this.remoteTarget.uri,
                                              /*this.routeSet,*/
                                              listener);
  });

//----------------------------------------------------------------------
// Specialized 'Confirmed' state

SipInviteClientDialog.obj(
  "Confirmed",
  {
    enterState : function enterState_InviteClient_Confirmed(d) {
      d.sendACK();
      if (d.handler)
        d.handler.dialogConfirmed(d);
    }
  });


////////////////////////////////////////////////////////////////////////
// SipInviteServerDialog : a dialog started by an incoming 'INVITE'

var SipInviteServerDialog = makeClass("SipInviteServerDialog",
                                      SipDialog);
// SipInviteServerDialog.addInterfaces(...

SipInviteServerDialog.fun(
  "Initialize the dialog state at the UAS end.",
  function init(stack, request, response) {
    this.initialRequest = request;
    
    // Construct dialog state (rfc3261 12.1.1):
    this._initState(stack,
                    constructServerDialogID(response),
                    false, // secure XXX
                    [], // route set XXX
                    null, // local cseq
                    request.getCSeqHeader().sequenceNumber, // remote cseq
                    request.getCallIDHeader().callID,
                    request.getToHeader().getParameter("tag"),
                    response.getFromHeader().getParameter("tag"),
                    request.getToHeader().address,
                    request.getFromHeader().address,
                    request.getTopContactHeader().address);

    this.changeState(response.statusCode[0]=='1' ? this.Early : this.Confirmed);
  });


SipInviteServerDialog.fun(
  function handleResponse(response) {
    this._dump("Stray response "+response.statusCode+" ("+response.reasonPhrase+"). Dropping.");
  });

SipInviteServerDialog.fun(
  function handleRequest(request) {
    // XXX implement 12.2.2 procedures
    
    this._dump("REQUEST "+request.method);
    if (request.method == "ACK") {
      //XXX implement automatic closing of dialog if ACK doesn't arrive
    }
    else if (request.method == "BYE") {
      var m = this.stack.formulateResponse("200", "OK", request, false);
      this.stack.transactionManager.executeNonInviteServerTransaction(request,
                                                                      null,
                                                                      m);
      this.changeState(this.Terminated);      
    }
  });



////////////////////////////////////////////////////////////////////////
// SipUAStack

var SipUAStack = makeClass("SipUAStack", SupportsImpl);
SipUAStack.addInterfaces(Components.interfaces.zapISipUAStack,
                       Components.interfaces.zapISipTransportSink);

SipUAStack.appendCtor(
  function ctor() {
    // Pool of active dialogs, indexed by dialog id:
    this._dialogPool = {};    
  });

//----------------------------------------------------------------------
// zapISipUAStack

// void init(in zapISipMSHFactory mshFactory)
SipUAStack.fun(
  function init(mshFactory) {
    this._dump("Building SIP User Agent Stack...");

    this.mshFactory = mshFactory;
    
    // Create transport layer:
    this.transport = createSipTransport();
    
    // Create transaction manager:
    this.transactionManager = createSipTransactionManager();

    // Hook up transport to transactionmanager:
    this.transactionManager.init(this.transport);
    
    // Add the transaction manager as the first transport sink so that
    // it gets first refusal at handling new messages:
    this.transport.appendTransportSink(this.transactionManager);    
    
    // Add ourselves as 'last-stop' transport sink:
    this.transport.appendTransportSink(this);

    this._dump("SIP User Agent Stack built.");    
  });


// void shutdown()
SipUAStack.fun(
  function shutdown() {
    this.transport.shutdown();
  });

// readonly attribute zapISipTransactionManager transactionManager
SipUAStack.obj("transactionManager", null);

// readonly attribute zapISipSyntaxFactory syntaxFactory
SipUAStack.obj("syntaxFactory", gSyntaxFactory);

// readonly attribute zapISipResolver resolver
SipUAStack.obj("resolver", gResolver);

// zapISipRequest formulateGenericRequest(in ACString method,
//                                        in zapISipURI requestURI,
//                                        in zapISipAddress ToAddress,
//                                        in zapISipAddress FromAddress);
SipUAStack.fun(
  function formulateGenericRequest(method, requestURI, ToAddress,
                                   FromAddress) {
    // XXX handle RouteSet
    
    var m = gSyntaxFactory.createRequest();    
    // method:
    m.method = method;
    // request uri:
    m.requestURI = requestURI;
    // To header:
    m.appendHeader(gSyntaxFactory.createToHeader(ToAddress));
    // From header + new tag:
    var fromHeader = gSyntaxFactory.createFromHeader(FromAddress);
    fromHeader.setParameter("tag", generateTag());
    m.appendHeader(fromHeader);
    // Call-ID header with new call id:
    var callIDHeader =
      gSyntaxFactory.createCallIDHeader(generateUUID()+
                                        "@"+ this.hostName);
    m.appendHeader(callIDHeader);
    
    // CSeq header matching the request method and with new sequence
    // number:
    m.appendHeader(gSyntaxFactory.createCSeqHeader(method, 1));
    
    // Max-Forwards header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());
    
    // New Via header:
    m.appendHeader(gSyntaxFactory.createViaHeader());
    
    return m;
  });

// zapISipRequest formulateInviteRequest(in zapISipAddress ToAddress,
//                                       in zapISipAddress FromAddress);
SipUAStack.fun(
  function formulateInviteRequest(ToAddress, FromAddress) {
    var message = this.formulateGenericRequest("INVITE", ToAddress.uri, ToAddress,
                                               FromAddress);
    // Set mandatory Contact header (rfc3261 8.1.1.8):
    // XXX should this be user-configurable?
    // for the time being just use user name of FromAddress and ip address
    // of local host:
    //var contactAddr = FromAddress.clone().QueryInterface(Components.interfaces.zapISipAddress);
    //contactAddr.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
    //contactAddr.uri.host = this.hostAddress;
    //contactAddr.uri.port = "";
    //message.appendHeader(gSyntaxFactory.createContactHeader(contactAddr));

    // XXX use FromAddress for the time being:
    message.appendHeader(gSyntaxFactory.createContactHeader(FromAddress));
    
    return message;
  });

// zapISipResponse formulateResponse(in ACString statusCode,
//                                   in AUTF8String reasonPhrase,
//                                   in zapISipRequest request,
//                                   in boolean generateToTag);
SipUAStack.fun(
  function formulateResponse(statusCode, reasonPhrase,
                             request, generateToTag) {
    var m = gSyntaxFactory.createResponse();
    m.statusCode = statusCode;
    m.reasonPhrase = reasonPhrase;
    
    m.appendHeader(request.getFromHeader());
    m.appendHeader(request.getCallIDHeader());
    m.appendHeader(request.getCSeqHeader());
    // copy Via headers:
    amap(function(v) { m.appendHeader(v); },
         request.getHeaders("Via", {}));
    var toHeader = request.getToHeader().clone().QueryInterface(Components.interfaces.zapISipToHeader);
    m.appendHeader(toHeader);
    if (generateToTag && !toHeader.getParameter("tag")) {
      toHeader.setParameter("tag", generateTag());
    }
    return m;
  });

// void createInviteMC(in zapISipRequest inviteRequest);
SipUAStack.fun(
  function createInviteMC(request, methodHandler) {
    var client = InviteUACCore.instantiate();
    client.init(this, request, methodHandler);
    return client;
  });

// readonly attribute ACString hostName
SipUAStack.getter(
  "hostName",
  function get_hostName() {
    return gDNSService.myHostName;
  });

// readonly attribute ACString hostAddress
SipUAStack.getter(
  "hostAddress",
  function get_hostAddress() {
    return gDNSService.resolve(gDNSService.myHostName,0).getNextAddrAsString();
  });


//----------------------------------------------------------------------
// zapISipTransportSink implementation:

SipUAStack.fun(
  function handleSipMessage(message, endpoint, connection) {
    if (message.isRequest) {
      message.QueryInterface(Components.interfaces.zapISipRequest);
      log(message.method+" request received");
      //check if we should route request to a dialog:
      var id = constructServerDialogID(message);
      var dialog = this.findDialog(id);
      if (dialog)
        dialog.handleRequest(message);
      else {
        // process request according to RFC3261 8.2
        switch (message.method) {
          case "INVITE":
            // create a new InviteMSH/InviteUASCore to handle the INVITE:
            var msh = this.mshFactory.createInviteMSH();
            var ms = InviteUASCore.instantiate();
            ms.init(this, message, msh);
            ms.execute();
            break;
          default:
            // generate 405 (Method Not Allowed)
            // XXX only do this for recognised methods??
            var response = this.formulateResponse("405", "Method Not Allowed",
                                                  message, true);
            this.transactionManager.executeNonInviteServerTransaction(message,
                                                                      null,
                                                                      response);
            break;
        }
      }
    }
    else {
      message.QueryInterface(Components.interfaces.zapISipResponse);
      // check if we should route response to a dialog:
      var id = constructClientDialogID(message);
      var dialog = this.findDialog(id);
      if (dialog)
        dialog.handleResponse(message);
      else
        this._dump("Stray response ("+message.statusCode+" --- "+
                   message.reasonPhrase+")");
    }
    return true;
  });

//----------------------------------------------------------------------
// Dialog management:

SipUAStack.fun(
  function registerDialog(dialog) {
    hashset(this._dialogPool, dialog.id, dialog);
  });

SipUAStack.fun(
  function unregisterDialog(dialog) {
    hashdel(this._dialogPool, dialog.id);
  });

SipUAStack.fun(
  function findDialog(id) {
    return hashget(this._dialogPool, id);
  });


////////////////////////////////////////////////////////////////////////
// InviteUACCore : UAC INVITE handler 

// XXX recode this as a state machine with state RESOLVING, INVITING, ...

var InviteUACCore = makeClass("InviteUACCore", SupportsImpl);
InviteUACCore.addInterfaces(Components.interfaces.zapISipClientTransactionUser,
                            Components.interfaces.zapISipInviteMC,
                            Components.interfaces.zapISipResolveListener);

// Array of dialogs established through this transaction:
InviteUACCore.obj("_dialogs", null);  
  
// Array of possible destinations for the request:
InviteUACCore.obj("_destinations", []);

// Current destination (index into destinations):
InviteUACCore.obj("_destinationIndex", -1);

// Request object:
InviteUACCore.obj("_request", null);

// UAStack object:
InviteUACCore.obj("_stack", null);

// zapISipInviteMCH peer:
InviteUACCore.obj("_methodHandler", null);

InviteUACCore.fun(
  function init(stack, request, methodHandler) {
    this._dialogs = [];
    this._stack = stack;
    this._request = request;
    this._methodHandler = methodHandler;
  });

//----------------------------------------------------------------------
// zapISipInviteMC implementation:

InviteUACCore.fun(
  function execute() {
    // This will asynchronously resolve the possible destinations for
    // our request and call 'resolveComplete' once finished:
    this._stack.resolver.resolveEndpointsAsync(this._request.requestURI,
                                               /*this._request.getHeaders("Route", {}),*/
                                               this);
  });

//----------------------------------------------------------------------
// zapISipResolveListener implementation:

InviteUACCore.fun(
  function resolveComplete(destinations, count) {
    // We've got destinations to try the request on now.
    this._destinations = destinations;
    this._tryNextDestination();
  });

//----------------------------------------------------------------------
// zapISipClientTransactionUser implementation:

InviteUACCore.fun(
  function handleFailureResponse(r) {
    if (this._methodHandler)
      this._methodHandler.failureResponse(this, r);
    this._dump(r.statusCode);
    this._terminateEarlyDialogs();    
    this._tryNextDestination();
  });

InviteUACCore.fun(
  function handleProvisionalResponse(r) {
    if (this._methodHandler)
      this._methodHandler.provisionalResponse(this, r);
    this._dump(r.statusCode);
    // rfc3261 12.1: only provisional 101-199 responses with a 'To'
    // tag create early dialogs
    if (r.statusCode>=101 && r.statusCode<199 && r.getToHeader().getParameter("tag")) {
      if (!this._stack.findDialog(constructClientDialogID(r)))
        this._createNewDialog(r);
    }
  });

InviteUACCore.fun(
  function handleSuccessResponse(r) {
    if (this._methodHandler)
      this._methodHandler.successResponse(this, r);
    this._dump(r.statusCode);
    var dialog = this._stack.findDialog(constructClientDialogID(r));
    if (dialog) {
      this._assert(dialog.state == dialog.Early, "Dialog not in Early state??");
    } else {
      dialog = this._createNewDialog(r);
    }
    dialog.confirm(r);
    this._terminateEarlyDialogs();
    
    if (this._methodHandler)
      this._methodHandler.success(this, dialog);
  });

InviteUACCore.fun(
  function handleTimeout() {
    this._terminateEarlyDialogs();
    // XXX notify UI
    this._tryNextDestination();
  });


//----------------------------------------------------------------------

InviteUACCore.fun(
  function _tryNextDestination() {
    if (++this._destinationIndex >= this._destinations.length) {
      this._dump("no more destinations to try");
      if (this._methodHandler)
        this._methodHandler.failure(this);
      return;
    }
    this._sendRequest(this._destinations[this._destinationIndex]);
  });

InviteUACCore.fun(
  function _sendRequest(dest) {
    if (this._methodHandler)
      this._methodHandler.calling(this, dest);
    // Give the top Via a new branch id:
    this._request.getTopViaHeader().setParameter("branch", BRANCH_COOKIE+generateUUID());
    this._stack.transactionManager.executeInviteClientTransaction(this._request,
                                                                  dest,
                                                                  this);
  });

InviteUACCore.fun(
  function _createNewDialog(response) {
    var dialog = SipInviteClientDialog.instantiate();
    dialog.init(this._stack, this._request, response);
    this._dialogs.push(dialog);
    log("spawning new dialog");
    if (this._methodHandler)
      this._methodHandler.dialogSpawned(this, dialog);
    return dialog;
  });

InviteUACCore.fun(
  function _terminateEarlyDialogs() {
    amap(function(d) { if (d.state == d.Early) d.terminate(); }, this._dialogs);
    this._dialogs = [];
  });


////////////////////////////////////////////////////////////////////////
// InviteUASCore : UAS INVITE handler

// XXX recode as a state machine

var InviteUASCore = makeClass("InviteUASCore", SupportsImpl);
InviteUASCore.addInterfaces(Components.interfaces.zapISipServerTransactionUser,
                            Components.interfaces.zapISipInviteMS);

// Request object:
InviteUACCore.obj("_request", null);

// UAStack object:
InviteUACCore.obj("_stack", null);

// zapISipInviteMSH peer:
InviteUACCore.obj("_methodHandler", null);

InviteUASCore.fun(
  function init(stack, request, methodHandler) {
    this._stack = stack;
    this._request = request;
    this._methodHandler = methodHandler;
  });

InviteUASCore.fun(
  function execute() {
    // create a new invite server transaction:
    this._transaction = this._stack.transactionManager.executeInviteServerTransaction(this._request, this, true);
    this._methodHandler.invite(this, this._request);
  });

InviteUASCore.fun(
  function _createNewDialog(response) {
    this.dialog = SipInviteServerDialog.instantiate();
    this.dialog.init(this._stack, this._request, response);
    log("spawning new dialog");
    if (this._methodHandler)
      this._methodHandler.dialogSpawned(this, this.dialog);
    return this.dialog;
  });

//----------------------------------------------------------------------
// zapISipServerTransactionUser implementation:

//  void handleTimeout();
InviteUASCore.fun(
  function handleTimeout() {
    this._dump("TIMEOUT!");
  });

//  void transactionTerminated(in zapISipServerTransaction transaction);
InviteUASCore.fun(
  function transactionTerminated(transaction) {
    this._dump("TERMINATED!");
  });

//----------------------------------------------------------------------
// zapISipInviteMS implementation:

// void sendResponse(in zapISipResponse response);
InviteUASCore.fun(
  function sendResponse(response) {
    if (response.statusCode[0] == "2") {
      // construct a new dialog:
      this._createNewDialog(response);
      this._transaction.sendResponse(response);
      if (this._methodHandler)
        this._methodHandler.success(this, this.dialog);
    }
    else if (response.statusCode[0] != "1") {
      this._transaction.sendResponse(response);
      if (this._methodHandler)
        this._methodHandler.failure(this);
    }
    else { // 1xx
    // XXX construct early dialog for 101-199
      this._transaction.sendResponse(response);
    }
  });

// zapISipResponse formulate200Response();
InviteUASCore.fun(
  function formulate200Response() {
    // construct a dialog-establishing response according to RFC3261
    // 12.1.1:
    var m = this._stack.formulateResponse("200", "OK", this._request, true);
    // copy record-route headers:
    amap(function(v) { m.appendHeader(v); },
         this._request.getHeaders("Record-Route", {}));
    // set contact header:
    // XXX use ToAddress of request for now:
    var ToAddress = this._request.getToHeader().address;
    m.appendHeader(gSyntaxFactory.createContactHeader(ToAddress));
    return m;
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP SIP User Agent Stack",
     cid        : Components.ID("{7377a41d-7bee-4729-8803-0b32218c1a9e}"),
     contractID : "@mozilla.org/zap/sipstack;1",
     factory    : ComponentUtils.generateFactory(function() { return SipUAStack.instantiate(); })
  }]);

