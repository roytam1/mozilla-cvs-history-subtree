
/*
  Flikr documentation

  --

  Example post image binaries: 
  http://www.flickr.com/services/api/upload.example.html

  Example: 
  ===
  http://flickr.com/services/rest/?
       method=flickr.auth.getToken
       &api_key=1234567890
       &frob=abcxzy
       &api_sig=3f3870be274f6c49b3e31a0c6728957f.

  Example full Token: 
  ===
    http://flickr.com/services/rest/?
       method=flickr.auth.getFullToken
       &api_key=fc739b58ccff44dd18190fbccd68c780
       &mini_token=
       &api_sig=3f3870be274f6c49b3e31a0c6728957f.

  SIGNATURE INFO:
  ====
   
  Example:

  000005fab4534d05api_key9a0554259914a86fb9e7eb014e4e5d52methodflickr.auth.getFullTokenmini_token123-456-789

  
  INFO: 
  ====

  API Key marcio: 

  fc739b58ccff44dd18190fbccd68c780

  Shared Secret: 

  c5a592e2a9a1857f

  marcio temporary token example
  
  //589-851-042


  
 
*/

function bmInit(targetDoc, targetElement) {


}

function auth() {
	
 var win;
 var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
 win = wm.getMostRecentWindow("navigator:browser");
 if(!win) win = window.opener;



//  window.openDialog("http://www.flickr.com/auth-21218","flikr auth","modal,centerscreeen,chrome,resizable=no");

  try {  
    win.gBrowser.selectedTab = win.gBrowser.addTab('http://www.flickr.com/auth-21218');   
    win.browserInit(win.gBrowser.selectedTab);
  } catch (e) {

  }  

	
}



var docX = null;
var api_key= null;
var sharedsecret = null;
var token = null;

function auth2() {

  	docX = document.implementation.createDocument("","",null);

	docX.addEventListener("load", auth2_check, false);

	var mtoken=document.getElementById("minitoken").value;
	sharedsecret="c5a592e2a9a1857f";
      api_key="fc739b58ccff44dd18190fbccd68c780";

	str=sharedsecret+"api_key"+api_key+"methodflickr.auth.getFullTokenmini_token"+mtoken;

	var sig= MD5(str);

	strUrl = "http://flickr.com/services/rest/?method=flickr.auth.getFullToken&api_key="+api_key+"&mini_token="+mtoken+"&api_sig="+sig;

      docX.load(strUrl);

}

function auth2_check() {

 document.getElementById("upload-button").style.backgroundColor="green";

 
}



////
/// Desktop mode Authentication 
//

/*
 * flikr.auth.getFrob 
 * 
 * 
 * 
 * 
 * 
 */

var docY = null;
var flickrDesktopFrob = null;

function auth_desktop() {
  	docY = document.implementation.createDocument("","",null);

	docY.addEventListener("load", auth_desktop_follow, false);

	sharedsecret="c5a592e2a9a1857f";
      api_key="fc739b58ccff44dd18190fbccd68c780";

	str=sharedsecret+"api_key"+api_key+"methodflickr.auth.getFrob";

	var sig = MD5(str);

	strUrl = "http://flickr.com/services/rest/?method=flickr.auth.getFrob&api_key="+api_key+"&api_sig="+sig;

      docY.load(strUrl);

}

////
/// Desktop mode authentication follow up. 
//
function auth_desktop_follow() {

flickrDesktopFrob = docY.getElementsByTagName("frob").item(0).firstChild.nodeValue;

auth_desktop_login();

}

////
/// Desktop mode Login Flow
//
function auth_desktop_login() {
	 var win;
 var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
 win = wm.getMostRecentWindow("navigator:browser");
 if(!win) win = window.opener;



	sharedsecret="c5a592e2a9a1857f";
      api_key="fc739b58ccff44dd18190fbccd68c780";

	str=sharedsecret+"api_key"+api_key+"frob"+flickrDesktopFrob+"perms"+"write";

	var sig = MD5(str);

	try {  
		win.gBrowser.selectedTab = win.gBrowser.addTab('http://www.flickr.com/services/auth/?api_key='+api_key+'&perms=write&frob='+flickrDesktopFrob+'&api_sig='+sig);   
		win.browserInit(win.gBrowser.selectedTab);
	} catch (e) {
	
	}  

}

////
/// Desktop mode get token - after login flow authorization and frob.
//

var docZ = null;

function auth_desktop_gettoken() {

  	docZ = document.implementation.createDocument("","",null);

	docZ.addEventListener("load", auth_desktop_gettoken_follow, false);

	sharedsecret="c5a592e2a9a1857f";
      api_key="fc739b58ccff44dd18190fbccd68c780";

	str=sharedsecret+"api_key"+api_key+"frob"+flickrDesktopFrob+"methodflickr.auth.getToken";

	var sig = MD5(str);

	strUrl = "http://flickr.com/services/rest/?method=flickr.auth.getToken&api_key="+api_key+"&frob="+flickrDesktopFrob+"&api_sig="+sig;
      docZ.load(strUrl);

}

////
/// Desktop mode get token follow up
//

desktopToken = null;

function auth_desktop_gettoken_follow() {

  //serialXML=new XMLSerializer();
  //alert(serialXML.serializeToString(docZ));

  desktopToken = docZ.getElementsByTagName("token").item(0).firstChild.nodeValue;

  alert("token = "+desktopToken);

}


function canvas_window_draw() {

 var win;
 var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
 win = wm.getMostRecentWindow("navigator:browser");
 if(!win) win = window.opener;

 var contentWidth=window._content.document.width;
 var contentHeight=window._content.document.height;


 canvas = document.getElementById("canvaselement");


 ctx = canvas.getContext("2d");
 ctx.clearRect(0, 0, 300,300);

 ctx.save();

 ctx.scale(1,1);

alert(win.gBrowser.contentWindow.content);

 ctx.drawWindow(win.gBrowser.contentWindow.content,0,0,300,300,"rgb(100,100,100)");

}


////
/// Desktop mode send picture
//
function desktop_sendpict() {

	var bytesFile = null;
	var bytesSize = null;
	var obj2 = null;
	var is = null;

	var win;
	var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
				 .getService(Components.interfaces.nsIWindowMediator);
	win = wm.getMostRecentWindow("navigator:browser");
	if(!win) win = window.opener;

	const nsIFilePicker = Components.interfaces.nsIFilePicker;
	const nsIFile = Components.interfaces.nsIFile;
	var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
	var refLocalFile = Components.classes["@mozilla.org/file/local;1"].createInstance(nsIFile );
	fp.init(win, null, nsIFilePicker.modeOpen);

	const nsILocalFile = Components.interfaces.nsILocalFile;


////
/// this is for minimo
// trying to set dir. 
   var fileCustomDirFile= refLocalFile.QueryInterface(nsILocalFile);
   fileCustomDirFile.initWithPath("\\Storage Card\\");
   fp.displayDirectory = fileCustomDirFile;


  fp.appendFilters(nsIFilePicker.filterAll);

  var returnFilePickerValue=fp.show();

  if (returnFilePickerValue == nsIFilePicker.returnOK) {
    var file = fp.file.QueryInterface(nsILocalFile);
	is = Components.classes["@mozilla.org/network/file-input-stream;1"]
		.createInstance( Components.interfaces.nsIFileInputStream );
	is.init( file,0x01, 00004, null);

	var sis = Components.classes["@mozilla.org/scriptableinputstream;1"]
		.createInstance( Components.interfaces.nsIScriptableInputStream );
	sis.init( is );
	
  }

  str=sharedsecret+"api_key"+api_key+"auth_token"+desktopToken;
  var sig = MD5(str);


	var ref="http://www.flickr.com/services/upload/";

	try {

		var boundaryString = '123456789';
		var boundary = '--' + boundaryString;

		req = new XMLHttpRequest();


		var requestBody = [boundary,'Content-Disposition: form-data; name="api_key"',	'',api_key,boundary,'Content-Disposition: form-data; name="auth_token"','',desktopToken,boundary,'Content-Disposition: form-data; name="api_sig"','',sig,boundary,'Content-Disposition: form-data; name="photo"; filename="minimo.png"','Content-Type: image/png','',''].join('\r\n');



		var requestBody2 = ['',boundary].join('\r\n');



		req.open('POST', ref, true); 

		req.onreadystatechange = function (evt) {
			if (req.readyState == 4) {
				alert("hello world:"+req.responseText);
			}
		}

		req.setRequestHeader('Content-Type','multipart/form-data; boundary='+boundaryString);


	var objmulti = Components.classes["@mozilla.org/io/multiplex-input-stream;1"].
            createInstance(Components.interfaces.nsIMultiplexInputStream);


 
	var obj1 = Components.classes["@mozilla.org/io/string-input-stream;1"].
	            createInstance(Components.interfaces.nsIStringInputStream);

	obj1.setData(requestBody, requestBody.length);
	
		
	var obj3 = Components.classes["@mozilla.org/io/string-input-stream;1"].
	            createInstance(Components.interfaces.nsIStringInputStream);

	obj3.setData('\r\n--123456789--\r\n', 13);
		
	objmulti.appendStream(obj1);



var image = Components.classes["@mozilla.org/network/buffered-input-stream;1"].
            createInstance(Components.interfaces.nsIBufferedInputStream);

image.init(is,is.available());



	objmulti.appendStream(image);

	objmulti.appendStream(obj3);

	var sis = Components.classes["@mozilla.org/scriptableinputstream;1"]
		.createInstance( Components.interfaces.nsIScriptableInputStream );

	sis.init( objmulti );


	req.send(objmulti);


	} catch (e) {
		alert(e);
	}

}





////////
///////
//////
/////  Other mobile approach
////
///
//





function fulltokenLoaded() {

 var bytesFile = null;
 var bytesSize = null;
 var obj2 = null;
 var is = null;

 var win;
 var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
 win = wm.getMostRecentWindow("navigator:browser");
 if(!win) win = window.opener;

  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  const nsIFile = Components.interfaces.nsIFile;
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  var refLocalFile = Components.classes["@mozilla.org/file/local;1"].createInstance(nsIFile );
  fp.init(win, null, nsIFilePicker.modeOpen);

  const nsILocalFile = Components.interfaces.nsILocalFile;


////
/// this is for minimo
// trying to set dir. 

   var fileCustomDirFile= refLocalFile.QueryInterface(nsILocalFile);
   fileCustomDirFile.initWithPath("\\Storage Card\\");
   fp.displayDirectory = fileCustomDirFile;


  fp.appendFilters(nsIFilePicker.filterAll);

  var returnFilePickerValue=fp.show();

  if (returnFilePickerValue == nsIFilePicker.returnOK) {
    var file = fp.file.QueryInterface(nsILocalFile);
	is = Components.classes["@mozilla.org/network/file-input-stream;1"]
		.createInstance( Components.interfaces.nsIFileInputStream );
	is.init( file,0x01, 00004, null);
	
  }


  token = docX.getElementsByTagName("token").item(0).firstChild.nodeValue;
  str=sharedsecret+"api_key"+api_key+"auth_token"+token;
  var sig = MD5(str);


	var ref="http://www.flickr.com/services/upload/";

	try {

		var boundaryString = '123456789';
		var boundary = '--' + boundaryString;

		req = new XMLHttpRequest();


		var requestBody = [boundary,'Content-Disposition: form-data; name="api_key"',	'',api_key,boundary,'Content-Disposition: form-data; name="auth_token"','',token,boundary,'Content-Disposition: form-data; name="api_sig"','',sig,boundary,'Content-Disposition: form-data; name="photo"; filename="test.jpg"','Content-Type: image/jpg','',''].join('\r\n');
		var requestBody2 = ['',boundary].join('\r\n');


		req.open('POST', ref, true); 

		req.onreadystatechange = function (evt) {
			if (req.readyState == 4) {
				alert("hello world:"+req.responseText);
			}
		}

		req.setRequestHeader('Content-Type','multipart/form-data; boundary='+boundaryString);


	var objmulti = Components.classes["@mozilla.org/io/multiplex-input-stream;1"].
            createInstance(Components.interfaces.nsIMultiplexInputStream);


 
	var obj1 = Components.classes["@mozilla.org/io/string-input-stream;1"].
	            createInstance(Components.interfaces.nsIStringInputStream);

	obj1.setData(requestBody, requestBody.length);
	

		
	var obj3 = Components.classes["@mozilla.org/io/string-input-stream;1"].
	            createInstance(Components.interfaces.nsIStringInputStream);

	obj3.setData('\r\n--123456789--\r\n', 13);
		
	objmulti.appendStream(obj1);



var image = Components.classes["@mozilla.org/network/buffered-input-stream;1"].
            createInstance(Components.interfaces.nsIBufferedInputStream);

image.init(is,is.available());


	objmulti.appendStream(image);

	objmulti.appendStream(obj3);

	req.send(objmulti);


	} catch (e) {
		alert(e);
	}


}


////
/// Deprecated.
//
function oldLoaded() {
  var s = new XMLSerializer();
  var str = s.serializeToString(docX);


  token = docX.getElementsByTagName("token").item(0).firstChild.nodeValue;

  document.getElementById("f_api_key").value=api_key;
  document.getElementById("f_auth_token").value=token;

  str=sharedsecret+"api_key"+api_key+"auth_token"+token;

  var sig = MD5(str);

  document.getElementById("f_api_sig").value = sig;
  
  document.getElementById("form_send").submit();

}

