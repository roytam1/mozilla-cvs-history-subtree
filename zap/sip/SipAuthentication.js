/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipAuthentication.js', null)" -*- */
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

debug("*** loading SipAuthentication.js\n");

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");

// name our global object:
// function toString() { return "[SipAuthentication.js]"; }

////////////////////////////////////////////////////////////////////////
// SipAuthentication

var SipAuthentication = makeClass("SipAuthentication", SupportsImpl);
SipAuthentication.addInterfaces(Components.interfaces.zapISipAuthentication);

//----------------------------------------------------------------------
// zapISipAuthentication

//  boolean addAuthorizationHeaders(in zapISipCredentialsProvider credentials,
//                                  in zapISipResponse response,
//                                  in zapISipRequest request,
//                                  in unsigned long authFlags);
SipAuthentication.fun(
  function addAuthorizationHeaders(credentials, response,
                                   request, authFlags) {
    var headersAdded = false;
    headersAdded = this.addAuthorizationHeadersInner("WWW-Authenticate",
                                                     Components.interfaces.zapISipWWWAuthenticateHeader,
                                                     "Authorization",
                                                     Components.interfaces.zapISipAuthorizationHeader,
                                                     credentials,
                                                     response,
                                                     request,
                                                     authFlags);
    headersAdded = headersAdded |
      this.addAuthorizationHeadersInner("Proxy-Authenticate",
                                        Components.interfaces.zapISipProxyAuthenticateHeader,
                                        "Proxy-Authorization",
                                        Components.interfaces.zapISipProxyAuthorizationHeader,
                                        credentials,
                                        response,
                                        request,
                                        authFlags);

    return headersAdded;
  });

//----------------------------------------------------------------------
// implementation helpers

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
                                        request,
                                        authFlags) {
    var headersAdded = false;

    var requestURI = request.requestURI.serialize();
    var method = request.method;

    var authenticateHeaders = response.getHeaders(authenticateHeaderName, {});
    var authorizationHeaders = request.getHeaders(authorizationHeaderName, {});

    for (var i=0, l=authenticateHeaders.length; i<l; ++i) {
      var header = authenticateHeaders[i].QueryInterface(authenticateHeaderInterface);

      if (header.scheme.toLowerCase() != "digest") continue;
      var realm = header.getParameter("realm");

      // check if there is already an Authorization for the given
      // realm in the request:
      var hasRejectedCredentials = false;      
      for (var j=0, m=authorizationHeaders.length; j<m; ++j) {
        if (authorizationHeaders[j].QueryInterface(authorizationHeaderInterface).getParameter("realm") == realm) {
          // yes, we have an authorization header already. check
          // whether the credentials were rejected or whether the
          // nonce was just stale:
          var hasRejectedCredentials = (unquote(header.getParameter("stale")).toLowerCase() != "true");

          if (hasRejectedCredentials &&
              (authFlags&Components.interfaces.zapISipAuthentication.ASTERISK_STALE_NONCE_HACK)) {
            // compare nonces to determine hasRejectedCredentials
            if (header.getParameter("nonce") !=
                authorizationHeaders[j].getParameter("nonce"))
              hasRejectedCredentials = false;
          }
          
          // remove authorization header from request and try
          // new credentials again:
          request.removeHeader(authorizationHeaders[j]);
          authorizationHeaders.splice(j, 1);
          break;
        }
      }

      var algorithm = header.getParameter("algorithm");
      if (!algorithm)
        algorithm = '"MD5"';
      else if (unquote(algorithm).toLowerCase() != "md5")
        continue; // we only support MD5 for the time being
      
      // try to obtain credentials for the given realm:
      var username  = {}, password = {};
      if (!credentials.getCredentialsForRealm(unquote(realm), username, password, hasRejectedCredentials))
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

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [{ className  : "ZAP SIP Authentication API",
     cid        : Components.ID("{5e6e20ab-3ebb-43f8-9b60-41adce227ab4}"),
     contractID : "@mozilla.org/zap/sipauth;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipAuthentication.instantiate(); })
  }]);

