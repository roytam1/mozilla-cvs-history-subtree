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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is mozilla.org.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Hammond (original author)
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
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimelineService.h"
#include "nsITimer.h"
#include "nsIArray.h"
#include "nsIAtom.h"
#include "prtime.h"
#include "nsString.h"

#include "nsPyContext.h"
#include "compile.h"
#include "eval.h"
#include "marshal.h"

#ifdef NS_DEBUG
nsPyDOMObjectLeakStats gLeakStats;
#endif

// Straight from nsJSEnvironment.
static inline const char *
AtomToEventHandlerName(nsIAtom *aName)
{
  const char *name;

  aName->GetUTF8String(&name);

#ifdef DEBUG
  const char *cp;
  char c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    NS_ASSERTION (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'),
                  "non-ASCII non-alphabetic event handler name");
  }
#endif

  return name;
}

nsPythonContext::nsPythonContext()
    : mGlobal(NULL)
{
  mIsInitialized = PR_FALSE;
  mNumEvaluations = 0;
  mOwner = nsnull;
  mScriptsEnabled = PR_TRUE;
  mProcessingScriptTag=PR_FALSE;
  mMapPyObjects.Init();
  PYLEAK_STAT_INCREMENT(ScriptContext);
}

nsPythonContext::~nsPythonContext()
{
  CleanPyNamespaces();
  PYLEAK_STAT_DECREMENT(ScriptContext);
}

// QueryInterface implementation for nsPythonContext
NS_INTERFACE_MAP_BEGIN(nsPythonContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContext)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptContext)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPythonContext)
NS_IMPL_RELEASE(nsPythonContext)


/*static*/ nsresult nsPythonContext::HandlePythonError()
{
  // need to raise an exception
  NS_WARNING("Python error calling DOM script");
  PyXPCOM_LogError("Python error");
  return PyXPCOM_SetCOMErrorFromPyException();
}

nsCAutoString
nsPythonContext::FixSource(const nsAString &aOrigSource)
{
  // As of python 2.3, this \r\n etc mangling is necessary.  However I heard
  // a rumour later versions were fixing that.
  nsCAutoString source;
  CopyUTF16toUTF8(aOrigSource, source);

  // Python insists on \n between lines and a trailing \n.  Fixup windows/mac

  // Windows linebreaks: Map CRLF to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r\n").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // Mac linebreaks: Map any remaining CR to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // trailing \n
  source.Append(NS_LITERAL_CSTRING("\n"));
  return source;
}

PyObject *
nsPythonContext::InternalCompile(const nsAString &aOrigSource, const char *url,
                                 PRUint32 lineNo)
{
  nsCAutoString source(FixSource(aOrigSource));
  // It is very hard to trick Python into using a different line number.
  // To do so would involve reimplementing PyParser_ParseStringFlagsFilename,
  // but that relies on Python's Parser/tokenizer.h, which is not included in
  // (windows at least) binary versions.
  // Another options is to trick Python by prepending lineNo '\n' chars
  // before the source, but that seems extreme!
  PyCompilerFlags cf;
  cf.cf_flags = PyCF_SOURCE_IS_UTF8;
  return Py_CompileStringFlags(source.get(), url, Py_file_input, &cf);
}

nsresult
nsPythonContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
  // Make sure callers of this use
  // WillInitializeContext/DidInitializeContext around this call?
  // We don't really care though.
  NS_ENSURE_TRUE(!mIsInitialized, NS_ERROR_ALREADY_INITIALIZED);
  CEnterLeavePython _celp;
  // Setup our global dict.
  Py_XDECREF(mGlobal);
  PyObject *obGlobal;
  if (aGlobalObject) {
    // must build the object with nsISupports, so it gets the automagic
    // interface flattening (I guess that is a bug in pyxpcom?)
    obGlobal = Py_nsISupports::PyObjectFromInterface(aGlobalObject,
                                                     NS_GET_IID(nsISupports),
                                                     PR_TRUE);
    if (!obGlobal)
      return HandlePythonError();
  } else {
    obGlobal = Py_None;
    Py_INCREF(Py_None);
  }
  mGlobal = Py_BuildValue("{s:N}", "this", obGlobal);
  if (!mGlobal)
    return HandlePythonError();
  // Add builtins to globals (not necessary if .py code does an exec ??)
  PyObject *bimod = PyImport_ImportModule("__builtin__");
  if (bimod == NULL || PyDict_SetItemString(mGlobal, "__builtins__", bimod) != 0) {
    NS_ERROR("can't add __builtins__ to __main__");
    return NS_ERROR_UNEXPECTED;
  }
  Py_DECREF(bimod);
  return NS_OK;
}

nsresult
nsPythonContext::EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    return NS_OK;
  }
  NS_ERROR("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::EvaluateString(const nsAString& aScript,
                            void *aScopeObject,
                            nsIPrincipal *aPrincipal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            PRUint32 aVersion,
                            nsAString *aRetValue,
                            PRBool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    *aIsUndefined = PR_TRUE;

    if (aRetValue) {
      aRetValue->Truncate();
    }

    return NS_OK;
  }

  NS_ERROR("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::ExecuteScript(void* aScriptObject,
                           void *aScopeObject,
                           nsAString* aRetValue,
                           PRBool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }
    if (aRetValue) {
      aRetValue->Truncate();
    }
    return NS_OK;
  }
  NS_ENSURE_TRUE(aScriptObject, NS_ERROR_NULL_POINTER);
  NS_ASSERTION(aScopeObject == nsnull || aScopeObject == mGlobal,
               "Global was changed??");
  PyObject *pyScopeObject = (PyObject *)aScopeObject;
  CEnterLeavePython _celp;
  if (pyScopeObject) {
    NS_ASSERTION(PyDict_Check(pyScopeObject), "aScopeObject globals not a dict?");
  // sigh - I still don't understand mGlobal vs aScopeObject :(
    NS_ASSERTION(pyScopeObject == mGlobal, "Scope is not the global?");
    NS_ENSURE_TRUE(PyDict_Check(pyScopeObject), NS_ERROR_UNEXPECTED);
  } else
    pyScopeObject = mGlobal;
  PyCodeObject *pyScriptObject = (PyCodeObject *)aScriptObject;
  NS_ASSERTION(PyCode_Check(pyScriptObject), "aScriptObject not a code object?");
  NS_ENSURE_TRUE(PyCode_Check(pyScriptObject), NS_ERROR_UNEXPECTED);
  PyObject *ret = PyEval_EvalCode(pyScriptObject, pyScopeObject, NULL);
  if (!ret) {
    HandlePythonError();
    // we should notify someone/something...
    // but either way, we return OK - it was "just" a script error
    return NS_OK;
  }
  Py_DECREF(ret);
  return NS_OK;
}

nsresult
nsPythonContext::CompileScript(const PRUnichar* aText,
                           PRInt32 aTextLength,
                           void *aScopeObject,
                           nsIPrincipal *aPrincipal,
                           const char *aURL,
                           PRUint32 aLineNo,
                           PRUint32 aVersion,
                           void** aScriptObject)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileScript");

  // nsDependentString looked good, but insists on no length!
  nsAutoString strSource(aText, aTextLength);

  CEnterLeavePython _celp;
  PyObject *co = InternalCompile(strSource, aURL, aLineNo);
  if (!co)
    return HandlePythonError();
  *aScriptObject = co;
  PYLEAK_STAT_INCREMENT(ScriptObject);
  return NS_OK;
}

nsresult
nsPythonContext::CompileEventHandler(nsIPrincipal *aPrincipal, nsIAtom *aName,
                                 const char *aEventName,
                                 const nsAString& aBody,
                                 const char *aURL, PRUint32 aLineNo,
                                 void** aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  CEnterLeavePython _celp;
  PyObject *co = InternalCompile(aBody, aURL, aLineNo);
  if (!co)
    return HandlePythonError();
  PYLEAK_STAT_INCREMENT(EventHandler);
  *aHandler = co;
  return NS_OK;
}

nsresult
nsPythonContext::BindCompiledEventHandler(nsISupports *aTarget, void *aScope,
                                          nsIAtom *aName, void *aHandler)
{
  PyObject *d = GetPyNamespaceFor(aTarget, PR_TRUE);
  if (!d)
    return HandlePythonError();
  if (!PyDict_Check(d)) {
    NS_ERROR("Expecting a Python dictionary!");
    return NS_ERROR_UNEXPECTED;
  }
  if (!aHandler || !PyCode_Check((PyObject *)aHandler)) {
    NS_ERROR("Expecting a Python code object");
    return NS_ERROR_UNEXPECTED;
  }
  const char *charName = AtomToEventHandlerName(aName);
  PyDict_SetItemString(d, (char *)charName, (PyObject *)aHandler);
  return NS_OK;
}


nsresult
nsPythonContext::CompileFunction(void* aTarget,
                             const nsACString& aName,
                             PRUint32 aArgCount,
                             const char** aArgArray,
                             const nsAString& aBody,
                             const char* aURL,
                             PRUint32 aLineNo,
                             PRBool aShared,
                             void** aFunctionObject)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ERROR("CompileFunction not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::CallEventHandler(nsISupports *aTarget, void *aScope, void* aHandler,
                                    nsIArray *aargv, nsISupports **arv)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    return NS_OK;
  }
  CEnterLeavePython _celp;
  PyObject *obEvent = NULL;
  PyObject *obTarget = NULL;
  PyObject *thisGlobals = NULL;
  PyObject *ret = NULL;
  PRBool ok = PR_FALSE;
  *arv = nsnull;
  // exit via goto
  thisGlobals = PyDict_Copy(mGlobal);
  if (!thisGlobals)
    goto done;

  // Could get perf by caching wrapped object
  obTarget = Py_nsISupports::PyObjectFromInterface(aTarget,
                                                  NS_GET_IID(nsISupports),
                                                  PR_TRUE);
  if (!obTarget)
    goto done;
  // XXX - what should this be exposed as?
  PyDict_SetItemString(thisGlobals, "target", obTarget);
  // This sucks - 'compile' is passed the literal name 'event', but that
  // is too early for us.  It also kinda sucks it is hard-coded as argv[0]
  if (aargv) {
    PRUint32 argc = 0;
    aargv->GetLength(&argc);
    if (argc) {
      nsCOMPtr<nsISupports> arg;
      nsresult rv;
      rv = aargv->QueryElementAt(0, NS_GET_IID(nsISupports), getter_AddRefs(arg));
      if (NS_SUCCEEDED(rv))
        obEvent = Py_nsISupports::PyObjectFromInterface(arg, NS_GET_IID(nsISupports),
                                                        PR_TRUE);
    }
  }
  if (obEvent)
    PyDict_SetItemString(thisGlobals, "event", obEvent);

  NS_ASSERTION(PyCode_Check((PyObject *)aHandler), "Not a Python code object");
  NS_ENSURE_TRUE(PyCode_Check((PyObject *)aHandler), NS_ERROR_UNEXPECTED);
  ret = PyEval_EvalCode((PyCodeObject *)aHandler,
                        thisGlobals, NULL);
  if (!ret)
    goto done;
  ok = Py_nsISupports::InterfaceFromPyObject(ret, NS_GET_IID(nsIVariant),
                                             arv, PR_TRUE, PR_TRUE);
done:
  nsresult rv = ok ? NS_OK : HandlePythonError();
  Py_XDECREF(ret);
  Py_XDECREF(obEvent);
  Py_XDECREF(obTarget);
  Py_XDECREF(thisGlobals);
  return rv;
}

nsresult
nsPythonContext::GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                      nsIAtom* aName,
                                      void** aHandler)
{
  *aHandler = nsnull;
  PyObject *ns = GetPyNamespaceFor(aTarget, PR_FALSE);
  if (!ns) {
    NS_WARNING("Nothing has been bound to this object");
    return NS_OK;
  }
  const char *charName = AtomToEventHandlerName(aName);
  PyObject *func = PyDict_GetItemString(ns, (char *)charName);
  if (!func) {
    NS_WARNING("No event handler of that name found");
    return NS_OK;
  }
  *aHandler = func;
  Py_INCREF(func);
  PYLEAK_STAT_INCREMENT(ScriptObject);
  return NS_OK;
}

nsresult
nsPythonContext::SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal)
{
    NS_ERROR("SetProperty not impl");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::InitClasses(void *aGlobalObj)
{
    return NS_OK;
}

nsresult
nsPythonContext::Serialize(nsIObjectOutputStream* aStream, void *aScriptObject)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::Serialize");
  CEnterLeavePython _celp;
  nsresult rv;
  if (!PyCode_Check((PyObject *)aScriptObject)) {
    NS_ERROR("aScriptObject is not a code object");
    return NS_ERROR_UNEXPECTED;
  }
  rv = aStream->Write32(PyImport_GetMagicNumber());
  if (NS_FAILED(rv)) return rv;

  PyObject *obMarshal =
#ifdef Py_MARSHAL_VERSION
      PyMarshal_WriteObjectToString((PyObject *)aScriptObject,
                                    Py_MARSHAL_VERSION);
#else
      // 2.3 etc - only takes 1 arg.
      PyMarshal_WriteObjectToString((PyObject *)aScriptObject);

#endif
  if (!obMarshal)
    return HandlePythonError();
  NS_ASSERTION(PyString_Check(obMarshal), "marshal returned a non string?");
  rv |= aStream->Write32(PyString_GET_SIZE(obMarshal));
  rv |= aStream->WriteBytes(PyString_AS_STRING(obMarshal), PyString_GET_SIZE(obMarshal));
  Py_DECREF(obMarshal);
  return rv;
}

nsresult
nsPythonContext::Deserialize(nsIObjectInputStream* aStream, void **aResult)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::Deserialize");
  nsresult rv;
  PRUint32 magic;
  rv = aStream->Read32(&magic);
  if (NS_FAILED(rv)) return rv;

  // Note that if we find a different marshalled version, we should
  // still read the bytes from the stream, but throw them away and
  // simply return nsnull in aResult.
  // Failure to do this throws the stream out for future objects.
  PRUint32 nBytes;
  rv = aStream->Read32(&nBytes);
  if (NS_FAILED(rv)) return rv;

  char* data = nsnull;
  rv = aStream->ReadBytes(nBytes, &data);
  if (NS_FAILED(rv)) return rv;

  if (magic != PyImport_GetMagicNumber()) {
    NS_WARNING("Python has different marshal version");
    if (data)
      nsMemory::Free(data);
    *aResult = nsnull;
    return NS_OK;
  }

  CEnterLeavePython _celp;
  PyObject *codeObject = PyMarshal_ReadObjectFromString(data, nBytes);
  if (data)
    nsMemory::Free(data);
  if (codeObject == NULL)
    return HandlePythonError();
  NS_ASSERTION(PyCode_Check(codeObject), "unmarshal returned a non string?");
  *aResult = codeObject;
  PYLEAK_STAT_INCREMENT(ScriptObject);
  return NS_OK;
}

void
nsPythonContext::SetDefaultLanguageVersion(PRUint32 aVersion)
{
  return;
}

nsIScriptGlobalObject *
nsPythonContext::GetGlobalObject()
{
  NS_ERROR("Not implemented");
  return nsnull;
}

void *
nsPythonContext::GetNativeContext()
{
  return nsnull;
}

void *
nsPythonContext::GetNativeGlobal()
{
  NS_ASSERTION(mGlobal, "GetNativeGlobal called before InitContext??");
  return mGlobal;
}

void
nsPythonContext::WillInitializeContext()
{
  mIsInitialized = PR_FALSE;
}

void
nsPythonContext::DidInitializeContext()
{
  mIsInitialized = PR_TRUE;
}

PRBool
nsPythonContext::IsContextInitialized()
{
  return mIsInitialized;
}

PRBool
nsPythonContext::GetProcessingScriptTag()
{
  return mProcessingScriptTag;
}

void
nsPythonContext::SetProcessingScriptTag(PRBool aResult)
{
  mProcessingScriptTag = aResult;
}

PRBool
nsPythonContext::GetScriptsEnabled()
{
  return mScriptsEnabled;
}

void
nsPythonContext::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
  // eeek - this seems the wrong way around - the global should callback
  // into the context.
  mScriptsEnabled = aEnabled;

  nsIScriptGlobalObject *global = GetGlobalObject();

  if (global) {
    global->SetScriptsEnabled(aEnabled, aFireTimeouts);
  }
}

void
nsPythonContext::SetOwner(nsIScriptContextOwner* owner)
{
  // The owner should not be addrefed!! We'll be told
  // when the owner goes away.
  mOwner = owner;
}

nsIScriptContextOwner *
nsPythonContext::GetOwner()
{
  return mOwner;
}

nsresult
nsPythonContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef)
{
    NS_ERROR("Term functions need thought");
    return NS_ERROR_UNEXPECTED;
}

void
nsPythonContext::ScriptEvaluated(PRBool aTerminated)
{
  ;
}

void
nsPythonContext::GC()
{
  ;
}

nsresult
nsPythonContext::FinalizeClasses(void *aGlobalObj)
{
  Py_XDECREF((PyObject *)aGlobalObj);
  CleanPyNamespaces();
  return NS_OK;
}

NS_IMETHODIMP
nsPythonContext::Notify(nsITimer *timer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

static PLDHashOperator
CleanupPyObjects(nsISupports *aKey,
                 PyObject *&aPyOb,
                 void *closure)
{
  NS_RELEASE(aKey);
  Py_DECREF(aPyOb);
  return PL_DHASH_REMOVE;
}

void nsPythonContext::CleanPyNamespaces()
{
  mMapPyObjects.Enumerate(CleanupPyObjects, nsnull);  
}

PyObject *nsPythonContext::GetPyNamespaceFor(nsISupports *aThing, PRBool bCreate)
{
  PyObject *ret = NULL;
  NS_ASSERTION(aThing, "Null pointer!");
  NS_ENSURE_TRUE(aThing, NULL);
  if (!mMapPyObjects.Get(aThing, &ret) && bCreate) {
    ret = PyDict_New();
    if (!ret)
      return NULL;
    // store the new object in the hash-table.  It then has ownership of the
    // reference.
    if (!mMapPyObjects.Put(aThing, ret)) {
      NS_ERROR("Failed to store in the hash-table");
      Py_DECREF(ret);
      return NULL;
    }
    NS_ADDREF(aThing);
  }
  return ret;
}