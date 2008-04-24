/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/StringUtils.jsm', null)" -*- */
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

Components.utils.import("resource://gre/components/FunctionUtils.jsm");

EXPORTED_SYMBOLS = [ "utf8ToUnicode",
                     "unicodeToUTF8",
                     "octetToHex",
                     "hexToOctet",
                     "hexCharCodeAt",
                     "MD5Hex",
                     "padright",
                     "padleft"
                     ];

//----------------------------------------------------------------------
// Globals

var CLASS_CHARCONV = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"];
var ITF_CHARCONV = Components.interfaces.nsIScriptableUnicodeConverter;

var _gUTF8Conv; // global UTF-8 -> Unicode converter

function getUTF8Converter() {
  if (!_gUTF8Conv) {
    _gUTF8Conv = CLASS_CHARCONV.createInstance(ITF_CHARCONV);
    _gUTF8Conv.charset = "UTF-8";
  }
  return _gUTF8Conv;
}


var _gCryptoHash; 

function getCryptoHash() {
  if (!_gCryptoHash) {
    _gCryptoHash = Components.classes["@mozilla.org/security/hash;1"].createInstance(Components.interfaces.nsICryptoHash);
  }
  return _gCryptoHash;
}


//----------------------------------------------------------------------
// utf8ToUnicode

defun(
  "Converts the JS unicode string 'str' to UTF-8.",
  function utf8ToUnicode(str) {
    return getUTF8Converter().ConvertToUnicode(str);
  });

//----------------------------------------------------------------------
// unicodeToUTF8

defun(
  "Converts the given UTF-8 encoded string 'str' to a JS unicode string.",
  function unicodeToUTF8(str) {
    var converter = getUTF8Converter();
    var rv = converter.ConvertFromUnicode(str);
    rv += converter.Finish();
    return rv;
  });

//----------------------------------------------------------------------
// octetToHex

var hexDigits = "0123456789abcdef";

defun(
  "octetToHex(str, [opt] separator, [opt] octets_per_line) returns the lower-case hexadecimal octet representation of the 'str'. 'str' must be a string of characters with codes between 0 and 255. Codes outside of this range will be truncated. 'separator' is an optional string to insert between the hex octets of the output (default=''). 'octets_per_line' is an optional count determining how many octets should be placed into the output string before a newline is inserted (default=0, don't place newlines). hexToOctet(octetToHex(s)) == s, if the separator string doesn't contain characters in the range [0-9a-fA-F].",
  function octetToHex(str, separator, octets_per_line) {
    var rv = "";
    for (var i=0, l=str.length; i<l; ++i) {
      var code = str.charCodeAt(i);
      rv += hexDigits[(code >> 4) & 0x0F] + hexDigits[code & 0x0F];
      if (octets_per_line &&
          i % octets_per_line == octets_per_line-1)
        rv += "\n";
      else if (separator && i!=l-1) rv += separator;
    }
    return rv;
  });

//----------------------------------------------------------------------
// hexToOctet

var hexTokenizer = /[0-9a-fA-F][0-9a-fA-F]/g;

defun(
  "hexToOctet(str) converts the given string of hexadecimal octets (as encoded by octetToHex) into a string of characters with codes between 0 and 255. characters outside of [0-9a-fA-F] will be skipped.",
  function hexToOctet(str) {
    var rv = "";
    hexTokenizer.lastIndex = 0;
    var match;
    while ((match = hexTokenizer(str))) {
      rv += String.fromCharCode(parseInt(match[0], 16));
    };
    return rv;
  });

//----------------------------------------------------------------------
// hexCharCodeAt

defun(
  "hexCharCodeAt(str, i) returns the charcode of str[i] as a 2 digit lower-case hexadecimal value. The charcode must be between 0 and 255 or it will be truncated.",
  function hexCharCodeAt(str, i) {
    var code = str.charCodeAt(i);
    return hexDigits[(code >> 4) & 0x0F] + hexDigits[code & 0x0F];
  });

//----------------------------------------------------------------------
// MD5Hex

defun(
  "calculate MD5 hash of data and convert to hex representation as explained in RFC2617",
  function MD5Hex(data) {
    getCryptoHash().initWithString("md5");
    if (data) {
      // XXX add update(ACString) method to nsICryptoHash interface so
      // that we don't have to jump through these hoops
      var stream = Components.classes["@mozilla.org/io/string-input-stream;1"].createInstance(Components.interfaces.nsIStringInputStream);
      stream.setData(data, data.length);
      getCryptoHash().updateFromStream(stream, data.length);
    }
    var hash = getCryptoHash().finish(false);
    var hashHex = "";
    for (var i=0,l=hash.length; i<l; ++i) {
      hashHex += hexCharCodeAt(hash, i);
    }
    return hashHex;
  });


//----------------------------------------------------------------------
// padright

defun(
  "Pads a string 'str' with char 'c' up to a total string length 'l'. "+
  "Returns padded string. String will be padded on the right.",
  function padright(str, c, l) {
    var i = l - str.length;
    while (--i>=0)
      str += c;
    return str;
  });

//----------------------------------------------------------------------
// padleft

defun(
  "Pads a string 'str' with char 'c' up to a total string length 'l'. "+
  "Returns padded string. String will be padded on the left.",
  function padleft(str, c, l) {
    var i = l - str.length;
    while (--i>=0)
      str = c + str;
    return str;
  });

