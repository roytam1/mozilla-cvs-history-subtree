/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SipAuthentication.js')" -*- */
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

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/SipUtils.js");

// name our global object:
function toString() { return "[SipAuthentication.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// Globals

var CryptoHash = Components.classes["@mozilla.org/security/hash;1"].createInstance(Components.interfaces.nsICryptoHash);


////////////////////////////////////////////////////////////////////////
// SipAuthentication

var SipAuthentication = makeClass("SipAuthentication", SupportsImpl);
SipAuthentication.addInterfaces(Components.interfaces.zapISipAuthentication);

//----------------------------------------------------------------------
// zapISipAuthentication

//  boolean addAuthorizationHeaders(in zapISipCredentialsProvider credentials,
//                                  in zapISipResponse response,
//                                  in zapISipRequest request);
SipAuthentication.fun(
  function addAuthorizationHeaders(credentials, response, request) {
    var headersAdded = false;
    headersAdded = this.addAuthorizationHeadersInner("WWW-Authenticate",
                                                     Components.interfaces.zapISipWWWAuthenticateHeader,
                                                     "Authorization",
                                                     Components.interfaces.zapISipAuthorizationHeader,
                                                     credentials,
                                                     response,
                                                     request);
    headersAdded = headersAdded |
      this.addAuthorizationHeadersInner("Proxy-Authenticate",
                                        Components.interfaces.zapISipProxyAuthenticateHeader,
                                        "Proxy-Authorization",
                                        Components.interfaces.zapISipProxyAuthorizationHeader,
                                        credentials,
                                        response,
                                        request);

    return headersAdded;
  });

//----------------------------------------------------------------------
// implementation helpers

// MD5Hex:
// calculate MD5 hash of data and convert to hex representation as
// explained in RFC2617:

var hexDigits = "0123456789abcdef";

function MD5Hex(data) {
  CryptoHash.initWithString("md5");
  if (data) {
    // XXX add update(ACString) method to nsICryptoHash interface so
    // that we don't have to jump through these hoops
    var stream = Components.classes["@mozilla.org/io/string-input-stream;1"].createInstance(Components.interfaces.nsIStringInputStream);
    stream.setData(data, data.length);
    CryptoHash.updateFromStream(stream, data.length);
  }
  var hash = CryptoHash.finish(false);
  var hashHex = "";
  for (var i=0,l=hash.length; i<l; ++i) {
    var code = hash.charCodeAt(i);
    hashHex += hexDigits[code >> 4] + hexDigits[code & 15];
  }
  return hashHex;
}

// remove quotes around data
function unquote(data) {
  if (!data) return "";
  if (data[0] == '"' && data[data.length-1] == '"')
    return data.substring(1, data.length-1);
  dump("warning: SipAuthentication.js::unquote: "+data+" is already unquoted\n");
  return data;
}

// calculate H(A1) as per HTTP Digest spec (RFC2617)
function DigestCalcHA1(alg,
                       userName,
                       realm,
                       password,
                       nonce,
                       cnonce)
{
  var A1 = unquote(userName) + ":" + unquote(realm) + ":" + password;
  var HA1 = MD5Hex(A1);
  if (unquote(alg) == "MD5-sess") {
    A1 = HA1 + ":" + unquote(nonce) + ":" + unquote(cnonce);
    HA1 = MD5Hex(A1);
  }
  return HA1;
}

// calculate request-digest as per HTTP Digest spec (RFC2617)
function DigestCalcResponse(HA1,   // H(A1)
                            nonce, // nonce from server 
                            nonceCount, // 8 hex digits
                            cnonce, // client nonce
                            qop, // qop-value: "", "auth", "auth-int"
                            method, // request method
                            digestURI, // request URI
                            HEntity // H(entity body) if qop="auth-int"
                            )
{
  var unq_qop = unquote(qop);
  
  // calculate H(A2) (RFC2617 3.2.2.3)
  var A2 = method + ":" + unquote(digestURI);
  if  (unq_qop == "auth-int") {
    A2 += ":" + HEntity;
  }
  var HA2 = MD5Hex(A2);

  // calculate response:
  var response = HA1 + ":" + unquote(nonce) + ":";
  if (unq_qop) {
    response += nonceCount + ":" + unquote(cnonce) + ":" + unq_qop + ":";
  }
  response += HA2;
  
  return '"'+MD5Hex(response)+'"';
}

SipAuthentication.fun(
  function addAuthorizationHeadersInner(authenticateHeaderName,
                                        authenticateHeaderInterface,
                                        authorizationHeaderName,
                                        authorizationHeaderInterface,
                                        credentials,
                                        response,
                                        request) {
    var headersAdded = false;

    var requestURI = request.requestURI.serialize();
    var method = request.method;

    var wwwAuthenticateHeaders = response.getHeaders(authenticateHeaderName, {});
    var authorizationHeaders = request.getHeaders(authorizationHeaderName, {});

    for (var i=0, l=wwwAuthenticateHeaders.length; i<l; ++i) {
      var header = wwwAuthenticateHeaders[i].QueryInterface(authenticateHeaderInterface);

      if (header.scheme.toLowerCase() != "digest") continue;
      var realm = header.getParameter("realm");

      // check if there is already an Authorization for the given
      // realm in the request:
      for (var j=0, m=authorizationHeaders.length; i<m; ++i) {
        if (authorizationHeaders[j].QueryInterface(authorizationHeaderInterface).getParameter("realm") == realm) {
          // yes. check if the nonce was stale:
          if (unquote(header.getParameter("stale")).toLowerCase == "stale") {
            // yup. remove authorization header from request and try
            // new credentials again:
            request.removeHeader(authorizationHeaders[j]);
            authorizationHeaders.splice(j, 1);
          }
          else {
            // We've tried the credentials and they were rejected.
            // Don't try them again. If the authorization user has new
            // credentials they should remove the relevant
            // Authorization headers prior to calling addAuthorizationHeaders.
            break;
          }
        }
      }
      if (i<m)
        continue; // the inner loop breaked -> don't retry
                  // authorization for this header

      var algorithm = header.getParameter("algorithm");
      if (!algorithm)
        algorithm = '"MD5"';
      else if (unquote(algorithm).toLowerCase() != "md5")
        continue; // we only support MD5 for the time being
      
      // try to obtain credentials for the given realm:
      var username  = {}, password = {};
      if (!credentials.getCredentialsForRealm(unquote(realm), username, password))
        continue;

      // ok, we have credentials; we understand the challange. let's
      // try to authenticate
      
      var nonce = header.getParameter("nonce");
      var qop = header.getParameter("qop");
      if (qop) {
        if (qop.indexOf("auth-int",0) != -1)
          qop = "auth-int";
        else if (qop.indexOf("auth", 0) != -1)
          qop = "auth";
        else {
          this._warning("unknown qop value: "+qop);
          continue; // can't authenticate
        }
      }
      
      // generate a cnonce:
      var cnonce = '"3213213kjhh223ssdkjh23"';

      // XXX nonce count should be specifiable by the caller:
      var nc = "00000001";
      
      // calculate response:
      var HA1 = DigestCalcHA1('"md5"',
                              username.value,
                              realm,
                              password.value,
                              nonce,
                              cnonce);

      var response = DigestCalcResponse(HA1,
                                        nonce,
                                        nc,
                                        cnonce,
                                        qop,
                                        method,
                                        requestURI,
                                        MD5Hex(request.body));

      var authorizationHeader = gSyntaxFactory.createHeader(authorizationHeaderName).QueryInterface(authorizationHeaderInterface);
      authorizationHeader.scheme = "Digest";
      authorizationHeader.setParameter("username", '"'+username.value+'"');
      authorizationHeader.setParameter("realm", realm);
      this._dump("nonce="+nonce);
      this._dump(header.serialize());
      this._dump(header.getParameter("nonce"));
      authorizationHeader.setParameter("nonce", nonce);
      authorizationHeader.setParameter("uri", '"'+requestURI+'"');
      authorizationHeader.setParameter("response", response);
      if (header.hasParameter("opaque"))
        authorizationHeader.setParameter("opaque", header.getParameter("opaque"));
      if (qop) {
        authorizationHeader.setParameter("qop", qop);
        authorizationHeader.setParameter("cnonce", cnonce);
        authorizationHeader.setParameter("nc", nc);
      }
          
      request.appendHeader(authorizationHeader);

      headersAdded = true;
    }

    return headersAdded;
  });

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP SIP Authentication API",
     cid        : Components.ID("{5e6e20ab-3ebb-43f8-9b60-41adce227ab4}"),
     contractID : "@mozilla.org/zap/sipauth;1",
     factory    : ComponentUtils.generateFactory(function() { return SipAuthentication.instantiate(); })
  }]);

