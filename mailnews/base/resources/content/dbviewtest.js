var dbview;
var nsMsgViewSortType = Components.interfaces.nsMsgViewSortType;
var nsMsgViewSortOrder = Components.interfaces.nsMsgViewSortOrder;

function createView()
{
    dbview = Components.classes["@mozilla.org/messenger/msgdbview;1?type=threaded"].createInstance(Components.interfaces.nsIMsgDBView);
    dump("dbview = " + dbview + "\n");
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
    dbview.sort(nsMsgViewSortType.bySender,nsMsgViewSortOrder.ascending);
}

function sortSenderDescending()
{
    dbview.sort(nsMsgViewSortType.bySender,nsMsgViewSortOrder.descending);
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
    dbview.populateView();
}
