/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SipUAStack.js')" -*- */
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

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/SipUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/SipUARequestCore.js");
Components.utils.importModule("resource:/jscodelib/zap/SipDialog.js");

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
  return Components.classes["@mozilla.org/moz/jsloader;1"].getService(Components.interfaces.xpcIJSComponentLoader).importModule("rel:SipTransport.js").SipTransport.instantiate();
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
  return Components.classes["@mozilla.org/moz/jsloader;1"].getService(Components.interfaces.xpcIJSComponentLoader).importModule("rel:SipTransactions.js").SipTransactionManager.instantiate();
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

    this.FromAddress = gSyntaxFactory.deserializeAddress("sip:thisis@anonymous.invalid");
    
    this.requestHandler = handler;

    // Unpack configuration parameters:
    var methods = "OPTIONS";
    var extensions = "";
    this.rport_client = true;
    if (config) {
      try { methods = config.getProperty("methods"); }catch(e){}
      try { extensions = config.getProperty("extensions"); }catch(e){}
      try { rport_client = config.getProperty("rport_client"); }catch(e){}
    }
    this.allowedMethods = methods.split(",");
    this.supportedExtensions = extensions.split(",");
    
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

    this._dump("SIP User Agent Stack built.");    
  });
  
//  void shutdown();
SipUAStack.fun(
  function shutdown() {
    this.transport.shutdown();
  });

// attribute zapISipAddress FromAddress;
SipUAStack.obj("FromAddress", null);

//  attribute zapISipRequestHandler requestHandler;
SipUAStack.obj("requestHandler", null);

//  zapISipNonInviteRC createNonInviteRequestClient(in zapISipAddress ToAddress);
SipUAStack.fun(
  function createNonInviteRequestClient(ToAddress) {
    var rc = SipNonInviteRC.instantiate();
    rc.init(this, null, ToAddress);
    return rc;
  });

//  zapISipInviteRC createInviteRequestClient(in zapISipAddress ToAddress);
SipUAStack.fun(
  function createInviteRequestClient(ToAddress) {
    var rc = SipInviteRC.instantiate();
    rc.init(this, null, ToAddress);
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
    var toHeader = request.getToHeader().clone().QueryInterface(Components.interfaces.zapISipToHeader);
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
SipUAStack.obj("resolver", gResolver);

//  readonly attribute ACString hostName;
SipUAStack.getter(
  "hostName",
  function get_hostName() {
    return gDNSService.myHostName;
  });

//  readonly attribute ACString hostAddress;
SipUAStack.getter(
  "hostAddress",
  function get_hostAddress() {
    try {
      return gDNSService.resolve(gDNSService.myHostName,0).getNextAddrAsString();
    } catch(e) {
      return "127.0.0.1";
    }
  });

//  readonly attribute unsigned short listeningPort;
SipUAStack.getter(
  "listeningPort",
  function get_listeningPort() {
    return this.transport.listeningPort;
  });

//----------------------------------------------------------------------
// zapISipTransportSink implementation:

SipUAStack.fun(
  function handleSipMessage(message, endpoint, connection) {
    if (message.isRequest) {
      // We have received a request.      
      message.QueryInterface(Components.interfaces.zapISipRequest);
      log(message.method+" request received");

      // Match against supported extensions (RFC3261 8.2.2.3):
      var reqHeaders = message.getHeaders("Require", {});
      if (reqHeaders.length) {
        var unknowns = [];
        reqHeaders.forEach(
          function(h) {
            h = h.QueryInterface(Components.interfaces.zapISipRequireHeader);
            if (!member(h.optionTag, this.supportedExtensions))
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
        }
      }
      
      // Check if we should dispatch to a dialog:
      if (message.getToHeader().getParameter("tag")) {
        var dialog = this.findDialog(constructServerDialogID(message));
        if (dialog)
          dialog.handleRequest(message);
        else {
          // The dialog does not exist. Reject with a 481
          // (Call/Transaction Does Not Exist). (RFC3261 12.2.2)
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
          if (!this.requestHandler ||
              !this.requestHandler.handleInviteRequest(rs))
            this.handleInviteRequest(rs);
        }
        else {
          var rs = SipNonInviteRS.instantiate();
          rs.init(this, null, message);
          // pass along the handler chain:
          // UA client request handler --> UA Stack request handler
          if (!this.requestHandler ||
              !this.requestHandler.handleNonInviteRequest(rs))
            this.handleNonInviteRequest(rs);
        }
      }
    }
    else {
      // We have received a response.
      message.QueryInterface(Components.interfaces.zapISipResponse);

      // Try to match to a pending invite request client:
      var rs = this.findInviteRC(constructClientTransactionKey(message));
      if (rs)
        rs.handleResponse(message);
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
        if (rs.dialog)
          response = rs.formulateResponse("200");
        else 
          response = rs.formulateResponse("481");
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

SipUAStack.fun(
  function formulateGenericRequest(method, ToAddress) {
    // XXX handle RouteSet

    var m = gSyntaxFactory.createRequest();
    // method:
    m.method = method;
    // requestURI:
    m.requestURI = ToAddress.uri;
    // Via header:
    var viaHeader = gSyntaxFactory.createViaHeader(); 
    if (this.rport_client)
      viaHeader.setParameter("rport", "");
    m.appendHeader(viaHeader);
    // To header:
    m.appendHeader(gSyntaxFactory.createToHeader(ToAddress));
    // From header + new tag:
    var fromHeader = gSyntaxFactory.createFromHeader(this.FromAddress);
    fromHeader.setParameter("tag", generateTag());
    m.appendHeader(fromHeader);
    // Call-ID header with new call id:
    var callIDHeader =
      gSyntaxFactory.createCallIDHeader(generateUUID() + "@" +
                                        this.hostName);
    m.appendHeader(callIDHeader);
    // CSeq header matching the request method and containing a new
    // sequence number:
    m.appendHeader(gSyntaxFactory.createCSeqHeader(method, 1));
    // Max-Forwards header:
    m.appendHeader(gSyntaxFactory.createMaxForwardsHeader());

    return m;
  });

// get the address to use for an Contact header for a
// dialog-establishing message:
// XXX this should be user configurable in some form or another
// XXX get fqdn from elsewhere?
// XXX should this also be the contact set for non-dialog establising requests?
// XXX distinguish between sip and sips
SipUAStack.fun(
  function getContactAddress() {
    var user = (this.FromAddress.uri).QueryInterface(Components.interfaces.zapISipSIPURI).userinfo;
    var host = this.hostAddress;
    return gSyntaxFactory.deserializeAddress("sip:"+user+"@"+host);
  });

// create a dialog at UAS end:
SipUAStack.fun(
  function createDialogUAS(request, response) {
    var dialog = SipDialog.instantiate();
    dialog.initUAS(this, request, response);
    return dialog;
  });

// create a dialog at UAC end:
SipUAStack.fun(
  function createDialogUAC(request, response) {
    var dialog = SipDialog.instantiate();
    dialog.initUAC(this, request, response);
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

  


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP SIP User Agent Stack",
     cid        : Components.ID("{7377a41d-7bee-4729-8803-0b32218c1a9e}"),
     contractID : "@mozilla.org/zap/sipstack;1",
     factory    : ComponentUtils.generateFactory(function() { return SipUAStack.instantiate(); })
  }]);

