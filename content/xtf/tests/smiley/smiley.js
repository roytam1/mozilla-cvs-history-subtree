// smiley.js
// sample implementation of simple svg-compatible tag in xtf

const SVG_NS = "http://www.w3.org/2000/svg";

//----------------------------------------------------------------------
// base element class
// (nsIXTFElement implementation to fall through to)

var genericElement = {
  getScriptingInterfaces: function(count) {
    var ifs = [Components.interfaces.nsIDOMElement];
    count.value = ifs.length;
    return ifs;
  },
  
  willChangeDocument: function(newDoc) {},
  documentChanged: function(newDoc) {},
  
  willChangeParent: function(newParent) {},
  parentChanged: function(newParent) {},
  
  willInsertChild: function(child, index) {},
  childInserted: function(child, index) {},
  
  willReplaceChild: function(child, index) {},
  childReplaced: function(child, index) {},
  
  willAppendChild: function(child) {},
  childAppended: function(child) {},
  
  willRemoveChild: function(index) {},
  childRemoved: function(index) {},
  
  willSetAttribute: function(name, newValue) {},
  AttributeSet: function(name, newValue) {},
  
  willUnsetAttribute: function(name) {},
  AttributeUnset: function(name) {}
};    


//----------------------------------------------------------------------
// smiley element

function xtfSmiley() {
  this.fragment =
    Components.classes["@mozilla.org/xtf/xml-contentfragment;1"]
              .createInstance(Components.interfaces.nsIXMLContentFragment);
  this.g = this.fragment.beginElement(SVG_NS, "g");
      this.fragment.attrib("style", "fill:yellow;");
    this.fragment.beginElement(SVG_NS, "circle");
      this.fragment.attrib("cx", "100");
      this.fragment.attrib("cy", "100");
      this.fragment.attrib("r", "120");
    this.fragment.endElement();
    this.fragment.beginElement(SVG_NS, "circle");
      this.fragment.attrib("cx", "50");
      this.fragment.attrib("cy", "50");
      this.fragment.attrib("r", "10");
      this.fragment.attrib("style", "fill:black;");
    this.fragment.endElement();
    this.fragment.beginElement(SVG_NS, "circle");
      this.fragment.attrib("cx", "150");
      this.fragment.attrib("cy", "50");
      this.fragment.attrib("r", "10");
      this.fragment.attrib("style", "fill:black;");
    this.fragment.endElement();
    this.mouth = this.fragment.beginElement(SVG_NS, "path");
      this.fragment.attrib("d", "M50,150 Q100,200 150,150");
      this.fragment.attrib("style", "fill:none;stroke:black;stroke-width:10;");
    this.fragment.endElement();
  this.fragment.endElement();

  var instance = this;
  this.g.addEventListener("mousedown", function(){instance.makeSad()}, true);
}

xtfSmiley.prototype = {
  __proto__: genericElement,
  
  // nsISupports implementation:
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIXTFSVGVisual) &&
        !iid.equals(Components.interfaces.nsIXTFElement) &&
        !iid.equals(Components.interfaces.nsISupports)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  
  // nsIXTFElement implementation:
  onDestroyed: function() {
    // clean up:
    delete this.fragment;
    delete this.g;
    delete this.wrapper;
  },
  get elementType() {
    return Components.interfaces.nsIXTFElement.ELEMENT_TYPE_SVG_VISUAL;
  },
  AttributeSet: function(name, newValue) {
    // let our 'g' element inherit all attribs:
    this.g.setAttribute(name, newValue);
  },
  
  // nsIXTFSVGVisual implementation:
  onCreated: function(wrapper) {
    this.wrapper = wrapper;
  },
  get visualContent() {
    return this.fragment.content;
  },

  // implementation helpers:
  makeSad: function(evt) {
    // move controlpoint of quadratic bezier element:
    var quad = this.mouth.pathSegList.getItem(1);
    quad.y1 -= 10;
  }
};

  
//----------------------------------------------------------------------
// xtfSmileyFactory

const XTF_SHAPE_FACTORY_CID = Components.ID("{7d2bff65-6d03-4d0e-ae9f-b3102c5e606e}");
const XTF_SHAPE_FACTORY_PROGID = "@mozilla.org/xtf/element-factory;1?namespace=urn:mozilla:xtf:tests:smiley";

function xtfSmileyFactory() { debug("xtfSmileyFactory CTOR \n"); }

xtfSmileyFactory.prototype = {
    // nsISupports implementation:
    QueryInterface: function (iid) {
      if (!iid.equals(Components.interfaces.nsIXTFElementFactory) &&
          !iid.equals(Components.interfaces.nsISupports)) {
        debug("unknown interface "+iid+" requested from xtfSmileyFactory\n");
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
    },

    // nsIXTFElementFactory implementation:
    createElement: function (tagName) {
      debug("xtfSmileyFactory::createElement("+tagName+")\n");
      if (tagName=="smiley")
        return new xtfSmiley();
      else
        throw Components.results.NS_ERROR_FAILURE;
    }
};

//----------------------------------------------------------------------
// xtfSmileyModule
    
var xtfSmileyModule = {
    registerSelf: function (compMgr, fileSpec, location, type) {
        debug("*** xtfSmileyModule::registerSelf\n");
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(XTF_SHAPE_FACTORY_CID,
                                        "XTFSmiley XTF element factory",
                                        XTF_SHAPE_FACTORY_PROGID, 
                                        fileSpec,
                                        location,
                                        type);
    },

    getClassObject: function (compMgr, cid, iid) {
        debug("xtfSmileyModule::getClassObject()\n");
        if (!cid.equals(XTF_SHAPE_FACTORY_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.myFactory;
    },

    myFactory: {
        createInstance: function (outer, iid) {
            debug("xtfSmileyModule::createInstance: " + iid + "\n");
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new xtfSmileyFactory()).QueryInterface(iid);
        }
    },

    canUnload: function(compMgr) {
        debug("*** xtfSmileyModule::canUnload\n");
        return true;
    }
};

function NSGetModule(compMgr, fileSpec) {
    return xtfSmileyModule;
}
