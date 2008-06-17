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
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/zapStunAuthentication.jsm");

//----------------------------------------------------------------------
// Helpers

var ITF_ATTR = Components.interfaces.zapIStunAttribute;
var ITF_ADDR = Components.interfaces.zapITransportAddress;
var ITF_ERR_ATTR = Components.interfaces.zapIStunErrorCodeAttribute;
var ITF_ADDR_ATTR = Components.interfaces.zapIStunAddressAttribute;

var CLS_TRANS = Components.classes["@mozilla.org/zap/stun-transport;1"];
var ITF_TRANS = Components.interfaces.zapIStunTransport;

// access the logging service directly (less overhead, no type safety,
// access to non-xpcom interface):
var gLoggingService = 
  Components.utils.import('resource://gre/components/LoggingService.js', 
                          null).theLoggingService;

function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("STUN CLIENT", level, mes);
}

////////////////////////////////////////////////////////////////////////
// Class StunTransaction
var StunTransaction = makeClass("StunTransaction", SupportsImpl);
StunTransaction.addInterfaces(Components.interfaces.nsICancelable,
                              Components.interfaces.nsITimerCallback);

StunTransaction.fun(
  function init(client, message, username) {
    this.client   = client;
    this.message  = message;
    this.message.initTransactionID(); 
    this.username = username;
    this.resends  = 0;
    this.redirectedServers = {};
    
    this.listener    = null;
    this.resendTimer = null;
    this.delayTimer  = null;
    this.serverInfo  = null;
    this.currServer  = null;
    this.serverIter  = null;
  });

StunTransaction.fun(
  function clearResend() {
    this.resends = 0;
    if (this.resendTimer) {
      this.resendTimer.cancel();
      this.resendTimer = null;      
    }
  });
  
StunTransaction.fun(
  function setResendTimer() {
    var delay = this.serverInfo.rto;
    
    if (this.resendTimer) {
      if (this.resends == this.maxRetransmissions - 1)
        // sending the last request
        resetOneShotTimer(this.resendTimer, 16 * delay);
      else
        resetOneShotTimer(this.resendTimer, 2 * this.resendTimer.delay);
    }
    else {
      this.resendTimer = makeOneShotTimer(this, delay);
    }
  });
//----------------------------------------------------------------------
// nsICancelable implementation:
   
StunTransaction.fun(
  function cancel(reason) {
    this.client.remove(this);
    this.listener = null;
  });

//----------------------------------------------------------------------
// nsITimerCallback implementation:

// void notify(in nsITimer timer);
StunTransaction.fun(
  function notify(timer) {
    if (this.delayTimer == timer) {
      // called for delaying a transaction
      this.delayTimer = null;
      this.client.executeTransaction(this);
    }
    else {
      this.resends++;   
      this.client.resendRequest(this);
    }
  });
  
////////////////////////////////////////////////////////////////////////
// Class StunServerInfo
var StunServerInfo = makeClass("StunServerInfo", SupportsImpl);
StunServerInfo.addInterfaces(Components.interfaces.nsITimerCallback);

StunServerInfo.fun(
  function init(rto, rtoCacheTimeout, address, hash) {
    this.outstandingTransactions = 0;
    this.lastTransaction = 0;
    this.nextTransaction = 0;
    this.expireTimer = null;
    this.address = address;
    this.hash = hash;
    
    this.rto     = rto; // initial default rto
    this.timeout = rtoCacheTimeout;
    this.rtt     = 0;
    this.srtt    = 0;
    this.rttvar  = 0;
  });

StunServerInfo.fun(
  function updateLastSent() {
    this.lastTransaction = (new Date()).getTime();

    // refresh serverInfo expiration timer
    if (this.expireTimer) {
      resetOneShotTimer(this.expireTimer, this.timeout * 1000);
    }
    else {
      this.expireTimer = makeOneShotTimer(this, this.timeout * 1000);
      hashset(this.hash, this.address, this);
    }
  });

StunServerInfo.fun(
  function startRTT(transaction) {
    // rtt is not measured for resends
    if (transaction.resends == 0)
      this.rtt = (new Date()).getTime();
  });
  
StunServerInfo.fun(
  function stopRTT(transaction) {
    // update state only for the first trial
    if (transaction.resends != 0)
      return;
    
    // compute rtt and update rto according to rfc 2988
    var r = (new Date()).getTime() - this.rtt;

    if (this.srtt == 0) {
      // first rtt measurment
      this.srtt = r;
      this.rttvar = r/2;
    }
    else {
      // subsequent rtt measurment  
      this.rttvar = .75 * this.rttvar + .25 * Math.abs(this.srtt - r);
      this.srtt = .875 * this.srtt + .125 * r;              
    }

    this.rto = this.srtt + 4*this.rttvar;
  });  
  
StunServerInfo.fun(
  function nextTransactionDelay() {
    var curTime = (new Date()).getTime();

    if (curTime - this.lastTransaction >= this.rto)
      return 0;

    if (this.nextTransaction < this.lastTransaction)
      this.nextTransaction = this.lastTransaction;
    
    this.nextTransaction += this.rto;
    
    // this.nextTransaction refers to a absolut time in the future
    return (this.nextTransaction - curTime);    
  });
  
//---------------------------------------------------------------------
// nsITimerCallback implementation:
                          
// void notify(in nsITimer timer);
StunServerInfo.fun(
  function notify(timer) {
    this.expireTimer = null;
    
    // remove myself from the client's hash
    hashdel(this.hash, this.address);
  });
                           
////////////////////////////////////////////////////////////////////////
// Class StunClient
var StunClient = makeClass("StunClient", SupportsImpl);
StunClient.addInterfaces(Components.interfaces.zapIStunAgent,
                         Components.interfaces.zapIStunClient2,
                         Components.interfaces.zapIStunTransportListener);

//----------------------------------------------------------------------
// zapIStunAgent implementation:

StunClient.fun(
  function init(config) {
    this.config = config;
    this._transport = CLS_TRANS.createInstance().QueryInterface(ITF_TRANS);
    this._transport.init(config);
    this._transport.clientListener = this;

    try { 
      this.maxRetransmissions = config.getProperty("maxRetransmissions");
    } catch(e){this.maxRetransmissions=7;}
    try { 
      this.defaultRTO = config.getProperty("defaultRTO");
    } catch(e){this.defaultRTO=500;}  
    try { 
      this.supportRFC3489 = config.getProperty("supportRFC3489");
    } catch(e){this.supportRFC3489=0;}
    try {
      this.supportRedirection = config.getProperty("supportRedirection");
    } catch(e){this.supportRedirection=0;}
    try {
      this.authMechanism = config.getProperty("authMechanism");
    } catch(e){this.authMechanism=0;}
    try {
      this.rtoCacheTimeout = config.getProperty("rtoCacheTimeout");
    } catch(e){this.rtoCacheTimeout=10 * 60;} // 10 minutes by default
    try {
      this.maxTransactions = config.getProperty("maxTransactions");
    } catch(e){this.maxTransactions=10;} // 10 by default
    try {
      this.spaceTransactionsMode = config.getProperty("spaceTransactionsMode");
    } catch(e){this.spaceTransactionsMode=1;} // space by rto by default
    try {
      this.transactionSpaceInterval = config.getProperty("transactionSpaceInterval");
    } catch(e){this.transactionSpaceInterval=0;} // 0 by default
    
    this.auth = getAuthMechanism(this.authMechanism, true);
    this.auth.init(config);
    
    // hash of pending stun transactions indexed by transactionID
    this.transactions = {};
    
    // cache of servers info objects indexed by servers IP address
    this.servers = {};
  });

StunClient.fun(
  function shutdown() {
    // abort pending transactions
    for (var key in this.transactions)
      this.remove(this.transactions[key]);
      
    for (var key in this.servers)
      this.servers[key].notify(null); // fake expiration

    this._transport.clientListener = null;
    this._transport = null;

    this.indicationHandler = null;
    this.config = null;
    this.auth.shutdown();
  });
  
StunClient.fun( 
  function sendIndication(indication, username, transportAddress) {
    var transaction = StunTransaction.instantiate();
    transaction.init(this, indication, username);
    transaction.message.messageClass = indication.CLASS_INDICATION;
    transaction.currServer = transportAddress;
    
    if (!this.auth.wrap(username, transportAddress, indication))
      log("authentication mechanism failed to wrap message");

    hashset(this.transactions, transaction.message.transactionID, transaction);
    
    this.executeTransaction(transaction);
  });  

//  attribute zapIStunTransport transport
StunClient.obj("_transport", null);
StunClient.gettersetter(
  "transport",
  function get_transport() {
    return this._transport;
  },
  function set_transport(val) {
    if (this._transport)
      this._transport.clientListener = null;

    this._transport = val;
    if (this._transport)
      this._transport.clientListener = this;
  });

//  attribute zapIStunIndicationHandler transport
StunClient.obj("indicationHandler", null);

//----------------------------------------------------------------------
// zapIStunClient2 implementation:

StunClient.fun(
  function sendRequest(request, username, destinations, listener) {
    if (!destinations.hasMoreElements())
      return null;
    
    var transaction = StunTransaction.instantiate();
    transaction.init(this, request, username);
    transaction.listener = listener;
    transaction.maxRetransmissions = this.maxRetransmissions;
    transaction.message.messageClass = request.CLASS_REQUEST;

    hashset(this.transactions, transaction.message.transactionID, transaction);
    
    transaction.serverIter = destinations;
    transaction.currServer = transaction.serverIter.getNext().QueryInterface(ITF_ADDR);
         
    if (!this.auth.wrap(transaction.username, transaction.currServer, 
                        transaction.message))
      log("authentication mechanism failed to wrap message");

    this.executeTransaction(transaction);

    return transaction;
  });

//----------------------------------------------------------------------
// zapIStunTransportListener implementation:

StunClient.fun(
  function handleMessage(message, transportAddress) {
    if (message == null) {
      // connection failure
      return this.failTransactionsToServer(transportAddress);
    }
    
    // the client should not process a request
    if (message.messageClass == message.CLASS_REQUEST)
        return false; 

    if (message.messageClass == message.CLASS_INDICATION) {
        this.handleIndication(message, transportAddress);
        return true;
    }

    return this.handleResponse(message, transportAddress);
  });

StunClient.fun(
  function handleResponse(message, transportAddress) {  
    var transaction = hashget(this.transactions, message.transactionID);
    if (!transaction) {
      // the message may need to be parsed by other protocol
      return false;
    }

    // validate source
    if (transportAddress.address != transaction.currServer.address ||
        transportAddress.port    != transaction.currServer.port) {
      // discard. this may be a reply from a previous server  
      return true;
    }

    // lookup cached serverInfo
    var serverInfo = hashget(this.servers, transaction.currServer.address);
    if (!serverInfo) {    
      // discard
      return true;
    }

    // update rtt
    if (transportAddress.transport == "UDP")
      serverInfo.stopRTT(transaction);

    // perform authentication validation
    var authResponse = {};
    if (!this.auth.unwrap(message, authResponse) && authResponse.value == null) {
      // failed to authenticate with no authResponse
      // discard response so retransmits will continue.
      return true;
    }

    var unknownCRAttributes = message.getUnknownCRAttributes({});
    if (unknownCRAttributes.length > 0 && 
        (!this.supportRFC3489 ||
         !this.obsoleteAttributes(unknownCRAttributes))) {
      // section 7.3.3. and 7.3.4. states this is a transaction failure
      this.failure(transaction);        
      return true;
    }

    if (message.messageClass == message.CLASS_ERROR_RESPONSE)
      this.handleErrorResponse(message, transaction, transportAddress, 
                               authResponse.value);
    else
      this.handleSuccessResponse(message, transaction, transportAddress);

    return true;      
  });  
  
StunClient.fun(
  function handleIndication(message, transportAddress) {  
    // section 7.3.2.  Processing an Indication
  
    // perform authentication validation
    var authResponse = {};
    if (!this.auth.unwrap(message, authResponse)) {
      // failed to authenticate
      return; // discard
    }
  
    var unknownCRAttributes = message.getUnknownCRAttributes({});
    if (unknownCRAttributes.length > 0)
      return; // discard
  
    // propagate to listener
    if (this.indicationHandler)   
      this.indicationHandler.handleIndication(message, transportAddress);
  });  
  
StunClient.fun(
  function handleErrorResponse(message, transaction, transportAddress, authResponse) {
    // section 7.3.4.  Processing an Error Response

    var errAttr = message.getAttribute(ITF_ATTR.ERROR_CODE);
    if (!errAttr) {
      this.failure(transaction);
      return;
    }

    // check if the auth mechanism requires a new transaction
    if (authResponse != null) {
      // retry as a new transaction 
      hashdel(this.transactions, transaction.message.transactionID);

      transaction.message = authResponse;
      transaction.clearResend();

      hashset(this.transactions, transaction.message.transactionID, transaction);
        
      this.executeTransaction(transaction);
      return;
    }

    // check if we should redirectRequest
    if (this.supportRedirection) {
      // check if we got a 300 error-code
      errAttr = errAttr.QueryInterface(ITF_ERR_ATTR);
    
      if (errAttr.code == 300) {
        // section 11
        this.redirectRequest(message, transaction);
        return;
      }
    }

    // transaction completed
    this.remove(transaction);

    // propagate response
    transaction.listener.handleErrorResponse(message, transaction, transportAddress);
    
    transaction.listener = null;
  });               

StunClient.fun(
  function handleSuccessResponse(message, transaction, transportAddress) {
    // section 7.3.3.  Processing a Success Response

    // transaction completed
    this.remove(transaction);
  
    // propagate response
    transaction.listener.handleSuccessResponse(message, transaction, transportAddress);

    transaction.listener = null;

    return true;
  });

StunClient.fun(
  function obsoleteAttributes(attrList) {
    for (var ii=0; ii<attrList.length; ++ii) {
      // if we got 4 - SOURCE-ADDRESS or 5 - CHANGED-ADDRESS
      // which are comprehension-required attributes that are not supported
      // but are obsolete, we don't fail the transaction
      if (attrList[ii] != 4 &&  attrList[ii] != 5)
        return false;
    }
    return true;
  });
  
StunClient.fun(
  function executeTransaction(transaction) {
    // section 7.2
    
    // lookup cached server related state
    var serverInfo = hashget(this.servers, transaction.currServer.address);
    if (!serverInfo) {
      serverInfo = StunServerInfo.instantiate();
      serverInfo.init(this.defaultRTO, this.rtoCacheTimeout, 
                      transaction.currServer.address, this.servers);
    }

    // space transactions 
    var delay;
    switch (this.spaceTransactionsMode) {
      case 1: // space by rto
        delay = serverInfo.nextTransactionDelay();
        break;
      case 2: // space by provided interval
        delay = this.transactionSpaceInterval;
        break;
      case 0:
      default:
        delay = 0;
        break;
    }
    
    if (delay != 0) {
      transaction.delayTimer = makeOneShotTimer(transaction, delay);
      return;
    }

    if (transaction.message.messageClass == transaction.message.CLASS_REQUEST)
      this.executeRequestTransaction(transaction, serverInfo);
    else
      this.executeIndicationTransaction(transaction);
  });
    
StunClient.fun(
  function executeRequestTransaction(transaction, serverInfo) {
    // section 7.2

    // limit number of outstanding request/response transactions
    if (serverInfo.outstandingTransactions == this.maxTransactions) {
      this.failure(transaction);
      return;
    }

    // associate serverInfo to the transaction
    if (transaction.serverInfo)
      transaction.serverInfo.outstandingTransactions--;

    transaction.serverInfo = serverInfo;
    transaction.serverInfo.outstandingTransactions++; 

    // send message
    this.sendRequestTransaction(transaction);
  });
  
StunClient.fun(
  function executeIndicationTransaction(transaction) {
    // send message
    this._transport.sendMessage(transaction.message, transaction.currServer);
    
    this.remove(transaction);
  });
  
StunClient.fun(
  function failTransactionsToServer(transportAddress) {  
    // iterate over pendig transactions
    var ret = false;
    for (var key in this.transactions) {
      var transaction = this.transactions[key];
      
      if (transaction.currServer.transport == transportAddress.transport &&
          transaction.currServer.address   == transportAddress.address &&
          transaction.currServer.port      == transportAddress.port) {
          this.failure(transaction);
          ret = true;
      }
    }
    return ret;
  });

StunClient.fun(
  function resendRequest(transaction) {
    if (this.maxRetransmissions == transaction.resends) {     
      transaction.clearResend();
      if (!transaction.serverIter.hasMoreElements()) {
        log("failed to contact the STUN destination");
        this.failure(transaction);
        return;
      }

      transaction.currServer = transaction.serverIter.getNext().QueryInterface(ITF_ADDR);
    
      // execute with the new server
      this.executeTransaction(transaction);
    }
    else {
      this.sendRequestTransaction(transaction);
    }
  });

StunClient.fun(
  function sendRequestTransaction(transaction) {  
    // send message
    this._transport.sendMessage(transaction.message, transaction.currServer);

    // update server state
    transaction.serverInfo.updateLastSent();
  
    if (transaction.currServer.transport == "UDP") {
      transaction.serverInfo.startRTT(transaction);
    
      // set timer
      transaction.setResendTimer();
    }
  });
  
StunClient.fun(
  function redirectRequest(message, transaction) {
    // section 11
    var asAttr = message.getAttribute(ITF_ATTR.ALTERNATE_SERVER);
    if (!asAttr) {
      // missing attribute. ignore message so
      // retransmition (if applicable) will continue
      return;
    }
    asAttr = asAttr.QueryInterface(ITF_ADDR_ATTR);
  
    var currTimestamp = (new Date()).getTime();

    // lookup the recirectedServers cache for this transaction
    // to see if we already recirectred this request
    var timestamp = hashget(transaction.redirectedServers, asAttr.address);
    if (timestamp && currTimestamp - timestamp < 5*60*1000) {
      // this request was already redirected to this server
      // within the last 5 minutes
      this.failure(transaction);
      return;
    }
    hashset(transaction.redirectedServers, asAttr.address, currTimestamp);
  
    // redirect as a new transaction
    hashdel(this.transactions, transaction.message.transactionID);
 
    transaction.clearResend();
    transaction.message.initTransactionID();
    transaction.currServer = { 
      address:asAttr.address, 
      port:asAttr.port, 
      transport:transaction.currServer.transport 
    };
 
    hashset(this.transactions, transaction.message.transactionID, transaction);
    
    this.executeTransaction(transaction);
  });
  
StunClient.fun(
  function failure(transaction) {
    if (transaction.listener)
      transaction.listener.handleFailure(transaction);

    this.remove(transaction);
  });  
   
StunClient.fun(
  function remove(transaction) {
    this.auth.discardState(transaction.message.transactionID);

    if (transaction.dnsRequest) {
      transaction.dnsRequest.cancel(0);
      transaction.dnsRequest = null;
    }
    if (transaction.resendTimer) {
      transaction.resendTimer.cancel();
      transaction.resendTimer = null;
    }
    if (transaction.delayTimer) {
      transaction.delayTimer.cancel();
      transaction.delayTimer = null;
    }

    // if a serverInfo state is associated, 
    // decrement the outstanding transactions count
    if (transaction.serverInfo)
      transaction.serverInfo.outstandingTransactions--;
    
    transaction.serverIter = null;
    transaction.serverInfo = null;
    transaction.currServer = null;
    
    hashdel(this.transactions, transaction.message.transactionID);
  });

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [
   { className  : "ZAP STUN Client",
     cid        : Components.ID("{2c978308-1eaa-4589-a96c-be17c00132e9}"),
     contractID : "@mozilla.org/zap/stun-client2;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return StunClient.instantiate(); })
   }
  ]);
