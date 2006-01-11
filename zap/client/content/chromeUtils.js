
// load a url in the correct external application (e.g. browser for html):
function loadExternalURL(url) {
  var extProtocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"].getService(Components.interfaces.nsIExternalProtocolService);
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);
  var uri = ioService.newURI(url, null, null);
  extProtocolSvc.loadUrl(uri);
}

//----------------------------------------------------------------------

// get the resource for the current selection in a ds-filled tree:
function getSelectedResource(tree) {
  var start = {};
  var end = {};
  tree.view.selection.getRangeAt(0, start, end);
  if (start.value<0) return null;

  return tree.builderView.getResourceAtIndex(start.value);
}

// select the node associated with the given resource in a ds-filled tree:
function selectResource(tree, resource) {
  var index = tree.builder.getIndexOfResource(resource);
  if (index < 0) return;
  tree.view.selection.select(index);
  tree.treeBoxObject.ensureRowIsVisible(index);
}

//----------------------------------------------------------------------

// maximum height before the slidebar gets scrollbars:
var VSLIDEBAR_MAX_HEIGHT = 100;

function showVSlideBar(elem, animate) {
  var bar = document.getElementById(elem);
  if (bar.slideOutTimeout) {
    clearTimeout(bar.slideOutTimeout);
    delete bar.slideOutTimeout;
  }
  
  bar.style.visibility = "visible";
  if (!animate) {
    bar.style.maxHeight = VSLIDEBAR_MAX_HEIGHT+"px";
    return;
  }
  // ... else animate:
  // setting the oldHeight to actual height-1, so that 
  // first slideIn call doesn't bail:
  var oldHeight = bar.boxObject.height - 1;
  function slideIn() {
    var height = bar.boxObject.height;
    if (height == oldHeight) {
      delete bar.slideInTimeout;
      return; // all done
    }
    if (height > VSLIDEBAR_MAX_HEIGHT) {
      bar.style.overflow = "-moz-scrollbars-vertical";
      delete bar.slideInTimeout;
      return; // all done
    }
    oldHeight = height;
    height += 2;
    bar.style.maxHeight = height+"px";
    bar.slideInTimeout = setTimeout(slideIn, 10);
  }
  slideIn();
}

function hideVSlideBar(elem, animate) {
  var bar = document.getElementById(elem);
  if (bar.slideInTimeout) {
    clearTimeout(bar.slideInTimeout);
    delete bar.slideInTimeout;
  }
  
  bar.style.overflow = "hidden";
  if (!animate) {
    bar.style.maxHeight = "0px";
    bar.style.visibility = "hidden";
    return;
  }
  // ... else animate:
  var i = bar.boxObject.height;
  function slideOut() {
    bar.style.maxHeight = i+"px";
    i -= 2;
    if (i>=0)
      bar.slideOutTimeout = setTimeout(slideOut, 10);
    else {
      bar.style.maxHeight = "0px";
      bar.style.visibility = "hidden";
      delete bar.slideOutTimeout;
    }
  }
  slideOut();
}

//----------------------------------------------------------------------

function makeNumberValidator(min, max, emptyAllowed) {
  return function(v) {
    if (!v || v.length==0) return emptyAllowed ? true : false;
    if (!/^\d+$/.test(v)) return false;
    v = parseInt(v);
    if (min !== undefined &&
        v < min) return false;
    if (max !== undefined &&
        v > max) return false;
    return true;
  };
}

function validateSIPURI(v) {
  var sipstack = window.wSipStack;
  if (!sipstack) sipstack = parent.wSipStack;
  try {
    sipstack.syntaxFactory.deserializeURI(v);
  }
  catch (e) {
    return false;
  }
  return true;
}

function validateSIPRouteSet(v) {
  var sipstack = window.wSipStack;
  if (!sipstack) sipstack = parent.wSipStack;
  try {
    sipstack.syntaxFactory.deserializeRouteSet(v, {});
  }
  catch (e) {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------
// formWidgetStateChanged: A 'widgetStateChanged' implementation used
// for zap forms to show/hide the 'buttonbar' element and
// enable/disable the 'apply' button as appropriate for the state of
// the widgets in the form. See e.g. config.xul for how this is used.

var modifiedFormWidgets = 0;
var invalidFormWidgets  = 0;
var stickyButtonBar = false;

function formWidgetStateChanged(widget, oldState, newState) {
  if (oldState & 0x0002 && !(newState & 0x0002)) {
    // modified -> non modified
    modifiedFormWidgets -= 1;
  }
  else if (newState & 0x0002 && !(oldState & 0x0002)) {
    // non modified -> modified
    modifiedFormWidgets += 1;
  }
  
  if (oldState & 0x0001 && !(newState & 0x0001)) {
    // invalid -> valid
    invalidFormWidgets -= 1;
  }
  else if (newState & 0x0001 && !(oldState & 0x0001)) {
    // valid -> invalid
    invalidFormWidgets += 1;
  }
  dump(widget.tagName+" state changed: modified:"+modifiedFormWidgets+" invalid:"+invalidFormWidgets+"\n");
  updateButtonBar();
}

// sticky==true: buttonbar will be sticky (e.g. for 'adding' mode)
function setButtonBarState(sticky) {
  if (sticky != stickyButtonBar) {
    stickyButtonBar = sticky;
    updateButtonBar();
  }
}

function updateButtonBar() {
  if (stickyButtonBar || modifiedFormWidgets > 0) 
    showVSlideBar("buttonbar", true);
  else 
    hideVSlideBar("buttonbar", true);
  if (invalidFormWidgets > 0)
    document.getElementById("apply").setAttribute("disabled", "true");
  else
    document.getElementById("apply").setAttribute("disabled", "false");          
}
