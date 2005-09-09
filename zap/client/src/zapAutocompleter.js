// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:zapAutocompleter.js')" -*-
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


debug("*** loading zapAutocompleter\n");

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/FileUtils.js");

// name our global object:
function toString() { return "[zapAutocompleter.js]"; }

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// Globals

var gRDF;
var gContactsDS;
var gContactsContainer;
var gResourceName;
var gResourceSip1;
var gResourceSip2;

var gInitialized = false;

function ensureModuleInitialized() {
  if (gInitialized) return;

  gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
  gContactsDS = gRDF.GetDataSourceBlocking(getProfileFileURL("contacts.rdf"));
  gContactsContainer = Components.classes["@mozilla.org/rdf/container;1"].createInstance(Components.interfaces.nsIRDFContainer);
gContactsContainer.Init(gContactsDS,
                        gRDF.GetResource("urn:mozilla:zap:contacts"));
  gResourceName = gRDF.GetResource("http://home.netscape.com/NC-rdf#Name");
  gResourceSip1 = gRDF.GetResource("urn:mozilla:zap:sip1");
  gResourceSip2 = gRDF.GetResource("urn:mozilla:zap:sip2");
  
  gInitialized = true;
}

////////////////////////////////////////////////////////////////////////
// helpers

// make sure str is of the form sip(s):xxx
function canoniziseSIPURL(str) {
  if (!/^sips?:/.test(str))
    return "sip:"+str;
  return str;
}

////////////////////////////////////////////////////////////////////////
// zapAutocompleResult

var zapAutocompleResult = makeClass("zapAutocompleResult", SupportsImpl);
zapAutocompleResult.addInterfaces(Components.interfaces.nsIAutoCompleteResult);

function makeResult(searchstring, results) {
  var res = zapAutocompleResult.instantiate();
  res.searchString = searchstring;
  res._results = results;
  return res;
}

// array of results:
zapAutocompleResult.obj("_results", []);

//----------------------------------------------------------------------
// nsIAutoCompleteResult

  /**
   * The original search string
   */
//  readonly attribute AString searchString;
zapAutocompleResult.obj("searchString", null);

  /**
   * The result of the search
   */
//  readonly attribute unsigned short searchResult;
zapAutocompleResult.getter(
  "searchResult",
  function get_searchResult() {
    if (this._results.length) 
      return Components.interfaces.nsIAutoCompleteResult.RESULT_SUCCESS;
    else
      return Components.interfaces.nsIAutoCompleteResult.RESULT_NOMATCH;
  });

  /**
   * Index of the default item that should be entered if none is selected
   */
//  readonly attribute long defaultIndex;
zapAutocompleResult.obj("defaultIndex", 0);

  /**
   * A string describing the cause of a search failure
   */
//  readonly attribute AString errorDescription;
zapAutocompleResult.obj("errorDescription", "");

  /**
   * The number of matches
   */
//  readonly attribute unsigned long matchCount;
zapAutocompleResult.getter(
  "matchCount",
  function get_matchCount() {
    return this._results.length;
  });

  /**
   * Get the value of the result at the given index
   */
//  AString getValueAt(in long index);
zapAutocompleResult.fun(
  function getValueAt(index) {
    return this._results[index];
  });

  /**
   * Get the comment of the result at the given index
   */
//  AString getCommentAt(in long index);
zapAutocompleResult.fun(
  function getCommentAt(index) {
    return "acomment";
  });

  /**
   * Get the style hint for the result at the given index
   */
//  AString getStyleAt(in long index);
zapAutocompleResult.fun(
  function getStyleAt(index) {
    return "";
  });

  /**
   * remove the value at the given index from the autocomplete results.
   * If removeFromDb is set to true, the value should be removed from
   * persistent storage as well.
   */
//  void removeValueAt(in long rowIndex, in boolean removeFromDb);
zapAutocompleResult.fun(
  function removeValueAt(rowIndex, removeFromDb) {
    //XXX
  });

////////////////////////////////////////////////////////////////////////
// zapAutocompleter

var zapAutocompleter = makeClass("zapAutocompleter", SupportsImpl);
zapAutocompleter.addInterfaces(Components.interfaces.nsIAutoCompleteSearch);

zapAutocompleter.appendCtor(ensureModuleInitialized);

//----------------------------------------------------------------------
// nsIAutoCompleteSearch

  /*
   * Search for a given string and notify a listener (either synchronously
   * or asynchronously) of the result
   *
   * @param searchString - The string to search for
   * @param searchParam - An extra parameter
   * @param previousResult - A previous result to use for faster searchinig
   * @param listener - A listener to notify when the search is complete
   */
//  void startSearch(in AString searchString,
//                   in AString searchParam,
//                   in nsIAutoCompleteResult previousResult,
//                   in nsIAutoCompleteObserver listener);
zapAutocompleter.fun(
  function startSearch(searchString, searchParam, previousResult, listener) {
    // match sip addresses and names; collect in arr:
    var arr = [];
    var re = new RegExp(searchString, "i");
    var contacts = gContactsContainer.GetElements();
    while (contacts.hasMoreElements()) {
      var resource = contacts.getNext();
      var name = gContactsDS.GetTarget(resource, gResourceName, true).QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      var sip1 = gContactsDS.GetTarget(resource, gResourceSip1, true).QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      var sip2 = gContactsDS.GetTarget(resource, gResourceSip2, true).QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      if (sip1) {
        var addr = name+" <"+canoniziseSIPURL(sip1)+">";
        if (re.test(addr))
          arr.push(addr);
      }
      if (sip2) {
        var addr = name+" <"+canoniziseSIPURL(sip2)+">";
        if (re.test(addr))
          arr.push(addr);
      }
    }
    arr.sort();
    var res = makeResult(searchString, arr);
    listener.onSearchResult(this, res);
  });


  /*
   * Stop an asynchronous search that is in progress
   */
//  void stopSearch();
zapAutocompleter.fun(function stopSearch() {});


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Autocompleter",
     cid        : Components.ID("{d65246a4-9778-4124-8c3d-e76efe44b6d8}"),
     contractID : "@mozilla.org/autocomplete/search;1?name=zap-autocomplete",
     factory    : ComponentUtils.generateFactory(function() { return zapAutocompleter.instantiate(); })
  }]);
