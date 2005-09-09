
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
