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

#ifndef _DOMINATOR_GRAPH_H_
#define _DOMINATOR_GRAPH_H_

#include "LogModule.h"

class ControlGraph;

struct DGNode
{
	Uint32* successorsBegin;
	Uint32* successorsEnd;
};

struct DGLinkedList
{
	DGLinkedList* next;
	Uint32 index;
};

class DominatorGraph
{
private:

	ControlGraph&	controlGraph;

	Uint32			vCount;

	Uint32*			VtoG;
	Uint32*			GtoV;
	Uint32*			parent;
	Uint32*			semi;
	Uint32*			vertex;
	Uint32*			label;
	Uint32*			size;
	Uint32*			ancestor;
	Uint32*			child;
	Uint32*			dom;
	DGLinkedList**	bucket;
	DGNode*			nodes;
	
private:

	void build();

	Uint32 DFS(Uint32 vx, Uint32 n);
	void LINK(Uint32 vx, Uint32 w);
	void COMPRESS(Uint32 vx);
	Uint32 EVAL(Uint32 vx);

public:

	DominatorGraph(ControlGraph& controlGraph);

	Uint32* getSuccessorsBegin(Uint32 n) const {return nodes[n].successorsBegin;}
	Uint32* getSuccessorsEnd(Uint32 n) const {return nodes[n].successorsEnd;}

#ifdef DEBUG_LOG
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

#endif // _DOMINATOR_GRAPH_H_
