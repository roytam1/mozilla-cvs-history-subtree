/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Doron Rosenberg (doronr@naboonline.com) 
 */

var gBrowser = null;
var gDocument = document;
var gUrl = null;
var gListenerRegistered = false;

var progressListener = {
     onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
     {
       if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
          setWindowTitle(gDocument, gUrl);
     },
     onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
     {
     },
 
     onLocationChange: function(aWebProgress, aRequest, aLocation)
     {

     },
 
     onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage)
     {
     },
 
     onSecurityChange: function(aWebProgress, aRequest, state)
     {
     },
 
     QueryInterface : function(iid)
     {
       if (iid.equals(Components.interfaces.nsIWebProgressListener) ||
           iid.equals(Components.interfaces.nsISupportsWeakReference) ||
           iid.equals(Components.interfaces.nsISupports))
         return this;
      
       throw Components.results.NS_NOINTERFACE;
     }
 };

function onLoadViewSource() 
{
  var url = window.arguments[0];
  viewSource(url);
  window._content.focus();
}

function onUnLoadViewSource()
{
  if (gListenerRegistered)
    getBrowser().removeProgressListener(progressListener);
}

function setWindowTitle(document, url)
{
  theWindow = document.getElementById("main-window");
  var headerIndex = url.lastIndexOf("?header=src");
  if (headerIndex > 0)
    url = url.substr(0, headerIndex - 1);
  document.title = theWindow.getAttribute("titlepreface") +
                 url + 
                 theWindow.getAttribute("titlemenuseparator") + 
                 theWindow.getAttribute("titlemodifier");
}

function getBrowser()
{
  if (!gBrowser)
    gBrowser = document.getElementById("content");
  return gBrowser;
}

function getMarkupDocumentViewer()
{
  return getBrowser().markupDocumentViewer;
}

function viewSource(url)
{
  if (!url)
    return false; // throw Components.results.NS_ERROR_FAILURE;

  gUrl = url;

  //
  // Parse the 'arguments' supplied with the dialog.
  //    arg[0] - URL string.
  //    arg[1] - Charset value in the form 'charset=xxx'.
  //
  getMarkupDocumentViewer().defaultCharacterSet = "";
  if ("arguments" in window) {
    var arg;
    //
    // Set the charset of the viewsource window...
    //
    if (window.arguments.length >= 2) {
      arg = window.arguments[1];
      try {
        if (typeof(arg) == "string" && arg.indexOf('charset=') != -1) {
          var arrayArgComponents = arg.split('=');
          if (arrayArgComponents) {
            getMarkupDocumentViewer().defaultCharacterSet = arrayArgComponents[1];
          } 
        }
      } catch (ex) {
        // Ignore the failure and keep processing arguments...
      }
    }
  }

  //
  // Currently, an exception is thrown if the URL load fails...
  //
  var loadFlags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
  getBrowser().addProgressListener(progressListener);
  gListenerRegistered = true;
  getBrowser().webNavigation.loadURI(url, loadFlags, null, null, null);

  window._content.focus();
  return true;
}

