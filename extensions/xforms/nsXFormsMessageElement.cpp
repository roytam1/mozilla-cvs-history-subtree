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
 * Olli Pettay.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Olli Pettay <Olli.Pettay@helsinki.fi> (original author)
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

#include "nsXFormsAtoms.h"
#include "nsXFormsStubElement.h"
#include "nsXFormsActionElement.h"
#include "nsIXFormsActionModuleElement.h"

#include "nsIXTFXMLVisual.h"
#include "nsIXTFXMLVisualWrapper.h"

#include "nsIDOM3Node.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMDOMImplementation.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"

#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsITimer.h"
#include "nsIDocument.h"
#include "nsIBoxObject.h"
#include "nsIServiceManager.h"

#include "prmem.h"
#include "plbase64.h"
#include "nsIDOMSerializer.h"

#define EPHEMERAL_STYLE \
  "position:absolute;z-index:2147483647; \
  background:inherit;color:inherit; \
  border:inherit;visibility:visible;"

#define EPHEMERAL_STYLE_HIDDEN \
  "position:absolute;z-index:2147483647;visibility:hidden;"

#define MESSAGE_WINDOW_PROPERTIES \
  "location=false,scrollbars=yes"

#define SHOW_EPHEMERAL_TIMEOUT 250
#define HIDE_EPHEMERAL_TIMEOUT 3000

/**
 * Implementation of the XForms \<message\> element.
 *
 * @see http://www.w3.org/TR/xforms/slice10.html#action-info
 */
class nsXFormsMessageElement : public nsXFormsXMLVisualStub,
                               public nsIDOMEventListener,
                               public nsIXFormsActionModuleElement
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  
  // nsIXTFElement overrides
  NS_IMETHOD WillChangeDocument(nsIDOMDocument *aNewDocument);
  NS_IMETHOD OnDestroyed();
  
  // nsIXTFXMLVisual overrides
  NS_IMETHOD OnCreated(nsIXTFXMLVisualWrapper *aWrapper);

  // nsIXTFVisual overrides
  NS_IMETHOD GetVisualContent(nsIDOMElement **aElement);
  NS_IMETHOD GetInsertionPoint(nsIDOMElement **aElement);

  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIXFORMSACTIONMODULEELEMENT

  // Start the timer, which is used to set the message visible
  void StartEphemeral();
  // Set the message visible and start timer to hide it later.
  void ShowEphemeral();
  // Hide the ephemeral message.
  void HideEphemeral();

  nsXFormsMessageElement() : mElement(nsnull), mPosX(0), mPosY(0) {}
private:
  void CloneNode(nsIDOMNode* aSrc, nsIDOMNode** aTarget);

  nsCOMPtr<nsIDOMElement> mVisualElement;
  nsIDOMElement *mElement;

  // The position of the ephemeral message
  PRInt32 mPosX;
  PRInt32 mPosY;

  nsCOMPtr<nsITimer> mEphemeralTimer;
  nsCOMPtr<nsIDOMDocument> mDocument;
};

NS_IMPL_ADDREF_INHERITED(nsXFormsMessageElement, nsXFormsXMLVisualStub)
NS_IMPL_RELEASE_INHERITED(nsXFormsMessageElement, nsXFormsXMLVisualStub)

NS_INTERFACE_MAP_BEGIN(nsXFormsMessageElement)
  NS_INTERFACE_MAP_ENTRY(nsIXFormsActionModuleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXTFXMLVisual)
NS_INTERFACE_MAP_END_INHERITING(nsXFormsXMLVisualStub)

// nsIXTFXMLVisual

NS_IMETHODIMP
nsXFormsMessageElement::OnCreated(nsIXTFXMLVisualWrapper *aWrapper)
{
  aWrapper->SetNotificationMask(nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT);
  nsresult rv;

  nsCOMPtr<nsIDOMElement> node;
  rv = aWrapper->GetElementNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);

  mElement = node;
  NS_ASSERTION(mElement, "Wrapper is not an nsIDOMElement, we'll crash soon");

  nsCOMPtr<nsIDOMDocument> domDoc;
  mElement->GetOwnerDocument(getter_AddRefs(domDoc));
  domDoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                          NS_LITERAL_STRING("div"),
                          getter_AddRefs(mVisualElement));
  if (mVisualElement)
    mVisualElement->SetAttribute(NS_LITERAL_STRING("style"),
                                 NS_LITERAL_STRING(EPHEMERAL_STYLE_HIDDEN));

  return NS_OK;
}

// nsIXTFVisual

NS_IMETHODIMP
nsXFormsMessageElement::GetVisualContent(nsIDOMElement **aElement)
{
  NS_IF_ADDREF(*aElement = mVisualElement);
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsMessageElement::GetInsertionPoint(nsIDOMElement **aElement)
{
  NS_IF_ADDREF(*aElement = mVisualElement);
  return NS_OK;
}

// nsIXTFElement

NS_IMETHODIMP
nsXFormsMessageElement::WillChangeDocument(nsIDOMDocument *aNewDocument)
{
  if (mDocument) {
    if (mEphemeralTimer) {
      mEphemeralTimer->Cancel();
      mEphemeralTimer = nsnull;
    }

    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    if (doc) {
      nsXFormsMessageElement *msg =
        NS_STATIC_CAST(nsXFormsMessageElement*,
                       doc->GetProperty(nsXFormsAtoms::messageProperty));
      if (msg == this)
        doc->UnsetProperty(nsXFormsAtoms::messageProperty);
    }
  }

  mDocument = aNewDocument;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsMessageElement::OnDestroyed()
{
  mElement = nsnull;
  mVisualElement = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsMessageElement::HandleEvent(nsIDOMEvent* aEvent)
{
  if (!aEvent) 
    return NS_ERROR_INVALID_ARG;
  return HandleAction(aEvent, nsnull);
}

void
nsXFormsMessageElement::CloneNode(nsIDOMNode* aSrc, nsIDOMNode** aTarget)
{
  nsAutoString ns;
  nsAutoString localName;
  aSrc->GetNamespaceURI(ns);
  aSrc->GetLocalName(localName);
  // Clone the visual content of the <output>.
  // According to the XForms Schema it is enough 
  // to support <output> here.
  if (ns.EqualsLiteral(NS_NAMESPACE_XFORMS) &&
      localName.EqualsLiteral("output")) {
    nsCOMPtr<nsIXTFVisual> xtfEl(do_QueryInterface(aSrc));
    if (xtfEl) {
      nsCOMPtr<nsIDOMElement> visual;
      xtfEl->GetVisualContent(getter_AddRefs(visual));
      if (visual)
        visual->CloneNode(PR_TRUE, aTarget);
    }
    return;
  }

  // Clone other elements
  aSrc->CloneNode(PR_FALSE, aTarget);

  if (!*aTarget)
    return;

  // Add the new children
  nsCOMPtr<nsIDOMNode> tmp;
  nsCOMPtr<nsIDOMNodeList> childNodes;
  aSrc->GetChildNodes(getter_AddRefs(childNodes));

  PRUint32 count = 0;
  if (childNodes)
    childNodes->GetLength(&count);

  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMNode> child;
    childNodes->Item(i, getter_AddRefs(child));
    
    if (child) {
      nsCOMPtr<nsIDOMNode> clone;
      CloneNode(child, getter_AddRefs(clone));
      if (clone)
        (*aTarget)->AppendChild(clone, getter_AddRefs(tmp));
    }
  }
}

NS_IMETHODIMP
nsXFormsMessageElement::HandleAction(nsIDOMEvent* aEvent, 
                                     nsIXFormsActionElement *aParentAction)
{
  if (!mElement)
    return NS_OK;

  nsCOMPtr<nsIDOMDocument> doc;
  mElement->GetOwnerDocument(getter_AddRefs(doc));

  nsCOMPtr<nsIDOMDocumentView> dview(do_QueryInterface(doc));
  if (!dview)
    return NS_OK;

  nsCOMPtr<nsIDOMAbstractView> aview;
  dview->GetDefaultView(getter_AddRefs(aview));

  nsCOMPtr<nsIDOMWindowInternal> internal(do_QueryInterface(aview));
  if (!internal)
    return NS_OK;

  nsAutoString instanceData;
  PRBool hasBinding = nsXFormsUtils::GetSingleNodeBindingValue(mElement,
                                                               instanceData);

  nsAutoString level;
  mElement->GetAttribute(NS_LITERAL_STRING("level"), level);

  // ephemeral
  //XXX How to handle the following:
  //XXX <message level="ephemeral" src="http://mozilla.org"/>
  if (level.EqualsLiteral("ephemeral")) {
    nsCOMPtr<nsIDOMEventTarget> target;
    aEvent->GetTarget(getter_AddRefs(target));
    nsCOMPtr<nsIDOMElement> targetEl(do_QueryInterface(target));
    if (targetEl) {
      nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));
      if (nsDoc) {
        nsCOMPtr<nsIDOMNSUIEvent> uie(do_QueryInterface(aEvent));
        if (uie) {
          uie->GetPageX(&mPosX);
          uie->GetPageY(&mPosY);
          // Move the message a bit.
          mPosY += 10;
        } else {
          nsCOMPtr<nsIBoxObject> box;
          nsDoc->GetBoxObjectFor(targetEl, getter_AddRefs(box));
          if (box) {
            box->GetX(&mPosX);
            box->GetY(&mPosY);
            PRInt32 height;
            box->GetHeight(&height);
            
            // Message 10 pixels from the left-bottom corner of the target element.
            mPosX += 10;
            mPosY = mPosY + height - 10;
          }
        }

        if (hasBinding) {
          nsCOMPtr<nsIDOM3Node> visualElement3(do_QueryInterface(mVisualElement));
          if (visualElement3) {
            visualElement3->SetTextContent(instanceData);
          }
        }
        StartEphemeral();
      }
    }
  } else { // modal and modeless etc.
    nsAutoString src;
    mElement->GetAttribute(NS_LITERAL_STRING("src"), src);
    PRBool hasSrc = !src.IsEmpty();

    // Try to get the calculated size of the message element. It will
    // be used for the new window.
    //XXX This could be extended also for 'top', 'left' etc. properties.
    nsCOMPtr<nsIDOMViewCSS> cssView(do_QueryInterface(internal));
    nsCOMPtr<nsIDOMCSSStyleDeclaration> styles;
    PRInt32 computedWidth = 0;
    PRInt32 computedHeight = 0;
    if (cssView) {
      nsAutoString tmp;
      cssView->GetComputedStyle(mElement, tmp, getter_AddRefs(styles));
      if (styles) {
        nsCOMPtr<nsIDOMCSSValue> cssWidth;
        styles->GetPropertyCSSValue(NS_LITERAL_STRING("width"),
                                    getter_AddRefs(cssWidth));
        nsCOMPtr<nsIDOMCSSPrimitiveValue> pvalueWidth(do_QueryInterface(cssWidth));
        float width = 0;
        if (pvalueWidth) {
          PRUint16 type;
          pvalueWidth->GetPrimitiveType(&type);
          if (type == nsIDOMCSSPrimitiveValue::CSS_PX)
            pvalueWidth->GetFloatValue(type, &width);
        }

        nsCOMPtr<nsIDOMCSSValue> cssHeight;
        styles->GetPropertyCSSValue(NS_LITERAL_STRING("height"),
                                    getter_AddRefs(cssHeight));
        nsCOMPtr<nsIDOMCSSPrimitiveValue> pvalueHeight(do_QueryInterface(cssHeight));
        float height = 0;
        if (pvalueHeight) {
          PRUint16 type;
          pvalueHeight->GetPrimitiveType(&type);
          if (type == nsIDOMCSSPrimitiveValue::CSS_PX)
            pvalueHeight->GetFloatValue(type, &height);
        }
        computedWidth = NS_STATIC_CAST(PRInt32, width);
        computedHeight = NS_STATIC_CAST(PRInt32, height);
      }
    }

    nsAutoString options;
    options.AppendLiteral(MESSAGE_WINDOW_PROPERTIES);
    if (computedWidth > 0 && computedHeight > 0) {
      options.AppendLiteral(",innerWidth=");
      options.AppendInt(computedWidth);
      options.AppendLiteral(",innerHeight=");
      options.AppendInt(computedHeight);
    }

    if (!hasSrc) {
      nsresult rv;
      nsCOMPtr<nsIDOMDocument> ddoc;
      nsCOMPtr<nsIDOMDOMImplementation> domImpl;
      rv = doc->GetImplementation(getter_AddRefs(domImpl));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = domImpl->CreateDocument(EmptyString(), EmptyString(), nsnull,
                                   getter_AddRefs(ddoc));
      NS_ENSURE_SUCCESS(rv, rv);
      if (!ddoc)
        return NS_OK;

      nsCOMPtr<nsIDOMNode> tmp;
      nsCOMPtr<nsIDOMElement> htmlEl;
      rv = ddoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                                 NS_LITERAL_STRING("html"),
                                 getter_AddRefs(htmlEl));
      NS_ENSURE_SUCCESS(rv, rv);
      ddoc->AppendChild(htmlEl, getter_AddRefs(tmp));
      
      nsCOMPtr<nsIDOMElement> headEl;
      rv = ddoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                                 NS_LITERAL_STRING("head"),
                                 getter_AddRefs(headEl));
      NS_ENSURE_SUCCESS(rv, rv);
      htmlEl->AppendChild(headEl, getter_AddRefs(tmp));
      
      nsCOMPtr<nsIDOMElement> titleEl;
      rv = ddoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                                 NS_LITERAL_STRING("title"),
                                 getter_AddRefs(titleEl));
      NS_ENSURE_SUCCESS(rv, rv);
      headEl->AppendChild(titleEl, getter_AddRefs(tmp));
      nsCOMPtr<nsIDOM3Node> title3(do_QueryInterface(titleEl));
      if (title3)
        title3->SetTextContent(NS_LITERAL_STRING("[XForms]"));
      
      nsCOMPtr<nsIDOMElement> bodyEl;
      rv = ddoc->CreateElementNS(NS_LITERAL_STRING(NS_NAMESPACE_XHTML),
                                 NS_LITERAL_STRING("body"),
                                 getter_AddRefs(bodyEl));
      NS_ENSURE_SUCCESS(rv, rv);
      htmlEl->AppendChild(bodyEl, getter_AddRefs(tmp));

      // Copying content from original document to modeless message document.
      if (hasBinding) {
        nsCOMPtr<nsIDOM3Node> body3(do_QueryInterface(bodyEl));
        if (body3)
          body3->SetTextContent(instanceData);
      } else {
        // Add the new children
        nsCOMPtr<nsIDOMNode> tmp;
        nsCOMPtr<nsIDOMNodeList> childNodes;
        mElement->GetChildNodes(getter_AddRefs(childNodes));

        PRUint32 count = 0;
        if (childNodes)
          childNodes->GetLength(&count);

        for (PRUint32 i = 0; i < count; ++i) {
          nsCOMPtr<nsIDOMNode> child;
          childNodes->Item(i, getter_AddRefs(child));
          
          if (child) {
            nsCOMPtr<nsIDOMNode> clone;
            CloneNode(child, getter_AddRefs(clone));
            if (clone)
              bodyEl->AppendChild(clone, getter_AddRefs(tmp));
          }
        }
      }
      
      nsCOMPtr<nsIDOMSerializer> serializer(do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);
  
      nsAutoString docString;
      rv = serializer->SerializeToString(ddoc, docString);
      NS_ENSURE_SUCCESS(rv, rv);

      docString = NS_LITERAL_STRING("<?xml version=\"1.0\"?>") + docString;
      char* b64 = PL_Base64Encode(NS_ConvertUTF16toUTF8(docString).get(), 0, nsnull);
      if (!b64)
        return NS_ERROR_FAILURE;
      nsCAutoString b64String;
      b64String.AppendLiteral("data:text/html;base64,");
      b64String.Append(b64);
      PR_Free(b64);
      src = NS_ConvertUTF8toUTF16(b64String);
    }
    
    nsCOMPtr<nsISupports> arg;
    if (level.EqualsLiteral("modal")) {
      options.AppendLiteral(",modal");
      // We need to have an argument to set the window modal.
      // Using nsXFormsAtoms::messageProperty so that we don't create new
      // cycles between the windows.
      arg = nsXFormsAtoms::messageProperty;
    }
    
    nsCOMPtr<nsIDOMWindow> messageWindow;
     //XXX Add support for xforms-link-error.
    internal->OpenDialog(src, level, options, arg, getter_AddRefs(messageWindow));
    nsCOMPtr<nsIDOMWindowInternal> msgWinInternal =
      do_QueryInterface(messageWindow);
    if (msgWinInternal)
          msgWinInternal->Focus();
  }
  return NS_OK;
}

void
sEphemeralCallbackShow(nsITimer *aTimer, void *aListener)
{
  nsXFormsMessageElement* self =
    NS_STATIC_CAST(nsXFormsMessageElement*, aListener);
  if (self)
    self->ShowEphemeral();
}

void
sEphemeralCallbackHide(nsITimer *aTimer, void *aListener)
{
  nsXFormsMessageElement* self =
    NS_STATIC_CAST(nsXFormsMessageElement*, aListener);
  if (self)
    self->HideEphemeral();
}

void
nsXFormsMessageElement::StartEphemeral()
{
  HideEphemeral();
  if (!mElement)
    return;
  nsCOMPtr<nsIDOMDocument> domdoc;
  mElement->GetOwnerDocument(getter_AddRefs(domdoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
  if (!doc)
    return;
  doc->SetProperty(nsXFormsAtoms::messageProperty, this);
  mEphemeralTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (mEphemeralTimer)
    mEphemeralTimer->InitWithFuncCallback(sEphemeralCallbackShow, this, 
                                          SHOW_EPHEMERAL_TIMEOUT,
                                          nsITimer::TYPE_ONE_SHOT);
}

void
nsXFormsMessageElement::ShowEphemeral()
{
  if (mEphemeralTimer) {
    mEphemeralTimer->Cancel();
    mEphemeralTimer = nsnull;
  }
  if (!mElement)
    return;

  nsAutoString style;
  style.AppendLiteral(EPHEMERAL_STYLE);
  style.AppendLiteral("left:");
  style.AppendInt(mPosX);
  style.AppendLiteral("px;top:");
  style.AppendInt(mPosY);
  style.AppendLiteral("px;");
  mVisualElement->SetAttribute(NS_LITERAL_STRING("style"), style);
  mEphemeralTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (mEphemeralTimer)
    mEphemeralTimer->InitWithFuncCallback(sEphemeralCallbackHide, this, 
                                          HIDE_EPHEMERAL_TIMEOUT,
                                          nsITimer::TYPE_ONE_SHOT);
}

void
nsXFormsMessageElement::HideEphemeral()
{
  if (mEphemeralTimer) {
    mEphemeralTimer->Cancel();
    mEphemeralTimer = nsnull;
  }
  if (!mElement)
    return;

  nsCOMPtr<nsIDOMDocument> domdoc;
  mElement->GetOwnerDocument(getter_AddRefs(domdoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
  if (!doc)
    return;
  nsXFormsMessageElement *msg =
    NS_STATIC_CAST(nsXFormsMessageElement*,
                   doc->GetProperty(nsXFormsAtoms::messageProperty));
  if (msg && msg != this) {
    msg->HideEphemeral();
    return;
  }
  doc->UnsetProperty(nsXFormsAtoms::messageProperty);

  if (mVisualElement)
    mVisualElement->SetAttribute(NS_LITERAL_STRING("style"),
                                 NS_LITERAL_STRING(EPHEMERAL_STYLE_HIDDEN));
}

NS_HIDDEN_(nsresult)
NS_NewXFormsMessageElement(nsIXTFElement **aResult)
{
  *aResult = new nsXFormsMessageElement();
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
