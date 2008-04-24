/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipTransactions.js', null)" -*- */
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

debug("*** loading SipTransactions\n");

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");

// name our global object:
// function toString() { return "[SipTransactions.js]"; }


//----------------------------------------------------------------------
// helpers

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP TRANSACT", level, mes);
}


////////////////////////////////////////////////////////////////////////
// SipTransaction : base class for client and server transactions

var SipTransaction = makeClass("SipTransaction", SupportsImpl);
SipTransaction.addInterfaces(Components.interfaces.nsITimerCallback);

// Transactions are statemachines. _state holds the current state
// object that events will be forwarded to:
SipTransaction.obj("_state", null);

SipTransaction.fun(
  function changeState(newState) {
    this._state = newState;
    if (newState.enterState)
      newState.enterState(this);
  });

// Initial request that created this transaction (initialized by
// subclasses):
SipTransaction.obj("initialRequest", null);

// Key to identify this transaction in the client or server pool
// (initialized by subclasses):
SipTransaction.obj("key", null);

// Turn off SipTransaction debug messages.
// XXX This should be configuarable
SipTransaction.spec(function _dump() {});

//----------------------------------------------------------------------
// nsITimerCallback implementation:

// void notify(in nsITimer timer);
SipTransaction.fun(
  function notify(timer) {
    // dispatch to current state:
    if (this._state.handleTimer)
      this._state.handleTimer(this, timer);
  });


////////////////////////////////////////////////////////////////////////
// SipClientTransaction

var SipClientTransaction = makeClass("SipClientTransaction",
                                     SipTransaction);      

SipClientTransaction.fun(
  function handleResponse(r, flow) {
    this._dump("Received a "+r.statusCode+" response ("+r.reasonPhrase+")");
    // dispatch to current state:
    if (this._state.handleResponse)
      this._state.handleResponse(this, r, flow);
  });

////////////////////////////////////////////////////////////////////////
// SipInviteClientTransaction : An INVITE Client Transaction (RFC3261,
// Section 17.1.1)
//
// An INVITE Client Transaction is a state-machine with 4 states
// (Calling, Proceeding, Completed, Terminated). We implement each
// state as an object on SipInviteClientTransaction. Responses and
// events will be dispatched to the appropriate method on the
// currently active state object.


var SipInviteClientTransaction = makeClass("SipInviteClientTransaction",
                                           SipClientTransaction);

SipInviteClientTransaction.getter(
  "_name_",
  function get_name_() {
    return "INVITE Client Transaction ("+this.key+")";
  });

SipInviteClientTransaction.fun(
  function execute(manager, transport, request, endpoint, TU, refresh) {
    this.initialRequest = request;
    this.key = constructClientTransactionKey(request);
    log("Executing "+this._name_);
    
    this.manager = manager;
    this.transport = transport;
    this.endpoint = endpoint;
    this.TU = TU;
    this.periodicRefresh = refresh;
    this.timers = {};
    
    // set initial state:
    this.changeState(this.Calling);
  });

SipInviteClientTransaction.fun(
  function sendACK(r) {
    // formulate an ACK request to acknowledge r (rfc3261 17.1.1.3):
    var m = gSyntaxFactory.createRequest();
    m.method = "ACK";
    // copy request uri & top 'Via' from original request:
    m.requestURI = this.initialRequest.requestURI;
    m.appendHeader(this.initialRequest.getTopViaHeader());
    // copy 'To' from r:
    m.appendHeader(r.getSingleHeader("To"));
    // copy 'Call-ID' and 'From' from original request:
    m.appendHeader(this.initialRequest.getSingleHeader('Call-ID'));
    m.appendHeader(this.initialRequest.getSingleHeader('From'));
    // set 'CSeq' to sequence number of original request + 'ACK':
    m.appendHeader(
      gSyntaxFactory.createCSeqHeader("ACK",
                                      this.initialRequest.getSingleHeader("CSeq").sequenceNumber));
    // add a 'Max-Forwards' header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());
    // copy any 'Route' headers from the original request:
    var routeHeaders = this.initialRequest.getHeaders("Route", {});
    routeHeaders.forEach(function(h) { m.appendHeader(h); });

    // now send the request:
    this._dump("sending ACK");
    this.transport.sendRequest(m, this.endpoint, this.connection);
  });

//----------------------------------------------------------------------
// 'Calling' state

SipInviteClientTransaction.obj(
  "Calling",
  {
    enterState : function enterState_Calling(t) {
      log(t._name_+": CALLING");
      t._dump("Entering 'CALLING' state");
      
      // add ourselves to our manager's transaction pool:
      t.manager.registerClientTransaction(t);

      // send the INVITE:
      t.connection = t.transport.sendRequest(t.initialRequest, t.endpoint, null);

      // start timer 'B':
      t.timers.timerB = makeOneShotTimer(t, t.manager.T1 * 64);
      
      // start timer 'A' if transport is not reliable:
      if (t.endpoint.transport == "UDP") {
        t.timers.timerA = makeOneShotTimer(t, t.manager.T1);
      }
    },
      
      handleResponse : function handleResponse_Calling(t, r, flow) {
      switch (r.statusCode[0]) {
        case '1':
          if (t.TU)
            t.TU.handleProvisionalResponse(r, flow);
          t.changeState(t.Proceeding);
          break;
        case '2':
          if (t.TU)
            t.TU.handleSuccessResponse(r, flow);
          t.changeState(t.Terminated);
          break;
        default: // 300 - 699
          t.sendACK(r);
          if (t.TU)
            t.TU.handleFailureResponse(r, flow);
          t.changeState(t.Completed);
          break;
      }
    },

    handleTimer : function handleTimer_Calling(t, timer) {
      if (timer == t.timers.timerA) {        
        t._dump("Timer A fired. Resending Invite");
        // Resend INVITE:
        t.connection = t.transport.sendRequest(t.initialRequest, t.endpoint, t.connection);
        // Reset A. Double interval:
        resetOneShotTimer(t.timers.timerA, t.timers.timerA.delay * 2);
      }
      else if (timer == t.timers.timerB) {
        t._dump("Timer B fired.");
        if (t.TU)
          t.TU.handleTimeout();
        t.changeState(t.Terminated);
      }
    }
  });

//----------------------------------------------------------------------
// 'Proceeding' state

SipInviteClientTransaction.obj(
  "Proceeding",
  {
    enterState : function enterState_Proceeding(t) {
      log(t._name_+": PROCEEDING");
      t._dump("Entering 'Proceeding' state");

      // start timer R if refreshing is requested:
      if (t.periodicRefresh)
        t.timers.timerR = makeOneShotTimer(t, t.manager.Trefresh);
    },
      
      handleResponse : function handleResponse_Proceeding(t, r, flow) {
      switch (r.statusCode[0]) {
        case '1':
          if (t.TU)
            t.TU.handleProvisionalResponse(r, flow);
          break;
        case '2':
          if (t.TU)
            t.TU.handleSuccessResponse(r, flow);
          t.changeState(t.Terminated);
          break;
        default: // 300 - 699
          t.sendACK(r);
          if (t.TU)
            t.TU.handleFailureResponse(r, flow);
          t.changeState(t.Completed);
          break;
      }
    },

    handleTimer : function handleTimer_Proceeding(t, timer) {
      if (timer == t.timers.timerR) {
        t._dump("Timer R fired. Resending Invite to refresh NAT binding");
        // Resend INVITE:
        t.connection = t.transport.sendRequest(t.initialRequest, t.endpoint,
                                               t.connection);
        // Reset R:
        resetOneShotTimer(t.timers.timerR, t.timers.timerR.delay);
      }
    }
  });

//----------------------------------------------------------------------
// 'Completed' state

SipInviteClientTransaction.obj(
  "Completed",
  {
    enterState : function enterState_Completed(t) {
      log(t._name_+": COMPLETED");
      t._dump("Entering 'Completed' state");
      if (t.endpoint.transport != "UDP") {
        // reliable transport
        // -> move straight to 'Terminated' state:
        t.changeState(t.Terminated);
      }
      else {
        // start timer 'D' with an absolute value of 32 seconds:
        t.timers.timerD = makeOneShotTimer(t, 32000);
      }
    },
      
      handleResponse : function handleResponse_Completed(t, r, flow) {
      switch (r.statusCode[0]) {
        case '1':
        case '2':
          t._dump("Weird. Unexpected 1XX or 2XX response.");
          break;
        default: // 300 - 699
          t.sendACK(r);
          break;
      }
    },

    handleTimer : function handleTimer_Completed(t, timer) {
      if (timer == t.timers.timerD) {        
        t._dump("Timer D fired after "+t.timers.timerD.delay+" at "+Date());
        t.changeState(t.Terminated);
      }
    }
  });

//----------------------------------------------------------------------
// 'Terminated' state

SipInviteClientTransaction.obj(
  "Terminated",
  {
    enterState : function enterState_Terminated(t) {
      log(t._name_+": TERMINATED");
      t._dump("Entering 'Terminated' state");
      // remove transaction from pool:
      t.manager.unregisterClientTransaction(t);
      // the next line is important to prevent a COM reference cycle:
      delete t.timers;
    }
  });


////////////////////////////////////////////////////////////////////////
// SipNonInviteClientTransaction : A non-INVITE Client Transaction
// (RFC3261, 17.1.2)

var SipNonInviteClientTransaction = makeClass("SipNonInviteClientTransaction",
                                              SipClientTransaction);

SipNonInviteClientTransaction.getter(
  "_name_",
  function get_name_() {
    return "NON-INVITE Client Transaction ("+this.key+")";
  });

SipNonInviteClientTransaction.fun(
  function execute(manager, transport, request, endpoint, TU) {
    this.initialRequest = request;
    this.key = constructClientTransactionKey(request);
    log("Executing "+this._name_);
    
    this.manager = manager;
    this.transport = transport;
    this.endpoint = endpoint;
    this.TU = TU;
    this.timers = {};
    
    // set initial state:
    this.changeState(this.Trying);
  });

//----------------------------------------------------------------------
// 'Trying' state

SipNonInviteClientTransaction.obj(
  "Trying",
  {
    enterState : function enterState_Trying(t) {
      t._dump("Entering 'TRYING' state");

      // add ourselves to our manager's transaction pool:
      t.manager.registerClientTransaction(t);

      // send the request:
      t.connection = t.transport.sendRequest(t.initialRequest, t.endpoint, null);

      // start timer 'F':
      t.timers.timerF = makeOneShotTimer(t, t.manager.T1 * 64);

      // start timer 'E':
      if (t.endpoint.transport == "UDP") {
        t.timers.timerE = makeOneShotTimer(t, t.manager.T1);
      }
    },

      handleResponse : function handleResponse_Trying(t, r, flow) {
      switch (r.statusCode[0]) {
        case '1':
          if (t.TU)
            t.TU.handleProvisionalResponse(r, flow);
          t.changeState(t.Proceeding);
          break;
        case '2':
          if (t.TU)
            t.TU.handleSuccessResponse(r, flow);
          t.changeState(t.Completed);
          break;
        default: // 300 - 699
          if (t.TU)
            t.TU.handleFailureResponse(r, flow);
          t.changeState(t.Completed);
          break;
      }
    },

    handleTimer : function handleTimer_Trying(t, timer) {
      if (timer == t.timers.timerE) {
        t._dump("Timer E fired. Resend request");
        // Resend request:
        t.connection = t.transport.sendRequest(t.initialRequest,
                                               t.endpoint, t.connection);
        // Reset E to fire at min(old-delay*2, T2):
        resetOneShotTimer(timer, Math.min(timer.delay*2, t.manager.T2));
      }
      else if (timer == t.timers.timerF) {
        t._dump("Timer F fired.");
        if (t.TU)
          t.TU.handleTimeout();
        t.changeState(t.Terminated);
      }
    }
  });

//----------------------------------------------------------------------
// 'Proceeding' state

SipNonInviteClientTransaction.obj(
  "Proceeding",
  {
    enterState : function enterState_Proceeding(t) {
      t._dump("Entering 'PROCEEDING' state");
    },

      handleResponse : function handleResponse_Proceeding(t, r, flow) {
      switch (r.statusCode[0]) {
        case '1':
          if (t.TU)
            t.TU.handleProvisionalResponse(r, flow);
          break;
        case '2':
          if (t.TU)
            t.TU.handleSuccessResponse(r, flow);
          t.changeState(t.Completed);
          break;
        default: // 300 - 699
          if (t.TU)
            t.TU.handleFailureResponse(r, flow);
          t.changeState(t.Completed);
          break;
      }
    },

    handleTimer : function handleTimer_Proceeding(t, timer) {
      if (timer == t.timers.timerE) {
        t._dump("Timer E fired. Resend request");
        // Resend request:
        t.connection = t.transport.sendRequest(t.initialRequest,
                                               t.endpoint, t.connection);
        // Reset E to fire at min(old-delay*2, T2):
        resetOneShotTimer(timer, Math.min(timer.delay*2, t.manager.T2));
      }
      else if (timer == t.timers.timerF) {
        t._dump("Timer F fired.");
        if (t.TU)
          t.TU.handleTimeout();
        t.changeState(t.Terminated);
      }
    }
  });

//----------------------------------------------------------------------
// 'Completed' state

SipNonInviteClientTransaction.obj(
  "Completed",
  {
    enterState : function enterState_Completed(t) {
      t._dump("Entering 'Completed' state");
      if (t.endpoint.transport != "UDP") {
        // reliable transport
        // -> move straight to 'Terminated' state:
        t.changeState(t.Terminated);
      }
      else {
        // start timer 'K' with a value of T4:
        t.timers.timerK = makeOneShotTimer(t, t.manager.T4);
      }
    },
      
      handleResponse : function handleResponse_Completed(t, r, flow) {
      t._dump("Buffering obsolete response!");
    },

    handleTimer : function handleTimer_Completed(t, timer) {
      if (timer == t.timers.timerK) {        
        t._dump("Timer K fired");
        t.changeState(t.Terminated);
      }
    }
  });

//----------------------------------------------------------------------
// 'Terminated' state

SipNonInviteClientTransaction.obj(
  "Terminated",
  {
    enterState : function enterState_Terminated(t) {
      t._dump("Entering 'Terminated' state");
      // remove transaction from pool:
      t.manager.unregisterClientTransaction(t);
      // the next line is important to prevent a COM reference cycle:
      delete t.timers;
    }
  });


////////////////////////////////////////////////////////////////////////
// SipServerTransaction: Baseclass for server transactions

var SipServerTransaction = makeClass("SipServerTransaction",
                                     SipTransaction);
SipServerTransaction.addInterfaces(Components.interfaces.zapISipServerTransaction);

SipServerTransaction.fun(
  function handleRequest(r) {
    if (this._state.handleRequest)
      this._state.handleRequest(this, r);
  });

//----------------------------------------------------------------------
// zapISipServerTransaction implementation:

//  readonly attribute zapISipServerTransactionUser transactionUser;
SipServerTransaction.getter(
  "transactionUser",
  function get_transactionUser() {
    return this.TU;
  });

//  readonly attribute zapISipRequest request;
SipServerTransaction.getter(
  "request",
  function get_request() {
    return this.initialRequest;
  });

// void sendResponse(in zapISipResponse response);
SipServerTransaction.fun(
  function sendResponse(r) {
    if (this._state.sendResponse)
      this._state.sendResponse(this, r);
  });


////////////////////////////////////////////////////////////////////////
// SipInviteServerTransaction: An INVITE Server Transaction
// (RFC3261, 17.2.1)

var SipInviteServerTransaction = makeClass("SipInviteServerTransaction",
                                           SipServerTransaction);

SipInviteServerTransaction.getter(
  "_name_",
  function get_name_() {
    return "INVITE Server Transaction ("+this.key+")";
  });

SipInviteServerTransaction.fun(
  function execute(manager, transport, request, TU, generate100) {
    this.initialRequest = request;
    this.key = constructServerTransactionKey(request);
    //XXX examine via header here:
    this.isReliableTransport = false;
    log("Executing "+this._name_);
    
    this.manager = manager;
    this.transport = transport;
    this.TU = TU;
    this.timers = {};
    
    // set initial state:
    this.changeState(this.Proceeding);

    // generate a 100 response if requested:
    if (generate100) {
      // construct response following the rules of RFC3261 8.2.6:
      var response = gSyntaxFactory.createResponse();
      response.statusCode = "100";
      response.reasonPhrase = "Trying";
      response.appendHeader(request.getFromHeader());
      response.appendHeader(request.getCallIDHeader());
      response.appendHeader(request.getCSeqHeader());
      // copy Via headers:
      request.getHeaders("Via", {}).forEach(function(v) {
                                              response.appendHeader(v); });
      response.appendHeader(request.getToHeader());
      //XXX delete To tag if there is one?

      // pass response to transport layer
      this.lastResponse = response;
      transport.sendResponse(response, null);
    }
  });

//----------------------------------------------------------------------
// 'Proceeding' state

SipInviteServerTransaction.obj(
  "Proceeding",
  {
    enterState : function enterState_Proceeding(t) {
      log(t._name_+": PROCEEDING");
      t._dump("Entering 'PROCEEDING' state");
      // add ourselves to our manager's transaction pool:
      t.manager.registerServerTransaction(t);
    },

    handleRequest : function handleRequest_Proceeding(t, r) {
      // resend the most recently sent provisional response:
      if (t.lastResponse)
        t.transport.sendResponse(t.lastResponse, null);
      else
        t._warning("Request restransmission, but no provisional response!");
    },

    sendResponse : function sendResponse_Proceeding(t, r) {
      switch(r.statusCode[0]) {
        case '1':
          break;
        case '2':
          t.changeState(t.Terminated);
          break;
        default: // 300-699
          t.changeState(t.Completed);
          break;
      }
      // pass response to transport layer:
      t.lastResponse = r;
      t.transport.sendResponse(r, null);
    }
  });

//----------------------------------------------------------------------
// 'Completed' state

SipInviteServerTransaction.obj(
  "Completed",
  {
    enterState : function enterState_Completed(t) {
      log(t._name_+": COMPLETED");
      t._dump("Entering 'COMPLETED' state");

      // start timer 'G' if we have an unreliable protocol:
      if (!t.isReliableTransport) {
        t.timers.timerG = makeOneShotTimer(t, t.manager.T1);
      }
      
      // start timer 'H':
      t.timers.timerH = makeOneShotTimer(t, t.manager.T1 * 64);
    },

    handleRequest : function handleRequest_Completed(t, r) {
      if (r.method == "ACK") {
        t.changeState(t.Confirmed);
      }
      else {
        // resend the most recently sent provisional response:
        t.transport.sendResponse(t.lastResponse, null);
      }
    },

    sendResponse : function sendResponse_Completed(t, r) {
      // discard
      t._warning("trying to send response while in completed state!!");
    },

    handleTimer : function handleTimer_Completed(t, timer) {
      if (timer == t.timers.timerG) {        
        t._dump("Timer G fired. Resending response");
        // Resend response:
        t.transport.sendResponse(t.lastResponse, null);
        // Reset G to fire at min(old-delay*2, T2):
        resetOneShotTimer(timer, Math.min(timer.delay*2, t.manager.T2));
      }
      else if (timer == t.timers.timerH) {
        t._dump("Timer H fired.");
        if (t.TU)
          t.TU.handleTimeout(t);
        t.changeState(t.Terminated);
      }
      
    }
  });

//----------------------------------------------------------------------
// 'Confirmed' state

SipInviteServerTransaction.obj(
  "Confirmed",
  {
    enterState : function enterState_Confirmed(t) {
      log(t._name_+": CONFIRMED");
      t._dump("Entering 'CONFIRMED' state");

      if (t.isReliableTransport) {
        // -> move straight to 'Terminated' state:
        t.changeState(t.Terminated);
      }
      else {
        // start timer 'I' with a value of T4:
        t.timers.timerI = makeOneShotTimer(t, t.manager.T4);
      }      
    },

    handleRequest : function handleRequest_Confirmed(t, r) {
      // silently absorb retransmitted ACKs
    },

    sendResponse : function sendResponse_Confirmed(t, r) {
      // discard
      t._warning("trying to send response while in 'Confirmed' state!!");
    },

    handleTimer : function handleTimer_Confirmed(t, timer) {
      if (timer == t.timers.timerI) {
        t._dump("Timer I fired.");
        t.changeState(t.Terminated);
      }
      
    }
  });

//----------------------------------------------------------------------
// 'Terminated' state

SipInviteServerTransaction.obj(
  "Terminated",
  {
    enterState : function enterState_Terminated(t) {
      log(t._name_+": TERMINATED");
      t._dump("Entering 'Terminated' state");
      if (t.TU)
        t.TU.transactionTerminated(t);
      
      // remove transaction from pool:
      t.manager.unregisterServerTransaction(t);
      // the next line is important to prevent a COM reference cycle:
      delete t.timers;
    }
  });


////////////////////////////////////////////////////////////////////////
// SipNonInviteServerTransaction: A non-INVITE Server Transaction
// (RFC3261, 17.2.2)

var SipNonInviteServerTransaction = makeClass("SipNonInviteServerTransaction",
                                              SipServerTransaction);

SipNonInviteServerTransaction.getter(
  "_name_",
  function get_name_() {
    return "NON-INVITE Server Transaction ("+this.key+")";
  });

SipNonInviteServerTransaction.fun(
  function execute(manager, transport, request, TU, response) {
    this.initialRequest = request;
    this.key = constructServerTransactionKey(request);
    log("Executing "+this._name_);
    
    this.manager = manager;
    this.transport = transport;
    this.TU = TU;
    this.timers = {};
    
    // set initial state:
    this.changeState(this.Trying);

    if (response)
      this.sendResponse(response);
  });

//----------------------------------------------------------------------
// 'Trying' state

SipNonInviteServerTransaction.obj(
  "Trying",
  {
    enterState : function enterState_Trying(t) {
      t._dump("Entering 'TRYING' state");

      // add ourselves to our manager's transaction pool:
      t.manager.registerServerTransaction(t);

      // In the Zap architecture we don't pass the request up, as the TU
      // will already have seen it!
      // Pass request to TU:
      //if (t.TU)
      //  t.TU.handleRequest(t.initialRequest);
    },

    handleRequest : function handleRequest_Trying(t, r) {
      // request retransmission -> discard
      log(this._name_+": Discarding retransmitted request");
    },
      
    sendResponse : function sendResponse_Trying(t, r) {
      switch(r.statusCode[0]) {
        case '1':
          t.changeState(t.Proceeding);
          break;
        default: // 200-699
          t.changeState(t.Completed);
          break;
      }
      // pass response to transport layer:
      // XXX need to handle connections
      t.lastResponse = r;
      t.transport.sendResponse(r, null);
    }
  });

//----------------------------------------------------------------------
// 'Proceeding' state

SipNonInviteServerTransaction.obj(
  "Proceeding",
  {
    enterState : function enterState_Proceeding(t) {
      log(this._name_+": PROCEEDING");
      t._dump("Entering 'PROCEEDING' state");
    },

    handleRequest : function handleRequest_Proceeding(t, r) {
      // resend the most recently sent provisional response:
      t.transport.sendResponse(t.lastResponse, null);
    },

    sendResponse : function sendResponse_Proceeding(t, r) {
      switch(r.statusCode[0]) {
        case '1':
          break;
        default: // 200-699
          t.changeState(t.Completed);
          break;
      }
      // pass response to transport layer:
      r.lastResponse = r;
      t.transport.sendResponse(r, null);
    }
  });

//----------------------------------------------------------------------
// 'Completed' state

SipNonInviteServerTransaction.obj(
  "Completed",
  {
    enterState : function enterState_Completed(t) {
      log(t._name_+": COMPLETED");
      t._dump("Entering 'COMPLETED' state");

      // start timer 'J':
      t.timers.timerJ = makeOneShotTimer(t, t.manager.T1 * 64);
    },

    handleRequest : function handleRequest_Completed(t, r) {
      // resend the most recently sent provisional response:
      t.transport.sendResponse(t.lastResponse, null);
    },

    sendResponse : function sendResponse_Completed(t, r) {
      // discard
      t._warning("trying to send response while in terminated state!!");
    },

    handleTimer : function handleTimer_Completed(t, timer) {
      t.changeState(t.Terminated);
    }
  });

//----------------------------------------------------------------------
// 'Terminated' state

SipNonInviteServerTransaction.obj(
  "Terminated",
  {
    enterState : function enterState_Terminated(t) {
      log(t._name_+": TERMINATED");
      t._dump("Entering 'Terminated' state");
      if (t.TU)
        t.TU.transactionTerminated(t);
      
      // remove transaction from pool:
      t.manager.unregisterServerTransaction(t);
      // the next line is important to prevent a COM reference cycle:
      delete t.timers;
    }
  });


////////////////////////////////////////////////////////////////////////
// SipTransactionManager class:

var SipTransactionManager  = makeClass("SipTransactionManager",
                                       SupportsImpl);
SipTransactionManager.addInterfaces(Components.interfaces.zapISipTransactionManager,
                                    Components.interfaces.zapISipTransportSink);

SipTransactionManager.appendCtor(
  function ctor() {
    // Pool of client transactions, indexed by
    // branch-parameter+";"+method:
    this._clientPool = {};

    // Pool of server transactions, indexed by
    // branch+";"+sent-by+";"+method:
    this._serverPool = {};
  });

//----------------------------------------------------------------------
// zapISipTransactionManager implementation:

SipTransactionManager.fun(
  function init(transport) {
    this._transport = transport;
  });

// Timer value 'T1' in ms (see RFC3261 17.1.1.1)
SipTransactionManager.obj("T1", 500);

// Timer value 'T2' in ms (RFC3261 17.1.2.2)
SipTransactionManager.obj("T2", 4000);

// Timer value 'T4' in ms (RFC3261 17.1.2.2)
SipTransactionManager.obj("T4", 5000);

// Timer for INVITE NAT binding refresh
SipTransactionManager.obj("Trefresh", 20000);

SipTransactionManager.fun(
  function executeInviteClientTransaction(request, endpoint, tu, refresh) {    
    var transaction = SipInviteClientTransaction.instantiate();
    transaction.execute(this, this._transport, request, endpoint, tu, refresh);
    //return transaction;
  });

SipTransactionManager.fun(
  function executeNonInviteClientTransaction(request, endpoint, tu) {
    var transaction = SipNonInviteClientTransaction.instantiate();
    transaction.execute(this, this._transport, request, endpoint, tu);
    //return transaction;
  });

SipTransactionManager.fun(
  function executeInviteServerTransaction(request, tu, generate100) {
    var transaction = SipInviteServerTransaction.instantiate();
    transaction.execute(this, this._transport, request, tu, generate100);
    return transaction;
  });

SipTransactionManager.fun(
  function executeNonInviteServerTransaction(request, tu, response) {
    var transaction = SipNonInviteServerTransaction.instantiate();
    transaction.execute(this, this._transport, request, tu, response);
    return transaction;
  });

//  zapISipServerTransaction findCancelledTransaction(in zapISipRequest cancelRequest); 
SipTransactionManager.fun(
  function findCancelledTransaction(cancelRequest) {
    // construct a regular expressions that matches relevant server
    // transaction keys:
    var topVia = cancelRequest.getTopViaHeader();
    var branch = topVia.getParameter("branch");
    var host = topVia.host;
    var port = topVia.port;
    if (!port)
      port = 5060;
    var reText = "$"+branch+";"+host+":"+port+";";
    // search server pool:
    for (var key in this._serverPool) {
      if (key.indexOf(reText,0)==0 &&
          this._serverPool[key].request.method != "CANCEL" &&
          this._serverPool[key].request.method != "ACK")
        return this._serverPool[key];
    }
    return null;
  });

SipTransactionManager.fun(
  function isForkedRequest(request) {
    // Check if this is a request arriving by different paths
    // (RFC3261 8.2.2.2):
    // XXX recode for efficiency.
    
    if (request.getToHeader().getParameter("tag")) return;
    
    var from = request.getFromHeader().getParameter("tag");
    var callID = request.getCallIDHeader().serialize();
    var cseq = request.getCSeqHeader().serialize();
    // search server pool:
    for (var key in this._serverPool) {
      var t = this._serverPool[key];
      if ((t.request.getFromHeader().getParameter("tag") == from) &&
          (t.request.getCallIDHeader().serialize() == callID) &&
          (t.request.getCSeqHeader().serialize() == cseq)) {
        return true;
      }
    }
    return false;
  });

//----------------------------------------------------------------------
// zapISipTransportSink implementation:

SipTransactionManager.fun(
  function handleSipMessage(message, flow) {
    var rv = false;
    
    // Try to match message to a transaction:
    
    if (message.isRequest) {
      // try server transactions
      message.QueryInterface(Components.interfaces.zapISipRequest);
      var key = constructServerTransactionKey(message);
      var transaction;
      if ((transaction = hashget(this._serverPool, key))) {
        transaction.handleRequest(message);
        rv = true;
      }
    }
    else {
      // try client transactions (RFC3261 17.1.3)
      var key = constructClientTransactionKey(message);
      var transaction;
      if ((transaction = hashget(this._clientPool, key))) {
        message.QueryInterface(Components.interfaces.zapISipResponse);
        transaction.handleResponse(message, flow);
        rv = true;
      }
      else {
        this._dump("response "+key+
                   " doesn't match any transaction in client pool:");
        for (var t in this._clientPool)
          this._dump(t);
      }
    }
    
    return rv;
  });

//----------------------------------------------------------------------
// Interface for SipClientTransactions:

SipTransactionManager.fun(
  function registerClientTransaction(transaction) {
    hashset(this._clientPool, transaction.key, transaction);
  });

SipTransactionManager.fun(
  function unregisterClientTransaction(transaction) {
    hashdel(this._clientPool, transaction.key);
  });

SipTransactionManager.fun(
  function registerServerTransaction(transaction) {
    hashset(this._serverPool, transaction.key, transaction);
  });

SipTransactionManager.fun(
  function unregisterServerTransaction(transaction) {
    hashdel(this._serverPool, transaction.key);
  });

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [{ className  : "ZAP SIP Transaction Manager",
     cid        : Components.ID("{8a6d1bad-ef92-4559-9ea1-4176ff630f07}"),
     contractID : "@mozilla.org/zap/siptransactmgr;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipTransactionManager.instantiate(); })
  }]);

