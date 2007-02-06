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
}

//=================================================
// Console implementation
Console.prototype = {
  _console : null,
  
  _init : function() {
    this._console = Components.classes['@mozilla.org/consoleservice;1']
                              .getService(Components.interfaces.nsIConsoleService);
  },
  
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
  _type : "",
  _data : "",
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
}

//=================================================
// Events implementation
Events.prototype = {
  _init : function() {
  	this._listeners = [];
  },
  
  add : function(event, handler) {
    function hasFilter(element, index, array) {
      return (element.event == event && element.handler == handler);
    }
    var hasHandler = this._listeners.some(hasFilter);
    if (hasHandler)
      return;

    var key = {};
    key.event = event;
    key.handler = handler;

    this._listeners.push(key);
  },
  
  remove : function(event, handler) {
    function removeFilter(element, index, array) {
      return (element.event != event && element.handler != handler);
    }
    var filtered = this._listeners.filter(removeFilter);
    this._listeners = filtered;
  },
  
  fire : function(event, eventItem) {
    for (var i=0; i<this._listeners.length; i++) {
      var key = this._listeners[i];
      if (key.event == event) {
        key.handler.handleEvent(eventItem);
      }
    }
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
function Preferences() {
}

//=================================================
// Preferences implementation
Preferences.prototype = {
  _prefs : null,
  _events : null,
  
  _init : function(branch) {
    this._prefs = Components.classes['@mozilla.org/preferences-service;1']
                            .getService(nsIPrefService);

    if (branch) {
      this._prefs = this._prefs.getBranch(branch);
    }
    
    this._prefs.QueryInterface(nsIPrefBranch);
    this._prefs.QueryInterface(nsIPrefBranch2);
    
    this._events = new Events();
    this._events._init();

    this._prefs.addObserver("", this, false);
  },

  // for nsIObserver
  observe: function(subject, topic, data) {
    if (topic == "nsPref:changed") {
      var evt = new EventItem("change", data);
      this._events.fire("change", evt);
    }
  },
  
  get events() {
    return this._events;
  },
  
  set : function(name, value) {
    var type = nsIPrefBranch2.PREF_STRING;

    try {
      switch (type) {
        case nsIPrefBranch2.PREF_STRING:
          var str = Components.classes['@mozilla.org/supports-string;1']
                              .createInstance(nsISupportsString);
          str.data = value;
          this._prefs.setComplexValue(name, nsISupportsString, str);
          break;
        case nsIPrefBranch2.PREF_BOOL:
          this._prefs.setBoolPref(name, value);
          break;
        case nsIPrefBranch2.PREF_INT:
          this._prefs.setIntPref(name, value);
          break;
        default:
          throw("No pref type found/specified!\n");
      }
    } catch(ex) {
      dump("ERROR: Unable to write pref \"" + name + "\".\n" + ex + "\n");
    }
  },
  
  get : function(name, defaultValue) {
    var value = defaultValue;
    
    var type = this._prefs.getPrefType(name);
    try {
      switch (type) {
        case nsIPrefBranch2.PREF_STRING:
          value = this._prefs.getComplexValue(name, nsISupportsString).data;
          break;
        case nsIPrefBranch2.PREF_BOOL:
          value = this._prefs.getBoolPref(name);
          break;
        case nsIPrefBranch2.PREF_BOOL:
          value = this._prefs.getIntPref(name);
          break;
        default:
          throw("No pref type found/specified!\n");
      }
    } catch(ex) {
      dump("ERROR: Unable to read pref \"" + name + "\".\n" + ex + "\n");
    }
    return value;
  }
};


//=================================================
// SessionStorage constructor
function SessionStorage() {
}

//=================================================
// SessionStorage implementation
SessionStorage.prototype = {
  _storage : {},
  _events : null,
  
  _init : function() {
    this._events = new Events();
    this._events._init();
  },
  
  get events() {
    return this._events;
  },
  
  has : function(name) {
    return this._storage.hasOwnProperty(name);
  },
  
  set : function(name, value) {
    this._storage[name] = value;

    var evt = new EventItem("change", name);
    this._events.fire("change", evt);
  },
  
  get : function(name, defaultValue) {
    var value = defaultValue;
    
    if (this.has(name)) {
      value = this._storage[name];
    }    
    return value;
  }
};


//=================================================
// Extension constructor
function Extension() {
}

//=================================================
// Extensions implementation
Extension.prototype = {
  _item : null,
  _prefs : null,
  _storage : null,
  _events : null,
  
  _init : function(item) {
    this._item = item;

    var domain = "extensions." + _item.id + ".";
    this._prefs = new Preferences();
    this._prefs._init(domain);

    this._storage = new SessionStorage();
    this._storage._init();

    this._events = new Events();
    this._events._init();
    
    var installedPref = "install-event-fired";
    if (this._prefs.has(installPref) == false) {
      this._prefs.set(installPref, true);

      var evt = new EventItem("install", this._item.id);
      this._events.fire("install", evt);
    }
  },
  
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
}

//=================================================
// Extensions implementation
Extensions.prototype = {
  _extmgr : null,
  
  _init : function() {
    this._extmgr = Components.classes["@mozilla.org/extensions/manager;1"]
                             .getService(Components.interfaces.nsIExtensionManager);
  },
  
  find : function(findKeys) {
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
    return (this._extmgr != null && this._extmgr.getItemForID(id) != null);
  },
  
  get : function(id) {
    var extension = null;

    if (this._extmgr) {
      var item = this._extmgr.getItemForID(id);
      if (item) {
        extension = new Extension();
        extension._init(item);
      }
    }
    return extension;
  }
};


const CLASS_ID = Components.ID("fe74cf80-aa2d-11db-abbd-0800200c9a66");
const CLASS_NAME = "Application wrapper";
const CONTRACT_ID = "@mozilla.org/application;1";

//=================================================
// Application constructor
function Application() {
}

//=================================================
// Application implementation
Application.prototype = {
  _console : null,
  _storage : null,
  _prefs : null,
  _extensions : null,
  _events : null,
  toolbars : [],
  
  _init : function() {
    this._console = new Console();
    this._console._init();

    this._prefs = new Preferences();
    this._prefs._init();

    this._storage = new SessionStorage();
    this._storage._init();

    this._events = new Events();
    this._events._init();
    
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);

    os.addObserver(this, "final-ui-startup", false);

    os.addObserver(this, "quit-application-requested", false);
    os.addObserver(this, "quit-application-granted", false);
    os.addObserver(this, "quit-application", false);
    os.addObserver(this, "xpcom-shutdown", false);
/*    
    var idleServ = Components.classes["@mozilla.org/widget/idleservice;1"]
                             .getService(Components.interfaces.nsIIdleService);

    idleServ.addIdleObserver(this, 0);
*/    
  },
  
  // for nsIObserver
  observe: function(subject, topic, data) {
    if (topic == "app-startup") {
      this._extensions = new Extensions();
      this._extensions._init();
      
      var evt = new EventItem("start", "application");
      this._events.fire("start", evt);
    }
    else if (topic == "final-ui-startup") {
      var evt = new EventItem("ready", "application");
      this._events.fire("ready", evt);
    }
    else if (topic == "quit-application-requested") {
      var evt = new EventItem("quit", "application");
      this._events.fire("quit", evt);
      // we can stop the quit by checking the evt._cancel
      if (evt._cancel) {
        data.value = true;
      }
    }
    else if (topic == "xpcom-shutdown") {
      var evt = new EventItem("unload", "application");
      this._events.fire("unload", evt);

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
      this.singleton._init();
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
