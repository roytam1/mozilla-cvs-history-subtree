// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/xtf_test_resources/perf.js', 1)" -*-


//importModule("resource:/jscodelib/StdLib.js");
importModule("resource:/jscodelib/ProfileLib.js");
//importModule("resource:/jscodelib/TemplateLib.js");
importModule("resource:/jscodelib/xtf.js");

var ds = Components.classes["@mozilla.org/js/jsd/debugger-service;1"].getService(Components.interfaces.jsdIDebuggerService);

//----------------------------------------------------------------------

/* Measure the time it takes to load a document. Loading is
 * asynchronous, so we let the jssh shell enter a 'suspended' state
 * where it will sit and pump messages until awakened by a call to
 * 'resume()'. We issue this call from
 * nsBrowserStatusHandler::endDocumentLoad().
 *
 * What exactly does this function measure?  endDocumentLoad() fires
 * after the content & frame tree construction & reflow, but before
 * the document is actually rendered. Ideally we would like to measure
 * the rendering time as well, but I don't know of any way to get an
 * event that fires after rendering. The DOM onpaint event, e.g.,
 * doesn't work (bug#239074).
 *
 * Measuring the rendering time separately from loading by initiating
 * a synchronous reload doesn't work either. nsIBaseWindow::reload()
 * isn't implemented for XUL Windows and for other windows it doesn't
 * respect the 'synchronous' flag. *sigh*
 * For SVG we can use nsIDOMSVGSVGElement::forceRedraw(), though.
 */
function timeLoad(url) {
  var t = Number.MAX_VALUE;
  var win0 = getWindows()[0];

  try{
    instrument("getWindows()[0].XULBrowserWindow", "endDocumentLoad",
               function(){},
               function(){ flushEventQueue(); resume();});
    t = time(function() { win0.loadURI(url); suspend(); });
  }
  catch(e){ dump(e);}
  finally{
    uninstrument("getWindows()[0].XULBrowserWindow", "endDocumentLoad");
  }
  return t;
}

//----------------------------------------------------------------------
//
function syncLoad(url) {
  var win0 = getWindows()[0];
  try{
    instrument("getWindows()[0].XULBrowserWindow", "endDocumentLoad",
               function(){},
               function(){ flushEventQueue(); resume();});
    win0.loadURI(url);
    suspend();
  }
  catch(e){ dump(e);}
  finally{
    uninstrument("getWindows()[0].XULBrowserWindow", "endDocumentLoad");
  }
}

//----------------------------------------------------------------------
function runTest(name,runs,reps) {
  var fct = this[name];
  function test() {
    for (var i=0; i<reps; ++i)fct();
  }
  var times = [];
  for (var i=0; i<runs; ++i)
    times.push(time(test)/reps);
  var moments = calculateMoments(times);
  print(name+": ("+moments.mean+" +/- "+moments.standard_deviation+") ms   [reps:"+reps+" runs:"+runs+"]\n");
  ds.GC();
  return {data:times, moments:moments};
}

//----------------------------------------------------------------------
// wrapping a native object I:
function wrapnative() {
  var b = Components.classes["@mozilla.org/xtf/xml-contentbuilder;1"].createInstance(Components.interfaces.nsIXMLContentBuilder);
}

//----------------------------------------------------------------------
var builder;

function nativeCall() {
  builder.setElementNamespace("http://www.w3.org/2000/svg");
}

//----------------------------------------------------------------------
function dumpTest() {
  dump("*");
}

//----------------------------------------------------------------------
function noop() {
}

//----------------------------------------------------------------------
function gc() {
  ds.GC();
}

//----------------------------------------------------------------------
function createDoc() {
  var d = Components.classes["@mozilla.org/xul/xul-document;1"].createInstance();
}

//----------------------------------------------------------------------
var doc = Components.classes["@mozilla.org/xul/xul-document;1"].createInstance();
function createXMLElement() {
  var elem = doc.createElement("foo");
}

//----------------------------------------------------------------------
function createSVGElement() {
  var elem = doc.createElementNS("http://www.w3.org/2000/svg", "circle");
}

//----------------------------------------------------------------------
function createHTMLElement() {
  var elem = doc.createElementNS("http://www.w3.org/1999/xhtml", "div");
}

//----------------------------------------------------------------------
function createXULElement() {
  var elem = doc.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "box");
}

//----------------------------------------------------------------------
function buildSVGElement() {
  builder.beginElement("circle");
  builder.endElement(); 
}

//----------------------------------------------------------------------
function buildAndWrapSVGElement() {
  builder.beginElement("circle");
  var elem = builder.current;
  //dump(elem);
  builder.endElement(); 
}

//----------------------------------------------------------------------
function buildSVGElementChain() {
  builder.beginElement("g");
}

//----------------------------------------------------------------------
function instantiateXTFSVGVisual() {
  var a = XTFSVGVisual.instantiate();
  a.onCreated({});
}

//----------------------------------------------------------------------
function buildSmileyElement() {
  builder.beginElement("smiley");
  builder.endElement(); 
}

//----------------------------------------------------------------------
function buildAndWrapSmileyElement() {
  builder.beginElement("smiley");
  var elem = builder.current;
  //dump(elem);
  builder.endElement(); 
}


//----------------------------------------------------------------------
function loadSmiley25() {
  syncLoad("resource:///xtf_test_resources/smiley25.svg");
}
function loadSmiley50() {
  syncLoad("resource:///xtf_test_resources/smiley50.svg");
}
function loadSmiley100() {
  syncLoad("resource:///xtf_test_resources/smiley100.svg");
}

function loadSmiley25c() {
  syncLoad("resource:///xtf_test_resources/smiley25-control.svg");
}
function loadSmiley50c() {
  syncLoad("resource:///xtf_test_resources/smiley50-control.svg");
}
function loadSmiley100c() {
  syncLoad("resource:///xtf_test_resources/smiley100-control.svg");
}


//----------------------------------------------------------------------
function tests() {
 builder = Components.classes["@mozilla.org/xtf/xml-contentbuilder;1"].createInstance(Components.interfaces.nsIXMLContentBuilder);
 builder.setElementNamespace("http://www.w3.org/2000/svg");
 builder.beginElement("svg");
 
//  runTest("wrapnative", 10, 200);
//  runTest("nativeCall", 10, 1000);
//  runTest("dumpTest", 10, 200);
//   runTest("noop", 10, 10000);
//   runTest("gc", 10, 5);
//   runTest("createDoc", 10, 200);
//   runTest("createXMLElement", 10, 200);
//   runTest("createSVGElement", 10, 200);
//   runTest("createHTMLElement", 10, 200);
//   runTest("createXULElement", 10, 200);
//  runTest("buildSVGElement", 10, 1000);
//  runTest("buildAndWrapSVGElement", 10, 50);
 var d = runTest("buildSVGElementChain", 10, 20).data;
 
 for (var i=0, l=d.length;i<l;++i) {
   print(i+","+d[i]+"\n");
 }
 print("\n");
 runTest("buildSVGElement", 10, 50);
// runTest("instantiateXTFSVGVisual", 10, 100);
 //builder.clear
 //builder.setElementNamespace("urn:mozilla:xtf:tests:smiley");
 //builder.beginElement("foo");
 // runTest("buildSmileyElement", 10, 100);
//  runTest("buildAndWrapSmileyElement", 10, 100);
//  runTest("loadSmiley25", 5, 1);
//  runTest("loadSmiley50", 5, 1);
//  runTest("loadSmiley100", 3, 1);
//  runTest("loadSmiley25c", 5, 1);
//  runTest("loadSmiley50c", 5, 1);
//  runTest("loadSmiley100c", 3, 1);
}
