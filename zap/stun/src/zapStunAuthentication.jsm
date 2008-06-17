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

EXPORTED_SYMBOLS = [ "getAuthMechanism",
                     "createErrorResponse" ];
                     
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");

///////////////////////////////////////////////////////////////////////
// This file provides 5 implementations for the following authentication
// mechanism interface:
// void  init(nsIPropertyBag2 config);
// void shutdown();
// boolean wrap(string username, zapITransportAddress address,
//              zapIStunMessage2 message);
// boolean unwrap(zapIStunMessage2 message, zapIStunMessage2 nextMessage);
// void discardState(string transactionID);
//
// 1 - short-term client side
// 2 - short-term server side
// 3 - long-term  client side
// 4 - long-term  server side
// 5 - none

function getAuthMechanism(type, client) {
  // type could be:
  // 0 - no authentication
  // 1 - short-term
  // 2 - long-term
  var authMechanism;
  switch (type) {
    case 1:
      if (client)
        authMechanism = zapStunShortTermClientAuth.instantiate();
      else
        authMechanism = zapStunShortTermServerAuth.instantiate();
      break;
    case 2:
      if (client)
        authMechanism = zapStunLongTermClientAuth.instantiate();
      else
        authMechanism = zapStunLongTermServerAuth.instantiate();
      break;
    case 0:
    default:
      authMechanism = zapStunNoAuth.instantiate();
      break;
  }
  return authMechanism;
}

////////////////////////////////////////////////////////////////////////
// globals

// zap cryptoutils service
var gZapCryptoUtil = Components.classes["@mozilla.org/zap/cryptoutils;1"];
gZapCryptoUtil = gZapCryptoUtil.getService(Components.interfaces.zapICryptoUtils);

// crypto hash
var gCryptoHash = Components.classes["@mozilla.org/security/hash;1"];
gCryptoHash = gCryptoHash.createInstance(Components.interfaces.nsICryptoHash);

var CLS_STR_ATTR = Components.classes["@mozilla.org/zap/stun-string-attribute;1"];
var ITF_STR_ATTR = Components.interfaces.zapIStunStringAttribute;

var CLS_RAW_ATTR = Components.classes["@mozilla.org/zap/stun-raw-attribute;1"];
var ITF_RAW_ATTR = Components.interfaces.zapIStunRawAttribute;

var CLS_ERR_ATTR = Components.classes["@mozilla.org/zap/stun-error-code-attribute;1"];
var ITF_ERR_ATTR = Components.interfaces.zapIStunErrorCodeAttribute;

var ITF_ATTR = Components.interfaces.zapIStunAttribute;

var CLS_MSG = Components.classes["@mozilla.org/zap/stun-message2;1"];
var ITF_MSG = Components.interfaces.zapIStunMessage2;

var ITF_AUTH = Components.interfaces.nsIAuthPrompt2;

var CLS_STRM = Components.classes["@mozilla.org/io/string-input-stream;1"];
var ITF_STRM = Components.interfaces.nsIStringInputStream;

//----------------------------------------------------------------------
// Helper functions

// access the logging service directly
var gLoggingService = 
  Components.utils.import('resource://gre/components/LoggingService.js', 
                          null).theLoggingService;

function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("STUN AUTH", level, mes);
}

function computeMessageIntegrity(key, message) {
  var fingerPrintAttr = message.getAttribute(ITF_ATTR.FINGERPRINT);

  // temporary remove the fingerprint attribute 
  // to adjust the message length for 
  // the message-integrity calculation
  if (fingerPrintAttr)
    message.removeAttribute(ITF_ATTR.FINGERPRINT);

  var data = message.serialize();
  var mi = gZapCryptoUtil.computeSHA1HMAC(data.substr(0, data.length - 24), key);

  // append back the fingerprint attribute
  if (fingerPrintAttr)
    message.appendAttribute(fingerPrintAttr);

  return mi;
};

function createErrorResponse(message, code, reason) {
  var errAttr    = CLS_ERR_ATTR.createInstance().QueryInterface(ITF_ERR_ATTR);    
  errAttr.type   = ITF_ATTR.ERROR_CODE;
  errAttr.code   = code;
  errAttr.reason = reason;

  var response = CLS_MSG.createInstance().QueryInterface(ITF_MSG);
  response.messageClass = message.CLASS_ERROR_RESPONSE;
  response.messageMethod = message.messageMethod;
  response.transactionID = message.transactionID;
  response.magicCookie = message.magicCookie;
  response.appendAttribute(errAttr);
 
  return response;  
};

function cloneMessage(message) {
  var clone = CLS_MSG.createInstance().QueryInterface(ITF_MSG);
  clone.messageClass  = message.messageClass;
  clone.messageMethod = message.messageMethod;
  clone.transactionID = message.transactionID;
  clone.magicCookie   = message.magicCookie;
 
  var enumerate = message.getAttributeEnumerator();
  while (enumerate.hasMoreElements()) {
    clone.appendAttribute(enumerate.getNext().QueryInterface(ITF_ATTR));
  }
  
  return clone;
}

function getOrSetAttribute(message, attrType, attrCls, attrItf) {
  var attr = message.getAttribute(attrType);
  if (attr) {
    attr = attr.QueryInterface(attrItf);
  }
  else {
    attr = attrCls.createInstance().QueryInterface(attrItf);
    attr.type = attrType;     
    message.appendAttribute(attr);
  }
  
  return attr;
}
    
function getOrSetMiAttr(message) {
  var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY);
  if (miAttr) {
    miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);
  }
  else {
    miAttr = CLS_RAW_ATTR.createInstance().QueryInterface(ITF_RAW_ATTR);
    miAttr.type = ITF_ATTR.MESSAGE_INTEGRITY;

    miAttr.value = "                    "; // dummy 20 bytes value
    message.appendAttribute(miAttr);
  }

  return miAttr;
}

function MD5(data) {
  gCryptoHash.initWithString("md5");
  if (data) {
    var stream = CLS_STRM.createInstance(ITF_STRM);
    stream.setData(data, data.length);
    gCryptoHash.updateFromStream(stream, data.length);
   }
  return gCryptoHash.finish(false);    
};

////////////////////////////////////////////////////////////////////////
// zapStunAuthInformation
var zapStunAuthInformation = makeClass("zapStunAuthInformation", SupportsImpl);
zapStunAuthInformation.addInterfaces(Components.interfaces.nsIAuthInformation);

// readonly attribute unsigned long flags;
zapStunAuthInformation.obj("flags", 0);

// readonly attribute AString realm;
zapStunAuthInformation.obj("realm", "");

// readonly attribute AUTF8String authenticationScheme;
zapStunAuthInformation.obj("authenticationScheme", "");

// attribute AString username
zapStunAuthInformation.obj("username", "");

// attribute AString password
zapStunAuthInformation.obj("password", "");

// attribute AString domain
zapStunAuthInformation.obj("domain", "");

////////////////////////////////////////////////////////////////////////
// 1 - zapStunShortTermClientAuth

var zapStunShortTermClientAuth = makeClass("zapStunShortTermClientAuth");

zapStunShortTermClientAuth.fun(
  function init(config) {
    this.authPrompt = config.getPropertyAsInterface("authPrompt", ITF_AUTH);
    
    // hash of requests authInfo indexed by message transactionID
    this._requests = {};
  });

zapStunShortTermClientAuth.fun(
  function shutdown() {
    this.authPrompt = null;
  });
  
zapStunShortTermClientAuth.fun(
  function wrap(username, address, message) {
  // 10.1.1.  Forming a Request or Indication
    
    var authInfo = zapStunAuthInformation.instantiate();
    authInfo.authenticationScheme = "stun-short-term";
    authInfo.username = username;
    authInfo.flags = authInfo.ONLY_PASSWORD;

    if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
      log("no credentials for " + username);
      return false;
    }

    var unAttr = getOrSetAttribute(message, ITF_ATTR.USERNAME, CLS_STR_ATTR, 
                                   ITF_STR_ATTR);
    unAttr.value = authInfo.username;

    var miAttr = getOrSetMiAttr(message);
    var mi = computeMessageIntegrity(authInfo.password, message);
    miAttr.value = mi;

    // hash the authInfo for Request class
    if (message.messageClass == message.CLASS_REQUEST) {
      hashset(this._requests, message.transactionID, authInfo);
    }
    return true;
  });

zapStunShortTermClientAuth.fun(
  function unwrap(message, nextMessage) {
    nextMessage.value = null;

    if (message.messageClass == message.CLASS_INDICATION) {
      // 10.1.2.  Receiving Indication

      var unAttr = message.getAttribute(ITF_ATTR.USERNAME);
      var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY); 

      if (!unAttr || !miAttr) {
        // missing attributes
        return false;
      }

      unAttr = unAttr.QueryInterface(ITF_STR_ATTR);
      miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);

      var authInfo = zapStunAuthInformation.instantiate();
      authInfo.authenticationScheme = "stun-short-term";
      authInfo.username = unAttr.value;
      authInfo.flags = authInfo.ONLY_PASSWORD;

      if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
        // missing credentials      
        return false;
      }
  
      var mi = computeMessageIntegrity(authInfo.password, message);
      if (mi != miAttr.value) {
        // mi does not match
        return false;
      }
 
      delete authInfo;
      return true;
    }
  
    // 10.1.3.  Receiving a Response
    var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY);
    if (!miAttr) {
      // missing mi attribute
      log("missing message-integrity attribute");
      return false;
    }
    miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);

    var authInfo = hashget(this._requests, message.transactionID);
    if (!authInfo) {
      // missing credentials
      log("missing cached credentials for transaction=" + message.transactionID);
      return false;
    }
    var mi = computeMessageIntegrity(authInfo.password, message);
    if (mi != miAttr.value) {
      // mi does not match
      log("message-integrity does not match");
      return false;
    }

    return true;
  });

// zapIStunAuthMechanism::discardState
zapStunShortTermClientAuth.fun(
  function discardState(transactionID) {
    hashdel(this._requests, transactionID);
  });

////////////////////////////////////////////////////////////////////////
// 2 - zapStunShortTermServerAuth

var zapStunShortTermServerAuth = makeClass("zapStunShortTermServerAuth");

zapStunShortTermServerAuth.fun(
  function init(config) {
    this.authPrompt = config.getPropertyAsInterface("authPrompt", ITF_AUTH);

    // hash of client request authInfo indexed by message transactionID
    this._requests = {};
  });

zapStunShortTermServerAuth.fun(
  function shutdown() {
    this.authPrompt = null;
  });
    
zapStunShortTermServerAuth.fun(
  function wrap(username, address, message) {
    // the server is sending a response or an indication
    var authInfo;
    if (message.messageClass == message.CLASS_INDICATION) {
      authInfo = zapStunAuthInformation.instantiate();
      authInfo.authenticationScheme = "stun-short-term";
      authInfo.username = username;
      authInfo.flags = authInfo.ONLY_PASSWORD;

      if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
        log("missing credentials for " + username);
        return false;
      }
 
      // add username attribute for indications
      var unAttr = getOrSetAttribute(message, ITF_ATTR.USERNAME, CLS_STR_ATTR, 
                                     ITF_STR_ATTR);
      unAttr.value = authInfo.username;
    }
    else {
      authInfo = hashget(this._requests, message.transactionID);
      if (!authInfo) {
        // missing credentials
        log("missing cached credentials for transaction=" + message.transactionID);
        return false;
      }
    }

    var miAttr = getOrSetMiAttr(message);
    var mi = computeMessageIntegrity(authInfo.password, message);
    miAttr.value = mi;
 
    return true;
  });

zapStunShortTermServerAuth.fun(
  function unwrap(message, nextMessage) {
    nextMessage.value = null;
    // 10.1.2.  Receiving a Request or Indication

    var unAttr = message.getAttribute(ITF_ATTR.USERNAME);
    var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY); 
  
    if (!unAttr || !miAttr) {
      if (message.messageClass == message.CLASS_REQUEST) {
        // set error response
        nextMessage.value = createErrorResponse(message, 400, "Bad Request");
      }
      return false;
    }
  
    unAttr = unAttr.QueryInterface(ITF_STR_ATTR);
    miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);

    var authInfo = zapStunAuthInformation.instantiate();
    authInfo.authenticationScheme = "stun-short-term";
    authInfo.username = unAttr.value;
    authInfo.flags = authInfo.ONLY_PASSWORD;

    if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
      // missing credentials
      if (message.messageClass == message.CLASS_REQUEST) {
        // set error response
        nextMessage.value = createErrorResponse(message, 401, "Unauthorized");
      }
      return false;
    }
    
    var mi = computeMessageIntegrity(authInfo.password, message);
    if (mi != miAttr.value) {
      // mi does not match
      if (message.messageClass == message.CLASS_REQUEST) {  
        // set error response
        nextMessage.value = createErrorResponse(message, 401, "Unauthorized");
      }
      return false;
    }

    if (message.messageClass == message.CLASS_REQUEST) {
      // cache credentials so we can process the response
      hashset(this._requests, message.transactionID, authInfo);
    }

    return true;
  });

zapStunShortTermServerAuth.fun(
  function discardState(transactionID) {
    hashdel(this._requests, transactionID);
  });

////////////////////////////////////////////////////////////////////////
// 3 - zapStunLongTermClientAuth

var zapStunLongTermClientAuth = makeClass("zapStunLongTermClientAuth");

zapStunLongTermClientAuth.fun(
  function init(config) {
    this.authPrompt = config.getPropertyAsInterface("authPrompt", ITF_AUTH);
    
    // hash of requests indexed by message transactionID
    this._requests = {};
    
    // hash of server credentials (username, password, realm, and nonce)
    // indexed by the server's ip+port
    this._serverCredentials = {};
  });
  
zapStunLongTermClientAuth.fun(
  function shutdown() {
    this.authPrompt = null;
  });
  
zapStunLongTermClientAuth.fun(
  function wrap(username, address, message) {
    // 10.2.1.  Forming a Request
    
    // long-term credential mechanism is not be used
    // to protect indications
    if (message.messageClass != message.CLASS_REQUEST)
      return true;
      
    var request = {
      message           : message,
      serverCredentials : null,
      address           : address.address,
      port              : address.port
    };
    
    // lookup the servers cache
    request.serverCredentials = hashget(this._serverCredentials, 
                                        address.address+":"+address.port);
    hashset(this._requests, message.transactionID, request);
    
    if (!request.serverCredentials) {
      // 10.2.1.1.  First Request
      // send the request as is with no authentication attributes
      return true;
    }

    // 10.2.1.2.  Subsequent Requests
    var unAttr = getOrSetAttribute(message, ITF_ATTR.USERNAME, CLS_STR_ATTR, 
                                   ITF_STR_ATTR);   
    unAttr.value = request.serverCredentials.username;
    
    var realmAttr = getOrSetAttribute(message, ITF_ATTR.REALM, CLS_STR_ATTR, 
                                      ITF_STR_ATTR);
    realmAttr.value = request.serverCredentials.realm;
    
    var nonceAttr = getOrSetAttribute(message, ITF_ATTR.NONCE, CLS_STR_ATTR, 
                                      ITF_STR_ATTR);
    nonceAttr.value = request.serverCredentials.nonce;
    
    var miAttr = getOrSetMiAttr(message);
    var key = MD5(request.serverCredentials.username+":"+
                  request.serverCredentials.realm+":"+
                  request.serverCredentials.password);
    var mi = computeMessageIntegrity(key, message);
    miAttr.value = mi;
    
    return true;
  });

zapStunLongTermClientAuth.fun(
  function unwrap(message, nextMessage) {
    nextMessage.value = null;
    // 10.2.3.  Receiving a Response
    
    // long-term credential mechanism is not be used
    // to protect indications
    if (message.messageClass == message.CLASS_INDICATION)
      return true;
      
    // lookup the cached request
    var request = hashget(this._requests, message.transactionID);
    if (!request)
      return false;

    if (message.messageClass == message.CLASS_SUCCESS_RESPONSE) {
      // validate mi
      var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY);
      if (!miAttr) {
        // missing mi attribute
        return false;
      }
      miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);

      var key = MD5(request.serverCredentials.username+":"+
                    request.serverCredentials.realm+":"+
                    request.serverCredentials.password);
      var mi = computeMessageIntegrity(key, message);
      if (mi != miAttr.value) {
        // mi does not match
        return false;
      }

      return true;
    }

    // handle error response
    var errAttr = message.getAttribute(ITF_ATTR.ERROR_CODE);
    if (!errAttr) {
      // missing error-code attribute
      return false;   
    }
    errAttr = errAttr.QueryInterface(ITF_ERR_ATTR);

    // check the error code
    if (errAttr.code != 401 && errAttr.code != 438) {
      // validate the response
      var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY);
      if (!miAttr) {
        // missing mi attribute
        return false;
      }
      miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);

      var key = MD5(request.serverCredentials.username+":"+
                    request.serverCredentials.realm+":"+
                    request.serverCredentials.password);
      var mi = computeMessageIntegrity(key, message);
      if (mi != miAttr.value) {
        // mi does not match
        return false;
      }

      if (errAttr.code == 300) {
        // server replied with an alternate-server
        // cache the credentials for this alternate-server
        var asAttr = message.getAttribute(ITF_ATTR.ALTERNATE_SERVER);
        if (asAttr) {
          asAttr = asAttr.QueryInterface(Components.interfaces.zapIStunAddressAttribute);
          hashset(this._serverCredentials, asAttr.address+":"+asAttr.port,
                  request.serverCredentials);
        }
      }

      return true;
    }

    // handle a 401 or 438 error response
    // prepare a retry request with response to the challenge
    var retryMsg = cloneMessage(request.message);
    retryMsg.initTransactionID();
  
    var realmAttr = message.getAttribute(ITF_ATTR.REALM);   
    realmAttr = realmAttr.QueryInterface(ITF_STR_ATTR);

    var nonceAttr = message.getAttribute(ITF_ATTR.NONCE);
    nonceAttr = nonceAttr.QueryInterface(ITF_STR_ATTR);

    var authInfo = zapStunAuthInformation.instantiate();
    authInfo.authenticationScheme = "stun-long-term";
    authInfo.realm = realmAttr.value;

    // lookup credentials by realm  
    if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
      // no credentials for the presented realm
      // propagate the failure
      return true;
    }

    var serverCredentials = {
      username : authInfo.username,
      password : authInfo.password,
      realm    : realmAttr.value,
      nonce    : nonceAttr.value
    };
  
    delete authInfo;
  
    // check if the original request included auth attributes
    if (retryMsg.getAttribute(ITF_ATTR.USERNAME) && errAttr.code == 401) {
      // for 401 error, make sure we are changing at least one auth attribute
      if (request.serverCredentials.username == serverCredentials.username &&
          request.serverCredentials.realm    == serverCredentials.realm    &&
          request.serverCredentials.password == serverCredentials.password) {
          // we are not changing credentials. do not retry. propagate failure

          return true;
      }
    }

    var unAttr = getOrSetAttribute(retryMsg, ITF_ATTR.USERNAME, CLS_STR_ATTR, 
                                   ITF_STR_ATTR);    
    unAttr.value = serverCredentials.username;
    
    var respRealmAttr = getOrSetAttribute(retryMsg, ITF_ATTR.REALM, CLS_STR_ATTR, 
                                          ITF_STR_ATTR);
    respRealmAttr.value = realmAttr.value;
    
    var respNonceAttr = getOrSetAttribute(retryMsg, ITF_ATTR.NONCE, CLS_STR_ATTR, 
                                          ITF_STR_ATTR);
    respNonceAttr.value = nonceAttr.value;

    var miAttr = getOrSetMiAttr(retryMsg);
    var key = MD5(serverCredentials.username+":"+serverCredentials.realm+":"+
                  serverCredentials.password);
    var mi = computeMessageIntegrity(key, retryMsg);
    miAttr.value = mi;

    request.message = retryMsg;
    request.serverCredentials = serverCredentials;
        
    // update the cache with the new request
    hashdel(this._requests, message.transactionID);
    hashset(this._requests, retryMsg.transactionID, request);

    // cache server credentials for subsequent requests
    hashset(this._serverCredentials, request.address+":"+request.port, 
            serverCredentials);

    nextMessage.value = retryMsg;
    
    return false;
  });
  
zapStunLongTermClientAuth.fun(
  function discardState(transactionID) {
    hashdel(this._requests, transactionID);    
  });
  
////////////////////////////////////////////////////////////////////////
// 4 - zapStunLongTermServerAuth

var zapStunLongTermServerAuth = makeClass("zapStunLongTermServerAuth");

zapStunLongTermServerAuth.fun(
  function init(config) {
    this.authPrompt = config.getPropertyAsInterface("authPrompt", ITF_AUTH);
    try { 
      this.realm = config.getProperty("serverRealm");
    } catch(e) {this.realm="";}
    try { 
      this.serverPassword = config.getProperty("serverPassword");
    } catch(e){this.serverPassword="";}  
    try { 
      this.nonceExpiration = config.getProperty("nonceExpiration");
    } catch(e){this.nonceExpiration=5000;}     
    
    // hash of client request authInfo indexed by message transactionID
    this._requests = {};
  });
  
zapStunLongTermServerAuth.fun(
  function shutdown() {
    this.authPrompt = null;
  });  

zapStunLongTermServerAuth.fun(
  function wrap(username, address, message) {
    // long-term credential mechanism can not be used
    // to protect indications
    if (message.messageClass == message.CLASS_INDICATION)
      return true;
    
    // look up cached credentials
    var authInfo = hashget(this._requests, message.transactionID);
    if (!authInfo) {
      // missing credentials
      return false;
    }
    
    var miAttr = getOrSetMiAttr(message);
    var key = MD5(authInfo.username+":"+this.realm+":"+authInfo.password);
    var mi = computeMessageIntegrity(key, message);
    miAttr.value = mi;
    
    return true;    
  });
  
zapStunLongTermServerAuth.fun(
  function unwrap(message, nextMessage) {
    nextMessage.value = null;
    // 10.2.2.  Receiving a Request
    
    // long-term credential mechanism can not be used
    // to protect indications
    if (message.messageClass != message.CLASS_REQUEST)
      return true;
    
    var miAttr = message.getAttribute(ITF_ATTR.MESSAGE_INTEGRITY); 
    if (!miAttr) {
      // missing mi attribute. create error response
      nextMessage.value = this.createChallengeResponse(message, 401, "Unauthorized");   
      return false;
    }
    
    var unAttr = message.getAttribute(ITF_ATTR.USERNAME);
    var nonceAttr = message.getAttribute(ITF_ATTR.NONCE);
    if (!unAttr || !nonceAttr ||
        !message.getAttribute(ITF_ATTR.REALM)) {
      // create error response
      nextMessage.value = createErrorResponse(message, 400, "Bad Request");
      return false;
    }
    
    unAttr = unAttr.QueryInterface(ITF_STR_ATTR);   
    nonceAttr = nonceAttr.QueryInterface(ITF_STR_ATTR);
    
    if (!this.validateNonce(nonceAttr.value)) {
      // stale nonce
      nextMessage.value = this.createChallengeResponse(message, 438, "Stale Nonce");    
      return false;
    }
    
    var authInfo = zapStunAuthInformation.instantiate();
    authInfo.authenticationScheme = "stun-long-term";
    authInfo.username = unAttr.value;
    authInfo.flags = authInfo.ONLY_PASSWORD;

    if (!this.authPrompt.promptAuth(null, 0, authInfo)) {
      // missing credentials      
      nextMessage.value = this.createChallengeResponse(message, 401, "Unauthorized");   
      return false;
    }
    
    miAttr = miAttr.QueryInterface(ITF_RAW_ATTR);
    var key = MD5(authInfo.username+":"+this.realm+":"+authInfo.password);
    var mi = computeMessageIntegrity(key, message);
    
    if (mi != miAttr.value) {
      // mi does not match
      nextMessage.value = this.createChallengeResponse(message, 401, "Unauthorized");   
      return false;
    }
    
    hashset(this._requests, message.transactionID, authInfo);
      
    return true;
    
  });
  
zapStunLongTermServerAuth.fun(
  function discardState(transactionID) {
    hashdel(this._requests, transactionID);    
  });
  
zapStunLongTermServerAuth.fun(
  function generateNonce() {
    var d = new Date();
    var timestamp = d.getTime();
    // from RFC 2617
    // nonce = time-stamp H(time-stamp ":" private-key)
    return timestamp+MD5Hex(timestamp+":"+this.serverPassword);
  });

zapStunLongTermServerAuth.fun(
  function validateNonce(nonce) {
    if (nonce.length <= 32)
      return false;
      
    var d = new Date();
    var timestamp = d.getTime();
    var nonceTimestamp = nonce.substr(0, nonce.length-32);
    
    var diff = parseInt(timestamp,10) - parseInt(nonceTimestamp, 10);
    if (diff > this.nonceExpiration)
      return false;
    var h = nonceTimestamp+MD5Hex(nonceTimestamp+":"+this.serverPassword);
    return (h == nonce);
  });  

zapStunLongTermServerAuth.fun(
  function createChallengeResponse(message, code, reason) {
    var errMsg = createErrorResponse(message, code, reason);
      
    var realmAttr = CLS_STR_ATTR.createInstance().QueryInterface(ITF_STR_ATTR);   
    realmAttr.type = ITF_ATTR.REALM;
    realmAttr.value = this.realm;

    var nonceAttr = CLS_STR_ATTR.createInstance().QueryInterface(ITF_STR_ATTR);   
    nonceAttr.type = ITF_ATTR.NONCE;
    nonceAttr.value = this.generateNonce();

    errMsg.appendAttribute(realmAttr);
    errMsg.appendAttribute(nonceAttr); 
  
    return errMsg;
  });  

////////////////////////////////////////////////////////////////////////
// 5 - zapStunNoAuth

var zapStunNoAuth = makeClass("zapStunNoAuth");

zapStunNoAuth.fun(
  function init(config) {
  });

zapStunNoAuth.fun(
  function shutdown() {
  });  
  
zapStunNoAuth.fun(
  function wrap(username, address, message) {
    return true;
  });
  
zapStunNoAuth.fun(
  function unwrap(message, nextMessage) {
    nextMessage.value = null;
    return true;
  });

zapStunNoAuth.fun(
  function discardState(transactionID) {
  });
