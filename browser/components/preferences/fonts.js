# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
# 
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
# 
# The Original Code is the Firefox Preferences System.
# 
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
# 
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
# 
# ***** END LICENSE BLOCK *****

# browser.display.languageList LOCK ALL when LOCKED

const kFontNameFmtSerif         = "font.name.serif.%LANG%";
const kFontNameFmtSansSerif     = "font.name.sans-serif.%LANG%";
const kFontNameFmtMonospace     = "font.name.monospace.%LANG%";
const kFontNameListFmtSerif     = "font.name-list.serif.%LANG%";
const kFontNameListFmtSansSerif = "font.name-list.sans-serif.%LANG%";
const kFontNameListFmtMonospace = "font.name-list.monospace.%LANG%";
const kFontSizeFmtVariable      = "font.size.variable.%LANG%";
const kFontSizeFmtFixed         = "font.size.fixed.%LANG%";
const kFontMinSizeFmt           = "font.minimum-size.%LANG%";

var gFontsDialog = {
  init: function ()
  {
  },
  
  _selectLanguageGroup: function (aLanguageGroup)
  {
    var prefs = [{ format: kFontNameFmtSerif,         type: "unichar", element: "serif",      fonttype: "serif"       },
                 { format: kFontNameFmtSansSerif,     type: "unichar", element: "sans-serif", fonttype: "sans-serif"  },
                 { format: kFontNameFmtMonospace,     type: "unichar", element: "monospace",  fonttype: "monospace"   },
                 { format: kFontNameListFmtSerif,     type: "unichar", element: "serif",      fonttype: "serif"       },
                 { format: kFontNameListFmtSansSerif, type: "unichar", element: "sans-serif", fonttype: "sans-serif"  },
                 { format: kFontNameListFmtMonospace, type: "unichar", element: "monospace",  fonttype: "monospace"   },
                 { format: kFontSizeFmtVariable,      type: "int",     element: "sizeVar",    fonttype: null          },
                 { format: kFontSizeFmtFixed,         type: "int",     element: "sizeMono",   fonttype: null          },
                 { format: kFontMinSizeFmt,           type: "int",     element: "minSize",    fonttype: null          }];
    var preferences = document.getElementById("fontPreferences");
    for (var i = 0; i < prefs.length; ++i) {
      var preference = document.getElementById(prefs[i].format.replace(/%LANG%/, aLanguageGroup));
      if (!preference) {
        preference = document.createElement("preference");
        var name = prefs[i].format.replace(/%LANG%/, aLanguageGroup);
        preference.id = name;
        preferences.appendChild(preference);
        preference.name = name;
        preference.type = prefs[i].type;
      }
      
      if (!prefs[i].element)
        continue;
        
      var element = document.getElementById(prefs[i].element);
      if (element) {
        element.setAttribute("preference", preference.id);
      
        if (prefs[i].fonttype)
          FontBuilder.buildFontList(aLanguageGroup, prefs[i].fonttype, element);
      
        preference.setElementValue(element);
      }
    }
  },
  
  readFontLanguageGroup: function ()
  {
    var languagePref = document.getElementById("font.language.group");
    this._selectLanguageGroup(languagePref.value);
    return undefined;
  },
  
  readFontSelection: function (aElement)
  {
    // Determine the appropriate value to select, for the following cases:
    // - there is no setting 
    // - the font selected by the user is no longer present (e.g. deleted from
    //   fonts folder)
    var preference = document.getElementById(aElement.getAttribute("preference"));
    if (preference.value) {
      dump("*** pv = |" + preference.value + "|\n");
      var fontItems = aElement.getElementsByAttribute("value", preference.value);
    
      // There is a setting that actually is in the list. Respect it.
      if (fontItems.length > 0)
        return undefined;
    }
    dump("*** awww sheeit\n");
    for (var i = 0; i < aElement.firstChild.childNodes.length; ++i) {
      // dump("*** curr = |" + aElement.firstChild.childNodes[i].getAttribute("value") + "| sought = |"+ preference.value+"|\n");
    }
    
    var defaultValue = aElement.firstChild.firstChild.getAttribute("value");
    var languagePref = document.getElementById("font.language.group");
    preference = document.getElementById("font.name-list." + aElement.id + "." + languagePref.value);
    if (!preference || !preference.hasUserValue)
      return defaultValue;
    
    var fontNames = preference.value.split(",");
    var stripWhitespace = /^\s*(.*)\s*$/;
    
    for (var i = 0; i < fontNames.length; ++i) {
      var fontName = fontNames[i].replace(stripWhitespace, "$1");
      fontItems = aElement.getElementsByAttribute("value", fontName);
      if (fontItems.length)
        break;
    }
    if (fontItems.length)
      return fontItems[0].getAttribute("value");
    return defaultValue;
  },    
};

var FontBuilder = {
  _enumerator: null,
  get enumerator ()
  {
    if (!this._enumerator) {
      this._enumerator = Components.classes["@mozilla.org/gfx/fontenumerator;1"]
                                   .createInstance(Components.interfaces.nsIFontEnumerator);
    }
    return this._enumerator;
  },

  _allFonts: null,
  buildFontList: function (aLanguage, aFontType, aMenuList) 
  {
    // Reset the list
    while (aMenuList.hasChildNodes())
      aMenuList.removeChild(aMenuList.firstChild);
    
    var defaultFont = null;
    // Load Font Lists
    var fonts = this.enumerator.EnumerateFonts(aLanguage, aFontType, { } );
    if (fonts.length > 0)
      defaultFont = this.enumerator.getDefaultFont(aLanguage, aFontType);
    else {
      fonts = this.enumerator.EnumerateFonts(aLanguage, "", { });
      if (fonts.length > 0)
        defaultFont = this.enumerator.getDefaultFont(aLanguage, "");
    }
    
    if (!this._allFonts)
      this._allFonts = this.enumerator.EnumerateAllFonts({});
    
    // Build the UI for the Default Font and Fonts for this CSS type.
    var popup = document.createElement("menupopup");
    var separator;
    if (fonts.length > 0) {
      if (defaultFont) {
        var bundlePreferences = document.getElementById("bundlePreferences");
        var label = bundlePreferences.getFormattedString("labelDefaultFont", [defaultFont]);
        var menuitem = document.createElement("menuitem");
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("value", ""); // Default Font has a blank value
        popup.appendChild(menuitem);
        
        separator = document.createElement("menuseparator");
        popup.appendChild(separator);
      }
      
      for (var i = 0; i < fonts.length; ++i) {
        menuitem = document.createElement("menuitem");
        menuitem.setAttribute("value", fonts[i]);
        menuitem.setAttribute("label", fonts[i]);
        popup.appendChild(menuitem);
      }
    }
    
    // Build the UI for the remaining fonts. 
    if (this._allFonts.length > fonts.length) {
      // Both lists are sorted, and the Fonts-By-Type list is a subset of the
      // All-Fonts list, so walk both lists side-by-side, skipping values we've
      // already created menu items for. 
      var builtItem = separator ? separator.nextSibling : popup.firstChild;
      
      separator = document.createElement("menuseparator");
      popup.appendChild(separator);
      
      for (i = 0; i < this._allFonts.length; ++i) {
        if (this._allFonts[i] != builtItem.getAttribute("value")) {
          menuitem = document.createElement("menuitem");
          menuitem.setAttribute("value", this._allFonts[i]);
          menuitem.setAttribute("label", this._allFonts[i]);
          popup.appendChild(menuitem);
        }
        else
          builtItem = builtItem.nextSibling;
      }
    }
    aMenuList.appendChild(popup);    
  },
};

  
/*  
  
function Startup()
  {
    // Initialize the sub-dialog  
    variableSize = document.getElementById( "sizeVar" );
    fixedSize    = document.getElementById( "sizeMono" );
    minSize      = document.getElementById( "minSize" );
    languageList = document.getElementById( "selectLangs" );

    gPrefutilitiesBundle = document.getElementById("bundle_prefutilities");

    // eventually we should detect the default language and select it by default
    selectLanguage();
    
    // Set up the labels for the standard issue resolutions
    var resolution = document.getElementById( "screenResolution" );

    // Set an attribute on the selected resolution item so we can fall back on
    // it if an invalid selection is made (select "Other...", hit Cancel)
    resolution.selectedItem.setAttribute("current", "true");

    var defaultResolution = "96";
    var otherResolution = "72";

    var dpi = resolution.getAttribute( "dpi" );
    resolution = document.getElementById( "defaultResolution" );
    resolution.setAttribute( "value", defaultResolution );
    resolution.setAttribute( "label", dpi.replace(/\$val/, defaultResolution ) );
    resolution = document.getElementById( "otherResolution" );
    resolution.setAttribute( "value", otherResolution );
    resolution.setAttribute( "label", dpi.replace(/\$val/, otherResolution ) );

    // Get the pref and set up the dialog appropriately. Startup is called
    // after SetFields so we can't rely on that call to do the business.
    var prefvalue = gPrefWindow.getPref( "int", "browser.display.screen_resolution" );
    if( prefvalue != "!/!ERROR_UNDEFINED_PREF!/!" )
        resolution = prefvalue;
    else
        resolution = 96; // If it all goes horribly wrong, fall back on 96.
    
    setResolution( resolution );

    // This prefstring is a contrived pref whose sole purpose is to lock some
    // elements in this panel.  The value of the pref is not used and does not matter.
    if ( gPrefWindow.getPrefIsLocked( "browser.display.languageList" ) ) {
      disableAllFontElements();
    }
  }
  
function lazyAppendFontNames( i )
  {
     // schedule the build of the next font list
     if (i+1 < fontTypes.length)
       {
         window.setTimeout(lazyAppendFontNames, 100, i+1);
       }

     // now build and populate the fonts for the requested font type
     var defaultItem = null;
     var selectElement = new listElement( fontTypes[i] );
     selectElement.clearList();
     try
       {
         defaultItem = selectElement.appendFontNames( languageList.value, fontTypes[i] );
       }
     catch(e) {
         dump("pref-fonts.js: " + e + "\nFailed to build the font list for " + fontTypes[i] + "\n");
         return;
       }

     // now set the selected font item for the drop down list

     if (!defaultItem)
       return; // nothing to select, so no need to bother

     // the item returned by default is our last resort fall-back
     var selectedItem = defaultItem;
     if( languageList.value in languageData )
       {
         // data exists for this language, pre-select items based on this information
         var dataVal = languageData[languageList.value].types[fontTypes[i]];
         if (!dataVal.length) // special blank means the default
           {
             selectedItem = defaultItem;
           }
         else
           {
             var dataEls = selectElement.listElement.getElementsByAttribute( "value", dataVal );
             selectedItem = dataEls.item(0) ? dataEls[0] : defaultItem;
           }
       }
     else
       {
         try
           {
             var fontPrefString = "font.name." + fontTypes[i] + "." + languageList.value;
             var selectVal = gPrefWindow.pref.CopyUnicharPref( fontPrefString );
             var dataEls = selectElement.listElement.getElementsByAttribute( "value", selectVal );

             // we need to honor name-list in case name is unavailable 
             if (!dataEls.item(0)) {
                 var fontListPrefString = "font.name-list." + fontTypes[i] + "." + languageList.value;
                 var nameList = gPrefWindow.pref.CopyUnicharPref( fontListPrefString );
                 var fontNames = nameList.split(",");
                 var stripWhitespace = /^\s*(.*)\s*$/;

                 for (j = 0; j < fontNames.length; j++) {
                   selectVal = fontNames[j].replace(stripWhitespace, "$1");
                   dataEls = selectElement.listElement.getElementsByAttribute("value", selectVal);
                   if (dataEls.item(0))  
                     break;  // exit loop if we find one
                 }
             }
             selectedItem = dataEls.item(0) ? dataEls[0] : defaultItem;
           }
         catch(e) {
             selectedItem = defaultItem;
           }
       }

     selectElement.listElement.selectedItem = selectedItem;
     selectElement.listElement.removeAttribute( "disabled" );
  }

function saveState()
  {
    for( var i = 0; i < fontTypes.length; i++ )
      {
        // preliminary initialisation
        if( currentLanguage && !( currentLanguage in languageData ) )
          languageData[currentLanguage] = [];
        if( currentLanguage && !( "types" in languageData[currentLanguage] ) )
          languageData[currentLanguage].types = [];
        // save data for the previous language
        if( currentLanguage && currentLanguage in languageData &&
            "types" in languageData[currentLanguage] )
          languageData[currentLanguage].types[fontTypes[i]] = document.getElementById( fontTypes[i] ).value;
      }

    if( currentLanguage && currentLanguage in languageData &&
        "types" in languageData[currentLanguage] )
      {
        languageData[currentLanguage].variableSize = parseInt( variableSize.value );
        languageData[currentLanguage].fixedSize = parseInt( fixedSize.value );
        languageData[currentLanguage].minSize = parseInt( minSize.value );
      }
  }

// Selects size (or the nearest entry that exists in the list)
// in the menulist minSize
function minSizeSelect(size)
  {
    var items = minSize.getElementsByAttribute( "value", size );
    if (items.item(0))
      minSize.selectedItem = items[0];
    else if (size < 6)
      minSizeSelect(6);
    else if (size > 24)
      minSizeSelect(24);
    else
      minSizeSelect(size - 1);
  }

function selectLanguage()
  {
    // save current state
    saveState();

    if( currentLanguage == languageList.value )
      return; // same as before, nothing changed
    
    currentLanguage = languageList.value;

    // lazily populate the successive font lists at 100ms intervals.
    // (Note: the third parameter to setTimeout() is going to be
    // passed as argument to the callback function.)
    window.setTimeout(lazyAppendFontNames, 100, 0);

    // in the meantime, disable the menu lists
    for( var i = 0; i < fontTypes.length; i++ )
      {
        var listElement = document.getElementById( fontTypes[i] );
        listElement.setAttribute( "value", "" );
        listElement.setAttribute( "label", "" );
        listElement.setAttribute( "disabled", "true" );
      }

    // and set the font sizes
    var dataObject = gPrefWindow.wsm.dataManager.pageData["chrome://browser/content/pref/pref-fonts.xul"].userData;
    var langData = null;
    try
      {
        var sizeVarVal, sizeFixedVal;
        if ('languageData' in dataObject) {
          langData = dataObject.languageData[currentLanguage];
          sizeVarVal = langData.variableSize;
          sizeFixedVal = langData.fixedSize;
        }
        else {          
          var variableSizePref = "font.size.variable." + languageList.value;
          sizeVarVal = gPrefWindow.pref.GetIntPref( variableSizePref );
          var fixedSizePref = "font.size.fixed." + languageList.value;
          sizeFixedVal = gPrefWindow.pref.GetIntPref( fixedSizePref );
        }
        variableSize.selectedItem = variableSize.getElementsByAttribute( "value", sizeVarVal )[0];
        fixedSize.selectedItem = fixedSize.getElementsByAttribute( "value", sizeFixedVal )[0];
      }
    catch(e) { } // font size lists can simply default to the first entry

    var minSizeVal = 0;
    try 
      {
        if (langData) {
          minSizeVal = langData.minSize;
        }
        else {        
          var minSizePref = "font.minimum-size." + languageList.value;
          minSizeVal = gPrefWindow.pref.GetIntPref( minSizePref );
        }
      }
    catch(e) { }
    minSizeSelect( minSizeVal );
  }

function changeScreenResolution()
  {
    var screenResolution = document.getElementById("screenResolution");
    var userResolution = document.getElementById("userResolution");

    var previousSelection = screenResolution.getElementsByAttribute("current", "true")[0];

    if (screenResolution.value == "other")
      {
        // If the user selects "Other..." we bring up the calibrate screen dialog
        var rv = { newdpi : 0 };
        var calscreen = window.openDialog("chrome://browser/content/pref/pref-calibrate-screen.xul", 
                                      "_blank", 
                                      "modal,chrome,centerscreen,resizable=no,titlebar",
                                      rv);
        if (rv.newdpi != -1) 
          {
            // They have entered values, and we have a DPI value back
            setResolution ( rv.newdpi );
            previousSelection.removeAttribute("current");
            screenResolution.selectedItem.setAttribute("current", "true");
          }
        else
          {
            // They've cancelled. We can't leave "Other..." selected, so...
            // we re-select the previously selected item.
            screenResolution.selectedItem = previousSelection;
          }
      }
    else if (!(screenResolution.value == userResolution.value))
      {
        // User has selected one of the hard-coded resolutions
        userResolution.setAttribute("hidden", "true");

        previousSelection.removeAttribute("current");
        screenResolution.selectedItem.setAttribute("current", "true");
      }
  }

function setResolution( resolution )
  {
    // Given a number, if it's equal to a hard-coded resolution we use that,
    // otherwise we set the userResolution field.
    var screenResolution = document.getElementById( "screenResolution" );
    var userResolution = document.getElementById( "userResolution" );

    var items = screenResolution.getElementsByAttribute( "value", resolution );
    if (items.item(0))
      {
        // If it's one of the hard-coded values, we'll select it directly 
        screenResolution.selectedItem = items[0];
        userResolution.setAttribute( "hidden", "true" );
      }   
    else
      {
        // Otherwise we need to set up the userResolution field
        var dpi = screenResolution.getAttribute( "dpi" );
        userResolution.setAttribute( "value", resolution );
        userResolution.setAttribute( "label", dpi.replace(/\$val/, resolution) );
        userResolution.removeAttribute( "hidden" );
        screenResolution.selectedItem = userResolution;   
      }
  }
  
*/

