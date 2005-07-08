/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('resource:/jscodelib/zap/StringUtils.js', null)" -*- */
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

EXPORTED_SYMBOLS = [ "utf8ToUnicode",
                     "unicodeToUTF8",
                     "padright",
                     "padleft"];

// object to hold module's documentation:
var _doc_ = {};

//----------------------------------------------------------------------
// 

var CLASS_CHARCONV = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"];
var ITF_CHARCONV = Components.interfaces.nsIScriptableUnicodeConverter;

var _gUTF8Conv; // global UTF-8 -> Unicode converter

function getUTF8Converter() {
  if (!_gUTF8Conv) {
    _gUTF8Conv = CLASS_CHARCONV.createInstance();
    _gUTF8Conv.QueryInterface(ITF_CHARCONV);
    _gUTF8Conv.charset = "UTF-8";
  }
  return _gUTF8Conv;
}

//----------------------------------------------------------------------
// utf8ToUnicode

_doc_.utf8ToUnicode = "\
 Converts the JS unicode string 'str' to UTF-8.                       ";

function utf8ToUnicode(str) {
  return getUTF8Converter().ConvertToUnicode(str);
}

//----------------------------------------------------------------------
// unicodeToUTF8

_doc_.utf8ToUnicode = "\
 Converts the given UTF-8 encoded string 'str' to a JS unicode string.";

function unicodeToUTF8(str) {
  var converter = getUTF8Converter();
  var rv = converter.ConvertFromUnicode(str);
  rv += converter.Finish();
  return rv;
}

//----------------------------------------------------------------------
// padright

_doc_.padright = "\
 Pads a string 'str' with char 'c' up to a total string length 'l'.  \n\
 Returns padded string. String will be padded on the right.           ";

function padright(str, c, l) {
  var i = l - str.length;
  while (--i>=0)
    str += c;
  return str;
}

//----------------------------------------------------------------------
// padleft

_doc_.padleft = "\
 Pads a string 'str' with char 'c' up to a total string length 'l'.  \n\
 Returns padded string. String will be padded on the left.            ";

function padleft(str, c, l) {
  var i = l - str.length;
  while (--i>=0)
    str = c + str;
  return str;
}

