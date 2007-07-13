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
 * The Original Code is the Mozilla browser.
 *
 * The Initial Developer of the Original Code is Mozilla.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Myk Melez <myk@mozilla.org>
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

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// RDF properties

const NC_NS                 = "http://home.netscape.com/NC-rdf#";

// nsIHandlerInfo
const NC_PREFERRED_APP      = NC_NS + "externalApplication";
const NC_SAVE_TO_DISK       = NC_NS + "saveToDisk";
const NC_HANDLE_INTERNALLY  = NC_NS + "handleInternal"; // sic: adverbally challenged!
const NC_USE_SYSTEM_DEFAULT = NC_NS + "useSystemDefault";
const NC_ALWAYS_ASK         = NC_NS + "alwaysAsk";

// nsIHandlerApp
const NC_PRETTY_NAME        = NC_NS + "prettyName";

// nsILocalHandlerApp
const NC_PATH               = NC_NS + "path";

// nsIWebHandlerApp
const NC_URI_TEMPLATE       = NC_NS + "uriTemplate";

function HamburgerHelperService() {}

HamburgerHelperService.prototype = {
  //**************************************************************************//
  // XPCOM Plumbing

  classDescription: "Hamburger Helper Service",
  classID:          Components.ID("{32314cc8-22f7-4f7f-a645-1a45453ba6a6}"),
  contractID:       "@mozilla.org/uriloader/hamburger-helper-service;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIHamburgerHelperService]),

  //**************************************************************************//
  // nsIHamburgerHelperService

  setPreferredHandler: function HHS_setPreferredHandler(aContentType,
                                                        aPreferredHandler) {
    var typeID = this._getTypeID(aContentType);
    var handlerID = this._getPreferredHandlerID(aContentType);

    // FIXME: when we switch from RDF to something with transactions (like
    // SQLite), enclose the following changes in a transaction so they all
    // get rolled back if any of them fail and we don't leave the datastore
    // in an inconsistent state.

    // First add a record for the preferred app to the datasource.  In the
    // process we also need to remove any vestiges of an existing record, so
    // we remove any properties that we aren't overwriting.
    this._setLiteral(handlerID, NC_PRETTY_NAME, aPreferredHandler.name);
    try {
      aPreferredHandler.QueryInterface(Ci.nsILocalHandlerApp);
      this._setLiteral(handlerID, NC_PATH, aPreferredHandler.executable.path);
      this._removeValue(handlerID, NC_URI_TEMPLATE);
    }
    catch(ex) {
      aPreferredHandler.QueryInterface(Ci.nsIWebHandlerApp);
      this._setLiteral(handlerID, NC_URI_TEMPLATE, aPreferredHandler.uriTemplate);
      this._removeValue(handlerID, NC_PATH);
    }

    // Finally, make the handler app be the preferred app for the handler info.
    // Note: at least some code completely ignores this setting and assumes
    // the preferred app is the one whose RDF URI follows the correct pattern
    // (i.e. |urn:(mimetype|scheme):externalApplication:<type>|).
    this._setResource(typeID, NC_PREFERRED_APP, handlerID);

    // XXX Should we also update aContentType to take the change into account?
  },

  setPreferredAction: function HHS_setPreferredAction(aContentType,
                                                      aPreferredAction) {
    var typeID = this._getTypeID(aContentType);

    switch(aPreferredAction) {
      case Ci.nsIHandlerInfo.saveToDisk:
        this._setLiteral(typeID, NC_SAVE_TO_DISK, "true");
        this._removeValue(typeID, NC_HANDLE_INTERNALLY);
        this._removeValue(typeID, NC_USE_SYSTEM_DEFAULT);
        break;

      case Ci.nsIHandlerInfo.handleInternally:
        this._setLiteral(typeID, NC_HANDLE_INTERNALLY, "true");
        this._removeValue(typeID, NC_SAVE_TO_DISK);
        this._removeValue(typeID, NC_USE_SYSTEM_DEFAULT);
        break;

      case Ci.nsIHandlerInfo.useSystemDefault:
        this._setLiteral(typeID, NC_USE_SYSTEM_DEFAULT, "true");
        this._removeValue(typeID, NC_SAVE_TO_DISK);
        this._removeValue(typeID, NC_HANDLE_INTERNALLY);
        break;

      // This value is indicated in the datastore either by the absence of
      // the three properties or by setting them all "false".  Of these two
      // options, the former seems preferable, because it reduces the size
      // of the RDF file and thus the amount of stuff we have to parse.
      case Ci.nsIHandlerInfo.useHelperApp:
      // XXX Should we throw instead if we don't recognize the value?
      default:
        this._removeValue(typeID, NC_SAVE_TO_DISK);
        this._removeValue(typeID, NC_HANDLE_INTERNALLY);
        this._removeValue(typeID, NC_USE_SYSTEM_DEFAULT);
        break;
    }

    // XXX Should we also update aContentType to take the change into account?
  },

  setAlwaysAsk: function HHS_setAlwaysAsk(aContentType, aAlwaysAsk) {
    var typeID = this._getTypeID(aContentType);
    this._setLiteral(typeID, NC_ALWAYS_ASK, aAlwaysAsk ? "true" : "false");

    // XXX Should we also update aContentType to take the change into account?
  },

  addPossibleHandler: function HHS_addPossibleHandler(aContentType,
                                                      aPossibleHandler) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  removePossibleHandler: function HHS_removePossibleHandler(aContentType,
                                                            aPossibleHandler) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },


  //**************************************************************************//
  // Helper Methods

  // RDF Service
  __rdf: null,
  get _rdf() {
    if (!this.__rdf)
      this.__rdf = Cc["@mozilla.org/rdf/rdf-service;1"].
                   getService(Ci.nsIRDFService);
    return this.__rdf;
  },

  // RDF datasource containing content handling config (i.e. mimeTypes.rdf)
  __ds: null,
  get _ds() {
    if (!this.__ds) {
      var fileLocator = Cc["@mozilla.org/file/directory_service;1"].
                        getService(Ci.nsIProperties);
      var file = fileLocator.get("UMimTyp", Ci.nsIFile);
      // FIXME: make this a memoizing getter if we use it anywhere else.
      var ioService = Cc["@mozilla.org/network/io-service;1"].
                      getService(Ci.nsIIOService);
      var fileHandler = ioService.getProtocolHandler("file").
                        QueryInterface(Ci.nsIFileProtocolHandler);
      this.__ds =
        this._rdf.GetDataSourceBlocking(fileHandler.getURLSpecFromFile(file));
    }

    return this.__ds;
  },

  /**
   * Return the unique identifier for a content type record, which for the
   * RDF-based datastore is a URI of the form: |urn:(mimetype|scheme):<type>|.
   *
   * FIXME: this should be a property of nsIHandlerInfo.
   *
   * @param aContentType {nsIHandlerInfo} the type for which to get the ID
   */
  _getTypeID: function HHS__getTypeID(aContentType) {
    // FIXME: once nsIHandlerInfo supports retrieving the scheme
    // (and differentiating between MIME and protocol infos), implement
    // support for protocol handlers.
    var mimeInfo = aContentType.QueryInterface(Ci.nsIMIMEInfo);
    try       { var mimeType = mimeInfo.MIMEType }
    catch(ex) { throw Cr.NS_ERROR_NOT_IMPLEMENTED }

    var id = "urn:mimetype:" + mimeType;

    return id;
  },

  /**
   * Return the unique identifier for a content type record, which for the
   * RDF-based datastore is a URI of the form: |urn:(mimetype|scheme):<type>|.
   *
   * FIXME: this should be a property of nsIHandlerApp, and we should retrieve
   * the preferred handler for a given content type via the
   * NC:externalApplication property rather than counting on preferred handlers
   * to have URIs of a specific form (i.e. handlers should have arbitrary
   * unique IDs that don't change depending on whether or not they are preferred
   * handlers for one or more content types).
   * 
   * @param aContentType {nsIHandlerInfo} the type for which to get the ID
   */
  _getPreferredHandlerID: function HHS__getPreferredHandlerID(aContentType) {
    // FIXME: once nsIHandlerInfo supports retrieving the scheme
    // (and differentiating between MIME and protocol types), implement
    // support for protocol handlers by constructing the handlerID
    // based on whether the handler is for a MIME type or a protocol.
    var mimeInfo = aContentType.QueryInterface(Ci.nsIMIMEInfo);
    try       { var mimeType = mimeInfo.MIMEType }
    catch(ex) { throw Cr.NS_ERROR_NOT_IMPLEMENTED }

    var id = "urn:mimetype:externalApplication:" + mimeType;

    return id;
  },

  /**
   * Set a property of an RDF source to a literal value.
   *
   * @param sourceURI   {string} the URI of the source
   * @param propertyURI {string} the URI of the property
   * @param value       {string} the literal value
   */
  _setLiteral: function HHS__setLiteral(sourceURI, propertyURI, value) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);
    var target = this._rdf.GetLiteral(value);
    
    this._setTarget(source, property, target);
  },

  /**
   * Set a property of an RDF source to a resource.
   *
   * @param sourceURI   {string} the URI of the source
   * @param propertyURI {string} the URI of the property
   * @param resourceURI {string} the URI of the resource
   */
  _setResource: function HHS__setResource(sourceURI, propertyURI, resourceURI) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);
    var target = this._rdf.GetResource(resourceURI);
    
    this._setTarget(source, property, target);
  },

  /**
   * Assert an arc into the RDF datasource if there is no arc with the given
   * source and property; otherwise, if there is already an existing arc,
   * change it to point to the given target.
   *
   * @param source    {nsIRDFResource}  the source
   * @param property  {nsIRDFResource}  the property
   * @param value     {nsIRDFNode}      the target
   */
  _setTarget: function HHS__setTarget(source, property, target) {
    if (this._ds.hasArcOut(source, property)) {
      var oldTarget = this._ds.GetTarget(source, property, true);
      this._ds.Change(source, property, oldTarget, target);
    }
    else
      this._ds.Assert(source, property, target, true);
  },

  /**
   * Remove a property of an RDF source.
   *
   * @param sourceURI   {string} the URI of the source
   * @param propertyURI {string} the URI of the property
   */
  _removeValue: function HHS__removeValue(sourceURI, propertyURI) {
    var source = this._rdf.GetResource(sourceURI);
    var property = this._rdf.GetResource(propertyURI);

    if (this._ds.hasArcOut(source, property)) {
      var target = this._ds.GetTarget(source, property, true);
      this._ds.Unassert(source, property, target, true);
    }
  },


  //**************************************************************************//
  // Utilities

  // FIXME: this stuff should all be in a JavaScript module given that I keep
  // copying it from component to component.

  /**
   * Get an app pref or a default value if the pref doesn't exist.
   *
   * @param   aPrefName
   * @param   aDefaultValue
   * @returns the pref's value or the default (if it is missing)
   */
  _getAppPref: function _getAppPref(aPrefName, aDefaultValue) {
    try {
      var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);
      switch (prefBranch.getPrefType(aPrefName)) {
        case prefBranch.PREF_STRING:
          return prefBranch.getCharPref(aPrefName);

        case prefBranch.PREF_INT:
          return prefBranch.getIntPref(aPrefName);

        case prefBranch.PREF_BOOL:
          return prefBranch.getBoolPref(aPrefName);
      }
    }
    catch (ex) { /* return the default value */ }
    
    return aDefaultValue;
  },

  // Console Service
  __consoleSvc: null,
  get _consoleSvc() {
    if (!this.__consoleSvc)
      this.__consoleSvc = Cc["@mozilla.org/consoleservice;1"].
                          getService(Ci.nsIConsoleService);
    return this.__consoleSvc;
  },

  _log: function _log(aMessage) {
    if (!this._getAppPref("browser.contentHandling.log", false))
      return;

    aMessage = "*** HamburgerHelperService: " + aMessage;
    dump(aMessage + "\n");
    this._consoleSvc.logStringMessage(aMessage);
  }
};


//****************************************************************************//
// XPCOM Plumbing

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([HamburgerHelperService]);
}
