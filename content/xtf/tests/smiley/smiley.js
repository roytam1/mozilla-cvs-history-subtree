// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.mozIJSComponentLib).probeComponent('rel:smiley.js')" -*-
// smiley.js
// sample implementation of simple svg-compatible tag in xtf


debug("*** loading smiley.js \n");

importModule("resource:/jscodelib/StdLib.js");
importModule("resource:/jscodelib/JSComponentUtils.js");
importModule("resource:/jscodelib/TemplateLib.js");
importModule("resource:/jscodelib/xtf.js");

//----------------------------------------------------------------------
// smileyElement

var smileyElement = makeTemplate("smileyElement");
smileyElement.mergeTemplate(XTFSVGVisual);

//smileyElement.addProtoObj("_dump", function() {});

// smileyElement.appendInitializer(
//   "smileyElement_eventsListeners",
//   function() {
//      var instance = this;
//      this.mousedownListener = function() {instance._makeSad();};
//   });

// by default we apply doc style sheets:
smileyElement.addProtoGetter(
  "applyDocumentStyleSheets",
  function() {
    return false;
  });

smileyElement.addProtoObj(
  "_buildVisualContent",
  function(builder) {
//    builder.setElementNamespace(SVG_NS);
    builder.beginElement("g");
//    this._g = builder.current;
//       builder.attribs({fill:"yellow"});
//       builder.beginElement("circle");
//         builder.attribs({cx:"100", cy:"100", r:"120"});
//       builder.endElement();
//       builder.beginElement("circle");
//         builder.attribs({cx:"50", cy:"50", r:"10", fill: "black"});
//       builder.endElement();
//       builder.beginElement("circle");
//         builder.attribs({cx:"150", cy:"50", r:"10", fill: "black"});
//       builder.endElement();
//       builder.beginElement("path");
//      this._mouth = builder.current;
//         builder.attrib("d", "M50,150 Q100,200 150,150");
//         builder.attribs({fill:"none", stroke:"black", "stroke-width":"10"});
//       builder.endElement();
    builder.endElement();

//     // set up event listener:
//     var instance = this;
//     this._g.addEventListener("mousedown", this.mousedownListener, true);
   });

// smileyElement.addProtoObj("_makeSad",
//   function() {
//     // move controlpoint of quadratic bezier element:
//     var quad = this._mouth.pathSegList.getItem(1);
//     quad.y1 -= 10;
//   });

// // let our 'g' element handle all of our attribs:
// smileyElement.mergeTemplate(XTFMappedAttributeHandler);

// smileyElement.mapAttribute("*",
//   {
//     get: function(name) {
//       return this._g.getAttribute(name);
//     },
//     set: function(name, value) {
//       this._g.setAttribute(name, value);
//       this._attributeCountChanged();
//     },
//     remove: function(name) {
//       this._dump("removing "+name);
//       this._g.removeAttribute(name);
//       this._attributeCountChanged();
//     },
//     has: function(name) {
//       return this._g.hasAttribute(name);
//     },
//     count: function() {
//       return this._g.attributes.length;
//     },
//     nameAt: function(i) {
//       return this._g.attributes.item(i).name;
//     }
//   });

// smileyElement.addProtoObj("XTFSVGVisual$onDestroyed",
//                           smileyElement.getProtoObj("onDestroyed"));
// smileyElement.addProtoObj("onDestroyed",
//   function() {
//     this._dump("onDestroyed");
//     delete this._mouth;
//     this._g.removeEventListener("mousedown", this.mousedownListener, true);
//     delete this._g;
//     this.XTFSVGVisual$onDestroyed();
//   });

//----------------------------------------------------------------------
// smileyFactory

var smileyFactory = makeTemplate("smileyFactory");
smileyFactory.mergeTemplate(XTFElementFactory);

smileyFactory.addProtoObj("_dump", function() {});


smileyFactory.addElement("smiley", smileyElement);

var theSmileyFactory = smileyFactory.instantiate();

// hand a proxied factory to XPCOM to support dynamic reloading of
// this file:
var theSmileyFactoryProxied = createDynamicProxy(function(){return theSmileyFactory;});
// XXX until bug#244696 is fixed we need to explicitly add a createElement method:
theSmileyFactory._createElement = theSmileyFactory.createElement;
theSmileyFactoryProxied.createElement = function(tn) {return this._createElement(tn);};

//----------------------------------------------------------------------
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "XTFSmiley XTF element factory",
     cid        : Components.ID("{7d2bff65-6d03-4d0e-ae9f-b3102c5e606e}"),
     contractID : "@mozilla.org/xtf/element-factory;1?namespace=urn:mozilla:xtf:tests:smiley",
     factory    : ComponentUtils.generateFactory(function() { return theSmileyFactoryProxied; })
  }]);
                                                  
