/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('resource:/jscodelib/zap/SipUARequestCore.js', null)" -*- */
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


/**
 * SipUARequestCore contains the UA request server and client code.
 */

debug("*** loading SipUARequestCore\n");

EXPORTED_SYMBOLS = [ "SipNonInviteRC",
                     "SipInviteRC",
                     "SipNonInviteRS",
                     "SipInviteRS"];

Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/SipUtils.js");



// name our global object:
function toString() { return "[SipUARequestCore.js]"; }

// object to hold component's documentation:
var _doc_ = {};


////////////////////////////////////////////////////////////////////////
// SipNonInviteRC
// request client for sending non-invite requests
// INITIALIZED --> RESOLVING --> CALLING --> TERMINATED

var SipNonInviteRC = makeClass("SipNonInviteRC", SupportsImpl, StateMachine);
SipNonInviteRC.addInterfaces(Components.interfaces.zapISipNonInviteRC,
                             Components.interfaces.zapISipResolveListener,
                             Components.interfaces.zapISipClientTransactionUser);

SipNonInviteRC.fun(
  function init(stack, dialog, ToAddress) {
    this.stack = stack;
    this.dialog = dialog;
    this.ToAddress = ToAddress;
    this.changeState("INITIALIZED");
  });

//----------------------------------------------------------------------
// zapISipNonInviteRC

//  attribute zapISipNonInviteRCListener listener;
SipNonInviteRC.obj("listener", null);

//  readonly attribute zapISipAddress ToAddress;
SipNonInviteRC.obj("ToAddress", null);

//  readonly attribute zapISipDialog dialog;
SipNonInviteRC.obj("dialog", null);

//  zapISipRequest formulateRequest(in ACString method);
SipNonInviteRC.statefun(
  "INITIALIZED",
  function formulateRequest(method) {
    var m;
    if (this.dialog)
      m = this.dialog.formulateGenericRequest(method);
    else
      m = this.stack.formulateGenericRequest(method, this.ToAddress);

    return m;
  });

//  void sendRequest(in zapISipRequest request);
SipNonInviteRC.statefun(
  "INITIALIZED",
  function sendRequest(request) {
    this.request = request;
    // Asynchronously resolve possible destinations for the request:
    this.changeState("RESOLVING");
    this.stack.resolver.resolveEndpointsAsync(request.requestURI,
                                              /*request.getHeaders("Route", {}), */
                                              this);
  });

//----------------------------------------------------------------------
// zapISipResolveListener

//  void resolveComplete([array, size_is(count)] in zapISipEndpoint endpoints,
//                       in unsigned long count);
SipNonInviteRC.statefun(
  "RESOLVING",
  function resolveComplete(destinations, count) {
    this._destinationIterator = makeArrayIterator(destinations);
    this.changeState("CALLING");
    this.tryNextDestination();
  });

SipNonInviteRC.statefun(
  "CALLING",
  function tryNextDestination() {
    if (!this._destinationIterator.hasMore()) {
      if (this.listener) {
        // generate a 408 (Request Timeout) to hand to the listener
        // (RFC3261 8.1.3.1):
        var r = this.stack.formulateResponse("408", this.request, this._ToTag);
        this.listener.notifyResponseReceived(this, this.dialog, r);
      }
      return this.terminate();
    }
    // send request to next destination:
    this.request.getTopViaHeader().setParameter("branch",
                                                 BRANCH_COOKIE+generateUUID());
    if (this.dialog) {
      // we're sending a request within a dialog.
      // update local sequence number
      if (this.dialog.localSequenceNumber ==
          Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER)
        this.dialog.localSequenceNumber = 0;
      this.request.getCSeqHeader().sequenceNumber =
        ++this.dialog.localSequenceNumber;
    }
    this.stack.transactionManager.executeNonInviteClientTransaction(this.request,
                                                                     this._destinationIterator.getNext(),
                                                                     this);
  });

//----------------------------------------------------------------------
// zapISipClientTransactionUser

//  void handleFailureResponse(in zapISipResponse response);
SipNonInviteRC.statefun(
  "CALLING",
  function handleFailureResponse(response) {
    if (this.listener)
      this.listener.notifyResponseReceived(this, this.dialog, response);
    this.terminate();
  });

//  void handleProvisionalResponse(in zapISipResponse response);
SipNonInviteRC.statefun(
  "CALLING",
  function handleProvisionalResponse(response) {
    if (this.listener)
      this.listener.notifyResponseReceived(this, this.dialog, response);
    // stay in CALLING state
  });

//  void handleSuccessResponse(in zapISipResponse response);
SipNonInviteRC.statefun(
  "CALLING",
  function handleSuccessResponse(response) {
    if (this.listener)
      this.listener.notifyResponseReceived(this, this.dialog, response);
    this.terminate();
  });

//  void handleTimeout();
SipNonInviteRC.statefun(
  "CALLING",
  function handleTimeout() {
    this.tryNextDestination();
  });

//----------------------------------------------------------------------
// Implementation helpers

SipNonInviteRC.fun(
  function terminate() {
    // make sure we terminate our associated dialog if this is a BYE:
    if (this.dialog &&
        this.request.method == "BYE" &&
        this.dialog.dialogState != "TERMINATED") {
      this.dialog.terminate();
    }

    this.changeState("TERMINATED");

    if (this.listener) {
      this.listener.notifyTerminated(this);
      this.listener = null;
    }
  });

////////////////////////////////////////////////////////////////////////
// SipInviteRC
// request client for sending invite requests
// INITIALIZED --> RESOLVING --> CALLING --> 2XX_RECEIVED --> TERMINATED

var SipInviteRC = makeClass("SipInviteRC", SupportsImpl, StateMachine);
SipInviteRC.addInterfaces(Components.interfaces.zapISipInviteRC,
                          Components.interfaces.zapISipResolveListener,
                          Components.interfaces.zapISipClientTransactionUser);

SipInviteRC.fun(
  function init(stack, dialog, ToAddress) {
    this.stack = stack;
    this.dialog = dialog;
    this.ToAddress = ToAddress;
    this.changeState("INITIALIZED");

    // Array of spawned dialog (for INVITE outside of dialog).
    // dialogs that haven't transitioned to CONFIRMED state will be
    // terminated in SipInviteRC::terminate()
    this.spawnedDialogs = [];

    // Hash of dialogID -> ack sent for the dialog
    // (for resending acks when 2XX responses are retransmitted)
    this.acks = {};
  });

//----------------------------------------------------------------------
// zapISipInviteRC

//  attribute zapISipInviteRCListener listener;
SipInviteRC.obj("listener", null);

//  readonly attribute zapISipDialog dialog;
SipInviteRC. obj("dialog", null);

//  readonly attribute zapISipAddress ToAddress;
SipInviteRC.obj("ToAddress", null);

//  zapISipRequest formulateInvite();
SipInviteRC.statefun(
  "INITIALIZED",
  function formulateInvite() {
    var m;
    if (this.dialog) {
      // request within dialog
      m = this.dialog.formulateGenericRequest("INVITE");
    }
    else {
      // request outside of a dialog
      m = this.stack.formulateGenericRequest("INVITE", this.ToAddress);
    }
    // add mandatory Contact header (rfc3261 8.1.1.8):
    m.appendHeader(gSyntaxFactory.createContactHeader(this.stack.getContactAddress()));
    return m;

  });

//  void sendInvite(in zapISipRequest request, in zapISipInviteResponseHandler rh);
SipInviteRC.statefun(
  "INITIALIZED",
  function sendInvite(request, rh) {
    this.request = request;
    this.responsehandler = rh;
    // Asynchronously resolve possible destinations for the request:
    this.changeState("RESOLVING");
    this.stack.resolver.resolveEndpointsAsync(request.requestURI,
                                              /*request.getHeaders("Route", {}), */
                                              this);
  });


//----------------------------------------------------------------------
// zapISipResolveListener

//  void resolveComplete([array, size_is(count)] in zapISipEndpoint endpoints,
//                       in unsigned long count);
SipInviteRC.statefun(
  "RESOLVING",
  function resolveComplete(destinations, count) {
    this._destinationIterator = makeArrayIterator(destinations);
    this.changeState("CALLING");
    this.tryNextDestination();
  });

// helper for sending INVITE request to next destination in our
// resolved destinations:
SipInviteRC.statefun(
  "CALLING",
  function tryNextDestination() { 
    if (!this._destinationIterator.hasMore()) {
      if (this.listener) {
        // generate a 408 (Request Timeout) to hand to the listener
        // (RFC3261 8.1.3.1):
        var r = this.stack.formulateResponse("408", this.request, this._ToTag);
        this.listener.notifyResponseReceived(this, this.dialog, r);
      }
      this.terminate();
      return;
    }
    // send request to next destination:
    this.request.getTopViaHeader().setParameter("branch",
                                                 BRANCH_COOKIE+generateUUID());
    if (this.dialog) {
      // we're sending a request within a dialog.
      // update local sequence number
      if (this.dialog.localSequenceNumber ==
          Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER)
        this.dialog.localSequenceNumber = 0;
      this.request.getCSeqHeader().sequenceNumber =
        ++this.dialog.localSequenceNumber;
    }
    this.stack.transactionManager.executeInviteClientTransaction(this.request,
                                                                 this._destinationIterator.getNext(),
                                                                 this);
  });

//----------------------------------------------------------------------
// zapISipClientTransactionUser

//  void handleProvisionalResponse(in zapISipResponse response);
SipInviteRC.statefun(
  "CALLING",
  function handleProvisionalResponse(response) {
    var dialog;
    if (this.dialog) {
      // This is a RE-INVITE; we don't spawn any new dialogs.
      // XXX maybe we should assert that the 'To' tag in the response
      // corresponds to our dialog.
      dialog = this.dialog;
    }
    else if (response.statusCode != "100" &&
             response.getToHeader().getParameter("tag")) {
      // Spawn a new dialog if this is a response !=100 with a 'To'
      // tag and we can't find a matching dialog (RFC3261 12.1):
      dialog = this.stack.findDialog(constructServerDialogID(response));
      if (!dialog) {
        // create new early dialog:
        dialog = this.stack.createDialogUAC(this.request, response);
        dialog.pendingInviteRC = this;
        this.spawnedDialogs.push(dialog);
      }
    }

    if (this.listener)
      this.listener.notifyResponseReceived(this, dialog, response);
  });

//  void handleFailureResponse(in zapISipResponse response);
SipInviteRC.statefun(
  "CALLING",
  function handleFailureResponse(response) {
    if (this.listener)
      this.listener.notifyResponseReceived(this, this.dialog, response);
    this.terminate();
  });

//  void handleSuccessResponse(in zapISipResponse response);
SipInviteRC.statefun(
  "CALLING",
  function handleSuccessResponse(response) {
    this.changeState("2XX_RECEIVED");

    this.handle2XXResponse(response);
    
    // RFC3261 13.2.24: We now wait for 64*T1 for further 2XX
    // responses.
    var timerA = makeOneShotTimer(this, this.stack.transactionManager.T1 * 64);
    // To ensure that we get any further 2XX responses, we register with the
    // stack:
    this.stack.registerInviteRC(this, constructClientTransactionKey(this.request));
    
  });

//  void handleTimeout();
SipInviteRC.statefun(
  "CALLING",
  function handleTimeout() {
    this.tryNextDestination();
  });

//----------------------------------------------------------------------
// nsITimerCallback

// void notify(in nsITimer timer);
SipInviteRC.statefun(
  "2XX_RECEIVED",
  function notify(timer) {
    // stop waiting for further responses:
    this.terminate();
  });

//----------------------------------------------------------------------
// interface to SipUAStack

// this method will be called when an additional 2XX response is
// received by the stack:
SipInviteRC.statefun(
  "2XX_RECEIVED",
  function handleResponse(response) {
    // take care of resending ACKs and generating new dialogs:
    this.handle2XXResponse(response);
  });

//----------------------------------------------------------------------
// Implementation helpers

SipInviteRC.fun(
  function terminate() {
    // remove ourselves as pending invite & terminate all spawned
    // dialogs that haven't transitioned to CONFIRMED state:
    this.spawnedDialogs.forEach(
      function(d) {
        if (d.dialogState == "EARLY")
          d.terminate();
        d.pendingInviteRC = null;
      });
    // remove ourselves as pending invite from initial dialog (if this
    // was an in-dialog request):
    if (this.dialog)
      this.dialog.pendingInviteRC = null;
    
    this.changeState("TERMINATED");

    if (this.listener) {
      this.listener.notifyTerminated(this);
      this.listener = null;
    }
  });

SipInviteRC.fun(
  function handle2XXResponse(response) {
    // check if we already have a dialog for the given To tag:
    var dialog = this.stack.findDialog(constructServerDialogID(response));
    if (!dialog) {
      // no -> create new dialog:
      this._assert(!this.dialog, "dialog establishing response for re-invite???");
      dialog = this.stack.createDialogUAC(this.request, response);
      dialog.pendingInviteRC = this;
      this.spawnedDialogs.push(dialog);
    }
    else if (this.dialog.dialogState == "EARLY") {
      dialog.confirm(response);
    }
    
    // We need to acknowledge the response.
    // Check if we already have an ACK that we can send:
    var ack = hashget(this.acks, dialog.dialogID);
    if (!ack) {
      // We only inform the listener of the response when there is no
      // ack yet to filter out 2xx retransmissions:
      if (this.listener)
        this.listener.notifyResponseReceived(this, dialog, response);
      
      // create an ACK template (RFC3261 13.2.2.4):
      var ackTemplate = dialog.formulateGenericRequest("ACK");
      // copy sequence number from INVITE:
      ackTemplate.getCSeqHeader().sequenceNumber =
        this.request.getCSeqHeader().sequenceNumber;
      // XXX copy credentials (RFC3261 13.2.4)

      // set a new branch id:
      ackTemplate.getTopViaHeader().setParameter("branch",
                                                 BRANCH_COOKIE+generateUUID());
      
      // invoke the response handler to generate the ACK:
      ack = this.responsehandler.handle2XXResponse(this, dialog,
                                                   response, ackTemplate);
      // cache ack for retransmissions:
      hashset(this.acks, dialog.dialogID, ack);
    }
    
    // send ACK
    var rc = this;
    var resolveListener = {
      resolveComplete : function (destinations, count) {
        // we send the ACK to the first destination:
        if (destinations.length == 0) {
          // XXX should we flag an error to the listener, or even
          // close the dialog? The problem is that we can NEVER be
          // sure whether the remote UAS receives the ACK, so maybe we
          // should just rely on the UAS sending us a BYE if it
          // doesn't see an ACK. At least we know that the UAS can
          // reach us.
          return;
        }
        if (destinations.length > 1) rc._warning("Multiple destinations for ACK!");
        // pass ACK request directly to transport layer:
        rc.stack.transport.sendRequest(ack, destinations[0], null);
      }
    };
    
    this.stack.resolver.resolveEndpointsAsync(ack.requestURI,
                                              /* ack.getHeaders("Route", {}),*/
                                              resolveListener);
  });


////////////////////////////////////////////////////////////////////////
// SipNonInviteRS
// request server for receiving non-invite requests
// INITIALIZED --> RESPONDING --> TERMINATED

var SipNonInviteRS = makeClass("SipNonInviteRS", SupportsImpl, StateMachine);
SipNonInviteRS.addInterfaces(Components.interfaces.zapISipNonInviteRS,
                             Components.interfaces.zapISipServerTransactionUser);

SipNonInviteRS.fun(
  function init(stack, dialog, request) {
    this.stack = stack;
    this.dialog = dialog;
    if (dialog)
      this._ToTag = dialog.localTag;

    this.request = request;
    this.changeState("INITIALIZED");
  });

//----------------------------------------------------------------------
// zapISipNonInviteRS

//  attribute zapISipNonInviteRSListener listener;
SipNonInviteRS.obj("listener", null);

//  readonly attribute zapISipRequest request;
SipNonInviteRS.obj("request", null);

//  readonly attribute zapISipDialog dialog;
SipNonInviteRS.obj("dialog", null);

//  zapISipResponse formulateResponse(in ACString statusCode);
SipNonInviteRS.fun(
  function formulateResponse(statusCode) {
    var m;
    if (!this._ToTag) {
      // Generate a To-tag.
      // The same tag MUST be used for all responses (RFC3261 8.2.6.2):
      this._ToTag = generateTag();
    }
    return this.stack.formulateResponse(statusCode, this.request, this._ToTag);
  });

//  zapISipDialog sendResponse(in zapISipResponse response);
SipNonInviteRS.statefun(
  "INITIALIZED",
  function sendResponse(response) {
    // Send our first response, which might a provisional or final response:
    this.transaction =
      this.stack.transactionManager.executeNonInviteServerTransaction(this.request,
                                                                      this,
                                                                      response);
    if (response.statusCode[0] == "1") {
      // We sent a transitional response.
      this.changeState("RESPONDING");
    } else {
      // We sent a final response.
      this.terminate();
    }

    return this.dialog;
  });

SipNonInviteRS.statefun(
  "RESPONDING",
  function sendResponse(response) {
    this.transaction.executeNonInviteServerTransaction(request,
                                                       this,
                                                       response);
    if (response.statusCode[0] != "1")
      this.terminate();

    return this.dialog;
  });

//----------------------------------------------------------------------
// zapISipServerTransactionUser

//  void handleTimeout(in zapISipServerTransaction transaction);
SipNonInviteRS.fun(
  function handleTimeout(transaction) {
    this.terminate();
  });

//  void transactionTerminated(in zapISipServerTransaction transaction);
SipNonInviteRS.fun(
  function transactionTerminated(transaction) {
    this.terminate();
  });

//----------------------------------------------------------------------

SipNonInviteRS.fun(
  function terminate() {
    // if this was a BYE request, we need to terminate our associated
    // dialog:
    if (this.dialog && this.request.method == "BYE")
      this.dialog.terminate();

    this.changeState("TERMINATED");

    if (this.listener) {
      this.listener.notifyTerminated(this);
      this.listener = null;
    }
  });


////////////////////////////////////////////////////////////////////////
// SipInviteRS
// request server for receiving invite requests
// INITIALIZED --> ACK_PENDING --> TERMINATED

var SipInviteRS = makeClass("SipInviteRS", SupportsImpl, StateMachine);
SipInviteRS.addInterfaces(Components.interfaces.zapISipInviteRS,
                          Components.interfaces.zapISipServerTransactionUser);

SipInviteRS.fun(
  function init(stack, dialog, request) {
    this.stack = stack;
    this.dialog = dialog;
    if (dialog) {
      dialog.pendingInviteRS = this;
      this._ToTag = dialog.localTag;
    }
    this.request = request;
    this.transaction =
      stack.transactionManager.executeInviteServerTransaction(request,
                                                              this,
                                                              false);
    this._dump("initialized");
    this.changeState("INITIALIZED");
  });

//----------------------------------------------------------------------
// zapISipInviteRS

//  attribute zapISipInviteRSListener listener;
SipInviteRS.obj("listener", null);

//  readonly attribute zapISipRequest request;
SipInviteRS.obj("request", null);

//  readonly attribute zapISipDialog dialog;
SipInviteRS.obj("dialog", null);

//  zapISipResponse formulateResponse(in ACString statusCode);
SipInviteRS.statefun(
  "INITIALIZED",
  function formulateResponse(statusCode) {
    var m;
    if (!this._ToTag) {
      // Generate a To-tag.
      // The same tag MUST be used for all responses (RFC3261 8.2.6.2):
      this._ToTag = generateTag();
    }
    m = this.stack.formulateResponse(statusCode, this.request, this._ToTag);
    if (statusCode[0] == "2" ||
        (statusCode[0] == "1" && statusCode != "100")) {
      // This is a dialog establishing response. Formulate according
      // to RFC3261 12.1.1:
      // copy Record-Route headers:
      this.request.getHeaders("Record-Route", {}).forEach(function(v) {
                                                            m.appendHeader(v);
                                                          });
      // set Contact header:
      m.appendHeader(gSyntaxFactory.createContactHeader(this.stack.getContactAddress()));
    }
    return m;
  });

//  zapISipDialog sendResponse(in zapISipResponse response);
SipInviteRS.statefun(
  "INITIALIZED",
  function sendResponse(response) {
    this.maybeCreateDialog(response);

    this._dump("sending response "+response.statusCode);
    this.transaction.sendResponse(response);
    if (response.statusCode[0] == "2") {
      this.response = response;
      if (this.dialog.dialogState != "CONFIRMED")
        this.dialog.confirm(response);
      this.changeState("ACK_PENDING");
      // We need to periodically resend the response until the ACK
      // arrives (RFC3261 13.3.1.4)
      // We'll transmit at maximum until A times out..
      this.timerA = makeOneShotTimer(this, this.stack.transactionManager.T1 * 64);
      // ... at doubleing T1 intervals (< T2)
      this.timerB = makeOneShotTimer(this, this.stack.transactionManager.T1);
    }
    else if (response.statusCode[0] != "1") {
      if (this.dialog)
        this.dialog.terminate();
      this.terminate();
    }
    return this.dialog;
  });

SipInviteRS.fun(
  function maybeCreateDialog(response) {
    if (this.dialog || response.statusCode == "100") return;

    this.dialog = this.stack.createDialogUAS(this.request, response);
    this.dialog.pendingInviteRS = this;
  });

//----------------------------------------------------------------------
// zapISipServerTransactionUser

//  void handleTimeout(in zapISipServerTransaction transaction);
SipInviteRS.fun(
  function handleTimeout(transaction) {
    this.terminate();
  });

//  void transactionTerminated(in zapISipServerTransaction transaction);
SipInviteRS.fun(
  function transactionTerminated(transaction) {
  });

//----------------------------------------------------------------------
// nsITimerCallback

// void notify(in nsITimer timer);
SipInviteRS.statefun(
  "ACK_PENDING",
  function notify(timer) {
    if (timer == this.timerA) {
      // stop retransmissions
      this.terminate();
    }
    else {
      // must be timer B.
      // retransmit response:
      this.stack.transport.sendResponse(this.response, null);
      // reset B to fire at min(old_delay*2, T2):
      resetOneShotTimer(this.timerB, Math.min(this.timerB.delay*2, this.stack.transactionManager.T2));
    }
  });

//----------------------------------------------------------------------
// Interface to dialog:

SipInviteRS.statefun(
  "ACK_PENDING",
  function handleACK(dialog, request) {
    if (this.listener) {
      this.listener.notifyACKReceived(this, request);
    }
    this.terminate();
  });

//----------------------------------------------------------------------

SipInviteRS.fun(
  function terminate() {
    if (this.timerA) {
      this.timerA.cancel();
      this.timerB.cancel();
      this.timerA = null;
      this.timerB = null;
    }

    if (this.dialog) {
      // terminate early dialogs:
      if (this.dialog.dialogState == "EARLY")
        this.dialog.terminate();
      this.dialog.pendingInviteRS = null;
    }
    this._dump("terminated");
    this.changeState("TERMINATED");

    if (this.listener) {
      this.listener.notifyTerminated(this);
      this.listener = null;
    }
  });
