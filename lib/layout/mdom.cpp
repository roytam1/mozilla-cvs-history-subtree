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

MDomNode::MDomNode(int node_type)
  : _nodeType(node_type),
    _parentNode(NULL),
    _nextSibling(NULL),
    _prevSibling(NULL),
    _firstChild(NULL),
    _lastChild(NULL),
    _numChildren(0)
    
{
}

int
MDomNode::getNodeType()
{
  return _nodeType;
}

MDomNode*
MDomNode::getParentNode()
{
  return _parentNode;
}

MDomNodeIterator*
MDomNode::getChildNodes()
{
  return 0;
}

XP_Bool
MDomNode::hasChildNodes()
{
  return _numChildren > 0;
}

MDomNode*
MDomNode::getFirstChild()
{
  return _firstChild;
}

MDomNode*
MDomNode::getPreviousSibling()
{
  return _prevSibling;
}

MDomNode*
MDomNode::getNextSibling()
{
  return _nextSibling;
}

MDomNode*
MDomNode::insertBefore(MDomNode *newChild,
                       MDomNode *refChild)
{
  /* if the refChild was not actually a child of this
     node, throw a NotMyChildException */
  if (refChild && refChild->_parentNode != this)
    {
      /* XXX throw NotMyChildException */
      return NULL;
    }

  _numChildren++;

  newChild->_parentNode = this;
  newChild->_nextSibling = refChild;
  if (refChild)
    {
      newChild->_prevSibling = refChild->_prevSibling;
      refChild->_prevSibling = newChild;
    }
  else
    {
      newChild->_prevSibling = _lastChild;
      _lastChild = newChild;
    }

  if (newChild->_prevSibling)
    newChild->_prevSibling->_nextSibling = newChild;

  return newChild;
}

MDomNode*
MDomNode::replaceChild(MDomNode *newChild,
                       MDomNode *oldChild)
{
  /* if the oldChild was not actually a child of this
     node, throw a NotMyChildException */
  if (oldChild->_parentNode != this)
    {
      /* XXX throw NotMyChildException */
      return NULL;
    }

  newChild->_nextSibling = oldChild->_nextSibling;
  newChild->_prevSibling = oldChild->_prevSibling;
  newChild->_parentNode = this;

  if (newChild->_nextSibling)
    newChild->_nextSibling->_prevSibling = newChild;

  if (newChild->_prevSibling)
    newChild->_prevSibling->_nextSibling = newChild;

  oldChild->_prevSibling =
    oldChild->_nextSibling = NULL;

  return oldChild;
}

MDomNode*
MDomNode::removeChild(MDomNode *oldChild)
{
  /* if the oldChild was not actually a child of this
     node, throw a NotMyChildException */
  if (oldChild->_parentNode != this)
    {
      /* XXX throw NotMyChildException */
      return NULL;
    }

  _numChildren--;

  if (oldChild->_prevSibling)
    oldChild->_prevSibling->_nextSibling = oldChild->_nextSibling;
  if (oldChild->_nextSibling)
    oldChild->_nextSibling->_prevSibling = oldChild->_prevSibling;

  return oldChild;
}

MDomElement::MDomElement(JSString *tag_name)
  : MDomNode(ID_ELEMENT),
    _tagName(tag_name)
{
}

MDomElement::MDomElement(JSString *tag_name,
                         MDomAttributeList *attributes)
  : MDomNode(ID_ELEMENT),
    _tagName(tag_name),
    _attr_list(attributes)
{
}

JSString*
MDomElement::getTagName()
{
  return 0;
}

MDomNodeIterator*
MDomElement::getAttributes()
{
  return 0;
}

JSString*
MDomElement::getAttribute(JSString */*name*/)
{
  return 0;
}

void
MDomElement::setAttribute(JSString */*name*/,
                          JSString */*value*/)
{
}

void
MDomElement::removeAttribute(JSString */*name*/)
{
}

MDomAttribute*
MDomElement::getAttributeNode(JSString */*name*/)
{
  return 0;
}

void
MDomElement::setAttributeNode(MDomAttribute* /*newAttr*/)
{
}

void
MDomElement::removeAttributeNode(MDomAttribute */*oldAttr*/)
{
}

MDomNodeIterator*
MDomElement::getElementsByTagName(JSString */*tagname*/)
{
  return 0;
}

void
MDomElement::normalize()
{
}

MDomText::MDomText(JSString *text_data)
  : MDomNode(ID_TEXT)
{
  set_data(text_data);
}

void
MDomText::set_data(JSString */*data*/)
{
}

JSString *
MDomText::get_data()
{
  return 0;
}

void
MDomText::append(JSString */*data*/)
{
}

void
MDomText::insert(int /*offset*/,
                 JSString */*data*/)
{
}

void
MDomText::delete_text(int /*offset*/,
                      int /*count*/)
{
}

void
MDomText::replace(int /*offset*/,
                  int /*count*/,
                  JSString */*data*/)
{
}

void
MDomText::splice(MDomElement */*element*/,
                 int /*offset*/,
                 int /*count*/)
{
}

MDomComment::MDomComment(JSString *comment_data)
  : MDomNode(ID_COMMENT)
{
  set_data(comment_data);
}

void
MDomComment::set_data(JSString */*data*/)
{
}

JSString *
MDomComment::get_data()
{
  return 0;
}

MDomPI::MDomPI(JSString *pi_name,
               JSString *pi_data)
  : MDomNode(ID_PI)
{
  set_name(pi_name);
  set_data(pi_data);
}

void
MDomPI::set_data(JSString */*data*/)
{
}

JSString *
MDomPI::get_data()
{
  return 0;
}

void
MDomPI::set_name(JSString */*data*/)
{
}

JSString *
MDomPI::get_name()
{
  return 0;
}

MDomAttribute::MDomAttribute(JSString *attr_name,
                             JSString *attr_value)
  : MDomNode(ID_ATTRIBUTE),
    _name(attr_name),
    _value(attr_value)
{
}

JSString*
MDomAttribute::getName()
{
  return _name;
}

JSString*
MDomAttribute::getValue()
{
  return _value;
}

XP_Bool
MDomAttribute::get_specified()
{
  return 0;
}

JSString*
MDomAttribute::toString()
{
  return 0;
}

MDomAttribute*
MDomAttributeList::getAttribute(JSString */*attrName*/)
{
  return 0;
}

MDomAttribute*
MDomAttributeList::setAttribute(MDomAttribute */*attr*/)
{
  return 0;
}

MDomAttribute*
MDomAttributeList::remove(JSString */*attrName*/)
{
  return 0;
}

MDomAttribute*
MDomAttributeList::item(unsigned long /*index*/)
{
  return 0;
}

unsigned long
MDomAttributeList::getLength()
{
  return 0;
}

unsigned long
MDomNodeIterator::getLength()
{
  return 0;
}

unsigned long
MDomNodeIterator::getCurrentPos()
{
  return 0;
}

XP_Bool
MDomNodeIterator::atFirst()
{
  return 0;
}

XP_Bool
MDomNodeIterator::atLast()
{
  return 0;
}

MDomNode*
MDomNodeIterator::toNextNode()
{
  return 0;
}

MDomNode*
MDomNodeIterator::toPrevNode()
{
  return 0;
}

MDomNode*
MDomNodeIterator::toFirstNode()
{
  return 0;
}

MDomNode*
MDomNodeIterator::toLastNode()
{
  return 0;
}

MDomNode*
MDomNodeIterator::moveTo(int /*n*/)
{
  return 0;
}

unsigned long
MDomTreeIterator::numChildren()
{
  return _currentNode->_getNumChildren();
}

unsigned long
MDomTreeIterator::numPreviousSiblings()
{
  long num = 0;
  MDomNode *node = _currentNode->getPreviousSibling();

  while (node)
    {
      num++;
      node = node->getPreviousSibling();
    }

  return num;
}

unsigned long
MDomTreeIterator::numNextSiblings()
{
  long num = 0;
  MDomNode *node = _currentNode->getNextSibling();

  while (node)
    {
      num++;
      node = node->getNextSibling();
    }

  return num;
}

MDomNode*
MDomTreeIterator::toParent()
{
  MDomNode* parent = _currentNode->getParentNode();

  if (parent)
    _currentNode = parent;

  return parent;
}

MDomNode*
MDomTreeIterator::toPreviousSibling()
{
  MDomNode* prev_sibling = _currentNode->getPreviousSibling();

  if (prev_sibling)
    _currentNode = prev_sibling;

  return prev_sibling;
}

MDomNode*
MDomTreeIterator::toNextSibling()
{
  MDomNode* next_sibling = _currentNode->getNextSibling();

  if (next_sibling)
    _currentNode = next_sibling;

  return next_sibling;
}

MDomNode*
MDomTreeIterator::toFirstChild()
{
  MDomNode* first_child = _currentNode->getFirstChild();

  if (first_child)
    _currentNode = first_child;

  return first_child;
}

MDomNode*
MDomTreeIterator::toLastChild()
{
  MDomNode* last_child = _currentNode->_getLastChild();

  if (last_child)
    _currentNode = last_child;

  return last_child;
}

/* does the node index start at zero or one? */
MDomNode*
MDomTreeIterator::toNthChild(int n)
{
  MDomNode *nth_node;

  if (n >= _currentNode->_getNumChildren())
    {
      /* throw NoSuchNodeException */
      return NULL;
    }

  nth_node = _currentNode->getFirstChild();

  while (n)
    {
      nth_node = nth_node->getNextSibling();
      n--;
    }

  if (nth_node)
    _currentNode = nth_node;

  return nth_node;
}

MDomDocument*
MDomDocumentContext::get_document()
{
  return 0;
}

void
MDomDocumentContext::set_document(MDomDocument */*document*/)
{
}

MDomDocumentFragment::MDomDocumentFragment()
  : MDomNode(ID_DOCUMENT)
{
}

MDomDocument*
MDomDocumentFragment::get_masterDoc()
{
  return 0;
}

void
MDomDocumentFragment::set_masterDoc(MDomDocument* /*document*/)
{
}

MDomDocument::MDomDocument()
{
  set_masterDoc(this);
}

MDomNode*
MDomDocument::get_documentType()
{
  return 0;
}

void
MDomDocument::set_documentType(MDomNode* /*type*/)
{
}

MDomElement*
MDomDocument::get_documentElement()
{
  return 0;
}

void
MDomDocument::set_documentElement(MDomElement* /*element*/)
{
}

MDomDocumentContext*
MDomDocument::get_contextInfo()
{
  return 0;
}

void
MDomDocument::set_contextInfo(MDomDocumentContext */*contextInfo*/)
{
}

MDomDocumentContext*
MDomDocument::createDocumentContext()
{
  return 0;
}

MDomElement*
MDomDocument::createElement(JSString *tagName,
                            MDomAttributeList* attributes)
{
  return new MDomElement(tagName, attributes);
}

MDomText*
MDomDocument::createTextNode(JSString */*data*/)
{
  return 0;
}

MDomComment*
MDomDocument::createComment(JSString */*data*/)
{
  return 0;
}

MDomPI*
MDomDocument::createPI(JSString */*name*/,
                       JSString */*data*/)
{
  return 0;
}

MDomAttribute*
MDomDocument::createAttribute(JSString */*name*/,
                              MDomNode */*value*/)
{
  return 0;
}

MDomAttributeList*
MDomDocument::createAttributeList()
{
  return 0;
}

MDomTreeIterator*
MDomDocument::createTreeIterator(MDomNode* /*node*/)
{
  return 0;
}

MDomNodeIterator*
MDomDocument::getElementsByTagName(JSString */*tagname*/)
{
  return 0;
}

MDomDocument*
MDomDOM::createDocument(JSString */*type*/)
{
  return 0;
}

XP_Bool
MDomDOM::hasFeature(JSString */*feature*/)
{
  return 0;
}
