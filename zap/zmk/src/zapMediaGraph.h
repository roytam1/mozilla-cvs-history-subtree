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

#ifndef __ZAP_MEDIAGRAPH_H__
#define __ZAP_MEDIAGRAPH_H__

#include "zapIMediaGraph.h"
#include "nsIEventTarget.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "zapIMediaNode.h"
#include "nsString.h"

class zapMediaGraph : public zapIMediaGraph,
                      public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIAGRAPH
  NS_DECL_NSIRUNNABLE
  
  zapMediaGraph();
  ~zapMediaGraph();

private:
  // A media graph is unlikely to contain many nodes (<< 20), so we
  // use a simple linked lists rather than a hashtable to keep track
  // of nodes, aliases and connections:

  struct Descriptor {
    Descriptor() : type(SENTINEL) { prev = this; next = this; };
    virtual ~Descriptor() {};
    
    enum { NODE, ALIAS, CONNECTION, SENTINEL } type;
    nsCString name;
    Descriptor* prev;
    Descriptor* next;
  };

  struct NodeDescriptor : public Descriptor {
    NodeDescriptor(const nsACString& _name, zapIMediaNode* _node,
                   Descriptor* _prev, Descriptor* _next)
        : node(_node) {
      type = NODE;
      name = _name;
      prev = _prev;
      next = _next;
    };
    
    nsCOMPtr<zapIMediaNode> node;
  };

  struct AliasDescriptor : public Descriptor {
    AliasDescriptor(const nsACString& _name, Descriptor* _target,
                    Descriptor* _prev, Descriptor* _next)
        : target(_target) {
      type = ALIAS;
      name = _name;
      prev = _prev;
      next = _next;
    };
    
    Descriptor* target;
  };

  struct ConnectionDescriptor : public Descriptor {
    ConnectionDescriptor(const nsACString& _name,
                         NodeDescriptor* _src_nd,
                         NodeDescriptor* _sink_nd,
                         zapIMediaSource* _src,
                         zapIMediaSink* _sink,
                         Descriptor* _prev, Descriptor* _next)
        : src_nd(_src_nd), sink_nd(_sink_nd),
          src(_src), sink(_sink)
    {
      type = CONNECTION;
      name = _name;
      prev = _prev;
      next = _next;
    };

    NodeDescriptor* src_nd;
    NodeDescriptor* sink_nd;
    nsCOMPtr<zapIMediaSource> src;
    nsCOMPtr<zapIMediaSink> sink;
  };
  
  // List of nodes and Aliases. Only contains descriptors of type
  // ALIAS, NODE and SENTINEL.
  Descriptor* mNodes;

  // List of connections. Only contains descriptors of type CONNECTION
  // and SENTINEL.
  Descriptor* mConnections;
  
  NodeDescriptor* ResolveNodeDescriptor(const nsACString& id_or_alias);
  void RemoveDescriptor(Descriptor* d);
  void RemoveConnection(ConnectionDescriptor* d);
  void RemoveNodeDescriptorConnections(NodeDescriptor* d);
  
  nsCOMPtr<nsIThread> mMediaThread;
  nsCOMPtr<nsIEventTarget> mEventTarget;
  PRBool mPumpingEvents;
  PRUint32 mNodeIdCounter;
  PRUint32 mConnectionIdCounter;
  
  friend void* ShutdownEventHandler(PLEvent *self);
};

// constructor to obtain a proxied zapMediaGraph object:
NS_IMETHODIMP ConstructMediaGraph(nsISupports *aOuter, REFNSIID aIID, void **result);

#endif // __ZAP_MEDIAGRAPH_H__
