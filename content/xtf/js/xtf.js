/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/xtf.js')" -*- */
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

MOZ_EXPORTED_SYMBOLS = [ "XHTML_NS", "SVG_NS", "XUL_NS",
                         "ContentBuilder",
                         "XTFGenericElement",
                         "XTFSVGVisual",
                         "XTFXMLVisual",
                         "XTFXULVisual",
                         "XTFElementFactory",
                         "XTFMappedAttributeHandler",
                         "XTFUnwrappable",
                         "unwrapXTFElement" ];

//----------------------------------------------------------------------
// Global constants

var XHTML_NS = "http://www.w3.org/1999/xhtml";
var SVG_NS   = "http://www.w3.org/2000/svg";
var XUL_NS   = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

//----------------------------------------------------------------------
// ContentBuilder: simple interface to facilitate constructing xml
// content

var ContentBuilder = makeTemplate("ContentBuilder");

ContentBuilder.mergeTemplate(ErrorTemplate);

ContentBuilder.appendInitializer(
  "ContentBuilder",
  function() {
    this.builder = Components.classes["@mozilla.org/xtf/xml-contentbuilder;1"].createInstance(Components.interfaces.nsIXMLContentBuilder);
  });

ContentBuilder.addProtoObj(
  "clear",
  function() {
    this.builder.clear();
  });

ContentBuilder.addProtoObj(
  "setElementNamespace",
  function(ns) {
    this.builder.setElementNamespace(ns);
    return this;
  });

ContentBuilder.addProtoObj(
  "beginElement",
  function(tagname) {
    this.builder.beginElement(tagname);
    return this;
  });

ContentBuilder.addProtoObj(
  "endElement",
  function() {
    this.builder.endElement();
    return this;
  });

ContentBuilder.addProtoObj(
  "attrib",
  function(name, value) {
    this.builder.attrib(name, value);
    return this;
  });

ContentBuilder.addProtoObj(
  "attribs",
  function(obj) {
    for (var a in obj)
      this.builder.attrib(a, obj[a]);
  });

ContentBuilder.addProtoObj(
  "textNode",
  function(text) {
    this.builder.textNode(text);
    return this;
  });

ContentBuilder.addProtoGetter(
  "root",
  function() {
    return this.builder.root;
  });

ContentBuilder.addProtoGetter(
  "current",
  function() {
    return this.builder.current;
  });

//----------------------------------------------------------------------
// XTFElement: common base template for XTFGenericElement and
// XTF visuals

var XTFElement = makeTemplate("XTFElement");

XTFElement.mergeTemplate(ErrorTemplate);
XTFElement.mergeTemplate(SupportsTemplate);

XTFElement.appendInitializer(
  "XTFElement",
  function(args) {
    if (!args || args.type === undefined) this._error("initializer arg 'type' missing");
    this._elementType = args.type;
  });
    
XTFElement.addInterface(Components.interfaces.nsIXTFElement);

XTFElement.addProtoGetter(
  "elementType",
  function () {
    return this._elementType;
  });

XTFElement.addProtoGetter(
  "isAttributeHandler",
  function () {
    return false;
  });

XTFElement.addProtoObj(
  "getScriptingInterfaces",
  function(count) {
    count.value = this._scriptingInterfaces.length;
    return this._scriptingInterfaces;
  });

XTFElement.addProtoObj(
  "_scriptingInterfaces",
  [],
  { mergenew:  cloneArray,
    mergeover: appendUnique });

XTFElement.addOpsObject(
  "addScriptingInterface",
  function(itf) {
    this.getProtoObj("_scriptingInterfaces").push(itf);
  });

XTFElement.addProtoGetter(
  "notificationMask",
  function() { return this._notificationMask; });

XTFElement.addProtoObj(
  "_notificationMask",
  0,
  { mergeover: function(d,s) { return d|s; } });

XTFElement.addOpsObject(
  "addNotification",
  function(n) {
    this._proto._notificationMask |= n;
  });

// onCreated really belongs to sub-interfaces of nsIXMLElement, but it
// makes sense to implement this here, because JS isn't typed and the
// different signatures of onCreate only differ in argument type:

XTFElement.addProtoObj(
  "onCreated",
  function(wrapper) {
    this._wrapper = wrapper;
  });

XTFElement.addProtoObj(
  "onDestroyed",
  function() {
    this._dump("onDestroyed");
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

// we generally want visuals to behave like 'normal' dom elements:
XTFVisual.addScriptingInterface(Components.interfaces.nsIDOMNode);
XTFVisual.addScriptingInterface(Components.interfaces.nsIDOMElement);

XTFVisual.addProtoObj("XTFElement$onCreated",
                      XTFVisual.getProtoObj("onCreated"));
XTFVisual.addProtoObj(
  "onCreated",
  function(wrapper) {
    var builder = ContentBuilder.instantiate();
    this._buildVisualContent(builder);
    this._visualContent = builder.root;
    
    return this.XTFElement$onCreated(wrapper);
  });

XTFVisual.addProtoGetter(
  "visualContent",
  function() {
    return this._visualContent;
  });

XTFVisual.addProtoObj(
  "_buildVisualContent",
  function(builder) {});

XTFVisual.addProtoObj(
  "XTFElement$onDestroyed",
  XTFVisual.getProtoObj("onDestroyed"));
XTFVisual.addProtoObj(
  "onDestroyed",
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

  
//----------------------------------------------------------------------
// XTFXMLVisual

var XTFXMLVisual = makeTemplate("XTFXMLVisual");
XTFXMLVisual.mergeTemplate(XTFVisual);
XTFXMLVisual.addInterface(Components.interfaces.nsIXTFXMLVisual);
XTFXMLVisual.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_XML_VISUAL});


//----------------------------------------------------------------------
// XTFXULVisual

var XTFXULVisual = makeTemplate("XTFXULVisual");
XTFXULVisual.mergeTemplate(XTFVisual);
XTFXULVisual.addInterface(Components.interfaces.nsIXTFXULVisual);
XTFXULVisual.bindInitializer("XTFElement",
  {type: Components.interfaces.nsIXTFElement.ELEMENT_TYPE_XUL_VISUAL});


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

//----------------------------------------------------------------------
// XTFMappedAttributeHandler

var XTFMappedAttributeHandler = makeTemplate("XTFMappedAttributeHandler");
XTFMappedAttributeHandler.mergeTemplate(ErrorTemplate);
XTFMappedAttributeHandler.mergeTemplate(SupportsTemplate);
XTFMappedAttributeHandler.addInterface(Components.interfaces.nsIXTFAttributeHandler);

XTFMappedAttributeHandler.addProtoGetter(
  "isAttributeHandler",
  function() {return true;});

XTFMappedAttributeHandler.addProtoObj(
  "handlesAttribute",
  function(name) {
    if (this._attributemap[name] || this._attributemap["*"])
      return true;
    return false;
  });

XTFMappedAttributeHandler.addProtoObj(
  "setAttribute",
  function(name, value) {
    this._dump("setAttribute "+name+" "+value);
    var attr;
    if ((attr = this._attributemap[name])) {
      if ("set" in attr) 
        attr.set.apply(this, arguments);
      else if ("varname" in attr)
        this[attr.varname] = value;
      else if ("delegate" in attr) {
        var attrName = attr.delegateAttrib;
        if (!attrName) attrName = name; 
        var hadAttr = this[attr.delegate].hasAttribute(attrName);
        this[attr.delegate].setAttribute(attrName, value);
        if (!hadAttr)
          this._attributeCountChanged();
      }
      else
        this._error("setAttribute: corrupt attribute descriptor for attrib '"+name+"'");
    }
    else if ((attr = this._attributemap["*"])) {
      if ("set" in attr)
        attr.set.apply(this, arguments);
      else
        this._error("setAttribute: corrupt '*' attribute descriptor on setting '"+name+"'");
    }
    else {
      this._error("setAttribute: unknown attribute '"+name+"'");
    }
  });

XTFMappedAttributeHandler.addProtoObj(
  "removeAttribute",
  function(name) {
    this._dump("removeAttribute "+name);
    var attr;
    if ((attr = this._attributemap[name])) {
      if ("remove" in attr) 
        attr.remove.apply(this, arguments);
      else if (("varname" in attr)) {
        if (this.hasOwnProperty(attr.varname))
          delete this[attr.varname];
        else
          this._error("removeAttribute: attribute '"+name+"' is already removed");
      }
      else if ("delegate" in attr) {
        var attrName = attr.delegateAttrib;
        if (!attrName) attrName = name;
        var hadAttr = this[attr.delegate].hasAttribute(attrName);
        this[attr.delegate].removeAttribute(attrName);
        if (hadAttr)
          this._attributeCountChanged();
      }
      else
        this._error("removeAttribute: corrupt attribute descriptor for attrib '"+name+"'");
    }
    else if ((attr = this._attributemap["*"])) {
      if ("remove" in attr)
        attr.remove.apply(this, arguments);
      else
        this._error("removeAttribute: corrupt '*' attribute descriptor on setting '"+name+"'");
    }
    else
      this._error("removeAttribute: unknown attribute '"+name+"'");
  });

XTFMappedAttributeHandler.addProtoObj(
  "getAttribute",
  function(name) {
    this._dump("getAttribute "+name);
    var attr;
    if ((attr = this._attributemap[name])) {
      if ("get" in attr) 
        return attr.get.apply(this, arguments);
      else if ("varname" in attr)
        return this[attr.varname];
      else if ("delegate" in attr) {
        var attrName = attr.delegateAttrib;
        if (!attrName) attrName = name;
        if (!this[attr.delegate].hasAttribute(attrName))
          return null;
        return this[attr.delegate].getAttribute(attrName);
      }
      else
        this._error("getAttribute: corrupt attribute descriptor for attrib '"+name+"'");
    }
    else if ((attr = this._attributemap["*"])) {
      if ("get" in attr)
        return attr.get.apply(this, arguments);
      else
        this._error("getAttribute: corrupt '*' attribute descriptor on getting '"+name+"'");
    }
    else {
      this._warning("getAttribute: unknown attribute '"+name+"'");
      return null;
    }
  });

XTFMappedAttributeHandler.addProtoObj(
  "hasAttribute",
  function(name) {
    this._dump("hasAttribute "+name);
    var attr;
    if ((attr = this._attributemap[name])) {
      if ("has" in attr)
        return attr.has.apply(this, arguments);
      else if ("varname" in attr) { 
        return this.hasOwnProperty(attr.varname);
      }
      else if ("delegate" in attr) { 
        var attrName = attr.delegateAttrib;
        if (!attrName) attrName = name;
        return this[attr.delegate].hasAttribute(attrName);
      }
      else
        this._error("hasAttribute: corrupt attribute descriptor for attrib '"+name+"'");
    }
    else if ((attr = this._attributemap["*"])) {
      if ("has" in attr)
        return attr.has.apply(this, arguments);
      else 
        this._error("hasAttribute("+name+"): corrupt '*' attribute descriptor");
    }
    else
      return false;
  });

XTFMappedAttributeHandler.addProtoObj(
  "getAttributeCount",
  function() {
    this._dump("getAttributeCount"); 
    return this._getAttributelist().length;
  });

// global ref to atom service for 'getAttributeNameAt':
var gAtomservice = Components.classes["@mozilla.org/atom-service;1"]
                   .getService(Components.interfaces.nsIAtomService);

XTFMappedAttributeHandler.addProtoObj(
  "getAttributeNameAt",
  function(index) {
    this._dump("getAttributeNameAt "+index); 
    return gAtomservice.getAtom(this._getAttributelist()[index]);
  });

XTFMappedAttributeHandler.addProtoObj(
  "_attributeCountChanged",
  function() {
    delete this.__attributelist;
  });

XTFMappedAttributeHandler.addProtoObj(
  "_getAttributelist",
  function() {
    if (!this.__attributelist) {
      // count the attribs and store names in array
      this.__attributelist = [];
      for (var a in this._attributemap) {
        if (a!="*") {
          if (this.hasAttribute(a))
            this.__attributelist.push(a);
        }
        else { // "*"-handler
          this._assert(this._attributemap[a].count &&
                       this._attributemap[a].nameAt,
                       "getAttributeCount: corrupt '*' attribute descriptor");
          var l = this._attributemap[a].count.apply(this);
          for (var i=0; i<l; ++i)
            this.__attributelist.push(this._attributemap[a].nameAt.call(this, i));
        }
      }
    }
    return this.__attributelist;
  });

XTFMappedAttributeHandler.addProtoObj("_attributemap", {}, {mergenew: cloneObj});

XTFMappedAttributeHandler.addOpsObject("mapAttribute",
  function(name, descriptor) {
    this.getProtoObj("_attributemap")[name] = descriptor;
  });

//----------------------------------------------------------------------
// XTFUnwrappable: a template to implement the nsIXTFPrivate interface
// and 'wrappedJSObject' property, giving access to the underlying JS XTF
// object via its wrapper.

var XTFUnwrappable = makeTemplate("XTFUnwrappable");
XTFUnwrappable.mergeTemplate(SupportsTemplate);
XTFUnwrappable.addInterface(Components.interfaces.nsIXTFPrivate);
XTFUnwrappable.addProtoGetter("inner",
  function() {
    return this;
  });
XTFUnwrappable.addProtoGetter("wrappedJSObject",
  function() {
    return this;
  });

// utility for unwrapping an unwrappable wrapped js-implemented xtf
// element:
function unwrapXTFElement(element) {
  element.QueryInterface(Components.interfaces.nsIXTFPrivate);
  return element.inner.wrappedJSObject;
}
