/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Johnny Stenback <jst@netscape.com> (original author)
 *
 */

#include "nsDOMClassInfo.h"
#include "nsCRT.h"

#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"

// JavaScript includes
#include "jsapi.h"

// General helper includes
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMEventListener.h"

// Window scriptable helper includes
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIScriptExternalNameSet.h"
#include "nsJSUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIScriptObjectOwner.h"
#include "nsIJSNativeInitializer.h"

// DOM base includes
#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeTypeArray.h"
#include "nsIDOMMimeType.h"
#include "nsIFormControl.h"

// DOM core includes
#include "nsDOMError.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMLocation.h"

// HTMLFormElement helper includes
#include "nsIForm.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormControlList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIHTMLDocument.h"

// HTMLOptionCollection includes
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNSHTMLOptionCollection.h"

// Event related includes
#include "nsIEventListenerManager.h"
#include "nsIDOMEventReceiver.h"

// XBL related includes.
#include "nsIXBLService.h"
#include "nsIXBLBinding.h"
#include "nsIBindingManager.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIScriptGlobalObject.h"

// ClassInfo data helper macros
#define NS_DEFINE_CLASSINFO_DATA_HEAD                                         \
  {                                                                           \
    if (sClassInfoData) {                                                     \
      return NS_OK;                                                           \
    }                                                                         \
                                                                              \
    PRUint32 i = eDOMClassInfoIDCount;                                        \
                                                                              \
    sClassInfoData =                                                          \
      (nsDOMClassInfoData *)nsMemory::Alloc(i * sizeof(nsDOMClassInfoData));  \
                                                                              \
    if (!sClassInfoData) {                                                    \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
                                                                              \
    nsCRT::zero(sClassInfoData, i * sizeof(nsDOMClassInfoData));              \
                                                                              \
    nsDOMClassInfoData *d;

#define NS_DEFINE_CLASSINFO_DATA(_class, _ctor, _flags)                       \
    d = sClassInfoData + e##_class##_id;                                      \
    d->mName = nsnull;                                                        \
    d->mGetIIDsFptr = nsnull;                                                 \
    d->mConstructorFptr = _ctor;                                              \
    d->mClassInfoFlags = nsIClassInfo::MAIN_THREAD_ONLY;                      \
    d->mScriptableFlags = _flags

#define NS_DEFINE_CLASSINFO_DATA_TAIL                                         \
  }

#define DEFAULT_SCRIPTABLE_FLAGS                                              \
  USE_JSSTUB_FOR_ADDPROPERTY |                                                \
  USE_JSSTUB_FOR_DELPROPERTY |                                                \
  USE_JSSTUB_FOR_SETPROPERTY |                                                \
  ALLOW_PROP_MODS_DURING_RESOLVE |                                            \
  DONT_ASK_INSTANCE_FOR_SCRIPTABLE

#define ELEMENT_SCRIPTABLE_FLAGS                                              \
  DEFAULT_SCRIPTABLE_FLAGS |                                                  \
  WANT_PRECREATE |                                                            \
  WANT_CREATE |                                                               \
  WANT_GETPROPERTY |                                                          \
  WANT_SETPROPERTY

#define DOCUMENT_SCRIPTABLE_FLAGS                                             \
  DEFAULT_SCRIPTABLE_FLAGS |                                                  \
  WANT_PRECREATE |                                                            \
  WANT_NEWRESOLVE |                                                           \
  WANT_GETPROPERTY |                                                          \
  WANT_SETPROPERTY

#define ARRAY_SCRIPTABLE_FLAGS                                                \
  DEFAULT_SCRIPTABLE_FLAGS |                                                  \
  WANT_GETPROPERTY


typedef nsIClassInfo* (*nsDOMClassInfoConstructorFnc)
  (nsDOMClassInfo::nsDOMClassInfoID aID);

struct nsDOMClassInfoData
{
  const char *mName;
  GetDOMClassIIDsFnc mGetIIDsFptr;
  nsDOMClassInfoConstructorFnc mConstructorFptr;
  nsIClassInfo *mCachedClassInfo;
  PRUint32 mClassInfoFlags;
  PRUint32 mScriptableFlags; // Do we need this here?
};


nsDOMClassInfoData* nsDOMClassInfo::sClassInfoData = nsnull;
nsIXPConnect *nsDOMClassInfo::sXPConnect = nsnull;
PRUint32 nsDOMClassInfo::sInstanceCount = 0;



JSString *nsDOMClassInfo::sTop_id             = nsnull;
JSString *nsDOMClassInfo::sScrollbars_id      = nsnull;
JSString *nsDOMClassInfo::sLocation_id        = nsnull;
JSString *nsDOMClassInfo::s_content_id        = nsnull;
JSString *nsDOMClassInfo::sContent_id         = nsnull;
JSString *nsDOMClassInfo::sSidebar_id         = nsnull;
JSString *nsDOMClassInfo::sPrompter_id        = nsnull;
JSString *nsDOMClassInfo::sMenubar_id         = nsnull;
JSString *nsDOMClassInfo::sToolbar_id         = nsnull;
JSString *nsDOMClassInfo::sLocationbar_id     = nsnull;
JSString *nsDOMClassInfo::sPersonalbar_id     = nsnull;
JSString *nsDOMClassInfo::sStatusbar_id       = nsnull;
JSString *nsDOMClassInfo::sDirectories_id     = nsnull;
JSString *nsDOMClassInfo::sControllers_id     = nsnull;
JSString *nsDOMClassInfo::sLength_id          = nsnull;
JSString *nsDOMClassInfo::sOnmousedown_id     = nsnull;
JSString *nsDOMClassInfo::sOnmouseup_id       = nsnull;
JSString *nsDOMClassInfo::sOnclick_id         = nsnull;
JSString *nsDOMClassInfo::sOnmouseover_id     = nsnull;
JSString *nsDOMClassInfo::sOnmouseout_id      = nsnull;
JSString *nsDOMClassInfo::sOnkeydown_id       = nsnull;
JSString *nsDOMClassInfo::sOnkeyup_id         = nsnull;
JSString *nsDOMClassInfo::sOnkeypress_id      = nsnull;
JSString *nsDOMClassInfo::sOnmousemove_id     = nsnull;
JSString *nsDOMClassInfo::sOnfocus_id         = nsnull;
JSString *nsDOMClassInfo::sOnblur_id          = nsnull;
JSString *nsDOMClassInfo::sOnsubmit_id        = nsnull;
JSString *nsDOMClassInfo::sOnreset_id         = nsnull;
JSString *nsDOMClassInfo::sOnchange_id        = nsnull;
JSString *nsDOMClassInfo::sOnselect_id        = nsnull;
JSString *nsDOMClassInfo::sOnload_id          = nsnull;
JSString *nsDOMClassInfo::sOnunload_id        = nsnull;
JSString *nsDOMClassInfo::sOnabort_id         = nsnull;
JSString *nsDOMClassInfo::sOnerror_id         = nsnull;
JSString *nsDOMClassInfo::sOnpaint_id         = nsnull;
JSString *nsDOMClassInfo::sOnresize_id        = nsnull;
JSString *nsDOMClassInfo::sOnscroll_id        = nsnull;

// static
nsresult
nsDOMClassInfo::DefineStaticJSStrings(JSContext *cx)
{
  sTop_id = ::JS_InternString(cx, "top");

  sScrollbars_id     = ::JS_InternString(cx, "scrollbars");
  sLocation_id       = ::JS_InternString(cx, "location");
  s_content_id       = ::JS_InternString(cx, "_content");
  sContent_id        = ::JS_InternString(cx, "content");
  sSidebar_id        = ::JS_InternString(cx, "sidebar");
  sPrompter_id       = ::JS_InternString(cx, "prompter");
  sMenubar_id        = ::JS_InternString(cx, "menubar");
  sToolbar_id        = ::JS_InternString(cx, "toolbar");
  sLocationbar_id    = ::JS_InternString(cx, "locationbar");
  sPersonalbar_id    = ::JS_InternString(cx, "personalbar");
  sStatusbar_id      = ::JS_InternString(cx, "statusbar");
  sDirectories_id    = ::JS_InternString(cx, "directories");
  sControllers_id    = ::JS_InternString(cx, "controllers");
  sLength_id         = ::JS_InternString(cx, "length");
  sOnmousedown_id    = ::JS_InternString(cx, "onmousedown");
  sOnmouseup_id      = ::JS_InternString(cx, "onmouseup");
  sOnclick_id        = ::JS_InternString(cx, "onclick");
  sOnmouseover_id    = ::JS_InternString(cx, "onmouseover");
  sOnmouseout_id     = ::JS_InternString(cx, "onmouseout");
  sOnkeydown_id      = ::JS_InternString(cx, "onkeydown");
  sOnkeyup_id        = ::JS_InternString(cx, "onkeyup");
  sOnkeypress_id     = ::JS_InternString(cx, "onkeypress");
  sOnmousemove_id    = ::JS_InternString(cx, "onmousemove");
  sOnfocus_id        = ::JS_InternString(cx, "onfocus");
  sOnblur_id         = ::JS_InternString(cx, "onblur");
  sOnsubmit_id       = ::JS_InternString(cx, "onsubmit");
  sOnreset_id        = ::JS_InternString(cx, "onreset");
  sOnchange_id       = ::JS_InternString(cx, "onchange");
  sOnselect_id       = ::JS_InternString(cx, "onselect");
  sOnload_id         = ::JS_InternString(cx, "onload");
  sOnunload_id       = ::JS_InternString(cx, "onunload");
  sOnabort_id        = ::JS_InternString(cx, "onabort");
  sOnerror_id        = ::JS_InternString(cx, "onerror");
  sOnpaint_id        = ::JS_InternString(cx, "onpaint");
  sOnresize_id       = ::JS_InternString(cx, "onresize");
  sOnscroll_id       = ::JS_InternString(cx, "onscroll");

  return NS_OK;
}

// static
nsresult
nsDOMClassInfo::WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
                           const nsIID& aIID, jsval *vp)
{
  if (!native) {
    *vp = JSVAL_NULL;

    return NS_OK;
  }

  NS_ENSURE_TRUE(sXPConnect, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  nsresult rv = sXPConnect->WrapNative(cx, scope, native, aIID,
                                       getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* obj = nsnull;
  rv = holder->GetJSObject(&obj);
  NS_ENSURE_SUCCESS(rv, rv);

  *vp = OBJECT_TO_JSVAL(obj);

  return rv;
}

nsDOMClassInfo::nsDOMClassInfo(nsDOMClassInfoID aID) : mID(aID)
{
  NS_INIT_REFCNT();

  sInstanceCount++;

  if (!sXPConnect) {
    // Hmm, all this should be moved into an Init() method...

    nsServiceManager::GetService(nsIXPConnect::GetCID(),
                                 nsIXPConnect::GetIID(),
                                 (nsISupports **)&sXPConnect);

    if (sXPConnect) {
      nsCOMPtr<nsIXPCFunctionThisTranslator> old;

      nsEventListenerThisTranslator *elt = new nsEventListenerThisTranslator();

      sXPConnect->SetFunctionThisTranslator(NS_GET_IID(nsIDOMEventListener),
                                            elt, getter_AddRefs(old));
    }
  }
}

nsDOMClassInfo::~nsDOMClassInfo()
{
  sInstanceCount--;

  if (!sInstanceCount) {
    NS_IF_RELEASE(sXPConnect);
  }
}

NS_IMPL_ADDREF(nsDOMClassInfo);
NS_IMPL_RELEASE(nsDOMClassInfo);

NS_INTERFACE_MAP_BEGIN(nsDOMClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCScriptable)
NS_INTERFACE_MAP_END

nsresult
nsDOMClassInfo::Init()
{
  if (!sLocation_id) {
    // This method better be called from JS through XPConnect, if not
    // we're out of luck!
    nsresult rv;
    nsCOMPtr<nsIJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    JSContext *cx = nsnull;

    rv = stack->Peek(&cx);
    NS_ENSURE_SUCCESS(rv, rv);

    // Initialize static JSString's
    DefineStaticJSStrings(cx);
  }


  NS_DEFINE_CLASSINFO_DATA_HEAD;

  // This list of NS_DEFINE_CLASSINFO_DATA macros is what gives the
  // DOM classes their correct behavior when used through
  // XPConnect. The arguments that are passed to
  // NS_DEFINE_CLASSINFO_DATA are
  //
  // 1. Class name as it should appear in JavaScript, this name is
  //    also used to find the id of the class in nsDOMClassInfo
  //    (i.e. e<classname>_id)
  // 2. Scriptable helper constructor function
  // 3. nsIClassInfo/nsIXPCScriptable flags (i.e. for GetFlags)

  // Base classes
  NS_DEFINE_CLASSINFO_DATA(Window, nsWindowSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WANT_GETPROPERTY |
                           WANT_SETPROPERTY |
                           WANT_NEWRESOLVE |
                           WANT_PRECREATE);
  NS_DEFINE_CLASSINFO_DATA(Location, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Plugin, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(PluginArray, nsPluginArraySH::Create,
                           ARRAY_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(MimeType, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(MimeTypeArray, nsMimeTypeArraySH::Create,
                           ARRAY_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(BarProp, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(History, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Screen, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Core classes
  NS_DEFINE_CLASSINFO_DATA(Document, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(DocumentType, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(DOMImplementation, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(DocumentFragment, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Element, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Attr, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Text, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Comment, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(CDATASection, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(ProcessingInstruction, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Entity, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(EntityReference, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Notation, nsNodeSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(NodeList, nsArraySH::Create,
                           ARRAY_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(NamedNodeMap, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Misc Core related classes


  // StyleSheet classes
  NS_DEFINE_CLASSINFO_DATA(DocumentStyleSheetList, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Event
  NS_DEFINE_CLASSINFO_DATA(Event, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Misc HTML classes
  NS_DEFINE_CLASSINFO_DATA(HTMLDocument, nsHTMLDocumentSH::Create,
                           DOCUMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLCollection,
                           nsArraySH::Create, // XXX????
                           ARRAY_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionCollection,
                           nsHTMLOptionCollectionSH::Create,
                           ARRAY_SCRIPTABLE_FLAGS | WANT_SETPROPERTY);
  NS_DEFINE_CLASSINFO_DATA(HTMLFormControlCollection,
                           nsFormControlListSH::Create,
                           ARRAY_SCRIPTABLE_FLAGS);

  // HTML element classes
  NS_DEFINE_CLASSINFO_DATA(HTMLAnchorElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLAppletElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLAreaElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBRElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseFontElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBodyElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLButtonElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDListElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDelElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDirectoryElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDivElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLEmbedElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFieldSetElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFontElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFormElement, nsHTMLFormElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameSetElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHRElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadingElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHtmlElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLIFrameElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLImageElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLInputElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLInsElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLIsIndexElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLIElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLabelElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLegendElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLinkElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMapElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMenuElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMetaElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLModElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOListElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLObjectElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptGroupElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLParagraphElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLParamElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLPreElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLQuoteElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLScriptElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSelectElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSpacerElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSpanElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLStyleElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCaptionElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCellElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableColElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableColGroupElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableRowElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableSectionElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTextAreaElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTitleElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLUListElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLUnknownElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLWBRElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);

  // CSS classes
  NS_DEFINE_CLASSINFO_DATA(CSSStyleRule, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(CSSRuleList, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(MediaList, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(StyleSheetList, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(CSSStyleSheet, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(CSSStyleDeclaration, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(ComputedCSSStyleDeclaration, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(ROCSSPrimitiveValue, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Range classes
  NS_DEFINE_CLASSINFO_DATA(Range, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Selection, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // XUL classes
  NS_DEFINE_CLASSINFO_DATA(XULDocument, nsDocumentSH::Create,
                           DOCUMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULTreeElement, nsElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULCommandDispatcher, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULNodeList, nsArraySH::Create,
                           ARRAY_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULNamedNodeMap, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULAttr, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(XULPDGlobalObject, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  NS_DEFINE_CLASSINFO_DATA_TAIL

#ifdef NS_DEBUG
  PRUint32 i;

  for (i = 0; i < eDOMClassInfoIDCount; i++) {
    nsDOMClassInfoData *d = sClassInfoData + i;

    if (!d->mConstructorFptr) {
      NS_ERROR("Class info data out of sync, you forgot to update "
               "nsDOMClassInfo.h and nsDOMClassInfo.cpp! Fix this, "
               "mozilla will not work without this fixed!");

      PRInt32 *foo = nsnull;
      *foo = 0;
    }
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  nsAutoVoidArray void_array;

  sClassInfoData[mID].mGetIIDsFptr(void_array);

  *aCount = void_array.Count();

  if (!*aCount) {
    *aArray = nsnull;

    return NS_OK;
  }

  *aArray =
    NS_STATIC_CAST(nsIID **, nsMemory::Alloc(*aCount * sizeof(nsIID *)));
  NS_ENSURE_TRUE(*aArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 i;
  for (i = 0; i < *aCount; i++) {
    nsIID *iid = NS_STATIC_CAST(nsIID *, nsMemory::Alloc(sizeof(nsIID)));

    if (!iid) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, *aArray);

      return NS_ERROR_OUT_OF_MEMORY;
    }

    *iid = *NS_STATIC_CAST(nsIID *, void_array.ElementAt(i));

    *((*aArray) + i) = iid;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  if (language == LANGUAGE_JAVASCRIPT) {
    *_retval = NS_STATIC_CAST(nsIXPCScriptable *, this);

    NS_ADDREF(*_retval);
  } else {
    *_retval = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetContractID(char **aContractID)
{
  *aContractID = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassID(nsCID **aClassID)
{
  *aClassID = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetImplementationLanguage(PRUint32 *aImplLanguage)
{
  *aImplLanguage = LANGUAGE_CPP;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetFlags(PRUint32 *aFlags)
{
  *aFlags = sClassInfoData[mID].mClassInfoFlags;

  return NS_OK;
}

// nsIXPCScriptable

NS_IMETHODIMP
nsDOMClassInfo::GetClassName(char **aClassName)
{
  *aClassName = nsCRT::strdup(sClassInfoData[mID].mName);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetScriptableFlags(PRUint32 *aFlags)
{
  *aFlags = sClassInfoData[mID].mScriptableFlags;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::PreCreate(nsISupports *nativeObj, JSContext *cx,
                          JSObject *globalObj, JSObject **parentObj)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Create(nsIXPConnectWrappedNative *wrapper,
                       JSContext *cx, JSObject *obj)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                             JSContext *cx, JSObject *obj, PRUint32 enum_op,
                             jsval *statep, jsid *idp, PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsval id, PRUint32 flags,
                           JSObject **objp, PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Convert(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, PRUint32 type, jsval *vp,
                        PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, PRUint32 mode,
                            jsval *vp, PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, PRUint32 argc, jsval *argv, jsval *vp,
                     PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRUint32 argc, jsval *argv,
                          jsval *vp, PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval val, PRBool *bp,
                            PRBool *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Mark(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, void *arg, PRUint32 *_retval)
{
  NS_ERROR("Don't call me!");

  return NS_ERROR_UNEXPECTED;
}


// static
nsISupports *
nsDOMClassInfo::GetClassInfoInstance(nsDOMClassInfoID aID,
                                     GetDOMClassIIDsFnc aGetIIDsFptr,
                                     const char *aName)
{
  if (!sClassInfoData) {
    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  if (!sClassInfoData[aID].mCachedClassInfo) {
    nsDOMClassInfoData& data = sClassInfoData[aID];

    data.mCachedClassInfo = data.mConstructorFptr(aID);
    NS_ENSURE_TRUE(data.mCachedClassInfo, nsnull);

    NS_ADDREF(data.mCachedClassInfo);

    data.mGetIIDsFptr = aGetIIDsFptr;
    data.mName = aName;
  }

  NS_ABORT_IF_FALSE(sClassInfoData[aID].mGetIIDsFptr == aGetIIDsFptr,
                    "Multiple GetIIDs function for the same nsDOMClassInfoID");

  nsISupports *classinfo = sClassInfoData[aID].mCachedClassInfo;

  return classinfo;
}

// static
void
nsDOMClassInfo::ShutDown()
{
  NS_IF_RELEASE(sXPConnect);

  if (sClassInfoData) {
    PRUint32 i;

    for (i = 0; i < eDOMClassInfoIDCount; i++) {
      nsDOMClassInfoData *d = sClassInfoData + i;

      NS_IF_RELEASE(d->mCachedClassInfo);
    }

    nsMemory::Free(sClassInfoData);

    sClassInfoData = nsnull;

#ifdef NS_DEBUG_jst
    sClassInfoData = (nsDOMClassInfoData *)1;
#endif
  }

  JSString *jsnullstring = nsnull;

#ifdef NS_DEBUG_jst
  jsnullstring = (JSString *)1;
#endif

  sTop_id             = jsnullstring;
  sScrollbars_id      = jsnullstring;
  sLocation_id        = jsnullstring;
  s_content_id        = jsnullstring;
  sContent_id         = jsnullstring;
  sSidebar_id         = jsnullstring;
  sPrompter_id        = jsnullstring;
  sMenubar_id         = jsnullstring;
  sToolbar_id         = jsnullstring;
  sLocationbar_id     = jsnullstring;
  sPersonalbar_id     = jsnullstring;
  sStatusbar_id       = jsnullstring;
  sDirectories_id     = jsnullstring;
  sControllers_id     = jsnullstring;
  sLength_id          = jsnullstring;
  sOnmousedown_id     = jsnullstring;
  sOnmouseup_id       = jsnullstring;
  sOnclick_id         = jsnullstring;
  sOnmouseover_id     = jsnullstring;
  sOnmouseout_id      = jsnullstring;
  sOnkeydown_id       = jsnullstring;
  sOnkeyup_id         = jsnullstring;
  sOnkeypress_id      = jsnullstring;
  sOnmousemove_id     = jsnullstring;
  sOnfocus_id         = jsnullstring;
  sOnblur_id          = jsnullstring;
  sOnsubmit_id        = jsnullstring;
  sOnreset_id         = jsnullstring;
  sOnchange_id        = jsnullstring;
  sOnselect_id        = jsnullstring;
  sOnload_id          = jsnullstring;
  sOnunload_id        = jsnullstring;
  sOnabort_id         = jsnullstring;
  sOnerror_id         = jsnullstring;
  sOnpaint_id         = jsnullstring;
  sOnresize_id        = jsnullstring;
  sOnscroll_id        = jsnullstring;
}


// Window helper

NS_IMETHODIMP
nsWindowSH::PreCreate(nsISupports *nativeObj, JSContext * cx,
                      JSObject * globalObj, JSObject * *parentObj)
{
  // Normally ::PreCreate() is used to give XPConnect the parent
  // object for the object that's being wrapped, this parent object is
  // set as the parent of the wrapper and it's also used to find the
  // right scope for the object being wrapped. Now, in the case of the
  // global object the wrapper shouldn't have a parent but we supply
  // one here anyway (the global object itself) and this will be used
  // by XPConnect only to find the right scope, once the scope is
  // found XPConnect will find the existing wrapper (which always
  // exists since it's created on window construction), since an
  // existing wrapper is found the parent we supply here is ignored
  // after the wrapper is found.

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeObj));
  NS_WARN_IF_FALSE(sgo, "nativeObj not a node!");

  nsCOMPtr<nsIScriptContext> sctx;

  sgo->GetContext(getter_AddRefs(sctx));

  if (sctx) {
    // Use the context from nativeObj to find the global JSObject on
    // that context.

    cx = (JSContext *)sctx->GetNativeContext();

    *parentObj = ::JS_GetGlobalObject(cx);
  } else {
    // We're most likely being called when the global object is
    // created, at that point we won't get a nsIScriptContext but we
    // know we're called on the correct context so we return globalObj

    *parentObj = globalObj;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    if (JSVAL_TO_STRING(id) == s_content_id) {
      // Map window._content to window.content for backwards
      // compatibility, this should spit out an message on the JS
      // console.

      nsCOMPtr<nsISupports> native;
      wrapper->GetNative(getter_AddRefs(native));

      nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(native));
      NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsIDOMWindow> content;
      nsresult rv = window->GetContent(getter_AddRefs(content));
      NS_ENSURE_SUCCESS(rv, rv);

      return WrapNative(cx, obj, content, NS_GET_IID(nsISupports), vp);
    }
  }

  return nsEventRecieverSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsWindowSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    if (str == sLocation_id) {
      JSString *val = JS_ValueToString(cx, *vp);
      NS_ENSURE_TRUE(val, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsISupports> native;
      wrapper->GetNative(getter_AddRefs(native));

      nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(native));
      NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsIDOMLocation> location;
      nsresult rv = window->GetLocation(getter_AddRefs(location));
      NS_ENSURE_SUCCESS(rv, rv);

      nsLiteralString href(NS_REINTERPRET_CAST(PRUnichar *,
                                               ::JS_GetStringChars(val)),
                           ::JS_GetStringLength(val));

      return location->SetHref(href);
    }

    if (str == sTop_id          ||
        str == sScrollbars_id   ||
        str == s_content_id     ||
        str == sContent_id      ||
        str == sSidebar_id      ||
        str == sPrompter_id     ||
        str == sMenubar_id      ||
        str == sToolbar_id      ||
        str == sLocationbar_id  ||
        str == sPersonalbar_id  ||
        str == sStatusbar_id    ||
        str == sDirectories_id  ||
        str == sControllers_id  ||
        str == sLength_id) {
      // A "replaceable" property is being set, define the property on
      // obj.

      *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                       ::JS_GetStringLength(str),
                                       *vp, nsnull, nsnull,
                                       JSPROP_ENUMERATE);

      return *_retval ? NS_OK : NS_ERROR_FAILURE;
    }
  }

  return nsEventRecieverSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}

// static
JSBool PR_CALLBACK
nsWindowSH::StubConstructor(JSContext *cx, JSObject *obj, uintN argc,
                            jsval *argv, jsval *rval)
{
  JSFunction *fun = ::JS_ValueToFunction(cx, argv[-2]);
  if (!fun)
    return JS_FALSE;

  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  const char *name = ::JS_GetFunctionName(fun);
 
  nsAutoString nameStr;
  nameStr.AssignWithConversion(name);

  const nsGlobalNameStruct *name_struct = NS_OK;

  nsresult rv = gNameSpaceManager->LookupName(nameStr, &name_struct);

  if (NS_FAILED(rv) || !name_struct ||
      name_struct->mType != nsGlobalNameStruct::eTypeConstructor) {
    return JS_FALSE;
  }

  nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
  NS_ENSURE_SUCCESS(rv, JS_FALSE);

  nsCOMPtr<nsIJSNativeInitializer> initializer(do_QueryInterface(native));
  if (initializer) {
    rv = initializer->Initialize(cx, obj, argc, argv);
    if (NS_FAILED(rv)) {
      return JS_FALSE;
    }
  }

  nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(native));
  if (owner) {
    nsCOMPtr<nsIScriptContext> context;

    nsJSUtils::GetStaticScriptContext(cx, obj, getter_AddRefs(context));

    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

    JSObject* new_obj;
    rv = owner->GetScriptObject(context, (void**)&new_obj);

    if (NS_SUCCEEDED(rv)) {
      *rval = OBJECT_TO_JSVAL(new_obj);
    }

    return rv;
  }

  return WrapNative(cx, ::JS_GetGlobalObject(cx), native,
                    NS_GET_IID(nsISupports), rval);
}

nsresult
nsWindowSH::GlobalResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, JSString *str, PRUint32 flags,
                          JSObject **objp, PRBool *_retval)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;

  nsCOMPtr<nsIScriptContext> my_context;
  nsJSUtils::GetStaticScriptContext(cx, obj, getter_AddRefs(my_context));

  if (!my_context || NS_FAILED(my_context->IsContextInitialized())) {
    // The context is not yet initialized so there's nothing we can do
    // here yet.

    return NS_OK;
  }

  nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar*,
                                           ::JS_GetStringChars(str)),
                       ::JS_GetStringLength(str));

  const nsGlobalNameStruct *name_struct = nsnull;

  rv = gNameSpaceManager->LookupName(name, &name_struct);

  if (!name_struct) {
    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeConstructor) {
    // If there was a JS_DefineUCFunction() I could use it here, but
    // no big deal...
    JSFunction *f = ::JS_DefineFunction(cx, obj, JS_GetStringBytes(str),
                                        StubConstructor, 0, JSPROP_READONLY);

    *_retval = !!f;

    *objp = obj;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeProperty) {
    nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval prop_val; // XPConnect-wrapped property value.

    nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(native));
    if (owner) {
      nsCOMPtr<nsIScriptContext> context;
      nsJSUtils::GetStaticScriptContext(cx, obj, getter_AddRefs(context));
      NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

      JSObject *prop_obj = nsnull;

      rv = owner->GetScriptObject(context, (void**)&prop_obj);
      NS_ENSURE_TRUE(prop_obj, NS_ERROR_UNEXPECTED);

      prop_val = OBJECT_TO_JSVAL(prop_obj);
    } else {
      rv = WrapNative(cx, ::JS_GetGlobalObject(cx), native,
                      NS_GET_IID(nsISupports), &prop_val);
    }

    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                     ::JS_GetStringLength(str),
                                     prop_val, nsnull, nsnull,
                                     JSPROP_ENUMERATE | JSPROP_READONLY);
    *objp = obj;

    return *_retval ? NS_OK : NS_ERROR_FAILURE;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeDynamicNameSet) {
    nsCOMPtr<nsIScriptExternalNameSet> nameset =
      do_CreateInstance(name_struct->mCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> native;

    wrapper->GetNative(getter_AddRefs(native));
    NS_ABORT_IF_FALSE(native, "No native!");

    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(native));
    NS_ENSURE_TRUE(sgo, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIScriptContext> context;

    sgo->GetContext(getter_AddRefs(context));
    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

    rv = nameset->InitializeClasses(context);

    *objp = obj;
  }

  return rv;
}

NS_IMETHODIMP
nsWindowSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval id, PRUint32 flags,
                       JSObject **objp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    if (str == sLocation_id) {
      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), JSVAL_VOID, nsnull,
                                 nsnull, 0)) {
        return NS_ERROR_FAILURE;
      }

      *objp = obj;

      return NS_OK;
    }

    const JSObject *o = *objp;

    nsresult rv = GlobalResolve(wrapper, cx, obj, str, flags, objp, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    if (o != *objp) {
      // GlobalResolve() resolved something, we're done here then.

      return NS_OK;
    }

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    NS_ENSURE_TRUE(native, NS_ERROR_UNEXPECTED);

    // Hmm, we do an aweful lot of QI's here, maybe we should add a
    // method on an interface that would let us just call into the
    // window code directly...

    nsCOMPtr<nsIScriptGlobalObject> global(do_QueryInterface(native));
    NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDocShell> docShell;

    global->GetDocShell(getter_AddRefs(docShell));

    nsCOMPtr<nsIDocShellTreeNode> dsn(do_QueryInterface(docShell));
    NS_ENSURE_TRUE(dsn, NS_ERROR_UNEXPECTED);

    PRInt32 count = 0;

    dsn->GetChildCount(&count);

    if (count > 0) {
      nsCOMPtr<nsIDocShellTreeItem> child;

      const jschar *chars = ::JS_GetStringChars(str);

      dsn->FindChildWithName(NS_REINTERPRET_CAST(const PRUnichar*, chars),
                             PR_FALSE, PR_FALSE, nsnull,
                             getter_AddRefs(child));

      if (child) {
        // We found a subframe of the right name, define the property
        // on the wrapper so that ::NewResolve() doesn't get called
        // for again for this property name.

        jsval v;

        rv = WrapNative(cx, ::JS_GetGlobalObject(cx), child,
                        NS_GET_IID(nsISupports), &v);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!::JS_DefineUCProperty(cx, obj, chars, ::JS_GetStringLength(str),
                                   v, nsnull, nsnull, 0)) {
          return NS_ERROR_FAILURE;
        }

        *objp = obj;

        return NS_OK;
      }
    }

    if (o == *objp && JSVAL_TO_STRING(id) == s_content_id) {
      // Map window._content to window.content for backwards
      // compatibility, this should spit out an message on the JS
      // console.

      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(s_content_id),
                                 ::JS_GetStringLength(s_content_id),
                                 JSVAL_VOID, nsnull, nsnull, 0)) {
        return NS_ERROR_FAILURE;
      }

      *objp = obj;
    }
  }

  return NS_OK;
}


// DOM Node helper

NS_IMETHODIMP
nsNodeSH::PreCreate(nsISupports *nativeObj, JSContext *cx, JSObject *globalObj,
                    JSObject **parentObj)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(nativeObj));
  nsCOMPtr<nsIDocument> doc;

  if (content) {
    content->GetDocument(*getter_AddRefs(doc));
  }

  if (!doc) {
#if 0
    // Once nsIDOMNode::GetOwnerDocument() does the right thing we
    // could call it here to get to the document even if the node is
    // not in a document.

    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(nativeObj));
    NS_WARN_IF_FALSE(node, "nativeObj not a node!");

    nsCOMPtr<nsIDOMDocument> owner_doc;

    node->GetOwnerDocument(getter_AddRefs(owner_doc));

    doc = do_QueryInterface(owner_doc);

    if (!doc) {
      ...
    }
#endif

    doc = do_QueryInterface(nativeObj);

    if (!doc) {
      // No document reachable from nativeObj, use the global object
      // that was passed to this method.

      *parentObj = globalObj;

      return NS_OK;
    }
  }

  nsISupports *native_parent = nsnull;

  if (content) {
    if (content->IsContentOfType(nsIContent::eELEMENT |
                                 nsIContent::eHTML |
                                 nsIContent::eHTML_FORM_CONTROL)) {
      nsCOMPtr<nsIFormControl> form_control(do_QueryInterface(content));

      if (form_control) {
        nsCOMPtr<nsIDOMHTMLFormElement> form;

        form_control->GetForm(getter_AddRefs(form));

        native_parent = form;
      }
    }

    if (!native_parent) {
      nsCOMPtr<nsIContent> parentContent;

      content->GetParent(*getter_AddRefs(parentContent));
      native_parent = parentContent;

      if (!native_parent) {
        native_parent = doc;
      }
    }
  }

  if (!native_parent) {
    // We're called for a document object (since content is null),
    // set the parent to be the document's global object, if there
    // is one

    // Get the script global object from the document.

    nsCOMPtr<nsIScriptGlobalObject> sgo;
    doc->GetScriptGlobalObject(getter_AddRefs(sgo));

    if (!sgo) {
      // No global object reachable from this document, use the
      // global object that was passed to this method.

      *parentObj = globalObj;

      return NS_OK;
    }

    native_parent = sgo;
  }

  jsval v;

  nsresult rv = WrapNative(cx, ::JS_GetGlobalObject(cx), native_parent,
                           NS_GET_IID(nsISupports), &v);

  *parentObj = JSVAL_TO_OBJECT(v);

  return rv;
}

// EventProp helper

// static
PRBool
nsEventRecieverSH::ReallyIsEventName(JSString *jsstr, jschar aFirstChar)
{
  switch (aFirstChar) {
  case 'a' :
  case 'b' :
  case 'e' :
  case 'f' :
    if (jsstr == sOnabort_id     ||
        jsstr == sOnblur_id      ||
        jsstr == sOnerror_id     ||
        jsstr == sOnfocus_id)
      return PR_TRUE;

    break;
  case 'c' :
  case 'l' :
  case 'p' :
    if (jsstr == sOnchange_id    ||
        jsstr == sOnclick_id     ||
        jsstr == sOnload_id      ||
        jsstr == sOnpaint_id)
      return PR_TRUE;

    break;
  case 'k' :
  case 'u' :
    if (jsstr == sOnkeydown_id   ||
        jsstr == sOnkeypress_id  ||
        jsstr == sOnkeyup_id     ||
        jsstr == sOnunload_id)
      return PR_TRUE;

    break;
  case 'm' :
    if (jsstr == sOnmousemove_id ||
        jsstr == sOnmouseout_id  ||
        jsstr == sOnmouseover_id ||
        jsstr == sOnmouseup_id   ||
        jsstr == sOnmousedown_id)
      return PR_TRUE;

    break;
  case 'r' :
  case 's' :
    if (jsstr == sOnreset_id     ||
        jsstr == sOnresize_id    ||
        jsstr == sOnscroll_id    ||
        jsstr == sOnselect_id    ||
        jsstr == sOnsubmit_id)
      return PR_TRUE;

    break;
  }

  return PR_FALSE;
}

nsresult
nsEventRecieverSH::RegisterCompileHandler(nsIXPConnectWrappedNative *wrapper,
                                          JSContext *cx, JSObject *obj,
                                          jsval id, jsval *vp,
                                          PRBool aCompile)
{
  JSString *str = JSVAL_TO_STRING(id);

  if (IsEventName(str)) {
    nsCOMPtr<nsIScriptContext> script_cx;
    nsresult rv =
      nsJSUtils::GetStaticScriptContext(cx, obj, getter_AddRefs(script_cx));

    if (NS_FAILED(rv)) {
      return rv;
    }

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    NS_ABORT_IF_FALSE(native, "No native!");

    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(native));

    if (receiver) {
      nsCOMPtr<nsIEventListenerManager> manager;

      receiver->GetListenerManager(getter_AddRefs(manager));

      if (manager) {
        const PRUnichar *ustr =
          NS_REINTERPRET_CAST(const PRUnichar *, ::JS_GetStringChars(str));

        nsCOMPtr<nsIAtom> atom(getter_AddRefs(NS_NewAtom(ustr)));
        NS_ENSURE_TRUE(atom, NS_ERROR_OUT_OF_MEMORY);

        if (aCompile) {
          rv = manager->CompileScriptEventListener(script_cx, receiver, atom);
        } else {
          rv = manager->RegisterScriptEventListener(script_cx, receiver, atom);
        }

        if (NS_FAILED(rv)) {
          return rv;
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventRecieverSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj, jsval id,
                               jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    return RegisterCompileHandler(wrapper, cx, obj, id, vp, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventRecieverSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj, jsval id,
                               jsval *vp, PRBool *_retval)
{
  if (::JS_TypeOfValue(cx, *vp) == JSTYPE_FUNCTION || JSVAL_IS_STRING(id)) {
    return RegisterCompileHandler(wrapper, cx, obj, id, vp, PR_FALSE);
  }

  return NS_OK;
}


// Element helper

NS_IMETHODIMP
nsElementSH::Create(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                    JSObject *obj)
{
  // XXX: Root???  aContext->AddNamedReference((void *)&slots->mScriptObject, slots->mScriptObject, "nsGenericElement::mScriptObject");

  nsCOMPtr<nsISupports> native;

  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIContent> content(do_QueryInterface(native));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDocument> doc;

  content->GetDocument(*getter_AddRefs(doc));

  if (!doc) {
    // There's not baseclass that cares about this call so we just
    // return here.

    return NS_OK;
  }

  // See if we have a frame.
  nsCOMPtr<nsIPresShell> shell = getter_AddRefs(doc->GetShellAt(0));

  if (!shell) {
    return NS_OK;
  }

  nsIFrame* frame = nsnull;
  shell->GetPrimaryFrameFor(content, &frame);

  if (frame) {
    // If we have a frame the frame has already loaded the binding.

    return NS_OK;
  }

  // We must ensure that the XBL Binding is installed before we hand
  // back this object.

  nsCOMPtr<nsIBindingManager> bindingManager;
  doc->GetBindingManager(getter_AddRefs(bindingManager));
  NS_ENSURE_TRUE(bindingManager, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIXBLBinding> binding;
  bindingManager->GetBinding(content, getter_AddRefs(binding));

  if (binding) {
    // There's already a binding for this element so nothing left to
    // be done here.

    return NS_OK;
  }

  nsCOMPtr<nsIScriptGlobalObject> global;
  doc->GetScriptGlobalObject(getter_AddRefs(global));

  nsCOMPtr<nsIDOMViewCSS> viewCSS(do_QueryInterface(global));
  NS_ENSURE_TRUE(viewCSS, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(native));

  viewCSS->GetComputedStyle(elt, nsString(), getter_AddRefs(cssDecl));
  NS_ENSURE_TRUE(cssDecl, NS_ERROR_UNEXPECTED);

  nsAutoString value;
  cssDecl->GetPropertyValue(NS_LITERAL_STRING("-moz-binding"), value);

  if (value.IsEmpty()) {
    // No binding, nothing left to do here.

    return NS_OK;
  }

  // We have a binding that must be installed.
  PRBool dummy;

  nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));
  NS_ENSURE_TRUE(xblService, NS_ERROR_NOT_AVAILABLE);

  xblService->LoadBindings(content, value, PR_FALSE, getter_AddRefs(binding),
                           &dummy);

  if (binding) {
    binding->ExecuteAttachedHandler();
  }

  return NS_OK;
}


// NodeList scriptable helper

nsresult
nsArraySH::GetItemAt(nsIXPConnectWrappedNative *wrapper, PRUint32 aIndex,
                     nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMNodeList> list(do_QueryInterface(native));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  nsIDOMNode *node = nsnull; // Weak, transfer the ownership over to aResult
  nsresult rv = list->Item(aIndex, &node);

  *aResult = node;

  return rv;
}

NS_IMETHODIMP
nsArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  int32 n = -1;

  if ((JSVAL_IS_NUMBER(id) || JSVAL_IS_STRING(id)) &&
      ::JS_ValueToInt32(cx, id, &n)) {
    if (n < 0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    nsCOMPtr<nsISupports> array_item;

    nsresult rv = GetItemAt(wrapper, n, getter_AddRefs(array_item));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = WrapNative(cx, ::JS_GetGlobalObject(cx), array_item,
                    NS_GET_IID(nsIDOMNode), vp);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


// Named Array helper

NS_IMETHODIMP
nsNamedArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    nsCOMPtr<nsISupports> item;

    nsresult rv = GetNamedItem(wrapper, id, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    // Do we wanto fall through to nsArraySH::GetProperty() here if
    // item is null?

    rv = WrapNative(cx, ::JS_GetGlobalObject(cx), item,
                    NS_GET_IID(nsISupports), vp);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  return nsArraySH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}


// FomrControlList helper

nsresult
nsFormControlListSH::GetItemAt(nsIXPConnectWrappedNative *wrapper,
                               PRUint32 aIndex, nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMHTMLCollection> collection(do_QueryInterface(native));
  NS_ENSURE_TRUE(collection, NS_ERROR_UNEXPECTED);

  nsIDOMNode *node = nsnull; // Weak, transfer the ownership over to aResult
  nsresult rv = collection->Item(aIndex, &node);

  *aResult = node;

  return rv;
}

nsresult
nsFormControlListSH::GetNamedItem(nsIXPConnectWrappedNative *wrapper, jsval id,
                                  nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMNSHTMLFormControlList> list(do_QueryInterface(native));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  JSString *jsstr = JSVAL_TO_STRING(id);

  nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                           ::JS_GetStringChars(jsstr)),
                       ::JS_GetStringLength(jsstr));

  return list->NamedItem(name, aResult);
}

// Document helper for document.location and document.on*

NS_IMETHODIMP
nsDocumentSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, PRUint32 flags,
                         JSObject **objp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    if (str == sLocation_id) {
      *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                       ::JS_GetStringLength(str),
                                       JSVAL_VOID, nsnull, nsnull, 
                                       JSPROP_ENUMERATE);
      *objp = obj;

      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *jsstr = JSVAL_TO_STRING(id);

    if (jsstr == sLocation_id) {
      nsCOMPtr<nsISupports> native;

      wrapper->GetNative(getter_AddRefs(native));
      NS_ABORT_IF_FALSE(native, "No native!");

      nsCOMPtr<nsIDOMNSDocument> doc(do_QueryInterface(native));
      NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsIDOMLocation> l;

      doc->GetLocation(getter_AddRefs(l));

      return WrapNative(cx, ::JS_GetGlobalObject(cx), l,
                        NS_GET_IID(nsIDOMLocation), vp);
    }
  }

  return nsNodeSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsDocumentSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *jsstr = JSVAL_TO_STRING(id);

    if (jsstr == sLocation_id && JSVAL_IS_STRING(*vp)) {
      nsCOMPtr<nsISupports> native;

      wrapper->GetNative(getter_AddRefs(native));
      NS_ABORT_IF_FALSE(native, "No native!");

      nsCOMPtr<nsIDocument> doc(do_QueryInterface(native));
      NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsIScriptGlobalObject> sgo;

      doc->GetScriptGlobalObject(getter_AddRefs(sgo));

      nsCOMPtr<nsIDOMWindowInternal> win(do_QueryInterface(sgo));

      if (win) {
        nsCOMPtr<nsIDOMLocation> location;

        win->GetLocation(getter_AddRefs(location));

        if (location) {
          JSString *l = JSVAL_TO_STRING(*vp);

          nsLiteralString href(NS_REINTERPRET_CAST(const PRUnichar *,
                                                   ::JS_GetStringChars(l)),
                               ::JS_GetStringLength(l));

          location->SetHref(href);
        }
      }
    }
  }

  return nsNodeSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}


// HTMLDocument helper

NS_IMETHODIMP
nsHTMLDocumentSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                              JSContext *cx, JSObject *obj, jsval id,
                              jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    nsCOMPtr<nsISupports> native;

    wrapper->GetNative(getter_AddRefs(native));
    NS_ABORT_IF_FALSE(native, "No native!");

    nsCOMPtr<nsIHTMLDocument> doc(do_QueryInterface(native));
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    JSString *jsstr = JSVAL_TO_STRING(id);

    nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                             ::JS_GetStringChars(jsstr)),
                         ::JS_GetStringLength(jsstr));

    nsCOMPtr<nsISupports> result;

    doc->ResolveName(name, nsnull, getter_AddRefs(result));

    if (result) {
      return WrapNative(cx, ::JS_GetGlobalObject(cx), result,
                        NS_GET_IID(nsISupports), vp);
    }
  }

  return nsDocumentSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}


// HTMLFormElement helper

NS_IMETHODIMP
nsHTMLFormElementSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    nsCOMPtr<nsISupports> native;

    wrapper->GetNative(getter_AddRefs(native));
    NS_ABORT_IF_FALSE(native, "No native!");

    nsCOMPtr<nsIForm> form(do_QueryInterface(native));

    JSString *jsstr = JSVAL_TO_STRING(id);

    nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                             ::JS_GetStringChars(jsstr)),
                         ::JS_GetStringLength(jsstr));

    nsCOMPtr<nsISupports> result;

    form->ResolveName(name, getter_AddRefs(result));

    if (!result) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(native));
      nsCOMPtr<nsIDOMHTMLFormElement> form_element(do_QueryInterface(native));

      nsCOMPtr<nsIDocument> doc;
      content->GetDocument(*getter_AddRefs(doc));

      nsCOMPtr<nsIHTMLDocument> html_doc(do_QueryInterface(doc));

      if (html_doc && form_element) {
        html_doc->ResolveName(name, form_element, getter_AddRefs(result));
      }
    }

    if (result) {
      // Wrap result, result can be either an element or a list of
      // elements
      return WrapNative(cx, ::JS_GetGlobalObject(cx), result,
                        NS_GET_IID(nsISupports), vp);
    }
  }

  return nsElementSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

// HTMLOptionCollection helper

NS_IMETHODIMP
nsHTMLOptionCollectionSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                                      JSContext *cx, JSObject *obj, jsval id,
                                      jsval *vp, PRBool *_retval)
{
  if (!JSVAL_IS_INT(id)) {
    return NS_OK;
  }

  // vp must refer to an object
  if (!JSVAL_IS_OBJECT(*vp) && !JS_ConvertValue(cx, *vp, JSTYPE_OBJECT, vp)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIDOMHTMLOptionElement> new_option;

  if (!JSVAL_IS_NULL(*vp)) {
    JSObject* option_obj = JSVAL_TO_OBJECT(*vp); 
    JSClass* jsclass = JS_GetClass(cx, option_obj);

    if (jsclass && !((~jsclass->flags) & (JSCLASS_HAS_PRIVATE |
                                          JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
      nsISupports *s = (nsISupports *)JS_GetPrivate(cx, option_obj);

      nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(s));
      NS_ENSURE_TRUE(wrapper, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsISupports> native;
      wrapper->GetNative(getter_AddRefs(native));

      new_option = do_QueryInterface(native);

      if (!new_option) {
        // Someone is trying to set an option to a non-option object.

        return NS_ERROR_UNEXPECTED;
      }
    } else {
      return NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMNSHTMLOptionCollection> oc(do_QueryInterface(native));
  NS_ENSURE_TRUE(oc, NS_ERROR_UNEXPECTED);

  return oc->SetOption(JSVAL_TO_INT(id), new_option);
}


// PluginArray helper

nsresult
nsPluginArraySH::GetItemAt(nsIXPConnectWrappedNative *wrapper,
                           PRUint32 aIndex, nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMPluginArray> array(do_QueryInterface(native));
  NS_ENSURE_TRUE(array, NS_ERROR_UNEXPECTED);

  nsIDOMPlugin *plugin = nsnull;
  nsresult rv = array->Item(aIndex, &plugin);

  *aResult = plugin;

  return rv;
}

nsresult
nsPluginArraySH::GetNamedItem(nsIXPConnectWrappedNative *wrapper, jsval id,
                              nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMPluginArray> array(do_QueryInterface(native));
  NS_ENSURE_TRUE(array, NS_ERROR_UNEXPECTED);

  JSString *jsstr = JSVAL_TO_STRING(id);

  nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                           ::JS_GetStringChars(jsstr)),
                       ::JS_GetStringLength(jsstr));

  nsIDOMPlugin *plugin = nsnull;

  nsresult rv = array->NamedItem(name, &plugin);

  *aResult = plugin;

  return rv;
}


// PluginArray helper

nsresult
nsMimeTypeArraySH::GetItemAt(nsIXPConnectWrappedNative *wrapper,
                             PRUint32 aIndex, nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMMimeTypeArray> array(do_QueryInterface(native));
  NS_ENSURE_TRUE(array, NS_ERROR_UNEXPECTED);

  nsIDOMMimeType *mime_type = nsnull;
  nsresult rv = array->Item(aIndex, &mime_type);

  *aResult = mime_type;

  return rv;
}

nsresult
nsMimeTypeArraySH::GetNamedItem(nsIXPConnectWrappedNative *wrapper, jsval id,
                                nsISupports **aResult)
{
  nsCOMPtr<nsISupports> native;
  wrapper->GetNative(getter_AddRefs(native));

  nsCOMPtr<nsIDOMMimeTypeArray> array(do_QueryInterface(native));
  NS_ENSURE_TRUE(array, NS_ERROR_UNEXPECTED);

  JSString *jsstr = JSVAL_TO_STRING(id);

  nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                           ::JS_GetStringChars(jsstr)),
                       ::JS_GetStringLength(jsstr));

  nsIDOMMimeType *mime_type = nsnull;

  nsresult rv = array->NamedItem(name, &mime_type);

  *aResult = mime_type;

  return rv;
}


// nsIDOMEventListener::HandleEvent() 'this' converter helper

NS_INTERFACE_MAP_BEGIN(nsEventListenerThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsIXPCFunctionThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsEventListenerThisTranslator)
NS_IMPL_RELEASE(nsEventListenerThisTranslator)


NS_IMETHODIMP
nsEventListenerThisTranslator::TranslateThis(nsISupports *aInitialThis,
                                             nsIInterfaceInfo *aInterfaceInfo,
                                             PRUint16 aMethodIndex,
                                             PRBool *aHideFirstParamFromJS,
                                             nsIID * *aIIDOfResult,
                                             nsISupports **_retval)
{
  *aHideFirstParamFromJS = PR_FALSE;
  *aIIDOfResult = nsnull;

  nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(aInitialThis));
  NS_ENSURE_TRUE(event, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMEventTarget> target;

  event->GetCurrentTarget(getter_AddRefs(target));

  NS_WARN_IF_FALSE(target, "Hmm, null target, weird.");

  *_retval = target;
  NS_IF_ADDREF(*_retval);

  return NS_OK;
}


