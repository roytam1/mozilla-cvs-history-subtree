var dbview;
var nsMsgViewSortType = Components.interfaces.nsMsgViewSortType;
var nsMsgViewSortOrder = Components.interfaces.nsMsgViewSortOrder;

var RDF = Components.classes['@mozilla.org/rdf/rdf-service;1'].getService();
RDF = RDF.QueryInterface(Components.interfaces.nsIRDFService);

function openView()
{
    var textField = document.getElementById('uriField');
    var uri = textField.value;
    dump("uri = " + uri + "\n");
    dbview = Components.classes["@mozilla.org/messenger/msgdbview;1?type=threaded"].createInstance(Components.interfaces.nsIMsgDBView);

    var resource = RDF.GetResource(uri);
    var folder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
    
    dump(folder + "\n");
    var count;
    dbview.open(folder, nsMsgViewSortType.bySubject, count);
}

function sortDateAscending()
{
    dbview.sort(nsMsgViewSortType.byDate,nsMsgViewSortOrder.ascending);
}

function sortDateDescending()
{
    dbview.sort(nsMsgViewSortType.byDate,nsMsgViewSortOrder.descending);
}

function sortThreadedAscending()
{
    dbview.sort(nsMsgViewSortType.byThread,nsMsgViewSortOrder.ascending);
}

function sortThreadedDescending()
{
    dbview.sort(nsMsgViewSortType.byThread,nsMsgViewSortOrder.descending);
}

function sortPriorityAscending()
{
    dbview.sort(nsMsgViewSortType.byPriority,nsMsgViewSortOrder.ascending);
}

function sortPriorityDescending()
{
    dbview.sort(nsMsgViewSortType.byPriority,nsMsgViewSortOrder.descending);
}

function sortSenderAscending()
{
    dbview.sort(nsMsgViewSortType.byAuthor,nsMsgViewSortOrder.ascending);
}

function sortSenderDescending()
{
    dbview.sort(nsMsgViewSortType.byAuthor,nsMsgViewSortOrder.descending);
}

function sortSubjectAscending()
{
    dbview.sort(nsMsgViewSortType.bySubject,nsMsgViewSortOrder.ascending);
}

function sortSubjectDescending()
{
    dbview.sort(nsMsgViewSortType.bySubject,nsMsgViewSortOrder.descending);
}

function sortSizeAscending()
{
    dbview.sort(nsMsgViewSortType.bySize,nsMsgViewSortOrder.ascending);
}

function sortSizeDescending()
{
    dbview.sort(nsMsgViewSortType.bySize,nsMsgViewSortOrder.descending);
}

function dumpView()
{   
    dbview.dumpView();
}

function populateView()
{
    dump("populate view\n");
    dbview.populateView();
}
