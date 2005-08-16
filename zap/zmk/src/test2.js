Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");

var adev = Components.classes["@mozilla.org/zap/audiodevicemanager;1"].getService(Components.interfaces.zapIAudioDeviceManager);

var dev0 = adev.getDeviceAt(0).QueryInterface(Components.interfaces.zapIPortaudioDevice);
var dev1 = adev.getDeviceAt(1);
var dev2 = adev.getDeviceAt(2);
var dev3 = adev.getDeviceAt(3);

var g = Components.classes["@mozilla.org/zap/mediagraph;1"].createInstance(Components.interfaces.zapIMediaGraph);

var PB = makePropertyBag;

var ain= g.addNode("audioin", PB({$device:dev0}));
var aout = g.addNode("audioout", PB({$device:dev2}));
var inp = g.getNode(ain, Components.interfaces.zapIAudioIn, true);
var enc = g.addNode("speex-encoder", PB({$sample_rate:32000}));
var enc_ctl = g.getNode(enc, Components.interfaces.zapISpeexEncoder, true);
var dec = g.addNode("speex-decoder", PB({$sample_rate:32000}));
var buf1 = g.addNode("buffer", PB({$prefill_size:2, $max_size:10}));
var buf2 = g.addNode("buffer", PB({$prefill_size:2, $max_size:10}));
var speex2rtp = g.addNode("speex2rtp-converter", null);
var rtp2speex = g.addNode("rtp2speex-converter", null);
var udp1 = g.addNode("udp-socket-pair", PB({$portbase:6060}));
var udp2 = g.addNode("udp-socket-pair", PB({$portbase:6060}));
var udp2_ctl = g.getNode(udp2, Components.interfaces.zapIUDPSocketPair, true);
var rtpsession = g.addNode("rtp-session", PB({$address:"127.0.0.1", $port:udp2_ctl.portA}));

var A = g.connect(ain, PB({$sample_rate:32000}), enc, null);
var B = g.connect(enc, null, speex2rtp, null);
var C = g.connect(speex2rtp, null, rtpsession, PB({$name:"local-rtp"}));
var D = g.connect(rtpsession, PB({$name:"remote-rtp"}), buf1, null);
var E = g.connect(buf1, null, udp1, PB({$name:"socket-a"}));

var F = g.connect(udp2, PB({$name:"socket-a"}), buf2, null);
var G = g.connect(buf2, null, rtpsession, PB({$name:"remote-rtp"}));
var H = g.connect(rtpsession, PB({$name:"local-rtp"}), rtp2speex, null);
var I = g.connect(rtp2speex, null, dec, null);
var J = g.connect(dec, null, aout, null);
