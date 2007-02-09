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
 * The Initial Developer of the Original Code is Mozilla.
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
 
const nsISupports = Components.interfaces.nsISupports;
const nsIClassInfo = Components.interfaces.nsIClassInfo;
const nsIObserver = Components.interfaces.nsIObserver;
const nsIApplication = Components.interfaces.nsIApplication;


//=================================================
// Console constructor
function Console() {
  this._console = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
}

//=================================================
// Console implementation
Console.prototype = {
  log : function(msg) {
    this._console.logStringMessage(msg);
  }
};


//=================================================
// EventItem constructor
function EventItem(type, data) {
  this._type = type;
  this._data = data;
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
  
  preventDefault : function() {
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
  add : function(event, handler) {
	if (this._listeners.some(hasFilter))
      return;

    this._listeners.push({
    	event: event,
    	handler: handler
    });
    
    function hasFilter(element) {
      return element.event == event && element.handler == handler;
    }
  },
  
  remove : function(event, handler) {
    this._listeners = this._listeners.filter(function(element){
      return element.event != event && element.handler != handler;
    });
  },
  
  fire : function(event, eventItem) {
  	eventItem = new EventItem( event, eventItem );
  	
  	this._listeners.forEach(function(key){
      if (key.event == event) {
        key.handler.handleEvent(eventItem);
      }
    });
    
    return !eventItem._cancel;
  }
};


//=================================================
// Preferences constants
const nsIPrefService = Components.interfaces.nsIPrefService;
const nsIPrefBranch = Components.interfaces.nsIPrefBranch;
const nsIPrefBranch2 = Components.interfaces.nsIPrefBranch2;
const nsISupportsString = Components.interfaces.nsISupportsString;

//=================================================
// Preferences constructor
function Preferences( branch ) {
  this._prefs = Components.classes['@mozilla.org/preferences-service;1']
                          .getService(nsIPrefService);

  if (branch) {
    this._prefs = this._prefs.getBranch(branch);
  }
    
  this._prefs.QueryInterface(nsIPrefBranch);
  this._prefs.QueryInterface(nsIPrefBranch2);
    
  this._events = new Events();

  this._prefs.addObserver("", this, false);
}

//=================================================
// Preferences implementation
Preferences.prototype = {
  // for nsIObserver
  observe: function(subject, topic, data) {
    if (topic == "nsPref:changed") {
      this._events.fire("change", data);
    }
  },
  
  get events() {
    return this._events;
  },
  
  find : function(findKeys) {
    // XXX need to implement
    return [];
  },
  
  set : function(name, value) {
    var type = value != null ? value.constructor.name : "";
    
    switch (type) {
      case "String":
        var str = Components.classes['@mozilla.org/supports-string;1']
                            .createInstance(nsISupportsString);
        str.data = value;
        this._prefs.setComplexValue(name, nsISupportsString, str);
        break;
      case "Boolean":
        this._prefs.setBoolPref(name, value);
        break;
      case "Number":
        this._prefs.setIntPref(name, value);
        break;
      default:
        throw("Unknown preference value specified.");
    }
    
    return value;
  },
  
  get : function(name, value) {
    var type = this._prefs.getPrefType(name);
    
    switch (type) {
      case nsIPrefBranch2.PREF_STRING:
        value = this._prefs.getComplexValue(name, nsISupportsString).data;
        break;
      case nsIPrefBranch2.PREF_BOOL:
        value = this._prefs.getBoolPref(name);
        break;
      case nsIPrefBranch2.PREF_INT:
        value = this._prefs.getIntPref(name);
        break;
    }
    
    return value;
  },
  
  reset : function(name) {
    if (name) {
      this._prefs.clearUserPref(name);
    }
    else {
      this._prefs.resetBranch("");
    }
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
  
  has : function(name) {
    return this._storage.hasOwnProperty(name);
  },
  
  set : function(name, value) {
    this._storage[name] = value;
    this._events.fire("change", name);
  },
  
  get : function(name, defaultValue) {
    return this.has(name) && this._storage[name] || defaultValue;
  }
};


//=================================================
// Extension constructor
function Extension( item ) {
  this._item = item;
  this._prefs = new Preferences( "extensions." + _item.id + "." );
  this._storage = new SessionStorage();
  this._events = new Events();
  
  var installPref = "install-event-fired";
  if ( !this._prefs.has(installPref) ) {
    this._prefs.set(installPref, true);
    this._events.fire("install", this._item.id);
  }
}

//=================================================
// Extensions implementation
Extension.prototype = {
  get id() {
    return this._item.id;
  },
  
  get name() {
    return this._item.name;
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
                           .getService(Components.interfaces.nsIExtensionManager);
}

//=================================================
// Extensions implementation
Extensions.prototype = {
  find : function(findKeys) {
    // XXX need to implement
    var extension = null;
  /*
    var items = this._extmgr.getItemList(type, { });
    for (var i=0; i<items.length; i++) {
      if (items[i].id == id)
        return true;
    }
  */
    return extension;
  },
  
  has : function(id) {
    return !!(this._extmgr && this._extmgr.getItemForID(id));
  },
  
  get : function(id) {
    return this.has(id) && new Extension(id) || null;
  }
};


const CLASS_ID = Components.ID("fe74cf80-aa2d-11db-abbd-0800200c9a66");
const CLASS_NAME = "Application wrapper";
const CONTRACT_ID = "@mozilla.org/application;1";

//=================================================
// Application constructor
function Application() {
  this._console = new Console();
  this._prefs = new Preferences();
  this._storage = new SessionStorage();
  this._events = new Events();
    
  var os = Components.classes["@mozilla.org/observer-service;1"]
                     .getService(Components.interfaces.nsIObserverService);

  os.addObserver(this, "final-ui-startup", false);
  os.addObserver(this, "quit-application-requested", false);
  os.addObserver(this, "quit-application-granted", false);
  os.addObserver(this, "quit-application", false);
  os.addObserver(this, "xpcom-shutdown", false);
  
/* XXX To implement 
  var idleServ = Components.classes["@mozilla.org/widget/idleservice;1"]
                           .getService(Components.interfaces.nsIIdleService);

  idleServ.addIdleObserver(this, 0);
*/
}

//=================================================
// Application implementation
Application.prototype = {
  // for nsIObserver
  observe: function(subject, topic, data) {
    if (topic == "app-startup") {
      this._extensions = new Extensions();
      this._events.fire("start", "application");
    }
    else if (topic == "final-ui-startup") {
      this._events.fire("ready", "application");
    }
    else if (topic == "quit-application-requested") {
      // we can stop the quit by checking the return value
      if (this._events.fire("quit", "application")) {
        data.value = true;
      }
    }
    else if (topic == "xpcom-shutdown") {
      this._events.fire("unload", "application");

      var os = Components.classes["@mozilla.org/observer-service;1"]
                         .getService(Components.interfaces.nsIObserverService);

      os.removeObserver(this, "final-ui-startup");

      os.removeObserver(this, "quit-application-requested");
      os.removeObserver(this, "quit-application-granted");
      os.removeObserver(this, "quit-application");
      
      os.removeObserver(this, "xpcom-shutdown");
    }
  },

  // for nsIClassInfo
  classDescription : "Application",
  classID : CLASS_ID,
  contractID : CONTRACT_ID,
  flags : nsIClassInfo.SINGLETON,
  implementationLanguage : Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,

  getInterfaces : function(count) {
    var interfaces = [nsIApplication, nsIObserver, nsIClassInfo];
    count.value = interfaces.length;
    return interfaces;
  },

  getHelperForLanguage : function(count) {
    return null;
  },
  
  // for nsISupports
  QueryInterface: function(aIID) {
    // add any other interfaces you support here
    if (!aIID.equals(nsIApplication) &&
        !aIID.equals(nsIObserver) &&
        !aIID.equals(nsIClassInfo) &&
        !aIID.equals(nsISupports))
    {
        throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  
  get console() {
    return this._console;
  },
  
  get storage() {
    return this._storage;
  },
  
  get prefs() {
    return this._prefs;
  },
  
  get extensions() {
    return this._extensions;
  },

  get events() {
    return this._events;
  }
}

//=================================================
// Factory - Treat Application as a singleton
var ApplicationFactory = {
  singleton: null,
  
  createInstance: function(aOuter, aIID)
  {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
      
    if (this.singleton == null) {
      this.singleton = new Application();
    }
    return this.singleton.QueryInterface(aIID);
  }
};

//=================================================
// Module
var ApplicationModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID, aFileSpec, aLocation, aType);
    
    // make the Update Service a startup observer
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Components.interfaces.nsICategoryManager);
    categoryManager.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);

    // add Application as a global property for easy access                                     
    categoryManager.addCategoryEntry("JavaScript global property", "Application", CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);        
  },
  
  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return ApplicationFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

//module initialization
function NSGetModule(aCompMgr, aFileSpec) { return ApplicationModule; }
