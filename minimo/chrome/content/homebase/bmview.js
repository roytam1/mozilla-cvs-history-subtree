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
 * Mozilla Foundation 
 * Portions created by the Initial Developer are Copyright (C) 2006
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
 * Homebase Application Implementation 
 */


var hbArrayClasses = new Array();

hbArrayClasses["rsslink"]=0;
hbArrayClasses["pagelink"]=1;
hbArrayClasses["extensions"]=2;
hbArrayClasses["timehistory"]=3;

function hbSelect(refShow) {

	for (var key in hbArrayClasses) {
	    document.styleSheets[0].cssRules[hbArrayClasses[key]].style.display="none";
	}

	document.styleSheets[0].cssRules[hbArrayClasses[refShow]].style.display="block";

}

function hbSelectAll() {

	for (var key in hbArrayClasses) {
	    document.styleSheets[0].cssRules[hbArrayClasses[key]].style.display="block";
	}

}

function hbOpenAsTab(ref) {

   var win;
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  win = wm.getMostRecentWindow("navigator:browser");
  if(!win) win = window.opener; 

  if (win) {

	 try {  
    win.gBrowser.selectedTab = win.gBrowser.addTab(ref);   
    win.browserInit(gBrowser.selectedTab);
  } catch (e) {
  }  

  }

}

function bmInit(targetDoc, targetElement) {

  var gHomebaseElements = null; 
  var bookmarkStore=null;



  var win;
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  win = wm.getMostRecentWindow("navigator:browser");
  if(!win) win = window.opener; 
  if (win) {

	gHomebaseElements = homebase_menuBuild(win);

  }


  try {
        bookmarkStore=null;
        gPref = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
        bookmarkStore = gPref.getCharPref("browser.bookmark.store");

  } catch (ignore) {}



	var myObserver = null;

	try { 
	
	  var mySearch = Components.classes["@mozilla.org/autocomplete/search;1?name=history"].
	                            getService(Components.interfaces.nsIAutoCompleteSearch);
	
	  myObserver = {
	      onSearchResult:function (q,w) { 
	        rr=w.QueryInterface(Components.interfaces.nsIAutoCompleteResult);
		  this.bookmarkStore="<bm>";
	        for(var ii=0;ii<rr.matchCount;ii++) {
	          this.bookmarkStore+="<li hbhistory='true' title='"+rr.getValueAt(ii)+"'>"+rr.getValueAt(ii)+"</li>";
	        }
		  this.bookmarkStore+="</bm>";
	      },
            bookmarkStore:""
	  }; 
	

	/* marcio - todo, fix the search string so that we can get everything */

	mySearch.startSearch("www.","",null, myObserver );
	
	} catch (ignore) {

	}

      var multiMarks = "<bmgroup>"+gHomebaseElements+bookmarkStore+myObserver.bookmarkStore+"</bmgroup>";


	var testLoad=new bmProcessor(multiMarks);
	testLoad.setTargetDocument(targetDoc);
	testLoad.setTargetElement(targetElement);
	testLoad.run();

}

function homebase_menuBuild(winRef) {

  gHomebaseElements ="<bm>";

  try { 
  var homebaseItems = winRef.document.getElementById("homebar").childNodes;
  var hasItems = ( homebaseItems.length > 0 );
  gHomebaseElements ="<bm>";
  for (var i = 0; i < homebaseItems.length; i++) {
    var refElement = homebaseItems[i];
    var rawUrl = refElement.getAttribute("oncommand").split("BrowserOpenURLasTab('")[1];
    rawUrl = rawUrl.split("')")[0];
    var eLi = "<li  title='"+refElement.getAttribute("description")+"'  iconsrc='"+refElement.getAttribute("image")+"' action='tab' >"+rawUrl+"</li>";
    gHomebaseElements +=eLi;
  } 

  } catch (i) { } 

  gHomebaseElements +="</bm>";

  return gHomebaseElements;

}

function bmProcessor(bookmarkStore) {

  this.xmlRef=null;
  this.xslRef=null;
  
  var aDOMParser = new DOMParser();

  try {

    this.xmlRef = aDOMParser.parseFromString(bookmarkStore,"text/xml");

  } catch (ignore) {}

  if(this.xmlRef&&this.xmlRef.firstChild&&this.xmlRef.firstChild.nodeName=="bmgroup") {
   

  } else {
    var bookmarkEmpty="<bm></bm>";
    gPref.setCharPref(prefStore,bookmarkEmpty);
    this.xmlRef = aDOMParser.parseFromString(bookmarkEmpty,"text/xml");
  }


  var aDOMParser = new DOMParser();
  var xsltTemplate = "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"                xmlns:dc=\"http://purl.org/dc/elements/1.1/\"                xmlns:rss=\"http://purl.org/rss/1.0/\" xmlns:html=\"http://www.w3.org/1999/xhtml\"                xmlns:xul=\"http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul\"><xsl:output method=\"html\" indent=\"yes\"/><xsl:template match=\"/\"><div id=\"container2\" >  <div style=\"text-align:center\">    <img src=\"chrome://minimo/skin/extensions/icon-urlbar.png\">    </img>    <input type=\"text\" id=\"urlbar2\" style=\"text-align:left;background-color:white;width:90%\" />  </div>	  <div style=\"text-align:center\" class=\"extensions\">      <img src=\"chrome://minimo/skin/extensions/google.png\">    </img>    <input type=\"text\" id=\"search-google\"                 style=\"text-align:left;background-color:white;width:90%\"                 onchange=\"return SearchGoogle(this.value);\"  />      </div>	  <xsl:for-each select=\"/bmgroup/bm/li\"><xsl:choose><xsl:when test=\"@action\"><!--  <div class=\"section\">&titleExtensions.label;</div>-->  <div class=\"item extensions\" >    <img>      <xsl:attribute name=\"src\" >        <xsl:value-of select=\"@iconsrc\"/>      </xsl:attribute> 	    </img>    <a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbOpenAsTab('<xsl:value-of select=\".\"/>');return false</xsl:attribute>      <xsl:value-of select=\"@title\"/>    </a>  </div>	</xsl:when></xsl:choose><xsl:choose><xsl:when test=\"@page\" >  <div class=\"item pagelink\" >    <img>      <xsl:attribute name=\"src\" >        <xsl:value-of select=\"@iconsrc\"/>      </xsl:attribute> 	    </img>    <a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbOpenAsTab('<xsl:value-of select=\".\"/>');return false</xsl:attribute>      <xsl:value-of select=\"@title\"/>    </a>  </div>  </xsl:when></xsl:choose><xsl:choose><xsl:when test=\"@rss\" >  <div class=\"item rsslink\" >    <img>      <xsl:attribute name=\"src\" >        <xsl:value-of select=\"@iconsrc\"/>      </xsl:attribute> 	    </img>    <a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbOpenAsTab('<xsl:value-of select=\".\"/>');return false</xsl:attribute>      <xsl:value-of select=\"@title\"/>    </a>  </div></xsl:when></xsl:choose></xsl:for-each><!--<h3>&titleHistory.label;</h3>--> <xsl:for-each select=\"/bmgroup/bm/li\"><xsl:choose><xsl:when test=\"@hbhistory\" >  <div  class=\"item timehistory\" ><a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbOpenAsTab('<xsl:value-of select=\"@value\"/>');return false</xsl:attribute><xsl:attribute name=\"class\"><xsl:value-of select=\"@classdomainvalue\"/></xsl:attribute>      <xsl:value-of select=\".\"/>    </a>  </div><history><xsl:attribute name=\"value\"><xsl:value-of select=\"@value\"/></xsl:attribute></history>  </xsl:when></xsl:choose><xsl:choose><xsl:when test=\"@hbhistoryhandler\" ><a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbToggleClass('<xsl:value-of select=\"@classdomainvalue\"/>');return false</xsl:attribute>      [+]    </a>  <div  class=\"item timehistory\" ><a>      <xsl:attribute name=\"href\">javascript:</xsl:attribute> 	      <xsl:attribute name=\"onclick\">hbOpenAsTab('<xsl:value-of select=\"@value\"/>');return false</xsl:attribute>      <xsl:value-of select=\".\"/>    </a>  </div><history><xsl:attribute name=\"value\"><xsl:value-of select=\"@value\"/></xsl:attribute></history>  </xsl:when></xsl:choose>  </xsl:for-each>  </div></xsl:template></xsl:stylesheet>";

  this.xslRef = aDOMParser.parseFromString(xsltTemplate,"text/xml");

  this.xmlLoadedState=true;
  this.xslLoadedState=true;
  
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

	this.apply();

}

