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
 *  Aaron Reed <aaronr@us.ibm.com>
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

#include "nsXFormsUtilitiesService.h"
#include "nsXFormsUtils.h"
#include "nsIXTFElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIXFormsModelElement.h"
#include "nsIDOMNodeList.h"
#include "nsIInstanceElementPrivate.h"

NS_IMPL_ISUPPORTS1(nsXFormsUtilitiesService, nsIXFormsUtilitiesService)

NS_IMETHODIMP
nsXFormsUtilitiesService::GetModelFromNode(nsIDOMNode *aNode, 
                                           nsIDOMNode **aModel)
{
  NS_ENSURE_ARG(aNode);
  NS_ENSURE_ARG_POINTER(aModel);

  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
  nsCOMPtr<nsIXTFElement> xtfele = do_QueryInterface(aNode);

  nsAutoString namespaceURI;
  aNode->GetNamespaceURI(namespaceURI);

  // If the node is in the XForms namespace and XTF based, then it should
  //   be able to be handled by GetModel.  Otherwise it is probably an instance
  //   node in a instance document.
  if ((namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) && xtfele) {
    nsCOMPtr<nsIDOMNode> modelElement = nsXFormsUtils::GetModel(element);
    if( modelElement ) {
      NS_IF_ADDREF(*aModel = modelElement);
    }

    // No model found
    NS_ENSURE_TRUE(*aModel, NS_ERROR_FAILURE);
  }

  return NS_OK;
}


/* Right now this function is only called by XPath in the case of the 
 * instance() function.  aModel is a nsXFormsModelElement.  aNode, if coming
 * from XPath, will be the contextNode of the expression that is getting
 * evaluated.  Right now that is either an XForms element or an instance node
 * in an instance document.  What this function needs to do is determine if
 * the given aNode is associated with the given model and return whether it
 * is or not in aModelAssocWithNode.
 */
NS_IMETHODIMP
nsXFormsUtilitiesService::IsThisModelAssocWithThisNode(nsIDOMNode *aModel,
                                                    nsIDOMNode *aNode,
                                                    PRBool *aModelAssocWithNode)
{
  NS_ENSURE_ARG(aModel);
  NS_ENSURE_ARG(aNode);
  NS_ENSURE_ARG_POINTER(aModelAssocWithNode);

  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
  nsCOMPtr<nsIXTFElement> xtfele = do_QueryInterface(aNode);

  nsAutoString namespaceURI;
  aNode->GetNamespaceURI(namespaceURI);

  // If the node is in the XForms namespace and XTF based, then it should
  //   be able to be handled by GetModel.  Otherwise it is probably an instance
  //   node in a instance document.
  if ((namespaceURI.EqualsLiteral(NS_NAMESPACE_XFORMS)) && xtfele) {
    nsCOMPtr<nsIDOMNode> modelNode = nsXFormsUtils::GetModel(element);

    if (modelNode && (modelNode == aModel)) {
      *aModelAssocWithNode = PR_TRUE;
    }
    else {
      *aModelAssocWithNode = PR_FALSE;
    }
  }
  else {
    // We are assuming that if the node coming in isn't a proper XForms element,
    //   then it is an instance element in an instance doc.  Now we just have
    //   to determine if the given model contains this instance document.
    nsCOMPtr<nsIDOMDocument> document;
    aNode->GetOwnerDocument(getter_AddRefs(document));
    *aModelAssocWithNode = PR_FALSE;

    // Guess that we'd better make sure that it is a model
    nsCOMPtr<nsIXFormsModelElement> modelEle = do_QueryInterface(aModel);
    if (modelEle) {
      // OK, we know that this is a model element.  So now we have to go
      //   instance element by instance element and find the associated
      //   document.  If it is equal to the document that contains aNode,
      //   then aNode is associated with this aModel element and we can return
      //   true.
      nsCOMPtr<nsIDOMNodeList> children;
      aModel->GetChildNodes(getter_AddRefs(children));
    
      if (!children)
        return NS_OK;
    
      PRUint32 childCount = 0;
      children->GetLength(&childCount);
    
      nsCOMPtr<nsIDOMNode> node;
      nsCOMPtr<nsIInstanceElementPrivate> instElement;
      nsCOMPtr<nsIDOMDocument> instDocument;
    
      for (PRUint32 i = 0; i < childCount; ++i) {
        children->Item(i, getter_AddRefs(node));
        NS_ASSERTION(node, "incorrect NodeList length?");
    
        instElement = do_QueryInterface(node);
        if (!instElement)
          continue;
    
        instElement->GetDocument(getter_AddRefs(instDocument));
        if (instDocument) {
          if (instDocument == document) {
            *aModelAssocWithNode = PR_TRUE;
            break;
          }
        }
      }
    }

  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsUtilitiesService::GetInstanceDocumentRoot(const nsAString& aID,
                                           nsIDOMNode *aModelNode,
                                           nsIDOMNode **aInstanceRoot)
{
  NS_ENSURE_ARG(aModelNode);
  NS_ENSURE_ARG_POINTER(aInstanceRoot);
  nsresult rv = NS_ERROR_FAILURE;

  if (!aID.IsEmpty()) {
    nsCOMPtr<nsIXFormsModelElement> modelElement = do_QueryInterface(aModelNode);
    nsCOMPtr<nsIDOMDocument> doc;
    rv = modelElement->GetInstanceDocument(aID, getter_AddRefs(doc));
    NS_ENSURE_SUCCESS(rv, rv);
   
    nsCOMPtr<nsIDOMElement> element;
    rv = doc->GetDocumentElement(getter_AddRefs(element));

    if (element) {
      nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
      NS_IF_ADDREF(*aInstanceRoot = element);
    }

  }

  return rv;
}

/* Gotta do this via the service since we don't want transformiix to require
 * any of the new extensions, like schema-validation
 */
NS_IMETHODIMP 
nsXFormsUtilitiesService::ValidateString(const nsAString & aValue, 
                                         const nsAString & aType, 
                                         const nsAString & aNamespace,
                                         PRBool *aResult)
{

  // XXX TODO This function needs to call the XForms validator layer from
  //   bug 274083 when it goes into the build.

#if 0
  nsresult rv = NS_ERROR_FAILURE;
  nsXFormsSchemaValidator *validator = new nsXFormsSchemaValidator();
  *aResult = validator->ValidateString(aValue, aType, aNamespace);
  return rv;
#endif

  return NS_ERROR_NOT_IMPLEMENTED;
}
