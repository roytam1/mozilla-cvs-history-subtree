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

#ifndef _MDOM_H_
#define _MDOM_H_

#include "jsapi.h"
#include "lo_ele.h"
#include "layout.h"

class MDomNodeIterator;
class MDomDocument;
class MDomAttribute;
class MDomAttributeList;

class MDomNode 
{
public:
  // NodeType's
#define	ID_DOCUMENT	1
#define	ID_ELEMENT	2
#define	ID_ATTRIBUTE	3
#define	ID_PI		4
#define	ID_COMMENT	5
#define	ID_TEXT		6

  MDomNode(int node_type);

  int 				getNodeType();
  MDomNode*			getParentNode();
  MDomNodeIterator*	getChildNodes();
  XP_Bool			hasChildNodes();
  MDomNode*			getFirstChild();
  MDomNode*			getPreviousSibling();
  MDomNode*			getNextSibling();
  MDomNode*			insertBefore(MDomNode *newChild,
                                 MDomNode *refChild);
  MDomNode*			replaceChild(MDomNode *newChild,
                                 MDomNode *oldChild);
  MDomNode*			removeChild(MDomNode *oldChild);

  int				_getNumChildren();
  MDomNode*			_getLastChild();
private:
  int				_nodeType;
  MDomNode*			_parentNode;
  MDomNode*			_nextSibling;
  MDomNode*			_prevSibling;
  MDomNode*			_firstChild;
  MDomNode*			_lastChild;
  int32				_numChildren;
};

class MDomElement : public MDomNode
{
public:
  MDomElement(JSString *tag_name);
  MDomElement(JSString *tag_name, MDomAttributeList* attributes);

  JSString*			getTagName();
  MDomNodeIterator*	getAttributes();
  JSString*			getAttribute(JSString *name);
  void				setAttribute(JSString *name,
                                 JSString *value);
  void				removeAttribute(JSString *name);
  MDomAttribute*	getAttributeNode(JSString *name);
  void				setAttributeNode(MDomAttribute* newAttr);
  void				removeAttributeNode(MDomAttribute *oldAttr);
  MDomNodeIterator*	getElementsByTagName(JSString *tagname);
  void				normalize();

private:
  JSString*				_tagName;
  MDomAttributeList*	_attr_list;
};

class MDomText : public MDomNode
{
public:
  MDomText(JSString *text_data);

  JSString*	data;

  void		append(JSString *data);
  void		insert(int offset,
                   JSString *data);
  void		delete_text(int offset,
                        int count);
  void		replace(int offset,
                    int count,
                    JSString *data);
  void		splice(MDomElement *element,
                   int offset,
                   int count);
};

class MDomComment : public MDomNode
{
public:
  MDomComment(JSString *comment_data);

  JSString*	data;
};

class MDomPI : public MDomNode
{
public:
  MDomPI(JSString *pi_name, JSString *pi_data);

  JSString*	name;
  JSString*	data;
};

class MDomAttribute : public MDomNode
{
public:
  MDomAttribute(JSString *attr_name, JSString *attr_value);

  JSString*	getName();
  JSString*	getValue();
  
  XP_Bool	specified;

  JSString*	toString();

private:
  JSString* _name;
  JSString* _value;
};

class MDomAttributeList
{
public:
  MDomAttribute*	getAttribute(JSString *attrName);
  MDomAttribute*	setAttribute(MDomAttribute *attr);
  MDomAttribute*	remove(JSString *attrName);
  MDomAttribute*	item(unsigned long index);
  unsigned long		getLength();
};

class MDomNodeIterator
{
public:
  unsigned long		getLength();
  unsigned long		getCurrentPos();
  XP_Bool			atFirst();
  XP_Bool			atLast();
  MDomNode*			toNextNode();
  MDomNode*			toPrevNode();
  MDomNode*			toFirstNode();
  MDomNode*			toLastNode();
  MDomNode*			moveTo(int n);
};

class MDomTreeIterator : public MDomNodeIterator
{
public:
  unsigned long		numChildren();
  unsigned long		numPreviousSiblings();
  unsigned long		numNextSiblings();
  MDomNode*			toParent();
  MDomNode*			toPreviousSibling();
  MDomNode*			toNextSibling();
  MDomNode*			toFirstChild();
  MDomNode*			toLastChild();
  MDomNode*			toNthChild(int n);

private:
  MDomNode*		_currentNode;
};

class MDomDocumentContext
{
public:
  /* attribute Document document; */
  MDomDocument*	document;
};

class MDomDocumentFragment : public MDomNode
{
public:
  MDomDocumentFragment();

  /* attribute Document masterDoc; */
  MDomDocument*	masterDoc;
};

class MDomDocument : public MDomDocumentFragment
{
public:
  MDomDocument();

  /* attribute Node documentType; */
  MDomNode*				documentType;
  /* attribute Element documentElement; */
  MDomElement*			documentElement;
  /* attribute DocumentContext contextInfo; */
  MDomDocumentContext*	contextInfo;

  /* DocumentContext createDocumentContext(); */
  MDomDocumentContext*	createDocumentContext();
  /* Element createElement(in wstring tagName, in AttributeList attributes) */
  MDomElement*			createElement(JSString *tagName,
                                      MDomAttributeList* attributes);
  /* Text createTextNode(in wstring data); */
  MDomText*				createTextNode(JSString *data);
  /* Comment createComment(in wstring data); */
  MDomComment*			createComment(JSString *data);
  /* PI createPI(in wstring name, in wstring data); */
  MDomPI*				createPI(JSString *name, JSString *data);
  /* Attribute createAttribute(in wstring name, in Node value); */
  MDomAttribute*		createAttribute(JSString *name, MDomNode *value);

  /* AttributeList createAttributeList(); */
  MDomAttributeList*	createAttributeList();
  /* TreeIterator createTreeIterator(in Node node) */
  MDomTreeIterator*		createTreeIterator(MDomNode* node);
  /* NodeIterator getElementsByTagName(in wstring tagname); */
  MDomNodeIterator*		getElementsByTagName(JSString *tagname);
};

class MDomDOM
{
public:
  /* Document createDocument(in wstring type); */
  MDomDocument*	createDocument(JSString *type);
  
  /* boolean hasFeature(in wstring feature); */
  XP_Bool		hasFeature(JSString *feature);
};

#endif /* _MDOM_H_ */
