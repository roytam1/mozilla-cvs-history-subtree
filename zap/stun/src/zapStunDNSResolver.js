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
Components.utils.import("resource://gre/components/FunctionUtils.jsm");

////////////////////////////////////////////////////////////////////////
// Helpers

var getDNSService = makeServiceGetter("@mozilla.org/network/dns-service;1",
                                      Components.interfaces.nsIDNSService);

var ITF_DNS = Components.interfaces.nsIDNSService;
var ITF_DNSSRV = Components.interfaces.nsIDNSSRVRecord;
var ITF_DNSREC = Components.interfaces.nsIDNSRecord;

// rfc3986 'unreserved'
var CHARSET_UNRESERVED = "A-Za-z0-9-_.~";

// rfc3986 'sub-delims'
var CHARSET_SUBDELIMS = "!$&'()*+,;=";

var PATTERN_HEX = "[a-fA-F\\d]";

// rfc3986 'pct-encoded'
var PATTERN_PCT_ENCODED = "%"+PATTERN_HEX+PATTERN_HEX;

// rfc3986 'dec-octet'
var PATTERN_DECOCTET = "(?:\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])";

// rfc3986 'IPv4address'
var PATTERN_IPV4ADDRESS = PATTERN_DECOCTET+"\\."+PATTERN_DECOCTET+"\\."+
                          PATTERN_DECOCTET+"\\."+PATTERN_DECOCTET;

// rfc3986 'H16'
var H16 = PATTERN_HEX+"{1,4}";

// rfc3986 'LS32'
var LS32 = "(?:"+H16+":"+H16+"|"+PATTERN_IPV4ADDRESS+")";

// rfc3986 'IPv6address'
var PATTERN_IPV6ADDRESS =   "(?:(?:"+H16+":){6}"+LS32+"|"+
                             "::(?:"+H16+":){5}"+LS32+"|"+
                 "(?:"+H16+")?::(?:"+H16+":){4}"+LS32+"|"+
 "(?:(?:"+H16+":){,1}"+H16+")?::(?:"+H16+":){3}"+LS32+"|"+
 "(?:(?:"+H16+":){,2}"+H16+")?::(?:"+H16+":){2}"+LS32+"|"+
 "(?:(?:"+H16+":){,3}"+H16+")?::(?:"+H16+":){1}"+LS32+"|"+
 "(?:(?:"+H16+":){,4}"+H16+")?::"               +LS32+"|"+
 "(?:(?:"+H16+":){,5}"+H16+")?::"                +H16+"|"+
 "(?:(?:"+H16+":){,6}"+")?::)";

// rfc3986 'port'
var PATTERN_PORT = "\\d+";

// rfc3986 'IPvFuture'
var PATTERN_IPVFUTURE = "(?:v(?:"+PATTERN_HEX+"){1,}\\."+
                        "(?:["+CHARSET_UNRESERVED+CHARSET_SUBDELIMS+":]){1,})";

// rfc3986 'IP-literal'
var PATTERN_IPLITERAL = "(?:\\["+PATTERN_IPV6ADDRESS+"\\]|"+
                           "\\["+PATTERN_IPVFUTURE+"\\])";

// rfc3986 'reg-name'
var PATTERN_REGNAME = "(?:(?:["+CHARSET_UNRESERVED+CHARSET_SUBDELIMS+"]|"+
                             PATTERN_PCT_ENCODED+"){1,})";

// rfc3986 'host'
var PATTERN_HOST = "("+PATTERN_IPLITERAL+  "|"+
                       PATTERN_IPV4ADDRESS+"|"+
                       PATTERN_REGNAME+")";

var PATTERN_AUTHORITY = PATTERN_HOST+"(?:\\:("+PATTERN_PORT+"))?";

var REGEXP_AUTHORITY = new RegExp("^"+PATTERN_AUTHORITY+"$");
                          
var MAXINT = Math.pow(2,31) - 1;                                                           
                                      
////////////////////////////////////////////////////////////////////////
// Class TransportAddress
var TransportAddress = makeClass("TransportAddress", SupportsImpl);
TransportAddress.addInterfaces(Components.interfaces.zapITransportAddress);

//  readonly attribute ACString address;
TransportAddress.obj("address", null);

//  readonly attribute unsigned long port;
TransportAddress.obj("port", 0);
  
//  readonly attribute ACString transport;
TransportAddress.obj("transport", null);

////////////////////////////////////////////////////////////////////////
// Class StunDNSRequest
var StunDNSRequest = makeClass("StunDNSRequest", SupportsImpl);
StunDNSRequest.addInterfaces(Components.interfaces.nsICancelable,
                             Components.interfaces.nsIDNSIterListener,
                             Components.interfaces.nsISimpleEnumerator);
                             
StunDNSRequest.appendCtor(
  function ctor() {
    this.dnsQuery     = null;
    this.dnsSRVQuery  = null;   
    this.listener     = null;
    this.protocol     = null;
    this.hostArray    = [];
    this.hostIndex    = 0;
    this.addressArray = [];
    this.addressIndex = 0;
    this.initTime     = (new Date()).getTime();
  });

//----------------------------------------------------------------------
// nsICancelable implementation:
   
StunDNSRequest.fun(
  function cancel(reason) {
    if (this.dnsQuery) {
      this.dnsQuery.cancel(reason);
      this.dnsQuery = null;
    }
    if (this.dnsSRVQuery) {
      this.dnsSRVQuery.cancel(reason);
      this.dnsSRVQuery = null;
    }
  
    this.listener = null;
  });  
  
//----------------------------------------------------------------------
// nsIDNSIterListener implementation:

StunDNSRequest.fun(
  function onLookupComplete(request, iter, status) {
    if (this.dnsSRVQuery == request) {
      if (iter) {
        this.hostArray.length = 0; // clear
        while (iter.hasMoreElements()) {
          var rec = iter.getNext().QueryInterface(ITF_DNSSRV);
          this.hostArray.push( {
            address: rec.host, 
            port: rec.port,
            ttl: rec.ttl
          });
        }
      }
        
      this.dnsSRVQuery = null;
        
      if (!iter && this.protocol == "TLS") {
        // for TLS, lack of SRV records is equivalent to a failure
        this.listener.onResolveComplete(this, this);
        this.listener = null;
      }
      else {
        // resolve the first host name from the list
        this.resolveHost();
      }
    }
    else {
      var rec;
      
      if (iter && (rec = iter.getNext().QueryInterface(ITF_DNSREC))) {         
        while (rec.hasMore()) {
          this.addressArray.push( {
            address: rec.getNextAddrAsString(),
            port: this.hostArray[this.hostIndex].port,
            ttl: this.hostArray[this.hostIndex].ttl
          });
        }
      }
          
      this.dnsQuery = null;
      this.hostIndex++;
      this.resolveHost();
    }
  });

StunDNSRequest.fun(
  function resolveHost() {
    if (this.hostIndex >= this.hostArray.length) {
      // no more servers to resolve
      this.listener.onResolveComplete(this, this);
      this.listener = null;
    }
    else {
      var host = this.hostArray[this.hostIndex].address;
      this.dnsQuery = 
        getDNSService().asyncResolveWithType(host, ITF_DNS.DNS_RECORD_ADDRESS,
                                             0, this, getMainThread());
    }
  });
  
//----------------------------------------------------------------------
// nsISimpleEnumerator implementation:
  
StunDNSRequest.fun(
  function hasMoreElements() {
    if (this.addressIndex >= this.addressArray.length)
      return false;
      
    // make sure we have a valid record to return
    var now = (new Date()).getTime();
    
    for (var ii=this.addressIndex; ii<this.addressArray.length; ++ii) {
      if (this.initTime + (this.addressArray[ii].ttl * 1000)>now)
        return true;
    }
    
    return false;
  }); 
  
StunDNSRequest.fun(
  function getNext() {
    if (this.addressIndex >= this.addressArray.length)
      return null;
      
    var now = (new Date()).getTime();
    
    do {
      var currIndex = this.addressIndex++;
      
      // make sure the record we're about to return hasn't been expired
      if (this.initTime + (this.addressArray[currIndex].ttl * 1000)>now) {
        var transportAddr = TransportAddress.instantiate();
        transportAddr.address = this.addressArray[currIndex].address;
        transportAddr.port = this.addressArray[currIndex].port;
        transportAddr.transport = this.protocol;
        return transportAddr;       
      }
    } while (this.addressIndex < this.addressArray.length);
    
    return null;
  });    
          
////////////////////////////////////////////////////////////////////////
// Class StunDNSResolver
var StunDNSResolver = makeClass("StunDNSResolver", SupportsImpl);
StunDNSResolver.addInterfaces(Components.interfaces.zapIStunDNSResolver);

StunDNSResolver.fun(
  function asyncResolve(hostname, protocol, listener) {
    // implement rfc3489bis section 9. DNS Discovery of a Server

    // try to split the address and port
    var desthost = hostname;
    var destport = 3478;
    var parsed_address = REGEXP_AUTHORITY(hostname);
    if (parsed_address) {
      desthost = parsed_address[1];
      if (parsed_address[2])
        destport = parsed_address[2];
    }
    
    var request = StunDNSRequest.instantiate();
    request.listener = listener;
    request.protocol = protocol;
    request.hostArray.push({address: desthost, port: destport, ttl: MAXINT});

    if (!parsed_address[2]) {
      // no port provided, use SRV
      var domain;
      
      if (protocol == "TLS") {
        domain = "_stuns._tcp.";
        domain += desthost;
      }
      else {
        domain = "_stun._";
        domain += protocol.toLowerCase();
        domain += ".";
        domain += desthost
      }

      request.dnsSRVQuery = 
        getDNSService().asyncResolveWithType(domain, ITF_DNS.DNS_RECORD_SRV, 0,
                                             request, getMainThread());
    }
    else {      
      request.resolveHost();
    }
    
    return request;
  });
  
//----------------------------------------------------------------------
// global StunDNSRequest object:

var theStunDNSResolver = StunDNSResolver.instantiate();
