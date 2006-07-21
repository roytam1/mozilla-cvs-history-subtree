/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Marcio S. Galli - mgalli@geckonnection.com
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
 
/*
  Flikr documentation
  THIS IS TEMPORARY AND SHOULD BE CHANGED - It uses marcio developers key. 
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

	var sig= hex_md5(str);

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

	var sig = hex_md5(str);

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

	var sig = hex_md5(str);

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

	var sig = hex_md5(str);

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
//   var fileCustomDirFile= refLocalFile.QueryInterface(nsILocalFile);
//   fileCustomDirFile.initWithPath("\\Storage Card\\");
//   fp.displayDirectory = fileCustomDirFile;


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
  var sig = hex_md5(str);


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
  var sig = hex_md5(str);


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

  var sig = hex_md5(str);

  document.getElementById("f_api_sig").value = sig;
  
  document.getElementById("form_send").submit();

}


/* This is called and currently being used from minimo.js, 
 * See the path bookmarks/... 
 */
function bmInitXUL(targetDoc, targetElement) {

    var testLoad=new bmProcessor("browser.bookmark.homebar");
    testLoad.xslSet("bookmarks/bookmark_template_xul.xml");
    testLoad.setTargetDocument(targetDoc);
    testLoad.setTargetElement(targetElement);
    testLoad.run();
    
}

function bmProcessor(prefStore) {



      /* init stuff */ 
  

	this.xmlRef=document.implementation.createDocument("","",null);
	this.xslRef=document.implementation.createDocument("http://www.w3.org/1999/XSL/Transform","stylesheet",null);

	var bookmarkStore=null;

	try {
	      gPref = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
	      bookmarkStore = gPref.getCharPref(prefStore);
     
      } catch (ignore) {}


	var aDOMParser = new DOMParser();
	this.xmlRef = aDOMParser.parseFromString(bookmarkStore,"text/xml");

	if(this.xmlRef&&this.xmlRef.firstChild&&this.xmlRef.firstChild.nodeName=="bm") {

		// All good to go. 
		
	} else {
		var bookmarkEmpty="<bm></bm>";
		gPref.setCharPref(prefStore,bookmarkEmpty);
		this.xmlRef = aDOMParser.parseFromString(bookmarkEmpty,"text/xml");
	}

	this.xslUrl="";

	var myThis=this;
	var omega=function thisScopeFunction2() { myThis.xslLoaded(); }

	this.xslRef.addEventListener("load",omega,false);

	this.xmlLoadedState=true;
	this.xslLoadedState=false;


}

bmProcessor.prototype.xmlLoaded = function () {
	this.xmlLoadedState=true;	
alert('loaded');
	this.apply();
}

bmProcessor.prototype.xslLoaded = function () {
	this.xslLoadedState=true;
	this.apply();
}

bmProcessor.prototype.xmlSet = function (urlstr) {
	this.xmlUrl=urlstr;
}

bmProcessor.prototype.xslSet = function (urlstr) {
	this.xslUrl=urlstr;
}

bmProcessor.prototype.setTargetDocument = function (targetDoc) {
	this.targetDocument=targetDoc;
}

bmProcessor.prototype.setTargetElement = function (targetEle) {
	this.targetElement=targetEle;
}

bmProcessor.prototype.apply = function () {
    if( this.xmlRef.getElementsByTagName("li").length < 1) {
      if( this.targetDocument && this.targetDocument ) {
        if(this.targetDocument.getElementById("message-empty")) {

            this.targetDocument.getElementById("message-empty").style.display="block";
        }
        // ... other checks? other formatting...
      } 
      return; 
    }

    if(this.xmlLoadedState&&this.xslLoadedState) {	
        var xsltProcessor = new XSLTProcessor();
        var htmlFragment=null;
        try {
          xsltProcessor.importStylesheet(this.xslRef);
          htmlFragment = xsltProcessor.transformToFragment(this.xmlRef, this.targetDocument);
        } catch (e) {
        }
        this.targetElement.appendChild(htmlFragment.firstChild);
    }    
}

bmProcessor.prototype.run = function () {
	try {
		// Already parsed.
		// this.xmlRef.load(this.xmlUrl);
	} catch (e) {
	}
	try {
		this.xslRef.load(this.xslUrl);
	} catch (e) {
	}

}

