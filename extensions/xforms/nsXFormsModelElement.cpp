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

#include "nsXFormsModelElement.h"
#include "nsIXTFGenericElementWrapper.h"
#include "nsMemory.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMElement.h"
#include "nsIDOM3Node.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsXFormsAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsINodeInfo.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMEventReceiver.h"

#include "nsISchemaLoader.h"
#include "nsAutoPtr.h"

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID,
  NS_IXFORMSMODELELEMENT_IID
};

// non-cancellable event
#define EVENT_HELPER_NC(name, event, bubble) \
inline nsresult \
Dispatch##name##Event(nsXFormsModelElement *elt) \
{ \
  PRBool defaultPrevented; \
  return elt->DispatchEvent("xforms-" event, bubble, PR_FALSE, &defaultPrevented); \
}

// cancellable event
#define EVENT_HELPER(name, event, bubble) \
inline nsresult \
Dispatch##name##Event(nsXFormsModelElement *elt, PRBool *aDefaultPrevented) \
{ return elt->DispatchEvent("xforms-" event, bubble, PR_TRUE, aDefaultPrevented); }

// Initialization events
EVENT_HELPER_NC(ModelConstruct, "model-construct", PR_TRUE)
EVENT_HELPER_NC(ModelConstructDone, "model-construct-done", PR_TRUE)
EVENT_HELPER_NC(Ready, "ready", PR_TRUE)
EVENT_HELPER_NC(ModelDestruct, "model-destruct", PR_TRUE)

// Interaction events
EVENT_HELPER(Rebuild, "rebuild", PR_TRUE)
EVENT_HELPER(Refresh, "refresh", PR_TRUE)
EVENT_HELPER(Revalidate, "revalidate", PR_TRUE)
EVENT_HELPER(Recalculate, "recalculate", PR_TRUE)
EVENT_HELPER(Reset, "reset", PR_TRUE)

// Notification events
EVENT_HELPER_NC(SubmitError, "submit-error", PR_TRUE)

// Error events
EVENT_HELPER_NC(BindingException, "binding-exception", PR_TRUE)
EVENT_HELPER_NC(LinkException, "link-exception", PR_TRUE)
EVENT_HELPER_NC(LinkError, "link-error", PR_TRUE)
EVENT_HELPER_NC(ComputeException, "compute-exception", PR_TRUE)

nsXFormsModelElement::nsXFormsModelElement()
  : mSchemaCount(0),
    mInstanceDataLoaded(PR_FALSE)
{
}

NS_IMPL_ADDREF(nsXFormsModelElement)
NS_IMPL_RELEASE(nsXFormsModelElement)

NS_INTERFACE_MAP_BEGIN(nsXFormsModelElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFGenericElement)
  NS_INTERFACE_MAP_ENTRY(nsIXFormsModelElement)
  NS_INTERFACE_MAP_ENTRY(nsISchemaLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXTFElement)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsXFormsModelElement::OnDestroyed()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetElementType(PRUint32 *aType)
{
  *aType = ELEMENT_TYPE_GENERIC_ELEMENT;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetScriptingInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  PRUint32 count = NS_ARRAY_LENGTH(sScriptingIIDs);

  nsIID **iids = NS_STATIC_CAST(nsIID**,
                                nsMemory::Alloc(count * sizeof(nsIID*)));
  if (!iids) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRUint32 i = 0; i < count; ++i) {
    iids[i] = NS_STATIC_CAST(nsIID*,
                           nsMemory::Clone(&sScriptingIIDs[i], sizeof(nsIID)));

    if (!iids[i]) {
      for (PRUint32 j = 0; j < i; ++j)
        nsMemory::Free(iids[j]);
      nsMemory::Free(iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aArray = iids;
  *aCount = count;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillChangeDocument(nsISupports* aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::DocumentChanged(nsISupports* aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillChangeParent(nsISupports* aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::ParentChanged(nsISupports* aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillInsertChild(nsISupports* aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::ChildInserted(nsISupports* aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillAppendChild(nsISupports* aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::ChildAppended(nsISupports* aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillRemoveChild(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::ChildRemoved(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::SetAttribute(const nsAString &aName,
                                   const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::UnsetAttribute(const nsAString& aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetAttribute(const nsAString& aName, nsAString& aValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::HasAttribute(const nsAString& aName, PRBool* aHasAttr)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetAttributeCount(PRUint32* aCount)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetAttributeNameAt(PRUint32 aIndex, nsAString& aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::DoneAddingChildren()
{
  // We wait until all children are added to dispatch xforms-model-construct,
  // since the model may have an action handler for this event.

  nsresult rv = DispatchModelConstructEvent(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // xforms-model-construct is not cancellable, so always proceed.
  // (XForms 4.2.1)
  // 1. load xml schemas

  nsAutoString schemaList;
  mContent->GetAttr(kNameSpaceID_None, nsXFormsAtoms::schema, schemaList);
  if (!schemaList.IsEmpty()) {
    nsCOMPtr<nsISchemaLoader> loader = do_GetService(NS_SCHEMALOADER_CONTRACTID);
    NS_ENSURE_TRUE(loader, NS_ERROR_FAILURE);

    // Parse the space-separated list.
    PRUint32 offset = 0;
    nsRefPtr<nsIURI> baseURI = mContent->GetBaseURI();

    while (1) {
      ++mSchemaCount;
      PRInt32 index = schemaList.FindChar(PRUnichar(' '), offset);
      rv = loader->LoadAsync(Substring(schemaList, offset, index - offset),
                             baseURI, this);
      if (NS_FAILED(rv)) {
        DispatchLinkExceptionEvent(this);  // this is a fatal error
        return NS_OK;
      }
      if (index == -1)
        break;

      offset = index + 1;
    }
  }

  // 2. construct an XPath data model from inline or external initial instance
  // data.

  // XXX the spec says there can be any number of <instance> nodes, but
  // I can't see how it makes sense to have more than one per model.

  PRUint32 childCount = mContent->GetChildCount();
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *child = mContent->GetChildAt(i);
    nsINodeInfo *ni = child->GetNodeInfo();
    if (ni && ni->Equals(nsXFormsAtoms::instance, kNameSpaceID_XForms)) {
      // Create a document which will hold the live instance data.
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(child->GetDocument());
      nsCOMPtr<nsIDOMDOMImplementation> domImpl;
      domDoc->GetImplementation(getter_AddRefs(domImpl));
                              
      nsAutoString src;
      child->GetAttr(kNameSpaceID_None, nsXFormsAtoms::src, src);

      rv = domImpl->CreateDocument(EmptyString(), EmptyString(), nsnull,
                                   getter_AddRefs(mInstanceDocument));
      NS_ENSURE_SUCCESS(rv, rv);

      if (src.IsEmpty()) {
        // No src means we should use the inline instance data, using the
        // first child element of the instance node as the root.

        PRUint32 instanceChildCount = child->GetChildCount();
        nsCOMPtr<nsIDOMNode> root;

        for (PRUint32 j = 0; j < instanceChildCount; ++j) {
          nsIContent *node = child->GetChildAt(j);
          if (node->IsContentOfType(nsIContent::eELEMENT)) {
            root = do_QueryInterface(node);
            break;
          }
        }

        if (root) {
          nsCOMPtr<nsIDOMNode> newNode;
          rv = mInstanceDocument->ImportNode(root, PR_TRUE,
                                             getter_AddRefs(newNode));
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<nsIDOMNode> nodeReturn;
          rv = mInstanceDocument->AppendChild(newNode,
                                              getter_AddRefs(nodeReturn));
          NS_ENSURE_SUCCESS(rv, rv);

          mInstanceDataLoaded = PR_TRUE;
        }
      } else {
        // We're using external instance data, so we need to load
        // the data into a new document which becomes our live instance data.

        // Hook up load an error listeners so we'll know when the document
        // is done loading.

        nsCOMPtr<nsIDOMEventReceiver> rec =
          do_QueryInterface(mInstanceDocument);
        rec->AddEventListenerByIID(this, NS_GET_IID(nsIDOMLoadListener));

        nsCOMPtr<nsIDOMXMLDocument> xmlDoc =
          do_QueryInterface(mInstanceDocument);

        PRBool success;
        xmlDoc->Load(src, &success);
        if (!success) {
          DispatchLinkExceptionEvent(this);
          return NS_OK;
        }
      }

      break;
    }
  }

  if (IsComplete()) {
    return FinishConstruction();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::OnCreated(nsIXTFGenericElementWrapper *aWrapper)
{
  nsCOMPtr<nsIDOMElement> node;
  aWrapper->GetElementNode(getter_AddRefs(node));
  mContent = do_QueryInterface(node);

  NS_ASSERTION(mContent, "Wrapper is not an nsIContent, we'll crash soon");

  aWrapper->SetShouldHandleAttributes(PR_TRUE);

  return NS_OK;
}


// nsIXFormsModelElement

NS_IMETHODIMP
nsXFormsModelElement::GetInstanceDocument(const nsAString& aInstanceID,
                                          nsIDOMDocument **aDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Rebuild()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Recalculate()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Revalidate()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Refresh()
{
  return NS_OK;
}

// nsISchemaLoadListener

NS_IMETHODIMP
nsXFormsModelElement::OnLoad(nsISchema* aSchema)
{
  mSchemas.AppendObject(aSchema);
  if (IsComplete()) {
    return FinishConstruction();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::OnError(PRInt32 aStatus, const nsAString &aStatusMessage)
{
  DispatchLinkExceptionEvent(this);
  return NS_OK;
}

// nsIDOMEventListener

NS_IMETHODIMP
nsXFormsModelElement::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Load(nsIDOMEvent* aEvent)
{
  mInstanceDataLoaded = PR_TRUE;
  if (IsComplete()) {
    return FinishConstruction();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::BeforeUnload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Unload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Abort(nsIDOMEvent* aEvent)
{
  DispatchLinkExceptionEvent(this);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Error(nsIDOMEvent* aEvent)
{
  DispatchLinkExceptionEvent(this);
  return NS_OK;
}

// internal methods

nsresult
nsXFormsModelElement::FinishConstruction()
{
  // 3. if applicable, initialize P3P

  // 4. construct instance data from initial instance data.  apply all
  // <bind> elements in document order.

  // 5. dispatch xforms-rebuild, xforms-recalculate, xforms-revalidate

  // mark this model as initialized

  return NS_OK;
}

nsresult
nsXFormsModelElement::DispatchEvent(const char *aEvent,
                                    PRBool aCanBubble, PRBool aCanCancel,
                                    PRBool *aDefaultPrevented)
{
  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMDocumentEvent> doc = do_QueryInterface(mContent->GetDocument());
  doc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  event->InitEvent(NS_ConvertUTF8toUTF16(aEvent), aCanBubble, aCanCancel);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mContent);
  return target->DispatchEvent(event, aDefaultPrevented);
}

nsresult
NS_NewXFormsModelElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsModelElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
