/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipUARequestCore.jsm', null)" -*- */
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
                     "SipSubscribeRC",
                     "SipInviteRC",
                     "SipNonInviteRS",
                     "SipInviteRS"];

Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");



// name our global object:
// function toString() { return "[SipUARequestCore.jsm]"; }

////////////////////////////////////////////////////////////////////////
// SipNonInviteRC
// request client for sending non-invite requests
// INITIALIZED --> RESOLVING --> CALLING --> INITIALIZED

var SipNonInviteRC = makeClass("SipNonInviteRC", SupportsImpl, StateMachine);
SipNonInviteRC.addInterfaces(Components.interfaces.zapISipNonInviteRC,
                             Components.interfaces.zapISipResolveListener,
                             Components.interfaces.zapISipClientTransactionUser);

SipNonInviteRC.fun(
  function init(stack, dialog, request, flags) {
    this.stack = stack;
    this.dialog = dialog;
    this.request = request;
    this.flags = flags;

    this.changeState("INITIALIZED");
  });

//----------------------------------------------------------------------
// zapISipNonInviteRC

//  readonly attribute zapISipRequest request;
SipNonInviteRC.obj("request", null);

//  readonly attribute zapISipResponse response;
SipNonInviteRC.obj("response", null);

//  readonly attribute zapISipDialog dialog;
SipNonInviteRC.obj("dialog", null);

//  void sendRequest();
SipNonInviteRC.statefun(
  "INITIALIZED",
  function sendRequest(listener) {
    this._listener = listener;
    this.active = true;

    // Figure out our destination uri (rfc3261 8.1.2):
    // If we have the top route is a loose router it will be our
    // destination. Otherwise we use the request uri:
    var destinationURI;
    var removeTopRoute; // should we remove the top route header prior
                        // to sending?
    var topRoute = this.request.getTopRouteHeader();
    if (topRoute &&
        topRoute.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("lr")) {
      destinationURI = topRoute.address.uri;
      removeTopRoute = this.flags & Components.interfaces.zapISipUAStack.ELIDE_DESTINATION_ROUTE_HEADER;
    }
    else {
      destinationURI = this.request.requestURI;
      removeTopRoute = false;
    }
    destinationURI = destinationURI.QueryInterface(Components.interfaces.zapISipSIPURI);

    // construct a (possibly cloned) request just for this send iteration:
    if (removeTopRoute) {
      // We need to specify somewhere which parts of the request
      // object are ok to be modified without cloning (like CSeq,
      // branch parameter, ...). At the moment we do a shallow clone,
      // so any header modifications feed through to the original
      // request. Maybe we should always clone deep.
      this.currentRequest = this.request.clone(false).QueryInterface(Components.interfaces.zapISipRequest);
      this.currentRequest.removeHeader(this.currentRequest.getTopHeader("Route"));
    }
    else
      this.currentRequest = this.request;
    
    // Asynchronously resolve possible destinations for the request:
    this.changeState("RESOLVING");

    // RFC3261 18.1.1:
    // If a request is within 200 bytes of the path MTU, or if it is
    // larger than 1300 bytes and the path MTU is unknown, the request
    // MUST be sent using an RFC 2914 [43] congestion controlled
    // transport protocol, such as TCP.
    // XXX We add another 100 bytes padding, since the request core still
    // adds branch parameters and the sip transport layer adds a sent-by
    // attribute.
    var attemptTCP = this.currentRequest.serialize().length > 1200;      
    
    this.stack.resolver.resolveDestinationsAsync(destinationURI,
                                                 this, attemptTCP);
  });

//  readonly attribute boolean active;
SipNonInviteRC.obj("active", false);

//  zapISipDialog createDialog();
SipNonInviteRC.fun(
  function createDialog() {
    if (!this.response) return null;
    return this.stack.createDialogUAC(this.currentRequest, this.response);
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
      this.changeState("INITIALIZED");
      
      // generate a 408 (Request Timeout)
      // (RFC3261 8.1.3.1):
      this.response = this.stack.formulateResponse("408", this.request, this._ToTag);
      this.active = false;
      
      if (this._listener) {
        var l = this._listener;
        this._listener = null;
        l.notifyResponseReceived(this, this.dialog, this.response, null);
      }
      return;
    }
    // send request to next destination:
    var destination = this._destinationIterator.getNext();
    var topVia = this.currentRequest.getTopViaHeader();
    topVia.setParameter("branch",
                        this.stack.shortBranchParameters ?
                        BRANCH_COOKIE+generateTag() :
                        BRANCH_COOKIE+gUUIDGenerator.generateUUIDString());
      
    if (this.dialog) {
      // we're sending a request within a dialog.
      // update local sequence number
      if (this.dialog.localSequenceNumber ==
          Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER)
        this.dialog.localSequenceNumber = 0;
      this.currentRequest.getCSeqHeader().sequenceNumber =
        ++this.dialog.localSequenceNumber;
    }
    else {
      this.currentRequest.getCSeqHeader().sequenceNumber += 1;
    }

    this.stack.transactionManager.executeNonInviteClientTransaction(this.currentRequest,
                                                                    destination,
                                                                    this);
  });

//----------------------------------------------------------------------
// zapISipClientTransactionUser

//  void handleFailureResponse(in zapISipResponse response, in zapISipFlow flow);
SipNonInviteRC.statefun(
  "CALLING",
  function handleFailureResponse(response, flow) {
    this.changeState("INITIALIZED");
    this.response = response;
    this.active = false;
    if (this._listener) {
      var l = this._listener;
      this._listener = null;
      l.notifyResponseReceived(this, this.dialog, this.response, flow);
    }
  });

//  void handleProvisionalResponse(in zapISipResponse response, in zapISipFlow flow);
SipNonInviteRC.statefun(
  "CALLING",
  function handleProvisionalResponse(response, flow) {
    this.response = response;
    if (this._listener)
      this._listener.notifyResponseReceived(this, this.dialog, this.response, flow);
    // stay in CALLING state
  });

//  void handleSuccessResponse(in zapISipResponse response, in zapISipFlow flow);
SipNonInviteRC.statefun(
  "CALLING",
  function handleSuccessResponse(response, flow) {
    this.changeState("INITIALIZED");
    this.response = response;
    this.active = false;
    if (this._listener) {
      var l = this._listener;
      this._listener = null;
      l.notifyResponseReceived(this, this.dialog, this.response, flow);
    }
  });

//  void handleTimeout();
SipNonInviteRC.statefun(
  "CALLING",
  function handleTimeout() {
    this.tryNextDestination();
  });


////////////////////////////////////////////////////////////////////////
// SipInviteRC
// request client for sending invite requests
// INITIALIZED --> RESOLVING --> CALLING --> 2XX_RECEIVED --> INITIALIZED

var SipInviteRC = makeClass("SipInviteRC", SupportsImpl, StateMachine);
SipInviteRC.addInterfaces(Components.interfaces.zapISipInviteRC,
                          Components.interfaces.zapISipResolveListener,
                          Components.interfaces.zapISipClientTransactionUser);

SipInviteRC.fun(
  function init(stack, dialog, request, flags) {
    this.stack = stack;
    this.dialog = dialog;
    this.request = request;
    this.flags = flags;
    
    this.changeState("INITIALIZED");

    // Array of spawned dialog (for INVITE outside of dialog).
    // dialogs that haven't transitioned to CONFIRMED state will be
    // terminated in SipInviteRC::resetCall()
    this.spawnedDialogs = [];

    // Hash of dialogID -> ack sent for the dialog
    // (for resending acks when 2XX responses are retransmitted)
    this.acks = {};
  });

// this is set to true if the request is cancelled while in the
// RESOLVING or CALLING state:
SipInviteRC.obj("cancelled", false);

// this is set to true if we have received a provisional response:
SipInviteRC.obj("provisional_received", false);

// this will be set to the current destination that a request has been
// sent to:
SipInviteRC.obj("current_destination", null);

//----------------------------------------------------------------------
// zapISipInviteRC

//  attribute zapISipRequest request;
SipInviteRC.obj("request", null);

//  attribute zapISipInviteRCListener listener;
SipInviteRC.obj("listener", null);

//  readonly attribute zapISipDialog dialog;
SipInviteRC. obj("dialog", null);

//  void sendInvite(in zapISipInviteResponseHandler rh);
SipInviteRC.statefun(
  "INITIALIZED",
  function sendInvite(rh) {
    // set ourselves as pending invite on the parent dialog (if any),
    // so that concurrent Invites are dealt with appropriately:
    if (this.dialog)
        this.dialog.pendingInviteRC = this;
    
    this.responsehandler = rh;
    
    // Figure out our destination uri (rfc3261 8.1.2):
    // If we have the top route is a loose router it will be our
    // destination. Otherwise we use the request uri:
    var destinationURI;
    var removeTopRoute; // should we remove the top route header prior
                        // to sending?
    var topRoute = this.request.getTopRouteHeader();
    if (topRoute &&
        topRoute.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("lr")) {
      destinationURI = topRoute.address.uri;
      removeTopRoute = this.flags & Components.interfaces.zapISipUAStack.ELIDE_DESTINATION_ROUTE_HEADER;
    }
    else {
      destinationURI = this.request.requestURI;
      removeTopRoute = false;
    }
    destinationURI = destinationURI.QueryInterface(Components.interfaces.zapISipSIPURI);
    
    // construct a (possibly cloned) request for this send iteration
    if (removeTopRoute) {
      // We need to specify somewhere which parts of the request
      // object are ok to be modified without cloning (like CSeq,
      // branch parameter, ...). At the moment we do a shallow clone,
      // so any header modifications feed through to the original
      // request. Maybe we should always clone deep.
      this.currentRequest = this.request.clone(false);
      this.currentRequest.removeHeader(this.currentRequest.getTopHeader("Route"));
    }
    else
      this.currentRequest = this.request;
    
    // Asynchronously resolve possible destinations for the request:
    this.changeState("RESOLVING");

    // RFC3261 18.1.1:
    // If a request is within 200 bytes of the path MTU, or if it is
    // larger than 1300 bytes and the path MTU is unknown, the request
    // MUST be sent using an RFC 2914 [43] congestion controlled
    // transport protocol, such as TCP.
    // XXX We add another 100 bytes padding, since the request core still
    // adds branch parameters and the sip transport layer adds a sent-by
    // attribute.
    var attemptTCP = this.currentRequest.serialize().length > 1200;    
    
    this.stack.resolver.resolveDestinationsAsync(destinationURI,
                                                 this, attemptTCP);
  });

//  void cancel();
SipInviteRC.statefun(
  "*",
  function cancel() {
    // XXX if the state is 2XX_RECEIVED or INITIALIZED it is too late
    // to cancel. Should we throw an exception in these cases?
    this.cancelled = true;
  });

SipInviteRC.statefun(
  "CALLING",
  function cancel() {
    this.cancelled = true;
    if (!this.provisional_received) return;
    
    // construct and send a cancel request (RFC3261 9.1):
    var req = gSyntaxFactory.createRequest();
    // method:
    req.method = "CANCEL";
    // request uri:
    req.requestURI = this.currentRequest.requestURI;
    // copy Route headers:
    this.currentRequest.getHeaders("Route", {}).forEach(function(v) {
                                                          req.appendHeader(v);
                                                        });
    
    // Via:
    req.appendHeader(this.currentRequest.getTopViaHeader());
    // To header:
    req.appendHeader(this.currentRequest.getToHeader());
    // From header:
    req.appendHeader(this.currentRequest.getFromHeader());
    // Call-ID header:
    req.appendHeader(this.currentRequest.getCallIDHeader());
    // CSeq header:
    req.appendHeader(this.currentRequest.getCSeqHeader()); //XXX maybe clone
    req.getCSeqHeader().method = "CANCEL";
    // Max-Forwards header:
    req.appendHeader(this.currentRequest.getSingleHeader("Max-Forwards"));

    this.stack.transactionManager.executeNonInviteClientTransaction(req,
                                                                    this.current_destination,
                                                                    null);

    // start a timer to timeout this transaction in case we don't
    // receive a 487 (Request Terminated) response for the INVITE
    // transaction (RFC3261 9.1):
    // XXX
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
      this.resetCall();
      if (this.listener) {
        // generate a 408 (Request Timeout) to hand to the listener
        // (RFC3261 8.1.3.1):
        var r = this.stack.formulateResponse("408", this.request, null);
        this.listener.notifyResponseReceived(this, this.dialog, r, null);
      }
      return;
    }

    if (this.cancelled) {
      this.resetCall();
      if (this.listener) {
        // generate a 487 (Request Terminated) to hand to the listener
        // (RFC3261 9.1):
        var r = this.stack.formulateResponse("487", this.request, null);
        this.listener.notifyResponseReceived(this, this.dialog, r, null);
      }
      return;
    }
    
    // send request to next destination:
    this.current_destination = this._destinationIterator.getNext();
    var topVia = this.currentRequest.getTopViaHeader();
    topVia.setParameter("branch",
                        this.stack.shortBranchParameters ?
                        BRANCH_COOKIE+generateTag() :
                        BRANCH_COOKIE+gUUIDGenerator.generateUUIDString());
    
    if (this.dialog) {
      // we're sending a request within a dialog.
      // update local sequence number
      if (this.dialog.localSequenceNumber ==
          Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER)
        this.dialog.localSequenceNumber = 0;
      this.currentRequest.getCSeqHeader().sequenceNumber =
        ++this.dialog.localSequenceNumber;
    }
    else {
      this.currentRequest.getCSeqHeader().sequenceNumber += 1;
    }
    
    this.stack.transactionManager.executeInviteClientTransaction(this.currentRequest,
                                                                 this.current_destination,
                                                                 this,
                                                                 this.stack.rport_client);
  });

//----------------------------------------------------------------------
// zapISipClientTransactionUser

//  void handleProvisionalResponse(in zapISipResponse response, in zapISipFlow flow);
SipInviteRC.statefun(
  "CALLING",
  function handleProvisionalResponse(response, flow) {

    if (this.cancelled && !this.provisional_received) {
      // we have received the first provisional response. send a
      // cancellation now:
      this.provisional_received = true;
      this.cancel();
    }

    this.provisional_received = true;
    
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
      dialog = this.stack.findDialog(constructClientDialogID(response));
      if (!dialog) {
        // create new early dialog:
        dialog = this.stack.createDialogUAC(this.currentRequest, response);
        dialog.pendingInviteRC = this;
        this.spawnedDialogs.push(dialog);
      }
    }

    if (this.listener)
      this.listener.notifyResponseReceived(this, dialog, response, flow);
  });

//  void handleFailureResponse(in zapISipResponse response, in zapISipFlow flow);
SipInviteRC.statefun(
  "CALLING",
  function handleFailureResponse(response, flow) {
    this.resetCall();
    
    if (this.listener)
      this.listener.notifyResponseReceived(this, this.dialog, response, flow);
  });

//  void handleSuccessResponse(in zapISipResponse response, in zapISipFlow flow);
SipInviteRC.statefun(
  "CALLING",
  function handleSuccessResponse(response, flow) {
    this.changeState("2XX_RECEIVED");

    this.handle2XXResponse(response, flow);
    
    // RFC3261 13.2.24: We now wait for 64*T1 for further 2XX
    // responses.
    var timerA = makeOneShotTimer(this, this.stack.transactionManager.T1 * 64);
    // To ensure that we get any further 2XX responses, we register with the
    // stack:
    this.stack.registerInviteRC(this, constructClientTransactionKey(this.currentRequest));
    
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
    this.resetCall();
  });

//----------------------------------------------------------------------
// interface to SipUAStack

// this method will be called when an additional 2XX response is
// received by the stack:
SipInviteRC.statefun(
  "2XX_RECEIVED",
  function handleResponse(response, flow) {
    // take care of resending ACKs and generating new dialogs:
    this.handle2XXResponse(response, flow);
  });

//----------------------------------------------------------------------
// Implementation helpers


SipInviteRC.fun(
  function resetCall() {
    // remove ourselves as pending invite & terminate all spawned
    // dialogs that haven't transitioned to CONFIRMED state:
    this.spawnedDialogs.forEach(
      function(d) {
        if (d.dialogState == "EARLY")
          d.terminateDialog();
        d.pendingInviteRC = null;
      });
    // remove ourselves as pending invite from initial dialog (if this
    // was an in-dialog request):
    if (this.dialog)
      this.dialog.pendingInviteRC = null;

    // reset our cancel state:
    this.cancelled = false;
    this.provisional_received = false;
      
    // change state to INITIALIZED; we can resend the response again now:
    this.changeState("INITIALIZED");

    if (this.responsehandler) {
      this.responsehandler.notifyTerminated(this);
      this.responsehandler = null;
    }
  });

SipInviteRC.fun(
  function handle2XXResponse(response, flow) {
    // check if we already have a dialog for the given To tag:
    var dialog = this.stack.findDialog(constructClientDialogID(response));
    if (!dialog) {
      this._dump("Creating new dialog "+constructClientDialogID(response)+
                 " because there was no early dialog");
      // no -> create new dialog:
      this._assert(!this.dialog, "dialog establishing response for re-invite???");
      dialog = this.stack.createDialogUAC(this.currentRequest, response);
      dialog.pendingInviteRC = this;
      this.spawnedDialogs.push(dialog);
    }
    else if (dialog.dialogState == "EARLY") {
      // confirm dialog. this will also recompute the routeset
      // (RFC3261 13.2.2.4)
      dialog.confirmUAC(response);
    }
    
    // We need to acknowledge the response.
    // Check if we already have an ACK that we can send:
    var ack = hashget(this.acks, dialog.dialogID);
    if (!ack) {
      // We only inform the listener of the response when there is no
      // ack yet to filter out 2xx retransmissions:
      if (this.listener)
        this.listener.notifyResponseReceived(this, dialog, response, flow);
      
      // create an ACK template (RFC3261 13.2.2.4):
      var ackTemplate = dialog.formulateACK();
      // copy sequence number from INVITE:
      ackTemplate.getCSeqHeader().sequenceNumber =
        this.currentRequest.getCSeqHeader().sequenceNumber;
      // copy credentials (RFC3261 13.2.2.4/22.1):
      // XXX should we copy?
      this.currentRequest.getHeaders("Authorization", {}).forEach(function(v) { ackTemplate.appendHeader(v); });
      this.currentRequest.getHeaders("Proxy-Authorization", {}).forEach(function(v) { ackTemplate.appendHeader(v); });

      // set a new branch id:
      ackTemplate.getTopViaHeader().setParameter("branch",
                                                       this.stack.shortBranchParameters ?
                                                       BRANCH_COOKIE+generateTag() :
                                                       BRANCH_COOKIE+gUUIDGenerator.generateUUIDString());
      
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
    
    // Figure out our destination uri (rfc3261 8.1.2):
    // If we have the top route is a loose router it will be our
    // destination. Otherwise we use the request uri:
    var ackDestinationURI;
    var topRoute = ack.getTopRouteHeader();
    if (topRoute &&
        topRoute.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("lr"))
      ackDestinationURI = topRoute.address.uri;
    else
      ackDestinationURI = ack.requestURI;
    ackDestinationURI = ackDestinationURI.QueryInterface(Components.interfaces.zapISipSIPURI);
    
    // RFC3261 18.1.1:
    // If a request is within 200 bytes of the path MTU, or if it is
    // larger than 1300 bytes and the path MTU is unknown, the request
    // MUST be sent using an RFC 2914 [43] congestion controlled
    // transport protocol, such as TCP.
    // XXX We add another 100 bytes padding, since the request core still
    // adds branch parameters and the sip transport layer adds a sent-by
    // attribute.
    var attemptTCP = ack.serialize().length > 1200;
    
    this.stack.resolver.resolveDestinationsAsync(ackDestinationURI,
                                                 resolveListener, attemptTCP);
  });


////////////////////////////////////////////////////////////////////////
// SipSubscribeRC
// request client for sending out-of-dialog SUBSCRIBE requests (RFC3265)
// INITIALIZED --> CALLING --> WAITING --> TERMINATED

var SipSubscribeRC = makeClass("SipSubscribeRC", SupportsImpl, StateMachine);
SipSubscribeRC.addInterfaces(Components.interfaces.zapISipSubscribeRC,
                             Components.interfaces.zapISipNonInviteRCListener);

SipSubscribeRC.fun(
  function init(stack, request, flags) {
    this.stack = stack;
    // we leverage SipNonInviteRC:
    this.rc = SipNonInviteRC.instantiate();
    this.rc.init(stack, null, request, flags);

    this.changeState("INITIALIZED");
  });

//----------------------------------------------------------------------
// zapISipSubscribeRC

//  attribute zapISipRequest request;
SipSubscribeRC.gettersetter(
  "request",
  function get_request() { return this.rc.request; },
  function set_request(r) { this.rc.request = r; });

//  void sendSubscribe(in zapISipSubscribeResponseHandler rh);
SipSubscribeRC.statefun(
  "INITIALIZED",
  function sendSubscribe(handler) {
    this._handler = handler;
    this.changeState("CALLING");
    this.rc.sendRequest(this);
  });

//  void stopWaitingForDialogs();
SipSubscribeRC.statefun(
  "WAITING",
  function stopWaitingForDialogs() {
    this.state = "TERMINATED";
  });

//----------------------------------------------------------------------
// zapISipNonInviteRCListener

//   void notifyResponseReceived(in zapISipNonInviteRC requestClient,
//                               in zapISipDialog dialog,
//                               in zapISipResponse response,
//                               in zapISipFlow flow);  
SipSubscribeRC.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    if (response.statusCode[0] == "2" && this.currentState != "TERMINATED") {
      // check if we have a dialog already:
      var dialog = this.stack.findDialog(constructClientDialogID(response));
      if (!dialog) {
        this.changeState("WAITING");
        dialog = this.stack.createDialogUAC(this.rc.request, response);
        if (dialog) {
          if (this._handler)
            this._handler.handleNewDialog(this, dialog, response);
        }
        else {
          // the response was not acceptable for creating a dialog
          // (e.g. missing contact header in the case of wengo)
          this._dump("error creating dialog with: \n"+
                     response.serialize());
          // XXX maybe we should pass the error to our handler
          // somehow
        }
      }
    }
    else if (response.statusCode[0] != "1") {
      this.changeState("INITIALIZED");
    }
    
    if (this._handler)
      this._handler.handleResponse(this, response);
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
  });

//----------------------------------------------------------------------

SipNonInviteRS.fun(
  function terminate() {
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

var SipInviteRS = makeClass("SipInviteRS",
                            SupportsImpl, StateMachine, Unwrappable);
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

//  zapISipResponse formulateResponse(in ACString statusCode, in zapISipAddress contactAddress);
SipInviteRS.statefun(
  "INITIALIZED",
  function formulateResponse(statusCode, contactAddress) {
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
      m.appendHeader(gSyntaxFactory.createContactHeader(contactAddress));
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
        this.dialog.confirmUAS(response);
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
        this.dialog.terminateDialog();
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
        this.dialog.terminateDialog();
      this.dialog.pendingInviteRS = null;
    }
    this._dump("terminated");
    this.changeState("TERMINATED");

    if (this.listener) {
      this.listener.notifyTerminated(this);
      this.listener = null;
    }
  });

//----------------------------------------------------------------------
// Interface to SipUAStack:

// the sip ua stack will call this to cancel the transaction
SipInviteRS.fun(
  function cancel() {
    if (this.currentState == "INITIALIZED") {
      if (this.listener) {
        this.listener.notifyCancelled(this);
      }
      // send a 487:
      var resp = this.formulateResponse("487");
      this.sendResponse(resp);
    }
    // else ...too late
  });
