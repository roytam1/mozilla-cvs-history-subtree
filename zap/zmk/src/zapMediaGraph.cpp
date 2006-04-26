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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

#include "zapMediaGraph.h"
#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "prmem.h"
#include "stdio.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsIPropertyBag2.h"

// ConstructMediaGraph will create a media graph on a new thread and
// return a (nearly) threadsafe proxy.
// If SAME_THREAD_MEDIA_GRAPH is set, the media graph will be created
// on the calling thread. This might be useful for debugging
// #define SAME_THREAD_MEDIA_GRAPH 1


static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);


////////////////////////////////////////////////////////////////////////
// zapMediaGraph

zapMediaGraph::zapMediaGraph()
    : mPumpingEvents(PR_FALSE),
      mNodeIdCounter(0),
      mConnectionIdCounter(0)
{
#ifdef DEBUG_afri_zmk
  printf("zapMediaGraph::zapMediaGraph()\n");
#endif

  mNodes = new Descriptor();
  mConnections = new Descriptor();
}

zapMediaGraph::~zapMediaGraph()
{
#ifdef DEBUG_afri_zmk
  printf("zapMediaGraph::~zapMediaGraph()\n");
#endif
  Shutdown();

  NS_ASSERTION(mNodes && mNodes->prev == mNodes && mNodes->next == mNodes,
               "corrupt node list");
  delete mNodes;

  NS_ASSERTION(mConnections && mConnections->prev == mConnections &&
               mConnections->next == mConnections,
               "corrupt connection list");
  delete mConnections;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapMediaGraph)

void* ShutdownEventHandler(PLEvent *event)
{
  zapMediaGraph* owner = (zapMediaGraph*)PL_GetEventOwner(event);
  owner->mPumpingEvents = PR_FALSE;
  return nsnull;
}

void ShutdownDestroyHandler(PLEvent *event)
{
  PR_FREEIF(event);
}
  
NS_IMETHODIMP_(nsrefcnt)
zapMediaGraph::Release()
{
  nsrefcnt count;
  NS_PRECONDITION(mRefCnt != 0, "dup release");
  count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
  NS_LOG_RELEASE(this, count, "zapMediaGraph");

  if (count == 0) {
    mRefCnt = 1; // stabilize
    NS_DELETEXPCOM(this);
    return 0;
  }
#ifndef SAME_THREAD_MEDIA_GRAPH
  else if (count == 1) {
    // By this time we should only have the nsIThread object holding onto us.
    if (!mEventQ) {
      // Hmm... we could have a race condition here if the
      // proxification failed.
      NS_ERROR("Uh-oh, we haven't got an event queue. Potential race condition. We might leave a thread running!");
    }
    else {
      // Spin down event loop. This will exit Run() and the nsIThread
      // will release us.
      PLEvent *event = PR_NEW(PLEvent);

      PL_InitEvent(event, this, ShutdownEventHandler, ShutdownDestroyHandler);
      mEventQ->PostEvent(event);
    }
  }
#endif
  return count;
}

NS_INTERFACE_MAP_BEGIN(zapMediaGraph)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRunnable)
  NS_INTERFACE_MAP_ENTRY(zapIMediaGraph)
  NS_INTERFACE_MAP_ENTRY(nsIRunnable)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsIRunnable methods:

NS_IMETHODIMP
zapMediaGraph::Run()
{
  nsresult rv = NS_OK;

  nsIThread::GetCurrent(getter_AddRefs(mMediaThread));
  if (!mMediaThread) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIEventQueueService> eventQService =
    do_GetService(kEventQueueServiceCID);
  if (!eventQService) return NS_ERROR_FAILURE;
  
#ifdef SAME_THREAD_MEDIA_GRAPH
  eventQService->GetSpecialEventQueue(nsIEventQueueService::CURRENT_THREAD_EVENT_QUEUE, getter_AddRefs(mEventQ));
#else
  // create an event queue:
  eventQService->CreateFromIThread(mMediaThread, PR_FALSE,
                                   getter_AddRefs(mEventQ));
#endif
  
  if (!mEventQ) return NS_ERROR_FAILURE;

#ifndef SAME_THREAD_MEDIA_GRAPH
  // run event loop:
#ifdef DEBUG_afri_zmk
  printf("running media thread\n");
#endif
  
  PLEvent *event;
  mPumpingEvents = PR_TRUE;
  
  while (mPumpingEvents) {
#ifdef DEBUG_afri_zmk
//    printf(".");
#endif
    rv = mEventQ->WaitForEvent(&event);
    NS_ASSERTION(NS_SUCCEEDED(rv), "WaitForEvent error");
    if (NS_SUCCEEDED(rv)) {
      rv = mEventQ->HandleEvent(event);
      NS_ASSERTION(NS_SUCCEEDED(rv), "HandleEvent error");
    }
  }

  // clean up:
  eventQService->DestroyThreadEventQueue();
  mEventQ = nsnull;
#endif // !SAME_THREAD_MEDIA_GRAPH
  
  return rv;
}

//----------------------------------------------------------------------
// Implementation helpers:

zapMediaGraph::NodeDescriptor*
zapMediaGraph::ResolveNodeDescriptor(const nsACString& id_or_alias)
{
  Descriptor* t = mNodes->next;

  // find target (an alias or node):
  while (1) {
    if (t->type == Descriptor::SENTINEL) return nsnull;
    if (t->name == id_or_alias) break;
    t = t->next;
  }

  // resolve aliases:
  while (t->type == Descriptor::ALIAS) {
    t = ((AliasDescriptor*)t)->target;
  }

  NS_ASSERTION(t->type == Descriptor::NODE, "corrupt node list");
  
  return NS_STATIC_CAST(NodeDescriptor*, t);
}

void
zapMediaGraph::RemoveDescriptor(Descriptor* d)
{
  // remove descriptor d from list:
  d->prev->next = d->next;
  d->next->prev = d->prev;
  
  // recursively remove all descriptors pointing to d:
  Descriptor* p = mNodes;
  while ((p = p->next)->type != Descriptor::SENTINEL) {
    if (p->type == Descriptor::ALIAS &&
        ((AliasDescriptor*)p)->target == d) {
      RemoveDescriptor(p);
      // restart from beginning, because the list will have changed.
      // this could be made more efficient, but our list should always
      // be relatively small and aliasing not deep.
      p = mNodes;
    }
  }

  if (d->type == Descriptor::NODE) {
    RemoveNodeDescriptorConnections(NS_STATIC_CAST(NodeDescriptor*, d));
    NS_STATIC_CAST(NodeDescriptor*, d)->node->RemovedFromGraph(this);
  }
  delete d;
}

void
zapMediaGraph::RemoveConnection(ConnectionDescriptor* d)
{
  // remove descriptor from list:
  d->prev->next = d->next;
  d->next->prev = d->prev;

  d->sink->DisconnectSource(d->src, d->name);
  d->src->DisconnectSink(d->sink, d->name);
  delete d;
}

void
zapMediaGraph::RemoveNodeDescriptorConnections(NodeDescriptor* d)
{
  // search for connections pointing to d and remove them:
  Descriptor* p = mConnections;
  while((p = p->next)->type != Descriptor::SENTINEL) {
    if (NS_STATIC_CAST(ConnectionDescriptor*, p)->src_nd == d ||
        NS_STATIC_CAST(ConnectionDescriptor*, p)->sink_nd == d) {
      RemoveConnection(NS_STATIC_CAST(ConnectionDescriptor*, p));
      // restart from beginning, because the list might have changed.
      // this could be made more efficient, but our list should be
      // relatively small.
      p = mConnections;
    }
  }
}

//----------------------------------------------------------------------
// zapIMediaGraph methods:

/* ACString addNode (in ACString type, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapMediaGraph::AddNode(const nsACString & type, nsIPropertyBag2* node_pars,
                       nsACString & _retval)
{
  // create a new instance of the given type:
  nsCString clazz = NS_LITERAL_CSTRING(ZAP_MEDIANODE_CONTRACTID_PREFIX)+type;
#ifdef DEBUG_afri_zmk
  printf("Trying to instantiate component %s\n", clazz.get());
#endif
  nsCOMPtr<zapIMediaNode> node = do_CreateInstance(clazz.get());
  if (!node) return NS_ERROR_FAILURE;

  // make a new id:
  nsCString id = NS_LITERAL_CSTRING("#");
  id.AppendInt(mNodeIdCounter++);
  
  // insert into node list:
  NodeDescriptor *descriptor = new NodeDescriptor(id, node, mNodes, mNodes->next);
  mNodes->next = descriptor;
  descriptor->next->prev = descriptor;

  // finally inform the node that it has been added to the graph:
  if (NS_FAILED(node->AddedToGraph(this, id, node_pars))) {
    // remove descriptor and all nodes pointing to it from list:
    RemoveDescriptor(descriptor);
    return NS_ERROR_FAILURE;
  }
  
  _retval = id;
  
  return NS_OK;
}

/* void removeNode (in ACString id_or_alias); */
NS_IMETHODIMP
zapMediaGraph::RemoveNode(const nsACString & id_or_alias)
{
  NodeDescriptor* d = ResolveNodeDescriptor(id_or_alias);
  if (!d) {
    NS_WARNING("Target node not found");
    return NS_ERROR_FAILURE;
  }
  RemoveDescriptor(d);
  return NS_OK;
}


  /* void getNode (in ACString id_or_alias, in nsIIDRef uuid, in boolean synchronous, [iid_is (uuid), retval] out nsQIResult result); */
NS_IMETHODIMP
zapMediaGraph::GetNode(const nsACString & id_or_alias,
                       const nsIID & uuid, PRBool synchronous, 
                       void * *result)
{
  NodeDescriptor* nd = ResolveNodeDescriptor(id_or_alias);
  if (!nd) {
    NS_ERROR("Target node not found");
    *result = nsnull;
    return NS_ERROR_FAILURE;
  }
  if (synchronous)
    return nd->node->QueryInterface(uuid, result);
  else
    return NS_GetProxyForObject(mEventQ, uuid, nd->node,
                                PROXY_ASYNC | PROXY_ALWAYS |
                                PROXY_AUTOPROXIFY /*| PROXY_ISUPPORTS*/,
                                result);
}

/* void setAlias (in ACString alias, in ACString id_or_alias); */
NS_IMETHODIMP
zapMediaGraph::SetAlias(const nsACString & alias, const nsACString & id_or_alias)
{
  // check if alias name is allowed:
  if (!alias.IsEmpty()) {
    nsACString::const_iterator p;
    alias.BeginReading(p);
    if (*p == '#') {
      NS_ERROR("Disallowed alias name");
      return NS_ERROR_FAILURE;
    }
  }
  
  // look for target node:
  Descriptor* t = mNodes->next;
  while(1) {
    if (t->type == Descriptor::SENTINEL) {
      NS_ERROR("Target node not found");
      return NS_ERROR_FAILURE;
    }
    if (t->name == id_or_alias) break;
    t= t->next;
  }
  
  // look for node with the alias name:
  Descriptor* a = mNodes;
  while ((a = a->next)->type != Descriptor::SENTINEL) {
    if (a->name == alias) {
      if (a->type == Descriptor::NODE) {
        NS_ERROR("Alias clashes with a node name");
        return NS_ERROR_FAILURE;
      }
      break;
    }
  }

  if (a->type == Descriptor::SENTINEL) {
    // create a new alias descriptor:
    a = new AliasDescriptor(alias, t, mNodes, mNodes->next);
    mNodes->next = a;
    a->next->prev = a;
  }
  else {
    // move exisiting alias to new target:
    NS_ASSERTION(a->type == Descriptor::ALIAS, "node not an alias ?!?");
    ((AliasDescriptor*)a)->target = t;
  }
  
  return NS_OK;
}

/* ACString connect (in ACString source_node, in zapIMap source_pars, in ACString sink_node, in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapMediaGraph::Connect(const nsACString & source_node_id, nsIPropertyBag2 *source_pars,
                       const nsACString & sink_node_id, nsIPropertyBag2 *sink_pars,
                       nsACString & _retval)
{
  NodeDescriptor* src_nd = ResolveNodeDescriptor(source_node_id);
  if (!src_nd) {
    NS_ERROR("Source node not found");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<zapIMediaNode> sourcenode = src_nd->node;

  NodeDescriptor* sink_nd = ResolveNodeDescriptor(sink_node_id);
  if (!sink_nd) {
    NS_ERROR("Sink node not found");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<zapIMediaNode> sinknode = sink_nd->node;
  
  // obtain source:
  nsCOMPtr<zapIMediaSource> source;
  nsresult rv = sourcenode->GetSource(source_pars, getter_AddRefs(source));
  if (NS_FAILED(rv)) return rv;

  // obtain sink:
  nsCOMPtr<zapIMediaSink> sink;
  rv = sinknode->GetSink(sink_pars, getter_AddRefs(sink));
  if (NS_FAILED(rv)) return rv;
  
  // generate new connection id:
  nsCString id = NS_LITERAL_CSTRING("~");
  id.AppendInt(mConnectionIdCounter++);

  // try to form connection:
  rv = source->ConnectSink(sink, id);
  if (NS_FAILED(rv)) return rv;
  rv = sink->ConnectSource(source, id);
  if (NS_FAILED(rv)) {
    source->DisconnectSink(sink, id);
    return rv;
  }
  
  // insert connection into list:
  // XXX possibly we need to move this before the connect{sink,source} calls
  // and clean up in error case
  ConnectionDescriptor *descriptor = new ConnectionDescriptor(id, src_nd, sink_nd, source, sink, mConnections, mConnections->next);
  mConnections->next = descriptor;
  descriptor->next->prev = descriptor;

  _retval = id;
  
  return NS_OK;
}

/* void disconnect (in ACString connection_id); */
NS_IMETHODIMP
zapMediaGraph::Disconnect(const nsACString & connection_id)
{
  // find connection:
  Descriptor* t = mConnections->next;

  while (1) {
    if (t->type == Descriptor::SENTINEL) {
      NS_ERROR("Connection not found");
      return NS_ERROR_FAILURE;
    }
    if (t->name == connection_id) break;
    t = t->next;
  }

  RemoveConnection(NS_STATIC_CAST(ConnectionDescriptor*, t));
  return NS_OK;
}


/* void shutdown (); */
NS_IMETHODIMP
zapMediaGraph::Shutdown()
{
  // release all nodes in a robust way:
  while (mNodes->next->type != Descriptor::SENTINEL)
    RemoveDescriptor(mNodes->next);
  
  // Don't set mPumpingEvents to false here! If we still have a proxy
  // holding onto us it would block. We have to keep pumping events
  // until our ref count goes down to one. The event loop will be spun
  // down from Release().
  return NS_OK;
}

/* readonly attribute nsIEventQueue eventQueue; */
NS_IMETHODIMP
zapMediaGraph::GetEventQueue(nsIEventQueue * *aEventQueue)
{
  *aEventQueue = mEventQ;
  NS_IF_ADDREF(*aEventQueue);
  return NS_OK;
}


//----------------------------------------------------------------------
// Constructor function:


NS_IMETHODIMP ConstructMediaGraph(nsISupports *aOuter, REFNSIID aIID,
                                  void **result)
{
  nsresult rv = NS_OK;
  *result = nsnull;

  if (aOuter != nsnull) return NS_ERROR_NO_AGGREGATION;
  
  nsCOMPtr<nsIEventQueueService> eventQService =
    do_GetService(kEventQueueServiceCID);
  if (!eventQService) {
    NS_ERROR("Can't get event queue service");
    return NS_ERROR_FAILURE;
  }
  nsIRunnable* mediaGraph = (nsIRunnable*) new zapMediaGraph();
  if (!mediaGraph) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mediaGraph);

  nsCOMPtr<nsIThread> mediaThread;
  nsCOMPtr<nsIEventQueue> eventQ;

#ifdef SAME_THREAD_MEDIA_GRAPH
  nsIThread::GetCurrent(getter_AddRefs(mediaThread));
  eventQService->GetSpecialEventQueue(nsIEventQueueService::CURRENT_THREAD_EVENT_QUEUE,
                                      getter_AddRefs(eventQ));
  // For the 'SAME THREAD' case, Run() doesn't go into an event loop,
  // but it still has to be called to initialize the media graph:
  rv = mediaGraph->Run();
  if (NS_FAILED(rv)) {
    NS_ERROR("media graph initialization error");
    NS_RELEASE(mediaGraph);
    return NS_ERROR_FAILURE;
  }
#else
  // Create a new thread and run the media graph:
  NS_NewThread(getter_AddRefs(mediaThread), mediaGraph, 0, PR_UNJOINABLE_THREAD,
               PR_PRIORITY_HIGH);
  if (!mediaThread) {
    NS_RELEASE(mediaGraph);
    return NS_ERROR_FAILURE;
  }
  
  // Get the thread's event queue. We call CreateFromIThread() instead
  // of GetThreadEventQueue() to make sure that the event queue
  // exists. This avoids potential race conditions in case Run()
  // hasn't had a chance to create the queue yet.
  eventQService->CreateFromIThread(mediaThread, PR_FALSE,
                                   getter_AddRefs(eventQ));
#endif

  if (!eventQ) {
    NS_ERROR("Strange. Couldn't get event queue");
    NS_RELEASE(mediaGraph);
    return NS_ERROR_FAILURE;
  }
  
  // Return a proxy for the media graph:
  rv = NS_GetProxyForObject(eventQ, aIID, mediaGraph,
                            PROXY_SYNC | PROXY_ALWAYS | PROXY_AUTOPROXIFY |
                            PROXY_NESTED_QUEUES
                            /* | PROXY_ISUPPORTS */,
                            result);
  if (NS_FAILED(rv)) {
    NS_ERROR("proxification failure. potential race condition.");
  }
  NS_RELEASE(mediaGraph);

  return rv;
}
