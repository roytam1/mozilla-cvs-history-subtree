
// load a url in the correct external application (e.g. browser for html):
function loadExternalURL(url) {
  var extProtocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"].getService(Components.interfaces.nsIExternalProtocolService);
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
    .getService(Components.interfaces.nsIIOService);
  var uri = ioService.newURI(url, null, null);
  extProtocolSvc.loadUrl(uri);
}

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


function showButtonBar(elem, animate) {
  var bar = document.getElementById(elem);
  bar.style.visibility = "visible";
  if (!animate) {
    bar.style.maxHeight = "200px"; // 200px should be enough
    return;
  }
  // ... else animate:
  // setting the oldHeight to actual height-1, so that 
  // first slideIn call doesn't bail:
  var oldHeight = bar.boxObject.height - 1;
  function slideIn() {
    var height = bar.boxObject.height;
    if (height == oldHeight) return; // all done
    oldHeight = height;
    height += 2;
    bar.style.maxHeight = height+"px";
    setTimeout(slideIn, 10);
  }
  slideIn();
}

function hideButtonBar(elem, animate) {
  var bar = document.getElementById(elem);
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
      setTimeout(slideOut, 10);
    else
      bar.style.visibility = "hidden";
  }
  slideOut();
}
