/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/xtf.js',1)" -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

/**
 * Utilities for writing XTF elements.
 */

importModule("resource:/jscodelib/StdLib.js");
importModule("resource:/jscodelib/TemplateLib.js");

MOZ_EXPORTED_SYMBOLS = [ "XHTML_NS", "SVG_NS",
                         "ContentBuilder",
                         "XTFGenericElement",
                         "XTFSVGVisual",
                         "XTFXMLVisual",
                         "XTFXULVisual",
                         "XTFElementFactory" ];

//----------------------------------------------------------------------
// Global constants

var XHTML_NS = "http://www.w3.org/1999/xhtml";
var SVG_NS   = "http://www.w3.org/2000/svg";


//----------------------------------------------------------------------
// ContentBuilder: simple interface to facilitate constructing xml
// content

var ContentBuilder = makeTemplate("ContentBuilder");

ContentBuilder.mergeTemplate(ErrorTemplate);

ContentBuilder.appendInitializer("ContentBuilder",
  function(args) {
    if (!args || args.document === undefined) this._error("initializer arg 'document' missing");
    this._document = args.document;
    this._top = null;
    this._current = null;
    this._ns = null;
  });

ContentBuilder.addProtoObj("setElementNamespace",
  function(ns) {
    this._ns = ns;
    return this;
  });

ContentBuilder.addProtoObj("beginElement",
  function(tagname) {
    var elem = this._ns ? this._document.createElementNS(this._ns, tagname) :
                          this._document.createElement(tagname);
    if (!this._current) {
      if (this._top) this._error("Building of multi-rooted trees not supported");
      this._current = this._top = elem;
    }
    else {
      this._current.appendChild(elem);
      this._current = elem;
    }
    return this._current;
  });

ContentBuilder.addProtoObj("endElement",
  function() {
    if (!this._current) this._error("Unbalanced begin/endElement");
    this._current = this._current.parentNode;
  });

ContentBuilder.addProtoObj("attrib",
  function(name, value) {
    if (!this._current) this._error("No element");
    this._current.setAttribute(name, value);
  });

ContentBuilder.addProtoObj("attribs",
  function(obj) {
    if (!this._current) this._error("No element");
    for (var a in obj)
      this._current.setAttribute(a, obj[a]);
  });

ContentBuilder.addProtoGetter("root",
  function() {
    return this._top;
  });


//----------------------------------------------------------------------
// XTFElement: common base template for XTFGenericElement and
// XTF visuals

var XTFElement = makeTemplate("XTFElement");

XTFElement.mergeTemplate(ErrorTemplate);
XTFElement.mergeTemplate(SupportsTemplate);

XTFElement.appendInitializer("XTFElement",
                             function(args) {
    if (!args || args.type === undefined) this._error("initializer arg 'type' missing");
    this._elementType = args.type;
  });
    
XTFElement.addInterface(Components.interfaces.nsIXTFElement);

XTFElement.addProtoGetter("elementType",
  function () {
    return this._elementType;
  });

var _nsIXTFElement_stubs = ["willChangeDocument", "documentChanged",
                            "willChangeParent", "parentChanged",
                            "willInsertChild", "childInserted",
                            "willAppendChild", "childAppended",
                            "willRemoveChild", "childRemoved",
                            "willSetAttribute", "attributeSet",
                            "willUnsetAttribute", "attributeUnset"];
for (var m=0; m<_nsIXTFElement_stubs.length; ++m) {
  XTFElement.addProtoObj(_nsIXTFElement_stubs[m],
    (function(m) {
      return function() {
        var args = "";
        for (var i=0; i<arguments.length; ++i) {
          if (i!=0) args+=", ";
          args+=arguments[i];
        }
        this._dump(_nsIXTFElement_stubs[m]+"("+args+") called");
      };})(m));
}

XTFElement.addProtoObj("getScriptingInterfaces",
  function(count) {
    count.value = this._scriptingInterfaces.length;
    return this._scriptingInterfaces;
  });

XTFElement.addProtoObj("_scriptingInterfaces",
                       [],
                       { mergeover: appendUnique });

XTFElement.addOpsObject("addScriptingInterface",
  function(itf) {
    this.getProtoObj("_scriptingInterfaces").push(itf);
  });

// onCreated really belongs to sub-interfaces of nsIXMLElement, but it
// makes sense to implement this here, because JS isn't typed and the
// different signatures of onCreate only differ in argument type:

XTFElement.addProtoObj("onCreated",
  function(wrapper) {
    this._wrapper = wrapper;
  });

XTFElement.addProtoObj("onDestroyed",
  function() {
    delete this._wrapper;
  });


//----------------------------------------------------------------------
// XTFGenericElement

var XTFGenericElement = makeTemplate("XTFGenericElement");
XTFGenericElement.mergeTemplate(XTFElement);
XTFGenericElement.addInterface(Components.interfaces.nsIXTFGenericElement);
XTFGenericElement.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_GENERIC_ELEMENT});

//----------------------------------------------------------------------
// XTFVisual: base template for the visuals
var XTFVisual = makeTemplate("XTFVisual");
XTFVisual.mergeTemplate(XTFElement);

XTFVisual.appendInitializer("XTFVisual",
  function(args) {
    if (!args || args.contentDocGetter === undefined) this._error("initializer arg 'contentDocGetter' missing");
    this._getVisualContentDoc = args.contentDocGetter;
  });

XTFVisual.addProtoObj("XTFElement$onCreated",
                      XTFVisual.getProtoObj("onCreated"));
XTFVisual.addProtoObj("onCreated",
  function(wrapper) {
    // We can't use owner owner doc to create our visual content,
    // because of the resulting cyclic referencing which will keep
    // out owner doc (along with us, our wrapper, etc) from ever
    // getting destroyed. This is because content nodes have their
    // owner document in their parent chain - see nsDOMClassInfo:
    // nsNodeSH::PreCreate(). So instead of the owner doc we use a
    // lazily-constructed document obtained via the initializer
    // parameter 'contentDocGetter'.
    var builder = ContentBuilder.instantiate({document:this._getVisualContentDoc()});
    this._buildVisualContent(builder);
    this._visualContent = builder.root;
    
    return this.XTFElement$onCreated(wrapper);
  });

XTFVisual.addProtoGetter("visualContent",
  function() {
    return this._visualContent;
  });

XTFVisual.addProtoObj("_buildVisualContent",
                      function(builder) {});

XTFVisual.addProtoObj("XTFElement$onDestroyed",
                      XTFVisual.getProtoObj("onDestroyed"));
XTFVisual.addProtoObj("onDestroyed",
  function() {
    delete this._visualContent;
    this.XTFElement$onDestroyed();
  });

//----------------------------------------------------------------------
// XTFSVGVisual

var XTFSVGVisual = makeTemplate("XTFSVGVisual");
XTFSVGVisual.mergeTemplate(XTFVisual);
XTFSVGVisual.addInterface(Components.interfaces.nsIXTFSVGVisual);
XTFSVGVisual.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_SVG_VISUAL});

// We share a document for the visual content between all visuals of
// the same type:
var _XTFSVGVisual_content_doc = null;
XTFSVGVisual.bindInitializer("XTFVisual",
{contentDocGetter: function() {
  if (!_XTFSVGVisual_content_doc) {
    _XTFSVGVisual_content_doc = Components.classes["@mozilla.org/svg/svg-document;1"].createInstance();
  }
  return _XTFSVGVisual_content_doc;
}});
  
//----------------------------------------------------------------------
// XTFXMLVisual

var XTFXMLVisual = makeTemplate("XTFXMLVisual");
XTFXMLVisual.mergeTemplate(XTFVisual);
XTFXMLVisual.addInterface(Components.interfaces.nsIXTFXMLVisual);
XTFXMLVisual.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_XML_VISUAL});

// We share a document for the visual content between all visuals of
// the same type:
var _XTFXMLVisual_content_doc = null;
XTFXMLVisual.bindInitializer("XTFVisual",
{contentDocGetter: function() {
  if (!_XTFXMLVisual_content_doc) {
    _XTFXMLVisual_content_doc = Components.classes["@mozilla.org/xml/xml-document;1"].createInstance();
  }
  return _XTFXMLVisual_content_doc;
}});


//----------------------------------------------------------------------
// XTFXULVisual

var XTFXULVisual = makeTemplate("XTFXULVisual");
XTFXULVisual.mergeTemplate(XTFVisual);
XTFXULVisual.addInterface(Components.interfaces.nsIXTFXULVisual);
XTFXULVisual.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_XUL_VISUAL});

// We share a document for the visual content between all visuals of
// the same type:
var _XTFXULVisual_content_doc = null;
XTFXULVisual.bindInitializer("XTFVisual",
{contentDocGetter: function() {
  if (!_XTFXULVisual_content_doc) {
    _XTFXULVisual_content_doc = Components.classes["@mozilla.org/xul/xul-document;1"].createInstance();
  }
  return _XTFXULVisual_content_doc;
}});


//----------------------------------------------------------------------
// XTFElementFactory

var XTFElementFactory = makeTemplate("XTFElementFactory");
XTFElementFactory.mergeTemplate(SupportsTemplate);
XTFElementFactory.addInterface(Components.interfaces.nsIXTFElementFactory);

XTFElementFactory.addProtoObj("createElement",
  function(tagname) {
    this._dump("createElement("+tagname+")\n");
    var instantiator = this._elements[tagname];
    if (!instantiator) throw Components.results.NS_ERROR_FAILURE;
    return instantiator.instantiate();
  });

XTFElementFactory.addProtoObj("_elements", {} /*, XXX want some merge flag here */);

XTFElementFactory.addOpsObject("addElement",
  function(name, instantiator) {
    this.getProtoObj("_elements")[name] = instantiator;
  });
