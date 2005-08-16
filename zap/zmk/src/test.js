Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");

var adev = Components.classes["@mozilla.org/zap/audiodevicemanager;1"].getService(Components.interfaces.zapIAudioDeviceManager);

var dev0 = adev.getDeviceAt(0).QueryInterface(Components.interfaces.zapIPortaudioDevice);
var dev1 = adev.getDeviceAt(1);
var dev2 = adev.getDeviceAt(2);
var dev3 = adev.getDeviceAt(3);

var g = Components.classes["@mozilla.org/zap/mediagraph;1"].createInstance(Components.interfaces.zapIMediaGraph);

var PB = makePropertyBag;

//var a = g.addNode("testsource", null);
var ain= g.addNode("audioin", PB({$device:dev0}));
var aout = g.addNode("audioout", PB({$device:dev2}));
var inp = g.getNode(ain, Components.interfaces.zapIAudioIn, true);
var enc = g.addNode("speex-encoder", PB({$sample_rate:8000}));
var enc_ctl = g.getNode(enc, Components.interfaces.zapISpeexEncoder, true);
var dec = g.addNode("speex-decoder", PB({$sample_rate:32000}));
var buf = g.addNode("buffer", PB({$prefill_size:2, $max_size:10}));
var speex2rtp = g.addNode("speex2rtp-converter", null);
var rtp2speex = g.addNode("rtp2speex-converter", null);
//var udp6060 = g.addNode("udp-socket", PB({$port:6060}));
//var udp6061 = g.addNode("udp-socket", PB({$port:6061}));

/*

   +----+      +----+      +----+      +----+      +----+
   |ain |----->|enc |----->|dec |----->|buf |----->|aout|
   +----+  A   +----+  B   +----+  C   +----+  D   +----+
*/

var A = g.connect(ain, PB({$frame_duration:0.02, $sample_rate:8000, $sample_format:"float32_32768"}), enc, null);
var B = g.connect(enc, null, speex2rtp, null);
var C = g.connect(speex2rtp, null, buf, null);
var D = g.connect(buf, null, rtp2speex, null);
var E = g.connect(rtp2speex, null, dec, null);
var F = g.connect(dec, null, aout, null);


//var A = g.connect(udp6060, null, buf, null);
//var B = g.connect(buf, null, udp6061, null);

// var ain2= g.addNode("audioin", null);
// var aout2 = g.addNode("audioout", null);
// var inp2 = g.getNode(ain2, Components.interfaces.zapIAudioIn, true);
// var buf2 = g.addNode("buffer", makePropertyBag({$prefill_size:70}));
//  var C = g.connect(ain2, null, buf2, null);
//  var D = g.connect(buf2, null, aout2, null);


