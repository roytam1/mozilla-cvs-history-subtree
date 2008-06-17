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
 * Portions created by the Initial Developer are Copyright (C) 2005-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Eilon Yardeni <eyardeni@8x8.com> (original author)
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

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/AsyncUtils.jsm");
Components.utils.import("resource://gre/components/zapStunAuthentication.jsm");

//----------------------------------------------------------------------
// Helpers

var ITF_ATTR = Components.interfaces.zapIStunAttribute;
var ITF_ADDR = Components.interfaces.zapITransportAddress;

var CLS_UKN_ATTR = Components.classes["@mozilla.org/zap/stun-unknown-attribute;1"];
var ITF_UKN_ATTR = Components.interfaces.zapIStunUnknownAttribute;

var CLS_TRANSPORT = Components.classes["@mozilla.org/zap/stun-transport;1"];
var ITF_TRANSPORT = Components.interfaces.zapIStunTransport;

// access the logging service directly
var gLoggingService = 
  Components.utils.import('resource://gre/components/LoggingService.js', 
                          null).theLoggingService;

function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("STUN SERVER", level, mes);
}
  
////////////////////////////////////////////////////////////////////////
// Class StunResponse
var StunResponse = makeClass("StunResponse", SupportsImpl);
StunResponse.addInterfaces(Components.interfaces.nsITimerCallback);

StunResponse.fun(
  function init(response, timeout, hash) {
    this.hash        = hash;
    this.response    = response;
    this.expireTimer = makeOneShotTimer(this, timeout * 1000);
    
    // add myself to the cache
    hashset(this.hash, this.response.transactionID, this);
  });
  
//---------------------------------------------------------------------
// nsITimerCallback implementation:
                          
// void notify(in nsITimer timer);
StunResponse.fun(
  function notify(timer) {    
    // remove myself from cache
    hashdel(this.hash, this.response.transactionID);
  });
                             
////////////////////////////////////////////////////////////////////////
// Class StunServer

var StunServer = makeClass("StunServer", SupportsImpl);
StunServer.addInterfaces(Components.interfaces.zapIStunAgent,
                         Components.interfaces.zapIStunServer2,
                         Components.interfaces.zapIStunTransportListener);

//----------------------------------------------------------------------
// zapIStunAgent implementation:

StunServer.fun(
  function init(config) {
    this.config = config;
    this._transport = CLS_TRANSPORT.createInstance().QueryInterface(ITF_TRANSPORT);
    this._transport.init(config);
    this._transport.serverListener = this;
 
    try {
      this.authMechanism = config.getProperty("authMechanism");
    } catch(e){this.authMechanism=0;}
    try {
      this.idempotentMethods = config.getProperty("idempotentMethods");
    } catch(e){this.idempotentMethods="1";}
    try {
      this.responseCacheTimeout = config.getProperty("responseCacheTimeout");
    } catch(e){this.responseCacheTimeout=40;} // default of 40 seconds
  
    this.idempotentMethods = this.idempotentMethods.split(":");
     
    this.auth = getAuthMechanism(this.authMechanism, false);
    this.auth.init(config);
    
    // cache of last-sent responses indexed by transactionID
    this._responses = {};
  });

StunServer.fun(
  function shutdown() {
    this._transport.serverListener = null;
    this._transport = null;
    
    this.indicationHandler = null;
    this.config = null;
    this.requestHandler = null;
    this.auth.shutdown();
  });
  
StunServer.fun(
  function sendIndication(indication, username, transportAddress) {
    indication.messageClass = indication.CLASS_INDICATION;
    indication.initTransactionID();
    
    if (!this.auth.wrap(username, transportAddress, indication))
      log("authentication mechanism failed to wrap message");

    // send message
    this._transport.sendMessage(indication, transportAddress);
  });  
  
//  attribute zapIStunTransport transport
StunServer.obj("_transport", null);
StunServer.gettersetter(
  "transport",
  function get_transport() {
    return this._transport;
  },
  function set_transport(val) {
    if (this._transport)
      this._transport.serverListener = null;

    this._transport = val;
    if (this._transport)
      this._transport.serverListener = this;
  });

//  attribute zapIStunIndicationHandler transport
StunServer.obj("indicationHandler", null);
  
//----------------------------------------------------------------------
// zapIStunServer implementation:
  
StunServer.fun(
  function sendResponse(response, transportAddress) {
    // section 7.3.1.1.  Forming a Success or Error Response
    
    // wrap the response with required authentication attributes
    if (!this.auth.wrap("", transportAddress, response)) {
      log("authentication mechanism failed to wrap message");
      return; // client will retransmit
    }
  
    if (transportAddress.transport == "UDP" && 
        response.messageClass == response.CLASS_SUCCESS_RESPONSE &&
        !member(response.messageMethod, this.idempotentMethods)) {
      // cache udp, non-idempotent, success responses
      var stunResponse = StunResponse.instantiate();
      stunResponse.init(response, this.responseCacheTimeout, this._responses);
    }
    
    this._transport.sendMessage(response, transportAddress);
  });

// attribute zapIStunRequestHandler listener
StunServer.obj("requestHandler", null);

//----------------------------------------------------------------------
// zapIStunTransportListener implementation:

StunServer.fun(
  function handleMessage(message, transportAddress) {
    if (message == null) {
      // connection failure
      return true;
    }
    
    // section 7.3 Receiving a STUN Message
    
    // the server should not process a response
    if (message.messageClass == message.CLASS_ERROR_RESPONSE ||
        message.messageClass == message.CLASS_SUCCESS_RESPONSE)
        return false;
  
    // perform authentication validation
    var authResponse = {};
    if (!this.auth.unwrap(message, authResponse) && authResponse.value == null) {
      // failed to authenticate with no authResponse
      // discard the request/indication
      return true;
    }
 
    // check if the authentication mechanism has generated a response
    if (authResponse.value != null) {
      this._transport.sendMessage(response, transportAddress);
      return true;
    }
  
    if (message.messageClass == message.CLASS_REQUEST)
      this.processRequest(message, transportAddress);
    else
      this.processIndication(message, transportAddress);
    
    this.auth.discardState(message.transactionID);
    
    return true;
  });
  
StunServer.fun(
  function processRequest(message, transportAddress) {  
    // section 7.3.1.  Processing a Request
    var unknownCRAttributes = message.getUnknownCRAttributes({});
    if (unknownCRAttributes > 0) {
      var response = createErrorResponse(message, 420, "Unknown Attribute");
      
      var unknownAttr = CLS_UKN_ATTR.createInstance().QueryInterface(ITF_UKN_ATTR);
      unknownAttr.type = ITF_ATTR.UNKNOWN_ATTRIBUTES;
      unknownAttr.setUnknownAttributes(unknownCRAttributes, unknownCRAttributes.length);

      response.appendAttribute(unknownAttr);
      
      // wrap the response with required authentication attributes
      this.auth.wrap("", transportAddress, response);
      this._transport.sendMessage(response, transportAddress);
      return;
    }
  
    if (transportAddress.transport == "UDP" && 
        !member(message.messageMethod, this.idempotentMethods)) {
      // lookup in the cache for recently sent non-idempotent
      // success responses for this request
      var response = hashget(this._responses, message.transactionID);
      if (response) {
        this._transport.sendMessage(response, transportAddress);
        return;
      }
    }
        
    // propagate to listener
    if (this.requestHandler)    
      this.requestHandler.handleRequest(message, transportAddress);
  });
  
StunServer.fun(
  function processIndication(message, transportAddress) {  
    // section 7.3.2.  Processing an Indication
  
    if (message.unknownCRAttributes)
      return; // discard
  
    // propagate to listener
    if (this.indicationHandler)   
      this.indicationHandler.handleIndication(message, transportAddress);
  });  

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [
   { className  : "ZAP STUN Server",
     cid        : Components.ID("{42692f97-39d5-441f-bcdc-d679ec436339}"),
     contractID : "@mozilla.org/zap/stun-server2;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return StunServer.instantiate(); })
   }
  ]);
