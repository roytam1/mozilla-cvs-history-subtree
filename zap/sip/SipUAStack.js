/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipUAStack.js', null)" -*- */
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

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");
Components.utils.import("resource://gre/components/SipUARequestCore.jsm");
Components.utils.import("resource://gre/components/SipDialog.jsm");
Components.utils.import("resource://gre/components/FunctionUtils.jsm");

// name our global object:
// function toString() { return "[SipUAStack.js]"; }

////////////////////////////////////////////////////////////////////////
// globals

//----------------------------------------------------------------------
// resolver

var getResolver = makeServiceGetter("@mozilla.org/zap/sipresolver;1",
                                    Components.interfaces.zapISipResolver);

//----------------------------------------------------------------------
// authentication

var getAuthentication = makeServiceGetter("@mozilla.org/zap/sipauth;1",
                                          Components.interfaces.zapISipAuthentication);

//----------------------------------------------------------------------
// sip transport

function createSipTransport() {
  // create a sip transport object via xpcom (more overhead):
  // return Components.classes["@mozilla.org/zap/siptransport;1"].createInstance().QueryInterface(Components.interfaces.zapISipTransport);

  // create a sip transport object directly from js (less overhead, no
  // type safety, access to non-xpcom interface):
  return Components.utils.import("resource://gre/components/SipTransport.js", null).SipTransport.instantiate();
}

//----------------------------------------------------------------------
// sip transaction manager

function createSipTransactionManager() {
  // create a sip transaction manager via xpcom (more overhead):
  // return Components.classes["@mozilla.org/zap/siptransactmgr"].createInstance().QueryInterface(Components.interfaces.zapISipTransactionManager);

  // create a sip transaction manager directly from js (less overhead,
  // no type safety, access to non-xpcom interface):
  return Components.utils.import("resource://gre/components/SipTransactions.js", null).SipTransactionManager.instantiate();
}

//----------------------------------------------------------------------
// Helpers

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP UA", level, mes);
}


////////////////////////////////////////////////////////////////////////
// SipUAStack


var SipUAStack = makeClass("SipUAStack", SupportsImpl, Unwrappable);
SipUAStack.addInterfaces(Components.interfaces.zapISipUAStack,
                         Components.interfaces.zapISipTransportSink);

SipUAStack.appendCtor(
  function ctor() {
    // Pool of active dialogs, indexed by dialog id:
    this.dialogPool = {};
    // Pool of active invite client servers (for response routing of
    // 2XX responses after the client transaction has terminated):
    this.inviteRCPool = {};
  });

//----------------------------------------------------------------------
// zapISipUAStack

//  void init(in zapISipRequestHandler handler,
//            in nsIPropertyBag2 stackConfiguration);
SipUAStack.fun(
  function init(handler, config) {
    this._dump("Building SIP User Agent Stack...");

    this.FromAddress = gSyntaxFactory.deserializeAddress('Anonymous <sip:thisis@anonymous.invalid>');
    
    this.requestHandler = handler;

    // Unpack configuration parameters:
    var instance_id = "";
    // port_base will be unpacked by transport
    var methods = "OPTIONS";
    var extensions = "";
    this.rport_client = true;
    this.shortBranchParameters = false;
    this.userAgent = "";
    if (config) {
      try { instance_id = config.getProperty("instance_id"); }catch(e){}
      try { methods = config.getProperty("methods"); }catch(e){}
      try { extensions = config.getProperty("extensions"); }catch(e){}
      try { this.rport_client = config.getProperty("rport_client"); }catch(e){}
      try { this.shortBranchParameters = config.getProperty("short_branch_parameters"); }catch(e){}
      try { this.userAgent = config.getProperty("user_agent"); }catch(e){}
    }
    this.instanceID = instance_id ? instance_id : gUUIDGenerator.generateUUIDURNString();
    this.allowedMethods = methods.length ? methods.split(",") : [];
    this.supportedExtensions = extensions.length ? extensions.split(",") : [];
    
    // Create transport layer:
    this.transport = createSipTransport();
    this.transport.init(config);
    
    // Create transaction manager:
    this.transactionManager = createSipTransactionManager();

    // Hook up transport to transactionmanager:
    this.transactionManager.init(this.transport);
    
    // Add the transaction manager as the first transport sink so that
    // it gets first refusal at handling new messages:
    this.transport.appendTransportSink(this.transactionManager);    
    
    // Add ourselves as 'last-stop' transport sink:
    this.transport.appendTransportSink(this);

    // Let the transport know about the sip ua stack, so that
    // it can use transactions for flow monitoring:
    this.transport.UAStack = this;

    this._dump("SIP User Agent Stack built.");    
  });
  
//  void shutdown();
SipUAStack.fun(
  function shutdown() {
    this.transport.shutdown();
  });

//  attribute zapISipRequestHandler requestHandler;
SipUAStack.obj("requestHandler", null);

//  zapISipNonInviteRC createNonInviteRequestClient(in zapISipAddress ToAddress, in zapSSipAddress FromAddress, in ACString method, in unsigned long rcFlags);
SipUAStack.fun(
  function createNonInviteRequestClient(ToAddress, FromAddress, method,
                                        routeset, count, rcFlags) {
    var rc = SipNonInviteRC.instantiate();
    var request = this.formulateGenericRequest(method, ToAddress.uri,
                                               ToAddress, FromAddress,
                                               routeset, count);
    rc.init(this, null, request, rcFlags);
    return rc;
  });


// zapISipNonInviteRC createRegisterRequestClient(in zapISipURI domain,
//                                                in zapISipURI addressOfRecord,
//                                                in zapISipAddress contactAddress,
//                                                [array, size_is(count)]
//                                                in zapISipAddress routeset,
//                                                in unsigned long count,
//                                                in unsigned long rcFlags);
SipUAStack.fun(
  function createRegisterRequestClient(domain, addressOfRecord, contactAddress,
                                       routeset, count, rcFlags) {
    var rc = SipNonInviteRC.instantiate();
    var addressOfRecordAddr = gSyntaxFactory.createAddress("", addressOfRecord);
    var request = this.formulateGenericRequest("REGISTER", domain,
                                               addressOfRecordAddr,
                                               addressOfRecordAddr,
                                               routeset, count);
    // add Contact header (RFC3261 10.2):
    var contact = gSyntaxFactory.createContactHeader(contactAddress);
    request.appendHeader(contact);
    rc.init(this, null, request, rcFlags);
    return rc;
  });


//  zapISipInviteRC createInviteRequestClient(in zapISipAddress ToAddress, in zapISipAddress FromAddress, in zapISipAddress ContactAddress, [array, size_is(count)] in zapISipAddress routeset, in unsigned long count, in unsigned long rcFlags);
SipUAStack.fun(
  function createInviteRequestClient(ToAddress, FromAddress, contactAddress,
                                     routeset, count, rcFlags) {
    var rc = SipInviteRC.instantiate();

    var uri = ToAddress.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
    var requestURI = uri.clone(true);
    uri.port = "";
    uri.removeURIParameter("maddr");
    uri.removeURIParameter("ttl");
    uri.removeURIParameter("transport");
    uri.removeURIParameter("lr");

    var request = this.formulateGenericRequest("INVITE", requestURI,
                                               ToAddress, FromAddress,
                                               routeset, count);
    // add mandatory Contact header (rfc3261 8.1.1.8):
    request.appendHeader(gSyntaxFactory.createContactHeader(contactAddress));
    rc.init(this, null, request, rcFlags);
    return rc;
  });

//   zapISipSubscribeRC createSubscribeRequestClient(in zapISipAddress ToAddress,
//                                                   in zapISipAddress FromAddress,
//                                                   in zapISipAddress contactAddress,
//                                                   in ACString eventType,
//                                                   [array, size_is(count)]
//                                                   in zapISipAddress routeset,
//                                                   in unsigned long count,
//                                                   in unsigned long rcFlags);
SipUAStack.fun(
  function createSubscribeRequestClient(ToAddress, FromAddress,
                                        contactAddress, eventType,
                                        routeset, count, rcFlags) {
    var rc = SipSubscribeRC.instantiate();
    var request = this.formulateGenericRequest("SUBSCRIBE", ToAddress.uri,
                                               ToAddress, FromAddress,
                                               routeset, count);
    // add mandatory Contact header (RFC3265 7.1):
    request.appendHeader(gSyntaxFactory.createContactHeader(contactAddress));
    // add mandatory Event header (RFC3265 3.1.2, 7.1):
    request.appendHeader(gSyntaxFactory.deserializeHeader("Event", eventType));
    rc.init(this, request, rcFlags);
    return rc;
  });

SipUAStack.fun(
  function formulateResponse(statusCode, request, toTag) {
    var m = gSyntaxFactory.createResponse();
    // set status code
    m.statusCode = statusCode;
    // add standard reason phrase
    m.reasonPhrase = gSyntaxFactory.getStandardReasonPhrase(statusCode);
    // copy Via headers:
    request.getHeaders("Via", {}).forEach(function(v) { m.appendHeader(v); });
    // copy From:
    m.appendHeader(request.getFromHeader());
    // copy To:
    var toHeader = request.getToHeader().clone(false).QueryInterface(Components.interfaces.zapISipToHeader);
    m.appendHeader(toHeader);
    // maybe add tag:
    var tag = toHeader.getParameter("tag");
    if (!tag && statusCode != "100" && !toTag) {
      toHeader.setParameter("tag", generateTag());
    }
    else if (toTag) {
      if (!tag) {
        toHeader.setParameter("tag", toTag);
      }
      else {
        this._assert(toTag == tag, 
                     "already have a different To tag in "+statusCode+ " response to "+request.method+"  :: CALLSTACK ::\n"+this._backtrace());
      }
    }
    // copy Call-ID:
    m.appendHeader(request.getCallIDHeader());
    // copy CSeq:
    m.appendHeader(request.getCSeqHeader());

    // for 405 add Allow headers:
    if (statusCode == "405") this.appendAllowHeaders(m);

    // for 2XX add Supported headers:
    if (statusCode[0] == "2") {
      this.supportedExtensions.forEach(
        function(opt) {
          m.appendHeader(gSyntaxFactory.deserializeHeader("Supported", opt));
        });
    }
    
    return m;
  });

SipUAStack.fun(
  function appendAllowHeaders(message) {
    this.allowedMethods.forEach(
      function(m) {
        message.appendHeader(gSyntaxFactory.createAllowHeader(m));
      });
  });

//  readonly attribute zapISipTransactionManager transactionManager;
SipUAStack.obj("transactionManager", null);

//  readonly attribute zapISipTransport transport;
SipUAStack.obj("transport", null);

//  readonly attribute zapISipSyntaxFactory syntaxFactory;
SipUAStack.obj("syntaxFactory", gSyntaxFactory);

//  readonly attribute zapISipResolver resolver;
SipUAStack.getter("resolver", getResolver);

//  readonly attribute zapISipAuthentication authentication;
SipUAStack.getter("authentication", getAuthentication);

//  readonly attribute ACString hostName;
SipUAStack.getter(
  "hostName",
  function get_hostName() {
    return getDNSService().myHostName;
  });

//  readonly attribute ACString hostAddress;
// XXX this field is problematic (there is no one host address) and should really go away
SipUAStack.getter(
  "hostAddress",
  function get_hostAddress() {
    try {
      var record = getDNSService().resolve(getDNSService().myHostName, 0);
      if (record.hasMore())
        return record.getNextAddrAsString();
    } catch(e) {
      this._dump("exception resolving our host: "+e);
    }
    return "127.0.0.1";
  });

//  readonly attribute unsigned short listeningPort;
SipUAStack.getter(
  "listeningPort",
  function get_listeningPort() {
    return this.transport.listeningPort;
  });

//  readonly attribute ACString instanceID;
SipUAStack.obj("instanceID", null);

//  readonly attribute AUTF8String userAgent;
SipUAStack.obj("userAgent", null);

//  attribute boolean shortBranchParameters;
SipUAStack.obj("shortBranchParameters", false);

//----------------------------------------------------------------------
// zapISipTransportSink implementation:

SipUAStack.fun(
  function handleSipMessage(message, flow) {
    if (message.isRequest) {
      // We have received a request.      
      message.QueryInterface(Components.interfaces.zapISipRequest);
      log(message.method+" request received");

      // Check if this is a request arriving by different paths
      // (RFC3261 8.2.2.2):
      if (this.transactionManager.isForkedRequest(message)) {
        // reject with 482 (Loop Detected)
        var response = this.formulateResponse("482", message, null);
        this.sendTransactionalResponse(response, message);
        return true;
      }
      
      // Match against supported extensions (RFC3261 8.2.2.3):
      var reqHeaders = message.getHeaders("Require", {});
      if (reqHeaders.length) {
        var unknowns = [];
        var me = this;
        reqHeaders.forEach(
          function(h) {
            h = h.QueryInterface(Components.interfaces.zapISipRequireHeader);
            if (!member(h.optionTag, me.supportedExtensions))
              unknowns.push(h.optionTag);
          });
        if (unknowns.length) {
          // We have unsupported extensions.
          // Generate a 420 (Bad Extension)
          var response = this.formulateResponse("420", message, null);
          // add Unsupported headers for all unknown extensions:
          unknowns.forEach(
            function(t) {
              response.appendHeader(gSyntaxFactory.createUnsupportedHeader(t));
            });
          this.sendTransactionalResponse(response, message);
          return true;
        }
      }
      
      // Check if we should dispatch to a dialog:
      if (message.getToHeader().getParameter("tag")) {
        var dialog = this.findDialog(constructServerDialogID(message));
        if (dialog)
          dialog.handleRequest(message);
        else if (message.method != "ACK") {
          // The dialog does not exist. Reject with a 481
          // (Call/Transaction Does Not Exist). (RFC3261 12.2.2)
          // But only if it's not an ACK!

          this._dump("no dialog found for "+constructServerDialogID(message));
          this._dump("dialog pool:");
          me = this;
          hashmap(this.dialogPool, function(n, o) { me._dump(n); });
          
          var response = this.formulateResponse("481", message, null);
          this.sendTransactionalResponse(response, message);
        }
      }
      else {
        // The request is outside of a dialog.
        // Create a new request server and dispatch to handler:
        if (message.method == "INVITE") {
          var rs = SipInviteRS.instantiate();
          rs.init(this, null, message);
          // pass along the handler chain:
          // UA client request handler --> UA Stack request handler
          try {
            if (!this.requestHandler ||
                !this.requestHandler.handleInviteRequest(rs))
              this.handleInviteRequest(rs);
          }
          catch(e) {
            this._dump("Error handling request: "+e);
            // -> 500 (Server Internal Error)
            var response = rs.formulateResponse("500");
            rs.sendResponse(response);
          }
        }
        else {
          var rs = SipNonInviteRS.instantiate();
          rs.init(this, null, message);
          // pass along the handler chain:
          // UA client request handler --> UA Stack request handler
          try {
            if (!this.requestHandler ||
                !this.requestHandler.handleNonInviteRequest(rs))
              this.handleNonInviteRequest(rs);
          }
          catch(e) {
            this._dump("Error handling request: "+e);
            // -> 500 (Server Internal Error)
            var response = rs.formulateResponse("500");
            rs.sendResponse(response);
          }
        }
      }
    }
    else {
      // We have received a response.
      message.QueryInterface(Components.interfaces.zapISipResponse);

      // Try to match to a pending invite request client:
      var rc = this.findInviteRC(constructClientTransactionKey(message));
      if (rc)
        rc.handleResponse(message, flow);
      else {
        // nope, this must be a stray response
        this._dump("Stray response (" + message.statusCode + " --- "+
                   message.reasonPhrase + ")");
      }
    }

    // true == message handled
    return true;
  });

//----------------------------------------------------------------------
// Default UA Stack request handlers:
// These are the 'last-stop' request handlers; called after any UA
// client or dialog handlers have had a chance to handle the request

SipUAStack.fun(
  function handleNonInviteRequest(rs) {
    var response;
    var method = rs.request.method;

    // always silently drop ACK:
    if (method == "ACK") return;
    
    if (member(method, this.allowedMethods)) {
      if (method == "OPTIONS") {
        response = rs.formulateResponse("200");
        this.appendAllowHeaders(response);
      }
      else if (method == "BYE") {
        // RFC3261 15.1.2
        if (rs.dialog) {
          response = rs.formulateResponse("200");
          if (rs.dialog.dialogState != "TERMINATED")
            rs.dialog.terminateDialog();
        }
        else {
          this._dump("BYE request with dialog id "+
                     constructServerDialogID(rs.request)+
                     " doesn't match any dialog in pool:");
          for (var d in this.dialogPool)
            this._dump(d);
          response = rs.formulateResponse("481");
        }
      }
      else if (method == "CANCEL") {
        // RFC3261 9.2
        // try to match the request to an ongoing server transaction:
        var transaction = this.transactionManager.findCancelledTransaction(rs.request);
        if (!transaction) {
          response = rs.formulateResponse("481");
        }
        else {
          response = rs.formulateResponse("200");
          
          if (transaction.request.method == "INVITE") {
            // XXX this is maybe a bit dodgy. we shouldn't have to use
            // wrappedJSObject to locate the server.
            
            // locate the invite server. This better be a SipInviteRS.
            var irs = transaction.transactionUser.wrappedJSObject;
            if(!irs || !irs.cancel)
              this._warning("can only cancel built-in invite request server");
            else
              irs.cancel();
          }
        }
      }
      else {
        // The UA handler should have really handled this!
        // -> 500 (Server Internal Error)
        this._dump("unhandled method "+method);
        response = rs.formulateResponse("500");
      }
    }
    else {
      // 405 (Method Not Allowed)
      response = rs.formulateResponse("405");
    }
    
    rs.sendResponse(response);
  });

SipUAStack.fun(
  function handleInviteRequest(rs) {
    this._assert(!rs.dialog, "re-invite should have been handled by dialog request handler");

    var code;
    if (member(rs.request.method, this.allowedMethods)) {
      // The UA handler should have really handled this!
      // -> 500 (Server Internal Error)
      this._dump("unhandled method "+rs.request.method);
      code = "500";
    } else {
      // Reject invites with a 405 (Method Not Allowed):
      code = "405";
    }
    
    var response = rs.formulateResponse(code);
    rs.sendResponse(response);
  });

//----------------------------------------------------------------------
// Interface to request servers/clients:

// create a dialog at UAS end:
SipUAStack.fun(
  function createDialogUAS(request, response) {
    var dialog = SipDialog.instantiate();
    try {
      dialog.initUAS(this, request, response);
    }
    catch(e) {
      this._dump("Error creating dialog: "+e);
      return null;
    }
    return dialog;
  });

// create a dialog at UAC end:
SipUAStack.fun(
  function createDialogUAC(request, response) {
    var dialog = SipDialog.instantiate();
    try {
      dialog.initUAC(this, request, response);
    }
    catch(e) {
      this._dump("Error creating dialog: "+e);
      return null;
    }
    return dialog;
  });

//----------------------------------------------------------------------
// Dialog management:

// newly created dialogs will call this to register themselves with
// the stack:
SipUAStack.fun(
  function registerDialog(dialog) {
    hashset(this.dialogPool, dialog.dialogID, dialog);
  });

// terminated dialogs will call this to unregister:
SipUAStack.fun(
  function unregisterDialog(dialog) {
    hashdel(this.dialogPool, dialog.dialogID);
  });

SipUAStack.fun(
  function findDialog(id) {
    return hashget(this.dialogPool, id);
  });

//----------------------------------------------------------------------
// Invite request client management:

// invite request clients will call this to register themselves with
// the stack after they have received their first 2XX response, so
// that subsequent 2XX can be routed to them:
SipUAStack.fun(
  function registerInviteRC(rc, transactionKey) {
    hashset(this.inviteRCPool, transactionKey, rc);
  });

// terminated invite request clients will call this to unregister:
SipUAStack.fun(
  function unregisterInviteRC(transactionKey) {
    hashdel(this.inviteRCPool, transactionKey);
  });

SipUAStack.fun(
  function findInviteRC(transactionKey) {
    return hashget(this.inviteRCPool, transactionKey);
  });

//----------------------------------------------------------------------
// Implementation helpers:

// Send a response to the given request via a server invite or
// non-invite transaction, as appropriate for the request method.
SipUAStack.fun(
  function sendTransactionalResponse(response, request) {
    if (request.method == "INVITE") {
      var tx = this.transactionManager.executeInviteServerTransaction(request,
                                                                      null,
                                                                      false);
      tx.sendResponse(response);
    }
    else {
      this.transactionManager.executeNonInviteServerTransaction(request,
                                                                null,
                                                                response);
    }
  });

  
// formulate a request following RFC3261 8.1.1:
SipUAStack.fun(
  function formulateGenericRequest(method, remoteTarget,
                                   ToAddress, FromAddress,
                                   routeset, count) {
    var m = gSyntaxFactory.createRequest();
    // method:
    m.method = method;
    // requestURI & route set:
    if (routeset.length) {
      // we have a pre-existing route. Follow the procedures in
      // RFC3261 12.2.1.1 for setting the Request-URI and Route
      // headers:
      var lr = routeset[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("lr");
      var i = 0;
      if (lr) {
        // We have a loose router. Set requestURI to remoteTarget and
        // make sure we construct route headers for all proxies in the
        // route set:
        m.requestURI = remoteTarget;
      }
      else {
        // We have a strict router. Set requestURI to first URI in
        // route set, construct route headers for all other proxies
        // and append remoteTarget:
        m.requestURI = routeset[0].uri;
        i = 1;
      }
      // construct route headers:
      for ( var l = routeset.length; i<l; ++i) {
        m.appendHeader(gSyntaxFactory.createRouteHeader(routeset[i]));
      }
      if (!lr) {
        // Append route header with remoteTarget
        var remoteAddress = gSyntaxFactory.createAddress("", remoteTarget);
        m.appendHeader(gSyntaxFactory.createRouteHeader(remoteAddress));
        
      }
    }
    else {
      m.requestURI = remoteTarget;
    }
    
    // Via header:
    var viaHeader = gSyntaxFactory.createViaHeader(); 
    if (this.rport_client)
      viaHeader.setParameter("rport", "");
    m.appendHeader(viaHeader);
    // To header:
    m.appendHeader(gSyntaxFactory.createToHeader(ToAddress));
    // From header + new tag:
    var fromHeader = gSyntaxFactory.createFromHeader(FromAddress);
    fromHeader.setParameter("tag", generateTag());
    m.appendHeader(fromHeader);
    // Call-ID header with new call id:
    var callIDHeader =
      gSyntaxFactory.createCallIDHeader(gUUIDGenerator.generateUUIDString() +
                                        "@" +
                                        this.hostName);
    m.appendHeader(callIDHeader);
    // CSeq header matching the request method and containing a new
    // sequence number:
    m.appendHeader(gSyntaxFactory.createCSeqHeader(method, 1));
    // Max-Forwards header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());
    // User-Agent header:
    if (this.userAgent)
      m.appendHeader(gSyntaxFactory.deserializeHeader("User-Agent", this.userAgent));
    // Supported headers:
    if (method != "ACK") {
      this.supportedExtensions.forEach(
        function(opt) {
          m.appendHeader(gSyntaxFactory.deserializeHeader("Supported", opt));
        });
    }
    
    return m;
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [{ className  : "ZAP SIP User Agent Stack",
     cid        : Components.ID("{7377a41d-7bee-4729-8803-0b32218c1a9e}"),
     contractID : "@mozilla.org/zap/sipstack;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipUAStack.instantiate(); })
  }]);

