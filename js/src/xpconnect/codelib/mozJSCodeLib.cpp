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
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIJSContextStack.h"
#include "nsIXPCScriptable.h"
#include "nsCRT.h"
#include "xpcprivate.h"
#include "nsIURI.h"
#include "nsIFileURL.h"
#include "nsILocalFile.h"

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
  {0}
};

////////////////////////////////////////////////////////////////////////
// mozJSCodeLib implementation

mozJSCodeLib::mozJSCodeLib()
    : mModules(nsnull)
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
  
  JSRuntime *rt = nsnull;
  mRuntimeService->GetRuntime(&rt);
  if (!rt) {
    NS_ERROR("null js runtime");
    return NS_ERROR_FAILURE;
  }

  // Create our compilation context.
  mContext = JS_NewContext(rt, 256);
  if (!mContext)
    return NS_ERROR_OUT_OF_MEMORY;
  
  uint32 options = JS_GetOptions(mContext);
  JS_SetOptions(mContext, options | JSOPTION_XML);
    
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

  mModules = PL_NewHashTable(16, PL_HashString, PL_CompareStrings,
                             PL_CompareValues, 0, 0);
  if (!mModules)
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}
  
//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_THREADSAFE_ADDREF(mozJSCodeLib)
NS_IMPL_THREADSAFE_RELEASE(mozJSCodeLib)

NS_INTERFACE_MAP_BEGIN(mozJSCodeLib)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(xpcIJSCodeLoader)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// xpcIJSCodeLoader methods

/* JSObject importModule (in AUTF8String moduleURL, [optional] in JSObject targetObj); */
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

  JSObject *globalObj = nsnull;
  nsresult rv = ImportModuleToJSObject(moduleName, targetObject, &globalObj);

  jsval *retval = nsnull;
  cc->GetRetValPtr(&retval);
  if (*retval)
    *retval = globalObj ? OBJECT_TO_JSVAL(globalObj) : JSVAL_NULL;
    
  return rv;
}

/* [noscript] JSObjectPtr importModuleToJSObject (in AUTF8String moduleURL, in JSObjectPtr targetObj); */
NS_IMETHODIMP
mozJSCodeLib::ImportModuleToJSObject(const nsACString & moduleName, JSObject * targetObj, JSObject * *_retval)
{
  JSObject *moduleObj = nsnull;
  const char *location = PromiseFlatCString(moduleName).get();
  PLHashNumber hash = PL_HashString(location);
  PLHashEntry **hep = PL_HashTableRawLookup(mModules, hash,
                                            (void *)location);
  PLHashEntry *he = *hep;
  if (he) {
    moduleObj = (JSObject *)he->value;
  }
  else {
    moduleObj = LoadModule(moduleName, hep);
  }
  if (!moduleObj) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  *_retval = moduleObj;
  
  if (targetObj) {
    // Retrieve the code module's EXPORTED_SYMBOLS array:
    
    jsval symbols;
    if (!JS_GetProperty(mContext, moduleObj, "EXPORTED_SYMBOLS", &symbols)) {
      NS_ERROR("error finding EXPORTED_SYMBOLS");
      return NS_ERROR_FAILURE;
    }

    JSObject *symbolsObj = nsnull;
    if (!JSVAL_IS_OBJECT(symbols) ||
        !(symbolsObj = JSVAL_TO_OBJECT(symbols)) ||
        !JS_IsArrayObject(mContext, symbolsObj)) {
      NS_ERROR("EXPORTED_SYMBOLS is not an array");
      return NS_ERROR_FAILURE;
    }

    // Iterate over symbols array, installing symbols on targetObj:

    jsuint symbolCount = 0;
    if (!JS_GetArrayLength(mContext, symbolsObj, &symbolCount)) {
      NS_ERROR("Error getting array length");
      return NS_ERROR_FAILURE;
    }
#ifdef DEBUG
    printf("Installing symbols [ ");
#endif
    
    for (jsuint i=0; i<symbolCount; ++i) {
      jsval val;
      JSString *symbolName;
      if (!JS_GetElement(mContext, symbolsObj, i, &val) ||
          !JSVAL_IS_STRING(val) ||
          !(symbolName = JSVAL_TO_STRING(val))) {
        NS_ERROR("Array element is not a string");
        return NS_ERROR_FAILURE;
      }
#ifdef DEBUG
      printf("%s ", JS_GetStringBytes(symbolName));
#endif
    
      if (!JS_GetProperty(mContext, moduleObj, JS_GetStringBytes(symbolName), &val)) {
        NS_ERROR("Could not get symbol");
        return NS_ERROR_FAILURE;
      }
      
      if (!JS_SetProperty(mContext, targetObj, JS_GetStringBytes(symbolName), &val)) {
        NS_ERROR("Could not set property on target");
        return NS_ERROR_FAILURE;
      }
    }
#ifdef DEBUG
    printf("] from %s\n", PromiseFlatCString(moduleName).get());
#endif
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------
// implementation helpers:

PRBool
mozJSCodeLib::LoadURL(nsCString &buf, const nsACString &url,
                      nsILocalFile** localfile)
{  
  nsCOMPtr<nsIIOService> ioserv = do_GetService(kIOServiceCID);
  if (!ioserv) {
    NS_WARNING("could not get io service");
    return false;
  }

  nsCOMPtr<nsIChannel> channel;
  ioserv->NewChannel(url, nsnull, nsnull, getter_AddRefs(channel));
  if (!channel) {
    NS_WARNING("could not create channel");
    return false;
  }

  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIFileURL> fileURL;
  channel->GetURI(getter_AddRefs(uri));
  if (uri && (fileURL = do_QueryInterface(uri))) {
    nsCOMPtr<nsIFile> file;
    fileURL->GetFile(getter_AddRefs(file));
    if (file)
      file->QueryInterface(NS_GET_IID(nsILocalFile), (void**)localfile);
  }
  
  nsCOMPtr<nsIInputStream> instream;
  channel->Open(getter_AddRefs(instream));
  if (!instream) {
    NS_WARNING("could not open stream");
    return false;
  }

  PRUint32 avail, read;
  nsresult rv;
    
  while(1) {
    rv = instream->Available(&avail);
    if (NS_FAILED(rv)) {
      NS_WARNING("stream error");
      return false;
    }
    if (avail == 0)
      break;

    buf.SetLength(buf.Length() + avail);
    char *p = buf.EndWriting() - avail;
    rv = instream->Read(p, avail, &read);
    if (NS_FAILED(rv) || read != avail) {
      NS_WARNING("read error");
      return false;
    }
  }
  
  return true;
}

JSObject *
mozJSCodeLib::LoadModule(const nsACString &module, PLHashEntry **hep)
{
  JSObject *moduleObj = nsnull;
  nsCString buf;
  nsCOMPtr<nsILocalFile> localFile;
  if (!LoadURL(buf, module, getter_AddRefs(localFile))) {
    NS_ERROR("error loading url");
    return nsnull;
  }
  
  jsval result;
  JSContextHelper cx(mContext);
  JSPrincipals* jsprincipals = nsnull;
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  nsCOMPtr<nsIXPCScriptable> backstagePass;
  mRuntimeService->GetBackstagePass(getter_AddRefs(backstagePass));
  if (!backstagePass) {
    NS_ERROR("can't get backstagePass");
    goto done;
  }
  
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

  if (localFile) {
    nsCOMPtr<nsIXPConnectJSObjectHolder> locationHolder;
    mXPConnect->WrapNative(cx, moduleObj, localFile,
                           NS_GET_IID(nsILocalFile),
                           getter_AddRefs(locationHolder));
    if (!locationHolder) {
      moduleObj = nsnull;
      NS_ERROR("could not wrap location object");
      goto done;
    }
    JSObject *locationObj;
    if (NS_FAILED(locationHolder->GetJSObject(&locationObj))) {
      moduleObj = nsnull;
      goto done;
    }
    if (!JS_DefineProperty(cx, moduleObj, "__LOCATION__",
                           OBJECT_TO_JSVAL(locationObj), NULL,
                           NULL, 0)) {
      moduleObj = nsnull;
      NS_ERROR("error setting __LOCATION__ property");
      goto done;
    }
  }
  
#ifndef XPCONNECT_STANDALONE
  mSystemPrincipal->GetJSPrincipals(cx, &jsprincipals);
  if (!jsprincipals) {
    moduleObj = nsnull;
    NS_ERROR("could not get principals");
    goto done;
  }
#endif
  
  if (!JS_EvaluateScriptForPrincipals(cx, moduleObj,
                                      jsprincipals,
                                      PromiseFlatCString(buf).get(),
                                      buf.Length(),
                                      PromiseFlatCString(module).get(), 1, &result)) {
    moduleObj = nsnull;
  }
  else {
    // Add obj to hash and protect from gc until unloaded in
    // UnloadModules()
    char *location = nsCRT::strdup(PromiseFlatCString(module).get());
    PLHashNumber hash = PL_HashString(location);
    PLHashEntry *he = PL_HashTableRawAdd(mModules, hep, hash, location, moduleObj);
    JS_AddNamedRoot(cx, &he->value, location);
  }
  
  done:
  if (jsprincipals)
    JSPRINCIPALS_DROP(cx, jsprincipals);  
  return moduleObj;
}

static PRIntn PR_CALLBACK
UnrootModulesCallback(PLHashEntry *he, PRIntn i, void *arg)
{
    JSRuntime *rt = (JSRuntime *)arg;
    JS_RemoveRootRT(rt, &he->value);
    nsCRT::free((char *)he->key);
    return HT_ENUMERATE_REMOVE;
}

void
mozJSCodeLib::UnloadModules()
{
#ifdef DEBUG
  printf("mozJSCodeLib::UnloadModules()\n");
#endif
  
  if (!mModules) return;

  JSRuntime *rt = nsnull;
  mRuntimeService->GetRuntime(&rt);
  if (!rt) {
    NS_ERROR("null js runtime");
    return;
  }

  PL_HashTableEnumerateEntries(mModules, UnrootModulesCallback, rt);
  PL_HashTableDestroy(mModules);
  mModules = nsnull;

  // Destroying our context will force a GC.
  JS_DestroyContext(mContext);
  mContext = nsnull;

  mRuntimeService = nsnull;
}
