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

smileyElement.appendInitializer("smileyElement_eventsListeners",
  function() {
    var instance = this;
    this.mousedownListener = function() {instance._makeSad();};
  });

smileyElement.addProtoObj("_buildVisualContent",
  function(builder) {
    builder.setElementNamespace(SVG_NS);
    this._g = builder.beginElement("g");
      builder.attrib("style", "fill:yellow;");
      builder.attrib("stroke", "blue");
      builder.attrib("stroke-width", "10px");
      builder.beginElement("circle");
        builder.attribs({cx:"100", cy:"100", r:"120"});
      builder.endElement();
      builder.beginElement("circle");
        builder.attribs({cx:"50", cy:"50", r:"10", style: "fill:black;"});
      builder.endElement();
      builder.beginElement("circle");
        builder.attribs({cx:"150", cy:"50", r:"10", style: "fill:black;"});
      builder.endElement();
      this._mouth = builder.beginElement("path");
        builder.attrib("d", "M50,150 Q100,200 150,150");
        builder.attrib("style", "fill:none;stroke:black;stroke-width:10;");
      builder.endElement();
    builder.endElement();

    // set up event listener:
    var instance = this;
    this._g.addEventListener("mousedown", this.mousedownListener, true);
   });

smileyElement.addProtoObj("_makeSad",
  function() {
    // move controlpoint of quadratic bezier element:
    var quad = this._mouth.pathSegList.getItem(1);
    quad.y1 -= 10;
  });

smileyElement.addProtoObj("attributeSet",
  function(name, value) {
    // let our 'g' element inherit all attribs:
    this._g.setAttribute(name, value);
  });

smileyElement.addProtoObj("XTFSVGVisual$onDestroyed",
                          smileyElement.getProtoObj("onDestroyed"));
smileyElement.addProtoObj("onDestroyed",
  function() {
    this._dump("onDestroyed");
    delete this._mouth;
    this._g.removeEventListener("mousedown", this.mousedownListener, true);
    delete this._g;
    this.XTFSVGVisual$onDestroyed();
  });

//----------------------------------------------------------------------
// smileyFactory

var smileyFactory = makeTemplate("smileyFactory");
smileyFactory.mergeTemplate(XTFElementFactory);

smileyFactory.addElement("smiley", smileyElement);

var theSmileyFactory = smileyFactory.instantiate();


//----------------------------------------------------------------------
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "XTFSmiley XTF element factory",
     cid        : Components.ID("{7d2bff65-6d03-4d0e-ae9f-b3102c5e606e}"),
     contractID : "@mozilla.org/xtf/element-factory;1?namespace=urn:mozilla:xtf:tests:smiley",
     factory    : ComponentUtils.generateFactory(function() { return createDynamicProxy(function(){return theSmileyFactory;}); })
  }]);
                                                  
