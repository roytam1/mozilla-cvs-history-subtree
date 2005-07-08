// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SipSyntaxFactory.js')" -*-
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

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");

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
var PARSE_ERROR = "Parse error"; // error to throw when parsing fails


////////////////////////////////////////////////////////////////////////
// Patterns, regexps and tokenizers for parsing various SIP
// constructs:

//----------------------------------------------------------------------
// Basic patterns used to construct regexps and tokenizers below:

// Match any single char (including linebreaks which '.' doesn't match
// by default!)
var PATTERN_ANYTHING = "[^]";

// rfc2234 'HEXDIG'
var PATTERN_HEXDIG = "[0-9AaBbCcDdEeFf]";

// rfc3261 'reserved'
var PATTERN_RESERVED = "[;/?:@&=+$,]";

// rfc3261 'unreserved'
var PATTERN_UNRESERVED = "[A-Za-z0-9\\-_.!~*'()]";

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

// rfc3261 'token'
var PATTERN_TOKEN = "[A-Za-z0-9\\-.!%*_+`'~]+";

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
var PATTERN_USER = "(?:"+PATTERN_UNRESERVED+"|"+
                   PATTERN_ESCAPED+"|[&=+$,;?\\/])+";

// rfc3261 'password'
var PATTERN_PASSWORD = "(?:"+PATTERN_UNRESERVED+"|"+
                       PATTERN_ESCAPED+"|[&=+$,])*";

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
var PATTERN_PARAMCHAR = "(?:[\\[\\]\\/:&+$]|"+ PATTERN_UNRESERVED+"|"+
                        PATTERN_ESCAPED+")";

// rfc3261 '(hnv-unreserved|unreserved|escaped)'
var PATTERN_HEADERCHAR = "(?:[\\[\\]\\/?:+$]|"+PATTERN_UNRESERVED+"|"+
                         PATTERN_ESCAPED+")";

// rfc3261 'display-name'
var PATTERN_DISPLAY_NAME = "(?:(?:"+PATTERN_TOKEN+PATTERN_LWS+")*|"+
                           PATTERN_QUOTED_STRING+")";

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
// XXX PATTERN_ADDR_SPEC only matches (SIP-URI|SIPS-URI), NOT absoluteURI
var PATTERN_ADDR_SPEC = "[Ss][Ii][Pp][Ss]?\\:(?:"+PATTERN_USERINFO+
                        "@)?"+PATTERN_HOST+"(?:\\:"+PATTERN_PORT+")?"+
                        PATTERN_URI_PARAMETERS+"(?:"+PATTERN_URI_HEADERS+")?";

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
var PATTERN_QVALUE = "(?:(?:0(?:\\.\\d{0,3}))|(?:1(?:\\.0{0,3})))";

// rfc3261 'delta-seconds'
var PATTERN_DELTA_SECONDS = "\\d+";

// rfc3261 'callid'
var PATTERN_CALLID = PATTERN_WORD+"(?:@"+PATTERN_WORD+")?";

// rfc3261 'Status-Code'
var PATTERN_STATUS_CODE = "\\d{3}";

// rfc3261 'Reason-Phrase'
var PATTERN_REASON_PHRASE = "(?:"+PATTERN_RESERVED+"|"+
                            PATTERN_UNRESERVED+"|"+
                            PATTERN_ESCAPED+"|"+
                            PATTERN_UTF8_NONASCII+"|"+
                            PATTERN_UTF8_CONT+"|[ \t])*";
  
//----------------------------------------------------------------------
// Regexps used for testing or parsing SIP elements:

// Matches the empty string only:
var REGEXP_NOTHING = new RegExp("^$");

// Match a token:
var REGEXP_TOKEN = new RegExp("^"+PATTERN_TOKEN+"$");

// Tests whether input is a sip response (as opposed to a request):
var REGEXP_IS_RESPONSE = new RegExp("^SIP\\/", "i");

// Match a sip-version:
var REGEXP_SIP_VERSION = new RegExp("^"+PATTERN_SIP_VERSION+"$");

// Match an extension header value, ensuring that any CRLF linebreaks
// are followed by whitespace:
var REGEXP_EXTENSION_HEADER_VALUE = new RegExp("^(?:.|\\r(?!\\n)|\\n|\\r\\n(?=[ \\t]))*$");

// Parse a To/From/Contact message header value into
// (addr-spec|name-addr[parsed], to-parameters[unparsed])
 var REGEXP_NAME_ADDR_PARS = new RegExp("^("+PATTERN_ADDR_SPEC+"|"+
                                        PATTERN_NAME_ADDR+")"+PATTERN_SWS+
                                        "(;"+PATTERN_ANYTHING+"*)?$");

// Parse a Content-Type message header into
// (type[parsed], subtype[parsed], m-paramters[unparsed])
var REGEXP_CONTENT_TYPE_HEADER_VALUE = new RegExp("^("+PATTERN_TOKEN+")"+
                                                  PATTERN_SLASH+"("+
                                                  PATTERN_TOKEN+")"+PATTERN_SWS+
                                                  "(;"+PATTERN_ANYTHING+"*)?$");
                                                  

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

// Matches a userinfo or nothing:
var REGEXP_USERINFO = new RegExp("^(?:"+PATTERN_USERINFO+")?$");

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

// parse a sip(s) uri into (secure?, userinfo, host, port, uri-parameters, headers):
var REGEXP_SIPURI = new RegExp("^[Ss][Ii][Pp]([Ss]?)\\:(?:("+PATTERN_USERINFO+")@)?"+
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

// serializes a hash of generic parameters (header value params,
// uri-params):
function SERIALIZER_PARAMS(hash) {
  var rv = "";
  for (var n in hash) {
    rv += ";" + n.substring(1);
    if (hash[n])
      rv += "=" + hash[n];
  }
  return rv;
}

// serializes a hash of uri-headers:
function SERIALIZER_URI_HEADERS(hash) {
  var rv = "";
  var first = true;
  for (var n in hash) {
    rv += first ? "?" : "&";
    rv += n.substring(1) + "=" + hash[n];
    first = false;
  }    
  return rv;
}

////////////////////////////////////////////////////////////////////////
// Class SipSyntaxObject

var SipSyntaxObject = makeClass("SipSyntaxObject",
                                SupportsImpl, AttributeParser);
SipSyntaxObject.addInterfaces(Components.interfaces.zapISipSyntaxObject);

SipSyntaxObject.metafun(
  "\
  Adds a 'parsed hash'. It consists of an instance variable '_<name>Hash' \n\
  and manipulation methods 'get<name>', 'has<name>', 'set<name>',         \n\
  'remove<name>' and 'get<name>Names', as well as '_deserialize<name>s'   \n\
  and '_serialize<name>s'.                                                \n\
  Keys are canonicized to lower case and will be tested against <re_name>.\n\
  Values will be tested against <re_value> or, if hash <re_hash> contains \n\
  a regular expression for the value's corresponding name, against this   \n\
  latter regular expression.                                              \n\
  For deserialization, <tokenizer> is expected to tokenize data passed to \n\
  '_deserialize<name>s' into (name, value) pairs.                         \n\
  For serialization, <serializer> is a function of one arg (the hash)     \n\
  which should return a serialized string representation of the elements. \n\
  TODO: XXX add escaping filters.                                          ",
  function parsedHash(/*[opt] doc, name, serializer, tokenizer,
                        re_name, re_value, [opt] re_hash*/) {
    // unpack args:
    var i = arguments.length-1;
    var re_hash = {};
    if (typeof(arguments[i]) == "object")
      re_hash = arguments[i--];
    var re_value = arguments[i--];
    var re_name = arguments[i--];
    var tokenizer = arguments[i--];
    var serializer = arguments[i--];
    var name = arguments[i--];
    var doc;
    if (i>=0)
      doc = arguments[i];

    // add a ctor that adds the hash to new instances:
    var hashname = "_"+name+"Hash";
    this.appendCtor(function() { this[hashname] = {}; });

    // construct our 7 hash manipulation functions:
    var get_fct, has_fct, set_fct, remove_fct, getnames_fct, deserialize_fct, serialize_fct;
    
    eval("get_fct = function get"+name+"(n) { return hashget(this[hashname], n.toLowerCase()); }");
    
    eval("has_fct = function has"+name+"(n) { return hashhas(this[hashname], n.toLowerCase()); }");
    
    eval("set_fct = function set"+name+"(n, v) {"+
         "var name = n.toLowerCase();"+
         "if (!re_name.test(name)) this._error(PARSE_ERROR);"+
         "var _re_value = hashget(re_hash, name);"+
         "if (!_re_value) _re_value = re_value;"+
         "if (!v) v = '';"+ // <-- this is to avoid 'null' or 'undefined' to be stringified when applying the regexp
         "if (!_re_value.test(v)) this._error(PARSE_ERROR);"+
         "return hashset(this[hashname], name, v);}");
    
    eval("remove_fct = function remove"+name+"(n) { hashdel(this[hashname], n.toLowerCase()); }");
    
    eval("getnames_fct = function get"+name+"Names(count) {"+
         "var keys = hashkeys(this[hashname]);"+
         "if (count) count.value = keys.length;"+
         "return keys; }");

    eval("deserialize_fct = function _deserialize"+name+"s(data) {"+
         "this[hashname] = {};"+ // clear old hash
         "if (!data) return;"+ // all done; no pars
         "resetTokenizer(tokenizer);"+
         "var match;"+
         "while ((match = tokenizer(data))) {"+
         "  if (this.has"+name+"(match[1]))"+
         "    this._error(PARSE_ERROR);"+
         "  this.set"+name+"(match[1], match[2]);}}");

    eval("serialize_fct = function _serialize"+name+"s() {"+
         "return serializer(this[hashname]);}");
    
    // install functions:
    this.fun(doc, get_fct);
    this.fun(doc, has_fct);
    this.fun(doc, set_fct);
    this.fun(doc, remove_fct);
    this.fun(doc, getnames_fct);
    this.fun(doc, deserialize_fct);
    this.fun(doc, serialize_fct);
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
    if (this._userinfo)
      rv += this._userinfo + "@";
    rv += this._host;
    if (this._port)
      rv += ":" + this._port;
    rv += this._serializeURIParameters();
    rv += this._serializeHeaders();
    return rv;
  });

SipSIPURI.fun(
  function clone() {
    var rv =SipSIPURI.instantiate();
    rv._userinfo= this._userinfo;
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

// attribute ACString userinfo;
SipSIPURI.parsedAttrib("userinfo", REGEXP_USERINFO);

// attribute ACString host;
SipSIPURI.parsedAttrib("host", REGEXP_HOST);

// attribute ACString port;
SipSIPURI.parsedAttrib("port", REGEXP_PORT);

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
  REGEXP_URI_PARAM_NAME,
  REGEXP_URI_PARAM_VALUE,
  { "$transport" : REGEXP_TOKEN,
    "$user"      : REGEXP_TOKEN,
    "$method"    : REGEXP_TOKEN,
    "$ttl"       : REGEXP_TTL,
    "$maddr"     : REGEXP_HOST,
    "$lr"        : REGEXP_NOTHING });

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
  REGEXP_URI_HEADER_NAME,
  REGEXP_URI_HEADER_VALUE);

//----------------------------------------------------------------------

SipSIPURI.fun(
  function deserialize(octets) {
    // parse into (secure?, userinfo, host, port, uri-parameters, headers):
    var matches = REGEXP_SIPURI(octets);
    if (!matches) this._error(PARSE_ERROR);

    if (matches[1].length)
      this.sips = true;
    else
      this.sips = false;
    this._userinfo = matches[2];
    this._host = matches[3];
    this._port = matches[4];
    this._deserializeURIParameters(matches[5]);
    this._deserializeHeaders(matches[6]);
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
  function clone() {
    var rv =SipAddress.instantiate();
    rv._displayName = this._displayName;
    rv.uri = this.uri.clone().QueryInterface(Components.interfaces.zapISipURI);
    return rv;
  });

//----------------------------------------------------------------------
// zapISipAddress implementation:

// attribute AUTF8String displayName;
SipAddress.parsedAttrib("displayName", REGEXP_DISPLAY_NAME);

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
      if (!matches) this._error(PARSE_ERROR);
      this.displayName = matches[1];
      this.uri = theSyntaxFactory.deserializeURI(matches[2]);
    }
    else {
      // an 'addr-spec':
      this.displayName = "";
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
  function clone() {
    var rv =SipToHeader.instantiate();
    rv.address = this.address.clone().QueryInterface(Components.interfaces.zapISipAddress);
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
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$tag" : REGEXP_TOKEN });

//----------------------------------------------------------------------

SipToHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_PARS(data);
    if (!matches) this._error(PARSE_ERROR);
    
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
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE);

//----------------------------------------------------------------------

SipReplyToHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_PARS(data);
    if (!matches) this._error(PARSE_ERROR);
    
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
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE,
  { "$tag" : REGEXP_TOKEN });

//----------------------------------------------------------------------

SipFromHeader.fun(
  function deserialize(name, data) {
    // parse into (name-addr|addr-spec[parsed], to-parameters[unparsed])
    var matches = REGEXP_NAME_ADDR_PARS(data);
    if (!matches) this._error(PARSE_ERROR);
    
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
      var matches = REGEXP_NAME_ADDR_PARS(data);
      if (!matches) this._error(PARSE_ERROR);

      this.address = theSyntaxFactory.deserializeAddress(matches[1]);
      this._deserializeParameters(matches[2]);
    }
  });


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
      this._error(PARSE_ERROR);
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
SipCSeqHeader.parsedAttrib("method", REGEXP_TOKEN); 

// attribute unsigned long sequenceNumber;
SipCSeqHeader.obj("sequenceNumber", 0);

//----------------------------------------------------------------------

SipCSeqHeader.fun(
  function deserialize(name, data) {
    // parse into (sequencenumber, method)
    var matches = REGEXP_CSEQ_HEADER_VALUE(data);
    if (!matches) this._error(PARSE_ERROR);

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
      this._error(PARSE_ERROR);
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
SipContentTypeHeader.parsedAttrib("type", REGEXP_TOKEN);

// attribute ACString subType;
SipContentTypeHeader.parsedAttrib("subType", REGEXP_TOKEN);

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
  REGEXP_TOKEN,
  REGEXP_GEN_VALUE);

//----------------------------------------------------------------------

SipContentTypeHeader.fun(
  function deserialize(name, data) {
    // parse into (type[parsed], subtype[parsed], m-parameters[unparsed])
    var matches = REGEXP_CONTENT_TYPE_HEADER_VALUE(data);
    if (!matches) this._error(PARSE_ERROR);
    
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
SipCallIDHeader.parsedAttrib("callID", REGEXP_CALLID);

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
SipViaHeader.parsedAttrib("protocolName", REGEXP_TOKEN);

// attribute ACString version;
SipViaHeader.parsedAttrib("protocolVersion", REGEXP_TOKEN);

// attribute ACString transport;
SipViaHeader.parsedAttrib("transport", REGEXP_TOKEN);

// attribute ACString host;
SipViaHeader.parsedAttrib("host", REGEXP_HOST);

// attribute ACString port;
SipViaHeader.parsedAttrib("port", REGEXP_PORT);

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
    if (!matches) this._error(PARSE_ERROR);

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

SipAllowHeader.parsedAttrib("method", REGEXP_TOKEN);

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
      this._error(PARSE_ERROR);
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
      this._error(PARSE_ERROR);
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
SipPriorityHeader.parsedAttrib("priority", REGEXP_TOKEN);

//----------------------------------------------------------------------

SipPriorityHeader.fun(
  function deserialize(name, data) {
    this.priority = data;
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
SipUnknownHeader.parsedAttrib("value", REGEXP_EXTENSION_HEADER_VALUE);

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
    arraysplit(function(h) { h.name == name; },
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
    if (headers.length < 1) this._error("Multiple headers of the given name");
    return headers[0];
  });

// zapISipViaHeader getTopViaHeader();
SipMessage.fun(
  function getTopViaHeader() {
    return this.getTopHeader("Via");
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
SipRequest.parsedAttrib("method", REGEXP_TOKEN);

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

//----------------------------------------------------------------------

SipRequest.fun(
  function deserialize(octets) {
    // parse into (method, request-uri, sip-version, headers, body):
    var matches = REGEXP_REQUEST(octets);
    if (!matches) this._error(PARSE_ERROR);
    
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
SipResponse.parsedAttrib("statusCode", REGEXP_STATUS_CODE);

// attribute AUTF8String reasonPhrase;
SipResponse.parsedAttrib("reasonPhrase", REGEXP_REASON_PHRASE);

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
    if (!matches) this._error(PARSE_ERROR);

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
