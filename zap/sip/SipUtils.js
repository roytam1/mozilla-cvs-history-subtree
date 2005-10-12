/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('resource:/jscodelib/zap/SipUtils.js', null)" -*- */
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

Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");

EXPORTED_SYMBOLS = ["BRANCH_COOKIE",
                    "gSIPEventQ",
                    "getProxyOnSIPThread",
                    "callAsync",
                    "gSyntaxFactory",
                    "gDNSService",
                    "gLoggingService",
                    "generateTag",
                    "gUUIDGenerator",
                    "constructClientTransactionKey",
                    "constructServerTransactionKey",
                    "constructClientDialogID",
                    "constructServerDialogID",
                    "makeOneShotTimer",
                    "resetOneShotTimer"];

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

var ITF_EVENT_Q_SERVICE = Components.interfaces.nsIEventQueueService;

var eventQService = Components.classes["@mozilla.org/event-queue-service;1"].getService(ITF_EVENT_Q_SERVICE);

// The SIP stack runs on the main UI thread; handing off to other
// threads for asynchronous actions, such as sending data or querying
// dns.
var gSIPEventQ = eventQService.getSpecialEventQueue(ITF_EVENT_Q_SERVICE.UI_THREAD_EVENT_QUEUE);

// Some of our objects need to be manually proxied so that they only
// get called on the main SIP thread,
// e.g. nsIInputStreamCallback. Other objects are automatically
// proxied, e.g. nsIServerSocketListener.
// XXX XPConnect seems to automatically proxy js-implemented objects
// when QueryInterface is not implemented in js. We want to implement
// QueryInterface in js though, because we want to be able to call
// js objects transparently from js or c++.
function getProxyOnSIPThread(aObject, aInterface) {
    var proxyManager = Components. classes["@mozilla.org/xpcomproxy;1"].
            getService(Components.interfaces.nsIProxyObjectManager);

    return proxyManager.getProxyForObject(gSIPEventQ,
                                          aInterface, aObject, 5);
    // 5 == PROXY_ALWAYS | PROXY_SYNC
}

// Call 'fct' asynchronous:
function callAsync(fct) {
  // XXX we *really* want a new method on nsIEventTarget for posting
  // from JS here. setTimeout is not a good idea, since, among other
  // things, it sets up a 10ms timer even if the timeout value is 0ms.
  // This means that there is the potential for race conditions - the
  // call is not guaranteed to be the next one in the queue.
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var enumerator = wm.getEnumerator(null);
  var retval = [];
  if (!enumerator.hasMoreElements()) return; // can't post
  enumerator.getNext().setTimeout(fct, 0);
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
var gSyntaxFactory = Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SipSyntaxFactory.js').theSyntaxFactory;

////////////////////////////////////////////////////////////////////////
// gDNSService: global dns service instance

var CLASS_DNS_SERVICE = Components.classes["@mozilla.org/network/dns-service;1"];
var ITF_DNS_SERVICE = Components.interfaces.nsIDNSService;

var gDNSService = CLASS_DNS_SERVICE.getService(ITF_DNS_SERVICE);

////////////////////////////////////////////////////////////////////////
// gLoggingService: global logging service instance

// access the logging service via xpcom (more overhead):
//var gLoggingService =
//  Components.classes['@mozilla.org/zap/loggingservice;1']
//      .getService(Components.interfaces.zapILoggingService);

// access the logging service directly (less overhead, no type safety,
// access to non-xpcom interface):
var gLoggingService = Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:LoggingService.js').theLoggingService;

////////////////////////////////////////////////////////////////////////
// gUUIDGenerator: global UUID generator service instance

// access directly via JS:
var gUUIDGenerator = Components.classes["@mozilla.org/moz/jsloader;1"].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:zapUUIDGenerator.js').theUUIDGenerator;


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


////////////////////////////////////////////////////////////////////////
// Timer utilities:

var CLASS_TIMER = "@mozilla.org/timer;1";

/** Create a new one-shot timer
 *
 * 'callback' must implement nsITimerCallback
 * 'duration' is the time in ms after which the timer fires
 *
 * The timer can be cancelled with timer.cancel() and reset with a
 * different duration using resetOneShotTimer(.).
 */
function makeOneShotTimer(callback, duration) {
  // We need to use 'this.Components' to ensure that xpconnect wraps
  // the timer for the caller's global object (assuming that the
  // callback has the same global object). See also the comments in
  // ClassUtils.js, function makeClass().
  var timer = this.Components.classes[CLASS_TIMER].createInstance(this.Components.interfaces.nsITimer);
  timer.initWithCallback(callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

function resetOneShotTimer(timer, duration) {
  timer.initWithCallback(timer.callback, duration, this.Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  return timer;
}
