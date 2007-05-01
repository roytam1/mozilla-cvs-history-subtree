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
 * URL Find Type in implementation
 */

var gWin = null;
var gURLBar = null;


function URLBarEventCatch2(event) {

   if(event.keyCode==KeyEvent.DOM_VK_DOWN) {
	
	document.commandDispatcher.advanceFocus();

   }   

}

function URLBarEventCatch(event) {

   var escapeToHistory = false; 

   if(event.keyCode==KeyEvent.DOM_VK_RETURN) {
	URLBarEntered();
   } 

   if(event.keyCode==KeyEvent.DOM_VK_DOWN) {
	escapeToHistory = true;
   } 

   var currentURLBarString = document.getElementById("urlbar2").value;

   var regExpFineString = currentURLBarString.replace(/\./g,"\.");

   var historyItemsList = document.getElementsByTagName("history");

   var regExp = new RegExp(currentURLBarString );

   var historyFound = false; 

   for(var i=0;i<historyItemsList.length;i++) {

	var currentElement = historyItemsList[i];

	var textValue = currentElement.getAttribute("value");

	var plainValue = unescape(textValue);

	var brother = currentElement.previousSibling;

	if(textValue.match(regExp)) {
        brother.style.display="block";
        historyFound=true;		
        if(escapeToHistory) {
          brother.childNodes[1].focus(); escapeToHistory = false;
        }
	} else {
          brother.style.display="none";
	} 
	
   } 
  
   if(historyFound) {
		hbSelect("timehistory");
   } else {
        hbSelectAll();
   }

}

/* 
 * This is called when the Hit Enter command goes on with the 
 * URLBAR field 
 */

function URLBarEntered()
{

  gURLBar = document.getElementById("urlbar2");

  try
  {
    if (!gURLBar)
      return;
    
    var url = gURLBar.value;
    if (gURLBar.value == "" || gURLBar.value == null)
      return;
    
    /* Trap to SB 'protocol' */ 
    
    if(gURLBar.value.substring(0,3)=="sb:") {
      gWin.DoBrowserSB(gURLBar.value.split("sb:")[1]);
      return;
    }

    /* Trap to chrome targets 'target' */ 
    
    if(gURLBar.value.substring(0,3)=="go:") {
      gWin.DoBrowserTarget(gURLBar.value.split("go:")[1]);
      return;
    }
    
    if(gURLBar.value.substring(0,4)=="rss:") {
      gWin.DoBrowserRSS(gURLBar.value.split("rss:")[1]);
      return;
    }
    
    if(gURLBar.value.substring(0,3)=="gm:") {
      gWin.DoBrowserGM(gURLBar.value.split("gm:")[1]);
      return;
    }
    
    /* Other normal cases */ 
    
    if (gURLBar.value.indexOf(" ") == -1)
    {
      gURLBar.value = gWin.BrowserFixUpURI(url);
      
      // Notify anyone interested that we are loading.

      try {
        var os = Components.classes["@mozilla.org/observer-service;1"]
          .getService(Components.interfaces.nsIObserverService);
        var host = gURLBar.value;
        os.notifyObservers(null, "loading-domain", host);
      }  catch(e) {gWin.onErrorHandler(e);}
    }

    gWin.BrowserOpenURLasTab(gURLBar.value);

    return true;

  }
  catch(ex) {onErrorHandler(ex);}
 
}

function SearchGoogle(vQuery) {
  try { 
    if(vQuery!="") {
      gWin.gBrowser.selectedTab = gWin.gBrowser.addTab('http://www.google.com/xhtml?q='+vQuery+'&hl=en&lr=&safe=off&btnG=Search&site=search&mrestrict=xhtml');
      gWin.browserInit(gBrowser.selectedTab);
    }
  } catch (e) {
    
  }  
}

/* 
 * This is called when the XSLT transformed document is alive. Search for blLoaded
 */

function bmLoaded() {


	document.getElementById("urlbar2").addEventListener("keydown",URLBarEventCatch,true);

	//document.addEventListener("keypress",URLBarEventCatch2,true);

	document.getElementById("urlbar2").focus();


}

/*
 * Homebase Application Implementation 
 */

var hbArrayClasses = new Array();
var hbArrayClassesFeedback= new Array();

hbArrayClasses["rsslink"]=0;
hbArrayClasses["pagelink"]=1;
hbArrayClasses["extensions"]=2;
hbArrayClasses["timehistory"]=3;

hbArrayClassesFeedback["rsslink"]=4;
hbArrayClassesFeedback["pagelink"]=5;
hbArrayClassesFeedback["extensions"]=6;
hbArrayClassesFeedback["timehistory"]=7;
hbArrayClassesFeedback["allFeedback"]=8;

function hbSelect(refShow) {
	for (var key in hbArrayClasses) {
	    document.styleSheets[0].cssRules[hbArrayClasses[key]].style.display="none";
	}
	document.styleSheets[0].cssRules[hbArrayClasses[refShow]].style.display="block";
    //hbFeedback(refShow);
}

function hbSelectAll() {
	for (var key in hbArrayClasses) {
	    document.styleSheets[0].cssRules[hbArrayClasses[key]].style.display="block";
	}
    //hbFeedback("allFeedback");
}

function hbFeedback(refShow) {

	for (var key in hbArrayClassesFeedback) {
	    document.styleSheets[0].cssRules[hbArrayClassesFeedback[key]].style.backgroundColor="transparent";
	    document.styleSheets[0].cssRules[hbArrayClassesFeedback[key]].style.color="white";
	}
	document.styleSheets[0].cssRules[hbArrayClassesFeedback[refShow]].style.color="black";
	document.styleSheets[0].cssRules[hbArrayClassesFeedback[refShow]].style.backgroundColor="white";

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
	
	gWin = win;

	gHomebaseElements = homebase_menuBuild(win);

  }

  // This now is not using the solution to tip the main minimo window on its chrome background 
  // color, because this is a panel so far with no background color. It's just white. We have to work this 
  // better. Maybe these panels could simply inherit some sort of live style. 
  // gWin.BrowserTellChromeThemeRules("homebase",document.styleSheets[0].cssRules[0].style);

  /* 
   * okay now the homebase / bookmarks starts...
   */

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
              //var prepareValue = escape(rr.getValueAt(ii));
		      var originalString = rr.getValueAt(ii);
		      var prepareValue = originalString.replace(/&/g,"&amp;");
	          this.bookmarkStore+="<li hbhistory='true' title='"+prepareValue+"'  value='"+prepareValue+"'>"+prepareValue+"</li>";
	        }
            this.bookmarkStore+="</bm>";
	      },
          bookmarkStore:""
	  }; 
	

	/* marcio - todo, fix the search string so that we can get everything */
    // Right now what happens is, if we put "" minimo shows nothing as the result instead everything. 
    // With FF 1.8 Desktop works fine "". In minimo the workaorund now is www. domains. 
    
	mySearch.startSearch("www.","",null, myObserver );
	
	} catch (ignore) {

	}

    var multiMarks = "<bmgroup>"+gHomebaseElements+bookmarkStore+myObserver.bookmarkStore+"</bmgroup>";

	var testLoad=new bmProcessor(multiMarks);
	testLoad.xslSet("bookmark_template_multiple.xml");
	testLoad.setTargetDocument(targetDoc);
	testLoad.setTargetElement(targetElement);
	testLoad.run();

  /*
   * preselection of the style rule. This has to do with the solution so that 
   * is possible to open a panel with sub selection of a given Target rule. 
   * For example, if you want to open the homebase/bookmarks and get some 
   * specific sub view in it. With the previous homebase it was a good thing. 
   * Now the homebase is more like a linear list, so we should consider to remove 
   * this feature or make it much better. For example a possible interesting rule 
   * could be #filter=/RegExpValue/
   */

   var targetLink = document.location.toString();

   var keywordLink = targetLink.split("#")[1];
   try {
	   if(keywordLink) {
		hbSelect(keywordLink);
	   } else {
		hbSelectAll();
	   }
	
   } catch(i) {
		hbSelectAll();
   } 

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

  this.xmlRef=document.implementation.createDocument("","",null);
  this.xslRef=document.implementation.createDocument("http://www.w3.org/1999/XSL/Transform","stylesheet",null);
  this.xmlRef=null;

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

  this.xslUrl="";

  var myThis=this;
  var omega=function thisScopeFunction2() { myThis.xslLoaded(); }

  this.xslRef.addEventListener("load",omega,false);

  this.xmlLoadedState=true;
  this.xslLoadedState=false;
}

bmProcessor.prototype.xmlLoaded = function () {
	this.xmlLoadedState=true;	
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
	
	  bmLoaded();

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

