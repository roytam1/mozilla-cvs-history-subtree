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
 */
#ifndef nsDOMClassInfo_h___
#define nsDOMClassInfo_h___

#include "nsIDOMClassInfo.h"
#include "nsIXPCScriptable.h"
#include "jsapi.h"


struct nsDOMClassInfoData;
typedef void (*GetDOMClassIIDsFnc)(nsVoidArray& aArray);

class nsDOMClassInfo : public nsIXPCScriptable,
                       public nsIDOMClassInfo
{
public:
  nsDOMClassInfo(nsDOMClassInfoID aID);
  virtual ~nsDOMClassInfo();

  NS_DECL_NSIXPCSCRIPTABLE

  NS_DECL_ISUPPORTS

  NS_DECL_NSICLASSINFO

  // Helper method that returns a *non* refcounted pointer to a
  // helper. So please note, don't release this pointer, if you do,
  // you better make sure you've addreffed before release.
  //
  // Whaaaaa! I wanted to name this method GetClassInfo, but nooo,
  // some of Microsoft devstudio's headers #defines GetClassInfo to
  // GetClassInfoA so I can't, those $%#@^! bastards!!! What gives
  // them the right to do that?

  static nsISupports* GetClassInfoInstance(nsDOMClassInfoID aID,
                                           GetDOMClassIIDsFnc aGetIIDsFptr,
                                           const char *aName);

protected:
  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsDOMClassInfo(aID);
  }

  static nsresult Init();

  nsDOMClassInfoID mID;

  static nsDOMClassInfoData *sClassInfoData;
  static nsIXPConnect *sXPConnect;
  static PRUint32 sInstanceCount;

  // nsIXPCScriptable code
  static nsresult DefineStaticJSStrings(JSContext *cx);

  static JSString *sTop_id;
  static JSString *sScrollbars_id;
  static JSString *sLocation_id;
  static JSString *s_content_id;
  static JSString *sContent_id;
  static JSString *sSidebar_id;
  static JSString *sPrompter_id;
  static JSString *sMenubar_id;
  static JSString *sToolbar_id;
  static JSString *sLocationbar_id;
  static JSString *sPersonalbar_id;
  static JSString *sStatusbar_id;
  static JSString *sDirectories_id;
  static JSString *sControllers_id;
  static JSString *sLength_id;
  static JSString *sOnmousedown_id;
  static JSString *sOnmouseup_id;
  static JSString *sOnclick_id;
  static JSString *sOnmouseover_id;
  static JSString *sOnmouseout_id;
  static JSString *sOnkeydown_id;
  static JSString *sOnkeyup_id;
  static JSString *sOnkeypress_id;
  static JSString *sOnmousemove_id;
  static JSString *sOnfocus_id;
  static JSString *sOnblur_id;
  static JSString *sOnsubmit_id;
  static JSString *sOnreset_id;
  static JSString *sOnchange_id;
  static JSString *sOnselect_id;
  static JSString *sOnload_id;
  static JSString *sOnunload_id;
  static JSString *sOnabort_id;
  static JSString *sOnerror_id;
  static JSString *sOnpaint_id;
  static JSString *sOnresize_id;
  static JSString *sOnscroll_id;
};

typedef nsDOMClassInfo nsDOMGenericSH;


// EventProp scriptable helper, this class should be the base class of
// all objects that should support things like
// obj.onclick=function{...}

class nsEventRecieverSH : public nsDOMGenericSH
{
protected:
  nsEventRecieverSH(nsDOMClassInfoID aID) : nsDOMGenericSH(aID)
  {
  }

  virtual ~nsEventRecieverSH()
  {
  }

  static PRBool ReallyIsEventName(JSString *str);

  static inline PRBool IsEventName(JSString *jsstr)
  {
    jschar *str = ::JS_GetStringChars(jsstr);

    if (str[0] == 'o' && str[1] == 'n' && str[2]) {
      return ReallyIsEventName(jsstr);
    }

    return PR_FALSE;
  }

  nsresult RegisterCompileHandler(nsIXPConnectWrappedNative *wrapper,
                                  JSContext *cx, JSObject *obj, jsval id,
                                  jsval *vp, PRBool aCompile);

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);
};


// Window scirptable helper

class nsWindowSH : public nsEventRecieverSH
{
protected:
  nsWindowSH(nsDOMClassInfoID aID) : nsEventRecieverSH(aID)
  {
  }

  virtual ~nsWindowSH()
  {
  }

  // XXX does this need to be a member???
  nsresult GlobalResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, PRUint32 flags,
                         JSObject **objp, PRBool *_retval);

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp, PRBool *_retval);
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsWindowSH(aID);
  }
};


// DOM Node helper, this class deals with setting the parent for the
// wrappers

class nsNodeSH : public nsEventRecieverSH
{
protected:
  nsNodeSH(nsDOMClassInfoID aID) : nsEventRecieverSH(aID)
  {
  }

  virtual ~nsNodeSH()
  {
  }

public:
  NS_IMETHOD PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsNodeSH(aID);
  }
};


// Element helper

class nsElementSH : public nsNodeSH
{
protected:
  nsElementSH(nsDOMClassInfoID aID) : nsNodeSH(aID)
  {
  }

  virtual ~nsElementSH()
  {
  }

public:
  NS_IMETHOD Create(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                    JSObject *obj);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsElementSH(aID);
  }
};


// NodeList scriptable helper

class nsNodeListSH : public nsDOMClassInfo
{
protected:
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


// FomrControlList scriptable helper

class nsFormControlListSH : public nsNodeListSH
{
protected:
  nsFormControlListSH(nsDOMClassInfoID aID) : nsNodeListSH(aID)
  {
  }

  virtual ~nsFormControlListSH()
  {
  }

public:
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp,
                         PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsFormControlListSH(aID);
  }
};


// Document helper, for document.location and document.on*

class nsDocumentSH : public nsNodeSH
{
public:
  nsDocumentSH(nsDOMClassInfoID aID) : nsNodeSH(aID)
  {
  }

  virtual ~nsDocumentSH()
  {
  }

public:
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp, PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsDocumentSH(aID);
  }
};


// HTMLDocument helper

class nsHTMLDocumentSH : public nsDocumentSH
{
private:
  nsHTMLDocumentSH(nsDOMClassInfoID aID) : nsDocumentSH(aID)
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


// HTMLFormElement helper

class nsHTMLFormElementSH : public nsElementSH
{
private:
  nsHTMLFormElementSH(nsDOMClassInfoID aID) : nsElementSH(aID)
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


// HTMLOptionCollection helper

class nsHTMLOptionCollectionSH : public nsNodeListSH
{
private:
  nsHTMLOptionCollectionSH(nsDOMClassInfoID aID) : nsNodeListSH(aID)
  {
  }

  virtual ~nsHTMLOptionCollectionSH()
  {
  }

public:
  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp, PRBool *_retval);

  static nsIClassInfo *Create(nsDOMClassInfoID aID)
  {
    return new nsHTMLOptionCollectionSH(aID);
  }
};



/**
 * nsIClassInfo helper macros
 */

#define NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(_class)                          \
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {                                \
    foundInterface =                                                          \
      nsDOMClassInfo::GetClassInfoInstance(nsDOMClassInfo::e##_class##_id,    \
                                           Get##_class##IIDs,                 \
                                           #_class);                          \
    NS_ENSURE_TRUE(foundInterface, NS_ERROR_OUT_OF_MEMORY);                   \
  } else

#endif /* nsDOMClassInfo_h___ */
