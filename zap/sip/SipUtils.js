/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:SipUtils.js', null)" -*- */
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
 * SipUtils contains common utility code shared by all SIP modules
 * apart from SipSyntaxFactory.
 */

debug("*** loading SipUtils\n");

Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:FunctionUtils.js");
Components.utils.importModule("gre:AsyncUtils.js");

EXPORTED_SYMBOLS = ["BRANCH_COOKIE",
                    "getSIPThread",
                    "getProxyOnSIPThread",
                    "gSyntaxFactory",
                    "getDNSService",
                    "getNetUtils",
                    "gLoggingService",
                    "generateTag",
                    "gUUIDGenerator",
                    "constructClientTransactionKey",
                    "constructServerTransactionKey",
                    "constructClientDialogID",
                    "constructServerDialogID",
                    "makeOneShotTimer", // from AsyncUtils.js
                    "resetOneShotTimer" // from AsyncUtils.js
                    ];

// name our global object:
function toString() { return "[SipUtils.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// Constants:

// Cookie to identify a branch id compliant to RFC3261:
var BRANCH_COOKIE = "z9hG4bK";



////////////////////////////////////////////////////////////////////////
// Thread management:

// The SIP stack runs on the main UI thread; handing off to other
// threads for asynchronous actions, such as sending data or querying
// dns.
var getSIPThread = getMainThread; // from AsyncUtils.js

// Some of our objects need to be manually proxied so that they only
// get called on the main SIP thread,
// e.g. nsIInputStreamCallback. Other objects are automatically
// proxied, e.g. nsIServerSocketListener.
// XXX XPConnect seems to automatically proxy js-implemented objects
// when QueryInterface is not implemented in js. We want to implement
// QueryInterface in js though, because we want to be able to call
// js objects transparently from js or c++.
function getProxyOnSIPThread(aObject, aInterface) {
  var IProxyManager = Components.interfaces.nsIProxyObjectManager;
  
  var proxyManager = Components. classes["@mozilla.org/xpcomproxy;1"].
    getService(IProxyManager);

  return proxyManager.getProxyForObject(getSIPThread(),
                                        aInterface, aObject,
                                        IProxyManager.INVOKE_SYNC |
                                        IProxyManager.FORCE_PROXY_CREATION);
}


////////////////////////////////////////////////////////////////////////
// gSyntaxFactory: global sip syntax factory instance

// access the syntax factory via xpcom (more overhead, all syntax
// objects need to be xpcom-wrapped):
//var gSyntaxFactory =
//  Components.classes['@mozilla.org/zap/sipsyntaxfactory;1']
//      .getService(Components.interfaces.zapISipSyntaxFactory);

// access the syntax factory directly (less overhead, no type safety,
// access to non-xpcom interface):
var gSyntaxFactory = Components.utils.importModule('gre:SipSyntaxFactory.js', null).theSyntaxFactory;

////////////////////////////////////////////////////////////////////////
// getDNSService: get the global dns service instance

var getDNSService = makeServiceGetter("@mozilla.org/network/dns-service;1",
                                      Components.interfaces.nsIDNSService);

////////////////////////////////////////////////////////////////////////
// getNetUtils

var getNetUtils = makeServiceGetter("@mozilla.org/zap/netutils;1",
                                    Components.interfaces.zapINetUtils);

////////////////////////////////////////////////////////////////////////
// gLoggingService: global logging service instance

// access the logging service via xpcom (more overhead):
//var gLoggingService =
//  Components.classes['@mozilla.org/zap/loggingservice;1']
//      .getService(Components.interfaces.zapILoggingService);

// access the logging service directly (less overhead, no type safety,
// access to non-xpcom interface):
var gLoggingService = Components.utils.importModule('gre:LoggingService.js', null).theLoggingService;

////////////////////////////////////////////////////////////////////////
// gUUIDGenerator: global UUID generator service instance

// access directly via JS:
var gUUIDGenerator = Components.utils.importModule('gre:zapUUIDGenerator.js', null).theUUIDGenerator;


////////////////////////////////////////////////////////////////////////
// Utilities for generating tags & branch parameters:

function getRandom32() {
  // Obtain a 32bit random number:
  // XXX we should use a better random number generator
  return Math.floor(Math.random()*4294967296);
}

function generateTag() {
  // see RFC3261 Section 19.3
  // XXX we would have to use our MAC address for global uniqueness.
  // let's just rely on statistical global uniqueness for now.

  var r = getRandom32();
  
  // return as base 36 to get as few chars as possible:
  // XXX could use base64 encoding
  return r.toString(36);
}

////////////////////////////////////////////////////////////////////////
// Transaction key construction:

function constructClientTransactionKey(message) {
  // client transactions are indexed by branch+";"+CSeq-method,
  // so that we can match them in the client pool according to
  // RFC3261 Section 17.1.3:
  var branch = message.getTopViaHeader().getParameter("branch");
  var method =
    message.getSingleHeader("CSeq").QueryInterface(Components.interfaces.zapISipCSeqHeader).method;
  return branch+";"+method;
}

function constructServerTransactionKey(request) {
  // indexed according to RFC3261 17.2.3 by
  // branch+";"+sent-by-host+":"+sent-by-port+";"+method. 
  // XXX implement RFC2543 compatibility as described in section 17.2.3
  var topVia = request.getTopViaHeader();
  var branch = topVia.getParameter("branch");
  var host = topVia.host;
  var port = topVia.port;
  if (!port)
    port = 5060;
  var method = request.method;
  if (method == "ACK")
    method = "INVITE";
  return branch+";"+host+":"+port+";"+method;
}

////////////////////////////////////////////////////////////////////////
// Dialog id construction:

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


