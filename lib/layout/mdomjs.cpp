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

#include "mdom.h"
#include "mdomjs.h"
#include "jsapi.h"

/***********************************************************************/
/* Object DOM                                                          */
/***********************************************************************/

PR_STATIC_CALLBACK(void)
dom_finalize(JSContext *cx,
	     JSObject *obj)
{
  MDomDOM *dom;

  dom = (MDomDOM*)JS_GetPrivate(cx, obj);
  if (!dom) return;

  delete dom;
}

JSClass mdom_dom_class = {
  "DOM", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, dom_finalize
};

PR_STATIC_CALLBACK(PRBool)
dom_create_document(JSContext *cx,
		    JSObject *obj,
		    uint argc,
		    jsval *argv,
		    jsval *rval)
{
  MDomDOM *dom;
  JSString *str;
  MDomDocument *new_doc;
  JSObject *doc_obj;

  dom = (MDomDOM*)JS_GetInstancePrivate(cx, obj, &mdom_dom_class, NULL);
  if (!dom)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  new_doc = dom->createDocument(str);
  if (new_doc == NULL)
    return JS_FALSE;

  doc_obj = MDOM_NewDocument(cx, new_doc);
  if (doc_obj == NULL)
    {
      delete new_doc;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(doc_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
dom_has_feature(JSContext *cx,
		JSObject *obj,
		uint argc,
		jsval *argv,
		jsval *rval)
{
  MDomDOM *dom;
  JSString *str;

  dom = (MDomDOM*)JS_GetInstancePrivate(cx, obj, &mdom_dom_class, NULL);
  if (!dom)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  *rval = BOOLEAN_TO_JSVAL(dom->hasFeature(str));

  return JS_TRUE;
}

static JSFunctionSpec dom_methods[] = {
  {"createDocument",	dom_create_document,	1},
  {"hasFeature",	dom_has_feature,	1},
  {0}
};

static void
mdom_initialize_dom_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewDOM(JSContext *cx, MDomDOM *dom)
{
  JSObject *obj = JS_NewObject(cx, &mdom_dom_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, dom_methods) &&
      JS_SetPrivate(cx, obj, dom))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object DocumentContext                                              */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
doccontext_setProperty(JSContext */*cx*/,
		       JSObject */*obj*/,
		       jsval /*id*/,
		       jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
doccontext_getProperty(JSContext */*cx*/,
		       JSObject */*obj*/,
		       jsval /*id*/,
		       jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
doccontext_finalize(JSContext *cx,
		    JSObject *obj)
{
  MDomDocumentContext *doccontext;

  doccontext = (MDomDocumentContext*)JS_GetPrivate(cx, obj);
  if (!doccontext) return;

  delete doccontext;
}

enum doccontext_slot {
  DOCCONTEXT_DOCUMENT	= -1
};

static JSPropertySpec doccontext_props[] = {
  { "document",	DOCCONTEXT_DOCUMENT,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_doccontext_class = {
  "DocumentContext", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, doccontext_getProperty, doccontext_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, doccontext_finalize
};

static void
mdom_initialize_doccontext_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewDocumentContext(JSContext *cx, MDomDocumentContext *doccontext)
{
  JSObject *obj = JS_NewObject(cx, &mdom_doccontext_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_SetPrivate(cx, obj, doccontext) &&
      JS_DefineProperties(cx, obj, doccontext_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object DocumentFragment                                             */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
docfragment_setProperty(JSContext */*cx*/,
			JSObject */*obj*/,
			jsval /*id*/,
			jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
docfragment_getProperty(JSContext */*cx*/,
			JSObject */*obj*/,
			jsval /*id*/,
			jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
docfragment_finalize(JSContext *cx,
		     JSObject *obj)
{
  MDomDocumentFragment *docfragment;

  docfragment = (MDomDocumentFragment*)JS_GetPrivate(cx, obj);
  if (!docfragment) return;

  delete docfragment;
}

enum docfragment_slot {
  DOCFRAGMENT_MASTERDOC	= -1
};

static JSPropertySpec docfragment_props[] = {
  { "masterDoc",	DOCFRAGMENT_MASTERDOC,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_docfragment_class = {
  "DocumentFragment", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, docfragment_getProperty, docfragment_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, docfragment_finalize
};

static void
mdom_initialize_docfragment_class(JSContext */*cx*/)
{
}


JSObject*
MDOM_NewDocumentFragment(JSContext *cx, MDomDocumentFragment *docfragment)
{
  JSObject *obj = JS_NewObject(cx, &mdom_docfragment_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_SetPrivate(cx, obj, docfragment) &&
      JS_DefineProperties(cx, obj, docfragment_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Document                                                     */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
document_setProperty(JSContext */*cx*/,
		     JSObject */*obj*/,
		     jsval /*id*/,
		     jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
document_getProperty(JSContext */*cx*/,
		     JSObject */*obj*/,
		     jsval /*id*/,
		     jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
document_finalize(JSContext *cx,
		  JSObject *obj)
{
  MDomDocument *doc;

  doc = (MDomDocument*)JS_GetPrivate(cx, obj);
  if (!doc) return;

  delete doc;
}


enum document_slot {
  DOCUMENT_TYPE		= -1,
  DOCUMENT_ELEMENT	= -2,
  DOCUMENT_CONTEXTINFO	= -3
};

static JSPropertySpec document_props[] = {
  { "documentType",	DOCUMENT_TYPE,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "documentElement",	DOCUMENT_ELEMENT,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "contextInfo",	DOCUMENT_CONTEXTINFO,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_document_class = {
  "Document", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, document_getProperty, document_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, document_finalize
};

PR_STATIC_CALLBACK(PRBool)
document_create_context(JSContext *cx,
			JSObject *obj,
			uint /*argc*/,
			jsval */*argv*/,
			jsval *rval)
{
  MDomDocument *doc;
  MDomDocumentContext *new_context;
  JSObject *context_obj;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  new_context = doc->createDocumentContext();
  if (new_context == NULL)
    return JS_FALSE;

  context_obj = MDOM_NewDocumentContext(cx, new_context);
  if (context_obj == NULL)
    {
      delete new_context;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(context_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_attribute_list(JSContext *cx,
			       JSObject *obj,
			       uint /*argc*/,
			       jsval */*argv*/,
			       jsval *rval)
{
  MDomDocument *doc;
  MDomAttributeList *new_attrlist;
  JSObject *attrlist_obj;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  new_attrlist = doc->createAttributeList();
  if (new_attrlist == NULL)
    return JS_FALSE;

  attrlist_obj = MDOM_NewAttributeList(cx, new_attrlist);
  if (attrlist_obj == NULL)
    {
      delete new_attrlist;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(attrlist_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_element(JSContext *cx,
			JSObject *obj,
			uint /*argc*/,
			jsval */*argv*/,
			jsval */*rval*/)
{
  MDomDocument *doc;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_text_node(JSContext *cx,
			  JSObject *obj,
			  uint argc,
			  jsval *argv,
			  jsval *rval)
{
  MDomDocument *doc;
  MDomText *new_text;
  JSObject *text_obj;
  JSString *str;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  new_text = doc->createTextNode(str);
  if (new_text == NULL)
    return JS_FALSE;

  text_obj = MDOM_NewText(cx, new_text);
  if (text_obj == NULL)
    {
      delete new_text;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(text_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_comment(JSContext *cx,
			JSObject *obj,
			uint argc,
			jsval *argv,
			jsval *rval)
{
  MDomDocument *doc;
  MDomComment *new_comment;
  JSObject *comment_obj;
  JSString *str;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  new_comment = doc->createComment(str);
  if (new_comment == NULL)
    return JS_FALSE;

  comment_obj = MDOM_NewComment(cx, new_comment);
  if (comment_obj == NULL)
    {
      delete new_comment;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(comment_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_pi(JSContext *cx,
		   JSObject *obj,
		   uint argc,
		   jsval *argv,
		   jsval *rval)
{
  MDomDocument *doc;
  MDomPI *new_pi;
  JSObject *pi_obj;
  JSString *name, *data;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  if (argc != 2 
      || !(name = JS_ValueToString(cx, argv[0]))
      || !(data = JS_ValueToString(cx, argv[1])))
    return JS_FALSE;

  new_pi = doc->createPI(name, data);
  if (new_pi == NULL)
    return JS_FALSE;

  pi_obj = MDOM_NewPI(cx, new_pi);
  if (pi_obj == NULL)
    {
      delete new_pi;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(pi_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_attribute(JSContext *cx,
			  JSObject *obj,
			  uint /*argc*/,
			  jsval */*argv*/,
			  jsval */*rval*/)
{
  MDomDocument *doc;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
document_create_tree_iterator(JSContext *cx,
			      JSObject *obj,
			      uint argc,
			      jsval *argv,
			      jsval *rval)
{
  MDomDocument *doc;
  MDomTreeIterator *new_iterator;
  JSObject *iterator_obj;
  JSObject *node_obj;
  MDomNode *node;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  if (argc != 1 
      || !(node_obj = JSVAL_TO_OBJECT(argv[0])))
    return JS_FALSE;

  /* does this need to be JS_GetInstancePrivate? */
  node = (MDomNode*)JS_GetPrivate(cx, node_obj);
  if (node == NULL)
    return JS_FALSE;

  new_iterator = doc->createTreeIterator(node);
  if (new_iterator == NULL)
    return JS_FALSE;

  iterator_obj = MDOM_NewTreeIterator(cx, new_iterator);
  if (iterator_obj == NULL)
    {
      delete new_iterator;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(iterator_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
document_get_elements_by_tag_name(JSContext *cx,
				  JSObject *obj,
				  uint argc,
				  jsval *argv,
				  jsval *rval)
{
  MDomDocument *doc;
  MDomNodeIterator *new_iterator;
  JSObject *iterator_obj;
  JSString *str;

  doc = (MDomDocument*)JS_GetInstancePrivate(cx, obj, &mdom_document_class, NULL);
  if (!doc)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  new_iterator = doc->getElementsByTagName(str);
  if (new_iterator == NULL)
    return JS_FALSE;

  iterator_obj = MDOM_NewNodeIterator(cx, new_iterator);
  if (iterator_obj == NULL)
    {
      delete new_iterator;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(iterator_obj);

  return JS_TRUE;
}

static JSFunctionSpec document_methods[] = {
  {"createDocumentContext",	document_create_context,		0},
  {"createAttributeList",	document_create_attribute_list,		0},
  {"createElement",		document_create_element,		2},
  {"createTextNode",		document_create_text_node,		1},
  {"createComment",		document_create_comment,		1},
  {"createPI",			document_create_pi,			2},
  {"createAttribute",		document_create_attribute,		2},
  {"createTreeIterator",	document_create_tree_iterator,		1},
  {"getElementsByTagName",	document_get_elements_by_tag_name,	1},
  {0}
};

static void
mdom_initialize_document_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewDocument(JSContext *cx, MDomDocument *doc)
{
  JSObject *obj = JS_NewObject(cx, &mdom_document_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, document_methods) &&
      JS_SetPrivate(cx, obj, doc) &&
      JS_DefineProperties(cx, obj, document_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Node                                                         */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
node_setProperty(JSContext */*cx*/,
		 JSObject */*obj*/,
		 jsval /*id*/,
		 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
node_getProperty(JSContext */*cx*/,
		 JSObject */*obj*/,
		 jsval /*id*/,
		 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
node_finalize(JSContext *cx,
	      JSObject *obj)
{
  MDomNode *node;

  node = (MDomNode*)JS_GetPrivate(cx, obj);
  if (!node) return;

  delete node;
}

enum node_slot {
  NODE_TYPE		= -1,
  NODE_PARENTNODE	= -2,
  NODE_CHILDNODES	= -3,
  NODE_HASCHILDNODES	= -4,
  NODE_FIRSTCHILD	= -5,
  NODE_PREVIOUSSIBLING	= -6,
  NODE_NEXTSIBLING	= -7
};

static JSPropertySpec node_props[] = {
  { "nodeType",		NODE_TYPE,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "parentNode",	NODE_PARENTNODE,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "childNodes",	NODE_CHILDNODES,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "hasChildNodes",	NODE_HASCHILDNODES,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "firstChild",	NODE_FIRSTCHILD,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "previousSibling",	NODE_PREVIOUSSIBLING,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "nextSibling",	NODE_NEXTSIBLING,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_node_class = {
  "Node", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, node_getProperty, node_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, node_finalize
};


PR_STATIC_CALLBACK(PRBool)
node_insert_before(JSContext *cx,
		   JSObject *obj,
		   uint argc,
		   jsval *argv,
		   jsval *rval)
{
  MDomNode *node;
  MDomNode *ret;
  JSObject *ret_obj;
  JSObject *newChild_obj, *refChild_obj;
  MDomNode *newChild;
  MDomNode *refChild;

  node = (MDomNode*)JS_GetInstancePrivate(cx, obj, &mdom_node_class, NULL);
  if (!node)
    return JS_FALSE;

  if (argc != 2 
      || !(newChild_obj = JSVAL_TO_OBJECT(argv[0]))
      || !(refChild_obj = JSVAL_TO_OBJECT(argv[1])))
    return JS_FALSE;

  refChild = (MDomNode*)JS_GetInstancePrivate(cx, refChild_obj, &mdom_node_class, NULL);
  newChild = (MDomNode*)JS_GetInstancePrivate(cx, newChild_obj, &mdom_node_class, NULL);
  if (!refChild || !newChild)
    return JS_FALSE;

  ret = node->insertBefore(newChild, refChild);
  if (!ret)
    return JS_FALSE;

  ret_obj = MDOM_NewNode(cx, ret);
  if (!ret_obj)
  if (ret_obj == NULL)
    {
      delete ret;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(ret_obj);

  return JS_TRUE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
node_replace_child(JSContext *cx,
		   JSObject *obj,
		   uint argc,
		   jsval *argv,
		   jsval *rval)
{
  MDomNode *node;
  MDomNode *ret;
  JSObject *ret_obj;
  JSObject *newChild_obj, *oldChild_obj;
  MDomNode *newChild;
  MDomNode *oldChild;

  node = (MDomNode*)JS_GetInstancePrivate(cx, obj, &mdom_node_class, NULL);
  if (!node)
    return JS_FALSE;

  if (argc != 2 
      || !(newChild_obj = JSVAL_TO_OBJECT(argv[0]))
      || !(oldChild_obj = JSVAL_TO_OBJECT(argv[1])))
    return JS_FALSE;

  oldChild = (MDomNode*)JS_GetInstancePrivate(cx, oldChild_obj, &mdom_node_class, NULL);
  newChild = (MDomNode*)JS_GetInstancePrivate(cx, newChild_obj, &mdom_node_class, NULL);
  if (!oldChild || !newChild)
    return JS_FALSE;

  ret = node->replaceChild(newChild, oldChild);
  if (!ret)
    return JS_FALSE;

  ret_obj = MDOM_NewNode(cx, ret);
  if (!ret_obj)
  if (ret_obj == NULL)
    {
      delete ret;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(ret_obj);

  return JS_TRUE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
node_remove_child(JSContext *cx,
		  JSObject *obj,
		  uint argc,
		  jsval *argv,
		  jsval *rval)
{
  MDomNode *node;
  MDomNode *ret;
  JSObject *ret_obj;
  JSObject *oldChild_obj;
  MDomNode *oldChild;

  node = (MDomNode*)JS_GetInstancePrivate(cx, obj, &mdom_node_class, NULL);
  if (!node)
    return JS_FALSE;

  if (argc != 1
      || !(oldChild_obj = JSVAL_TO_OBJECT(argv[0])))
    return JS_FALSE;

  oldChild = (MDomNode*)JS_GetInstancePrivate(cx, oldChild_obj, &mdom_node_class, NULL);
  if (!oldChild)
    return JS_FALSE;

  ret = node->removeChild(oldChild);
  if (!ret)
    return JS_FALSE;

  ret_obj = MDOM_NewNode(cx, ret);
  if (!ret_obj)
  if (ret_obj == NULL)
    {
      delete ret;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(ret_obj);

  return JS_TRUE;

  return JS_FALSE;
}

static JSFunctionSpec node_methods[] = {
  {"insertBefore",	node_insert_before,		2},
  {"replaceChild",	node_replace_child,		2},
  {"removeChild",	node_remove_child,		1},
  {0}
};

static void
mdom_initialize_node_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewNode(JSContext *cx, MDomNode *node)
{
  JSObject *obj = JS_NewObject(cx, &mdom_node_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, node_methods) &&
      JS_SetPrivate(cx, obj, node) &&
      JS_DefineProperties(cx, obj, node_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object NodeIterator                                                 */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
nodeiterator_setProperty(JSContext */*cx*/,
			 JSObject */*obj*/,
			 jsval /*id*/,
			 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
nodeiterator_getProperty(JSContext */*cx*/,
		 JSObject */*obj*/,
		 jsval /*id*/,
		 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
nodeiterator_finalize(JSContext *cx,
		      JSObject *obj)
{
  MDomNodeIterator *nodeiterator;

  nodeiterator = (MDomNodeIterator*)JS_GetPrivate(cx, obj);
  if (!nodeiterator) return;

  delete nodeiterator;
}

enum nodeiterator_slot {
  NODEITERATOR_LENGTH		= -1,
  NODEITERATOR_CURRENTPOS	= -2,
  NODEITERATOR_ATFIRST		= -3,
  NODEITERATOR_ATLAST		= -4,
  NODEITERATOR_TONEXTNODE	= -5,
  NODEITERATOR_TOPREVNODE	= -6,
  NODEITERATOR_TOFIRSTNODE	= -7,
  NODEITERATOR_TOLASTNODE	= -8
};

static JSPropertySpec nodeiterator_props[] = {
  { "length",		NODEITERATOR_LENGTH,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "currentPos",	NODEITERATOR_CURRENTPOS,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "atFirst",		NODEITERATOR_ATFIRST,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "atLast",		NODEITERATOR_ATLAST,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toNextNode",	NODEITERATOR_TONEXTNODE,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toPrevNode",	NODEITERATOR_TOPREVNODE,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toFirstNode",	NODEITERATOR_TOFIRSTNODE,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toLastNode",	NODEITERATOR_TOLASTNODE,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_nodeiterator_class = {
  "Nodeiterator", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, nodeiterator_getProperty, nodeiterator_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nodeiterator_finalize
};


PR_STATIC_CALLBACK(PRBool)
nodeiterator_move_to(JSContext *cx,
		     JSObject *obj,
		     uint argc,
		     jsval *argv,
		     jsval *rval)
{
  MDomNodeIterator *iterator;
  MDomNode *ret;
  JSObject *ret_obj;
  int n;

  iterator = (MDomNodeIterator*)JS_GetInstancePrivate(cx, obj, &mdom_nodeiterator_class, NULL);
  if (!iterator)
    return JS_FALSE;

  if (argc != 1
      || !(JSVAL_IS_INT(argv[0])))
    return JS_FALSE;

  n = JSVAL_TO_INT(argv[0]);

  ret = iterator->moveTo(n);
  if (!ret)
    return JS_FALSE;

  ret_obj = MDOM_NewNode(cx, ret);
  if (!ret_obj)
  if (ret_obj == NULL)
    {
      delete ret;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(ret_obj);

  return JS_TRUE;
}

static JSFunctionSpec nodeiterator_methods[] = {
  {"moveTo",	nodeiterator_move_to,	1},
  {0}
};

static void
mdom_initialize_nodeiterator_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewNodeIterator(JSContext *cx, MDomNodeIterator *nodeiterator)
{
  JSObject *obj = JS_NewObject(cx, &mdom_nodeiterator_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, nodeiterator_methods) &&
      JS_SetPrivate(cx, obj, nodeiterator) &&
      JS_DefineProperties(cx, obj, nodeiterator_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object TreeIterator                                                 */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
treeiterator_setProperty(JSContext */*cx*/,
			 JSObject */*obj*/,
			 jsval /*id*/,
			 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
treeiterator_getProperty(JSContext */*cx*/,
			 JSObject */*obj*/,
			 jsval /*id*/,
			 jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
treeiterator_finalize(JSContext *cx,
		      JSObject *obj)
{
  MDomTreeIterator *treeiterator;

  treeiterator = (MDomTreeIterator*)JS_GetPrivate(cx, obj);
  if (!treeiterator) return;

  delete treeiterator;
}


enum treeiterator_slot {
  TREEITERATOR_NUMCHILDREN		= -1,
  TREEITERATOR_NUMPREVIOUSSIBLINGS	= -2,
  TREEITERATOR_NUMNEXTSIBLINGS		= -3,
  TREEITERATOR_TOPARENT			= -4,
  TREEITERATOR_TOPREVIOUSSIBLINGS	= -5,
  TREEITERATOR_TONEXTSIBLINGS		= -6,
  TREEITERATOR_TOFIRSTCHILD		= -7,
  TREEITERATOR_TOLASTCHILD		= -8
};

static JSPropertySpec treeiterator_props[] = {
  { "numChildren",	TREEITERATOR_NUMCHILDREN,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "numPreviousSiblings", TREEITERATOR_NUMPREVIOUSSIBLINGS, JSPROP_ENUMERATE|JSPROP_READONLY },
  { "numNextSiblings",	TREEITERATOR_NUMNEXTSIBLINGS,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toParent",		TREEITERATOR_TOPARENT,		JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toPreviousSiblings", TREEITERATOR_TOPREVIOUSSIBLINGS,JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toNextSiblings",	TREEITERATOR_TONEXTSIBLINGS,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toFirstChild",	TREEITERATOR_TOFIRSTCHILD,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toLastChild",	TREEITERATOR_TOLASTCHILD,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_treeiterator_class = {
  "Treeiterator", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, treeiterator_getProperty, treeiterator_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, treeiterator_finalize
};

PR_STATIC_CALLBACK(PRBool)
treeiterator_to_nth_child(JSContext *cx,
			  JSObject *obj,
			  uint argc,
			  jsval *argv,
			  jsval *rval)
{
  MDomTreeIterator *iterator;
  MDomNode *ret;
  JSObject *ret_obj;
  int n;

  iterator = (MDomTreeIterator*)JS_GetInstancePrivate(cx, obj, &mdom_treeiterator_class, NULL);
  if (!iterator)
    return JS_FALSE;

  if (argc != 1
      || !(JSVAL_IS_INT(argv[0])))
    return JS_FALSE;

  n = JSVAL_TO_INT(argv[0]);

  ret = iterator->toNthChild(n);
  if (!ret)
    return JS_FALSE;

  ret_obj = MDOM_NewNode(cx, ret);
  if (!ret_obj)
  if (ret_obj == NULL)
    {
      delete ret;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(ret_obj);

  return JS_TRUE;
}

static JSFunctionSpec treeiterator_methods[] = {
  {"toNthChild",	treeiterator_to_nth_child,	1},
  {0}
};


static void
mdom_initialize_treeiterator_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewTreeIterator(JSContext *cx, MDomTreeIterator *treeiterator)
{
  JSObject *obj = JS_NewObject(cx, &mdom_treeiterator_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, treeiterator_methods) &&
      JS_SetPrivate(cx, obj, treeiterator) &&
      JS_DefineProperties(cx, obj, treeiterator_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Attribute                                                    */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
attribute_setProperty(JSContext */*cx*/,
		      JSObject */*obj*/,
		      jsval /*id*/,
		      jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
attribute_getProperty(JSContext */*cx*/,
		      JSObject */*obj*/,
		      jsval /*id*/,
		      jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
attribute_finalize(JSContext *cx,
		   JSObject *obj)
{
  MDomAttribute *attribute;

  attribute = (MDomAttribute*)JS_GetPrivate(cx, obj);
  if (!attribute) return;

  delete attribute;
}

enum attribute_slot {
  ATTR_SPECIFIED	= -1,
  ATTR_NAME		= -2,
  ATTR_VALUE		= -3,
  ATTR_TOSTRING		= -4
};

static JSPropertySpec attribute_props[] = {
  { "specified",	ATTR_SPECIFIED,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "name",		ATTR_SPECIFIED,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "value",		ATTR_SPECIFIED,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "toString",		ATTR_SPECIFIED,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_attribute_class = {
  "Attribute", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, attribute_getProperty, attribute_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, attribute_finalize
};

static void
mdom_initialize_attribute_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewAttribute(JSContext *cx, MDomAttribute *attribute)
{
  JSObject *obj = JS_NewObject(cx, &mdom_attribute_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_SetPrivate(cx, obj, attribute) &&
      JS_DefineProperties(cx, obj, attribute_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object AttributeList                                                */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
attributelist_setProperty(JSContext */*cx*/,
		      JSObject */*obj*/,
		      jsval /*id*/,
		      jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
attributelist_getProperty(JSContext */*cx*/,
		      JSObject */*obj*/,
		      jsval /*id*/,
		      jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
attributelist_finalize(JSContext *cx,
		       JSObject *obj)
{
  MDomAttributeList *attributelist;

  attributelist = (MDomAttributeList*)JS_GetPrivate(cx, obj);
  if (!attributelist) return;

  delete attributelist;
}

enum attributelist_slot {
  ATTRLIST_LENGTH	= -1
};

static JSPropertySpec attributelist_props[] = {
  { "length",	ATTRLIST_LENGTH,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_attributelist_class = {
  "Attributelist", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, attributelist_getProperty, attributelist_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, attributelist_finalize
};

PR_STATIC_CALLBACK(PRBool)
attributelist_get_attribute(JSContext *cx,
			    JSObject *obj,
			    uint argc,
			    jsval *argv,
			    jsval *rval)
{
  MDomAttributeList *attr_list;
  MDomAttribute *attr;
  JSObject *attr_obj;
  JSString *str;

  attr_list = (MDomAttributeList*)JS_GetInstancePrivate(cx, obj, &mdom_attributelist_class, NULL);
  if (!attr_list)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  attr = attr_list->getAttribute(str);
  if (attr == NULL)
    return JS_FALSE;

  attr_obj = MDOM_NewAttribute(cx, attr);
  if (attr_obj == NULL)
    {
      delete attr;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(attr_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
attributelist_set_attribute(JSContext *cx,
			    JSObject *obj,
			    uint /*argc*/,
			    jsval */*argv*/,
			    jsval */*rval*/)
{
  MDomAttributeList *attr_list;

  attr_list = (MDomAttributeList*)JS_GetInstancePrivate(cx, obj, &mdom_attributelist_class, NULL);
  if (!attr_list)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
attributelist_remove(JSContext *cx,
		     JSObject *obj,
		     uint argc,
		     jsval *argv,
		     jsval *rval)
{
  MDomAttributeList *attr_list;
  MDomAttribute *attr;
  JSObject *attr_obj;
  JSString *str;

  attr_list = (MDomAttributeList*)JS_GetInstancePrivate(cx, obj, &mdom_attributelist_class, NULL);
  if (!attr_list)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  attr = attr_list->remove(str);
  if (attr == NULL)
    return JS_FALSE;

  attr_obj = MDOM_NewAttribute(cx, attr);
  if (attr_obj == NULL)
    {
      delete attr;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(attr_obj);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
attributelist_item(JSContext *cx,
		   JSObject *obj,
		   uint argc,
		   jsval *argv,
		   jsval *rval)
{
  MDomAttributeList *attr_list;
  MDomAttribute *attr;
  JSObject *attr_obj;
  int n;

  attr_list = (MDomAttributeList*)JS_GetInstancePrivate(cx, obj, &mdom_attributelist_class, NULL);
  if (!attr_list)
    return JS_FALSE;

  if (argc != 1 
      || !JSVAL_IS_INT(argv[0]))
    return JS_FALSE;

  n = JSVAL_TO_INT(argv[0]);

  attr = attr_list->item(n);
  if (attr == NULL)
    return JS_FALSE;

  attr_obj = MDOM_NewAttribute(cx, attr);
  if (attr_obj == NULL)
    {
      delete attr;
      return JS_FALSE;
    }

  *rval = OBJECT_TO_JSVAL(attr_obj);

  return JS_TRUE;
}

static JSFunctionSpec attributelist_methods[] = {
  {"getAttribute",	attributelist_get_attribute,	1},
  {"setAttribute",	attributelist_set_attribute,	1},
  {"remove",		attributelist_remove,		1},
  {"item",		attributelist_item,		1},
  {0}
};

static void
mdom_initialize_attributelist_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewAttributeList(JSContext *cx, MDomAttributeList *attributelist)
{
  JSObject *obj = JS_NewObject(cx, &mdom_attributelist_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, attributelist_methods) &&
      JS_SetPrivate(cx, obj, attributelist) &&
      JS_DefineProperties(cx, obj, attributelist_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Element                                                      */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
element_setProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
element_getProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
element_finalize(JSContext *cx,
		 JSObject *obj)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetPrivate(cx, obj);
  if (!element) return;

  delete element;
}

enum element_slot {
  ELEMENT_TAGNAME	= -1,
  ELEMENT_ATTRIBUTES	= -2
};

static JSPropertySpec element_props[] = {
  { "tagName",		ELEMENT_TAGNAME,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "attributes",	ELEMENT_ATTRIBUTES,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_element_class = {
  "Element", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, element_getProperty, element_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, element_finalize
};


PR_STATIC_CALLBACK(PRBool)
element_normalize(JSContext *cx,
		  JSObject *obj,
		  uint /*argc*/,
		  jsval */*argv*/,
		  jsval */*rval*/)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
element_get_attribute(JSContext *cx,
		      JSObject *obj,
		      uint argc,
		      jsval *argv,
		      jsval *rval)
{
  MDomElement *element;
  JSString *str, *ret_str;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  ret_str = element->getAttribute(str);
  if (ret_str == NULL)
    return JS_FALSE;

  *rval = STRING_TO_JSVAL(ret_str);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
element_set_attribute(JSContext *cx,
		      JSObject *obj,
		      uint argc,
		      jsval *argv,
		      jsval */*rval*/)
{
  MDomElement *element;
  JSString *name, *value;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  if (argc != 2 
      || !(name = JS_ValueToString(cx, argv[0]))
      || !(value = JS_ValueToString(cx, argv[1])))
    return JS_FALSE;

  element->setAttribute(name, value);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
element_remove_attribute(JSContext *cx,
			 JSObject *obj,
			 uint argc,
			 jsval *argv,
			 jsval */*rval*/)
{
  MDomElement *element;
  JSString *str;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  if (argc != 1 
      || !(str = JS_ValueToString(cx, argv[0])))
    return JS_FALSE;

  element->removeAttribute(str);

  return JS_TRUE;
}

PR_STATIC_CALLBACK(PRBool)
element_get_attribute_node(JSContext *cx,
			   JSObject *obj,
			   uint /*argc*/,
			   jsval */*argv*/,
			   jsval */*rval*/)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
element_set_attribute_node(JSContext *cx,
			   JSObject *obj,
			   uint /*argc*/,
			   jsval */*argv*/,
			   jsval */*rval*/)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
element_remove_attribute_node(JSContext *cx,
			      JSObject *obj,
			      uint /*argc*/,
			      jsval */*argv*/,
			      jsval */*rval*/)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
element_get_elements_by_tag_name(JSContext *cx,
				 JSObject *obj,
				 uint /*argc*/,
				 jsval */*argv*/,
				 jsval */*rval*/)
{
  MDomElement *element;

  element = (MDomElement*)JS_GetInstancePrivate(cx, obj, &mdom_element_class, NULL);
  if (!element)
    return JS_FALSE;

  return JS_FALSE;
}

static JSFunctionSpec element_methods[] = {
  {"normalize",			element_normalize,			1},
  {"getAttribute",		element_get_attribute,			1},
  {"setAttribute",		element_set_attribute,			2},
  {"removeAttribute",		element_remove_attribute,		1},
  {"getAttributeNode",		element_get_attribute_node,		1},
  {"setAttributeNode",		element_set_attribute_node,		1},
  {"removeAttributeNode",	element_remove_attribute_node,		1},
  {"getElementsByTagName",	element_get_elements_by_tag_name,	1},
  {0}
};

static void
mdom_initialize_element_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewElement(JSContext *cx, MDomElement *element)
{
  JSObject *obj = JS_NewObject(cx, &mdom_element_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, element_methods) &&
      JS_SetPrivate(cx, obj, element) &&
      JS_DefineProperties(cx, obj, element_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Text                                                         */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
text_setProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
text_getProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
text_finalize(JSContext *cx,
	      JSObject *obj)
{
  MDomText *text;

  text = (MDomText*)JS_GetPrivate(cx, obj);
  if (!text) return;

  delete text;
}

enum text_slot {
  TEXT_DATA	= -1
};

static JSPropertySpec text_props[] = {
  { "data",		TEXT_DATA,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_text_class = {
  "Text", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, text_getProperty, text_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, text_finalize
};

PR_STATIC_CALLBACK(PRBool)
text_append(JSContext *cx,
	    JSObject *obj,
	    uint /*argc*/,
	    jsval */*argv*/,
	    jsval */*rval*/)
{
  MDomText *text;

  text = (MDomText*)JS_GetInstancePrivate(cx, obj, &mdom_text_class, NULL);
  if (!text)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
text_insert(JSContext *cx,
	    JSObject *obj,
	    uint /*argc*/,
	    jsval */*argv*/,
	    jsval */*rval*/)
{
  MDomText *text;

  text = (MDomText*)JS_GetInstancePrivate(cx, obj, &mdom_text_class, NULL);
  if (!text)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
text_delete(JSContext *cx,
	    JSObject *obj,
	    uint /*argc*/,
	    jsval */*argv*/,
	    jsval */*rval*/)
{
  MDomText *text;

  text = (MDomText*)JS_GetInstancePrivate(cx, obj, &mdom_text_class, NULL);
  if (!text)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
text_replace(JSContext *cx,
	     JSObject *obj,
	     uint /*argc*/,
	     jsval */*argv*/,
	     jsval */*rval*/)
{
  MDomText *text;

  text = (MDomText*)JS_GetInstancePrivate(cx, obj, &mdom_text_class, NULL);
  if (!text)
    return JS_FALSE;

  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
text_splice(JSContext *cx,
	    JSObject *obj,
	    uint /*argc*/,
	    jsval */*argv*/,
	    jsval */*rval*/)
{
  MDomText *text;

  text = (MDomText*)JS_GetInstancePrivate(cx, obj, &mdom_text_class, NULL);
  if (!text)
    return JS_FALSE;

  return JS_FALSE;
}

static JSFunctionSpec text_methods[] = {
  {"append",		text_append,		1},
  {"insert",		text_insert,		2},
  {"delete",		text_delete,		2},
  {"replace",		text_replace,		3},
  {"splice",		text_splice,		3},
  {0}
};

static void
mdom_initialize_text_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewText(JSContext *cx, MDomText *text)
{
  JSObject *obj = JS_NewObject(cx, &mdom_text_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_DefineFunctions(cx, obj, text_methods) &&
      JS_SetPrivate(cx, obj, text) &&
      JS_DefineProperties(cx, obj, text_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object Comment                                                      */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
comment_setProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
comment_getProperty(JSContext */*cx*/,
		    JSObject */*obj*/,
		    jsval /*id*/,
		    jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
comment_finalize(JSContext *cx,
		 JSObject *obj)
{
  MDomComment *comment;

  comment = (MDomComment*)JS_GetPrivate(cx, obj);
  if (!comment) return;

  delete comment;
}

enum comment_slot {
  COMMENT_DATA	= -1
};

static JSPropertySpec comment_props[] = {
  { "data",		COMMENT_DATA,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_comment_class = {
  "Comment", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, comment_getProperty, comment_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, comment_finalize
};

static void
mdom_initialize_comment_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewComment(JSContext *cx, MDomComment *comment)
{
  JSObject *obj = JS_NewObject(cx, &mdom_comment_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_SetPrivate(cx, obj, comment) &&
      JS_DefineProperties(cx, obj, comment_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************************************************/
/* Object PI                                                           */
/***********************************************************************/

PR_STATIC_CALLBACK(PRBool)
pi_setProperty(JSContext */*cx*/,
	       JSObject */*obj*/,
	       jsval /*id*/,
	       jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(PRBool)
pi_getProperty(JSContext */*cx*/,
	       JSObject */*obj*/,
	       jsval /*id*/,
	       jsval */*vp*/)
{
  return JS_FALSE;
}

PR_STATIC_CALLBACK(void)
pi_finalize(JSContext *cx,
	    JSObject *obj)
{
  MDomPI *pi;

  pi = (MDomPI*)JS_GetPrivate(cx, obj);
  if (!pi) return;

  delete pi;
}

enum pi_slot {
  PI_NAME	= -1,
  PI_DATA	= -2
};

static JSPropertySpec pi_props[] = {
  { "name",		PI_NAME,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { "data",		PI_DATA,	JSPROP_ENUMERATE|JSPROP_READONLY },
  { 0 }
};

JSClass mdom_pi_class = {
  "PI", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, pi_getProperty, pi_setProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, pi_finalize
};

static void
mdom_initialize_pi_class(JSContext */*cx*/)
{
}

JSObject*
MDOM_NewPI(JSContext *cx, MDomPI *pi)
{
  JSObject *obj = JS_NewObject(cx, &mdom_pi_class, NULL, NULL);

  if (!obj) return NULL;

  if (JS_SetPrivate(cx, obj, pi) &&
      JS_DefineProperties(cx, obj, pi_props))
    return obj;
  else
    {
      JS_free(cx, obj);
      return NULL;
    }
}

/***********************************/

void
MDOM_InitializeJSClasses(JSContext *cx)
{
  mdom_initialize_dom_class(cx);
  mdom_initialize_document_class(cx);
  mdom_initialize_doccontext_class(cx);
  mdom_initialize_docfragment_class(cx);
  mdom_initialize_element_class(cx);
  mdom_initialize_node_class(cx);
  mdom_initialize_text_class(cx);
  mdom_initialize_comment_class(cx);
  mdom_initialize_pi_class(cx);
  mdom_initialize_attribute_class(cx);
  mdom_initialize_attributelist_class(cx);
  mdom_initialize_nodeiterator_class(cx);
  mdom_initialize_treeiterator_class(cx);
}
