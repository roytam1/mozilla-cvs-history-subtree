/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _MDOM_JS_H_
#define _MDOM_JS_H_

#include "jsapi.h"
#include "xp_core.h"

XP_BEGIN_PROTOS

extern JSObject* MDOM_NewDOM(JSContext *cx, MDomDOM *dom);
extern JSObject* MDOM_NewDocumentContext(JSContext *cx, MDomDocumentContext *doccontext);
extern JSObject* MDOM_NewDocumentFragment(JSContext *cx, MDomDocumentFragment *docfragment);
extern JSObject* MDOM_NewDocument(JSContext *cx, MDomDocument *doc);
extern JSObject* MDOM_NewNode(JSContext *cx, MDomNode *node);
extern JSObject* MDOM_NewNodeIterator(JSContext *cx, MDomNodeIterator *nodeiterator);
extern JSObject* MDOM_NewTreeIterator(JSContext *cx, MDomTreeIterator *treeiterator);
extern JSObject* MDOM_NewAttribute(JSContext *cx, MDomAttribute *attribute);
extern JSObject* MDOM_NewAttributeList(JSContext *cx, MDomAttributeList *attributelist);
extern JSObject* MDOM_NewElement(JSContext *cx, MDomElement *element);
extern JSObject* MDOM_NewText(JSContext *cx, MDomText *text);
extern JSObject* MDOM_NewComment(JSContext *cx, MDomComment *comment);
extern JSObject* MDOM_NewPI(JSContext *cx, MDomPI *pi);

XP_END_PROTOS

#endif /* _MDOM_JS_H_ */
