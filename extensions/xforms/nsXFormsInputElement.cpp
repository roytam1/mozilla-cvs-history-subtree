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

#include "nsIXTFXMLVisual.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Node.h"
#include "nsIDOMElement.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIXTFXMLVisualWrapper.h"
#include "nsIDOMDocument.h"
#include "nsXFormsControl.h"
#include "nsISchema.h"
#include "nsXFormsModelElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsXFormsAtoms.h"
#include "nsAutoPtr.h"
#include "nsIDOMXPathResult.h"

static const nsIID sScriptingIIDs[] = {
  NS_IDOMELEMENT_IID,
  NS_IDOMEVENTTARGET_IID,
  NS_IDOM3NODE_IID
};

class nsXFormsInputElement : public nsIXTFXMLVisual,
                             public nsXFormsControl
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXTFXMLVISUAL
  NS_DECL_NSIXTFELEMENT

  // nsXFormsControl
  virtual NS_HIDDEN_(void) Refresh();

private:
  nsCOMPtr<nsIDOMHTMLInputElement> mInput;
};

NS_IMPL_ISUPPORTS2(nsXFormsInputElement, nsIXTFXMLVisual, nsIXTFElement)

// nsIXTFXMLVisual

NS_IMETHODIMP
nsXFormsInputElement::OnCreated(nsIXTFXMLVisualWrapper *aWrapper)
{
  mWrapper = aWrapper;

  nsCOMPtr<nsIDOMElement> node;
  mWrapper->GetElementNode(getter_AddRefs(node));
  nsCOMPtr<nsIDOMDocument> domDoc;
  node->GetOwnerDocument(getter_AddRefs(domDoc));

  nsCOMPtr<nsIDOMElement> inputElement;
  domDoc->CreateElementNS(NS_LITERAL_STRING("http://www.w3.org/1999/xhtml"),
                          NS_LITERAL_STRING("input"),
                          getter_AddRefs(inputElement));

  mInput = do_QueryInterface(inputElement);
  NS_ENSURE_TRUE(mInput, NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::GetVisualContent(nsIDOMElement **aElement)
{
  NS_ADDREF(*aElement = mInput);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::GetDisplayType(PRUint32 *aDisplayType)
{
  *aDisplayType = nsIXTFXMLVisual::DISPLAY_INLINE;
  return NS_OK;
}

// nsIXTFElement

NS_IMETHODIMP
nsXFormsInputElement::OnDestroyed()
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::GetElementType(PRUint32 *aType)
{
  *aType = ELEMENT_TYPE_XML_VISUAL;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::GetIsAttributeHandler(PRBool *aIsHandler)
{
  *aIsHandler = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::GetScriptingInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  return CloneScriptingInterfaces(sScriptingIIDs,
                                  NS_ARRAY_LENGTH(sScriptingIIDs),
                                  aCount, aArray);
}

NS_IMETHODIMP
nsXFormsInputElement::GetNotificationMask(PRUint32 *aMask)
{
  *aMask = (nsIXTFElement::NOTIFY_WILL_SET_ATTRIBUTE |
            nsIXTFElement::NOTIFY_ATTRIBUTE_SET);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillChangeDocument(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::DocumentChanged(nsISupports *aNewDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillChangeParent(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::ParentChanged(nsISupports *aNewParent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillInsertChild(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::ChildInserted(nsISupports *aChild, PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillAppendChild(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::ChildAppended(nsISupports *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillRemoveChild(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::ChildRemoved(PRUint32 aIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillSetAttribute(nsIAtom *aName, const nsAString &aValue)
{
  if (aName == nsXFormsAtoms::bind || aName == nsXFormsAtoms::ref) {
    nsCOMPtr<nsIDOMElement> bindElement;
    nsXFormsModelElement *model = GetModelAndBind(getter_AddRefs(bindElement));
    if (model)
      model->RemoveFormControl(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::AttributeSet(nsIAtom *aName, const nsAString &aValue)
{
  if (aName == nsXFormsAtoms::bind || aName == nsXFormsAtoms::ref) {
    Refresh();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::WillUnsetAttribute(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::AttributeUnset(nsIAtom *aName)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputElement::DoneAddingChildren()
{
  return NS_OK;
}

void
nsXFormsInputElement::Refresh()
{
  if (!mInput)
    return;

  nsRefPtr<nsXFormsModelElement> model;
  nsCOMPtr<nsIDOMElement> bindElement;
  nsCOMPtr<nsIDOMXPathResult> result =
    EvaluateBinding(nsIDOMXPathResult::STRING_TYPE,
                    getter_AddRefs(model), getter_AddRefs(bindElement));

  if (model) {
    model->AddFormControl(this);

    if (result) {
      nsAutoString nodeValue;
      result->GetStringValue(nodeValue);

      nsCOMPtr<nsISchemaType> type = model->GetTypeForControl(this);
      nsCOMPtr<nsISchemaBuiltinType> biType = do_QueryInterface(type);
      PRUint16 typeValue = nsISchemaBuiltinType::BUILTIN_TYPE_STRING;

      if (biType)
        biType->GetBuiltinType(&typeValue);

      if (typeValue == nsISchemaBuiltinType::BUILTIN_TYPE_BOOLEAN) {
        mInput->SetAttribute(NS_LITERAL_STRING("type"),
                             NS_LITERAL_STRING("checkbox"));

        mInput->SetChecked(nodeValue.EqualsASCII("true") ||
                           nodeValue.EqualsASCII("1"));
      } else {
        mInput->RemoveAttribute(NS_LITERAL_STRING("type"));
        mInput->SetValue(nodeValue);
      }
    }
  }
}

NS_HIDDEN_(nsresult)
NS_NewXFormsInputElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsInputElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
