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
#include "nsIScriptContext.h"
#include "nsContentUtils.h"

// JavaScript includes
#include "jsapi.h"

// General helper includes
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsLayoutAtoms.h"

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

// DOM includes
#include "nsDOMError.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMLocation.h"

// HTMLFormElement helper includes
#include "nsIForm.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormControlList.h"
#include "nsIHTMLDocument.h"

// HTMLOptionCollection includes
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNSHTMLOptionCollection.h"

// Event related includes
#include "nsIEventListenerManager.h"
#include "nsIDOMEventReceiver.h"


// XXX most of these can be removed once I merge with the tip.

#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMPaintListener.h"

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
  WANT_GETPROPERTY |                                                          \
  WANT_SETPROPERTY


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
nsDOMClassInfo::DoDefineStaticJSIds(JSContext *cx)
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

nsDOMClassInfo::nsDOMClassInfo(nsDOMClassInfoID aID) : mID(aID)
{
  NS_INIT_REFCNT();

  sInstanceCount++;

  if (!sXPConnect) {
    nsServiceManager::GetService(nsIXPConnect::GetCID(),
                                 nsIXPConnect::GetIID(),
                                 (nsISupports **)&sXPConnect);
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
                           USE_JSSTUB_FOR_ADDPROPERTY |
                           USE_JSSTUB_FOR_DELPROPERTY |
                           USE_JSSTUB_FOR_SETPROPERTY |
                           ALLOW_PROP_MODS_DURING_RESOLVE |
                           WANT_GETPROPERTY |
                           WANT_NEWRESOLVE |
                           WANT_PRECREATE);

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
  NS_DEFINE_CLASSINFO_DATA(NodeList, nsNodeListSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY);
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
                           nsNodeListSH::Create, // XXX????
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY |
                           WANT_SETPROPERTY);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionCollection,
                           nsHTMLOptionCollectionSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY |
                           WANT_SETPROPERTY);
  NS_DEFINE_CLASSINFO_DATA(HTMLFormControlCollection,
                           nsFormControlListSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY);

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
  NS_DEFINE_CLASSINFO_DATA(XULNodeList, nsNodeListSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY);
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
               "mozilla will now crash!");

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
nsWindowSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                        JSObject * obj, jsval id, jsval * vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    DefineStaticJSIds(cx);

    // Look for a child frame with the name of the property we're
    // getting, if we find one we'll return that child frame.

    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));

    nsCOMPtr<nsIScriptGlobalObject> global(do_QueryInterface(native));

    nsCOMPtr<nsIDocShell> docShell;

    global->GetDocShell(getter_AddRefs(docShell));

    nsCOMPtr<nsIDocShellTreeNode> dsn(do_QueryInterface(docShell));

    PRInt32 count;

    nsresult rv = dsn->GetChildCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    if (count) {
      nsCOMPtr<nsIDocShellTreeItem> child;

      JSString *str = JSVAL_TO_STRING(id);
      const jschar *chars = ::JS_GetStringChars(str);

      rv = dsn->FindChildWithName(NS_REINTERPRET_CAST(const PRUnichar*, chars),
                                  PR_FALSE, PR_FALSE, nsnull,
                                  getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);

      if (child) {
        nsCOMPtr<nsIDOMWindow> child_window(do_GetInterface(child));
        NS_ENSURE_TRUE(child_window, NS_ERROR_UNEXPECTED);

        // We found a subframe of the right name.  The rest of this code
        // is to get its script object.

        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

        rv = sXPConnect->WrapNative(cx, obj, child_window,
                                    NS_GET_IID(nsIDOMWindow),
                                    getter_AddRefs(holder));
        NS_ENSURE_SUCCESS(rv, rv);

        JSObject* child_obj = nsnull; // XPConnect-wrapped property value.

        rv = holder->GetJSObject(&child_obj);
        NS_ENSURE_SUCCESS(rv, rv);

        *vp = OBJECT_TO_JSVAL(child_obj);

        return NS_OK;
      }
    }

    if (JSVAL_TO_STRING(id) == s_content_id) {
      // Map window._content to window.content for backwards
      // compatibility, this should spit out an message on the JS
      // console.

      nsCOMPtr<nsISupports> native;
      wrapper->GetNative(getter_AddRefs(native));

      nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(native));
      nsCOMPtr<nsIDOMWindow> content;

      nsresult rv = window->GetContent(getter_AddRefs(content));
      NS_ENSURE_SUCCESS(rv, rv);

      if (content) {
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

        rv = sXPConnect->WrapNative(cx, obj, content, NS_GET_IID(nsISupports),
                                    getter_AddRefs(holder));
        NS_ENSURE_SUCCESS(rv, rv);

        JSObject* content_obj = nsnull; // XPConnect-wrapped property value.

        rv = holder->GetJSObject(&content_obj);
        NS_ENSURE_SUCCESS(rv, rv);

        *vp = OBJECT_TO_JSVAL(content_obj);
      }
    }
  }

  return NS_OK;
}

static JSBool PR_CALLBACK
StubConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  JSFunction *fun;
  
  fun = ::JS_ValueToFunction(cx, argv[-2]);
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

    nsJSUtils::nsGetStaticScriptContext(cx, obj, getter_AddRefs(context));

    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

    JSObject* new_obj;
    rv = owner->GetScriptObject(context, (void**)&new_obj);

    if (NS_SUCCEEDED(rv)) {
      *rval = OBJECT_TO_JSVAL(new_obj);
    }

    return rv;
  }
    
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, JS_FALSE);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  rv = xpc->WrapNative(cx, ::JS_GetGlobalObject(cx), native,
                       NS_GET_IID(nsISupports), getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* new_obj = nsnull; // XPConnect-wrapped property value.

  rv = holder->GetJSObject(&new_obj);
  NS_ENSURE_SUCCESS(rv, JS_FALSE);

  *rval = OBJECT_TO_JSVAL(new_obj);

  return JS_TRUE;
}

nsresult
nsWindowSH::GlobalResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, PRUint32 flags,
                          JSObject **objp, PRBool *_retval)
{
  if (!JSVAL_IS_STRING(id)) {
    return NS_OK;
  }

  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;

  JSString* jsstr = JSVAL_TO_STRING(id);
  nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar*,
                                           ::JS_GetStringChars(jsstr)),
                       ::JS_GetStringLength(jsstr));

  nsIScriptContext *script_cx = (nsIScriptContext *)::JS_GetContextPrivate(cx);




#if 0 // Do we really need to do this???



  nsCOMPtr<nsIScriptContext> my_context;
  nsJSUtils::nsGetStaticScriptContext(cx, obj,
                                      getter_AddRefs(my_context));
  NS_ENSURE_TRUE(my_context, NS_ERROR_UNEXPECTED);

  rv = my_context->IsContextInitialized();
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  const nsGlobalNameStruct *name_struct = nsnull;

  rv = gNameSpaceManager->LookupName(name, &name_struct);

  if (!name_struct) {
    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeConstructor) {
    JSFunction * f = ::JS_DefineFunction(cx, obj, JS_GetStringBytes(jsstr),
                                         StubConstructor, 0, JSPROP_READONLY);

    *_retval = !!f;

    *objp = obj;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeProperty) {
    nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject* prop = nsnull; // XPConnect-wrapped property value.

    nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(native));
    if (owner) {
      nsCOMPtr<nsIScriptContext> context;
      nsJSUtils::nsGetStaticScriptContext(cx, obj,
                                          getter_AddRefs(context));
      NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

      rv = owner->GetScriptObject(context, (void**)&prop);
    } else {
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx), native,
                                  NS_GET_IID(nsISupports),
                                  getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = holder->GetJSObject(&prop);
    }

    NS_ENSURE_SUCCESS(rv, rv);

    *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(jsstr),
                                     ::JS_GetStringLength(jsstr),
                                     OBJECT_TO_JSVAL(prop), nsnull, nsnull, 
                                     JSPROP_ENUMERATE | JSPROP_READONLY);
    *objp = obj;

    return NS_OK;
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

    return nameset->InitializeClasses(context);
  }

  return rv;
}

NS_IMETHODIMP
nsWindowSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval id, PRUint32 flags,
                       JSObject **objp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    DefineStaticJSIds(cx);

    JSString *str = JSVAL_TO_STRING(id);

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    NS_ENSURE_TRUE(native, NS_ERROR_UNEXPECTED);

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

        if (!::JS_DefineUCProperty(cx, obj, chars, ::JS_GetStringLength(str),
                                   JSVAL_VOID, nsnull, nsnull, 0)) {
          return NS_ERROR_FAILURE;
        }

        *objp = obj;

        return NS_OK;
      }
    }

    JSObject *o = *objp;

    nsresult rv = GlobalResolve(wrapper, cx, obj, id, flags, objp, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

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
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(nativeObj));
  NS_WARN_IF_FALSE(node, "nativeObj not a node!");

  nsCOMPtr<nsIDOMDocument> owner_doc;

  node->GetOwnerDocument(getter_AddRefs(owner_doc));

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(owner_doc));

  if (!doc) {
    doc = do_QueryInterface(nativeObj);

    if (!doc) {
      // No document reachable from nativeObj, use the global object
      // that was passed to this method.

      *parentObj = globalObj;

      return NS_OK;
    }
  }

  // Get the script global object from the document and get the
  // JSContext from the global object.

  nsCOMPtr<nsIScriptGlobalObject> sgo;

  doc->GetScriptGlobalObject(getter_AddRefs(sgo));

  if (sgo) {
    nsCOMPtr<nsIScriptContext> sctx;

    sgo->GetContext(getter_AddRefs(sctx));

    if (sctx) {
      // Use the context of the global object in stead of the one
      // we're called from.

      cx = (JSContext *)sctx->GetNativeContext();
    }
  }



  // XXX: Form controls, they need their form as parent!!!



  nsCOMPtr<nsIDOMNode> parentNode;

  nsresult rv = node->GetParentNode(getter_AddRefs(parentNode));

  nsISupports *p = parentNode;

  if (!parentNode) {
    if (!sgo) {
      // No global object reachable from this document, use the
      // global object that was passed to this method.

      *parentObj = globalObj;

      return NS_OK;
    }

    p = sgo;
  }

  NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx), p,
                              NS_GET_IID(nsISupports), getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  // We've wrapped the parent, return the parent wrapper as our parent
  // object.
  rv = holder->GetJSObject(parentObj);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// EventProp helper

NS_IMETHODIMP
nsEventPropSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsval id, jsval *vp,
                           PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    if (canBeEventName(str)) {
      NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

      // event code goes here...

    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventPropSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsval id, jsval *vp,
                           PRBool *_retval)
{
  if (::JS_TypeOfValue(cx, *vp) != JSTYPE_FUNCTION || !JSVAL_IS_STRING(id)) {
    return NS_OK;
  }

  JSString *str = JSVAL_TO_STRING(id);

  if (canBeEventName(str)) {
    DefineStaticJSIds(cx);

    const PRUnichar *ustr = NS_REINTERPRET_CAST(const PRUnichar *,
                                                ::JS_GetStringChars(str));
    const nsIID *iid = nsnull;

    if (str == sOnmousedown_id ||
        str == sOnmouseup_id ||
        str == sOnclick_id ||
        str == sOnmouseover_id ||
        str == sOnmouseout_id) {
      iid = &NS_GET_IID(nsIDOMMouseListener);
    } else if (str == sOnkeydown_id ||
               str == sOnkeyup_id ||
               str == sOnkeypress_id) {
      iid = &NS_GET_IID(nsIDOMKeyListener);
    } else if (str == sOnmousemove_id) {
      iid = &NS_GET_IID(nsIDOMMouseMotionListener);
    } else if (str == sOnfocus_id ||
               str == sOnblur_id) {
      iid = &NS_GET_IID(nsIDOMFocusListener);
    } else if (str == sOnsubmit_id ||
               str == sOnreset_id ||
               str == sOnchange_id ||
               str == sOnselect_id) {
      iid = &NS_GET_IID(nsIDOMFormListener);
    } else if (str == sOnload_id ||
               str == sOnunload_id ||
               str == sOnabort_id ||
               str == sOnerror_id) {
      iid = &NS_GET_IID(nsIDOMLoadListener);
    } else if (str == sOnpaint_id ||
               str == sOnresize_id ||
               str == sOnscroll_id) {
      iid = &NS_GET_IID(nsIDOMPaintListener);
    }

    if (iid) {
      nsCOMPtr<nsIScriptContext> script_cx;
      nsresult rv =
        nsJSUtils::nsGetStaticScriptContext(cx, obj,
                                            getter_AddRefs(script_cx));

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
          nsCOMPtr<nsIAtom> atom(getter_AddRefs(NS_NewAtom(ustr)));
          NS_ENSURE_TRUE(atom, NS_ERROR_OUT_OF_MEMORY);

          rv = manager->RegisterScriptEventListener(script_cx, native, atom);

          if (NS_FAILED(rv)) {
            return rv;
          }
        }
      }
    }
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

NS_IMETHODIMP
nsNodeListSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp,
                          PRBool *_retval)
{
  int32 n = -1;

  if ((JSVAL_IS_NUMBER(id) || JSVAL_IS_STRING(id)) &&
      ::JS_ValueToInt32(cx, id, &n)) {
    if (n < 0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }
      
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));

    nsCOMPtr<nsIDOMNodeList> list(do_QueryInterface(native));
    NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMNode> node;

    nsresult rv = list->Item(n, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    if (node) {
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx), node,
                                  NS_GET_IID(nsIDOMNode),
                                  getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSObject* node_obj = nsnull;

      rv = holder->GetJSObject(&node_obj);
      NS_ENSURE_SUCCESS(rv, rv);

      *vp = OBJECT_TO_JSVAL(node_obj);
    } else {
      *vp = JSVAL_NULL;
    }
  }

  return NS_OK;
}


// FomrControlList scriptable helper

NS_IMETHODIMP
nsFormControlListSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    JSString *jsstr = JSVAL_TO_STRING(id);

    nsLiteralString name(NS_REINTERPRET_CAST(const PRUnichar *,
                                             ::JS_GetStringChars(jsstr)),
                         ::JS_GetStringLength(jsstr));

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));

    nsCOMPtr<nsIDOMNSHTMLFormControlList> list(do_QueryInterface(native));
    NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsISupports> item;

    nsresult rv = list->NamedItem(name, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    if (item) {
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx), item,
                                  NS_GET_IID(nsISupports),
                                  getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSObject* item_obj = nsnull;

      rv = holder->GetJSObject(&item_obj);
      NS_ENSURE_SUCCESS(rv, rv);

      *vp = OBJECT_TO_JSVAL(item_obj);
    } else {
      *vp = JSVAL_NULL;
    }

    return NS_OK;
  }

  return nsNodeListSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}


// Document helper for document.location and document.on*

NS_IMETHODIMP
nsDocumentSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    DefineStaticJSIds(cx);

    JSString *jsstr = JSVAL_TO_STRING(id);

    if (jsstr == sLocation_id && JSVAL_IS_STRING(*vp)) {
      NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

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
          nsLiteralString href(NS_REINTERPRET_CAST(const PRUnichar *,
                                                   ::JS_GetStringChars(jsstr)),
                               ::JS_GetStringLength(jsstr));

          location->SetHref(href);
        }
      }
    }
  }

  return nsEventPropSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}


// HTMLDocument helper

NS_IMETHODIMP
nsHTMLDocumentSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                              JSContext *cx, JSObject *obj, jsval id,
                              jsval *vp, PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

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
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      nsresult rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx),
                                           result, NS_GET_IID(nsISupports),
                                           getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSObject* prop_obj = nsnull;
      rv = holder->GetJSObject(&prop_obj);
      NS_ENSURE_SUCCESS(rv, rv);

      *vp = OBJECT_TO_JSVAL(prop_obj);

      return NS_OK;
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
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

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
      nsCOMPtr<nsIDOMHTMLFormElement> form(do_QueryInterface(native));

      nsCOMPtr<nsIDocument> doc;
      content->GetDocument(*getter_AddRefs(doc));

      nsCOMPtr<nsIHTMLDocument> html_doc(do_QueryInterface(doc));

      if (html_doc && form) {
        html_doc->ResolveName(name, form, getter_AddRefs(result));
      }
    }

    if (result) {
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      // Wrap result, result can be either an element or a list of
      // elements
      nsresult rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx),
                                           result, NS_GET_IID(nsISupports),
                                           getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSObject* prop_obj = nsnull;
      holder->GetJSObject(&prop_obj);
      NS_ENSURE_SUCCESS(rv, rv);

      *vp = OBJECT_TO_JSVAL(prop_obj);
    }
  }

  return NS_OK;
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

