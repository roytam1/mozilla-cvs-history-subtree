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

int
MDomNode::getNodeType()
{
  return 0;
}

MDomNode*
MDomNode::getParentNode()
{
  return 0;
}

MDomNodeIterator*
MDomNode::getChildNodes()
{
  return 0;
}

XP_Bool
MDomNode::hasChildNodes()
{
  return 0;
}

MDomNode*
MDomNode::getFirstChild()
{
  return 0;
}

MDomNode*
MDomNode::getPreviousSibling()
{
  return 0;
}

MDomNode*
MDomNode::getNextSibling()
{
  return 0;
}

MDomNode*
MDomNode::insertBefore(MDomNode */*newChild*/,
                       MDomNode */*refChild*/)
{
  return 0;
}

MDomNode*
MDomNode::replaceChild(MDomNode */*newChild*/,
                       MDomNode */*refChild*/)
{
  return 0;
}

MDomNode*
MDomNode::removeChild(MDomNode */*oldChild*/)
{
  return 0;
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

JSString*
MDomAttribute::getName()
{
  return 0;
}

JSString*
MDomAttribute::getValue()
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
  return 0;
}

unsigned long
MDomTreeIterator::numPreviousSiblings()
{
  return 0;
}

unsigned long
MDomTreeIterator::numNextSiblings()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toParent()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toPreviousSibling()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toNextSibling()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toFirstChild()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toLastChild()
{
  return 0;
}

MDomNode*
MDomTreeIterator::toNthChild(int /*n*/)
{
  return 0;
}

MDomDocumentContext*
MDomDocument::createDocumentContext()
{
  return 0;
}

MDomElement*
MDomDocument::createElement(JSString */*tagName*/,
                            MDomAttributeList* /*attributes*/)
{
  return 0;
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
