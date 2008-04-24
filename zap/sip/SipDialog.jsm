/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipDialog.jsm', null)" -*- */
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


debug("*** loading SipDialog\n");

EXPORTED_SYMBOLS = [ "constructClientDialogID",
                     "constructServerDialogID",
                     "SipDialog" ];

Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");
Components.utils.import("resource://gre/components/SipUARequestCore.jsm");

// name our global object:
// function toString() { return "[SipDialog.jsm]"; }


////////////////////////////////////////////////////////////////////////
// SipDialog

var SipDialog = makeClass("SipDialog", SupportsImpl, StateMachine);
SipDialog.addInterfaces(Components.interfaces.zapISipDialog);

// init at UAS end
SipDialog.fun(
  function initUAS(stack, request, response) {
    this.stack = stack;

    // initialize state according to RFC3261 12.1.1:
    this.dialogID = constructServerDialogID(response);
    this.callID = request.getCallIDHeader().callID;
    this.localTag = response.getToHeader().getParameter("tag");
    this.remoteTag = request.getFromHeader().getParameter("tag");
    // keep local sequence number uninitialized
    this.remoteSequenceNumber = request.getCSeqHeader().sequenceNumber;
    // we store local/remoteURI as local/remoteAddress, because that's
    // what we'll need for constructing the headers when sending messages:
    this.localAddress = response.getToHeader().address.clone(false)
      .QueryInterface(Components.interfaces.zapISipAddress);
    this.localAddress.displayName = "";
    this.remoteAddress = request.getFromHeader().address.clone(false)
      .QueryInterface(Components.interfaces.zapISipAddress);
    this.remoteAddress.displayName = "";
    this.remoteTarget = request.getTopContactHeader().address.uri;
    this.secure = false; // XXX
    var recordRouteHeaders = request.getHeaders("Record-Route", {});
    if (recordRouteHeaders.length) {
      this._routeSet = [];
      var me = this;
      recordRouteHeaders.forEach(function(rr) {
                                   me._routeSet.push(rr.QueryInterface(Components.interfaces.zapISipRecordRouteHeader).address);
                                 });
    }
    
    this.changeState(response.statusCode[0] == '1' ?
                     "EARLY" :
                     "CONFIRMED");
    this._dump("Dialog created in "+this.currentState+" state");
    // register with stack so that we get messages routed to us:
    stack.registerDialog(this);
  });

// init at UAC end
SipDialog.fun(
  function initUAC(stack, request, response) {
    this.stack = stack;

    // initialize state according to RFC3261 12.1.1:
    this.dialogID = constructClientDialogID(response);
    this.callID = request.getCallIDHeader().callID;
    this.remoteTag = response.getToHeader().getParameter("tag");
    this.localTag = request.getFromHeader().getParameter("tag");
    // keep remote sequence number uninitialized
    this.localSequenceNumber = request.getCSeqHeader().sequenceNumber;
    // we store local/remoteURI as local/remoteAddress, because that's
    // what we'll need for constructing the headers when sending messages:
    this.remoteAddress = response.getToHeader().address.clone(false)
      .QueryInterface(Components.interfaces.zapISipAddress);
    this.remoteAddress.displayName = "";
    this.localAddress = request.getFromHeader().address.clone(false)
      .QueryInterface(Components.interfaces.zapISipAddress);
    this.localAddress.displayName = "";
    this.remoteTarget = response.getTopContactHeader().address.uri;
    this.secure = false; // XXX
    // copy route set in reverse order:
    var recordRouteHeaders = request.getHeaders("Record-Route", {});
    if (recordRouteHeaders.length) {
      this._routeSet = [];
      for (var i=recordRouteHeaders.length-1; i>=0; --i)
        this._routeSet.push(recordRouteHeaders[i].QueryInterface(Components.interfaces.zapISipRecordRouteHeader).address);
    }
    
    this.changeState(response.statusCode[0] == '1' ?
                     "EARLY" :
                     "CONFIRMED");
    this._dump("Dialog created in "+this.currentState+" state");
    // register with stack so that we get messages routed to us:
    stack.registerDialog(this);
  });


// name appearing in debug messages:
SipDialog.getter(
  "_name_",
  function get_name_() {
    return "Dialog "+this.dialogID;
  });

//----------------------------------------------------------------------
// zapISipDialog

//  attribute zapISipDialogListener listener;
SipDialog.obj("listener", null);

//  attribute zapISipRequestHandler requestHandler;
SipDialog.obj("requestHandler", null);

//  void terminateDialog();
SipDialog.fun(
  function terminateDialog() {
    this._dump("Dialog terminated");
    this.stack.unregisterDialog(this);
    this.changeState("TERMINATED");
    if (this.listener) {
      this.listener.notifyDialogTerminated(this);
      this.listener = null;
    }
    else
      this._warning("Dialog "+this.dialogID+" ("+this.currentState+") terminated but no listener. Backtrace: "+this._backtrace());
  });    

//  zapISipNonInviteRC createNonInviteRequestClient(in ACString method);
SipDialog.fun(
  function createNonInviteRequestClient(method) {
    var rc = SipNonInviteRC.instantiate();
    var request = this.formulateGenericRequest(method);
    rc.init(this.stack, this, request, 0);
    return rc;
  });

//  zapISipInviteRC createInviteRequestClient();
SipDialog.fun(
  function createInviteRequestClient() {
    var rc = SipInviteRC.instantiate();
    var request = this.formulateGenericRequest("INVITE");
    // add mandatory Contact header (rfc3261 8.1.1.8):
    request.appendHeader(gSyntaxFactory.createContactHeader(this.stack.getContactAddress()));    
    rc.init(this.stack, this, request, 0);
  });
  
//  readonly attribute ACString dialogID;
SipDialog.obj("dialogID", null);

//  readonly attribute ACString callID;
SipDialog.obj("callID", null);

//  readonly attribute ACString localTag;
SipDialog.obj("localTag", null);

//  readonly attribute ACString remoteTag;
SipDialog.obj("remoteTag", null);

//  readonly attribute unsigned long localSequenceNumber;
SipDialog.obj("localSequenceNumber",
              Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER);

//  readonly attribute unsigned long remoteSequenceNumber;
SipDialog.obj("remoteSequenceNumber",
              Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER);

//  readonly attribute zapISipURI localURI
SipDialog.getter(
  "localURI",
  function get_localURI() {
    return this.localAddress.uri;
  });

//  readonly attribute zapISipURI remoteURI;
SipDialog.getter(
  "remoteURI",
  function get_remoteURI() {
    return this.remoteAddress.uri;
  });

//  readonly attribute zapISipURI remoteTarget;
SipDialog.obj("remoteTarget", null);

//  readonly attribute boolean secure;
SipDialog.obj("secure", false);

//  void getRouteSet(out unsigned long count,
//                   [array, retval, size_is(count)] out
//                   zapISipAddress routeset);
SipDialog.obj("_routeSet", []);
SipDialog.fun(
  function getRouteSet(count) {
    if (count)
      count.value = this._routeSet.length;
    return this._routeSet;
  });

//  readonly attribute ACString dialogState;
SipDialog.getter(
  "dialogState",
  function get_dialogState() {
    return this.currentState;
  });

//----------------------------------------------------------------------
// interface to stack:

SipDialog.fun(
  function handleRequest(request) {
    // check if this is an ACK targeted at a pendingInviteRS:
    if (request.method == "ACK") {
      if (this.pendingInviteRS)
        this.pendingInviteRS.handleACK(this, request);
      // else silently drop
      return;
    }
    
    // Check sequence numbering (RFC3261 12.2.2):
    var seq = request.getCSeqHeader().sequenceNumber;
    if (this.remoteSequenceNumber != Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER &&
        seq <= this.remoteSequenceNumber) {
      // An out of order request.
      // Send a 500 (Server Internal Error):
      var response = this.stack.formulateResponse("500", request, null);
      this.stack.sendTransactionalResponse(response, request);
      return;
    }

    // Set new sequence number:
    this.remoteSequenceNumber = seq;
      
    // Check for concurrent INVITES (RFC3261 14.2):
    if (request.method == "INVITE") {
      if (this.pendingInviteRS) {
        // Send a 500 (Server Internal Error):
        var response = this.stack.formulateResponse("500", request, null);
        // append a Retry-After header with a randomly chosen value of
        // between 0 and 10 seconds:
        // XXX
        this.stack.sendTransactionalResponse(response, request);
      }
      else if (this.pendingInviteRC) {
        // Send a 491 (Request pending):
        var response = this.stack.formulateResponse("491", request, null);
        this.stack.sendTransactionalResponse(response, request);
      }
      else {
        var rs = SipInviteRS.instantiate();
        rs.init(this.stack, this, request);
        // pass to handler, then to UA application component handler,
        // then to UAStack if not handled:
        if (!this.requestHandler ||
            !this.requestHandler.handleInviteRequest(rs)) {
          if (!this.stack.requestHandler ||
              !this.stack.requestHandler.handleInviteRequest(rs))
            this.stack.handleInviteRequest(rs);
        }
      }
    }
    else {
      var rs = SipNonInviteRS.instantiate();
      rs.init(this.stack, this, request);
        // pass to handler, then to UA application component handler,
        // then to UAStack if not handled:
      if (!this.requestHandler ||
          !this.requestHandler.handleNonInviteRequest(rs)) {
        if (!this.stack.requestHandler ||
            !this.stack.requestHandler.handleNonInviteRequest(rs))
          this.stack.handleNonInviteRequest(rs);
      }
    }
  });

//----------------------------------------------------------------------
// interface to request servers/clients:

// request servers and clients will set/remove references to them
// here, so that we can ensure that only one invite is pending at any
// one time (RFC3261 14.1):
SipDialog.obj("pendingInviteRS", null);
SipDialog.obj("pendingInviteRC", null);

SipDialog.statefun(
  "EARLY",
  function confirmUAS(response) {
    this._dump("Dialog confirmed");
    this.changeState("CONFIRMED");
  });

SipDialog.statefun(
  "EARLY",
  function confirmUAC(response) {
    // recompute route set RFC3261 13.2.2.4
    // We're at the UAC end, so need to copy in reverse order:
    this._routeSet = [];
    var recordRouteHeaders = response.getHeaders("Record-Route", {});
    if (recordRouteHeaders.length) {
      for (var i=recordRouteHeaders.length-1; i>=0; --i)
        this._routeSet.push(recordRouteHeaders[i].QueryInterface(Components.interfaces.zapISipRecordRouteHeader).address);
    }
    
    this._dump("Dialog confirmed");
    this.changeState("CONFIRMED");
  });

SipDialog.fun(
  function formulateACK() {
    return this.formulateGenericRequest("ACK");
  });

//----------------------------------------------------------------------
// Implementation helpers:

SipDialog.fun(
  function formulateGenericRequest(method) {
    // construct a generic (non-invite) request following RFC3261 12.2.1.1
    var m = gSyntaxFactory.createRequest();
    // method:
    m.method = method;
    
    // request uri & route set:
    if (this._routeSet.length) {
      // we have a pre-existing route. 
      var lr = this._routeSet[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("lr");
      var i = 0;
      if (lr) {
        // We have a loose router. Set requestURI to remote target and
        // make sure we construct route headers for all proxies in the
        // route set:
        m.requestURI = this.remoteTarget;
      }
      else {
        // We have a strict router. Set requestURI to first URI in
        // route set, construct route headers for all other proxies
        // and append remoteTarget:
        m.requestURI = this._routeSet[0].uri;
        i = 1;
      }
      // construct route headers:
      for ( var l = this._routeSet.length; i<l; ++i) {
        m.appendHeader(gSyntaxFactory.createRouteHeader(this._routeSet[i]));
      }
      if (!lr) {
        // Append route header with remoteTarget.
        var remoteAddress = gSyntaxFactory.createAddress("", this.remoteTarget);
        m.appendHeader(gSyntaxFactory.createRouteHeader(remoteAddress));
      }
    }
    else {
      m.requestURI = this.remoteTarget;
    }

    // Via header:
    var viaHeader = gSyntaxFactory.createViaHeader();
    if (this.stack.rport_client)
      viaHeader.setParameter("rport", "");
    m.appendHeader(viaHeader);    
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
    // Note: will be updated on sending
    m.appendHeader(gSyntaxFactory.createCSeqHeader(method,
                                                   Components.interfaces.zapISipDialog.UNINITIALIZED_SEQUENCE_NUMBER));
    // Max-Forwards header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());
    // User-Agent header:
    if (this.stack.userAgent)
      m.appendHeader(gSyntaxFactory.deserializeHeader("User-Agent", this.stack.userAgent));
    // Supported headers:
    if (method != "ACK") {
      this.stack.supportedExtensions.forEach(
        function(opt) {
          m.appendHeader(gSyntaxFactory.deserializeHeader("Supported", opt));
        });
    }
    
    return m;
  });
