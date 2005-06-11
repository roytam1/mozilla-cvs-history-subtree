// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.mozIJSComponentLib).probeComponent('rel:SdpService.js', true)" -*-
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


debug("*** loading SdpService\n");

importModule("resource:/jscodelib/JSComponentUtils.js");
importModule("resource:/jscodelib/zap/ClassUtils.js");
importModule("resource:/jscodelib/zap/ArrayUtils.js");
importModule("resource:/jscodelib/zap/StringUtils.js");
importModule("resource:/jscodelib/zap/ObjectUtils.js");

// name our global object:
function toString() { return "[SdpService.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// global constants

var CRLF = "\r\n";
var PARSE_ERROR = "Parse error"; // error to throw when parsing fails

////////////////////////////////////////////////////////////////////////
// Patterns, regexps and tokenizers for parsing various SDP
// constructs:

//----------------------------------------------------------------------
// Basic patterns used to construct regexps and tokenizers below:

// rfc2234 'HEXDIG'
var PATTERN_HEXDIG = "[0-9AaBbCcDdEeFf]";

// rfc2234 'VCHAR'
var PATTERN_VCHAR = "[\\x21-\\x7e]";

// draft-ietf-mmusic-sdp-new-24.txt 'text'
var PATTERN_TEXT = "[\\x01-\\x09\\x0b\\x0c\\x0e-\\xff]+";

// draft-ietf-mmusic-sdp-new-24.txt 'non-ws-string' (similar to
// rfc2327 'safe')
var PATTERN_NON_WS_STRING = PATTERN_VCHAR + "+";

// draft-ietf-mmusic-sdp-new-24.txt 'token'
var PATTERN_TOKEN = "[\\x21\\x23-\\x27\\x2a\\x2b\\x2d\\x2e\\x30-\\x39\\x41-\\x5a\\x5e-\\x7e]+";

// draft-ietf-mmusic-sdp-new-24.txt 'decimal-uchar'
var PATTERN_DECIMAL_UCHAR = "(?:\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])";

// draft-ietf-mmusic-sdp-new-24.txt 'IP4-address'
var PATTERN_IP4_ADDRESS = PATTERN_DECIMAL_UCHAR+"(?:\\."+PATTERN_DECIMAL_UCHAR+"){3}";

// draft-ietf-mmusic-sdp-new-24.txt 'hexseq'
var PATTERN_HEXSEQ = PATTERN_HEXDIG+"{1,4}(?:\\:"+PATTERN_HEXDIG+"{1,4})*";

// draft-ietf-mmusic-sdp-new-24.txt 'hexpart'
var PATTERN_HEXPART = "(?:"+PATTERN_HEXSEQ+"|"+PATTERN_HEXSEQ+"\\:\\:(?:"+
  PATTERN_HEXSEQ+")?|\\:\\:+(?:"+PATTERN_HEXSEQ+")?)";

// draft-ietf-mmusic-sdp-new-24.txt 'IP6-address'
var PATTERN_IP6_ADDRESS = PATTERN_HEXPART+"(?:\\:"+PATTERN_IP4_ADDRESS+")?";

// draft-ietf-mmusic-sdp-new-24.txt 'FQDN'
var PATTERN_FQDN = "[A-Za-z0-9\\.\\-]{4,}";

// draft-ietf-mmusic-sdp-new-24.txt 'unicast-address'
// XXX more complicated than it needs to be. PATTERN_NON_WS_STRING should be sufficent
var PATTERN_UNICAST_ADDRESS = "(?:"+PATTERN_IP4_ADDRESS+"|"+PATTERN_IP6_ADDRESS+
  "|"+PATTERN_FQDN+"|"+PATTERN_NON_WS_STRING+")";


//----------------------------------------------------------------------
// Regexps used for testing or parsing SDP elements:

// Matches a draft-ietf-mmusic-sdp-new-24.txt 'token':
var REGEXP_TOKEN = new RegExp("^"+PATTERN_TOKEN+"$");

// Matches a 1*DIGIT:
var REGEXP_DIGITS = new RegExp("^\\d+$");

// Matches a *DIGIT:
var REGEXP_DIGITS_OR_NOTHING = new RegExp("^\\d*$");

// Match a draft-ietf-mmusic-sdp-new-24.txt 'text':
var REGEXP_TEXT = new RegExp("^"+PATTERN_TEXT+"$");

// Matches a prototype field value:
var REGEXP_PROTOCOLVERSION = new RegExp("^\\d+$");

// Matches a username field value (draft-ietf-mmusic-sdp-new-24.txt 'username'):
var REGEXP_USERNAME = new RegExp("^"+PATTERN_NON_WS_STRING+"$");

// Matches a draft-ietf-mmusic-sdp-new-24.txt 'unicast-address':
var REGEXP_UNICAST_ADDRESS = new RegExp("^"+PATTERN_UNICAST_ADDRESS+"$");

// Matches a uri field value:
// XXX to be implemented properly
var REGEXP_URI = REGEXP_TEXT;

// Matches a draft-ietf-mmusic-sdp-new-24.txt 'proto':
var REGEXP_PROTOCOL = new RegExp("^"+PATTERN_TOKEN+"(?:/"+PATTERN_TOKEN+")*$");

// Parses an origin-field into (username, session id, session-version,
// nettype, addrtype, unicast-address):
var REGEXP_ORIGIN = new RegExp("^([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*)$");

// Parses a connection field value into (nettype, addrtype, address):
var REGEXP_CONNECTION = new RegExp("^([^ ]*) ([^ ]*) ([^ ]*)$");

// Parses a media description field value into (media, port, count, proto, formats):
var REGEXP_MEDIADESCRIPTION = new RegExp("^([^ ]*) ([^/ ]*)(?:/([^ ]*))? ([^ ]*) (.*)$");

//----------------------------------------------------------------------
// Tokenizers to be applied repeatedly to input:

function makeTokenizer(re) {
  return new RegExp(re, "g");
}

// Helper to reset a tokenizer:
function resetTokenizer(tokenizer) {
  tokenizer.lastIndex = 0;
}

// Tokenizes a field into (type, value):
var TOKENIZER_FIELD = makeTokenizer("([A-Za-z])=(.*)\\r\\n");

// Tokenizes a list of formats:
var TOKENIZER_FORMATS = makeTokenizer("([^ ]+) ?");

////////////////////////////////////////////////////////////////////////
// Class SdpGenericValue
// a generic 'stub' field value

var SdpGenericValue = makeClass("SdpGenericValue",
                                SupportsImpl, AttributeParser);
SdpGenericValue.addInterfaces(Components.interfaces.zapISdpSyntaxObject,
                              Components.interfaces.zapISdpGenericValue);

SdpGenericValue.fun(
  function deserialize(iter) {
    this.value = iter.current[2];
    iter.advance();
  });

// to be implemented by subclass
SdpGenericValue.obj("fieldName", null);

//----------------------------------------------------------------------
// zapISdpSyntaxObject implementation:

// ACString serialize();
SdpGenericValue.fun(
  function serialize() {
    return this.fieldName + "=" + this.value + CRLF;
  });

//----------------------------------------------------------------------
// zapISdpGenericValue implementation:

// attribute ACString value;
SdpGenericValue.parsedAttrib("value", REGEXP_TEXT);


////////////////////////////////////////////////////////////////////////
// Class SdpEmailAddress XXX

var SdpEmailAddress = makeClass("SdpEmailAddress", SdpGenericValue);
SdpEmailAddress.addInterfaces(Components.interfaces.zapISdpEmailAddress);
SdpEmailAddress.obj("fieldName", "e");


////////////////////////////////////////////////////////////////////////
// Class SdpPhoneNumber XXX

var SdpPhoneNumber = makeClass("SdpPhoneNumber", SdpGenericValue);
SdpPhoneNumber.addInterfaces(Components.interfaces.zapISdpPhoneNumber);
SdpPhoneNumber.obj("fieldName", "p");


////////////////////////////////////////////////////////////////////////
// Class SdpBandwidth XXX

var SdpBandwidth = makeClass("SdpBandwidth", SdpGenericValue);
SdpBandwidth.addInterfaces(Components.interfaces.zapISdpBandwidth);
SdpBandwidth.obj("fieldName", "b");


////////////////////////////////////////////////////////////////////////
// Class SdpTime XXX

var SdpTime = makeClass("SdpTime", SdpGenericValue);
SdpTime.addInterfaces(Components.interfaces.zapISdpTime);
SdpTime.obj("fieldName", "t");


////////////////////////////////////////////////////////////////////////
// Class SdpZoneAdjustments XXX

var SdpZoneAdjustments = makeClass("SdpZoneAdjustments", SdpGenericValue);
SdpZoneAdjustments.addInterfaces(Components.interfaces.zapISdpZoneAdjustments);
SdpZoneAdjustments.obj("fieldName", "z");


////////////////////////////////////////////////////////////////////////
// Class SdpKey XXX

var SdpKey = makeClass("SdpKey", SdpGenericValue);
SdpKey.addInterfaces(Components.interfaces.zapISdpKey);
SdpKey.obj("fieldName", "k");


////////////////////////////////////////////////////////////////////////
// Class SdpAttrib XXX

var SdpAttrib = makeClass("SdpAttrib", SdpGenericValue);
SdpAttrib.addInterfaces(Components.interfaces.zapISdpAttrib);
SdpAttrib.obj("fieldName", "a");


////////////////////////////////////////////////////////////////////////
// Class SdpConnection

var SdpConnection = makeClass("SdpConnection", 
                              SupportsImpl, AttributeParser);
SdpConnection.addInterfaces(Components.interfaces.zapISdpConnection,
                            Components.interfaces.zapISdpSyntaxObject);

SdpConnection.fun(
  function deserialize(iter) {
    var matches = REGEXP_CONNECTION(iter.current[2]);
    if (!matches) this._error(PARSE_ERROR);
    this.addressType = matches[2];
    this.address = matches[3];
    iter.advance();
  });

//----------------------------------------------------------------------
// zapISdpSyntaxObject implementation:

// ACString serialize();
SdpConnection.fun(
  function serialize() {
    return  "c=IN " + this.addressType + " " + this.address + CRLF;
  });

//----------------------------------------------------------------------
// zapISdpConnection implementation:

// attribute ACString addressType;
SdpConnection.parsedAttrib("addressType", REGEXP_TOKEN);

// attribute ACString address;
SdpConnection.parsedAttrib("address", REGEXP_UNICAST_ADDRESS); //XXX add multicast


////////////////////////////////////////////////////////////////////////
// Class SdpMediaFormat

var SdpMediaFormat = makeClass("SdpMediaFormat",
                               SupportsImpl, AttributeParser);
SdpMediaFormat.addInterfaces(Components.interfaces.zapISdpMediaFormat);

SdpMediaFormat.fun(
  function deserialize(value) {
    this.format = value;
  });

//----------------------------------------------------------------------
// zapISdpMediaFormat implementation:

// attribute ACString format;
SdpMediaFormat.parsedAttrib("format", REGEXP_TOKEN);


////////////////////////////////////////////////////////////////////////
// Class SdpFieldContainer : Baseclass for Session & Media descriptions

var SdpFieldContainer = makeClass("SdpFieldContainer",
                                  SupportsImpl, AttributeParser);
SdpFieldContainer.addInterfaces(Components.interfaces.zapISdpSyntaxObject);

SdpFieldContainer.metafun(
  "\
 Adds an instance array called '_<name>', a getter                   \n\
 'function get<name>(count)' and a setter                            \n\
 'function set<name>(array, count).                                   ",
  function arr(name) {
    // add a ctor that adds the array to new instances:
    var arrName = "_"+name;
    this.appendCtor(function() { this[arrName] = []; });

    // construct getters/setters:
    var get_fct, set_fct;
    eval("get_fct = function get"+name+"(c) {"+
         "if (c) c.value = this[arrName].length; "+
         "return this[arrName]; }");
    eval("set_fct = function set"+name+"(a, c) { this[arrName] = a; }");

    // install functions:
    this.fun(get_fct);
    this.fun(set_fct);
  });

//----------------------------------------------------------------------
// deserialization utils:

SdpFieldContainer.fun(
  function _deserializeField(name, variableName, iter, optional) {
    if (iter.current && iter.current[1] == name) {
      this[variableName] = iter.current[2];
      iter.advance();
    }
    else {
      this[variableName] = null;
      if (!optional)
        this._error(PARSE_ERROR);
    }
  });

SdpFieldContainer.fun(
  function _deserializeObjectField(name, variableName, elementClass, iter, optional) {
    if (iter.current && iter.current[1] == name) {
      this[variableName] = elementClass.instantiate();
      this[variableName].deserialize(iter);
    }
    else {
      this[variableName] = null;
      if (!optional)
        this._error(PARSE_ERROR);
    }
  });

SdpFieldContainer.fun(
  function _deserializeArray(name, arrayName, elementClass, iter, optional) {
    var arr = [];
    while (iter.current && iter.current[1] == name) {
      var elem = elementClass.instantiate();
      elem.deserialize(iter);
      arr.push(elem);
    }
    if (!optional && arr.length==0) this._error(PARSE_ERROR);
    this[arrayName] = arr;
  });

////////////////////////////////////////////////////////////////////////
// Class SdpMediaDescription

var SdpMediaDescription = makeClass("SdpMediaDescription", SdpFieldContainer);
SdpMediaDescription.addInterfaces(Components.interfaces.zapISdpMediaDescription);

//----------------------------------------------------------------------
// deserialization

SdpMediaDescription.fun(
  function deserialize(iter) {
    this._assert(iter.current[1] == "m", "not a media description!");

    // deserialize media description field components:
    var matches = REGEXP_MEDIADESCRIPTION(iter.current[2]);
    if (!matches) this._error(PARSE_ERROR);
    this.media = matches[1];
    this.port = matches[2];
    this.portCount = matches[3] ? matches[3] : "";
    this.protocol = matches[4];
    this._Formats = [];
    resetTokenizer(TOKENIZER_FORMATS);
    var formatVal;
    while ((formatVal = TOKENIZER_FORMATS(matches[5]))) {
      var format = SdpMediaFormat.instantiate();
      format.deserialize(formatVal[1]);
      this._Formats.push(format);
    }
    if (this._Formats.length == 0) this._error(PARSE_ERROR);
    iter.advance();
    
    this._deserializeField("i", "information", iter, true);
    this._deserializeObjectField("c", "connection", SdpConnection, iter, true);
    this._deserializeArray("b", "_Bandwidths", SdpBandwidth, iter, true);
    this._deserializeObjectField("k", "key", SdpKey, iter, true);
    this._deserializeArray("a", "_Attribs", SdpAttrib, iter, true);
  });

//----------------------------------------------------------------------
// zapISdpSyntaxObject implementation:

// ACString serialize();
SdpMediaDescription.fun(
  function serialize() {
    var rv = "";

    // helper to serialize an array:
    function callSerialize(e) {
      rv += e.serialize();
    }
    
    // serialize media field:
    rv += "m=" + this.media + " " + this.port;
    if (this.portCount)
      rv += "/" + this.portCount;
    rv += " " + this.protocol;
    amap(function(f) { rv+=" "+f.format; },
         this._Formats);
    rv += CRLF;

    if (this.information) 
      rv += "i=" + this.information + CRLF;
    if (this.connection)
      rv += this.connection.serialize();
    amap(callSerialize, this._Bandwidths);
    if (this.key)
      rv += this.key.serialize();
    amap(callSerialize, this._Attribs);

    return rv;
  });

//----------------------------------------------------------------------
// zapISdpMediaDescription implementation:

// attribute ACString media;
SdpMediaDescription.parsedAttrib("media", REGEXP_TOKEN);
  
// attribute ACString long port;
SdpMediaDescription.parsedAttrib("port", REGEXP_DIGITS);

// attribute ACString portCount;
SdpMediaDescription.parsedAttrib("portCount", REGEXP_DIGITS_OR_NOTHING);

// attribute ACString protocol;
SdpMediaDescription.parsedAttrib("protocol", REGEXP_PROTOCOL);

// void getFormats(out unsigned long count,
//                 [retval, array, size_is(count)] out zapISdpMediaFormat formats);
// void setFormats([array, size_is(count)] in zapISdpMediaFormat formats,
//                 in unsigned long count);
SdpMediaDescription.arr("Formats");

// attribute ACString information;
SdpMediaDescription.parsedAttrib("information", REGEXP_TEXT);

// attribute zapISdpConnection connection;
SdpMediaDescription.obj("connection", null);

// void getBandwidths(out unsigned long count,
//                    [retval, array, size_is(count)] out zapISdpBandwidth bws);
// void setBandwidths([array, size_is(count)] in zapISdpBandwidth bws,
//                    in unsigned long count);
SdpMediaDescription.arr("Bandwidths");

// attribute zapISdpKey key;
SdpMediaDescription.obj("key", null);

// void getAttribs(out unsigned long count,
//                 [retval, array, size_is(count)] out zapISdpAttrib attribs);
// void setAttribs([array, size_is(count)] in zapISdpAttrib attribs,
//                 in unsigned long count);  
SdpMediaDescription.arr("Attribs");

////////////////////////////////////////////////////////////////////////
// Class SdpSessionDescription

var SdpSessionDescription = makeClass("SdpSessionDescription", SdpFieldContainer);
SdpSessionDescription.addInterfaces(Components.interfaces.zapISdpSessionDescription);

//----------------------------------------------------------------------
// deserialization

SdpSessionDescription.fun(
  function deserialize(octets) {
    resetTokenizer(TOKENIZER_FIELD);

    // iterator to iterate over fields:
    var iter = {
      current : TOKENIZER_FIELD(octets),
      advance : function() {
        if (!this.current) return;
        this.current = TOKENIZER_FIELD(octets);
      }
    };

    this._deserializeField("v", "protocolVersion", iter);
    
    // deserialize origin field components:
    if (!iter.current || !iter.current[1] == "o") this._error(PARSE_ERROR);
    var matches = REGEXP_ORIGIN(iter.current[2]);
    if (!matches) this._error(PARSE_ERROR);
    this.username = matches[1];
    this.sessionID = matches[2];
    this.sessionVersion = matches[3];
    // nettype not stored
    this.originAddressType = matches[5];
    this.originAddress = matches[6];
    iter.advance();
    
    this._deserializeField("s", "sessionName", iter);
    this._deserializeField("i", "information", iter, true);
    this._deserializeField("u", "uri", iter, true);
    this._deserializeArray("e", "_EmailAddresses", SdpEmailAddress, iter, true);
    this._deserializeArray("p", "_PhoneNumbers", SdpPhoneNumber, iter, true);
    this._deserializeObjectField("c", "connection", SdpConnection, iter, true);
    this._deserializeArray("b", "_Bandwidths", SdpBandwidth, iter, true);
    // XXX this won't parse 'r' fields yet:
    this._deserializeArray("t", "_Times", SdpTime, iter);
    this._deserializeObjectField("z", "zoneAdjustments", SdpZoneAdjustments, iter, true);
    this._deserializeObjectField("k", "key", SdpKey, iter, true);
    this._deserializeArray("a", "_Attribs", SdpAttrib, iter, true);
    this._deserializeArray("m", "_MediaDescriptions", SdpMediaDescription, iter, true);
  });

//----------------------------------------------------------------------
// zapISdpSyntaxObject implementation:

// ACString serialize();
SdpSessionDescription.fun(
  function serialize() {
    var rv = "";

    // helper to serialize an array:
    function callSerialize(e) {
      rv += e.serialize();
    }

    rv += "v="+this.protocolVersion + CRLF;

    // serialize origin field:
    rv += "o=" + this.username + " " + this.sessionID + " " + this.sessionVersion +
      " IN " + this.originAddressType + " " + this.originAddress + CRLF;

    rv += "s=" + this.sessionName + CRLF;    
    if (this.information) 
      rv += "i=" + this.information + CRLF;
    if (this.uri)
      rv += "u=" + this.uri + CRLF;
    amap(callSerialize, this._EmailAddresses);
    amap(callSerialize, this._PhoneNumbers);      
    if (this.connection)
      rv += this.connection.serialize();
    amap(callSerialize, this._Bandwidths);
    amap(callSerialize, this._Times);
    if (this.zoneAdjustments)
      rv += this.zoneAdjustments.serialize();
    if (this.key)
      rv += this.key.serialize();
    amap(callSerialize, this._Attribs);
    amap(callSerialize, this._MediaDescriptions);

    return rv;
  });

//----------------------------------------------------------------------
// zapISdpSessionDescription implementation:

// attribute ACString protocolVersion;
SdpSessionDescription.parsedAttrib("protocolVersion", REGEXP_PROTOCOLVERSION, "0");

// attribute ACString username;
SdpSessionDescription.parsedAttrib("username", REGEXP_USERNAME);

// attribute ACString sessionID;
SdpSessionDescription.parsedAttrib("sessionID", REGEXP_DIGITS);

// attribute ACString sessionVersion;
SdpSessionDescription.parsedAttrib("sessionVersion", REGEXP_DIGITS);

// attribute ACString originAddressType;
SdpSessionDescription.parsedAttrib("originAddressType", REGEXP_TOKEN);

// attribute ACString originAddress;
SdpSessionDescription.parsedAttrib("originAddress", REGEXP_UNICAST_ADDRESS);

// attribute ACString sessionName;
SdpSessionDescription.parsedAttrib("sessionName", REGEXP_TEXT);

// attribute ACString information;
SdpSessionDescription.parsedAttrib("information", REGEXP_TEXT);

// attribute ACString uri;
SdpSessionDescription.parsedAttrib("uri", REGEXP_URI);

// void getEmailAddresses(out unsigned long count,
//                        [retval, array, size_is(count)] out zapISdpEmailAddress addresses);
// void setEmailAddresses([array, size_is(count)] in zapISdpEmailAddress addresses,
//                        in unsigned long count);
SdpSessionDescription.arr("EmailAddresses");

// void getPhoneNumbers(out unsigned long count,
//                      [retval, array, size_is(count)] out zapISdpPhoneNumber numbers);

// void setPhoneNumbers([array, size_is(count)] in zapISdpPhoneNumber numbers,
//                      in unsigned long count);
SdpSessionDescription.arr("PhoneNumbers");

// attribute zapISdpConnection connection;
SdpSessionDescription.obj("connection", null);

// void getBandwidths(out unsigned long count,
//                    [retval, array, size_is(count)] out zapISdpBandwidth bws);

// void setBandwidths([array, size_is(count)] in zapISdpBandwidth bws,
//                    in unsigned long count);
SdpSessionDescription.arr("Bandwidths");

// void getTimes(out unsigned long count,
//               [retval, array, size_is(count)] out zapISdpTime times);

// void setTimes([array, size_is(count)] in zapISdpTime times,
//               in unsigned long count);
SdpSessionDescription.arr("Times");

// attribute zapISdpZoneAdjustments zoneAdjustments;
SdpSessionDescription.obj("zoneAdjustments", null);

// attribute zapISdpKey key;
SdpSessionDescription.obj("key", null);
  
// void getAttribs(out unsigned long count,
//                 [retval, array, size_is(count)] out zapISdpAttrib attribs);

// void setAttribs([array, size_is(count)] in zapISdpAttrib attribs,
//                 in unsigned long count);
SdpSessionDescription.arr("Attribs");

// void getMediaDescriptions(out unsigned long count,
//                           [retval, array, size_is(count)] out zapISdpMediaDescription media);

// void setMediaDescriptions([array, size_is(count)] in zapISdpMediaDescription media,
//                           in unsigned long count);
SdpSessionDescription.arr("MediaDescriptions");

////////////////////////////////////////////////////////////////////////
// Class SdpService

var SdpService = makeClass("SdpService", SupportsImpl);
SdpService.addInterfaces(Components.interfaces.zapISdpService);

//----------------------------------------------------------------------
// zapISdpService implementation:

// zapISdpSessionDescription deserializeSessionDescription(in ACString octets);
SdpService.fun(
  function deserializeSessionDescription(octets) {
    var descr = SdpSessionDescription.instantiate();
    descr.deserialize(octets);

    return descr;
  });

// zapISdpSessionDescription createSessionDescription();
SdpService.fun(
  function createSessionDescription() {
    return SdpSessionDescription.instantiate();
  });

// zapISdpConnection createConnection();
SdpService.fun(
  function createConnection() {
    return SdpConnection.instantiate();
  });

// zapISdpMediaDescription createMediaDescription();
SdpService.fun(
  function createMediaDescription() {
    return SdpMediaDescription.instantiate();
  });

// zapISdpAttrib createAttrib();
SdpService.fun(
  function createAttrib() {
    return SdpAttrib.instantiate();
  });

// zapISdpTime createTime();
SdpService.fun(
  function createTime() {
    return SdpTime.instantiate();
  });

// zapISdpMediaFormat createMediaFormat();
SdpService.fun(
  function createMediaFormat() {
    return SdpMediaFormat.instantiate();
  });

//----------------------------------------------------------------------
// global SdpService object:

var theSdpService = SdpService.instantiate();


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP SDP Service",
     cid        : Components.ID("{5de0fd79-a03e-4640-ab99-186f5c66f5bb}"),
     contractID : "@mozilla.org/zap/sdpservice;1",
     factory    : ComponentUtils.generateFactory(function() { return theSdpService; })
  }]);
