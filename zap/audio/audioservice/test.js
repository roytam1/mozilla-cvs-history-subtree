var as = Components.classes["@mozilla.org/zap/audioservice;1"].getService(Components.interfaces.zapIAudioService);

var t = as.openAudioTransport(as.defaultInputDevice, 1, 1 ,0.2, as.defaultOutputDevice, 1, 1, 0.2, 44100, 1000, 0);

var op = t.openOutputStream();

var ip = t.openInputStream();
var ips = Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStreamEx);
ips.init(ip);


function record(seconds, sample) {
  t.start();
  sample.bytes = "";
  var count = seconds*44100*4;
  sample.bytes = ips.readEx(count);
  t.stop();
}
  
function playback(sample) {
  t.start();
  op.write(sample.bytes, sample.bytes.length);
  t.stop();
}

function loop(seconds) {
  var frames = seconds*44100*4/1024;
  t.start();
  while (--frames>0) {
    print(frames+"\n");
    var a = ips.readEx(1024);
    op.write(a, 1024);
  }
  t.stop();
}

function distort(a) {
  var old = a.bytes;
  a.bytes = "";
  for (var i=0,l=old.length; i<l; ++i) {
    a.bytes += String.fromCharCode(old.charCodeAt(i)*Math.sin(i/44100*Math.Pi*2));
  }
}
