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
 *  Allan Beaufour <abeaufour@novell.com>
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
#include "nsIDOMXPathResult.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathExpression.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIScriptGlobalObject.h"
#include "nsIContent.h"
#include "nsXFormsControl.h"
#include "nsXFormsTypes.h"
#include "nsXFormsXPathParser.h"
#include "nsXFormsXPathAnalyzer.h"

#include "nsISchemaLoader.h"
#include "nsAutoPtr.h"

#ifdef DEBUG_beaufour
#include "nsIDOMSerializer.h"
#endif

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID,
  NS_IXFORMSMODELELEMENT_IID
};

struct EventData
{
  const char *name;
  PRBool      canCancel;
  PRBool      canBubble;
};

enum {
  eEvent_ModelConstruct,
  eEvent_ModelConstructDone,
  eEvent_Ready,
  eEvent_ModelDestruct,
  eEvent_Rebuild,
  eEvent_Refresh,
  eEvent_Revalidate,
  eEvent_Recalculate,
  eEvent_Reset,
  eEvent_BindingException,
  eEvent_LinkException,
  eEvent_LinkError,
  eEvent_ComputeExeception
};

static const EventData sModelEvents[] = {
  { "xforms-model-construct",      PR_FALSE, PR_TRUE },
  { "xforms-model-construct-done", PR_FALSE, PR_TRUE },
  { "xforms-ready",                PR_FALSE, PR_TRUE },
  { "xforms-model-destruct",       PR_FALSE, PR_TRUE },
  { "xforms-rebuild",              PR_TRUE,  PR_TRUE },
  { "xforms-refresh",              PR_TRUE,  PR_TRUE },
  { "xforms-revalidate",           PR_TRUE,  PR_TRUE },
  { "xforms-recalculate",          PR_TRUE,  PR_TRUE },
  { "xforms-reset",                PR_TRUE,  PR_TRUE },
  { "xforms-binding-exception",    PR_FALSE, PR_TRUE },
  { "xforms-link-exception",       PR_FALSE, PR_TRUE },
  { "xforms-link-error",           PR_FALSE, PR_TRUE },
  { "xforms-compute-exception",    PR_FALSE, PR_TRUE }
};

static nsIAtom* sModelPropsList[eModel__count];

struct nsXFormsModelElement::ModelItemProperties
{
  nsCOMPtr<nsIDOMXPathExpression> properties[eModel__count];
};

nsXFormsModelElement::nsXFormsModelElement()
  : mContent(nsnull),
    mSchemaCount(0),
    mInstanceDataLoaded(PR_FALSE)
{
}

NS_IMPL_ADDREF(nsXFormsModelElement)
NS_IMPL_RELEASE(nsXFormsModelElement)

NS_INTERFACE_MAP_BEGIN(nsXFormsModelElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFGenericElement)
  NS_INTERFACE_MAP_ENTRY(nsIXTFPrivate)
  NS_INTERFACE_MAP_ENTRY(nsIXFormsModelElement)
  NS_INTERFACE_MAP_ENTRY(nsISchemaLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXTFElement)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsXFormsModelElement::OnDestroyed()
{
  nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(mContent);
  NS_ASSERTION(receiver, "xml elements must be event receivers");

  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  receiver->GetSystemEventGroup(getter_AddRefs(systemGroup));
  NS_ASSERTION(systemGroup, "system event group must exist");
  
  nsCOMPtr<nsIDOM3EventTarget> targ = do_QueryInterface(mContent);
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(sModelEvents); ++i) {
    targ->RemoveGroupedEventListener(NS_ConvertUTF8toUTF16(sModelEvents[i].name),
                                     this, PR_FALSE, systemGroup);
  }

  RemoveModelFromDocument();

  mContent = nsnull;
  return NS_OK;
}

void
nsXFormsModelElement::RemoveModelFromDocument()
{
  // Find out if we are handling the model-construct-done for this document.
  nsIDocument *doc = mContent->GetDocument();
  nsIScriptGlobalObject *window = nsnull;
  if (doc)
    window = doc->GetScriptGlobalObject();
  nsCOMPtr<nsIDOMEventTarget> targ2 = do_QueryInterface(window);
  if (targ2) {
    nsVoidArray *models = NS_STATIC_CAST(nsVoidArray*,
                          doc->GetProperty(nsXFormsAtoms::modelListProperty));

    if (models) {
      if (models->SafeElementAt(0) == this) {
        nsXFormsModelElement *next =
          NS_STATIC_CAST(nsXFormsModelElement*, models->SafeElementAt(1));
        if (next) {
          targ2->AddEventListener(NS_LITERAL_STRING("load"), next, PR_TRUE);
        }

        targ2->RemoveEventListener(NS_LITERAL_STRING("load"), this, PR_TRUE);
      }

      models->RemoveElement(this);
    }
  }
}

NS_IMETHODIMP
nsXFormsModelElement::GetElementType(PRUint32 *aType)
{
  *aType = ELEMENT_TYPE_GENERIC_ELEMENT;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetIsAttributeHandler(PRBool *aIsHandler)
{
  *aIsHandler = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::GetScriptingInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  return CloneScriptingInterfaces(sScriptingIIDs,
                                  NS_ARRAY_LENGTH(sScriptingIIDs),
                                  aCount, aArray);
}

NS_IMETHODIMP
nsXFormsModelElement::GetNotificationMask(PRUint32 *aMask)
{
  *aMask = (nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT |
            nsIXTFElement::NOTIFY_DOCUMENT_CHANGED);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillChangeDocument(nsISupports* aNewDocument)
{
  RemoveModelFromDocument();
  return NS_OK;
}

static void
DeleteVoidArray(void    *aObject,
                nsIAtom *aPropertyName,
                void    *aPropertyValue,
                void    *aData)
{
  delete NS_STATIC_CAST(nsVoidArray*, aPropertyValue);
}

NS_IMETHODIMP
nsXFormsModelElement::DocumentChanged(nsISupports* aNewDocument)
{
  // Add this model to the document's model list.  If this is the first
  // model to be created, register an onload handler so that we can
  // do model-construct-done notifications.

  if (!aNewDocument)
    return NS_OK;

  nsIDocument *doc = mContent->GetDocument();

  nsVoidArray *models = NS_STATIC_CAST(nsVoidArray*,
                  doc->GetProperty(nsXFormsAtoms::modelListProperty));

  if (!models) {
    models = new nsVoidArray(16);
    doc->SetProperty(nsXFormsAtoms::modelListProperty,
                     models, DeleteVoidArray);

    nsIScriptGlobalObject *window = doc->GetScriptGlobalObject();

    nsCOMPtr<nsIDOMEventTarget> targ = do_QueryInterface(window);
    targ->AddEventListener(NS_LITERAL_STRING("load"), this, PR_TRUE);
  }

  models->AppendElement(this);
                                 
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
nsXFormsModelElement::WillSetAttribute(nsIAtom *aName,
                                       const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::AttributeSet(nsIAtom *aName, const nsAString &aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::WillUnsetAttribute(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::AttributeUnset(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::DoneAddingChildren()
{
  // We wait until all children are added to dispatch xforms-model-construct,
  // since the model may have an action handler for this event.

  nsresult rv = DispatchEvent(eEvent_ModelConstruct);
  NS_ENSURE_SUCCESS(rv, rv);

  // xforms-model-construct is not cancellable, so always proceed.
  // We continue here rather than doing this in HandleEvent since we know
  // it only makes sense to perform this default action once.

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
        DispatchEvent(eEvent_LinkException);  // this is a fatal error
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

  // XXX schema and external instance data loads should delay document onload

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

        // XXX We need to come up with a mechanism so that the content sink
        // can use xmlns declarations in effect for the <model> to resolve
        // tag prefixes in the external instance data.

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
          DispatchEvent(eEvent_LinkException);
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

  // It's ok to keep a weak pointer to mContent.  mContent will have an
  // owning reference to this object, so as long as we null out mContent in
  // OnDestroyed, it will always be valid.

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  mContent = content;

  NS_ASSERTION(mContent, "Wrapper is not an nsIContent, we'll crash soon");

  nsresult rv = mMDG.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// nsIXTFPrivate
NS_IMETHODIMP
nsXFormsModelElement::GetInner(nsISupports **aInner)
{
  NS_ENSURE_ARG_POINTER(aInner);
  NS_ADDREF(*aInner = NS_STATIC_CAST(nsIXFormsModelElement*, this));
  return NS_OK;
}

// nsIXFormsModelElement

NS_IMETHODIMP
nsXFormsModelElement::GetInstanceDocument(const nsAString& aInstanceID,
                                          nsIDOMDocument **aDocument)
{
  if (!mInstanceDocument)
    return NS_ERROR_FAILURE; // what sort of exception should this be?

  NS_ADDREF(*aDocument = mInstanceDocument);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Rebuild()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Rebuild()\n");
#endif

  // TODO: Clear graph and re-attach elements

  // 1 . Clear graph
  // mMDG.Clear();

  // 2. Re-attach all elements

  // 3. Rebuild graph
  return mMDG.Rebuild();
}

NS_IMETHODIMP
nsXFormsModelElement::Recalculate()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Recalculate()\n");
#endif
  
  nsXFormsMDGSet changedNodes;
  // TODO: Handle changed nodes. That is, dispatch events, etc.
  
  return mMDG.Recalculate(changedNodes);
}

NS_IMETHODIMP
nsXFormsModelElement::Revalidate()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Revalidate()\n");
#endif

#ifdef DEBUG_beaufour
  // Dump instance document to stdout
  nsresult rv;
  nsCOMPtr<nsIDOMSerializer> serializer(do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: Should use SerializeToStream and write directly to stdout...
  nsAutoString instanceString;
  rv = serializer->SerializeToString(mInstanceDocument, instanceString);
  NS_ENSURE_SUCCESS(rv, rv);
  
  printf("Instance data:\n%s\n", NS_ConvertUCS2toUTF8(instanceString).get());
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Refresh()
{
#ifdef DEBUG
  printf("nsXFormsModelElement::Refresh()\n");
#endif

  return NS_OK;
}

// nsISchemaLoadListener

NS_IMETHODIMP
nsXFormsModelElement::OnLoad(nsISchema* aSchema)
{
  mSchemas.AppendObject(aSchema);
  if (IsComplete()) {
    nsresult rv = FinishConstruction();
    NS_ENSURE_SUCCESS(rv, rv);

    DispatchEvent(eEvent_Refresh);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::OnError(PRInt32 aStatus, const nsAString &aStatusMessage)
{
  DispatchEvent(eEvent_LinkException);
  return NS_OK;
}

// nsIDOMEventListener

NS_IMETHODIMP
nsXFormsModelElement::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMNSUIEvent> evt = do_QueryInterface(aEvent);
  NS_ASSERTION(evt, "event should implement nsIDOMNSUIEvent");

  PRBool defaultPrevented;
  evt->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented)
    return NS_OK;

  nsAutoString type;
  aEvent->GetType(type);

  if (type.EqualsLiteral("xforms-refresh")) {
    // refresh all of our form controls
    PRInt32 controlCount = mFormControls.Count();
    for (PRInt32 i = 0; i < controlCount; ++i) {
      NS_STATIC_CAST(nsXFormsControl*, mFormControls[i])->Refresh();
    }
  } else if (type.EqualsLiteral("xforms-revalidate")) {
    Revalidate();
  } else if (type.EqualsLiteral("xforms-recalculate")) {
    Recalculate();
  } else if (type.EqualsLiteral("xforms-rebuild")) {
    Rebuild();
  } else if (type.EqualsLiteral("xforms-reset")) {
#ifdef DEBUG
    printf("nsXFormsModelElement::Reset()\n");
#endif    
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Load(nsIDOMEvent* aEvent)
{
  // This could be a load event for us or for the document.
  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  nsCOMPtr<nsIDocument> document = do_QueryInterface(target);
  if (document) {
    // The document has finished loading; that means that all of the models
    // in it are initialized.  Fire the model-construct-done event to each
    // model.

    nsVoidArray *models = NS_STATIC_CAST(nsVoidArray*,
                      document->GetProperty(nsXFormsAtoms::modelListProperty));

    NS_ASSERTION(models, "models list is empty!");
    for (PRInt32 i = 0; i < models->Count(); ++i) {
      NS_STATIC_CAST(nsXFormsModelElement*, models->ElementAt(i))
        ->DispatchEvent(eEvent_ModelConstructDone);
    }
  } else {
    mInstanceDataLoaded = PR_TRUE;
    if (IsComplete()) {
      nsresult rv = FinishConstruction();
      NS_ENSURE_SUCCESS(rv, rv);

      DispatchEvent(eEvent_Refresh);
    }
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
  DispatchEvent(eEvent_LinkException);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsModelElement::Error(nsIDOMEvent* aEvent)
{
  DispatchEvent(eEvent_LinkException);
  return NS_OK;
}

// internal methods

nsresult
nsXFormsModelElement::FinishConstruction()
{
  // 3. if applicable, initialize P3P

  // 4. construct instance data from initial instance data.  apply all
  // <bind> elements in document order.

  // The instance data is in our mInstanceDocument.

  PRUint32 childCount = mContent->GetChildCount();
  nsCOMPtr<nsIDOMXPathEvaluator> xpath = do_QueryInterface(mInstanceDocument);

  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *child = mContent->GetChildAt(i);
    nsINodeInfo *ni = child->GetNodeInfo();

    if (ni && ni->Equals(nsXFormsAtoms::bind, kNameSpaceID_XForms)) {
      if (!ProcessBind(xpath, child)) {
        DispatchEvent(eEvent_BindingException);
        return NS_OK;
      }
    }
  }

  // 5. dispatch xforms-rebuild, xforms-recalculate, xforms-revalidate

  // First hook up our event listener so we invoke the default action for
  // these events.  We listen on the system event group so that we can check
  // whether preventDefault() was called by any content listeners.

  nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(mContent);
  NS_ASSERTION(receiver, "xml elements must be event receivers");

  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  receiver->GetSystemEventGroup(getter_AddRefs(systemGroup));
  NS_ASSERTION(systemGroup, "system event group must exist");
  
  nsCOMPtr<nsIDOM3EventTarget> targ = do_QueryInterface(mContent);
  for (unsigned int j = 0; j < NS_ARRAY_LENGTH(sModelEvents); ++j) {
    targ->AddGroupedEventListener(NS_ConvertUTF8toUTF16(sModelEvents[j].name),
                                  this, PR_FALSE, systemGroup);
  }

  DispatchEvent(eEvent_Rebuild);
  DispatchEvent(eEvent_Recalculate);
  DispatchEvent(eEvent_Revalidate);

  // We're done initializing this model.

  return NS_OK;
}

static void
ReleaseExpr(void    *aElement,
            nsIAtom *aPropertyName,
            void    *aPropertyValue,
            void    *aData)
{
  nsIDOMXPathExpression *expr = NS_STATIC_CAST(nsIDOMXPathExpression*,
                                               aPropertyValue);

  NS_RELEASE(expr);
}

PRBool
nsXFormsModelElement::ProcessBind(nsIDOMXPathEvaluator *aEvaluator,
                                  nsIContent *aBindElement)
{
  // Get the expression for the nodes that this <bind> applies to.
  nsAutoString expr;
  aBindElement->GetAttr(kNameSpaceID_None, nsXFormsAtoms::nodeset, expr);
  if (expr.IsEmpty())
    return PR_TRUE;

  nsCOMPtr<nsIDOMXPathNSResolver> resolver;
  aEvaluator->CreateNSResolver(nsCOMPtr<nsIDOMNode>(do_QueryInterface(aBindElement)),
                               getter_AddRefs(resolver));

  // Get the model item properties specified by this <bind>.
  nsCOMPtr<nsIDOMXPathExpression> props[eModel__count];
  nsAutoString exprStrings[eModel__count];
  PRInt32 propCount = 0;
  nsresult rv = NS_OK;

  for (int i = 0; i < eModel__count; ++i) {
    if (aBindElement->GetAttr(kNameSpaceID_None, sModelPropsList[i], exprStrings[i]) != NS_CONTENT_ATTR_NOT_THERE) {

      rv = aEvaluator->CreateExpression(exprStrings[i], resolver,
                                        getter_AddRefs(props[i]));
      if (NS_FAILED(rv))
        return PR_FALSE;

      ++propCount;
    }
  }

  if (propCount == 0)
    return PR_TRUE;  // successful, but nothing to do

  nsCOMPtr<nsIDOMXPathResult> result;
  nsCOMPtr<nsIDOMElement> docElement;
  mInstanceDocument->GetDocumentElement(getter_AddRefs(docElement));
  rv = aEvaluator->Evaluate(expr, docElement, resolver,
                            nsIDOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
                            nsnull, getter_AddRefs(result));
  if (NS_FAILED(rv))
    return PR_FALSE;

  PRUint32 snapLen;
  rv = result->GetSnapshotLength(&snapLen);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsXFormsMDGSet set;
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 contextPosition = 1;
  for (PRUint32 snapItem = 0; snapItem < snapLen; ++snapItem) {
    rv = result->SnapshotItem(snapItem, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!node) {
      NS_WARNING("nsXFormsModelElement::ProcessBind(): Empty node in result set.");
      continue;
    }
    
    nsXFormsXPathParser parser;
    nsXFormsXPathAnalyzer analyzer(aEvaluator, resolver);
    
    // We must check whether the properties already exist on the node.
    for (int j = 0; j < eModel__count; ++j) {
      if (props[j]) {
        nsCOMPtr<nsIContent> content = do_QueryInterface(node, &rv);

        if (NS_FAILED(rv)) {
          NS_WARNING("nsXFormsModelElement::ProcessBind(): Node is not IContent!\n");
          continue;
        }

        nsIDOMXPathExpression *expr = props[j];
        NS_ADDREF(expr);

        // Set property
        rv = content->SetProperty(sModelPropsList[j], expr, ReleaseExpr);
        if (rv == NS_PROPTABLE_PROP_OVERWRITTEN) {
          return PR_FALSE;
        }
        
        // Get node dependencies
        nsAutoPtr<nsXFormsXPathNode> xNode(parser.Parse(exprStrings[j]));
        set.Clear();
        rv = analyzer.Analyze(node, xNode, expr, &exprStrings[j], &set);
        NS_ENSURE_SUCCESS(rv, rv);
        
        // Insert into MDG
        rv = mMDG.AddMIP((ModelItemPropName) j, expr, &set, parser.UsesDynamicFunc(),
                         node, contextPosition++, snapLen);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  
  return PR_TRUE;
}

nsresult
nsXFormsModelElement::DispatchEvent(unsigned int aEvent)
{
  const EventData *data = &sModelEvents[aEvent];
  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMDocumentEvent> doc = do_QueryInterface(mContent->GetDocument());
  doc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  event->InitEvent(NS_ConvertUTF8toUTF16(data->name),
                   data->canBubble, data->canCancel);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mContent);
  PRBool cancelled;
  return target->DispatchEvent(event, &cancelled);
}

already_AddRefed<nsISchemaType>
nsXFormsModelElement::GetTypeForControl(nsXFormsControl *aControl)
{
  return nsnull;
}

void
nsXFormsModelElement::AddFormControl(nsXFormsControl *aControl)
{
  if (mFormControls.IndexOf(aControl) == -1)
    mFormControls.AppendElement(aControl);
}

void
nsXFormsModelElement::RemoveFormControl(nsXFormsControl *aControl)
{
  mFormControls.RemoveElement(aControl);
}

/* static */ void
nsXFormsModelElement::Startup()
{
  sModelPropsList[eModel_type] = nsXFormsAtoms::type;
  sModelPropsList[eModel_readonly] = nsXFormsAtoms::readonly;
  sModelPropsList[eModel_required] = nsXFormsAtoms::required;
  sModelPropsList[eModel_relevant] = nsXFormsAtoms::relevant;
  sModelPropsList[eModel_calculate] = nsXFormsAtoms::calculate;
  sModelPropsList[eModel_constraint] = nsXFormsAtoms::constraint;
  sModelPropsList[eModel_p3ptype] = nsXFormsAtoms::p3ptype;
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
