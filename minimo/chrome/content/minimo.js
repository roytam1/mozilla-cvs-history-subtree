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
 *   Doug Turner - dougt@meer.net
 *   Marcio Galli - mgalli@geckonnection.com
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


const imgICache                = Components.interfaces.imgICache;
const nsIBrowserDOMWindow      = Components.interfaces.nsIBrowserDOMWindow;
const nsIBrowserHistory        = Components.interfaces.nsIBrowserHistory;
const nsIClipboard             = Components.interfaces.nsIClipboard;
const nsIDeviceSupport         = Components.interfaces.nsIDeviceSupport;
const nsIDOMChromeWindow       = Components.interfaces.nsIDOMChromeWindow;
const nsIDOMDocument           = Components.interfaces.nsIDOMDocument;
const nsIDOMWindow             = Components.interfaces.nsIDOMWindow;
const nsIDocShellHistory       = Components.interfaces.nsIDocShellHistory;
const nsIDocShellTreeItem      = Components.interfaces.nsIDocShellTreeItem;
const nsIDocShellTreeNode      = Components.interfaces.nsIDocShellTreeNode;
const nsIInterfaceRequestor    = Components.interfaces.nsIInterfaceRequestor;
const nsIPhoneSupport          = Components.interfaces.nsIPhoneSupport;
const nsIPrefBranch            = Components.interfaces.nsIPrefBranch;
const nsIPrefService           = Components.interfaces.nsIPrefService;
const nsISHistory              = Components.interfaces.nsISHistory;
const nsISupportsString        = Components.interfaces.nsISupportsString;
const nsISupportsWeakReference = Components.interfaces.nsISupportsWeakReference;
const nsITransferable          = Components.interfaces.nsITransferable;
const nsIURIFixup              = Components.interfaces.nsIURIFixup;
const nsIWebNavigation         = Components.interfaces.nsIWebNavigation;
const nsIXULBrowserWindow      = Components.interfaces.nsIXULBrowserWindow;
const nsIWebProgress           = Components.interfaces.nsIWebProgress;
const nsIWebProgressListener   = Components.interfaces.nsIWebProgressListener;
const nsIWebProgressListener2  = Components.interfaces.nsIWebProgressListener2;
const nsIXULWindow             = Components.interfaces.nsIXULWindow;
const nsITransfer              = Components.interfaces.nsITransfer;
const nsIConsoleService        = Components.interfaces.nsIConsoleService;
const nsIComponentRegistrar    = Components.interfaces.nsIComponentRegistrar;
const nsISoftKeyBoard          = Components.interfaces.nsISoftKeyBoard;
const nsIObserverService       = Components.interfaces.nsIObserverService;
const nsIPrefBranch2           = Components.interfaces.nsIPrefBranch2;
const nsIFile                  = Components.interfaces.nsIFile;
const nsILocalFile             = Components.interfaces.nsILocalFile;
const nsIFilePicker            = Components.interfaces.nsIFilePicker;

const NS_BINDING_ABORTED = 0x804b0002;

/* 
 * Software keyboard 
 */
var gkeyBoardService = null;
var gKeyBoardToggle = true;
var gBackGroundColor = "#dddddd";

var gTextSize = null;
var gPanMode = null;
var appCore = null;
var gBrowser = null;
var gBookmarksDoc=null; 
var gURLBar = null;
var gBrowserStatusHandler;
var gSelectedTab=null;
var gRSSTag="minimo";
var gGlobalHistory = null;
var gURIFixup = null;
var gShowingMenuCurrent=null;
var gPopupNodeContextMenu=null;
var gDeckMode=0; // 0 = site, 1 = sb, 2= rss. Used for the URLBAR selector, DeckMode impl.
var gDeckMenuChecked=null; // to keep the state of the checked URLBAR selector mode. 
var gURLBarBoxObject = null; // stores the urlbar boxObject so the background loader can update itself based on actual urlbar size width;

var gPref = null;            // direct access to pref.
var gMinimoBundle = null;    // Strings and such.


/*
 * Keyboard Spin Menu Control 
 */
var gKeySpinCurrent     = null;
var gKeySpinMode        = 0;
var gKeySpinLastFocused = null;
var gSpinLast           = null;
var gSpinUrl            = null;
var gSpinFirst          = null;
var gSpinDocument       = null;
var gSpinTemp           = null;


function onErrorHandler(x)
{
  try {
    var consoleService = Components.classes["@mozilla.org/consoleservice;1"].getService(nsIConsoleService);
    consoleService.logStringMessage(x);
  }
  catch(ex) { alert("onErrorHandler: " + x); }
}

function nsBrowserStatusHandler() {}
nsBrowserStatusHandler.prototype = 
{
  QueryInterface : function(aIID)
  {
    if (aIID.equals(nsIWebProgressListener) ||
        aIID.equals(nsIXULBrowserWindow) ||
        aIID.equals(nsISupportsWeakReference) ||
        aIID.equals(nsISupports))
    {
      return this;
    }
    throw Components.results.NS_NOINTERFACE;
  },
  
  init : function()
  {
    this.urlBar= document.getElementById("urlbar");
    this.progressBGPosition = 0;  /* To be removed, fix in onProgressChange ... */ 

    this.currentTotalProgress = 0;
    this.maxTotalProgress     = 0;

    var securityUI = gBrowser.securityUI;
    this.onSecurityChange(null, null, nsIWebProgressListener.STATE_IS_INSECURE);
  },
  
  destroy : function()
  {
    this.urlBar = null;
    this.progressBGPosition = null;  /* To be removed, fix in onProgressChange ... */ 
  },
  
  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    var refBrowser=null;
    var tabItem=null;
    
    if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK)
    {
      
      if (aStateFlags & nsIWebProgressListener.STATE_START)
      {
        // disable and hides the nav-menu-button; and enables unhide the stop button

        document.getElementById("nav-stopreload").className="stop-button";
        document.getElementById("nav-stopreload").setAttribute("command","cmd_BrowserStop");

        document.getElementById("statusbar").hidden=false;


        //        document.getElementById("menu_NavPopup").



		if(aRequest && aWebProgress.DOMWindow == content) {
          this.startDocumentLoad(aRequest);
		}
        return;
      }
      
      if (aStateFlags & nsIWebProgressListener.STATE_STOP)
      {
        document.getElementById("statusbar").hidden=true;
        document.getElementById("statusbar-text").label="";

        // The background progress bar is currently disabled. 
        //  document.styleSheets[1].cssRules[0].style.backgroundPosition=percentage+"px 100% ! important";
        
        if (aRequest) {
            if (aWebProgress.DOMWindow == content) this.endDocumentLoad(aRequest, aStatus);
        }

        // disable and hides the nav-stop-button; and enables unhides the nav-menu-button button
        document.getElementById("nav-stopreload").className="reload-button";
        document.getElementById("nav-stopreload").setAttribute("command","cmd_BrowserReload");

        return;
      }
      return;
    }
    
    if (aStateFlags & nsIWebProgressListener.STATE_IS_DOCUMENT)
    { 
      if (aStateFlags & nsIWebProgressListener.STATE_START)
      {
        return;
      }
      
      if (aStateFlags & nsIWebProgressListener.STATE_STOP)
      {
        // 
        //        try {
        //          var imageCache = Components.classes["@mozilla.org/image/cache;1"]
        //                                   .getService(imgICache);
        //          imageCache.clearCache(false);
        //        }
        //        catch(e) {}
        
        
        return;
      }
      return;
    }
  },
  onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {

    //    document.getElementById("statusbar-text").label= "dbg:onProgressChange " + aCurTotalProgress + " " + aMaxTotalProgress;

    this.currentTotalProgress = aCurTotalProgress;
    this.maxTotalProgress     = aMaxTotalProgress;

    var percentage = parseInt((aCurTotalProgress/aMaxTotalProgress)*parseInt(gURLBarBoxObject.width));
    if(percentage<0) percentage=10;

   // The background progress bar is currently disabled. 
   // document.styleSheets[1].cssRules[0].style.backgroundPosition=percentage+" 0! important";

  },
  onLocationChange : function(aWebProgress, aRequest, aLocation)
  {
    /* Ideally we dont want to check this here.
       Better to have some other protocol view-rss in the chrome */
    
    const rssmask = "chrome://minimo/content/rssview/rssload.xhtml?url=";
    const sbmask = "chrome://minimo/content/rssview/rssload.xhtml?url=http://del.icio.us/rss/tag/";
    
    if(aLocation.spec.substr(0, rssmask .length) == rssmask ) {
      
      if(aLocation.spec.substr(0, sbmask .length) == sbmask ) {
        /* We trap the URL */ 
        this.urlBar.value="sb:"+gRSSTag; 
        
      } else {
        
        /* We trap the URL */ 
        this.urlBar.value="rss:"+gRSSTag; 
        
      }
      
    } else {
      domWindow = aWebProgress.DOMWindow;
      // Update urlbar only if there was a load on the root docshell
      if (domWindow == domWindow.top) {
        this.urlBar.value = aLocation.spec;
      }
    }
    
    BrowserUpdateBackForwardState();
    
    BrowserUpdateFeeds();

    BrowserPanRefresh();

  },
  
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {
    document.getElementById("statusbar-text").label= aMessage +
                                                     " (" +
                                                     this.currentTotalProgress +
                                                     "/" +
                                                     this.maxTotalProgress +
                                                     ")";
  },
  startDocumentLoad : function(aRequest)
  {
    gBrowser.mCurrentBrowser.feeds = null;
  },
  endDocumentLoad : function(aRequest, aStatus)
  {
  },
  onSecurityChange : function(aWebProgress, aRequest, aState)
  {
    /* Color is temporary. We shall dynamically assign a new class to the element and or to 
       evaluate access from another class rule, the security identity color has to be with the minimo.css */ 
    
    switch (aState) {
    case nsIWebProgressListener.STATE_IS_SECURE | nsIWebProgressListener.STATE_SECURE_HIGH:
  
    document.getElementById("nav-lock").className="security-notbroken";
    break;	
    case nsIWebProgressListener.STATE_IS_SECURE | nsIWebProgressListener.STATE_SECURE_LOW:
    // this.urlBar.value="level low";
    document.getElementById("nav-lock").className="security-notbroken";
    break;
    case nsIWebProgressListener.STATE_IS_BROKEN:
    //this.urlBar.value="level broken";
    document.getElementById("nav-lock").className="security-broken";
    break;
    case nsIWebProgressListener.STATE_IS_INSECURE:
    default:
    document.getElementById("nav-lock").className="security-na";
    break;
    }   
  },
  
  setJSStatus : function(status)
  {
  },
  
  setJSDefaultStatus : function(status)
  {
  },
  
  setDefaultStatus : function(status)
  {
  },
  
  setOverLink : function(link, b)
  {
  }
}
  
/** 
 * Initial Minimo Startup 
 * 
 **/
  
/* moved this as global */ 
  
function MiniNavStartup()
{

  // Chris suggestion - defaults to the homebase in startup time. 
  var homepage = "chrome://minimo/content/bookmarks/bmview.xhtml";
  var homepages = null; 
    
  try {
    
    gBrowser = document.getElementById("content");
    
    gURLBar = document.getElementById("urlbar");
    gURLBar.setAttribute("completedefaultindex", "true");
    
    var currentTab=gBrowser.selectedTab;
    browserInit(currentTab);
    gSelectedTab=currentTab;
    
    gBrowserStatusHandler = new nsBrowserStatusHandler();
    gBrowserStatusHandler.init();
    
    window.XULBrowserWindow = gBrowserStatusHandler;
    window.QueryInterface(nsIInterfaceRequestor)
      .getInterface(nsIWebNavigation)
      .QueryInterface(nsIDocShellTreeItem).treeOwner
      .QueryInterface(nsIInterfaceRequestor)
      .getInterface(nsIXULWindow)
      .XULBrowserWindow = window.XULBrowserWindow;
    
    gBrowser.addProgressListener(gBrowserStatusHandler, nsIWebProgress.NOTIFY_ALL);
    
    window.QueryInterface(nsIDOMChromeWindow).browserDOMWindow =
      new nsBrowserAccess();

    gBrowser.webNavigation.sessionHistory = 
      Components.classes["@mozilla.org/browser/shistory;1"].createInstance(nsISHistory);
    
    gBrowser.docShell.QueryInterface(nsIDocShellHistory).useGlobalHistory = true;
    
    gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
      .getService(nsIBrowserHistory);
    
    gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
      .getService(nsIURIFixup);
    
    var bookmarkstore=null; 
    
    try {
      gPref = Components.classes["@mozilla.org/preferences-service;1"]
        .getService(nsIPrefBranch);
      var page = null;
      try {
        page = gPref.getCharPref("browser.startup.homepage.override");
      } 
      catch(e) {page=null;}

      try {
        gBackGroundColor = gPref.getCharPref("ui.chromebackgroundcolor");
      } 
      catch(e) {gBackGroundColor="#eeeeee";}

      if (page == null)
        page = gPref.getCharPref("browser.startup.homepage");

      gPref.clearUserPref("browser.startup.homepage.override");

      if ( page.split("|").length > 1 ) {
           homepages = page.split("|");
      } else {
        if (page != null) {
          homepage = BrowserFixUpURI(page);
        }
      }
    } catch (ignore) { }

  } catch (e) {
    onErrorHandler("Error trying to startup browser.  Please report this as a bug:\n" + e);
  }

  /* 
   * 
   */

  var reg = Components.manager.QueryInterface(nsIComponentRegistrar);
  reg.registerFactory(Components.ID("{fe4d6bd5-e4cd-45f9-95bd-1e1796d2c7f7}"),
                       "Minimo Transfer Item",
                       "@mozilla.org/transfer;1",
                       new TransferItemFactory());

  if(homepages) {
    /* 
     * We need to review this
     * Now we have the concept of the Homebase as a possible location. We need to 
     * check the store of Homepage custom vs Homepage multiple vs Homebase
     */

    gBrowser.loadTabs(homepages,true,true); // force load in background.

  } else {

    /* Because this is the very first time we load a Content, we need to register 
       the URI through our singleton BrowserChrome services. This happens because 
       we want chrome:// uris to be registered and associated with tabs to avoid 
       multiple instantes of a Chrome tab */

	loadURI(homepage);

    if(homepage.indexOf("chrome")==0) {
      /* We only do this Tab Memorize effect for chrome urls */
      BrowserChromeRegisterTab(homepage,gBrowser.selectedTab);
    }
  }

  try { 
     bookmarkstore = gPref.getCharPref("browser.bookmark.store");
  } catch (e) {
  }

  loadBookmarks(bookmarkstore);
  
  /*
   * Override the title attribute <title /> in this doc with a setter.
   * This is our workaround solution so that when the tabbrowser::updateTitle
   * tries to update this document's title, nothing happens. Bug 311564
   */ 
  
  document.__defineSetter__("title",function(x){}); // Stays with the titled defined by the XUL element. 
  
  gBrowser.addEventListener("DOMLinkAdded", BrowserLinkAdded, false);

  /*
   * We save the inputField (anonymous node within textbox urlbar) BoxObject, so we can measure its width 
   * on progress load
   */

  /* CHECK THIS - We need to think in other approach to have the Progress bar. */
  
  gURLBarBoxObject=(document.getBoxObjectFor(document.getElementById("nav-bar")));  

  /*
   * Local bundle repository
   */

  gMinimoBundle = document.getElementById("minimo_properties");

  /*
   * Check to see if we should set ourselves as the default
   * app.  no annoying dialog -- just do it if the pref is
   * set.
   */
  
  try {  // so we pass with the Desktop. 
  
  var device = Components.classes["@mozilla.org/device/support;1"].getService(nsIDeviceSupport);
  if (!device.isDefaultBrowser() && device.shouldCheckDefaultBrowser)
    device.setDefaultBrowser();
   
  } catch ( ignore ) { } 
    
 /*
  * Software Keyboard Windows CE Interaction 
  */

  try {
	gKeyboardService = Components.classes["@mozilla.org/softkbservice/service;1"]
                                 .getService(nsISoftKeyBoard);
  } catch (i) { }

  /*
   * Add an observer to deal with the OS Soft keyboard 
   * and the XUL UE adjust. We add XUL space which is a virtual
   * placeholder to the keyboard. This should cover URLbar and 
   * content. 
   */
   
  var keyboardObserver = { 
    observe:function (subj, topic, data) {
      var device = Components.classes["@mozilla.org/device/support;1"].getService(nsIDeviceSupport);
      if (device.has("hasSoftwareKeyboard") != "yes")
        return;
      if(data=="open")  {  
      
          document.getElementById("keyboardContainer").setAttribute("hidden","false");
          
          try {
        
          var t = { };
          var b = { };
          var l = { };
          var r = { };
          gKeyboardService.getWindowRect(t,b,l,r);
          keyboardHeight = parseInt(b.value-t.value);
          keyboardLeft =parseInt(l.value);
          keyboardRight=parseInt(r.value);
          document.getElementById("keyboardHolder").style.height=keyboardHeight +"px";

          document.getElementById("keyboardContainer").style.height=keyboardHeight +"px";

                
          var gKeyboardXULBox = document.getBoxObjectFor(document.getElementById("keyboardHolder"));

          gKeyboardService.setWindowRect(gKeyboardXULBox.screenY,gKeyboardXULBox.screenY+keyboardHeight,keyboardLeft,keyboardRight);   

        } catch (e) { }
        
      } else if(data=="close")  {
        document.getElementById("keyboardContainer").setAttribute("hidden","true");
      }  

    }
  };

  function InformUserAboutLowMem() {
    document.getElementById("statusbar").hidden=false;
    document.getElementById("statusbar-text").label="Stopped. Low on memory.";        
  }

  var minimoAppObserver = { 
    observe:function (subj, topic, data) {

      if (topic=="open-url")
      {
        try { 
          gBrowser.selectedTab = gBrowser.addTab(data);
          browserInit(gBrowser.selectedTab);
        } catch (e) {
          onErrorHandler(e);
        }
      }

      else if (topic=="add-bm")
      {
        BrowserBookmarkURL(data, null, null);
      }  

      else if (topic=="low-mem")
      {
        gBrowser.webNavigation.stop(nsIWebNavigation.STOP_ALL);

        setTimeout("InformUserAboutLowMem()",10);
      }  
      else if (topic=="softkey")
      {
        if (data=="left")
        {
          spinCycle();
        }
        else if (data=="left+shift")
        {
          DoLeftSoftkeyWithModifier();
        }
        else if (data=="right")
        {
          DoToggleSoftwareKeyboard();
        }
        else if (data=="right+shift")
        {
          DoRightSoftkeyWithModifier();
        }
      }
    }
  };
     
  try {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(nsIObserverService);
    os.addObserver(keyboardObserver,"software-keyboard", false);
    os.addObserver(minimoAppObserver,"open-url", false);
    os.addObserver(minimoAppObserver,"add-bm", false);
    os.addObserver(minimoAppObserver,"low-mem", false);
    os.addObserver(minimoAppObserver,"softkey", false);

  } catch(ignore) { }
  
 /*
  * Enable Key Spin Control 
  */
 
 spinCreate();

 /* 
  * XUL hack to the Toolkit ( bug 341017 )
  * -----------------------
  * We trap the showPopup method for the element id=PopupAutoComplete ( see minimo.xul )
  * so we can force the showPopup to be called with the right parameters. 
  * See bug: 341017
  */ 
  
 document.getElementById("PopupAutoComplete").StoredShowPopup = document.getElementById("PopupAutoComplete").showPopup;
 document.getElementById("PopupAutoComplete").showPopup = PopupAutoCompleteShowPop;

 /* 
  * XUL hack to the Toolkit ( bug 355320 )
  * -----------------------
  * Now the Context menu on URLbar goes up. 
  */
  
  var urlBarTextInputBox = (document.getAnonymousElementByAttribute(document.getElementById("urlbar"),"anonid","textbox-input-box"));
  var urlBarTextInputBoxContextMenu = (document.getAnonymousElementByAttribute(urlBarTextInputBox ,"anonid","input-box-contextmenu"));
  urlBarTextInputBoxContextMenu.setAttribute("position","before_start");

 /*
  * Setup the screen for fullscreen or not
  */
  setTimeout("setScreenUpTimeout()",10);
 
 /*
  *  Stores the initial page ( defined currently in CSS )
  *  text size value in a global gTextSize. If we are to use 
  *  preferences, then we can associate this initialization 
  *  with the pref value load. 
  */

  BrowserInitializeTextSizeValue();
 
 /* 
  * We get the visibility mask for the control bar 
  * and apply to it in the XUL
  */

  syncControlBar();

  BrowserChromeThemeColorSync(gBackGroundColor);

 /* 
  * Hack to Debug the soft keybord emulation in the desktop 
  */ 

  //document.addEventListener("keydown",debug_test_f10,true);


  /* 
   * We watch closing tabs.. so that we know how to keep some of them 
   */

   gBrowser.addEventListener("TabClose", BrowserClosingTabs, true);

}

function debug_test_f10(e) {

 	if(e.keyCode==KeyEvent.DOM_VK_F10) {

		// marcio 20000 

		spinCycle();

      } 

}


function syncControlBar(fullList) {

 /* 
  * Custom Toolbar 
  * so far we have this check here in JS,. The ui.controlbar pref has a list of   
  * ids that should be visible
  */ 

  try { 
	var listItems = fullList.split(";");
	for(var i=0;i<listItems.length;i++) {
        try {			
          var elementName =listItems[i]; 
          if(document.getElementById(elementName)) {
            document.getElementById(elementName).setAttribute("hidden","true");
          }
        } catch (i) { } 
	}
  } catch (e) {
  }
  

  try { 
    visibleToolbarItemsList = gPref.getCharPref("ui.controlbar");
	var listItems = visibleToolbarItemsList.split(";");
	for(var i=0;i<listItems.length;i++) {
        try {			
          var elementName =listItems[i]; 
          if(document.getElementById(elementName)) {
            document.getElementById(elementName).setAttribute("hidden","false");
          }
        } catch (i) { } 
	}
  } catch (e) {
  }

}


function setScreenUpTimeout() {

  try {
    if (gPref.getBoolPref("ui.fullscreen") == true)
      DoFullScreen(true);
    else
      DoFullScreen(false);
  } catch(e) {}

}

/* 
 * Trap function to PopupAutoComplete (id=) / bug 341017
 */ 
function PopupAutoCompleteShowPop(t1,t2,t3,t4,t5,t6) {
  document.getElementById("PopupAutoComplete").StoredShowPopup(t1,-1,-1,t4,"topleft","bottomleft");
}

/* 
 * UTILs to the keyboard / XUL interaction 
 */
 
function getPosX(refElement) {

  var calcLeft = 0;
  if (refElement.offsetParent)
  {
    while (refElement.offsetParent)
    {
      calcLeft += refElement.offsetLeft
        refElement= refElement.offsetParent;
    }
  }
  
  return calcLeft;
}

/* 
 * UTILs to the keyboard / XUL interaction 
 */

function getPosY(refElement) {

  var calcTop = 0;
  if (refElement.offsetParent)
  {
    while (refElement.offsetParent)
    {
      calcTop += refElement.offsetTop
        refElement= refElement.offsetParent;
    }
  }
  
  return calcTop ;
}

/* 
 * XUL > Menu > Tabs > Creates menuitems for each tab. 
 * When the XUL Nav menu > Tabs Item is selected,
 * meaning the MenuTabsContainer is show, 
 * contents are dynamically written. Check id="MenuTabsContainer"
 * 
 */
function BrowserMenuTabsActive() {
  for (var i = 0; i < gBrowser.mPanelContainer.childNodes.length; i++) {
    tabItem=gBrowser.mTabContainer.childNodes[i];
    var tabMenuElement=document.createElement("menuitem");
    tabMenuElement.setAttribute("label",tabItem.label);
    tabMenuElement.setAttribute("oncommand","BrowserTabFocus("+i+")");    
    document.getElementById("MenuTabsContainer").appendChild(tabMenuElement);	
  }
}

function BrowserTabFocus(i) {
  gBrowser.selectedTab=gBrowser.mTabContainer.childNodes[i];
  gBrowser.contentWindow.focus();
}

/* 
 * Menu > Tabs -> destroy tab reference elements.
 * When the XUL Nav menu > id="MenuTabsContainer" is hidden,
 * menuitems are removed from the menu. 
 */
function BrowserMenuTabsDestroy() {
  var refTabMenuContainer=document.getElementById("MenuTabsContainer");
  while(refTabMenuContainer.firstChild) {
    refTabMenuContainer.removeChild(refTabMenuContainer.firstChild);
  }
}

/*
 * Page's new Link tag handlers. This should be able to be smart about RSS, CSS, and maybe other Minimo stuff?  
 * So far we have this here, so we can experience and try some new stuff. To be tabrowsed.
 */
function BrowserLinkAdded(event) {
  // ref http://lxr.mozilla.org/mozilla/source/browser/base/content/browser.js#2070
  
  /* 
   * Taken from browser.js - yes this should be in tabbrowser
   */
  
  var erel = event.target.rel;
  var etype = event.target.type;
  var etitle = event.target.title;
  var ehref = event.target.href;
  
  const alternateRelRegex = /(^|\s)alternate($|\s)/i;
  const rssTitleRegex = /(^|\s)rss($|\s)/i;
  
  if (!alternateRelRegex.test(erel) || !etype) return;
  
  etype = etype.replace(/^\s+/, "");
  etype = etype.replace(/\s+$/, "");
  etype = etype.replace(/\s*;.*/, "");
  etype = etype.toLowerCase();
  
  if (etype == "application/rss+xml" || etype == "application/atom+xml" || (etype == "text/xml" || etype == "application/xml" || etype == "application/rdf+xml") && rssTitleRegex.test(etitle))
  {
    
    const targetDoc = event.target.ownerDocument;
    
    var browsers = gBrowser.browsers;
    var shellInfo = null;
    
    for (var i = 0; i < browsers.length; i++) {
      var shell = findChildShell(targetDoc, browsers[i].docShell, null);
      if (shell) shellInfo = { shell: shell, browser: browsers[i] };
    }
    
    //var shellInfo = this._getContentShell(targetDoc);
    
    var browserForLink = shellInfo.browser;
    
    if(!browserForLink) return;
    
    var feeds = [];
    if (browserForLink.feeds != null) feeds = browserForLink.feeds;
    var wrapper = event.target;
    feeds.push({ href: wrapper.href, type: etype, title: wrapper.title});
    browserForLink.feeds = feeds;
    
    if (browserForLink == gBrowser || browserForLink == gBrowser.mCurrentBrowser) {
      var feedButton = document.getElementById("feed-button");
      if (feedButton) {
        feedButton.setAttribute("feeds", "true");
        //				feedButton.setAttribute("tooltiptext", gNavigatorBundle.getString("feedHasFeeds"));	
        document.getElementById("feed-button-menu").setAttribute("onpopupshowing","DoBrowserRSS('"+ehref+"')");
      }

      var feedToolbarButton = document.getElementById("nav-rss");
	if(	feedToolbarButton ) {

		feedToolbarButton.collapsed=false;
		feedToolbarButton.setAttribute("oncommand","DoBrowserRSS('"+ehref+"')");
	}
    } 

  }
}

function BrowserUpdateFeeds() {
  var feedButton = document.getElementById("feed-button");
  var feedToolbarButton = document.getElementById("nav-rss");

  if (!feedButton)
    return;

  if (!feedToolbarButton)
    return;
  
  var feeds = gBrowser.mCurrentBrowser.feeds;
  
  if (!feeds || feeds.length == 0) {

    if (feedButton.hasAttribute("feeds")) feedButton.removeAttribute("feeds");

    feedToolbarButton.collapsed=true;
    feedToolbarButton.setAttribute("oncommand","");

  } else {

    feedButton.setAttribute("feeds", "true");
    document.getElementById("feed-button-menu").setAttribute("onpopupshowing","DoBrowserRSS('"+feeds[0].href+"')");
    
    feedToolbarButton.collapsed=false;
    feedToolbarButton.setAttribute("oncommand","DoBrowserRSS('"+feeds[0].href+"')");

  }
}

/* 
 * For now, this updates via DOM the top menu. Context menu should be here as well. 
 */

function BrowserUpdateBackForwardState() {

  if(gBrowser.webNavigation.canGoBack) {
    document.getElementById("command_back").hidden = false;
    document.getElementById("nav-back").className="";
  } else {
    document.getElementById("command_back").hidden = true;
    document.getElementById("nav-back").className="unactive";
  }
        
  if(gBrowser.webNavigation.canGoForward) {
    document.getElementById("command_forward").hidden = false;
    document.getElementById("nav-forward").className="";
  } else {
    document.getElementById("command_forward").hidden = true;
    document.getElementById("nav-forward").className="unactive";
  }
}


function findChildShell(aDocument, aDocShell, aSoughtURI) {
  aDocShell.QueryInterface(nsIWebNavigation);
  aDocShell.QueryInterface(nsIInterfaceRequestor);
  var doc = aDocShell.getInterface(nsIDOMDocument);
  if ((aDocument && doc == aDocument) || 
      (aSoughtURI && aSoughtURI.spec == aDocShell.currentURI.spec))
    return aDocShell;
  
  var node = aDocShell.QueryInterface(nsIDocShellTreeNode);
  for (var i = 0; i < node.childCount; ++i) {
    var docShell = node.getChildAt(i);
    docShell = findChildShell(aDocument, docShell, aSoughtURI);
    if (docShell) return docShell;
  }
  return null;
}


/** 
 * Init stuff
 * 
 **/
function browserInit(refTab)  
{

  var refBrowser=gBrowser.getBrowserForTab(refTab);
  
  try {
    refBrowser.markupDocumentViewer.textZoom = .90;
  } catch (e) {
    
  }
  gURLBar = document.getElementById("urlbar");

}

function MiniNavShutdown()
{
  if (gBrowserStatusHandler) gBrowserStatusHandler.destroy();
  try {
    var psvc = Components.classes["@mozilla.org/preferences-service;1"]
      .getService(nsIPrefService);
    
    psvc.savePrefFile(null);
    
  } catch (e) { onErrorHandler(e); }
}

function loadURI(uri)
{
  gBrowser.webNavigation.loadURI(uri, nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP, null, null, null);
}

function BrowserHome()
{
  var homepage = "http://www.mozilla.org";

  var page = gPref.getCharPref("browser.startup.homepage");
  if (page != null)
  {
    homepage = BrowserFixUpURI(page);
  }

  loadURI(homepage);

}

function BrowserBack()
{
  gBrowser.webNavigation.goBack();
}

function BrowserForward()
{
  gBrowser.webNavigation.goForward();
}

function BrowserStop()
{
  gBrowser.webNavigation.stop(nsIWebNavigation.STOP_ALL);
}

function BrowserReload()
{
  gBrowser.webNavigation.reload(nsIWebNavigation.LOAD_FLAGS_NONE);
}

/* 
 * Combine the two following functions in one
 */
function BrowserOpenTab()
{
  try { 
    gBrowser.selectedTab = gBrowser.addTab('about:blank');
    browserInit(gBrowser.selectedTab);
  } catch (e) {
    onErrorHandler(e);
  }
  //  if (gURLBar) setTimeout(function() { gURLBar.focus(); }, 0);  
}


function BrowserCloseTab()
{

  gBrowser.removeCurrentTab();

}

/* 
 * This function is called always when tbabrowser tries to close a tab ..
 */ 

function BrowserClosingTabs(e) {

	BrowserChromeUnregisterTab(e.originalTarget);

}

/* 
 * Used by the Context Menu - Open link as Tab 
 */
function BrowserOpenLinkAsTab() 
{
  
  if(gPopupNodeContextMenu) {
    try { 
      gBrowser.selectedTab = gBrowser.addTab(gPopupNodeContextMenu);
      browserInit(gBrowser.selectedTab);
    } catch (e) {
      onErrorHandler(e);
    }
  }
}

/*
 * Open URL as Tab. 
 */

function BrowserOpenURLasTab(tabUrl) {
  try {  
    gBrowser.selectedTab = gBrowser.addTab(tabUrl);   
    browserInit(gBrowser.selectedTab);
  } catch (e) {
  }  
}

/**
 * FOR - keyboard acessibility - context menu for tabbed area *** 
 * Launches the popup for the tabbed area / tabbrowser. Make sure to call this function 
 * when the tabbed panel is available. WARNING somehow need to inform which tab was lack clicked 
 * or mouse over.  
 *
 **/
function BrowserLaunchTabbedPopup() {
  var tabMenu = document.getAnonymousElementByAttribute(document.getElementById("content"),"anonid","tabContextMenu");
  tabMenu.showPopup(gBrowser.selectedTab,-1,-1,"popup","bottomleft", "topleft");
}

/**
 * Has to go through some other approach like a XML-based rule system. 
 * Those are constraints conditions and action. 
 **/

function BrowserViewOptions() {
  document.getElementById("toolbar-view").collapsed=!document.getElementById("toolbar-view").collapsed;
  if(document.getElementById("toolbar-view").collapsed &&  document.getElementById("command_ViewOptions").getAttribute("checked")=="true") {
	document.getElementById("command_ViewOptions").setAttribute("checked","false");
  }
}

/**
 * Has to go through some other approach like a XML-based rule system. 
 * Those are constraints conditions and action. 
 **/

function BrowserViewRSS() {
  document.getElementById("toolbar-rss").collapsed=!document.getElementById("toolbar-rss").collapsed;
  if(document.getElementById("toolbar-rss").collapsed &&  document.getElementById("command_ViewRSS").getAttribute("checked")=="true") {
	document.getElementById("command_ViewRSS").setAttribute("checked","false");
  }
}

/**
 * Deckmode urlbar selector. 
 * Toggles menu item and deckmode.
 */
function BrowserViewDeckSB() {
  BrowserSetDeck(1,document.getElementById("command_ViewDeckSB"));
}

function BrowserViewDeckSearch() {
  BrowserSetDeck(2,document.getElementById("command_ViewDeckSearch"));
}

function BrowserViewDeckDefault() {
  BrowserSetDeck(0,document.getElementById("command_ViewDeckDefault"));
}

/**
 * Has to go through some other approach like a XML-based rule system. 
 * Those are constraints conditions and action. 
 **/

function BrowserViewSearch() {
  document.getElementById("toolbar-search").collapsed=!document.getElementById("toolbar-search").collapsed;
  if(document.getElementById("toolbar-search").collapsed &&  document.getElementById("command_ViewSearch").getAttribute("checked")=="true") {
	document.getElementById("command_ViewSearch").setAttribute("checked","false");
  }
}

/**
 * Has to go through some other approach like a XML-based rule system. 
 * Those are constraints conditions and action. 
 **/

function BrowserViewFind() {
  document.getElementById("toolbar-find").collapsed=!document.getElementById("toolbar-find").collapsed;
  if(document.getElementById("toolbar-find").collapsed &&  document.getElementById("command_ViewFind").getAttribute("checked")=="true") {
	document.getElementById("command_ViewFind").setAttribute("checked","false");
  }
  else
  {
    document.getElementById("toolbar-find-tag").focus();
  }
}

/* 
 * The new URLBar is a separated toolbar. Actually we have moved and created 
 * a new URLBAR option in the bookmarks/homebase. This is here because some users
 * may want to have an URLBar simultaneous experience 
 */

function BrowserViewURLBar() {
  document.getElementById("toolbar-urlbar").collapsed=!document.getElementById("toolbar-urlbar").collapsed;
  if(document.getElementById("toolbar-urlbar").collapsed) {
  }
  else
  {
    document.getElementById("urlbar").focus();
  }
}

/** 
 * urlbar indentity, style, progress indicator.
 **/ 
function urlbar() {
}

/* 
 * Text Size functions - works accross all the Tabs 
 */

function BrowserResetZoomPlus() {
  gTextSize+= .1;
  BrowsersZoomUpdate();
}

function BrowserResetZoomMinus() {
  gTextSize-= .1;
  BrowsersZoomUpdate();
}

function BrowsersZoomUpdate() {
 for (var i = 0; i < gBrowser.mPanelContainer.childNodes.length; i++) {
    tabItem=gBrowser.mTabContainer.childNodes[i];
    gBrowser.getBrowserForTab(tabItem).markupDocumentViewer.textZoom=gTextSize;
  }
}

function BrowserInitializeTextSizeValue() {
  gTextSize = gBrowser.selectedBrowser.markupDocumentViewer.textZoom;
}


function isContentFrame(aFocusedWindow)
{
  if (!aFocusedWindow)
    return false;
  
  return (aFocusedWindow.top == window.content);
}

function BrowserContentAreaPopupShowing () {

  var selectedRange=gBrowser.selectedBrowser.contentDocument.getSelection();

  /* Enable Copy */
  
  if( selectedRange && selectedRange.toString() ) {
    document.getElementById("item-copy").hidden=false;
  } else {
    document.getElementById("item-copy").hidden=true;
  }
  
  /* Enable Paste - Can paste only if the focused element has a value attribute. :) 
     THis may affect XHTML nodes. Users may be able to paste things within XML nodes. 
  */

  var targetPopupNode = document.popupNode; 

  if( targetPopupNode instanceof HTMLInputElement || targetPopupNode instanceof HTMLTextAreaElement ) {
      if(DoClipCheckPaste()) {
        document.getElementById("item-paste").hidden=false;
        gPopupNodeContextMenu = targetPopupNode; 
      }
  } else {
    document.getElementById("item-paste").hidden=true;
  } 

  /*
   * Open Link as New Tab  
   */ 
  if( targetPopupNode.href ) { 
    gPopupNodeContextMenu = targetPopupNode.href;
    document.getElementById("link_as_new_tab").hidden=false;
  } else {
    document.getElementById("link_as_new_tab").hidden=true;
  }

  /*
   * Open Frame in new tab
   */

  var frameItem = document.getElementById("open_frame_in_tab");
  if (!content || !content.frames.length || !isContentFrame(document.commandDispatcher.focusedWindow))
    frameItem.setAttribute("hidden", "true");
  else
    frameItem.removeAttribute("hidden");

}

/* Bookmarks */ 

function BrowserBookmarkURL (uri, title, icon) {

 /* So far to force resync load bookmark from the pref, there are cases, bookmark is 
  * erased and we need to check
  */
  var bookmarkstore = gPref.getCharPref("browser.bookmark.store");
  loadBookmarks(bookmarkstore);

  var newLi=gBookmarksDoc.createElement("li");

  if (title)
    newLi.setAttribute("title",title);
  else
    newLi.setAttribute("title",uri);

  if(icon)
	newLi.setAttribute("iconsrc",icon);
  else
	newLi.setAttribute("iconsrc","chrome://minimo/skin/m.gif");
	
  if(uri.indexOf("rss")==24) { newLi.setAttribute("rss","yes") } 
  else {
   newLi.setAttribute("page","yes") 
  }

  var bmContent=gBookmarksDoc.createTextNode(uri);

  newLi.appendChild(bmContent);
  gBookmarksDoc.getElementsByTagName("bm")[0].appendChild(newLi);
  storeBookmarks();	
  refreshBookmarks();

}

function BrowserBookmarkThis() {

  var currentURI=gBrowser.selectedBrowser.webNavigation.currentURI.spec;
  var currentContentTitle=gBrowser.selectedBrowser.contentTitle;
  var currentIconURL=gBrowser.selectedBrowser.mIconURL;

  BrowserBookmarkURL( currentURI, currentContentTitle, currentIconURL);
}

function BrowserBookmark() {

	/* Now we use the Chrome Open because we want to reuse tabs  instead
         openning new ones */

	BrowserChromeOpen("chrome://minimo/content/bookmarks/bmview.xhtml");

}

/* Toolbar specific code - to be removed from here */ 

function DoBrowserSearch() {
  BrowserViewSearch();
  try { 
    var vQuery=document.getElementById("toolbar-search-tag").value;
    if(vQuery!="") {
      gBrowser.selectedTab = gBrowser.addTab('http://www.google.com/xhtml?q='+vQuery+'&hl=en&lr=&safe=off&btnG=Search&site=search&mrestrict=xhtml');
      browserInit(gBrowser.selectedTab);
    }
  } catch (e) {
    
  }  
}

/*
 * New preferences launches it in the tab 
 */

function DoBrowserPreferences() {
  
	/* Now we use the Chrome Open because we want to reuse tabs  instead
     * openning new ones. 
     * WARNING: We need to check the case where we close the preferences panel.
     */
	BrowserChromeOpen("chrome://minimo/content/preferences/preferences.xul#general");
}

/* 
 * Search extension to urlbar, deckmode.
 * Called form the deckmode urlbar selector
 */

function DoBrowserSearchURLBAR(vQuery) {
  try { 
    if(vQuery!="") {
      gBrowser.selectedTab = gBrowser.addTab('http://www.google.com/xhtml?q='+vQuery+'&hl=en&lr=&safe=off&btnG=Search&site=search&mrestrict=xhtml');
      browserInit(gBrowser.selectedTab);
    }
  } catch (e) {
  }  
}

/* Toolbar specific code - to be removed from here */ 

function DoBrowserRSS(sKey) {
  if(!sKey) BrowserViewRSS(); // The toolbar is being used. Otherwise it is via the sb: trap protocol. 
  try { 
    if(sKey) {
      gRSSTag=sKey;
    } else if(document.getElementById("toolbar-rss-rsstag").value!="") {
      gRSSTag=document.getElementById("toolbar-rss-rsstag").value;
    }
    gBrowser.selectedTab = gBrowser.addTab('chrome://minimo/content/rssview/rssload.xhtml?url='+gRSSTag);
    browserInit(gBrowser.selectedTab);
  } catch (e) { }  
}

function DoBrowserGM(xmlRef) {
  try {     
    gBrowser.selectedTab = gBrowser.addTab('chrome://minimo/content/moduleview/moduleload.xhtml?url='+xmlRef);
    browserInit(gBrowser.selectedTab);
  } catch (e) { }  
}

/* Toolbar specific code - to be removed from here */ 

function DoBrowserSB(sKey) {
  
  if(!sKey) BrowserViewRSS(); // The toolbar is being used. Otherwise it is via the sb: trap protocol. 
  
  try { 
    if(sKey) {
      gRSSTag=sKey;
    } else if(document.getElementById("toolbar-rss-rsstag").value!="") {
      gRSSTag=document.getElementById("toolbar-rss-rsstag").value;
    }
    
    gBrowser.selectedTab = gBrowser.addTab('chrome://minimo/content/rssview/rssload.xhtml?url=http://del.icio.us/rss/tag/'+gRSSTag);
    browserInit(gBrowser.selectedTab);
  } catch (e) {
    
  }  
}

function DoBrowserTarget(sKey) {
  
  /* skey format: handler,value */

  var baseHandler=sKey.split(",");

  if(baseHandler[0]=="home") {

     var baseURL="chrome://minimo/content/bookmarks/bmview.xhtml#";

  }

  var baseURL="chrome://minimo/content/bookmarks/bmview.xhtml#";

  if(baseHandler[0]=="preferences") {
	baseURL="chrome://minimo/content/preferences/preferences.xul#"
  } 
  
  try { 
	
    gBrowser.selectedTab = gBrowser.addTab(baseURL+baseHandler[1]);
    browserInit(gBrowser.selectedTab);

  } catch (e) {
    
  }  

}

function DoBrowserGM(xmlRef) {
  try { 
    gBrowser.selectedTab = gBrowser.addTab('chrome://minimo/content/moduleview/moduleload.xhtml?url='+xmlRef);
    browserInit(gBrowser.selectedTab);
  } catch (e) {   
  }  
}


/*
 * Toolbar Find section
 */

function DoBrowserFind() {
  //  BrowserViewFind();
  try { 
    var vQuery=document.getElementById("toolbar-find-tag").value;
    if(vQuery!="") {
      gBrowser.contentWindow.focus();
      
      /* FIND DOCUMENTATION: 
         41 const FIND_NORMAL = 0;
         42 const FIND_TYPEAHEAD = 1;
         43 const FIND_LINKS = 2;
         http://lxr.mozilla.org/mozilla/source/toolkit/components/typeaheadfind/content/findBar.js
      */
      gBrowser.fastFind.find(vQuery,0);
      document.getElementById("toolbar-find-tag").inputField.focus();
    }
  } catch (e) {
    onErrorHandler(e);
  }  
}

function onFindBarKeyPress(evt) {
  if(evt.keyCode == KeyEvent.DOM_VK_RETURN) {
    DoBrowserFindNext();
  }
}

function DoBrowserFindNext() {
  try { 
	gBrowser.fastFind.findNext();
    document.getElementById("toolbar-find-tag").inputField.focus();
  } catch (e) {
    onErrorHandler(e);
  }  
}

function DoPanelPreferences() {
  window.openDialog("chrome://minimo/content/preferences/preferences.xul","preferences","modal,centerscreeen,chrome,resizable=no");
  // BrowserReload(); 
}

/* 
   Testing the SMS and Call Services 
*/
function DoTestSendCall(toCall) {
  var phoneInterface= Components.classes["@mozilla.org/phone/support;1"].createInstance(nsIPhoneSupport);
  phoneInterface.makeCall(toCall,"");
}

function DoGoogleToggle() {

  //google xhtml string call http://www.google.com/gwt/n?q=xml&site=mozilla_minimo&u=www.xml.com/
  
  var locationAddress="google.com";

  if(gURLBar.value.indexOf("http://")>-1) {
	locationAddress=gURLBar.value.split("http://")[1];	
  }

  try {  
    gBrowser.selectedTab = gBrowser.addTab('http://www.google.com/gwt/n?q=xml&site=mozilla_minimo&u='+locationAddress);
    browserInit(gBrowser.selectedTab);
  } catch (e) {}  
}

function DoSSRToggle()
{
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"].getService(nsIPrefBranch);
    pref.setBoolPref("ssr.enabled", !pref.getBoolPref("ssr.enabled"));
    
    gBrowser.webNavigation.reload(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
  }
  catch(ex) { onErrorHandler(ex); }
}

function DoSNavToggle()
{
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"].getService(nsIPrefBranch);
    pref.setBoolPref("snav.enabled", !pref.getBoolPref("snav.enabled"));
    
    content.focus();    
  }
  catch(ex) { onErrorHandler(ex); }

}

function DoToggleSoftwareKeyboard()
{

 /* 
  * If the menu is on display, then we hide it..
  */
  
  if(gShowingMenuCurrent) {
  
    gShowingMenuCurrent.hidePopup();
   
  }
  
 /*
  * .. and.. this is called to create an Escape Entry in the keyboard spin State Machine. 
  * We want to remember this gKeySpinCurrent state, when the user press the Left Softkey.
  */
  
  spinSetnext(gKeySpinCurrent); 

  // During a page load, this key cause the page to stop
  // loading.  Probably should rename this function.

  try {

    if ( document.getElementById("nav-menu-button").className=="stop-button" )
    {
      BrowserStop();
      return;
    }

    var device = Components.classes["@mozilla.org/device/support;1"].getService(nsIDeviceSupport);

    if (device.has("hasSoftwareKeyboard") == "yes") {
      
      // use global... xxx fix
      keyboard = Components.classes["@mozilla.org/softkbservice/service;1"]
                           .getService(Components.interfaces.nsISoftKeyBoard);

      if (gKeyBoardToggle)
        keyboard.hide();
      else
        keyboard.show();
      
        gKeyBoardToggle = !gKeyBoardToggle;
    }
    else {
      document.commandDispatcher.advanceFocus();
    }
  }
  catch(ex) { onErrorHandler(ex); }
}

function OpenFrameInTab()
{
  var url = document.popupNode.ownerDocument.location.href;
  BrowserOpenURLasTab(url);
}

function FullScreenToggle()
{
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"].getService(nsIPrefBranch);
    var value = !pref.getBoolPref("ui.fullscreen");
    pref.setBoolPref("ui.fullscreen", value);

    DoFullScreen(value)
  }
  catch(ex) { onErrorHandler(ex); }
}

function DoFullScreen(fullscreen)
{
  document.getElementById("nav-bar").hidden = fullscreen;

  // Show a Quit in the context menu
  document.getElementById("context_menu_quit").hidden = !fullscreen;
  
  // Is this the simpler approach to count tabs? 
  if(gBrowser.mPanelContainer.childNodes.length>1) {
    gBrowser.setStripVisibilityTo(!fullscreen);
  } 
  
  window.fullScreen = fullscreen;  
}

/* 
 * 
 */
function DoClipCopy()
{
  var copytext=gBrowser.selectedBrowser.contentDocument.getSelection().toString();
  var str = Components.classes["@mozilla.org/supports-string;1"].createInstance(nsISupportsString);
  if (!str) return false;
  str.data = copytext;
  var trans = Components.classes["@mozilla.org/widget/transferable;1"].createInstance(nsITransferable);
  if (!trans) return false;
  trans.addDataFlavor("text/unicode");
  trans.setTransferData("text/unicode",str,copytext.length * 2);
  var clipid = nsIClipboard;

  var clip = Components.classes["@mozilla.org/widget/clipboard;1"].getService(clipid);
  if (!clip) return false;
  clip.setData(trans,null,clipid.kGlobalClipboard);
}

/* 
   Currently supports text/unicode. 
*/
function DoClipCheckPaste()
{
  var clip = Components.classes["@mozilla.org/widget/clipboard;1"].getService(nsIClipboard);
  if (!clip) return false;
  var trans = Components.classes["@mozilla.org/widget/transferable;1"].createInstance(nsITransferable);
  if (!trans) return false;
  trans.addDataFlavor("text/unicode");
  clip.getData(trans,clip.kGlobalClipboard);
  var str = new Object();
  var strLength = new Object();
  var pastetext = null;
  trans.getTransferData("text/unicode",str,strLength);
  if (str) str = str.value.QueryInterface(nsISupportsString);
  if (str) pastetext = str.data.substring(0,strLength.value / 2);
  if(pastetext) {
    return pastetext;
  } else return false;
}

function DoClipPaste()
{
  
  gPopupNodeContextMenu.focus();   // Hack. When the context menu goes open, then we store the Pastable element in gPopupNodeContextMenu
                                   // If the user clicks the element in the Context menu, the focused element changes, then is not pastable
                                   // anymore. 

  var disp = document.commandDispatcher;
  var cont = disp.getControllerForCommand("cmd_paste");
  cont.doCommand("cmd_paste");

}

function URLBarEntered()
{
  try
  {
    if (!gURLBar)
      return;
    
    var url = gURLBar.value;
    if (gURLBar.value == "" || gURLBar.value == null)
      return;
    
    /* Trap to SB 'protocol' */ 
    
    if(gURLBar.value.substring(0,3)=="sb:") {
      DoBrowserSB(gURLBar.value.split("sb:")[1]);
      return;
    }

    /* Trap to chrome targets 'target' */ 
    
    if(gURLBar.value.substring(0,3)=="go:") {
      DoBrowserTarget(gURLBar.value.split("go:")[1]);
      return;
    }
    
    /* Trap to RSS 'protocol' */ 
    
    if(gURLBar.value.substring(0,4)=="pan:") {
      gPanMode=true;
      document.getElementById("button-icon-pan").hidden=false;
      return;
    }

    if(gURLBar.value.substring(0,4)=="rss:") {
      DoBrowserRSS(gURLBar.value.split("rss:")[1]);
      return;
    }
    
    if(gURLBar.value.substring(0,3)=="gm:") {
      DoBrowserGM(gURLBar.value.split("gm:")[1]);
      return;
    }
    
    // SB mode
    if(gDeckMode==1) {
      DoBrowserSB(gURLBar.value);
      BrowserSetDeck(0,document.getElementById("command_ViewDeckDefault"));
      return;
    }
    
    if(gDeckMode==2) {
      DoBrowserSearchURLBAR(gURLBar.value);
      BrowserSetDeck(0,document.getElementById("command_ViewDeckDefault"));
      return;
    }

    /* Other normal cases */ 
    
    if (gURLBar.value.indexOf(" ") == -1)
    {

      gURLBar.value = BrowserFixUpURI(url);
      
      // Notify anyone interested that we are loading.
      try {
        var os = Components.classes["@mozilla.org/observer-service;1"]
          .getService(Components.interfaces.nsIObserverService);
        var host = gURLBar.value;
        os.notifyObservers(null, "loading-domain", host);
      }
      catch(e) {onErrorHandler(e);}

    }

    /* 
     * Each new typed URL becomes a new Tab now 
     */
     
    BrowserOpenURLasTab(gURLBar.value);

    content.focus();
  }
  catch(ex) {onErrorHandler(ex);}
  
  
  return true;
}

function PageProxyClickHandler(aEvent) {
  document.getElementById("urlbarModeSelector").showPopup(document.getElementById("proxy-deck"),-1,-1,"popup","bottomleft", "topleft");
}

/*
 * The URLBAR Deck mode selector 
 */

function BrowserSetDeck(dMode,menuElement) {
  
  gDeckMode=dMode;
  if(dMode==2) document.getElementById("urlbar-deck").className='search';
  if(dMode==1) document.getElementById("urlbar-deck").className='sb';
  if(dMode==0) document.getElementById("urlbar-deck").className='';
  
}

/* 
 * ripped from browser.js, this should be shared in toolkit.
 */
function nsBrowserAccess()
{
}

nsBrowserAccess.prototype =
{
  QueryInterface : function(aIID)
  {
    if (aIID.equals(nsIBrowserDOMWindow) ||
        aIID.equals(nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
  
  openURI : function(aURI, aOpener, aWhere, aContext)
  {
    var url = aURI ? aURI.spec : "about:blank";
    var newTab     = gBrowser.addTab(url);
    var newWindow  = gBrowser.getBrowserForTab(newTab).docShell
                             .QueryInterface(nsIInterfaceRequestor)
                             .getInterface(nsIDOMWindow);
    return newWindow;
  },
  
  isTabContentWindow : function(aWindow)
  {
    var browsers = gBrowser.browsers;
    for (var ctr = 0; ctr < browsers.length; ctr++)
      if (browsers.item(ctr).contentWindow == aWindow)
        return true;
    return false;
  }
}

/*
 * Download Service - Work-in-progress not-for-long
 */

function BrowserViewDownload(cMode) {
  document.getElementById("toolbar-download").collapsed=!cMode;
}

function DownloadSet( aCurTotalProgress, aMaxTotalProgress ) {
  gInputBoxObject=(document.getBoxObjectFor(document.getElementById("toolbar-download-tag").inputField));
  var percentage = parseInt((aCurTotalProgress/aMaxTotalProgress)*parseInt(gInputBoxObject.width));
  if(percentage<0) percentage=2;
  document.getElementById("toolbar-download-tag").inputField.style.backgroundPosition=percentage+"px 100%";
}

function DownloadCancel(refId) {

  try {  document.getElementById("toolbar-download-tag").cachedCancelable.cancel(NS_BINDING_ABORTED) } catch (e) { onErrorHandler(e) };
  document.getElementById("download-button-stop").disabled=false;
  BrowserViewDownload(false);

}

function DownloadLaunch() {
  var url = document.getElementById("toolbar-download-tag").getAttribute("destlocation");

  var ioSvc = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
 
  const fileUrl = ioSvc.newURI(url, null, null).QueryInterface(Components.interfaces.nsIFileURL);

  fileUrl.file.QueryInterface(nsILocalFile).launch();
  
  //  fileUrl.file.QueryInterface(Components.interfaces.nsILocalFile).reveal();
  
}

function TransferItemFactory() {
}

TransferItemFactory.prototype = {
   createInstance: function(delegate, iid) {
    return new TransferItem().QueryInterface(iid);
  },
  lockFactory: function(lock) {
  }
};

function TransferItem() {
}

TransferItem.prototype = {
  
  QueryInterface: function (iid) {
    if (iid.equals(nsITransfer) ||
        iid.equals(nsIWebProgressListener) ||
        iid.equals(nsIWebProgressListener2) ||
        iid.equals(nsISupports))
      return this;
    
    Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  },
  
  init: function (aSource, aTarget, aDisplayName, aMIMEInfo, startTime, aTempFile, aCancelable) {
    
   // document.getElementById("statusbar").hidden=false;
    BrowserViewDownload(true);
    document.getElementById("toolbar-download-tag").cachedCancelable=aCancelable;
    document.getElementById("download-button-stop").disabled=false;
    document.getElementById("toolbar-download-tag").value=aSource.spec; 
    document.getElementById("toolbar-download-tag").setAttribute("sourcelocation",aSource);
    document.getElementById("toolbar-download-tag").setAttribute("destlocation",aTarget.spec);
    document.getElementById("toolbar-download-tag").inputField.style.backgroundColor="lightgreen";
    document.getElementById("download-close").hidden=true;

  },
  
  onStateChange: function( aWebProgress, aRequest, aStateFlags, aStatus ) {

       if ( aStateFlags & nsIWebProgressListener.STATE_STOP ) {

          document.getElementById("download-button-stop").label=document.getElementById("minimo_properties").getString("downloadButtonLaunch");
          document.getElementById("download-button-stop").setAttribute("oncommand","DownloadLaunch()");
          document.getElementById("toolbar-download-tag").inputField.style.backgroundColor="lightgreen";
          document.getElementById("download-close").hidden=false;

       }
    
  },
  
  onProgressChange: function( aWebProgress,
                              aRequest,
                              aCurSelfProgress,
                              aMaxSelfProgress,
                              aCurTotalProgress,
                              aMaxTotalProgress ) {
    
    return onProgressChange64(aWebProgress, aRequest, aCurSelfProgress, 
                              aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
  },
  
  onProgressChange64: function( aWebProgress,
                                aRequest,
                                aCurSelfProgress,
                                aMaxSelfProgress,
                                aCurTotalProgress,
                                aMaxTotalProgress ) {
    
    //document.getElementById("statusbar-text").label= "dbg:onProgressChange " + aCurTotalProgress + " " + aMaxTotalProgress;

    DownloadSet( aCurTotalProgress, aMaxTotalProgress );

  },
  
  onStatusChange: function( aWebProgress, aRequest, aStatus, aMessage ) {
  },
  
  onLocationChange: function( aWebProgress, aRequest, aLocation ) {
  },
  
  onSecurityChange: function( aWebProgress, aRequest, state ) {
  },
}


/* Prototype PAN */ 

  
function BrowserPan() {

	if(!gBrowser.contentWindow.gInPan) {  

		gBrowser.contentDocument.addEventListener("mousedown",BrowserPanMouseHandler,true);
		gBrowser.contentDocument.addEventListener("mouseup",BrowserPanMouseHandlerDestroy,true);
		gBrowser.contentDocument.addEventListener("click",BrowserPanMouseHandlerPanNull,true);
		document.getElementById("toolbar-pan").collapsed=false;
		gBrowser.contentWindow.gInPan=true;
	} else {
		gBrowser.contentDocument.removeEventListener("mousedown",BrowserPanMouseHandler,true);
		gBrowser.contentDocument.removeEventListener("mouseup",BrowserPanMouseHandlerDestroy,true);
		gBrowser.contentDocument.removeEventListener("click",BrowserPanMouseHandlerPanNull,true);
		document.getElementById("toolbar-pan").collapsed=true;
		gBrowser.contentWindow.gInPan=false;
	}
}

function BrowserPanRefresh() {
  if(gBrowser.contentWindow.gInPan) {
		document.getElementById("toolbar-pan").collapsed=false;

  } else {
		document.getElementById("toolbar-pan").collapsed=true;
  }
}
var gInPan=false;
var gPanY=-1;
var gPanX=-1;
var gInitialPanX=null;
var gInitialPanY=null;

function BrowserPanMouseHandler(e) {

  gBrowser.contentDocument.addEventListener("mousemove",BrowserPanMouseHandlerPan,true); 
  gPanY=e.clientY;
  gPanX=e.clientX;
  e.preventDefault();
  e.stopPropagation();
  gInitialPanY=gPanY;
  gInitialPanX=gPanX;

}

function BrowserPanMouseHandlerPan(e) {
  panDeltaY=gPanY-e.clientY;
  panDeltaX=gPanX-e.clientX;
  
  /* Workaround to bug 327934 */
  gBrowser.contentWindow.scrollBy(0,panDeltaY);
  gBrowser.contentWindow.scrollBy(panDeltaX,0);

  gPanY=e.clientY;
  gPanX=e.clientX;
}

function BrowserPanMouseHandlerPanNull(e) {
    e.preventDefault();
}

function BrowserPanMouseHandlerDestroy(e) {
  gBrowser.contentDocument.removeEventListener("mousemove",BrowserPanMouseHandlerPan,true);
  e.preventDefault();
  e.stopPropagation();
  if(e.clientY==gInitialPanY || e.clientX==gInitialPanX) {
	BrowserPan();
  }
}

function DoLeftSoftkeyWithModifier()
{
  alert("DoLeftSoftkeyWithModifier");
}

function DoRightSoftkeyWithModifier()
{
  alert("DoRightSoftkeyWithModifier");
}

/*
 * Keyboard Spin Menu Control 
 * --
 * The key spin engine. This will call the SpinOut() state of the current State, 
 * will set the current State to be the .next ( of the linked list defined in 
 * spinCreate function ) and will call the SpinIn state of the next. The setTimeout
 * was used because of a bug. 
 */

function spinCycle() {

  gKeySpinCurrent.SpinOut();
  gKeySpinCurrent = gKeySpinCurrent.next;
  setTimeout("gKeySpinCurrent.SpinIn()",30);
}

/* 
 * The spinSetNext
 * ---
 * Is used to set a temporary state to the keyboard spin state machine.
 * Let's say if the user hits the Keyboard softkey when the state is
 * over a menu, we want to tell that the Next State is the Current state. 
 * So when it press the Left softkey, it will recover the current state. 
 */
 
function spinSetnext(ref) {

  /* 
   * This should be performed only once to break the normal Spin states. 
   * If you call this twice it will call itself thus loopback. 
   */
   
  if(ref!=gSpinTemp) {
    gSpinTemp.next  = ref;
    gKeySpinCurrent = gSpinTemp;
  } 
  
}

function spinCreate() {

  /* 
   * not in use with the new homebase 
   */

  var spinUrlBar = { 
    SpinIn:function () {
      var urlbar = document.getElementById("urlbar");
      urlbar.focus();
      urlbar.select();
    }, 
    SpinOut:function () {

      var device = Components.classes["@mozilla.org/device/support;1"].getService(nsIDeviceSupport);
      
      if (device.has("hasSoftwareKeyboard") == "yes") {
        // use global... xxx fix
        keyboard = Components.classes["@mozilla.org/softkbservice/service;1"]
                             .getService(Components.interfaces.nsISoftKeyBoard);

        if (gKeyBoardToggle)
          keyboard.hide();
        else
          keyboard.show();
        
        gKeyBoardToggle = !gKeyBoardToggle;
      }
    }
  }

  /* 
   * New homebase version uses it 
   */

  var spinRightMenu = { 
    SpinIn:function () {
      document.getElementById("nav-menu-button").focus();
      document.getElementById("menu_NavPopup").showPopup(document.getElementById("nav-menu-button"),-1,-1,"popup","bottomright", "topright");
      gShowingMenuCurrent=document.getElementById("menu_NavPopup");		
    }, 
    SpinOut:function () {
      document.getElementById("menu_NavPopup").hidePopup();
    }
  }

  /* 
   * This reaches the actual content of the selected Tab 
   */

  var spinContent = { 

    SpinIn:function () {

        /* Ask marcio, somehow advance and rewind Igot a better behavior, it kicks the focus to finds its first element in
           in the doc, then the backwards allows to reach the actual first focused one */

	  gBrowser.contentWindow.focus();
	  document.commandDispatcher.advanceFocus();

    }, 
    SpinOut:function () {

    }
  }

  /* 
   *  Reaches the tab level 
   */

  var spinTabs = { 

    SpinIn:function () {


 	 if(gBrowser.mPanelContainer.childNodes.length>1) {
	  gBrowser.selectedTab.focus();
 	 } else {
		spinCycle();
	 } 

    }, 
    SpinOut:function () {

    }
  }

  var spinDocument = { 
    SpinIn:function () {
      if(gKeySpinLastFocused) {
        gKeySpinLastFocused.focus();	// pass through some other previous state stored.
      } else {
        spinCycle();			// this should not occur
      }
    }, 
    SpinOut:function () {
    }
  }


  gSpinTemp = {	
    SpinIn:function () { }, 
    SpinOut:function () { }
  }

  gKeySpinCurrent = spinContent;

  spinContent.next=spinRightMenu;  
  spinRightMenu.next=spinTabs;  
  spinTabs.next=spinContent;
  gSpinLast=spinContent;
  gSpinDocument = spinDocument;
  gSpinFirst=spinRightMenu;
  gSpinUrl=spinUrlBar;

}

/* 
 * Menu Menu Controls
 */ 

function BrowserNavMenuPopup() {

   ref=document.getElementById("menu_NavPopup");

   if(gShowingMenuCurrent==ref) {

	gShowingMenuCurrent.hidePopup();

   } else {

	if(!gShowingMenuCurrent) {
		gShowingMenuCurrent=ref;
	} 

      gShowingMenuCurrent.showPopup(document.getElementById("nav-menu-button"),-1,-1,"popup","bottomright", "topright");

   }
}
  

function BrowserNavMenuRefresh() {

   try {
    var hasTabs = (gBrowser.tabContainer.childNodes.length > 1);
    document.getElementById("command_BrowserCloseTab").hidden=!hasTabs;
    document.getElementById("command_TabFocus").hidden=!hasTabs;
  }
  catch(ex) { onErrorHandler(ex); }

}

function BrowserNavMenuHidden() {

  gShowingMenuCurrent=null;

}


/*
 * File Open Functionality. For now we allow multiple selections and 
 * we open multiple tabs at the same time. 
 */ 
 
function BrowserFileOpen() {

  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  var refLocalFile = Components.classes["@mozilla.org/file/local;1"].createInstance(nsIFile);
  fp.init(window, null, nsIFilePicker.modeOpenMultiple);

  var fileCustomDirFile= refLocalFile.QueryInterface(nsILocalFile);
  fileCustomDirFile.initWithPath("\\");
  fp.displayDirectory = fileCustomDirFile;

  fp.appendFilters(nsIFilePicker.filterAll);

  var returnFilePickerValue=fp.show();

  if (returnFilePickerValue == nsIFilePicker.returnOK) {

    entries =(fp.files);
    while(entries.hasMoreElements()) {
      var entry = entries.getNext();
      entry.QueryInterface(nsIFile);
      try { 
        gBrowser.selectedTab = gBrowser.addTab("file:///"+entry.path);    
        browserInit(gBrowser.selectedTab);
      } catch (e) {}  
    }
    
  }
  
}

/*
 * URI canonizer / fix up util 
 */

function BrowserFixUpURI(pageURI) {

  var fixedUpURI = gURIFixup.createFixupURI(pageURI, 0);
  return fixedUpURI.spec;

}

/* 
 * Layout Theme System. This is the solution so that the general background color 
 * that is set via preferences->layout can affect the whole chrome panels.
 */

var gBrowserChromeTabs = new Array();
var gBrowserChromeThemeRules = new Array();

function BrowserChromeThemeColorSync(value) {
  document.styleSheets[1].cssRules[1].style.cssText="background-color:"+value;
  for ( keyVar in gBrowserChromeThemeRules ) {
    try {
      gBrowserChromeThemeRules[keyVar].cssText="background-color:"+value;		
    } catch (i) {} 
  }
}

function BrowserChromeThemeColorSyncRaw(value) {
  document.styleSheets[1].cssRules[1].style.cssText=value;
  for ( keyVar in gBrowserChromeThemeRules ) {
    try { 
      gBrowserChromeThemeRules[keyVar].cssText=value;		
    } catch (i) {
    } 
  }
}

function BrowserTellChromeThemeRules(refName,ruleReference) {
  gBrowserChromeThemeRules[refName] = ruleReference;
}

function BrowserChromeThemeColorGet() {
  gGlobalThemeValue = document.styleSheets[1].cssRules[1].style.cssText;
  gGlobalThemeValue = gGlobalThemeValue.split(";")[0];

  return gGlobalThemeValue;
}

function BrowserChromeRegisterTab(URIvalue,tabReference) {
  var newChromeApp = {
    URIsrc:URIvalue,
    tabReference:tabReference
  } 
  gBrowserChromeTabs[URIvalue] = newChromeApp;
}

function BrowserChromeUnregisterTab(tabReference) {

  var uriToGo = gBrowser.getBrowserForTab(tabReference).currentURI.spec;

  if(gBrowserChromeTabs[uriToGo]) {
     gBrowserChromeTabs[uriToGo]=null;
  } 
}

function BrowserChromeOpen(URIvalue) {
  if(gBrowserChromeTabs[URIvalue]) {
    gBrowser.selectedTab = gBrowserChromeTabs[URIvalue].tabReference;
  } else {
    try {  
      gBrowser.selectedTab = gBrowser.addTab(URIvalue);   
      browserInit(gBrowser.selectedTab);
      BrowserChromeRegisterTab(URIvalue,gBrowser.selectedTab);
    } catch (e) {}  
  } 
}

