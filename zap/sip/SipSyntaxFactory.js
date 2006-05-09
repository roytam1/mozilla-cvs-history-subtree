// -*- moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:SipSyntaxFactory.js', null)" -*-
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

debug("*** loading SipSyntaxFactory\n");

Components.utils.importModule("gre:ComponentUtils.jsm");
Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:ArrayUtils.js");
Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:ObjectUtils.js");

// name our global object:
function toString() { return "[SipSyntaxFactory.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// global constants

var SIP_PROTOCOL_NAME = "SIP";
var SIP_PROTOCOL_VERSION = "2.0";
var SIP_VERSION = SIP_PROTOCOL_NAME+"/"+SIP_PROTOCOL_VERSION;
var CRLF = "\r\n";
var PARSE_ERROR = "SIP parse error"; // generic error message for parse failure


////////////////////////////////////////////////////////////////////////
// Charsets, patterns, regexps and tokenizers for parsing various SIP
// constructs:

//----------------------------------------------------------------------
// Basic charsets for constructing patters:

// rfc2234 'WSP'
// var CHARSET_WSP = " \\t"; (inlined)

// rfc3261 'unreserved'
var CHARSET_UNRESERVED = "A-Za-z0-9\\-_.!~*'()";

// rfc3261 'param-unreserved'
var CHARSET_PARAM_UNRESERVED = "\\[\\]\\/:&+$";

// rdf3261 'user-unreserved'
var CHARSET_USER_UNRESERVED = "&=+$,;?\\/";

// characters unreserved in 'password'
var CHARSET_PASSWORD_UNRESERVED = "&=+$,";

//----------------------------------------------------------------------
// Basic patterns used to construct regexps and tokenizers below:

// Match any single char (including linebreaks which '.' doesn't match
// by default!)
var PATTERN_ANYTHING = "[^]";

// rfc2234 'HEXDIG'
var PATTERN_HEXDIG = "[0-9AaBbCcDdEeFf]";

// rfc3261 'reserved'
var PATTERN_RESERVED = "[;/?:@&=+$,]";

// rfc3261 'escaped'
var PATTERN_ESCAPED = "%"+PATTERN_HEXDIG+PATTERN_HEXDIG;

// rfc3261 'LWS'
var PATTERN_LWS = "(?:[ \\t]*\\r\\n)?[ \\t]+";

// rfc3261 'SWS'
var PATTERN_SWS = "(?:"+PATTERN_LWS+")?";

// rfc3261 'HCOLON'
var PATTERN_HCOLON = "[ \\t]*\\:"+PATTERN_SWS;

// rfc3261 'UTF8-CONT'
var PATTERN_UTF8_CONT = "[\\x80-\\xbf]";

// rfc3261 'UTF8-NONASCII'
var PATTERN_UTF8_NONASCII = "(?:[\\xc0-\\xdf]"+PATTERN_UTF8_CONT+"|"+
                            "[\\xe0-\\xef]"+PATTERN_UTF8_CONT+"{2}|"+
                            "[\\xf0-\\xf7]"+PATTERN_UTF8_CONT+"{3}|"+
                            "[\\xf8-\\xfb]"+PATTERN_UTF8_CONT+"{4}|"+
                            "[\\xfc-\\xfd]"+PATTERN_UTF8_CONT+"{5})";

// rfc3261 'TEXT-UTF8char'
var PATTERN_TEXT_UTF8char = "(?:[\\x21-\\x7e]|"+PATTERN_UTF8_NONASCII+")";

// rfc3261 'TEXT-UTF8-TRIM'
var PATTERN_TEXT_UTF8_TRIM = PATTERN_TEXT_UTF8char+
                             "+(?:(?:"+PATTERN_LWS+")*"+PATTERN_TEXT_UTF8char+
                             ")*";

// rfc3261 'token'
var PATTERN_TOKEN = "[A-Za-z0-9\\-.!%*_+`'~]+";

// rfc3265 'token-nodot'
var PATTERN_TOKEN_NODOT = "[A-Za-z0-9\\-!%*_+`'~]+";

// rfc3261 'word'
var PATTERN_WORD = "[A-Za-z0-9\\-.!%*_+`'~()<>:\\\\\"/\\[\\]?{}]+";

// rfc3261 'STAR'
var PATTERN_STAR = PATTERN_SWS+"\\*"+PATTERN_SWS;

// rfc3261 'SLASH'
var PATTERN_SLASH = PATTERN_SWS+"/"+PATTERN_SWS;

// rfc3261 'EQUAL'
var PATTERN_EQUAL = PATTERN_SWS+"="+PATTERN_SWS;

// rfc3261 'RAQUOT'
var PATTERN_RAQUOT = ">"+PATTERN_SWS;

// rfc3261 'LAQUOT'
var PATTERN_LAQUOT = PATTERN_SWS+"<";

// rfc3261 'COMMA'
var PATTERN_COMMA = PATTERN_SWS+","+PATTERN_SWS;

// rfc3261 'SEMI'
var PATTERN_SEMI = PATTERN_SWS+";"+PATTERN_SWS;

// rfc3261 'qdtext'
var PATTERN_QDTEXT = "(?:"+PATTERN_LWS+"|[\\x21\\x23-\\x5b\\x5d-\\x7e]|"+
                     PATTERN_UTF8_NONASCII+")";

// rfc3261 'quoted-pair'
var PATTERN_QUOTED_PAIR = "\\\\[\\x00-\\x09\\x0b-\\x0c\\x0e-\\x7f]";

// rfc3261 'quoted-string'
var PATTERN_QUOTED_STRING = PATTERN_SWS+'\\"(?:'+PATTERN_QDTEXT+'|'+
                            PATTERN_QUOTED_PAIR+')*\\"';

// rfc3261 'SIP-Version'
var PATTERN_SIP_VERSION = "[sS][iI][pP]\\/\\d+\\.\\d+";

// rfc3261 'user'
var PATTERN_USER = "(?:["+CHARSET_USER_UNRESERVED+
                   CHARSET_UNRESERVED+"]|"+
                   PATTERN_ESCAPED+")+";

// rfc3261 'password'
var PATTERN_PASSWORD = "(?:["+CHARSET_PASSWORD_UNRESERVED+
                       CHARSET_UNRESERVED+"]|"+
                       PATTERN_ESCAPED+")*";

// ~ rfc3261 'userinfo'
// XXX PATTERN_USERINFO doesn't include telephone-subscriber yet.
// Also note that, contrary to the ABNF in RFC3261, '@' is NOT
// included in this pattern.
var PATTERN_USERINFO = PATTERN_USER+"(?::"+PATTERN_PASSWORD+")?";

// rfc3261 'hostname'
var PATTERN_HOSTNAME = "(?:(?:[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9]|[A-Za-z0-9])\\.)*"+
                       "(?:[A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z])\\.?";

// rfc3261 'IPv4address'
var PATTERN_IPV4ADDRESS = "\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}";

// rfc3261 'hexseq'
var PATTERN_HEXSEQ = PATTERN_HEXDIG+"{1,4}(?:\\:"+PATTERN_HEXDIG+"{1,4})*";

// rfc3261 'hexpart'
var PATTERN_HEXPART = "(?:"+PATTERN_HEXSEQ+")?(?:\\:\\:(?:"+PATTERN_HEXSEQ+")?)?";

// rfc3261 'IPv6address'
var PATTERN_IPV6ADDRESS = PATTERN_HEXPART+"(?:\\:"+PATTERN_IPV4ADDRESS+")?";

// rfc3261 'IPv6reference'
var PATTERN_IPV6REFERENCE = "\\["+PATTERN_IPV6ADDRESS+"\\]";

// rfc3261 'host'
var PATTERN_HOST = "(?:"+PATTERN_HOSTNAME+"|"+
                   PATTERN_IPV4ADDRESS+"|"+PATTERN_IPV6REFERENCE+")";

// rfc3261 'port'
var PATTERN_PORT = "\\d+";

// rfc3261 'paramchar'
var PATTERN_PARAMCHAR = "(?:["+CHARSET_PARAM_UNRESERVED+
                        CHARSET_UNRESERVED+"]|"+
                        PATTERN_ESCAPED+")";

// rfc3261 '(hnv-unreserved|unreserved|escaped)'
var PATTERN_HEADERCHAR = "(?:[\\[\\]\\/?:+$]|["+CHARSET_UNRESERVED+"]|"+
                         PATTERN_ESCAPED+")";

// rfc3261 'display-name' ammended to the BNF suggested in http://bugs.sipit.net/show_bug.cgi?id=727
var PATTERN_DISPLAY_NAME = "(?:(?:"+PATTERN_TOKEN+
                           "(?:"+PATTERN_LWS+PATTERN_TOKEN+")*)|"+
                           PATTERN_QUOTED_STRING+")";
//var PATTERN_DISPLAY_NAME = "(?:(?:"+PATTERN_TOKEN+PATTERN_LWS+")*|"+
//                           PATTERN_QUOTED_STRING+")";

// ~ rfc3261 'uri-parameter'
// XXX PATTERN_URI_PARAMETER matches uri-parameter, but doesn't check
// syntactic constraints on individual parameters (e.g. parameter
// 'ttl'-value being constrained to 1*3DIGIT, etc.), i.e. it matches
// other-param.
var PATTERN_URI_PARAMETER = "(?:"+PATTERN_PARAMCHAR+"+(?:\\="+
                            PATTERN_PARAMCHAR+"+)?)";

// ~ rfc3261 'uri-parameters'
// XXX constraints on individual parameters aren't checked, see
// PATTERN_URI_PARAMETER
var PATTERN_URI_PARAMETERS = "(?:;"+PATTERN_URI_PARAMETER+")*";

// rfc3261 'header'
var PATTERN_URI_HEADER = PATTERN_HEADERCHAR+"+\\="+PATTERN_HEADERCHAR+"*";

// rfc3261 'headers'
var PATTERN_URI_HEADERS = "\\?"+PATTERN_URI_HEADER+"(?:&"+PATTERN_URI_HEADER+")*";

// ~ rfc3261 'addr-spec'
// XXX PATTERN_ADDR_SPEC and PATTERN_ADDR_SPEC_NO_PARS only matche
// (SIP-URI|SIPS-URI), NOT absoluteURI
var PATTERN_ADDR_SPEC = "[Ss][Ii][Pp][Ss]?\\:(?:"+PATTERN_USERINFO+
                        "@)?"+PATTERN_HOST+"(?:\\:"+PATTERN_PORT+")?"+
                        PATTERN_URI_PARAMETERS+"(?:"+PATTERN_URI_HEADERS+")?";
// 'addr-spec' without semicolon deliminated pars for matching
// 'addr-spec' when it is not enclosed by LAQUOT/RAQUOT. See RFC3261
// 20 last para.
var PATTERN_ADDR_SPEC_NO_PARS = "[Ss][Ii][Pp][Ss]?\\:(?:"+PATTERN_USERINFO+
                         "@)?"+PATTERN_HOST+"(?:\\:"+PATTERN_PORT+")?"+
                         "(?:"+PATTERN_URI_HEADERS+")?";

// rfc3261 'name-addr'
var PATTERN_NAME_ADDR = PATTERN_DISPLAY_NAME+"?"+PATTERN_LAQUOT+
                        PATTERN_ADDR_SPEC+PATTERN_RAQUOT;

// rfc3261 'gen-value'
var PATTERN_GEN_VALUE = "(?:"+PATTERN_TOKEN+"|"+PATTERN_HOST+"|"+
                        PATTERN_QUOTED_STRING+")";

// rfc3261 'generic-param'
var PATTERN_GENERIC_PARAM = PATTERN_TOKEN+"(?:"+PATTERN_EQUAL+
                            PATTERN_GEN_VALUE+")?";

// rfc3261 'qvalue'
var PATTERN_QVALUE = "(?:(?:0(?:\\.\\d{0,3})?)|(?:1(?:\\.0{0,3})?))";

// rfc3261 'delta-seconds'
var PATTERN_DELTA_SECONDS = "\\d+";

// rfc3261 'callid'
var PATTERN_CALLID = PATTERN_WORD+"(?:@"+PATTERN_WORD+")?";

// rfc3261 'Status-Code'
var PATTERN_STATUS_CODE = "\\d{3}";

// rfc3261 'Reason-Phrase'
var PATTERN_REASON_PHRASE = "(?:"+PATTERN_RESERVED+"|["+
                            CHARSET_UNRESERVED+"]|"+
                            PATTERN_ESCAPED+"|"+
                            PATTERN_UTF8_NONASCII+"|"+
                            PATTERN_UTF8_CONT+"|[ \\t])*";

// (token / quoted-string)
var PATTERN_AUTH_VALUE = "(?:"+PATTERN_TOKEN+"|"+PATTERN_QUOTED_STRING+")";

// rfc3265 'event-type'
var PATTERN_EVENTTYPE = "(?:"+PATTERN_TOKEN_NODOT+"(?:\\."+
                        PATTERN_TOKEN_NODOT+")*)";

//----------------------------------------------------------------------
// Regexps used for testing or parsing SIP elements:

// Matches the empty string only:
var REGEXP_NOTHING = new RegExp("^$");

// Trim whitespace either side of an arbitrary string:
var REGEXP_TRIM_WSP = new RegExp("^[ \\t]*([^ \\t]+(?:.*[^ \\t])?)[ \\t]*$");

// Match a token:
var REGEXP_TOKEN = new RegExp("^"+PATTERN_TOKEN+"$");

// Match a sequence of tokens separated by single spaces (used for
// deciding whether to use the token or quoted string forms for
// a display name):
var REGEXP_DISPLAY_NAME_TOKENS = new RegExp("^"+PATTERN_TOKEN+
                                            "(?: "+PATTERN_TOKEN+")*$");

// Tests whether input is a sip response (as opposed to a request):
var REGEXP_IS_RESPONSE = new RegExp("^SIP\\/", "i");

// Match a sip-version:
var REGEXP_SIP_VERSION = new RegExp("^"+PATTERN_SIP_VERSION+"$");

// Match an extension header value, ensuring that any CRLF linebreaks
// are followed by whitespace:
var REGEXP_EXTENSION_HEADER_VALUE = new RegExp("^(?:.|\\r(?!\\n)|\\n|\\r\\n(?=[ \\t]))*$");

// Parse a To/From/Contact message header value into
// (addr-spec|name-addr[parsed], to-parameters[unparsed])
 var REGEXP_NAME_ADDR_SPEC_PARS = new RegExp("^("+PATTERN_ADDR_SPEC_NO_PARS+"|"+
                                             PATTERN_NAME_ADDR+")"+PATTERN_SWS+
                                             "(;"+PATTERN_ANYTHING+"*)?$");

// Parse Route/Record header value into
// (name-addr[parsed], rr-params[unparsed])
 var REGEXP_NAME_ADDR_PARS = new RegExp("^("+PATTERN_NAME_ADDR+")"+PATTERN_SWS+
                                        "(;"+PATTERN_ANYTHING+"*)?$");


// Parse a Content-Type message header into
// (type[parsed], subtype[parsed], m-paramters[unparsed])
var REGEXP_CONTENT_TYPE_HEADER_VALUE = new RegExp("^("+PATTERN_TOKEN+")"+
                                                  PATTERN_SLASH+"("+
                                                  PATTERN_TOKEN+")"+PATTERN_SWS+
                                                  "(;"+PATTERN_ANYTHING+"*)?$");

// Parse a WWW-Authenticate, Proxy-Authenticate, Authorization or
// Proxy-Authorization message header into (scheme[parsed],
// auth-parameters[unparsed])
REGEXP_AUTH_HEADER_VALUE = new RegExp("^("+PATTERN_TOKEN+")"+
                                      PATTERN_LWS+"("+PATTERN_TOKEN+
                                      PATTERN_ANYTHING+"*)$");

// Parses a sip request into (method, request-uri, sip-version,
// unparsed headers, body):
var REGEXP_REQUEST = new RegExp("^("+PATTERN_TOKEN+") ([^ ]*) "+
                                "("+PATTERN_SIP_VERSION+")\\r\\n"+
                                "((?:"+PATTERN_ANYTHING+"*?\\r\\n)*?)\\r\\n"+
                                "("+PATTERN_ANYTHING+"*)$");

// Parses a sip response into (sip-version, status-code,
// reason-phrase, headers, body):
var REGEXP_RESPONSE = new RegExp("^("+PATTERN_SIP_VERSION+") ("+
                                 PATTERN_STATUS_CODE+") ("+
                                 PATTERN_REASON_PHRASE+")\\r\\n"+
                                 "((?:"+PATTERN_ANYTHING+"*?\\r\\n)*?)\\r\\n"+
                                 "("+PATTERN_ANYTHING+"*)$");

// Matches a Status-Code:
var REGEXP_STATUS_CODE = new RegExp("^"+PATTERN_STATUS_CODE+"$");

// Matches a Reason-Phrase:
var REGEXP_REASON_PHRASE = new RegExp("^"+PATTERN_REASON_PHRASE+"$");

// Matches a user:
var REGEXP_USER = new RegExp("^"+PATTERN_USER+"$");

// Matches a password:
var REGEXP_PASSWORD = new RegExp("^"+PATTERN_PASSWORD+"$");

// Matches a host:
var REGEXP_HOST = new RegExp("^"+PATTERN_HOST+"$");

// Matches a port or nothing:
var REGEXP_PORT = new RegExp("^(?:"+PATTERN_PORT+")?$");

// Matches ttl:
var REGEXP_TTL = new RegExp("^\\d{1,3}$");

// Match generic uri-parameters:
var REGEXP_URI_PARAM_NAME = new RegExp("^"+PATTERN_PARAMCHAR+"+$"); // generic param name
var REGEXP_URI_PARAM_VALUE = new RegExp("^"+PATTERN_PARAMCHAR+"*$"); // generic param value (including empty values)

// Match uri headers (i.e. headers in a uri)
var REGEXP_URI_HEADER_NAME = new RegExp("^"+PATTERN_HEADERCHAR+"+$");
var REGEXP_URI_HEADER_VALUE = new RegExp("^"+PATTERN_HEADERCHAR+"*$");

// parse a sip(s) uri into (secure?, user?, :?, password?, host, port?, uri-parameters?, headers?):
var REGEXP_SIPURI = new RegExp("^[Ss][Ii][Pp]([Ss])?\\:(?:("+PATTERN_USER+
                               ")(?:(\\:)("+PATTERN_PASSWORD+"))?@)?"+
                               "("+PATTERN_HOST+")(?:\\:("+PATTERN_PORT+"))?"+
                               "(;[^?]*)?(?:\\?(.*))?$");
                               
// parses a display-name or nothing:
var REGEXP_DISPLAY_NAME = new RegExp("^"+PATTERN_DISPLAY_NAME+"?$");

// parses a name-addr into (display-name[parsed, ?], addr-spec[unparsed]):
var REGEXP_NAME_ADDR = new RegExp("^("+PATTERN_DISPLAY_NAME+"?)"+
                                  PATTERN_LAQUOT+"("+PATTERN_ANYTHING+"+)"+
                                  PATTERN_RAQUOT+"$");

// matches gen-value or nothing:
var REGEXP_GEN_VALUE = new RegExp("^"+PATTERN_GEN_VALUE+"?$");

// matches 'qvalue':
var REGEXP_QVALUE = new RegExp("^"+PATTERN_QVALUE+"$");

// matches 'delta-seconds':
var REGEXP_DELTA_SECONDS = new RegExp("^"+PATTERN_DELTA_SECONDS+"$");

// matches 'STAR':
var REGEXP_STAR = new RegExp("^"+PATTERN_STAR+"$");

// parses a cseq header value into (number, method):
var REGEXP_CSEQ_HEADER_VALUE = new RegExp("^(\\d+)"+PATTERN_LWS+"("+
                                          PATTERN_TOKEN+")");

// matches a 'callid'
var REGEXP_CALLID = new RegExp("^"+PATTERN_CALLID+"$");

// matches (IPv4address|IPv6address), i.e. the value of via-received
var REGEXP_VIA_PARAM_RECEIVED_VALUE = new RegExp("^"+PATTERN_IPV4ADDRESS+
                                                 "|"+PATTERN_IPV6ADDRESS+"$");

// parses a via-parm into (protocol, version, transport, host, port, via-params[unparsed]):
var REGEXP_VIA_HEADER_VALUE = new RegExp("^("+PATTERN_TOKEN+")"+PATTERN_SLASH+
                                         "("+PATTERN_TOKEN+")"+PATTERN_SLASH+
                                         "("+PATTERN_TOKEN+")"+PATTERN_LWS+
                                         "("+PATTERN_HOST+")(?:\\:("+
                                         PATTERN_PORT+"))?"+PATTERN_SWS+
                                         "(;"+PATTERN_ANYTHING+"*)?$");

// Parse a retry-after header value into (delta-seconds, comment[?],
// params[unparsed])
// The matched comment[?] will exclude the outermost parenthesis.
// Unfortunately 'comment' is not regular (it can nest), so we can't
// use regexps here, but we imitate the regexp return value style, i.e.
// the returned object will have members 1,2,3 for the corresponding
// matches.
function PARSE_RETRY_AFTER_HEADER_VALUE(data) {
  var matches = {};
  var l = data.length;
  var m = 0;
  var i = 0;

  function skipSWS() {
    while (i<l) {
      var code = data.charCodeAt(i);
      if (code != 9 && code != 10 && code != 13 && code != 32)
        break; // not whitespace
      ++i;
    }
    m = i;
  }
  
  // match delta-seconds:
  while (i<l) {
    var code = data.charCodeAt(i);
    if (code<48 || code>57)
      break; // not a digit
    ++i;
  }
  if (i==m) return null; // parse error: empty delta-seconds
  matches[1] = data.substring(m,i);

  skipSWS();

  // match comment[?]
  if (data[i] == "(") {
    var level = 1;
    while (++i<l && level!=0) {
      var c = data[i];
      if (c == "(") ++level;
      else if (c == ")") --level;
    }
    if (level!=0) return null; //parse error: unterminated comment
    matches[2] = data.substring(m+1,i-1); 
  }
  else {
    matches[2] = "";
  }
  
  skipSWS();

  // match rest:
  matches[3] = data.substring(m);
  
  return matches;
}

// matches (token / quoted-string):
var REGEXP_AUTH_VALUE = new RegExp("^"+PATTERN_AUTH_VALUE+"$");

// matches TEXT_UTF8_TRIM|nothing:
var REGEXP_TEXT_UTF8_TRIM = new RegExp("^(?:"+PATTERN_TEXT_UTF8_TRIM+")?$");

// matches (server-val *(LWS server-val)):
// XXX implement the proper parsing here
var REGEXP_SERVER_VALS = new RegExp("^.*$");

// matches an RFC3265 'event-type':
var REGEXP_EVENTTYPE = new RegExp("^"+PATTERN_EVENTTYPE+"$");

// parses an RFC3265 Event header into (eventtype,
// event-params[unparsed]):
var REGEXP_EVENT_HEADER_VALUE = new RegExp("^("+PATTERN_EVENTTYPE+")"+
                                           "("+PATTERN_SEMI+
                                           PATTERN_ANYTHING+"*)?$");

// parses an RFC3265 Subscription-State header into (substate,
// subexp-params[unparsed]):
var REGEXP_SUBSTATE_HEADER_VALUE = new RegExp("^("+PATTERN_TOKEN+")"+
                                              "("+PATTERN_SEMI+
                                              PATTERN_ANYTHING+"*)?$");

//----------------------------------------------------------------------
// Tokenizers to be applied repeatedly to input:

function makeTokenizer(re) {
  return new RegExp(re, "g");
}

// Helper to reset a tokenizer:
function resetTokenizer(tokenizer) {
  tokenizer.lastIndex = 0;
}

// Tokenizes a group of headers into (header-name, header-value)
// pairs. Each message header is terminated by CRLF, but care must be
// taken to allow linear whitespace within header-values:
var TOKENIZER_HEADER = makeTokenizer("("+PATTERN_TOKEN+")"+
                                     PATTERN_HCOLON+
                                     "((?:.|\\r(?!\\n)|\\n|(?:\\r\\n(?=[ \\t])))*)\\r\\n");

// Tokenizes a list of uri-parameters into (name, value) pairs:
var TOKENIZER_URI_PARAMS = makeTokenizer(";([^;=]+)(?:\\=([^;]+))?");

// Tokenizes a list of uri-headers into (name, value) pairs:
var TOKENIZER_URI_HEADERS = makeTokenizer("(?:^|&)([^&=]+)\\=([^&]*)");

// Tokenizes a list *(SEMI generic-param) into (name, value) pairs:
var TOKENIZER_GENERIC_PARAMS =
  makeTokenizer(PATTERN_SEMI+"("+PATTERN_TOKEN+")(?:"+PATTERN_EQUAL+
                "("+PATTERN_GEN_VALUE+"))?");

// Tokenizes a list of auth-param separated by COMMA into (name, value) pairs:
// XXX For compatibility with older Asterisk versions we also allow
// trailing whitespace
var TOKENIZER_AUTH_PARAMS =
  makeTokenizer("("+PATTERN_TOKEN+")"+PATTERN_EQUAL+"("+PATTERN_AUTH_VALUE+")(?:"+
                PATTERN_COMMA+"|[ \\t]*$)");

// Tokenizes a comma-separated header value of the form
//   header-value *(COMMA header-value)
// into individual header-values, making sure that whitespace binds
// with COMMA, not with header-value header-value may contain
// ([^,]|quoted_string|<[^>]*>)*
var TOKENIZER_COMMA_SEPARATED_LIST =
  makeTokenizer("((?:[^,]|"+PATTERN_QUOTED_STRING+"|<[^>]*>"+
                ")+?)(?:"+PATTERN_COMMA+"|$)");


////////////////////////////////////////////////////////////////////////
// Escaping/unescaping/serializers:

function ESCAPE_NONE(v) { return v; }
function UNESCAPE_NONE(v) { return v; }

var REGEXP_ESCAPED_HEX = new RegExp(PATTERN_ESCAPED, "g");

function UNESCAPE_HEX(v) {
  // unescape all %XX:
  return v.replace(REGEXP_ESCAPED_HEX, function(p) {
                     return String.fromCharCode(parseInt(p.substring(1), 16));
                   });
}

var REGEXP_QUOTED_PAIR = new RegExp(PATTERN_QUOTED_PAIR, "g");

// unescape a quoted string. the argument must be a trimmed quoted
// string including the quotation marks.
function UNESCAPE_QUOTED_STRING(v) {
  // remove quotation marks:
  v = v.substring(1,v.length-1);
  return utf8ToUnicode(v.replace(REGEXP_QUOTED_PAIR, function(p) {
                                   return p.substring(1);
                                 }));
}

function makeHexEscaper(literal_set) {
  var escapee_re = new RegExp("[^"+literal_set+"]", "g");
  return function(v) {
    return v.replace(escapee_re, function(c) {
                       return "%"+hexCharCodeAt(c, 0);
                     });
  };
}

// pname/pvalue escaping:
var ESCAPE_URI_PARAM = makeHexEscaper(CHARSET_UNRESERVED+
                                      CHARSET_PARAM_UNRESERVED);
// user escaping:
var ESCAPE_USER = makeHexEscaper(CHARSET_UNRESERVED+
                                 CHARSET_USER_UNRESERVED);

// password escaping:
var ESCAPE_PASSWORD = makeHexEscaper(CHARSET_UNRESERVED+
                                     CHARSET_PASSWORD_UNRESERVED);

// regexp containing characters that need to be escaped in a quoted
// string. This is a subset of the valid characters for quoted-pair:
var REGEXP_QUOTED_STRING_ESCAPEE = new RegExp("[\\x00-\\x08\\x0b-\\x0c\\x0e-\\x1F\\x22\\x5c\\x7f]", "g");

function ESCAPE_QUOTED_STRING(v) {
  return '"'+unicodeToUTF8(v).replace(REGEXP_QUOTED_STRING_ESCAPEE,
                                      function(p) { return "\\"+p; })+'"';
}

// serializes a hash of generic parameters (header value params,
// uri-params):
function SERIALIZER_PARAMS(hash) {
  var rv = "";
  for (var n in hash) {
    rv += ";" + hash[n][0];
    if (hash[n][1])
      rv += "=" + hash[n][1];
  }
  return rv;
}

// serialize a hash of auth-params (WWW-Authenticate, Proxy-Authenticate, ...):
function SERIALIZER_AUTH_PARAMS(hash) {
  var rv = "";
  var first = true;
  for (var n in hash) {
    if (!first)
      rv += ",";
    else
      first = false;
    rv += hash[n][0] + "=" + hash[n][1];
  }
  return rv;
}

// serializes a hash of uri-headers:
function SERIALIZER_URI_HEADERS(hash) {
  var rv = "";
  var first = true;
  for (var n in hash) {
    rv += first ? "?" : "&";
    rv += hash[n][0] + "=" + hash[n][1];
    first = false;
  }    
  return rv;
}

////////////////////////////////////////////////////////////////////////
// Class SipSyntaxObject

var SipSyntaxObject = makeClass("SipSyntaxObject",
                                SupportsImpl, AttributeParser, Unwrappable);
SipSyntaxObject.addInterfaces(Components.interfaces.zapISipSyntaxObject);

SipSyntaxObject.metafun(
  "\
  Adds a 'parsed hash'. It consists of an instance variable '_<name>Hash' \n\
  and manipulation methods 'get<name>', 'has<name>', 'set<name>',         \n\
  'remove<name>' and 'get<name>Names', as well as '_deserialize<name>s'   \n\
  and '_serialize<name>s'.                                                \n\
  Parameter names will be parsed against <re_name>.                       \n\
  Values will be parsed against <re_value> or, if hash <re_hash> contains \n\
  a regular expression for the value's corresponding name, against this   \n\
  latter regular expression.                                              \n\
  For deserialization, <tokenizer> is expected to tokenize data passed to \n\
  '_deserialize<name>s' into (name, value) pairs.                         \n\
  For serialization, <serializer> is a function of one arg (the hash)     \n\
  which should return a serialized string representation of the elements. \n\
  The parameter name and value passed to get<name>, has<name> and         \n\
  remove<name> will be escaped using name_escape and value_escape.        \n\
  The parameter names returned by get<name>Names are unescaped using      \n\
  name_unescape.                                                          \n\
  The parameter value returned by get<name> will be unescaped using       \n\
  value_unescape.                                                         \n\
  get<name>, has<name>, remove<name>, set<name> locate paramters case-    \n\
  insensitively. get<name> returns '' if the parameter can't be found.    \n\
  The hash keys are in lower-case, unescaped form (as needed for          \n\
  comparison) and  contain as value an array: [pname, pvalue], where both \n\
  pname and pvalue are in escaped form (as serialized).                    ",
  function parsedHash(/*[opt] doc, name, serializer, tokenizer,
                        name_escape, name_unescape,
                        value_escape, value_unescape,
                        re_name, re_value,  re_hash*/) {
    // unpack args:
    var i = arguments.length-1;
    var re_hash = arguments[i--];
    var re_value = arguments[i--];
    var re_name = arguments[i--];
    var vunescape = arguments[i--];
    var vescape = arguments[i--];
    var nunescape = arguments[i--];
    var nescape = arguments[i--];
    var tokenizer = arguments[i--];
    var serializer = arguments[i--];
    var name = arguments[i--];
    var doc;
    if (i>=0)
      doc = arguments[i];

    // add a ctor that adds the hash to new instances:    
    // the hash is indexed by the canonicized lower-case unescaped
    // pname and contains as value an array: [pname, pvalue] (both
    // escaped as for serialization)
    var hashname = "_"+name+"Hash";
    this.appendCtor(function() { this[hashname] = {}; });

    // construct our 7 hash manipulation functions:
    
    var get_fct = function(pname) {
      var hentry = hashget(this[hashname], pname.toLowerCase());
      if (!hentry ||!hentry[1]) return "";
      return vunescape(hentry[1]);
    };

    var has_fct = function(pname) {
      return hashhas(this[hashname], pname.toLowerCase());
    };

    // 'args_escaped' is a flag to signify escaped pname, pvalue arguments
    // (for internal use by deserialize_fct).
    var set_fct = function(pname, pvalue, args_escaped) {
      var key = args_escaped ? nunescape(pname) : pname;
      key = key.toLowerCase();
      if (!pvalue) pvalue = ''; 
      if (!args_escaped) {
        pname = nescape(pname);
        if (pvalue)
          pvalue = vescape(pvalue);
      }
      if (!re_name.test(pname)) this._verboseError(PARSE_ERROR);
      var _re_value = hashget(re_hash, key);
      if (!_re_value) _re_value = re_value;
      if (!_re_value.test(pvalue)) this._verboseError(PARSE_ERROR);
      return hashset(this[hashname], key, [pname, pvalue]);
    };

    var remove_fct = function(pname) {
      hashdel(this[hashname], pname.toLowerCase());
    };

    // returns unescaped pnames (rather than keys) to maintain case
    var getnames_fct = function(count) {
      var rv = [];
      hashmap(this[hashname],
              function(k,v) {rv.push(nunescape(v[0]));});
      if (count) count.value = rv.length;
      return rv;
    };

    var deserialize_fct = function(data) {
      // clear old hash:
      this[hashname] = {};
      if (!data) return; // all done
      resetTokenizer(tokenizer);
      var match;
      while ((match = tokenizer(data))) {
        if (has_fct.call(this, match[1]))
          this._verboseError(PARSE_ERROR);
        set_fct.call(this, match[1], match[2], true);
      }
    };

    var serialize_fct = function(data) {
      return serializer(this[hashname]);
    };
    
    // install functions:
    this.obj(doc, "get"+name, get_fct);
    this.obj(doc, "has"+name, has_fct);
    this.obj(doc, "set"+name, set_fct);
    this.obj(doc, "remove"+name, remove_fct);
    this.obj(doc, "get"+name+"Names", getnames_fct);
    this.obj(doc, "_deserialize"+name+"s", deserialize_fct);
    this.obj(doc, "_serialize"+name+"s", serialize_fct);
  });

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation:

SipSyntaxObject.stub("serialize", "to be implemented by subclass");

SipSyntaxObject.stub("clone", "to be implemented by subclass");

//----------------------------------------------------------------------
// implementation:

SipSyntaxObject.stub("deserialize", "to be implemented by subclass");


////////////////////////////////////////////////////////////////////////
// Class SipSIPURI
var SipSIPURI = makeClass("SipSIPURI", SipSyntaxObject);
SipSIPURI.addInterfaces(Components.interfaces.zapISipURI,
                        Components.interfaces.zapISipSIPURI);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation:

SipSIPURI.fun(
  function serialize() {
    var rv = "";
    rv += this.sips ? "sips:" : "sip:";
    if (this._user) {
      rv += this._user;
      if (this._password != null)
        rv += ":" + this._password;
      rv += "@";
    }
    rv += this._host;
    if (this._port)
      rv += ":" + this._port;
    rv += this._serializeURIParameters();
    rv += this._serializeHeaders();
    return rv;
  });

SipSIPURI.fun(
  function clone(deep) {
    var rv =SipSIPURI.instantiate();
    rv._user= this._user;
    rv._password = this._password;
    rv._host = this._host;
    rv._port = this._port;
    rv._URIParameterHash = objclone(this._URIParameterHash);
    rv._HeaderHash = objclone(this._HeaderHash);
    return rv;
  });


//----------------------------------------------------------------------
// zapISipSIPURI implementation:

// attribute boolean sips;
SipSIPURI.obj("sips", false);

// attribute ACString user;
SipSIPURI.obj("_user", "");
SipSIPURI.gettersetter(
  "user",
  function get_user() {
    if (!this._user) return "";
    return UNESCAPE_HEX(this._user);
  },
  function set_user(v) {
    if (!v)
      this._user = "";
    else
      this._user = ESCAPE_USER(v);
  });

// attribute ACString password;
SipSIPURI.obj("_password", "");
SipSIPURI.gettersetter(
  "password",
  function get_password() {
    if (!this._password) return this._password; // note that this can be either null or ''
    return UNESCAPE_HEX(this._password);
  },
  function set_password(v) {
    if (!v)
      this._password = v;
    else
      this._password = ESCAPE_PASSWORD(v);
  });

// attribute ACString host;
SipSIPURI.parsedAttrib("host", REGEXP_HOST, null);

// attribute ACString port;
SipSIPURI.parsedAttrib("port", REGEXP_PORT, "");

// ACString getURIParameter(in ACString name);
// boolean hasURIParameter(in ACString name);
// void setURIParameter(in ACString name, in ACString value);
// void removeURIParameter(in ACString name);
// void getURIParameterNames(out unsigned long count,
//                           [retval, array, size_is(count)] out string names);
SipSIPURI.parsedHash(
  "URIParameter",
  SERIALIZER_PARAMS,
  TOKENIZER_URI_PARAMS,
  ESCAPE_URI_PARAM, UNESCAPE_HEX,
  ESCAPE_URI_PARAM, UNESCAPE_HEX,
  REGEXP_URI_PARAM_NAME,
  REGEXP_URI_PARAM_VALUE,
  { "$transport" : REGEXP_TOKEN,
    "$user"      : REGEXP_TOKEN,
    "$method"    : REGEXP_TOKEN,
    "$ttl"       : REGEXP_TTL,
/*  some proxies set lr=on, so we relax the parsing for lr
    "$lr"        : REGEXP_NOTHING, */
    "$maddr"     : REGEXP_HOST
  });

// ACString getHeader(in ACString name);
// boolean hasHeader(in ACString name);
// void setHeader(in ACString name, in ACString value);
// void removeHeader(in ACString name);
// void getHeaderNames(out unsigned long count,
//                     [retval, array, size_is(count)] out string names);
SipSIPURI.parsedHash(
  "Header",
  SERIALIZER_URI_HEADERS,
  TOKENIZER_URI_HEADERS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_URI_HEADER_NAME,
  REGEXP_URI_HEADER_VALUE,
  {});

// boolean equals(in zapISipSIPURI other)
SipSIPURI.fun(
  function equals(other) {
    // compare according to the rules of RFC3261 19.1.4
    if (this.sips != other.sips) return false;
    if (this.user != other.user) return false;
    if (this.password != other.password) return false;
    if (this.host.toLowerCase() != other.host.toLowerCase()) return false;
    if (this.port != other.port) return false;
    // XXX compare URI parameters & headers
    return true;
  });

//----------------------------------------------------------------------

SipSIPURI.fun(
  function deserialize(octets) {
    // parse into (secure?, user?, :?, password?, host, port?, uri-parameters, headers):
    var matches = REGEXP_SIPURI(octets);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed SIP URI ("+octets+")");

    if (matches[1])
      this.sips = true;
    else
      this.sips = false;
    if (matches[2]) {
      this._user = matches[2];
      if (matches[3]) {
        this._password = matches[4];
      }
      else
        this._password = null;
    }
    else {
      this._user = "";
      this._password = null;
    }
    this._host = matches[5];
    this._port = matches[6] ? matches[6] : "";
    this._deserializeURIParameters(matches[7]);
    this._deserializeHeaders(matches[8]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipAddress: a sip name-addr or addr-spec as used in Contact,
// From, Record-Route, Reply-To, Route and To message headers

var SipAddress = makeClass("SipAddress", SipSyntaxObject);
SipAddress.addInterfaces(Components.interfaces.zapISipAddress);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipAddress.fun(
  function serialize() {
    var rv = "";
    // always serialize to addr-spec:
    // (addr-spec is appropriate everywhere name-addr is used, but not
    // vice versa)
    if (this._displayName)
      rv += this._displayName + " ";
    rv += "<" + this.uri.serialize() + ">";
    return rv;
  });

SipAddress.fun(
  function clone(deep) {
    var rv =SipAddress.instantiate();
    rv._displayName = this._displayName;
    if (deep)
      rv.uri = this.uri.clone(true).QueryInterface(Components.interfaces.zapISipURI);
    else
      rv.uri = this.uri;
    return rv;
  });

//----------------------------------------------------------------------
// zapISipAddress implementation:

// attribute AUTF8String displayName;
SipAddress.obj("_displayName", "");
SipAddress.gettersetter(
  "displayName",
  function get_displayName() {
    if (this._displayName && this._displayName[0] == '"') {
      // a quoted string. -> unescape
      return UNESCAPE_QUOTED_STRING(this._displayName);
    }
    // tokens separated by whitespace. -> no need to unescape anything
    return this._displayName;
  },
  function set_displayName(v) {
    if (!v) {
      this._displayName = "";
      return;
    }
    if (REGEXP_DISPLAY_NAME_TOKENS.test(v))
      this._displayName = v; // a sequence of tokens with non-significant ws
    else if (!/[\n\r]/(v)) {
      this._displayName = ESCAPE_QUOTED_STRING(v);
    }
    else
      this._error("Can't have carriage returns or newlines in display name");
  });

// attribute zapISipURI uri;
SipAddress.obj("uri", null);

//----------------------------------------------------------------------

SipAddress.fun(
  "\
 Deserialize from a 'name-addr' or 'addr-spec'.                       ",
  function deserialize(octets) {
    // decide whether we need to parse a 'name-addr' or 'addr-spec'
    // -> 'addr-spec' can't have '<' character, 'name-addr' always will:
    if (octets.indexOf("<") != -1) {
      // a 'name-addr':
      // -> parse into (display-name[parsed,?], addr-spec[unparsed])
      var matches = REGEXP_NAME_ADDR(octets);
      if (!matches) this._verboseError(PARSE_ERROR+": malformed address ("+octets+")");
      this._displayName = matches[1] ? REGEXP_TRIM_WSP(matches[1])[1] : "";
      this.uri = theSyntaxFactory.deserializeURI(matches[2]);
    }
    else {
      // an 'addr-spec':
      this.displayName = null;
      this.uri = theSyntaxFactory.deserializeURI(octets);
    }
  });


////////////////////////////////////////////////////////////////////////
// Class SipHeader : base class for all known SIP header classes

var SipHeader = makeClass("SipHeader", SipSyntaxObject);
SipSyntaxObject.addInterfaces(Components.interfaces.zapISipHeader);

// Global hash [lower case header names] -> [implementing class]. It
// will be filled in by class method SipHeader.setAliases()
var gKnownHeaders = {};

SipHeader.metafun(
  "\
 Set the canonical name and aliases of this header. The aliases (incl.\n\
 canonical name) will be entered lowercase into the global hash       \n\
 'gKnownHeaders', which is used by SipMessage to lookup header classes\n\
 for given header field names during parsing.                         \n\
 The canonical name (first parameter) will also be set as             \n\
 property 'canonicalName' on the class and 'name' on the prototype.   ",
  function setAliases(/* canonical name, alias1, alias2, ... */) {
    this.canonicalName = this.prototype.name = arguments[0];
    for (var i=0, l=arguments.length; i<l; ++i) {
      var alias = arguments[i].toLowerCase();
      this._assert(!gKnownHeaders[alias],
                   "We already have a header with alias "+alias);
      // insert into gKnownHeaders:
      gKnownHeaders[alias] = this;
    }
  });

SipHeader.metaobj(
  "\
 Determines whether this header class follows the grammar form       \n\
   header = 'header-name' HCOLON header-value *(COMMA header-value)  \n\
 which allows for combining header fields of the same name into a    \n\
 comma-separated list (see RFC3261 - 7.3)                            \n\
 If 'true', the message header deserialization process will tokenize \n\
 the comma-separated header-value list and create a new header object\n\
 for each separate header-value.                                     \n\
 If 'false', the message header deserialization process will create  \n\
 just one header object for the given header field and pass all data \n\
 after the HCOLON to the header object for deserialization.           ",
  "isCommaSeparatedList", false);

//----------------------------------------------------------------------
// zapISipHeader implementation:

// readonly attribute ACString name;
// This field will be initialized by setAliases() to the default for
// the given subclass
SipHeader.obj("name", null);


////////////////////////////////////////////////////////////////////////
// Class SipToHeader

var SipToHeader = makeClass("SipToHeader", SipHeader);
SipToHeader.addInterfaces(Components.interfaces.zapISipToHeader);
SipToHeader.setAliases("To", "t");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipToHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this.address.serialize();
    rv += this._serializeParameters();
    return rv;
  });

SipToHeader.fun(
  function clone(deep) {
    var rv =SipToHeader.instantiate();
    if (deep)
      rv.address = this.address.clone(true).QueryInterface(Components.interfaces.zapISipAddress);
    else
      rv.address = this.address;
    rv._ParameterHash = objclone(this._ParameterHash);
    return rv;
  });

//----------------------------------------------------------------------
// zapISipToHeader implementation

// attribute zapISipAddress address;
SipToHeader.obj("address", null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipToHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$tag" : REGEXP_TOKEN });

//----------------------------------------------------------------------

SipToHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_SPEC_PARS(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed To Header ("+data+")");
    
    this.address = theSyntaxFactory.deserializeAddress(matches[1]);
    this._deserializeParameters(matches[2]);
  });

////////////////////////////////////////////////////////////////////////
// Class SipReplyToHeader

var SipReplyToHeader = makeClass("SipReplyToHeader", SipHeader);
SipReplyToHeader.addInterfaces(Components.interfaces.zapISipReplyToHeader);
SipReplyToHeader.setAliases("Reply-To");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipReplyToHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this.address.serialize();
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipReplyToHeader implementation

// attribute zapISipAddress address;
SipReplyToHeader.obj("address", null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipReplyToHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  {});

//----------------------------------------------------------------------

SipReplyToHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_SPEC_PARS(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed Reply-To header ("+data+")");
    
    this.address = theSyntaxFactory.deserializeAddress(matches[1]);
    this._deserializeParameters(matches[2]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipFromHeader

var SipFromHeader = makeClass("SipFromHeader", SipHeader);
SipFromHeader.addInterfaces(Components.interfaces.zapISipFromHeader);
SipFromHeader.setAliases("From", "f");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipFromHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this.address.serialize();
    rv += this._serializeParameters();
    return rv;
  });


//----------------------------------------------------------------------
// zapISipFromHeader implementation

// attribute zapISipAddress address;
SipFromHeader.obj("address", null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipFromHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$tag" : REGEXP_TOKEN });

//----------------------------------------------------------------------

SipFromHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_SPEC_PARS(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed From header ("+data+")");
    
    this.address = theSyntaxFactory.deserializeAddress(matches[1]);
    this._deserializeParameters(matches[2]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipContactHeader

var SipContactHeader = makeClass("SipContactHeader", SipHeader);
SipContactHeader.addInterfaces(Components.interfaces.zapISipContactHeader);
SipContactHeader.setAliases("Contact", "m");
SipContactHeader.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipContactHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    if (this.wildcard)
      rv += "*";
    else {
      rv += this.address.serialize();
      rv += this._serializeParameters();
    }
    return rv;
  });

//----------------------------------------------------------------------
// zapISipContactHeader implementation

// attribute boolean wildcard;
SipContactHeader.obj("_wildcard", false);
SipContactHeader.gettersetter(
  "wildcard",
  function get_wildcard() {
    return this._wildcard;
  },
  function set_wildcard(val) {
    if (val) {
      this._address = null;
      this._ParameterHash = {};
    }
    return this._wildcard = val;
  });

// attribute zapISipAddress address;
SipContactHeader.obj("_address", null);
SipContactHeader.gettersetter(
  "address",
  function get_address() {
    return this._address;
  },
  function set_address(addr) {
    if (addr)
      this._wildcard = false;
    return this._address = addr;
  });

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
// XXX clear 'wildcard' when setting parameters
SipContactHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$q" : REGEXP_QVALUE,
    "$expires" : REGEXP_DELTA_SECONDS });

//----------------------------------------------------------------------

SipContactHeader.fun(
  function deserialize(name, data) {
    if (REGEXP_STAR.test(data)) {
      this.wildcard = true;
    }
    else {
      // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
      var matches = REGEXP_NAME_ADDR_SPEC_PARS(data);
      if (!matches) this._verboseError(PARSE_ERROR+": malformed Contact header ("+data+")");

      this.address = theSyntaxFactory.deserializeAddress(matches[1]);
      this._deserializeParameters(matches[2]);
    }
  });

////////////////////////////////////////////////////////////////////////
// Class SipRouteHeaderBase: base class for 'Route'-like headers
// (Route, Record-Route, Path, Service-Route)

var SipRouteHeaderBase = makeClass("SipRouteHeaderBase", SipHeader);
SipRouteHeaderBase.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipRouteHeaderBase.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this.address.serialize();
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
//zapISipRouteHeader, zapISipRecordRouteHeader, zapISipPathHeader,
//zapISipServiceRouteHeader implementation

// attribute zapISipAddress address;
SipRouteHeaderBase.obj("address", null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipRouteHeaderBase.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  {});

//----------------------------------------------------------------------

SipRouteHeaderBase.fun(
  function deserialize(name, data) {
    // parse into (name-addr[parsed], parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_PARS(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed "+this.name+" header ("+data+")");
    
    this.address = theSyntaxFactory.deserializeAddress(matches[1]);
    this._deserializeParameters(matches[2]);
  });

////////////////////////////////////////////////////////////////////////
// Class SipRouteHeader

var SipRouteHeader = makeClass("SipRouteHeader", SipRouteHeaderBase);
SipRouteHeader.addInterfaces(Components.interfaces.zapISipRouteHeader);
SipRouteHeader.setAliases("Route");

////////////////////////////////////////////////////////////////////////
// Class SipRecordRouteHeader

var SipRecordRouteHeader = makeClass("SipRecordRouteHeader",
                                     SipRouteHeaderBase);
SipRecordRouteHeader.addInterfaces(Components.interfaces.zapISipRecordRouteHeader);
SipRecordRouteHeader.setAliases("Record-Route");

////////////////////////////////////////////////////////////////////////
// Class SipPathHeader (RFC3327)

var SipPathHeader = makeClass("SipPathHeader",
                              SipRouteHeaderBase);
SipPathHeader.addInterfaces(Components.interfaces.zapISipPathHeader);
SipPathHeader.setAliases("Path");

////////////////////////////////////////////////////////////////////////
// Class SipServiceRouteHeader (RFC3608)

var SipServiceRouteHeader = makeClass("SipServiceRouteHeader",
                                      SipRouteHeaderBase);
SipServiceRouteHeader.addInterfaces(Components.interfaces.zapISipServiceRouteHeader);
SipServiceRouteHeader.setAliases("Service-Route");

////////////////////////////////////////////////////////////////////////
// Class SipMaxForwardsHeader

var SipMaxForwardsHeader = makeClass("SipMaxForwardsHeader", SipHeader);
SipMaxForwardsHeader.addInterfaces(Components.interfaces.zapISipMaxForwardsHeader);
SipMaxForwardsHeader.setAliases("Max-Forwards");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipMaxForwardsHeader.fun(
  function serialize() {
    return this.name + ": " + this.maxForwards;
  });

//----------------------------------------------------------------------
// zapISipMaxForwardsHeader implementation

// attribute unsigned long maxForwards;
SipMaxForwardsHeader.obj("maxForwards", 70);

//----------------------------------------------------------------------

SipMaxForwardsHeader.fun(
  function deserialize(name, data) {
    if (isNaN(this.maxForwards = parseInt(data)))
      this._verboseError(PARSE_ERROR+": malformed Max-Forwards header ("+data+")");
  });


////////////////////////////////////////////////////////////////////////
// Class SipCSeqHeader

var SipCSeqHeader = makeClass("SipCSeqHeader", SipHeader);
SipCSeqHeader.addInterfaces(Components.interfaces.zapISipCSeqHeader);
SipCSeqHeader.setAliases("CSeq");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipCSeqHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": " + this.sequenceNumber + " ";
    rv += this._method;
    return rv;
  });

//----------------------------------------------------------------------
// zapISipCSeqHeader implementation

// attribute ACString method;
SipCSeqHeader.parsedAttrib("method", REGEXP_TOKEN, null); 

// attribute unsigned long sequenceNumber;
SipCSeqHeader.obj("sequenceNumber", 0);

//----------------------------------------------------------------------

SipCSeqHeader.fun(
  function deserialize(name, data) {
    // parse into (sequencenumber, method)
    var matches = REGEXP_CSEQ_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed CSeq header ("+data+")");

    this.sequenceNumber = parseInt(matches[1]);
    this._method = matches[2];
  });

////////////////////////////////////////////////////////////////////////
// Class SipContentLengthHeader

var SipContentLengthHeader = makeClass("SipContentLengthHeader", SipHeader);
SipContentLengthHeader.addInterfaces(Components.interfaces.zapISipContentLengthHeader);
SipContentLengthHeader.setAliases("Content-Length", "l");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipContentLengthHeader.fun(
  function serialize() {
    return this.name + ": " + this.contentLength;
  });

//----------------------------------------------------------------------
// zapISipContentLengthHeader implementation

// attribute unsigned long contentLength;
SipContentLengthHeader.obj("contentLength", 70);

//----------------------------------------------------------------------

SipContentLengthHeader.fun(
  function deserialize(name, data) {
    // XXX parsing is not strict
    if (isNaN(this.contentLength = parseInt(data)) || this.contentLength<0)
      this._verboseError(PARSE_ERROR+": malformed Content-Length header ("+data+")");
  });


////////////////////////////////////////////////////////////////////////
// Class SipContentTypeHeader

var SipContentTypeHeader = makeClass("SipContentTypeHeader", SipHeader);
SipContentTypeHeader.addInterfaces(Components.interfaces.zapISipContentTypeHeader);
SipContentTypeHeader.setAliases("Content-Type", "c");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipContentTypeHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": " + this._type + "/" + this._subType;
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipContentTypeHeader implementation

// attribute ACString type;
SipContentTypeHeader.parsedAttrib("type", REGEXP_TOKEN, null);

// attribute ACString subType;
SipContentTypeHeader.parsedAttrib("subType", REGEXP_TOKEN, null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipContentTypeHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  {});

//----------------------------------------------------------------------

SipContentTypeHeader.fun(
  function deserialize(name, data) {
    // parse into (type[parsed], subtype[parsed], m-parameters[unparsed])
    var matches = REGEXP_CONTENT_TYPE_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed Content-Type header ("+data+")");
    
    this._type = matches[1];
    this._subType = matches[2];
    this._deserializeParameters(matches[3]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipCallIDHeader

var SipCallIDHeader = makeClass("SipCallIDHeader", SipHeader);
SipCallIDHeader.addInterfaces(Components.interfaces.zapISipCallIDHeader);
SipCallIDHeader.setAliases("Call-ID", "i");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipCallIDHeader.fun(
  function serialize() {
    return this.name + ": " + this._callID;
  });

//----------------------------------------------------------------------
// zapISipCallIDHeader implementation

// attribute ACString callID;
SipCallIDHeader.parsedAttrib("callID", REGEXP_CALLID, null);

//----------------------------------------------------------------------

SipCallIDHeader.fun(
  function deserialize(name, data) {
    this.callID = data;
  });


////////////////////////////////////////////////////////////////////////
// Class SipViaHeader

var SipViaHeader = makeClass("SipViaHeader", SipHeader);
SipViaHeader.addInterfaces(Components.interfaces.zapISipViaHeader);
SipViaHeader.setAliases("Via", "v");
SipViaHeader.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipViaHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this._protocolName + "/" + this._protocolVersion + "/" + this._transport + " ";
    rv += this._host;
    if (this._port)
      rv += ":" + this._port;
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipViaHeader implementation

// attribute ACString protocol;
SipViaHeader.parsedAttrib("protocolName", REGEXP_TOKEN, null);

// attribute ACString version;
SipViaHeader.parsedAttrib("protocolVersion", REGEXP_TOKEN, null);

// attribute ACString transport;
SipViaHeader.parsedAttrib("transport", REGEXP_TOKEN, null);

// attribute ACString host;
SipViaHeader.parsedAttrib("host", REGEXP_HOST, null);

// attribute ACString port;
SipViaHeader.parsedAttrib("port", REGEXP_PORT, null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipViaHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$ttl" : REGEXP_TTL,
    "$maddr" : REGEXP_HOST,
    "$received" : REGEXP_VIA_PARAM_RECEIVED_VALUE,
    "$branch" : REGEXP_TOKEN });


//----------------------------------------------------------------------

SipViaHeader.fun(
  function deserialize(name, data) {
    // parse into (protocol, version, transport, host, port, via-params[unparsed])
    var matches = REGEXP_VIA_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed Via header ("+data+")");

    this._protocolName = matches[1];
    this._protocolVersion = matches[2];
    this._transport = matches[3];
    this._host = matches[4];
    this._port = matches[5];
    this._deserializeParameters(matches[6]);
  });

////////////////////////////////////////////////////////////////////////
// Class SipAllowHeader

var SipAllowHeader = makeClass("SipAllowHeader", SipHeader);
SipAllowHeader.addInterfaces(Components.interfaces.zapISipAllowHeader);
SipAllowHeader.setAliases("Allow");
SipAllowHeader.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipAllowHeader.fun(
  function serialize() {
    return this.name + ": " + this._method;
  });

//----------------------------------------------------------------------
// zapISipAllowHeader implementation

SipAllowHeader.parsedAttrib("method", REGEXP_TOKEN, null);

//----------------------------------------------------------------------

SipAllowHeader.fun(
  function deserialize(name, data) {
    this.method = data;
  });

////////////////////////////////////////////////////////////////////////
// Class SipExpiresHeader

var SipExpiresHeader = makeClass("SipExpiresHeader", SipHeader);
SipExpiresHeader.addInterfaces(Components.interfaces.zapISipExpiresHeader);
SipExpiresHeader.setAliases("Expires");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipExpiresHeader.fun(
  function serialize() {
    return this.name + ": " + this.deltaSeconds;
  });

//----------------------------------------------------------------------
// zapISipExpiresHeader implementation

// attribute unsigned long deltaSeconds;
SipExpiresHeader.obj("deltaSeconds", 0);

//----------------------------------------------------------------------

SipExpiresHeader.fun(
  function deserialize(name, data) {
    // XXX parsing is not strict
    if (isNaN(this.deltaSeconds = parseInt(data)) || this.deltaSeconds<0)
      this._verboseError(PARSE_ERROR+": malformed Expires header ("+data+")");
  });

////////////////////////////////////////////////////////////////////////
// Class SipMinExpiresHeader

var SipMinExpiresHeader = makeClass("SipMinExpiresHeader", SipHeader);
SipMinExpiresHeader.addInterfaces(Components.interfaces.zapISipMinExpiresHeader);
SipMinExpiresHeader.setAliases("Min-Expires");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipMinExpiresHeader.fun(
  function serialize() {
    return this.name + ": " + this.deltaSeconds;
  });

//----------------------------------------------------------------------
// zapISipMinExpiresHeader implementation

// attribute unsigned long deltaSeconds;
SipMinExpiresHeader.obj("deltaSeconds", 0);

//----------------------------------------------------------------------

SipMinExpiresHeader.fun(
  function deserialize(name, data) {
    // XXX parsing is not strict
    if (isNaN(this.deltaSeconds = parseInt(data)) || this.deltaSeconds<0)
      this._verboseError(PARSE_ERROR+": malformed Min-Expires header ("+data+")");
  });

////////////////////////////////////////////////////////////////////////
// Class SipPriorityHeader

var SipPriorityHeader = makeClass("SipPriorityHeader", SipHeader);
SipPriorityHeader.addInterfaces(Components.interfaces.zapISipPriorityHeader);
SipPriorityHeader.setAliases("Priority");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipPriorityHeader.fun(
  function serialize() {
    return this.name + ": " + this.priority;
  });

//----------------------------------------------------------------------
// zapISipPriorityHeader implementation

// attribute ACString priority;
SipPriorityHeader.parsedAttrib("priority", REGEXP_TOKEN, null);

//----------------------------------------------------------------------

SipPriorityHeader.fun(
  function deserialize(name, data) {
    this.priority = data;
  });

////////////////////////////////////////////////////////////////////////
// Class SipOptionTagHeaderBase: baseclass for SipRequireHeader,
// SipProxyRequireHeader, SipSupportedHeader and SipUnsupportedHeader

var SipOptionTagHeaderBase = makeClass("SipOptionTagHeaderBase", SipHeader);
SipOptionTagHeaderBase.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipOptionTagHeaderBase.fun(
  function serialize() {
    return this.name + ": " + this._optionTag;
  });

//----------------------------------------------------------------------
// zapISipRequireHeader, zapISipProxyRequireHeader, zapISipSupportedHeader,
// SipUnsupportedHeader implementation

SipOptionTagHeaderBase.parsedAttrib("optionTag", REGEXP_TOKEN, null);

//----------------------------------------------------------------------

SipOptionTagHeaderBase.fun(
  function deserialize(name, data) {
    this.optionTag = data;
  });

////////////////////////////////////////////////////////////////////////
// Class SipRequireHeader

var SipRequireHeader = makeClass("SipRequireHeader", SipOptionTagHeaderBase);
SipRequireHeader.addInterfaces(Components.interfaces.zapISipRequireHeader);
SipRequireHeader.setAliases("Require");

////////////////////////////////////////////////////////////////////////
// Class SipProxyRequireHeader

var SipProxyRequireHeader = makeClass("SipProxyRequireHeader",
                                      SipOptionTagHeaderBase);
SipProxyRequireHeader.addInterfaces(Components.interfaces.zapISipProxyRequireHeader);
SipProxyRequireHeader.setAliases("Proxy-Require");

////////////////////////////////////////////////////////////////////////
// Class SipSupportedHeader

var SipSupportedHeader = makeClass("SipSupportedHeader",
                                   SipOptionTagHeaderBase);
SipSupportedHeader.addInterfaces(Components.interfaces.zapISipSupportedHeader);
SipSupportedHeader.setAliases("Supported", "k");

////////////////////////////////////////////////////////////////////////
// Class SipUnsupportedHeader

var SipUnsupportedHeader = makeClass("SipUnsupportedHeader",
                                     SipOptionTagHeaderBase);
SipUnsupportedHeader.addInterfaces(Components.interfaces.zapISipUnsupportedHeader);
SipUnsupportedHeader.setAliases("Unsupported");



////////////////////////////////////////////////////////////////////////
// Class SipAuthHeaderBase : baseclass for Proxy-Authenticate,
// WWW-Authenticate, Authorization and Proxy-Authorization headers

var SipAuthHeaderBase = makeClass("SipAuthHeaderBase", SipHeader);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipAuthHeaderBase.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": " + this._scheme + " ";
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipWWWAuthenticateHeader/zapISipProxyAuthenticateHeader/
// zapISipAuthorizationHeader/zapISipProxyAuthorizationHeader
// implementation:

// attribute ACString type;
SipAuthHeaderBase.parsedAttrib("scheme", REGEXP_TOKEN, "Digest");

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipAuthHeaderBase.parsedHash(
  "Parameter",
  SERIALIZER_AUTH_PARAMS,
  TOKENIZER_AUTH_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_AUTH_VALUE,
  {});

//----------------------------------------------------------------------

SipAuthHeaderBase.fun(
  function deserialize(name, data) {
    // parse into (scheme[parsed], auth-parameters[unparsed])
    var matches = REGEXP_AUTH_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed "+this.name+" header ("+data+")");
    
    this._type = matches[1];
    this._deserializeParameters(matches[2]);
  });

////////////////////////////////////////////////////////////////////////
// Class SipAuthorizationHeader

var SipAuthorizationHeader = makeClass("SipAuthorizationHeader",
                                       SipAuthHeaderBase);
SipAuthorizationHeader.addInterfaces(Components.interfaces.zapISipAuthorizationHeader);
SipAuthorizationHeader.setAliases("Authorization");

////////////////////////////////////////////////////////////////////////
// Class SipWWWAuthenticateHeader

var SipWWWAuthenticateHeader = makeClass("SipWWWAuthenticateHeader",
                                         SipAuthHeaderBase);
SipWWWAuthenticateHeader.addInterfaces(Components.interfaces.zapISipWWWAuthenticateHeader);
SipWWWAuthenticateHeader.setAliases("WWW-Authenticate");

////////////////////////////////////////////////////////////////////////
// Class SipProxyAuthorizationHeader

var SipProxyAuthorizationHeader = makeClass("SipProxyAuthorizationHeader",
                                            SipAuthHeaderBase);
SipProxyAuthorizationHeader.addInterfaces(Components.interfaces.zapISipProxyAuthorizationHeader);
SipProxyAuthorizationHeader.setAliases("Proxy-Authorization");

////////////////////////////////////////////////////////////////////////
// Class SipProxyAuthenticateHeader

var SipProxyAuthenticateHeader = makeClass("SipProxyAuthenticateHeader",
                                           SipAuthHeaderBase);
SipProxyAuthenticateHeader.addInterfaces(Components.interfaces.zapISipProxyAuthenticateHeader);
SipProxyAuthenticateHeader.setAliases("Proxy-Authenticate");


////////////////////////////////////////////////////////////////////////
// Class SipSubjectHeader

var SipSubjectHeader = makeClass("SipSubjectHeader", SipHeader);
SipSubjectHeader.addInterfaces(Components.interfaces.zapISipSubjectHeader);
SipSubjectHeader.setAliases("Subject", "s");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipSubjectHeader.fun(
  function serialize() {
    return this.name + ": " + this.subject;
  });

//----------------------------------------------------------------------
// zapISipPriorityHeader implementation

// attribute AUTF8String subject;
SipSubjectHeader.parsedAttrib("subject", REGEXP_TEXT_UTF8_TRIM, null);

//----------------------------------------------------------------------

SipSubjectHeader.fun(
  function deserialize(name, data) {
    this.subject = data;
  });

////////////////////////////////////////////////////////////////////////
// Class SipOrganizationHeader

var SipOrganizationHeader = makeClass("SipOrganizationHeader", SipHeader);
SipOrganizationHeader.addInterfaces(Components.interfaces.zapISipOrganizationHeader);
SipOrganizationHeader.setAliases("Organization");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipOrganizationHeader.fun(
  function serialize() {
    return this.name + ": " + this.organization;
  });

//----------------------------------------------------------------------
// zapISipOrganizationHeader implementation

// attribute AUTF8String organization;
SipOrganizationHeader.parsedAttrib("organization",
                                   REGEXP_TEXT_UTF8_TRIM, null);

//----------------------------------------------------------------------

SipOrganizationHeader.fun(
  function deserialize(name, data) {
    this.organization = data;
  });


////////////////////////////////////////////////////////////////////////
// Class SipServerHeader

var SipServerHeader = makeClass("SipServerHeader", SipHeader);
SipServerHeader.addInterfaces(Components.interfaces.zapISipServerHeader);
SipServerHeader.setAliases("Server");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipServerHeader.fun(
  function serialize() {
    return this.name + ": " + this.userAgent;
  });

//----------------------------------------------------------------------
// zapISipServerHeader implementation

// attribute AUTF8String organization;
SipServerHeader.parsedAttrib("userAgent",
                                REGEXP_SERVER_VALS, null);

//----------------------------------------------------------------------

SipServerHeader.fun(
  function deserialize(name, data) {
    this.userAgent = data;
  });


////////////////////////////////////////////////////////////////////////
// Class SipUserAgentHeader

var SipUserAgentHeader = makeClass("SipUserAgentHeader", SipHeader);
SipUserAgentHeader.addInterfaces(Components.interfaces.zapISipUserAgentHeader);
SipUserAgentHeader.setAliases("User-Agent");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipUserAgentHeader.fun(
  function serialize() {
    return this.name + ": " + this.userAgent;
  });

//----------------------------------------------------------------------
// zapISipUserAgentHeader implementation

// attribute AUTF8String organization;
SipUserAgentHeader.parsedAttrib("userAgent",
                                REGEXP_SERVER_VALS, null);

//----------------------------------------------------------------------

SipUserAgentHeader.fun(
  function deserialize(name, data) {
    this.userAgent = data;
  });


////////////////////////////////////////////////////////////////////////
// Class SipEventHeader : RFC3265 'Event' header

var SipEventHeader = makeClass("SipEventHeader", SipHeader);
SipEventHeader.addInterfaces(Components.interfaces.zapISipEventHeader);
SipEventHeader.setAliases("Event", "o");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipEventHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this._eventType;
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipEventHeader implementation

// attribute ACString eventType;
SipEventHeader.parsedAttrib("eventType", REGEXP_EVENTTYPE, null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipEventHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$id" : REGEXP_TOKEN });

//----------------------------------------------------------------------

SipEventHeader.fun(
  function deserialize(name, data) {
    // parse into (eventtype, event-params[unparsed])
    var matches = REGEXP_EVENT_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed Event header ("+data+")");

    this._eventType = matches[1];
    this._deserializeParameters(matches[2]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipAllowEventsHeader : RFC3265 'Allow-Events' header

var SipAllowEventsHeader = makeClass("SipAllowEventsHeader", SipHeader);
SipAllowEventsHeader.addInterfaces(Components.interfaces.zapISipAllowEventsHeader);
SipAllowEventsHeader.setAliases("Allow-Events", "u");
SipAllowEventsHeader.metaobj("isCommaSeparatedList", true);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipAllowEventsHeader.fun(
  function serialize() {
    return this.name + ": " + this._eventType;
  });

//----------------------------------------------------------------------
// zapISipAllowEventsHeader implementation

// attribute ACString eventType;
SipAllowEventsHeader.parsedAttrib("eventType", REGEXP_EVENTTYPE, null);

//----------------------------------------------------------------------

SipAllowEventsHeader.fun(
  function deserialize(name, data) {
    this.eventType = data;
  });


////////////////////////////////////////////////////////////////////////
// Class SipSubscriptionStateHeader: RFC3265 'Subscription-State'

var SipSubscriptionStateHeader = makeClass("SipSubscriptionStateHeader",
                                           SipHeader);
SipSubscriptionStateHeader.addInterfaces(Components.interfaces.zapISipSubscriptionStateHeader);
SipSubscriptionStateHeader.setAliases("Subscription-State");

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipSubscriptionStateHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": ";
    rv += this._subState;
    rv += this._serializeParameters();
    return rv;
  });

//----------------------------------------------------------------------
// zapISipSubscriptionStateHeader implementation

// attribute ACString subState;
SipSubscriptionStateHeader.parsedAttrib("subState", REGEXP_TOKEN, null);

// ACString getParameter(in ACString name);
// boolean hasParameter(in ACString name);
// void setParameter(in ACString name, in ACString value);
// void removeParameter(in ACString name);
// void getParameterNames(out unsigned long count,
//                        [retval, array, size_is(count)] out string names);
SipSubscriptionStateHeader.parsedHash(
  "Parameter",
  SERIALIZER_PARAMS,
  TOKENIZER_GENERIC_PARAMS,
  ESCAPE_NONE, UNESCAPE_NONE,
  ESCAPE_NONE, UNESCAPE_NONE,
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$reason" : REGEXP_TOKEN,
    "$expires" : REGEXP_DELTA_SECONDS,
    "$retry-after" : REGEXP_DELTA_SECONDS  });

//----------------------------------------------------------------------

SipSubscriptionStateHeader.fun(
  function deserialize(name, data) {
    // parse into (substate, subexp-params[unparsed])
    var matches = REGEXP_SUBSTATE_HEADER_VALUE(data);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed Subscription-State header ("+data+")");

    this._subState = matches[1];
    this._deserializeParameters(matches[2]);
  });


////////////////////////////////////////////////////////////////////////
// Class SipUnknownHeader : header class used for header types unknown
// to the implementation

var SipUnknownHeader = makeClass("SipUnknownHeader", SipHeader);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation

SipUnknownHeader.fun(
  function serialize() {
    var rv = "";
    rv += this.name + ": "+this.value;
    return rv;
  });

//----------------------------------------------------------------------
// zapISipUnknownHeader implementation:

// attribute AUTF8String value;
SipUnknownHeader.parsedAttrib("value", REGEXP_EXTENSION_HEADER_VALUE, null);

//----------------------------------------------------------------------

SipUnknownHeader.fun(
  function deserialize(name, data) {
    this.name = name;
    this._value = utf8ToUnicode(data);
  });


////////////////////////////////////////////////////////////////////////
// Classs SipMessage

var SipMessage = makeClass("SipMessage", SipSyntaxObject);
SipMessage.addInterfaces(Components.interfaces.zapISipMessage);

SipMessage.appendCtor(
  function() {
    // _headers is an array of all message headers. Its elements are
    // unparsed structures  { name: 'canonical headername',
    //                        data: 'raw header data' }
    // or parsed zapISipHeader objects which will be constructed lazily
    // when asked for.
    this._headers = [];
  });

//----------------------------------------------------------------------
// zapISipMessage implementation:

// attribute boolean isRequest;
// SipMessage.obj("isRequest", null); <- implemented by subclass

// attribute ACString version;
SipMessage.parsedAttrib("version", REGEXP_SIP_VERSION, SIP_VERSION);

// readonly attribute unsigned long headerCount;
SipMessage.getter(
  "headerCount",
  function get_headerCount() {
    return this._headers.length;
  });

// AUTF8String headerNameAt(in unsigned long i);
SipMessage.fun(
  function headerNameAt(i) {
    return this._headers[i].name;
  });

// zapISipHeader headerAt(in unsigned long i);
SipMessage.fun(
  function headerAt(i) {
    this._ensureHeaderParsed(i);
    return this._headers[i];
  });

// void appendHeader(in zapISipHeader header);
SipMessage.fun(
  function appendHeader(header) {
    this._headers.push(header);
  });

// void insertHeaderBefore(in zapISipHeader header, in unsigned long i);
SipMessage.fun(
  function insertHeaderBefore(header, i) {
    this._headers.splice(i, 0, header);
  });

// void removeHeaderAt(in unsigned long i);
SipMessage.fun(
  function removeHeaderAt(i) {
    this._headers.splice(i, 1);
  });

// void removeHeader(in zapISipHeader header);
SipMessage.fun(
  function removeHeader(header) {
    var headers = this._headers;
    // unwrap inner object:
    header = header.wrappedJSObject;
    this._assert(header, "Invalid argument");
    
    for (var i=0,l=headers.length; i<l; ++i) {
      if (headers[i].wrappedJSObject == header) {
        this.removeHeaderAt(i);
        return;
      }
    }
    // not found
    this._warning("header "+header+" ("+header.name+") not found");
  });

// attribute ACString body;
SipMessage.obj("body", "");

// void getHeaders(in ACString name, out unsigned long count,
//                 [retval, array, size_is(count)] out zapISipHeader headers);
SipMessage.fun(
  function getHeaders(name, count) {
    var headers = this._headers;
    var rv = [];
    for (var i=0,l=headers.length; i<l; ++i) {
      if (headers[i].name == name) {
        this._ensureHeaderParsed(i);
        rv.push(headers[i]);
      }
    }
    if (count)
      count.value = rv.length;
    return rv;
  });

// void getHeaderIndices(in ACString name, out unsigned long count,
//                       [retval, array, size_is(count)] out unsigned long indices);
SipMessage.fun(
  function getHeaderIndices(name, count) {
    var headers = this._headers;
    var rv = [];
    for (var i=0,l=headers.length; i<l; ++i) {
      if (headers[i].name == name)
        rv.push(i);
    }
    if (count)
      count.value = rv.length;
    return rv;
  });

// void removeHeaders(in ACString name);
SipMessage.fun(
  function removeHeaders(name) {
    arraysplit(function(h) { return h.name == name; },
               this._headers);
  });

// zapISipHeader getTopHeader(in ACString name);
SipMessage.fun(
  function getTopHeader(name) {
    var headers = this._headers;
    for (var i=0,l=headers.length; i<l; ++i) {
      if (headers[i].name == name) {
        this._ensureHeaderParsed(i);
        return headers[i];
      }
    }
    return null;
  });

// zapISipHeader getSingleHeader(in ACString name);
SipMessage.fun(
  function getSingleHeader(name) {
    var headers = this.getHeaders(name);
    if (headers.length == 0) return null;
    if (headers.length < 1) this._verboseError(PARSE_ERROR+": Message contains multiple "+name+" headers");
    return headers[0];
  });

// zapISipViaHeader getTopViaHeader();
SipMessage.fun(
  function getTopViaHeader() {
    return this.getTopHeader("Via");
  });

// zapISipRouteHeader getTopRouteHeader();
SipMessage.fun(
  function getTopRouteHeader() {
    return this.getTopHeader("Route");
  });

// zapISipContactHeader getTopContactHeader();
SipMessage.fun(
  function getTopContactHeader() {
    return this.getTopHeader("Contact");
  });

// zapISipCallIDHeader getCallIDHeader();
SipMessage.fun(
  function getCallIDHeader() {
    return this.getSingleHeader("Call-ID");
  });

// zapISipToHeader getToHeader();
SipMessage.fun(
  function getToHeader() {
    return this.getSingleHeader("To");
  });

// zapISipFromHeader getFromHeader();
SipMessage.fun(
  function getFromHeader() {
    return this.getSingleHeader("From");
  });

// zapISipCSeqHeader getCSeqHeader();
SipMessage.fun(
  function getCSeqHeader() {
    return this.getSingleHeader("CSeq");
  });

// void setContent(in ACString contentType, in ACString contentSubType,
//                 in ACString bodyContent);
SipMessage.fun(
  function setContent(contentType, contentSubType, bodyContent) {
    this.body = bodyContent;
    this.removeHeaders("Content-Type");
    this.appendHeader(theSyntaxFactory.createContentTypeHeader(contentType,
                                                               contentSubType));
    this.removeHeaders("Content-Length");
    this.appendHeader(theSyntaxFactory.createContentLengthHeader(bodyContent.length));
  });


//----------------------------------------------------------------------
// implementation helpers:

SipMessage.fun(
  function _ensureHeaderParsed(i) {
    if (this._headers[i].QueryInterface) return; // already parsed
    
    // try our list of known headers first:
    var headerClass = gKnownHeaders[this._headers[i].name.toLowerCase()];
    if (!headerClass) {
      // XXX try extension modules before constructing SipUnknownHeader
      headerClass = SipUnknownHeader;
    }
    var header = headerClass.instantiate();
    header.deserialize(this._headers[i].name, this._headers[i].data);
    this._headers[i] = header;
  });

// _deserializeHeaders expects a string of header fields, with each
// field terminated by CRLF. Header fields may contain linear
// whitespace.
// Known headers with 'comma-separated list' syntax will be parsed into
// individual header objects for each item in the list.
// The names of known header types will be canonicized.
SipMessage.fun(
  function _deserializeHeaders(hdata) {
    // clear headers:
    this._headers = [];
    
    resetTokenizer(TOKENIZER_HEADER);
    var match;
    while ((match = TOKENIZER_HEADER(hdata))) {
      var hname = match[1];
      var hvalue = match[2];
      
      // canonicize header name 
      var headerClass = gKnownHeaders[hname.toLowerCase()];
      if (headerClass)
        hname = headerClass.canonicalName;
      
      // check if this header class implements 'comma-separated list'
      // syntax:
      if (headerClass && headerClass.isCommaSeparatedList) {
        // yes -> tokenize further; push individual header objects for list
        // items
        resetTokenizer(TOKENIZER_COMMA_SEPARATED_LIST);
        var match2;
        while ((match2 = TOKENIZER_COMMA_SEPARATED_LIST(hvalue))) {
          this._headers.push({name:hname, data:match2[1]});
        }
      }
      else {
        // no -> just push opaque hvalue
        this._headers.push({name:hname, data:hvalue});
      }
    }
  });

SipMessage.fun(
  function _serializeHeaders() {
    var rv = "";
    this._headers.forEach(
      function(h) {
        if (h.serialize)
          rv += h.serialize();
        else {
          rv += h.name + ": " + h.data;
        }
        rv += CRLF;
      });
    return rv;
  });

SipMessage.fun(
  function _serializeBody() {
    return this.body;
  });


////////////////////////////////////////////////////////////////////////
// Class SipRequest

var SipRequest = makeClass("SipRequest", SipMessage);
SipRequest.addInterfaces(Components.interfaces.zapISipRequest);

//----------------------------------------------------------------------
// zapISipMessage implementation:

// attribute boolean isRequest;
SipRequest.obj("isRequest", true);

//----------------------------------------------------------------------
// zapISipRequest implementation:

// attribute ACString method;
SipRequest.parsedAttrib("method", REGEXP_TOKEN, null);

// attribute zapISipURI requestURI;
SipMessage.obj("requestURI", null);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation:

SipRequest.fun(
  function serialize() {
    var rv = "";
    rv += this.method + " " + this.requestURI.serialize() + " " + this._version + CRLF;
    rv += this._serializeHeaders();
    rv += CRLF;
    rv += this._serializeBody();
    return rv;
  });

SipRequest.fun(
  function clone(deep) {
    this._assert(!deep, "can't do deep request copy! write me!");
    
    var rv =SipRequest.instantiate();
    rv._method = this._method;
    rv.requestURI = this.requestURI;
    rv._version = this._version;
    rv._headers = arrayclone(this._headers);
    rv.body = this.body;
    return rv;
  });


//----------------------------------------------------------------------

SipRequest.fun(
  function deserialize(octets) {
    // parse into (method, request-uri, sip-version, headers, body):
    var matches = REGEXP_REQUEST(octets);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed SIP request (\n"+octets+"\n)");
    
    this._method = matches[1];
    this.requestURI = theSyntaxFactory.deserializeURI(matches[2]);
    this._version = matches[3];
    this._deserializeHeaders(matches[4]);
    this.body = matches[5];
  });


////////////////////////////////////////////////////////////////////////
// Class SipResponse

var SipResponse = makeClass("SipResponse", SipMessage);
SipResponse.addInterfaces(Components.interfaces.zapISipResponse);

//----------------------------------------------------------------------
// zapISipMessage implementation:

// attribute boolean isRequest;
SipResponse.obj("isRequest", false);

//----------------------------------------------------------------------
// zapISipResponse implementation:

// attribute ACString statusCode;
SipResponse.parsedAttrib("statusCode", REGEXP_STATUS_CODE, null);

// attribute AUTF8String reasonPhrase;
SipResponse.parsedAttrib("reasonPhrase", REGEXP_REASON_PHRASE, null);

//----------------------------------------------------------------------
// zapISipSyntaxObject implementation:

SipResponse.fun(
  function serialize() {
    var rv = "";
    rv += this._version + " " + this._statusCode + " " + this._reasonPhrase + CRLF;
    rv += this._serializeHeaders();
    rv += CRLF;
    rv += this._serializeBody();
    return rv;
  });

//----------------------------------------------------------------------

SipResponse.fun(
  function deserialize(octets) {
    // parse into (sip-version, status-code, reason-phrase, headers, body):
    var matches = REGEXP_RESPONSE(octets);
    if (!matches) this._verboseError(PARSE_ERROR+": malformed SIP response (\n"+octets+"\n)");

    this._version = matches[1];
    this._statusCode = matches[2];
    this._reasonPhrase = matches[3];
    this._deserializeHeaders(matches[4]);
    this.body = matches[5];
  });



////////////////////////////////////////////////////////////////////////
// Class SipSyntaxFactory

var SipSyntaxFactory = makeClass("SipSyntaxFactory", SupportsImpl);
SipSyntaxFactory.addInterfaces(Components.interfaces.zapISipSyntaxFactory);

//----------------------------------------------------------------------
// zapISipSyntaxFactory implementation:

// zapISipMessage deserializeMessage(in ACString octets);
SipSyntaxFactory.fun(
  function deserializeMessage(octets) {
    var message;
    if (REGEXP_IS_RESPONSE.test(octets))
      message = SipResponse.instantiate();
    else
      message = SipRequest.instantiate();

    message.deserialize(octets);
    return message;
  });

// zapISipURI deserializeURI(in ACString octets);
SipSyntaxFactory.fun(
  function deserializeURI(octets) {
    // XXX we only know about sip and sips uris for the time being:
    var uri = SipSIPURI.instantiate();
    uri.deserialize(octets);
    return uri;
  });

// zapISipAddress deserializeAddress(in ACString octets);
SipSyntaxFactory.fun(
  function deserializeAddress(octets) {
    var addr = SipAddress.instantiate();
    addr.deserialize(octets);
    return addr;
  });

// zapISipHeader deserializeHeader(in ACString headerName, in ACString headerValue);
SipSyntaxFactory.fun(
  function deserializeHeader(headerName, headerValue) {
    var h = this.createHeader(headerName);
    h.deserialize(headerName, headerValue);
    return h;
  });

// void deserializeRouteSet(in ACString octets, out unsigned long count,
//                          [array, retval, size_is(count)] out
//                          zapISipAddress routeset);
SipSyntaxFactory.fun(
  function deserializeRouteSet(octets, count) {
    var rv = [];
    if (octets.length) {
      resetTokenizer(TOKENIZER_COMMA_SEPARATED_LIST);
      var match;
      while((match = TOKENIZER_COMMA_SEPARATED_LIST(octets))) {
        var matches = REGEXP_NAME_ADDR_PARS(match[1]);
        if (!matches) this._verboseError(PARSE_ERROR+": malformed route set");
        rv.push(this.deserializeAddress(matches[1]));
      }
    }
    
    if (count)
      count.value = rv.length;
    return rv;
  });

// zapISipRequest createRequest();
SipSyntaxFactory.fun(
  function createRequest() {
    return SipRequest.instantiate();
  });

// zapISipResponse createResponse();
SipSyntaxFactory.fun(
  function createResponse() {
    return SipResponse.instantiate();
  });

// zapISipAddress createAddress(in AUTF8String displayName,
//                              in zapISipURI uri);
SipSyntaxFactory.fun(
  function createAddress(displayName, uri) {
    var address = SipAddress.instantiate();
    address.displayName = displayName;
    address.uri = uri;
    return address;
  });

// zapISipHeader createHeader(in ACString name);
SipSyntaxFactory.fun(
  function createHeader(name) {
    var headerClass = gKnownHeaders[name.toLowerCase()];
    if (!headerClass)
      headerClass = SipUnknownHeader;
    return headerClass.instantiate();
  });

// zapISipToHeader createToHeader(in zapISipAddress address);
SipSyntaxFactory.fun(
  function createToHeader(address) {
    var h = SipToHeader.instantiate();
    h.address = address;
    return h;
  });

// zapISipFromHeader createFromHeader(in zapISipAddress address);
SipSyntaxFactory.fun(
  function createFromHeader(address) {
    var h = SipFromHeader.instantiate();
    h.address = address;
    return h;
  });

// zapISipRouteHeader createRouteHeader(in zapISipAddress address);
SipSyntaxFactory.fun(
  function createRouteHeader(address) {
    var h = SipRouteHeader.instantiate();
    h.address = address;
    return h;
  });

// zapISipCallIDHeader createCallIDHeader(in ACString callID);
SipSyntaxFactory.fun(
  function createCallIDHeader(callID) {
    var h = SipCallIDHeader.instantiate();
    h.callID = callID;
    return h;
  });

// zapISipCSeqHeader createCSeqHeader(in ACString method,
//                                    in unsigned long sequenceNumber);
SipSyntaxFactory.fun(
  function createCSeqHeader(method, sequenceNumber) {
    var h = SipCSeqHeader.instantiate();
    h.method = method;
    h.sequenceNumber = sequenceNumber;
    return h;
  });

// zapISipMaxForwardsHeader createMaxForwardsHeader();
SipSyntaxFactory.fun(
  function createMaxForwardsHeader() {
    return SipMaxForwardsHeader.instantiate();
  });

// zapISipViaHeader createViaHeader();
SipSyntaxFactory.fun(
  function createViaHeader() {
    var h = SipViaHeader.instantiate();
    h.protocolName = SIP_PROTOCOL_NAME;
    h.protocolVersion = SIP_PROTOCOL_VERSION;
    return h;
  });

// zapISipContentTypeHeader createContentTypeHeader(in ACString contentType,
//                                                  in ACString contentSubType);
SipSyntaxFactory.fun(
  function createContentTypeHeader(contentType, contentSubType) {
    var h = SipContentTypeHeader.instantiate();
    h.type = contentType;
    h.subType = contentSubType;
    return h;
  });

// zapISipContentLengthHeader createContentLengthHeader(in unsigned long contentLength);
SipSyntaxFactory.fun(
  function createContentLengthHeader(contentLength) {
    var h = SipContentLengthHeader.instantiate();
    h.contentLength = contentLength;
    return h;
  });

// zapISipContactHeader createContactHeader(in zapISipAddress address);
SipSyntaxFactory.fun(
  function createContactHeader(address) {
    var h = SipContactHeader.instantiate();
    h.address = address;
    return h;
  });

// zapISipRequireHeader createRequireHeader(in ACString optionTag);
SipSyntaxFactory.fun(
  function createRequireHeader(optionTag) {
    var h = SipRequireHeader.instantiate();
    h.optionTag = optionTag;
    return h;
  });

// zapISipUnsupportedHeader createUnsupportedHeader(in ACString optionTag);
SipSyntaxFactory.fun(
  function createUnsupportedHeader(optionTag) {
    var h = SipUnsupportedHeader.instantiate();
    h.optionTag = optionTag;
    return h;
  });

// zapISipAllowHeader createAllowHeader(in ACString method);
SipSyntaxFactory.fun(
  function createAllowHeader(method) {
    var h = SipAllowHeader.instantiate();
    h.method = method;
    return h;
  });

// hash of RFC3261/3265 status code -> reason phrase
var ReasonPhraseHash = {
  "$100" : "Trying",
  "$180" : "Ringing",
  "$181" : "Call Is Being Forwarded",
  "$182" : "Queued",
  "$183" : "Session Progress",
  "$200" : "OK",
  "$202" : "Accepted", // RFC3265
  "$300" : "Multiple Choices",
  "$301" : "Moved Permanently",
  "$302" : "Moved Temporarily",
  "$305" : "Use Proxy",
  "$380" : "Alternative Service",
  "$400" : "Bad Request",
  "$401" : "Unauthorized",
  "$402" : "Payment Required",
  "$403" : "Forbidden",
  "$404" : "Not Found",
  "$405" : "Method Not Allowed",
  "$406" : "Not Acceptable",
  "$407" : "Proxy Authentication Required",
  "$408" : "Request Timeout",
  "$410" : "Gone",
  "$413" : "Request Entity Too Large",
  "$414" : "Request-URI Too Long",
  "$415" : "Unsupported Media Type",
  "$416" : "Unsupported URI Scheme",
  "$420" : "Bad Extension",
  "$421" : "Extension Required",
  "$423" : "Interval Too Brief",
  "$480" : "Temporarily Unavailable",
  "$481" : "Call/Transaction Does Not Exist",
  "$482" : "Loop Detected",
  "$483" : "Too Many Hops",
  "$484" : "Address Incomplete",
  "$485" : "Ambiguous",
  "$486" : "Busy Here",
  "$487" : "Request Terminated",
  "$488" : "Not Acceptable Here",
  "$489" : "Bad Event",
  "$491" : "Request Pending",
  "$493" : "Undecipherable",
  "$500" : "Server Internal Error",
  "$501" : "Not Implemented",
  "$502" : "Bad Gateway",
  "$503" : "Service Unavailable",
  "$504" : "Server Time-out",
  "$505" : "Version Not Supported",
  "$513" : "Message Too Large",
  "$600" : "Busy Everywhere",
  "$603" : "Decline",
  "$604" : "Does Not Exist Anywhere",
  "$606" : "Not Acceptable"
};

// ACString getStandardReasonPhrase(in ACString statusCode);
SipSyntaxFactory.fun(
  function getStandardReasonPhrase(statusCode) {
    return hashget(ReasonPhraseHash, statusCode);
  });

//----------------------------------------------------------------------
// global SipSyntaxFactory object:

var theSyntaxFactory = SipSyntaxFactory.instantiate();


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP SIP Syntax Factory",
     cid        : Components.ID("{a8e60fb1-5880-4392-95b4-404ab52ea323}"),
     contractID : "@mozilla.org/zap/sipsyntaxfactory;1",
     factory    : ComponentUtils.generateFactory(function() { return theSyntaxFactory; })
  }]);
