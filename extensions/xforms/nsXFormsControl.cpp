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
 *  Brian Ryner <bryner@brianryner.com>
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

#include "nsXFormsControl.h"
#include "nsXFormsModelElement.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXFormsAtoms.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathResult.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsXFormsModelElement.h"

nsXFormsModelElement*
nsXFormsControl::GetModelAndBind(nsIDOMElement **aBindElement)
{
  *aBindElement = nsnull;
  NS_ENSURE_TRUE(mWrapper, nsnull);

  nsCOMPtr<nsIDOMElement> node;
  mWrapper->GetElementNode(getter_AddRefs(node));

  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  NS_ASSERTION(content, "wrapper must implement nsIContent");

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(content->GetDocument());
  if (!domDoc)
    return nsnull;

  nsAutoString bindId;
  content->GetAttr(kNameSpaceID_None, nsXFormsAtoms::bind, bindId);

  nsCOMPtr<nsIDOMNode> modelWrapper;

  if (!bindId.IsEmpty()) {
    // Get the bind element with the given id.
    domDoc->GetElementById(bindId, aBindElement);

    nsCOMPtr<nsIContent> bindContent = do_QueryInterface(*aBindElement);
    nsIContent *modelContent = bindContent;

    while ((modelContent = modelContent->GetParent())) {
      nsINodeInfo *nodeInfo = modelContent->GetNodeInfo();
      if (nodeInfo &&
	  nodeInfo->NamespaceEquals(NS_LITERAL_STRING("http://www.w3.org/2002/xforms")) &&
	  nodeInfo->NameAtom() == nsXFormsAtoms::model)
	break;
    }

    modelWrapper = do_QueryInterface(modelContent);
  } else {
    // If no bind was given, we use model.
    nsAutoString modelId;
    content->GetAttr(kNameSpaceID_None, nsXFormsAtoms::model, modelId);

    if (modelId.IsEmpty()) {
      // No model given, so use the first one in the document.
      nsCOMPtr<nsIDOMNodeList> nodes;
      domDoc->GetElementsByTagNameNS(NS_LITERAL_STRING("http://www.w3.org/2002/xforms"),
				     NS_LITERAL_STRING("model"),
				     getter_AddRefs(nodes));

      if (!nodes)
	return nsnull;

      nodes->Item(0, getter_AddRefs(modelWrapper));
    } else {
      nsCOMPtr<nsIDOMElement> wrapperElement;
      domDoc->GetElementById(modelId, getter_AddRefs(wrapperElement));
      modelWrapper = wrapperElement;
    }
  }

  nsCOMPtr<nsIXFormsModelElement> modelElt = do_QueryInterface(modelWrapper);
  return NS_STATIC_CAST(nsXFormsModelElement*,
			NS_STATIC_CAST(nsIXFormsModelElement*, modelElt));
}

already_AddRefed<nsIDOMNode>
nsXFormsControl::GetInstanceNode()
{
  // A control may be attached to a model by either using the 'bind'
  // attribute to give the id of a bind element, or using the 'model'
  // attribute to give the id of a model.  If neither of these are given,
  // the control belongs to the first model in the document.

  nsCOMPtr<nsIDOMElement> bindElement;
  nsXFormsModelElement *model = GetModelAndBind(getter_AddRefs(bindElement));
  if (!model)
    return nsnull;

  nsCOMPtr<nsIDOMElement> contextNode;

  if (bindElement) {
    contextNode = bindElement;
  } else {
    mWrapper->GetElementNode(getter_AddRefs(contextNode));
  }

  nsAutoString ref;
  contextNode->GetAttribute(NS_LITERAL_STRING("ref"), ref);

  if (ref.IsEmpty())
    return nsnull;

  // Get the instance data and evaluate the xpath expression.
  // XXXfixme when xpath extensions are implemented (instance())
  nsCOMPtr<nsIDOMDocument> instanceDoc;
  model->GetInstanceDocument(NS_LITERAL_STRING(""),
			     getter_AddRefs(instanceDoc));

  if (!instanceDoc)
    return nsnull;

  nsCOMPtr<nsIDOMXPathEvaluator> eval = do_QueryInterface(instanceDoc);
  nsCOMPtr<nsIDOMXPathNSResolver> resolver;
  eval->CreateNSResolver(instanceDoc, getter_AddRefs(resolver));
  NS_ENSURE_TRUE(resolver, nsnull);

  nsCOMPtr<nsIDOMXPathResult> result;
  eval->Evaluate(ref, contextNode, resolver,
		 nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE, nsnull,
		 getter_AddRefs(result));

  nsIDOMNode *resultNode = nsnull;

  if (result)
    result->GetSingleNodeValue(&resultNode); // addrefs

  return resultNode;
}
