/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "Fundamentals.h"
#include <string.h>
#include "ControlGraph.h"
#include "ControlNodes.h"

#include "DominatorGraph.h"

DominatorGraph::DominatorGraph(ControlGraph& controlGraph) : controlGraph(controlGraph)
{
	Uint32 nNodes = controlGraph.nNodes;

	GtoV = new Uint32[nNodes + 1];
	VtoG = new Uint32[nNodes + 1];

	Uint32 v = 1;
	for (Uint32 n = 0; n < nNodes; n++) {
		VtoG[v] = n;
		GtoV[n] = v++;
	}

	// Initialize all the 1-based arrays.
	//
	parent =	new Uint32[v];
	semi =		new Uint32[v];
	vertex =	new Uint32[v];
	label =		new Uint32[v];
	size =		new Uint32[v];
	ancestor =	new Uint32[v];
	child =		new Uint32[v];
	dom =		new Uint32[v];
	bucket =	new DGLinkedList*[v];

	memset(semi, '\0', v * sizeof(Uint32));
	memset(bucket, '\0', v * sizeof(DGLinkedList*));

	vCount = v;

	build();

	delete parent;
	delete semi;
	delete vertex;
	delete label;
	delete size;
	delete ancestor;
	delete child;
	delete dom;
	delete bucket;
}

Uint32 DominatorGraph::DFS(Uint32 vx, Uint32 n)
{
	semi[vx] = ++n;
	vertex[n] = label[vx] = vx;
	ancestor[vx] = child[vx] = 0;
	size[vx] = 1;


	ControlNode& node = *controlGraph.dfsList[VtoG[vx]];
	ControlEdge* successorEnd = node.getSuccessorsEnd();
	for (ControlEdge* successorPtr = node.getSuccessorsBegin(); successorPtr < successorEnd; successorPtr++) {
		Uint32 w = GtoV[successorPtr->getTarget().dfsNum];
		if (semi[w] == 0) {
			parent[w] = vx;
			n = DFS(w, n);
		}
	}
	return n;
}

void DominatorGraph::LINK(Uint32 vx, Uint32 w)
{
    Uint32 s = w;

	while (semi[label[w]] < semi[label[child[s]]]) {
		if (size[s] + size[child[child[s]]] >= (size[child[s]] << 1)) {
			ancestor[child[s]] = s;
			child[s] = child[child[s]];
		} else {
			size[child[s]] = size[s];
			s = ancestor[s] = child[s];
		}
	}
	label[s] = label[w];
	size[vx] += size[w];
	if(size[vx] < (size[w] << 1)) {
		Uint32 t = s;
		s = child[vx]; 
		child[vx] = t;
	}
	while( s != 0 ) {
		ancestor[s] = vx;
		s = child[s];
	}
}


void DominatorGraph::COMPRESS(Uint32 vx)
{
	if(ancestor[ancestor[vx]] != 0) {
		COMPRESS(ancestor[vx]);
		if(semi[label[ancestor[vx]]] < semi[label[vx]])
			label[vx] = label[ancestor[vx]];
		ancestor[vx] = ancestor[ancestor[vx]];
	}
}

Uint32 DominatorGraph::EVAL(Uint32 vx)
{
	if(ancestor[vx] == 0) 
		return label[vx];
	COMPRESS(vx);
	return (semi[label[ancestor[vx]]] >= semi[label[vx]]) ? label[vx] : label[ancestor[vx]];
}

void DominatorGraph::build()
{
	Uint32 n = DFS(GtoV[0], 0);
	size[0] = label[0] = semi[0];

	for (Uint32 i = n; i >= 2; i--) {
		Uint32 w = vertex[i];

		ControlNode& node = *controlGraph.dfsList[VtoG[w]];
		const DoublyLinkedList<ControlEdge>& predecessors = node.getPredecessors();
		for (DoublyLinkedList<ControlEdge>::iterator p = predecessors.begin(); !predecessors.done(p); p = predecessors.advance(p)) {
			Uint32 vx = GtoV[predecessors.get(p).getSource().dfsNum];
			Uint32 u = EVAL(vx);

			if(semi[u] < semi[w])
				semi[w] = semi[u];
		}

		DGLinkedList* elem = new DGLinkedList();
		elem->next = bucket[vertex[semi[w]]];
		elem->index = w;
		bucket[vertex[semi[w]]] = elem;

		LINK(parent[w], w);

		elem = bucket[parent[w]];
		while(elem != NULL) {
			Uint32 vx = elem->index;
			Uint32 u = EVAL(vx);
			dom[vx] = (semi[u] < semi[vx]) ? u : parent[w];
			elem = elem->next;
		}
	}

	memset(size, '\0', n * sizeof(Uint32));
	Pool& pool = controlGraph.pool;
	nodes = new(pool) DGNode[n];

	for(Uint32 j = 2; j <= n; j++) {
		Uint32 w = vertex[j];
		Uint32 d = dom[w];
		if(d != vertex[semi[w]]) {
			d = dom[d];
			dom[w] = d;
		}
		size[d]++;
	}
	dom[GtoV[0]] = 0;

	for (Uint32 k = 1; k <= n; k++) {
		DGNode& node = nodes[VtoG[k]];
		Uint32 count = size[k];
		node.successorsEnd = node.successorsBegin = (count) ? new(pool) Uint32[count] : (Uint32*) 0;
	}

	for (Uint32 l = 2; l <= n; l++)
		*(nodes[VtoG[dom[l]]].successorsEnd)++ = VtoG[l];
}

#ifdef DEBUG_LOG
void DominatorGraph::printPretty(LogModuleObject log)
{
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Dominator Graph:\n"));
	Uint32 nNodes = controlGraph.nNodes;
	for (Uint32 i = 0; i < nNodes; i++) {
		DGNode& node = nodes[i];
		if (node.successorsBegin !=  node.successorsEnd) {
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\tN%d dominates ", i));
			for (Uint32* successorsPtr = node.successorsBegin; successorsPtr < node.successorsEnd; successorsPtr++)
				UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("N%d ", *successorsPtr));
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
		}
	}
}

#endif // DEBUG_LOG



