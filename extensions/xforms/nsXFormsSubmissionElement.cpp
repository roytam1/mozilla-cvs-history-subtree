/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
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

#include <stdio.h>

#include "nsXFormsSubmissionElement.h"
#include "nsXFormsAtoms.h"
#include "nsIXTFGenericElementWrapper.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOM3Node.h"
#include "nsIDOMXPathResult.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathExpression.h"
#include "nsIWebNavigation.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID
};

// nsISupports

NS_IMPL_ISUPPORTS3(nsXFormsSubmissionElement,
                   nsIXTFElement,
                   nsIXTFGenericElement,
                   nsIDOMEventListener)

// nsIXTFElement

NS_IMETHODIMP
nsXFormsSubmissionElement::OnDestroyed()
{
  mContent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::GetElementType(PRUint32 *aElementType)
{
  *aElementType = nsIXTFElement::ELEMENT_TYPE_GENERIC_ELEMENT;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::GetIsAttributeHandler(PRBool *aIsAttributeHandler)
{
  *aIsAttributeHandler = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::GetScriptingInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  return CloneScriptingInterfaces(sScriptingIIDs,
                                  NS_ARRAY_LENGTH(sScriptingIIDs),
                                  aCount, aArray);
}

NS_IMETHODIMP
nsXFormsSubmissionElement::GetNotificationMask(PRUint32 *aNotificationMask)
{
  *aNotificationMask = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillChangeDocument(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::DocumentChanged(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillChangeParent(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::ParentChanged(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillInsertChild(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::ChildInserted(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillAppendChild(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::ChildAppended(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillRemoveChild(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::ChildRemoved(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillSetAttribute(nsIAtom *aName,
                                            const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::AttributeSet(nsIAtom *aName, const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::WillUnsetAttribute(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::AttributeUnset(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsSubmissionElement::DoneAddingChildren()
{
  return NS_OK;
}

// nsIXTFGenericElement

NS_IMETHODIMP
nsXFormsSubmissionElement::OnCreated(nsIXTFGenericElementWrapper *aWrapper)
{
  nsCOMPtr<nsIDOMElement> node;
  aWrapper->GetElementNode(getter_AddRefs(node));

  // It's ok to keep a weak pointer to mContent.  mContent will have an
  // owning reference to this object, so as long as we null out mContent in
  // OnDestroyed, it will always be valid.

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  mContent = content;

  NS_ASSERTION(mContent, "Wrapper is not an nsIContent, we'll crash soon");

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mContent);
  NS_ASSERTION(target, "Wrapper is not a DOM event target");

  nsresult rv = target->AddEventListener(NS_LITERAL_STRING("xforms-submit"),
                                         this, PR_FALSE);
  if (NS_FAILED(rv))
    printf("+++ AddEventListener failed [rv=%x]\n", rv);
  return NS_OK;
}

// nsIDOMEventListener

NS_IMETHODIMP
nsXFormsSubmissionElement::HandleEvent(nsIDOMEvent *aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);
  if (type.EqualsLiteral("xforms-submit"))
    Submit();
  return NS_OK;
}

// private methods

void
nsXFormsSubmissionElement::Submit()
{
  printf("+++ nsXFormsSubmissionElement::Submit\n");

  // 1. ensure that we are not currently processing a xforms-submit on our model


  // 2. get selected node from the instance data (use xpath, gives us node iterator)
  nsCOMPtr<nsIDOMNode> data;
  GetSelectedInstanceData(getter_AddRefs(data));
  if (!data)
  {
    NS_WARNING("could not get selected instance data");
    return;
  }


  // 3. revalidate selected instance data (only for namespaces considered for
  //    serialization)

  // XXX call nsISchemaValidator::validate on each node


  // 4. serialize instance data

  nsCOMPtr<nsIInputStream> stream;
  nsAutoString uri;
  if (NS_FAILED(SerializeData(data, uri, getter_AddRefs(stream))))
  {
    NS_WARNING("failed to serialize data");
    return;
  }


  // 5. dispatch network request
  
  if (NS_FAILED(SendData(uri, stream)))
  {
    NS_WARNING("failed to send data");
    return;
  }
}

nsresult
nsXFormsSubmissionElement::SubmitEnd(PRBool succeeded)
{
  nsCOMPtr<nsIDOMDocumentEvent> doc = do_QueryInterface(mContent->GetDocument());

  nsCOMPtr<nsIDOMEvent> event;
  doc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  event->InitEvent(succeeded ? NS_LITERAL_STRING("xforms-submit-done")
                             : NS_LITERAL_STRING("xforms-submit-error"),
                   PR_TRUE, PR_FALSE);

  nsCOMPtr<nsIDOMEventTarget> target;
  if (succeeded)
    target = do_QueryInterface(mContent);
  //else
  //  target = GetModel();

  PRBool cancelled;
  return target->DispatchEvent(event, &cancelled);
}

void
nsXFormsSubmissionElement::GetSelectedInstanceData(nsIDOMNode **result)
{
  // XXX need to support 'instance(id)' xpath function.  for now, we assume
  // that any xpath expression is relative to the first <instance> element.

  nsCOMPtr<nsIDOMNode> instance;
  GetDefaultInstanceData(getter_AddRefs(instance));
  if (!instance)
  {
    NS_WARNING("model has no instance data!");
    return;
  }

  nsAutoString value;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::bind, value);
  if (value.IsEmpty())
  {
    // inspect 'ref' attribute
    mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::ref, value);
    if (value.IsEmpty())
    {
      // select first <instance> element
      NS_ADDREF(*result = instance);
      return;
    }
  }
  else
  {
    // XXX get bind element, and inspect the 'ref' attribute
    // XXX or we should be able to 
  }
}

void
nsXFormsSubmissionElement::GetDefaultInstanceData(nsIDOMNode **result)
{
  *result = nsnull;

  // default <instance> element is the first <instance> child node of 
  // our parent, which should be a <model> element.

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(mContent);

  nsCOMPtr<nsIDOMNode> parent;
  node->GetParentNode(getter_AddRefs(parent));
  if (!parent)
  {
    NS_WARNING("no parent node!");
    return;
  }

  parent->GetFirstChild(getter_AddRefs(node));
  while (node)
  {
    PRUint16 nodeType;
    node->GetNodeType(&nodeType);
    if (nodeType == nsIDOMNode::ELEMENT_NODE)
    {
      nsAutoString name;
      node->GetLocalName(name);
      if (name.EqualsLiteral("instance"))
      {
        NS_ADDREF(*result = node);
        return;
      }
    }

    nsIDOMNode *temp = nsnull;
    node->GetNextSibling(&temp);
    node.swap(temp);
  }
}

nsresult
nsXFormsSubmissionElement::SerializeData(nsIDOMNode *data, nsString &uri,
                                         nsIInputStream **stream)
{
  uri.Truncate();
  *stream = nsnull;

  nsAutoString action;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::action, action);

  nsAutoString method;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::method, method);

  // XXX case sensistive?
  if (!method.EqualsLiteral("get"))
  {
    NS_NOTREACHED("method not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsAutoString separator;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::separator, separator);
  if (separator.IsEmpty())
    separator.AssignLiteral(";");

  // 1. Each element node is visited in document order. Each element that has
  //    one text node child is selected for inclusion.

  // 2. Element nodes selected for inclusion are encoded as EltName=value{sep},
  //    where = is a literal character, {sep} is the separator character from the
  //    separator attribute on submission, EltName represents the element local
  //    name, and value represents the contents of the text node.

  // NOTE:
  //    The encoding of EltName and value are as follows: space characters are
  //    replaced by +, and then non-ASCII and reserved characters (as defined
  //    by [RFC 2396] as amended by subsequent documents in the IETF track) are
  //    escaped by replacing the character with one or more octets of the UTF-8
  //    representation of the character, with each octet in turn replaced by
  //    %HH, where HH represents the uppercase hexadecimal notation for the
  //    octet value and % is a literal character. Line breaks are represented
  //    as "CR LF" pairs (i.e., %0D%0A).

  // 'get' method:
  // The URI is constructed as follows:
  //  o The submit URI from the action attribute is examined. If it does not
  //    already contain a ? (question mark) character, one is appended. If it
  //    does already contain a question mark character, then a separator
  //    character from the attribute separator is appended.
  //  o The serialized form data is appended to the URI.

  if (action.FindChar(PRUnichar('?')) == kNotFound)
    action.Append(PRUnichar('?'));
  else
    action.Append(separator);

  // recursively build uri
  AppendDataToURI(data, action, separator);

  uri = action;
  return NS_OK;
}

void
nsXFormsSubmissionElement::AppendDataToURI(nsIDOMNode *data, nsString &uri,
                                           const nsString &separator)
{
  nsCOMPtr<nsIDOMNode> firstChild;
  data->GetFirstChild(getter_AddRefs(firstChild));
  if (!firstChild)
    return;
  nsCOMPtr<nsIDOMNode> node;
  data->GetLastChild(getter_AddRefs(node));
  if (firstChild != node)
  {
    // call AppendDataToURI on each child node
    do
    {
      AppendDataToURI(firstChild, uri, separator);
      firstChild->GetNextSibling(getter_AddRefs(node));
      firstChild.swap(node);
    }
    while (firstChild);
    return;
  }

  PRUint16 nodeType;
  firstChild->GetNodeType(&nodeType);
  if (nodeType != nsIDOMNode::TEXT_NODE)
  {
    AppendDataToURI(firstChild, uri, separator);
    return;
  }

  nsAutoString localName;
  data->GetLocalName(localName);

  nsAutoString value;
  firstChild->GetNodeValue(value);
  value.ReplaceChar(PRUnichar(' '), PRUnichar('+'));

  uri.Append(localName + NS_LITERAL_STRING("=") + value + separator);
}

nsresult
nsXFormsSubmissionElement::SendData(nsString &uri, nsIInputStream *stream)
{
  printf("+++ sending to uri=%s [stream=%p]\n",
      NS_ConvertUTF16toUTF8(uri).get(), (void*) stream);

  // XXX need to properly support the various 'replace' modes and trigger
  //     xforms-submit-done or xforms-submit-error when appropriate.

  nsAutoString replace;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::replace, replace);
  if (!replace.IsEmpty() && !replace.EqualsLiteral("all"))
  {
    NS_WARNING("replace != 'all' not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsIDocument *doc = mContent->GetDocument();

  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(container);

  // XXX this is wrong since we need to handle load errors ourselves.

  // XXX we need an API for handing off our channel to the URI loader,
  //     once we decide to load its content into the browser, so that
  //     the URI loader can run its DispatchContent algorithm.
  //     see bug 263084.

  return webNav->LoadURI(uri.get(), nsIWebNavigation::LOAD_FLAGS_NONE,
                         doc->GetDocumentURI(), nsnull, nsnull);
}

// factory constructor

nsresult
NS_NewXFormsSubmissionElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsSubmissionElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
