/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla JSCodeLib.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "mozJSCodeLib.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "JSAutoContext.h" 
#include "JSBackstagePass.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIJSContextStack.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

const char kJSContextStackContractID[]   = "@mozilla.org/js/xpc/ContextStack;1";
const char kJSRuntimeServiceContractID[] = "@mozilla.org/js/xpc/RuntimeService;1";

#ifndef XPCONNECT_STANDALONE
#include "nsIScriptSecurityManager.h"
#include "nsIScriptObjectPrincipal.h"

const char kScriptSecurityManagerContractID[] = NS_SCRIPTSECURITYMANAGER_CONTRACTID;
#endif

static JSFunctionSpec gGlobalFunctions[] = {
  {"dump", JSDump, 1},
  {"debug", JSDebug, 1 },
  {"importModule", JSImportModule, 1 },
  {0}
};

////////////////////////////////////////////////////////////////////////
// mozJSCodeLib implementation

mozJSCodeLib::mozJSCodeLib()
{
}

mozJSCodeLib::~mozJSCodeLib()
{
  UnloadModules();
}

nsresult mozJSCodeLib::Init()
{
  mXPConnect = do_GetService(nsIXPConnect::GetCID());
  if (!mXPConnect) {
    NS_ERROR("failed to get xpconnect service");
    return NS_ERROR_FAILURE;
  }
  
  mRuntimeService = do_GetService(kJSRuntimeServiceContractID);
  if (!mRuntimeService) {
    NS_ERROR("could not get JSRuntimeService");
    return NS_ERROR_FAILURE;
  }

#ifndef XPCONNECT_STANDALONE
  nsCOMPtr<nsIScriptSecurityManager> secman = 
    do_GetService(kScriptSecurityManagerContractID);
  if (!secman) {
    NS_ERROR("could not get script security manager");
    return NS_ERROR_FAILURE;
  }
  
  secman->GetSystemPrincipal(getter_AddRefs(mSystemPrincipal));
  if (!mSystemPrincipal) {
    NS_ERROR("could not get system principal");
    return NS_ERROR_FAILURE;
  }
#endif

  if (!mModules.Init(4))
    return NS_ERROR_FAILURE;
  return NS_OK;
}
  
//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_THREADSAFE_ADDREF(mozJSCodeLib)
NS_IMPL_THREADSAFE_RELEASE(mozJSCodeLib)

NS_INTERFACE_MAP_BEGIN(mozJSCodeLib)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(mozIJSCodeLib)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// mozIJSCodeLib methods

/* void importModule (in AUTF8String moduleURL, [optional] in JSObject obj); */
NS_IMETHODIMP
mozJSCodeLib::ImportModule(const nsACString & moduleName)
{
  // This function should only be called from JS.

  nsCOMPtr<nsIXPCNativeCallContext> cc;
  mXPConnect->GetCurrentNativeCallContext(getter_AddRefs(cc));
  if (!cc) {
    NS_ERROR("could not get native call context");
    return NS_ERROR_FAILURE;
  }

  JSContext *cx = nsnull;
  cc->GetJSContext (&cx);
  
  JSObject *targetObject = nsnull;
  
  PRUint32 argc = 0;
  cc->GetArgc(&argc);

  if (argc>1) {
    // The caller passed in the optional second argument. Get it.    
    jsval *argv = nsnull;
    cc->GetArgvPtr (&argv);

    if (!JS_ValueToObject(cx, argv[1], &targetObject)) {
      cc->SetExceptionWasThrown(JS_TRUE);
      return NS_OK;
    }
    if (!targetObject) {
      // This happens when the caller passes in 'null' as targetObject.
      return NS_ERROR_FAILURE;
    }
  }
  else {
    // Our targetObject is the caller's global object. Find it by
    // walking the calling object's parent chain.
    
    nsCOMPtr<nsIXPConnectWrappedNative> wn;
    cc->GetCalleeWrapper (getter_AddRefs(wn));
    if (!wn) {
      NS_ERROR("null callee wrapper");
      return NS_ERROR_FAILURE;    
    }
    wn->GetJSObject (&targetObject);
    if (!targetObject) {
      NS_ERROR("null calling object");
      return NS_ERROR_FAILURE;
    }
    
    JSObject *parent = nsnull;
    while ((parent = JS_GetParent(cx, targetObject)))
      targetObject = parent;
    
  }
  NS_ASSERTION(targetObject, "null targetObject");

  return ImportModuleToJSObject(moduleName, targetObject);
}

/* [noscript] void importModuleToJSObject (in AUTF8String moduleURL, in JSObjectPtr obj); */
NS_IMETHODIMP
mozJSCodeLib::ImportModuleToJSObject(const nsACString & moduleName, JSObject * targetObj)
{
#ifdef DEBUG
  printf("mozJSCodeLib::ImportModuleToJSObject(%s,%p)\n", PromiseFlatCString(moduleName).get(), targetObj);
#endif
  if (!targetObj) {
    NS_ERROR("null target object");
    return NS_ERROR_FAILURE;
  }

  JSObject *moduleObj = nsnull;
  if (!mModules.Get(moduleName, &moduleObj)) {
    moduleObj = LoadModule(moduleName);
  }
  if (!moduleObj) {
    return NS_ERROR_FAILURE;
  }

  // Retrieve the code module's MOZ_EXPORTED_SYMBOLS array:

  // we need a context; any will do
  nsCOMPtr<nsIThreadJSContextStack> cxstack = 
    do_GetService(kJSContextStackContractID);
  JSContext*cx = nsnull;
  cxstack->GetSafeJSContext(&cx);

  jsval symbols;
  if (!JS_GetProperty(cx, moduleObj, "MOZ_EXPORTED_SYMBOLS", &symbols)) {
    NS_ERROR("error finding MOZ_EXPORTED_SYMBOLS");
    return NS_ERROR_FAILURE;
  }

  JSObject *symbolsObj = nsnull;
  if (!JSVAL_IS_OBJECT(symbols) ||
      !(symbolsObj = JSVAL_TO_OBJECT(symbols)) ||
      !JS_IsArrayObject(cx, symbolsObj)) {
    NS_ERROR("MOZ_EXPORTED_SYMBOLS is not an array");
    return NS_ERROR_FAILURE;
  }

  // Iterate over symbols array, installing symbols on targetObj:

  jsuint symbolCount = 0;
  if (!JS_GetArrayLength(cx, symbolsObj, &symbolCount)) {
    NS_ERROR("Error getting array length");
    return NS_ERROR_FAILURE;
  }
#ifdef DEBUG
  printf("Installing symbols [ ");
#endif

  for (jsuint i=0; i<symbolCount; ++i) {
    jsval val;
    JSString *symbolName;
    if (!JS_GetElement(cx, symbolsObj, i, &val) ||
        !JSVAL_IS_STRING(val) ||
        !(symbolName = JSVAL_TO_STRING(val))) {
      NS_ERROR("Array element is not a string");
      return NS_ERROR_FAILURE;
    }
#ifdef DEBUG
    printf("%s ", JS_GetStringBytes(symbolName));
#endif
    
    if (!JS_GetProperty(cx, moduleObj, JS_GetStringBytes(symbolName), &val)) {
      NS_ERROR("Could not get symbol");
      return NS_ERROR_FAILURE;
    }

    if (!JS_SetProperty(cx, targetObj, JS_GetStringBytes(symbolName), &val)) {
      NS_ERROR("Could not set property on target");
      return NS_ERROR_FAILURE;
    }
  }
#ifdef DEBUG
  printf("]\n");
#endif
  
  return NS_OK;
}

/* JSObject probeModule (in AUTF8String moduleURL, [optional] in boolean force); */
NS_IMETHODIMP
mozJSCodeLib::ProbeModule(const nsACString & moduleName)
{
  // This function should only be called from JS.

  nsCOMPtr<nsIXPCNativeCallContext> cc;
  mXPConnect->GetCurrentNativeCallContext(getter_AddRefs(cc));
  if (!cc) {
    NS_ERROR("could not get native call context");
    return NS_ERROR_FAILURE;
  }

  JSBool force = PR_FALSE;
  
  PRUint32 argc = 0;
  cc->GetArgc(&argc);

  if (argc>1) {
    // The caller passed in the optional second argument. Get it.
    JSContext *cx = nsnull;
    cc->GetJSContext (&cx);
    
    jsval *argv = nsnull;
    cc->GetArgvPtr (&argv);

    if (!JS_ValueToBoolean(cx, argv[1], &force)) {
      cc->SetExceptionWasThrown(JS_TRUE);
      return NS_OK;
    }
  }
    
  JSObject *moduleObj = nsnull;
  if (!mModules.Get(moduleName, &moduleObj)) {
    moduleObj = LoadModule(moduleName, force);
  }

  jsval *retval = nsnull;
  cc->GetRetValPtr(&retval);
  if (*retval)
    *retval = moduleObj ? OBJECT_TO_JSVAL(moduleObj) : JSVAL_NULL;
  
  return NS_OK;
}

//----------------------------------------------------------------------
// implementation helpers:

PRInt32
mozJSCodeLib::LoadURL(char **buf, const nsACString &url)
{
  *buf = nsnull;
  
  PRInt32 content_length = -1;
  PRUint32 bytesRead = 0;

  nsCOMPtr<nsIIOService> ioserv;
  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsIInputStream> instream;
  
  ioserv = do_GetService(kIOServiceCID);
  if (!ioserv) {
    NS_WARNING("could not get io service");
    goto failure;
  }

  ioserv->NewChannel(url, nsnull, nsnull, getter_AddRefs(channel));
  if (!channel) {
    NS_WARNING("could not create channel");
    goto failure;
  }
  
  channel->Open(getter_AddRefs(instream));
  if (!instream) {
    NS_WARNING("could not open stream");
    goto failure;
  }

  if (NS_FAILED(channel->GetContentLength(&content_length))) {
    NS_WARNING("could not get content length");
    goto failure;
  }
  
  *buf = new char[content_length+1];
  if (!*buf) {
    NS_WARNING("could not alloc buffer");
    goto failure;
  }

  instream->Read(*buf, content_length, &bytesRead);
  if (bytesRead != content_length) {
    NS_WARNING("stream read error");
    goto failure;
  }

  return content_length;
  
  failure:
  if (*buf)
    delete[] *buf;
  return -1;
}

JSObject *
mozJSCodeLib::LoadModule(const nsACString &module, PRBool force)
{
  JSObject *moduleObj = nsnull;
  char *buf = nsnull;
  PRInt32 content_length = LoadURL(&buf, module);
  if (!buf && !force) {
    NS_ERROR("error loading url");
    return nsnull;
  }
  
  jsval result;
  JSAutoContext cx;
  JSPrincipals* jsprincipals = nsnull;

#ifndef XPCONNECT_STANDALONE
  nsCOMPtr<nsIScriptObjectPrincipal> backstagePass =
    new JSBackstagePass(mSystemPrincipal);  
#else
  nsCOMPtr<nsISupports> backstagePass = new JSBackstagePass();
#endif
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  mXPConnect->InitClassesWithNewWrappedGlobal(cx, backstagePass,
                                              NS_GET_IID(nsISupports),
                                              PR_FALSE,
                                              getter_AddRefs(holder));
  if (!holder) {
    NS_ERROR("global initialization failed");
    goto done;
  }
  
  holder->GetJSObject(&moduleObj);
  if (!moduleObj) {
    NS_ERROR("bad global object");
    goto done;
  }
  
  if (!JS_DefineFunctions(cx, moduleObj, gGlobalFunctions)) {
    moduleObj = nsnull;
    NS_ERROR("error setting functions");
    goto done;
  }
  
#ifndef XPCONNECT_STANDALONE
  mSystemPrincipal->GetJSPrincipals(cx, &jsprincipals);
  if (!jsprincipals) {
    moduleObj = nsnull;
    NS_ERROR("could not get principals");
    goto done;
  }
#endif
  
  if (buf && !JS_EvaluateScriptForPrincipals(cx, moduleObj,
                                             jsprincipals, buf,
                                             content_length,
                                             PromiseFlatCString(module).get(), 1, &result)) {
    moduleObj = nsnull;
  }
  else {
    // Add obj to hash and protect from gc until unloaded in
    // UnloadModules()
    ModulesHash::EntryType *entry = mModules.PutEntry(module);
    if (!entry) {
      moduleObj = nsnull;
      goto done;
    }
    entry->mData = moduleObj;
    JS_AddNamedRoot(cx, &(entry->mData), "mozJSCodeLib");
  }
  
  done:
  if (jsprincipals)
    JSPRINCIPALS_DROP(cx, jsprincipals);  
  if (buf)
    delete[] buf;
  return moduleObj;
}

PR_STATIC_CALLBACK(PLDHashOperator)
UnrootModulesCallback(const nsACString& key, JSObject* &module, void* arg)
{
  JS_RemoveRootRT((JSRuntime*)arg, &module);
  return PL_DHASH_REMOVE;
}

void
mozJSCodeLib::UnloadModules()
{
#ifdef DEBUG
  printf("mozJSCodeLib::UnloadModules()\n");
#endif
  JSRuntime *rt = nsnull;
  mRuntimeService->GetRuntime(&rt);
  if (!rt) {
    NS_ERROR("null js runtime");
    return;
  }
  mModules.Enumerate(UnrootModulesCallback, rt);
}
