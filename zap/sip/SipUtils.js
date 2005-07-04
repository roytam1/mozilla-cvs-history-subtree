/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.util.importModule('resource:/jscodelib/zap/SipUtils.js', null)" -*- */
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


debug("*** loading SipUtils\n");

Components.util.importModule("resource:/jscodelib/zap/StringUtils.js");

EXPORTED_SYMBOLS = ["BRANCH_COOKIE",
                    "gSIPEventQ",
                    "getProxyOnSIPThread",
                    "gSyntaxFactory",
                    "gDNSService",
                    "gLoggingService",
                    "generateTag",
                    "generateUUID"];

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

function generateUUID() {
  // generate a 'version 4'-type UUID following
  // draft-leach-uuids-guids-01.txt
  var time_low_32 = getRandom32();
  var time_mid_16 = getRandom32() & 0xFFFF;
  var time_high_and_version_16 = 0x4000 | (getRandom32() & 0x0FFF);
  var clock_seq_hi_and_reserved_8 = 0x80 | (getRandom32() & 0x3F);
  var clock_seq_low_8 = getRandom32() & 0xFF;
  var node1_16 = getRandom32() & 0xFFFF;
  var node2_32 = getRandom32();
  
  return padleft(time_low_32.toString(16), "0", 8) + "-" +
    padleft(time_mid_16.toString(16), "0", 4) + "-" +
    padleft(time_high_and_version_16.toString(16), "0", 4) + "-" +
    padleft(clock_seq_hi_and_reserved_8.toString(16), "0", 2) +
    padleft(clock_seq_low_8.toString(16), "0", 2) + "-" +
    padleft(node1_16.toString(16), "0", 4) +
    padleft(node2_32.toString(16), "0", 8);
}
