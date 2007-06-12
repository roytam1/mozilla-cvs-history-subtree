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
 * The Original Code is FUEL.
 *
 * The Initial Developer of the Original Code is Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Finkle <mfinkle@mozilla.com> (Original Author)
 *  John Resig  <jresig@mozilla.com> (Original Author)
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

const Ci = Components.interfaces;
const Cc = Components.classes;

const nsISupports = Ci.nsISupports;
const nsIClassInfo = Ci.nsIClassInfo;
const nsIObserver = Ci.nsIObserver;
const fuelIApplication = Ci.fuelIApplication;


//=================================================
// Shutdown - used to store cleanup functions which will
//            be called on Application shutdown
var gShutdown = [];

//=================================================
// Console constructor
function Console() {
  this._console = Components.classes["@mozilla.org/consoleservice;1"]
    .getService(Ci.nsIConsoleService);
}

//=================================================
// Console implementation
Console.prototype = {
  log : function cs_log(aMsg) {
    this._console.logStringMessage(aMsg);
  },
  
  open : function cs_open() {
    var wMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                              .getService(Ci.nsIWindowMediator);
    var console = wMediator.getMostRecentWindow("global:console");
    if (!console) {
      var wWatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                             .getService(Ci.nsIWindowWatcher);
      wWatch.openWindow(null, "chrome://global/content/console.xul", "_blank",
                        "chrome,dialog=no,all", cmdLine);
    } else {
      // console was already open
      console.focus();
    }
  }
};


//=================================================
// EventItem constructor
function EventItem(aType, aData) {
  this._type = aType;
  this._data = aData;
}

//=================================================
// EventItem implementation
EventItem.prototype = {
  _cancel : false,
  
  get type() {
    return this._type;
  },
  
  get data() {
    return this._data;
  },
  
  preventDefault : function ei_pd() {
    this._cancel = true;
  }
};


//=================================================
// Events constructor
function Events() {
  this._listeners = [];
}

//=================================================
// Events implementation
Events.prototype = {
  addListener : function evts_al(aEvent, aListener) {
    if (this._listeners.some(hasFilter))
      return;

    this._listeners.push({
      event: aEvent,
      listener: aListener
    });
    
    function hasFilter(element) {
      return element.event == aEvent && element.listener == aListener;
    }
  },
  
  removeListener : function evts_rl(aEvent, aListener) {
    this._listeners = this._listeners.filter(function(element){
      return element.event != aEvent && element.listener != aListener;
    });
  },
  
  dispatch : function evts_dispatch(aEvent, aEventItem) {
    eventItem = new EventItem(aEvent, aEventItem);
    
    this._listeners.forEach(function(key){
      if (key.event == aEvent) {
        key.listener.handleEvent ?
          key.listener.handleEvent(eventItem) :
          key.listener(eventItem);
      }
    });
    
    return !eventItem._cancel;
  }
};


//=================================================
// Preferences constants
const nsIPrefService = Ci.nsIPrefService;
const nsIPrefBranch = Ci.nsIPrefBranch;
const nsIPrefBranch2 = Ci.nsIPrefBranch2;
const nsISupportsString = Ci.nsISupportsString;

//=================================================
// PreferenceBranch constructor
function PreferenceBranch(aBranch) {
  if (!aBranch)
    aBranch = "";
  
  this._root = aBranch;
  this._prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(nsIPrefService);

  if (aBranch)
    this._prefs = this._prefs.getBranch(aBranch);
    
  this._prefs.QueryInterface(nsIPrefBranch);
  this._prefs.QueryInterface(nsIPrefBranch2);
  
  this._prefs.addObserver(this._root, this, false);
  this._events = new Events();
  
  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

//=================================================
// PreferenceBranch implementation
PreferenceBranch.prototype = {
  // cleanup observer so we don't leak
  _shutdown: function prefs_shutdown() {
    this._prefs.removeObserver(this._root, this);

    this._prefs = null;
    this._events = null;
  },
  
  // for nsIObserver
  observe: function prefs_observe(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed")
      this._events.dispatch("change", aData);
  },

  get root() {
    return this._root;
  },
  
  get all() {
    return this.find({});
  },
  
  get events() {
    return this._events;
  },
  
  // XXX: Disabled until we can figure out the wrapped object issues
  // name: "name" or /name/
  // path: "foo.bar." or "" or /fo+\.bar/
  // type: Boolean, Number, String (getPrefType)
  // locked: true, false (prefIsLocked)
  // modified: true, false (prefHasUserValue)
  find : function prefs_find(aOptions) {
    var retVal = [];
    var items = this._prefs.getChildList("", []);
    
    for (var i = 0; i < items.length; i++) {
      retVal.push(new Preference(items[i], this));
    }

    return retVal;
  },
  
  has : function prefs_has(aName) {
    return (this._prefs.getPrefType(aName) != nsIPrefBranch.PREF_INVALID);
  },
  
  get : function prefs_get(aName) {
    return this.has(aName) ? new Preference(aName, this) : null;
  },

  getValue : function prefs_gv(aName, aValue) {
    var type = this._prefs.getPrefType(aName);
    
    switch (type) {
      case nsIPrefBranch2.PREF_STRING:
        aValue = this._prefs.getComplexValue(aName, nsISupportsString).data;
        break;
      case nsIPrefBranch2.PREF_BOOL:
        aValue = this._prefs.getBoolPref(aName);
        break;
      case nsIPrefBranch2.PREF_INT:
        aValue = this._prefs.getIntPref(aName);
        break;
    }
    
    return aValue;
  },
  
  setValue : function prefs_sv(aName, aValue) {
    var type = aValue != null ? aValue.constructor.name : "";
    
    switch (type) {
      case "String":
        var str = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(nsISupportsString);
        str.data = aValue;
        this._prefs.setComplexValue(aName, nsISupportsString, str);
        break;
      case "Boolean":
        this._prefs.setBoolPref(aName, aValue);
        break;
      case "Number":
        this._prefs.setIntPref(aName, aValue);
        break;
      default:
        throw("Unknown preference value specified.");
    }
  },
  
  reset : function prefs_reset() {
    this._prefs.resetBranch("");
  }
};


//=================================================
// Preference constructor
function Preference(aName, aBranch) {
  this._name = aName;
  this._branch = aBranch;
  this._events = new Events();
  
  var self = this;
  
  this.branch.events.addListener("change", function(aEvent){
    if (aEvent.data == self.name)
      self.events.dispatch(aEvent.type, aEvent.data);
  });
}

//=================================================
// Preference implementation
Preference.prototype = {
  get name() {
    return this._name;
  },
  
  get type() {
    var value = "";
    var type = this._prefs.getPrefType(name);
    
    switch (type) {
      case nsIPrefBranch2.PREF_STRING:
        value = "String";
        break;
      case nsIPrefBranch2.PREF_BOOL:
        value = "Boolean";
        break;
      case nsIPrefBranch2.PREF_INT:
        value = "Number";
        break;
    }
    
    return value;
  },
  
  get value() {
    return this.branch.getValue(this._name, null);
  },
  
  set value(aValue) {
    return this.branch.setValue(this._name, aValue);
  },
  
  get locked() {
    return this.branch._prefs.prefIsLocked(this.name);
  },
  
  set locked(aValue) {
    this.branch._prefs[ aValue ? "lockPref" : "unlockPref" ](this.name);
  },
  
  get modified() {
    return this.branch._prefs.prefHasUserValue(this.name);
  },
  
  get branch() {
    return this._branch;
  },
  
  get events() {
    return this._events;
  },
  
  reset : function pref_reset() {
    this.branch._prefs.clearUserPref(this.name);
  }
};


//=================================================
// SessionStorage constructor
function SessionStorage() {
  this._storage = {};
  this._events = new Events();
}

//=================================================
// SessionStorage implementation
SessionStorage.prototype = {
  get events() {
    return this._events;
  },
  
  has : function ss_has(aName) {
    return this._storage.hasOwnProperty(aName);
  },
  
  set : function ss_set(aName, aValue) {
    this._storage[aName] = aValue;
    this._events.dispatch("change", aName);
  },
  
  get : function ss_get(aName, aDefaultValue) {
    return this.has(aName) ? this._storage[aName] : aDefaultValue;
  }
};


//=================================================
// Extension constants
const nsIUpdateItem = Ci.nsIUpdateItem;

//=================================================
// Extension constructor
function Extension(aItem) {
  this._item = aItem;
  this._firstRun = false;
  this._prefs = new PreferenceBranch("extensions." + this._item.id + ".");
  this._storage = new SessionStorage();
  this._events = new Events();
  
  var installPref = "install-event-fired";
  if (!this._prefs.has(installPref)) {
    this._prefs.setValue(installPref, true);
    this._firstRun = true;
  }

  var os = Components.classes["@mozilla.org/observer-service;1"]
                     .getService(Ci.nsIObserverService);
  os.addObserver(this, "em-action-requested", false);
  
  var self = this;
  gShutdown.push(function(){ self._shutdown(); });
}

//=================================================
// Extensions implementation
Extension.prototype = {
  // cleanup observer so we don't leak
  _shutdown: function ext_shutdown() {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Ci.nsIObserverService);
    os.removeObserver(this, "em-action-requested");

    this._prefs = null;
    this._storage = null;
    this._events = null;
  },
  
  // for nsIObserver  
  observe: function ext_observe(aSubject, aTopic, aData)
  {
    if ((aData == "item-uninstalled") &&
        (aSubject instanceof nsIUpdateItem) &&
        (aSubject.id == this._item.id))
    {
      this._events.dispatch("uninstall", this._item.id);
    }
  },

  get id() {
    return this._item.id;
  },
  
  get name() {
    return this._item.name;
  },
  
  get version() {
    return this._item.version;
  },
  
  get firstRun() {
    return this._firstRun;
  },
  
  get storage() {
    return this._storage;
  },
  
  get prefs() {
    return this._prefs;
  },
  
  get events() {
    return this._events;
  }
};


//=================================================
// Extensions constructor
function Extensions() {
  this._extmgr = Components.classes["@mozilla.org/extensions/manager;1"]
                           .getService(Ci.nsIExtensionManager);
                             
  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

//=================================================
// Extensions implementation
Extensions.prototype = {
  _shutdown : function() {
    this._extmgr = null;
  },
  
  get all() {
    return this.find({});
  },
  
  // XXX: Disabled until we can figure out the wrapped object issues
  // id: "some@id" or /id/
  // name: "name" or /name/
  // version: "1.0.1"
  // minVersion: "1.0"
  // maxVersion: "2.0"
  find : function exts_find(aOptions) {
    var retVal = [];
    var items = this._extmgr.getItemList(nsIUpdateItem.TYPE_EXTENSION, {});
    
    for (var i = 0; i < items.length; i++) {
      retVal.push(new Extension(items[i]));
    }

    return retVal;
  },
  
  has : function exts_has(aId) {
    // getItemForID never returns null for a non-existent id, so we
    // check the type of the returned update item, which should be
    // greater than 1 for a valid extension.
    return !!(this._extmgr.getItemForID(aId).type);
  },
  
  get : function exts_get(aId) {
    return this.has(aId) ? new Extension(this._extmgr.getItemForID(aId)) : null;
  }
};

//=================================================
// Singleton that holds serivces and utilities
var Utilities = {
  _bookmarks : null,
  get bookmarks() {
    if (!this._bookmarks) {
      this._bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                        getService(Ci.nsINavBookmarksService);
    }
    return this._bookmarks;
  },

  _livemarks : null,
  get livemarks() {
    if (!this._livemarks) {
      this._livemarks = Cc["@mozilla.org/browser/livemark-service;2"].
                        getService(Ci.nsILivemarkService);
    }
    return this._livemarks;
  },

  _annotations : null,
  get annotations() {
    if (!this._annotations) {
      this._annotations = Cc["@mozilla.org/browser/annotation-service;1"].
                          getService(Ci.nsIAnnotationService);
    }
    return this._annotations;
  },
  
  _history : null,
  get history() {
    if (!this._history) {
      this._history = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsINavHistoryService);
    }
    return this._history;
  },
  
  _window : null,
  get window() {
    if (!this._window) {
  	  this._window = Cc["@mozilla.org/appshell/window-mediator;1"].
  	                   getService(Components.interfaces.nsIWindowMediator);
  	}
  	return this._window;
  },
  
  specToURI : function(aSpec) {
    if (!aSpec)
      return null;
    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    return ios.newURI(aSpec, null, null);
  },
  
  free : function() {
    this._bookmarks = null;
    this._livemarks = null;
    this._annotations = null;
    this._history = null;
  }
};


//=================================================
// Annotations implementation
function Annotations(aId) {
  this._id = aId;
}

Annotations.prototype = {
  has : function(aName) {
    return Utilities.annotations.itemHasAnnotation(this._id, aName);
  },
  
  get : function(aName) {
    return Utilities.annotations.getItemAnnotationString(this._id, aName);
  },
  
  set : function(aName, aValue, aExpiration) {
    var type = aValue != null ? aValue.constructor.name : "";
    
    switch (type) {
      case "String":
        Utilities.annotations.setItemAnnotationString(this._id, aName, aValue, 0, aExpiration);
        break;
      case "Boolean":
        Utilities.annotations.setItemAnnotationInt32(this._id, aName, aValue, 0, aExpiration);
        break;
      case "Number":
        Utilities.annotations.setItemAnnotationInt32(this._id, aName, aValue, 0, aExpiration);
        break;
      default:
        throw("Unknown annotation value specified.");
    }
  },
    
  remove : function(aName) {
    if (aName == null)
      Utilities.annotations.removeItemAnnotations(this._id);
    else
      Utilities.annotations.removeItemAnnotation(this._id, aName);
  }
};


//=================================================
// Browser implementation
function Browser(aBrowser) {
  this._browser = aBrowser;
  this._events = new Events();
  this._cleanup = {};
  
  this._watch("TabOpen");
  this._watch("TabMove");
  this._watch("TabClose");
  this._watch("TabSelect");
                                 
  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

Browser.prototype = {
  get events() {
    return this._events;
  },
  
  _watch : function(aType) {
  	var self = this;
  	this._browser.mTabContainer.addEventListener(aType, 
		  this._cleanup[aType] = function(e){ self._event(e); },
		  false);
  },
  
  _event : function(aEvent) {
  	this._events.dispatch(aEvent.type, "");
  },
  
  get tabs() {
  	var tabs = [];
  	var browsers = this._browser.browsers;
  	
  	for (var i=0; i<browsers.length; i++)
  		tabs.push(new BrowserTab(this._browser, browsers[i]));
  	
  	return tabs;
  },
  
  get activeTab() {
  	return new BrowserTab(this._browser, this._browser.selectedBrowser);
  },
  
  insertBefore : function(aInsert, aBefore) {
  	this._browser.mTabContainer.insertBefore(aInsert, aBefore);
  },
  
  append : function(aInsert) {
  	this._browser.mTabContainer.appendChild(aInsert);
  },
  
  open : function(aURL) {
  	return new BrowserTab(this._browser, this._browser.addTab(aURL).linkedBrowser);
  },
  
  _shutdown : function() {
  	this._browser = null;
  	this._events = null;
  	
  	for (var type in this._cleanup)
  		this._browser.removeListener(type, this._cleanup[type]);
  		
  	this._cleanup = null;
  }
};


//=================================================
// BrowserTab implementation
function BrowserTab(aBrowser, aBrowserTab) {
  this._browser = aBrowser;
  this._browsertab = aBrowserTab;
  this._events = new Events();
  this._cleanup = {};
  
  this._watch("load");
                                 
  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

BrowserTab.prototype = {
  get url() {
    return this._browsertab.currentURI.spec;
  },
  
  set url(aSpec) {
    return this._browsertab.currentURI.spec = aSpec;
  },
  
  get index() {
  	var tabs = this._browser.mTabContainer.childNodes;
  	for (var i=0; i<tabs.length; i++) {
  	  if (tabs[i].linkedBrowser == this._browsertab)
  	    return i;
  	}
  	return -1;
  },

  get events() {
    return this._events;
  },
  
  get browser() {
  	return this._browser;
  },
  
  get document() {
  	return this._browsertab.content.document;
  },
  
  _watch : function(aType) {
  	var self = this;
  	this._browser.addEventListener(aType,
		  this._cleanup[aType] = function(e){ self._event(e); },
		  false);
  },
  
  _event : function(aEvent) {
  	if (aEvent.type == "load" && (!aEvent.originalTarget instanceof HTMLDocument ||
  		aEvent.originalTarget.defaultView.frameElement))
  		return;
  		
  	this._events.dispatch(aEvent.type, "");
  },
  
  _getTab : function() {
  	var tabs = this._browser.mTabContainer.childNodes;
  	for (var i=0; i<tabs.length; i++) {
  	  if (tabs[i].linkedBrowser == this._browsertab)
  	    return tabs[i];
  	}
  	return null;
  },
  
  focus : function() {
  	this._browser.selectedTab = this._getTab();
  	this._browser.focus();
  },
  
  close : function() {
  	this._browser.removeTab(this.getTab());
  },
  
  _shutdown : function() {
  	this._browser = null;
  	this._browsertab = null;
  	this._events = null;
  	
   	for (var type in this._cleanup)
  		this._browser.removeListener(type, this._cleanup[type]);
  		
  	this._cleanup = null;
  }
};


//=================================================
// Bookmark implementation
function Bookmark(aId, aParent, aType) {
  this._id = aId;
  this._parent = aParent;
  this._type = aType || "unknown";
  this._annotations = new Annotations(this._id);
  this._events = new Events();

  Utilities.bookmarks.addObserver(this, false);  
                                 
  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

Bookmark.prototype = {
  _shutdown : function() {
    this._annotations = null;
    this._events = null;
    
    Utilities.bookmarks.removeObserver(this);  
  },
  
  get title() {
    return Utilities.bookmarks.getItemTitle(this._id);
  },

  set title(aTitle) {
    Utilities.bookmarks.setItemTitle(this._id, aTitle);
  },

  get uri() {
    return Utilities.bookmarks.getBookmarkURI(this._id).spec;
  },

  set uri(aSpec) {
    return Utilities.bookmarks.changeBookmarkURI(this._id, Utilities.specToURI(aSpec));
  },

  get description() {
    return this._annotations.get("bookmarkProperties/description");
  },

  set description(aDesc) {
    this._annotations.set("bookmarkProperties/description", aDesc, Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  get keyword() {
    return Utilities.bookmarks.getKeywordForBookmark(this._id);
  },

  set keyword(aKeyword) {
    Utilities.bookmarks.setKeywordForBookmark(this._id, aKeyword);
  },

  get type() {
    return this._type;
  },

  get parent() {
    return this._parent;
  },
  
  get annotations() {
    return this._annotations;
  },
  
  get events() {
    return this._events;
  },
  
  remove : function() {
    Utilities.bookmarks.removeItem(this._id);
  },
  
  // observer
  onBeginUpdateBatch : function() {
  },

  onEndUpdateBatch : function() {
  },

  onItemAdded : function(id, folder, index) {
    // bookmark object doesn't exist at this point
  },

  onItemRemoved : function(id, folder, index) {
    if (this._id == id)
      this._events.dispatch("remove", "");
  },

  onItemChanged : function(id, property, isAnnotationProperty, value) {
    if (this._id == id)
      this._events.dispatch("change", property);
  },

  onItemVisited: function(id, visitID, time) {
  },

  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex) {
    if (this._id == id)
      this._events.dispatch("move", newParent); // xxx - get new parent
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.fuelIBookmark) ||
        iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Component.result.NS_ERROR_NO_INTERFACE;
  }
}; 


//=================================================
// BookmarkFolder implementation
function BookmarkFolder(aId, aParent) {
  this._id = aId;
  if (this._id == null)
    this._id = Utilities.bookmarks.bookmarksRoot;
  
  this._parent = aParent;
                                 
  this._annotations = new Annotations(this._id);
  this._events = new Events();

  Utilities.bookmarks.addObserver(this, false);  

  var self = this;
  gShutdown.push(function() { self._shutdown(); });
}

BookmarkFolder.prototype = {
  _shutdown : function() {
    this._annotations = null;
    this._events = null;
    
    Utilities.bookmarks.removeObserver(this);  
  },
  
  get title() {
    return Utilities.bookmarks.getItemTitle(this._id);
  },

  set title(aTitle) {
    Utilities.bookmarks.setItemTitle(this._id, aTitle);
  },

  get description() {
    return this._annotations.get("bookmarkProperties/description");
  },

  set description(aDesc) {
    this._annotations.set("bookmarkProperties/description", aDesc, Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  get type() {
    return "folder";
  },

  get parent() {
    return this._parent;
  },

  get annotations() {
    return this._annotations;
  },
  
  get events() {
    return this._events;
  },
  
  get all() {
    var items = [];
    
    var options = Utilities.history.getNewQueryOptions();
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    var query = Utilities.history.getNewQuery();
    query.setFolders([this._id], 1);
    var result = Utilities.history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      if (node.type == node.RESULT_TYPE_FOLDER) {
        var folder = new BookmarkFolder(node.itemId, this._id);
        items.push(folder);
      }
      else if (node.type == node.RESULT_TYPE_SEPARATOR) {
        var separator = new Bookmark(node.itemId, this._id, "separator");
        items.push(separator);
      }
      else {
        var bookmark = new Bookmark(node.itemId, this._id, "bookmark");
        items.push(bookmark);
      }
    }
    rootNode.containerOpen = false;

    return items;
  },
  
  addBookmark : function(aTitle, aSpec) {
    var uri = Utilities.specToURI(aSpec);
    var newBookmarkID = Utilities.bookmarks.insertItem(this._id, uri, Utilities.bookmarks.DEFAULT_INDEX);
    var newBookmark = new Bookmark(newBookmarkID, this, "bookmark");
    newBookmark.title = aTitle;
    return newBookmark;
  },
  
  addLivemark : function(aTitle, aSpec, aFeedSpec) {
    var uri = Utilities.specToURI(aSpec);
    var uriFeed = Utilities.specToURI(aFeedSpec);
    var newBookmarkID = Utilities.livemarks.createLivemark(this._id, uri, uriFeed, Utilities.bookmarks.DEFAULT_INDEX);
    var newBookmark = new Bookmark(newBookmarkID, this, "livemark");
    newBookmark.title = aTitle;
    return newBookmark;
  },

  addFolder : function(aTitle) {
    var newFolderID = Utilities.bookmarks.createFolder(this._id, aTitle, Utilities.bookmarks.DEFAULT_INDEX);
    var newFolder = new BookmarkFolder(newFolderID, this);
    return newFolder;
  },
  
  remove : function() {
    Utilities.bookmarks.removeFolder(this._id);
  },
  
  // observer
  onBeginUpdateBatch : function() {
  },

  onEndUpdateBatch : function() {
  },

  onItemAdded : function(id, folder, index) {
    // handle root folder events
    if (!this._parent)
      this._events.dispatch("add", id);
    
    // handle this folder events  
    if (this._id == folder)
      this._events.dispatch("addchild", id);
  },

  onItemRemoved : function(id, folder, index) {
    // handle root folder events
    if (!this._parent || this._id == id)
      this._events.dispatch("remove", id);

    // handle this folder events      
    if (this._id == folder)
      this._events.dispatch("removechild", id);
  },

  onItemChanged : function(id, property, isAnnotationProperty, value) {
    // handle root folder and this folder events
    if (!this._parent || this._id == id)
      this._events.dispatch("change", property);
  },

  onItemVisited: function(id, visitID, time) {
  },

  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex) {
    // handle root folder and this folder event
    if (!this._parent || this._id == id)
      this._events.dispatch("move", newParent); // xxx - update _parent
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.fuelIBookmarkFolder) ||
        iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Component.result.NS_ERROR_NO_INTERFACE;
  }
}; 


const CLASS_ID = Components.ID("fe74cf80-aa2d-11db-abbd-0800200c9a66");
const CLASS_NAME = "Application wrapper";
const CONTRACT_ID = "@mozilla.org/fuel/application;1";

//=================================================
// Application constructor
function Application() {
  this._console = null;
  this._prefs = null;
  this._storage = null;
  this._events = null;
  this._bookmarks = null;
  
  this._info = Components.classes["@mozilla.org/xre/app-info;1"]
                     .getService(Ci.nsIXULAppInfo);
    
  var os = Components.classes["@mozilla.org/observer-service;1"]
                     .getService(Ci.nsIObserverService);

  os.addObserver(this, "final-ui-startup", false);
  os.addObserver(this, "quit-application-requested", false);
  os.addObserver(this, "quit-application-granted", false);
  os.addObserver(this, "quit-application", false);
  os.addObserver(this, "xpcom-shutdown", false);
}

//=================================================
// Application implementation
Application.prototype = {
  get id() {
    return this._info.ID;
  },
  
  get name() {
    return this._info.name;
  },
  
  get version() {
    return this._info.version;
  },
  
  // for nsIObserver
  observe: function app_observe(aSubject, aTopic, aData) {
    if (aTopic == "app-startup") {
      this._extensions = new Extensions();
      this.events.dispatch("load", "application");
    }
    else if (aTopic == "final-ui-startup") {
      this.events.dispatch("ready", "application");
    }
    else if (aTopic == "quit-application-requested") {
      // we can stop the quit by checking the return value
      if (this.events.dispatch("quit", "application") == false)
        aSubject.data = true;
    }
    else if (aTopic == "xpcom-shutdown") {
      this.events.dispatch("unload", "application");

      // call the cleanup functions and empty the array
      while (gShutdown.length) {
        gShutdown.shift()();
      }

      // release our observers      
      var os = Components.classes["@mozilla.org/observer-service;1"]
                         .getService(Ci.nsIObserverService);

      os.removeObserver(this, "final-ui-startup");

      os.removeObserver(this, "quit-application-requested");
      os.removeObserver(this, "quit-application-granted");
      os.removeObserver(this, "quit-application");
      
      os.removeObserver(this, "xpcom-shutdown");

      this._info = null;
      this._console = null;
      this._prefs = null;
      this._storage = null;
      this._events = null;
      this._extensions = null;
      this._bookmarks = null;
      
      Utilities.free();
    }
  },

  // for nsIClassInfo
  classDescription : "Application",
  classID : CLASS_ID,
  contractID : CONTRACT_ID,
  flags : nsIClassInfo.SINGLETON,
  implementationLanguage : Ci.nsIProgrammingLanguage.JAVASCRIPT,

  getInterfaces : function app_gi(aCount) {
    var interfaces = [fuelIApplication, nsIObserver, nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  getHelperForLanguage : function app_ghfl(aCount) {
    return null;
  },
  
  // for nsISupports
  QueryInterface: function app_qi(aIID) {
    // add any other interfaces you support here
    if (aIID.equals(fuelIApplication) ||
        aIID.equals(nsIObserver) ||
        aIID.equals(nsIClassInfo) ||
        aIID.equals(nsISupports))
    {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  
  get console() {
    if (this._console == null)
        this._console = new Console();

    return this._console;
  },
  
  get storage() {
    if (this._storage == null)
        this._storage = new SessionStorage();

    return this._storage;
  },
  
  get prefs() {
    if (this._prefs == null)
        this._prefs = new PreferenceBranch("");

    return this._prefs;
  },
  
  get extensions() {
    return this._extensions;
  },

  get events() {
    if (this._events == null)
        this._events = new Events();

    return this._events;
  },

  get bookmarks() {
    if (this._bookmarks == null)
        this._bookmarks = new BookmarkFolder(null, null);

    return this._bookmarks;
  },
  
  get browsers() {
  	var win = [], enum = Utilities.window.getEnumerator("navigator:browser");
  	
	while (enum.hasMoreElements())
		win.push(new Browser(enum.getNext().getBrowser()));
	
	return win;
  },
  
  get activeBrowser() {
  	return new Browser(Utilities.window.getMostRecentWindow("navigator:browser").getBrowser());
  }
};

//=================================================
// Factory - Treat Application as a singleton
var gSingleton = null;
var ApplicationFactory = {
  
  createInstance: function af_ci(aOuter, aIID) {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
      
    if (gSingleton == null) {
      gSingleton = new Application();
    }

    return gSingleton.QueryInterface(aIID);
  }
};

//=================================================
// Module
var ApplicationModule = {
  registerSelf: function am_rs(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID, aFileSpec, aLocation, aType);
    
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Ci.nsICategoryManager);
    // make Application a startup observer
    categoryManager.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);

    // add Application as a global property for easy access                                     
    categoryManager.addCategoryEntry("JavaScript global property", "Application", CONTRACT_ID, true, true);
  },

  unregisterSelf: function am_us(aCompMgr, aLocation, aType) {
    aCompMgr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);        

    // cleanup categories
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Ci.nsICategoryManager);
    categoryManager.deleteCategoryEntry("app-startup", "service," + CONTRACT_ID, true);
    categoryManager.deleteCategoryEntry("JavaScript global property", CONTRACT_ID, true);
  },
  
  getClassObject: function am_gco(aCompMgr, aCID, aIID) {
    if (!aIID.equals(Ci.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return ApplicationFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function am_cu(aCompMgr) { return true; }
};

//module initialization
function NSGetModule(aCompMgr, aFileSpec) { return ApplicationModule; }
