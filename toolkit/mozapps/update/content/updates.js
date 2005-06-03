const nsIUpdateItem = Components.interfaces.nsIUpdateItem;

var gCheckingPage = {
  /**
   * The nsIUpdateChecker that is currently checking for updates. We hold onto 
   * this so we can cancel the update check if the user closes the window.
   */
  _checker: null,
  
  /**
   * Starts the update check when the page is shown.
   */
  onPageShow: function() {
    var wiz = document.documentElement;
    wiz.getButton("next").disabled = true;
    
    var aus = Components.classes["@mozilla.org/updates/update-service;1"]
                        .getService(Components.interfaces.nsIApplicationUpdateService);
    this._checker = aus.checkForUpdates(this.updateListener);
  },
  
  /**
   * The user has closed the window, either by pressing cancel or using a Window
   * Manager control, so stop checking for updates.
   */
  onClose: function() {
    if (this._checker)
      this._checker.stopChecking();
  },
  
  updateListener: {
    /**
     * See nsIUpdateCheckListener.idl
     */
    onProgress: function(request, position, totalSize) {
      var pm = document.getElementById("checkingProgress");
      checkingProgress.setAttribute("mode", "normal");
      checkingProgress.setAttribute("value", Math.floor(100 * (position/totalSize)));
    },

    /**
     * See nsIUpdateCheckListener.idl
     */
    onCheckComplete: function(updates, updateCount) {
      gUpdatesAvailablePage.updates = updates;
      document.documentElement.advance();
    },

    /**
     * See nsIUpdateCheckListener.idl
     */
    onError: function() {
      dump("*** UpdateCheckListener: ERROR\n");
    },
    
    /**
     * See nsISupports.idl
     */
    QueryInterface: function(iid) {
      if (!aIID.equals(Components.interfaces.nsIUpdateCheckListener) &&
          !aIID.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  }
};

var gUpdatesAvailablePage = {
  updates: null,
  _incompatibleItems: null,
  
  onPageShow: function() {
    var newestUpdate = this.updates[0];
    var vc = Components.classes["@mozilla.org/updates/version-checker;1"]
                       .createInstance(Components.interfaces.nsIVersionChecker);
    for (var i = 0; i < this.updates.length; ++i) {
      if (vc.compare(this.updates[i].version, newestUpdate.version) > 0)
        newestVersion = this.updates[i];
    }
    
    var updateStrings = document.getElementById("updateStrings");
    var brandStrings = document.getElementById("brandStrings");
    var brandName = brandStrings.getString("brandShortName");
    var updateName = updateStrings.getFormattedString("updateName", 
                                                      [brandName, newestVersion.version]);
    var updateNameElement = document.getElementById("updateName");
    updateNameElement.value = updateName;
    
    var displayType = updateStrings.getString("updateType_" + newestVersion.type);
    var updateTypeElement = document.getElementById("updateType");
    updateTypeElement.setAttribute("type", newestVersion.type);
    var intro = updateStrings.getFormattedString("introType_" + newestVersion.type, [brandName]);
    while (updateTypeElement.hasChildNodes())
      updateTypeElement.removeChild(updateTypeElement.firstChild);
    updateTypeElement.appendChild(document.createTextNode(intro));
    
    var updateMoreInfoURL = document.getElementById("updateMoreInfoURL");
    updateMoreInfoURL.href = newestVersion.detailsurl;
    
    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    var items = em.getIncompatibleItemList("", newestVersion.version,
                                           nsIUpdateItem.TYPE_ADDON, { });
    if (items.length > 0) {
      // There are addons that are incompatible with this update, so show the 
      // warning message.
      var incompatibleWarning = document.getElementById("incompatibleWarning");
      incompatibleWarning.hidden = false;
      
      this._incompatibleItems = items;
    }
  },
  
  showIncompatibleItems: function() {
    openDialog("chrome://mozapps/content/update/incompatible.xul", "", 
               "dialog,centerscreen,modal,resizable,titlebar", this._incompatibleItems);
  }
};

