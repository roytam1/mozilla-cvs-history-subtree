// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.mozIJSComponentLib).probeComponent('rel:canvas.js')" -*-
// canvas.js
// simple canvas/widgets in xtf


debug("*** loading canvas.js \n");

importModule("resource:/jscodelib/StdLib.js");
importModule("resource:/jscodelib/JSComponentUtils.js");
importModule("resource:/jscodelib/TemplateLib.js");
importModule("resource:/jscodelib/xtf.js");

//----------------------------------------------------------------------
// docElement

var docElement = makeTemplate("docElement");
docElement.mergeTemplate(XTFGenericElement);

//----------------------------------------------------------------------
// canvasElement

var canvasElement = makeTemplate("canvasElement");
canvasElement.mergeTemplate(XTFXMLVisual);

canvasElement.addProtoObj(
  "_buildVisualContent",
  function(builder) {
    builder.setElementNamespace(SVG_NS);
    builder.beginElement("svg");
    this._svg = builder.current;
      builder.attribs({height:"1000px", width:"1000px"});
      builder.beginElement("rect");
        builder.attribs({style:"fill:#ddeeff;", width:"100%", height:"100%"});        
      builder.endElement();
    builder.endElement();
  });


canvasElement.addProtoObj(
  "childInserted",
  function(child, index) {
    try {
      child.QueryInterface(Components.interfaces.nsIXTFWidget);
    }
    catch(e) {
      this._warning("childInserted("+child+"): no nsIXTFWidget interface");
      return;
    }
    this._dump("appending child "+child);
    this._svg.appendChild(child.visual);
  });

canvasElement.addProtoObj(
  "childAppended",
  function(child) { this.childInserted(child, -1); });

canvasElement.addProtoObj(
  "documentChanged",
  function(doc) {
    this._dump(" documentChanged: documentFrameElement="+this._wrapper.documentFrameElement);
  });

canvasElement.addProtoObj(
  "_XTFXMLVisual$onCreated",
  canvasElement.getProtoObj("onCreated"));
canvasElement.addProtoObj(
  "onCreated",
  function(wrapper) {
    this._XTFXMLVisual$onCreated(wrapper);
    this.addNotification(Components.interfaces.nsIXTFElement.NOTIFY_CHILD_INSERTED |
                         Components.interfaces.nsIXTFElement.NOTIFY_CHILD_APPENDED |
                         Components.interfaces.nsIXTFElement.NOTIFY_DOCUMENT_CHANGED);

    this._dump("onCreated: elementNode="+this._wrapper.elementNode+"\ndocumentFrameElement="+this._wrapper.documentFrameElement+"\nownerDoc="+this._wrapper.ownerDocument+"\nelementNode.ownerDoc="+this._wrapper.elementNode.ownerDocument);
  });

canvasElement.addProtoObj(
  "_XTFXMLVisual$onDestroyed",
  canvasElement.getProtoObj("onDestroyed"));
canvasElement.addProtoObj("onDestroyed",
  function() {
    this._dump("onDestroyed");
    delete this._svg;
    this._XTFXMLVisual$onDestroyed();
  });

// canvasElement.addProtoObj("getAttribute",
//                           function(name) {
//                             this._dump("getting attr "+name);
//                             if (name=="minwidth"||name=="minheight") return "10000px"; });

//----------------------------------------------------------------------
// line element

var lineElement = makeTemplate("lineElement");
lineElement.mergeTemplate(XTFGenericElement);

lineElement.addInterface(Components.interfaces.nsIXTFWidget);
lineElement.addProtoGetter("visual",
  function() {
    if (!this._visual) {
      var builder = ContentBuilder.instantiate();
      builder.setElementNamespace(SVG_NS);
      builder.beginElement("circle");
        builder.attribs({cx:"100", cy:"100", r:"100", style:"fill:red;"});
      builder.endElement();
      this._visual = builder.root;
    }
    return this._visual;
  });

//----------------------------------------------------------------------
// canvasFactory

var canvasFactory = makeTemplate("canvasFactory");
canvasFactory.mergeTemplate(XTFElementFactory);

canvasFactory.addElement("doc", docElement);
canvasFactory.addElement("canvas", canvasElement);
canvasFactory.addElement("line", lineElement);

var theCanvasFactory = canvasFactory.instantiate();

// hand a proxied factory to XPCOM to support dynamic reloading of
// this file:
var theCanvasFactoryProxied = createDynamicProxy(function(){return theCanvasFactory;});
// XXX until bug#244696 is fixed we need to explicitly add a createElement method:
theCanvasFactory._createElement = theCanvasFactory.createElement;
theCanvasFactoryProxied.createElement = function(tn) {return this._createElement(tn);};

//----------------------------------------------------------------------
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "XTFCanvas XTF element factory",
     cid        : Components.ID("{2d2c2227-70db-44f7-906f-324429c9efed}"),
     contractID : "@mozilla.org/xtf/element-factory;1?namespace=urn:mozilla:xtf:tests:canvas",
     factory    : ComponentUtils.generateFactory(function() { return theCanvasFactoryProxied; })
  }]);
                                                  
