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
#include "nsLayoutAtoms.h"

// DOM includes
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLDocument.h"

// HTMLFormElement helper includes
#include "nsIForm.h"

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

typedef nsDOMClassInfo nsDOMGenericSH;


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

  nsISupports *classinfo = sClassInfoData[aID].mCachedClassInfo;

  return classinfo;
}

// DOM Node scriptable helper, this class deals with setting the
// parent for the wrappers

class nsNodeSH : public nsDOMGenericSH
{
protected:
  nsNodeSH(nsDOMClassInfoID aID) : nsDOMGenericSH(aID)
  {
  }

  virtual ~nsNodeSH()
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

  // Is this method needed?
#if 0
  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsNodeSH(aID);
  }
#endif
};

NS_IMETHODIMP
nsNodeSH::PreCreate(nsISupports *nativeObj, JSContext *cx, JSObject *globalObj,
                    JSObject **parentObj)
{
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(nativeObj));
  NS_WARN_IF_FALSE(node, "nativeObj not a node!");

  nsCOMPtr<nsIDOMNode> parent;

  nsresult rv = node->GetParentNode(getter_AddRefs(parent));

  if (parent) {
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

    rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx), parent,
                                NS_GET_IID(nsISupports),
                                getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = holder->GetJSObject(parentObj);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    *parentObj = globalObj;
  }

  return NS_OK;
}

// EventProp scriptable helper, this class should be the base class of
// all objects that should support things like
// obj.onclick=function{...}

class nsEventPropSH : public nsNodeSH
{
protected:
  nsEventPropSH(nsDOMClassInfoID aID) : nsNodeSH(aID)
  {
  }

  virtual ~nsEventPropSH()
  {
  }

  inline PRBool canBeEventName(JSString *jsstr)
  {
    jschar *str = ::JS_GetStringChars(jsstr);

    return str[0] == 'o' && str[1] == 'n';
  }

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsEventPropSH(aID);
  }
};

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





           *_retval = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsEventPropSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsval id, jsval *vp,
                           PRBool *_retval)
{
  if (::JS_TypeOfValue(cx, *vp) != JSTYPE_FUNCTION || !JSVAL_IS_STRING(id)) {



    *_retval = PR_TRUE;

    return NS_OK;
  }

  JSString *str = JSVAL_TO_STRING(id);

  if (canBeEventName(str)) {
    const PRUnichar *ustr = NS_REINTERPRET_CAST(const PRUnichar *,
                                                ::JS_GetStringChars(str));
    nsCOMPtr<nsIAtom> atom(getter_AddRefs(NS_NewAtom(ustr)));

    const nsIID *iid = nsnull;

    if (atom.get() == nsLayoutAtoms::onmousedown ||
        atom.get() == nsLayoutAtoms::onmouseup ||
        atom.get() == nsLayoutAtoms::onclick ||
        atom.get() == nsLayoutAtoms::onmouseover ||
        atom.get() == nsLayoutAtoms::onmouseout) {
      iid = &NS_GET_IID(nsIDOMMouseListener);
    } else if (atom.get() == nsLayoutAtoms::onkeydown ||
               atom.get() == nsLayoutAtoms::onkeyup ||
               atom.get() == nsLayoutAtoms::onkeypress) {
      iid = &NS_GET_IID(nsIDOMKeyListener);
    } else if (atom.get() == nsLayoutAtoms::onmousemove) {
      iid = &NS_GET_IID(nsIDOMMouseMotionListener);
    } else if (atom.get() == nsLayoutAtoms::onfocus ||
               atom.get() == nsLayoutAtoms::onblur) {
      iid = &NS_GET_IID(nsIDOMFocusListener);
    } else if (atom.get() == nsLayoutAtoms::onsubmit ||
               atom.get() == nsLayoutAtoms::onreset ||
               atom.get() == nsLayoutAtoms::onchange ||
               atom.get() == nsLayoutAtoms::onselect) {
      iid = &NS_GET_IID(nsIDOMFormListener);
    } else if (atom.get() == nsLayoutAtoms::onload ||
               atom.get() == nsLayoutAtoms::onunload ||
               atom.get() == nsLayoutAtoms::onabort ||
               atom.get() == nsLayoutAtoms::onerror) {
      iid = &NS_GET_IID(nsIDOMLoadListener);
    } else if (atom.get() == nsLayoutAtoms::onpaint ||
               atom.get() == nsLayoutAtoms::onresize ||
               atom.get() == nsLayoutAtoms::onscroll) {
      iid = &NS_GET_IID(nsIDOMPaintListener);
    }

    if (iid) {
      nsCOMPtr<nsIScriptContext> script_cx;
      nsresult rv =
        nsContentUtils::GetStaticScriptContext(cx, obj,
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
          rv = manager->RegisterScriptEventListener(script_cx, native, atom,
                                                    *iid);

          if (NS_FAILED(rv)) {
            *_retval = PR_FALSE;

            return rv;
          }
        }
      }
    }
  }




        *_retval = PR_TRUE;
  return NS_OK;
}

// NodeList scriptable helper

class nsNodeListSH : public nsDOMClassInfo
{
private:
  nsNodeListSH(nsDOMClassInfoID aID) : nsDOMClassInfo(aID)
  {
  }

  virtual ~nsNodeListSH()
  {
  }

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsNodeListSH(aID);
  }
};

NS_IMETHODIMP
nsNodeListSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp,
                          PRBool *_retval)
{
  if (JSVAL_IS_INT(id)) {
    NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_AVAILABLE);

    PRInt32 n = JSVAL_TO_INT(id);

    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));

    nsCOMPtr<nsIDOMNodeList> list(do_QueryInterface(native));

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





  *_retval = PR_TRUE;

  return NS_OK;
}

// HTMLDocument scriptable helper

class nsHTMLDocumentSH : public nsEventPropSH
{
private:
  nsHTMLDocumentSH(nsDOMClassInfoID aID) : nsEventPropSH(aID)
  {
  }

  virtual ~nsHTMLDocumentSH()
  {
  }

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsHTMLDocumentSH(aID);
  }
};

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

    nsCOMPtr<nsIDOMHTMLDocument> doc(do_QueryInterface(native));

    JSString *jsstr = JSVAL_TO_STRING(id);

    nsLiteralString prop_name(NS_REINTERPRET_CAST(const PRUnichar *,
                                                  ::JS_GetStringChars(jsstr)),
                              ::JS_GetStringLength(jsstr));

    nsCOMPtr<nsIDOMNodeList> node_list;

    doc->GetElementsByName(prop_name, getter_AddRefs(node_list));

    if (node_list) {
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

      nsresult rv = sXPConnect->WrapNative(cx, ::JS_GetGlobalObject(cx),
                                           node_list,
                                           NS_GET_IID(nsIDOMNodeList),
                                           getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSObject* prop_obj = nsnull;
      holder->GetJSObject(&prop_obj);

      if (prop_obj) {
        *vp = OBJECT_TO_JSVAL(prop_obj);

*_retval = PR_TRUE;


        return NS_OK;
      }
    }
  }





  return nsEventPropSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}


// HTMLFormElement scriptable helper

class nsHTMLFormElementSH : public nsNodeSH
{
private:
  nsHTMLFormElementSH(nsDOMClassInfoID aID) : nsNodeSH(aID)
  {
  }

  virtual ~nsHTMLFormElementSH()
  {
  }

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsHTMLFormElementSH(aID);
  }
};

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

      nsCOMPtr<nsIDocument> doc;
      content->GetDocument(*getter_AddRefs(doc));

      nsCOMPtr<nsIDOMHTMLDocument> html_doc(do_QueryInterface(doc));

      if (html_doc) {
        nsCOMPtr<nsIDOMNodeList> node_list;





        // XXX This is wrong!

        html_doc->GetElementsByName(name, getter_AddRefs(node_list));

        if (node_list) {
          PRUint32 len;

          node_list->GetLength(&len);

          if (len) {
            result = node_list;
          }
        }
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

      if (prop_obj) {
        *vp = OBJECT_TO_JSVAL(prop_obj);
      }
    }
  }





  *_retval = PR_TRUE;

  return NS_OK;
}



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
  NS_DEFINE_CLASSINFO_DATA(Text, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Comment, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(CDATASection, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(ProcessingInstruction, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Entity, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(EntityReference, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(Notation, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(NodeList, nsNodeListSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(NamedNodeMap, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Event
  NS_DEFINE_CLASSINFO_DATA(Event, nsDOMGenericSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS);

  // Misc HTML classes
  NS_DEFINE_CLASSINFO_DATA(HTMLDocument, nsHTMLDocumentSH::Create,
                           DEFAULT_SCRIPTABLE_FLAGS | WANT_GETPROPERTY);

  // HTML element classes
  NS_DEFINE_CLASSINFO_DATA(HTMLAnchorElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLAppletElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLAreaElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBRElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseFontElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLBodyElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLButtonElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDListElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDelElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDirectoryElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLDivElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLEmbedElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFieldSetElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFontElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFormElement, nsHTMLFormElementSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameSetElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHRElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadingElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLHtmlElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLIFrameElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLImageElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLInputElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLInsElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLIsIndexElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLIElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLabelElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLegendElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLLinkElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMapElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMenuElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLMetaElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLModElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOListElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLObjectElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptGroupElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLParagraphElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLParamElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLPreElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLQuoteElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLScriptElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSelectElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSpacerElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLSpanElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLStyleElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCaptionElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCellElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableColElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableColGroupElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableRowElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTableSectionElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTextAreaElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLTitleElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLUListElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLUnknownElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
  NS_DEFINE_CLASSINFO_DATA(HTMLWBRElement, nsEventPropSH::Create,
                           ELEMENT_SCRIPTABLE_FLAGS);
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

