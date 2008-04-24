/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipResolver.js', null)" -*- */
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


debug("*** loading SipResolver.js\n");

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");

// name our global object:
// function toString() { return "[SipResolver.js]"; }


////////////////////////////////////////////////////////////////////////
// Helpers

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP RESOLVER", level, mes);
}

////////////////////////////////////////////////////////////////////////
// SipResolver

var SipResolver = makeClass("SipResolver", SupportsImpl);
SipResolver.addInterfaces(Components.interfaces.zapISipResolver);

//----------------------------------------------------------------------
// zapISipResolver implementation:

SipResolver.fun(
  function resolveDestinationsAsync(uri, resolveListener, attemptTCP) {
    // XXX do the lookup properly (SRV,NAPTR) (rfc3261 8.1.2, rfc3263)    

    // first check if the transport has been specified as a URI parameter:
    var transport = uri.getURIParameter("transport");
    transport = transport.toUpperCase();
    if (transport != "UDP" && transport != "TCP") {
      // no. -> determine transport based on scheme:
      if (uri.sips)
        transport = "TCP";
      else
        transport = "UDP";
    }
    
    var dnsListener = {
      onLookupComplete : function(req, rec, status) {
        var rv = [];
        if (rec) {
          var port = uri.port ? uri.port : "5060";
          while (rec.hasMore()) {
            var addr = rec.getNextAddrAsString();
            if (transport!="TCP" && attemptTCP) {
              var endpoint = {
                transport : "TCP",
                address   : addr,
                port      : port
              };
              rv.push(endpoint);
            }
            var endpoint = {
              transport : transport,
              address   : addr,
              port      : port
              };
            rv.push(endpoint);

          }
        }
        resolveListener.resolveComplete(rv, rv.length);
        resolveListener = null;
      }
    };
    
    getDNSService().asyncResolve(uri.host, 0, dnsListener, getSIPThread());
    
  });

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [{ className  : "ZAP SIP Resolver",
     cid        : Components.ID("{5dcd9252-efd0-4664-9a6d-546e89a5cd22}"),
     contractID : "@mozilla.org/zap/sipresolver;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipResolver.instantiate(); })
  }]);

