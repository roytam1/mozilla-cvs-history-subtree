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
#include "nsITimer.h"
#include "nsIArray.h"
#include "prtime.h"
#include "nsString.h"

#include "nsPyContext.h"

#include "compile.h"
#include "eval.h"

static PRInt32 sContextCount;

#ifdef NS_DEBUG
class nsPyDOMObjectLeakStats
  {
    public:
      nsPyDOMObjectLeakStats()
        : mEventHandlerCount(0), mScriptObjectCount(0), mBindingCount(0) {}

      ~nsPyDOMObjectLeakStats()
        {
          printf("nsPyDOMObjectLeakStats lost object counts\n");
          printf(" => mEventHandlerCount:   % 10d\n", mEventHandlerCount);
          printf(" => mScriptObjectCount:   % 10d\n", mScriptObjectCount);
          printf(" => mBindingCount:        % 10d\n", mBindingCount);
        }

      PRInt32 mEventHandlerCount;
      PRInt32 mScriptObjectCount;
      PRInt32 mBindingCount;
  };
static nsPyDOMObjectLeakStats gLeakStats;
#define PYLEAK_STAT_INCREMENT(_s) PR_AtomicIncrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_XINCREMENT(_what, _s) if (_what) PR_AtomicIncrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_DECREMENT(_s) PR_AtomicDecrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_XDECREMENT(_what, _s) if (_what) PR_AtomicDecrement(&gLeakStats.m ## _s ## Count)
#else
#define PYLEAK_STAT_INCREMENT(_s)
#define PYLEAK_STAT_XINCREMENT(_what, _s)
#define PYLEAK_STAT_DECREMENT(_s)
#define PYLEAK_STAT_XDECREMENT(_what, _s)
#endif

#ifndef NS_DEBUG
// non debug build store it once.  Debug builds refetch, allowing someone
// to reload(mod) to get new versions)
static PyObject *delegateModule = NULL;
#endif

nsPythonContext::nsPythonContext()
    : mGlobal(NULL)
{
  ++sContextCount;

  mIsInitialized = PR_FALSE;
  mNumEvaluations = 0;
  mOwner = nsnull;
  mScriptsEnabled = PR_TRUE;
  mProcessingScriptTag=PR_FALSE;
}

nsPythonContext::~nsPythonContext()
{
                  
  // Unregister our "javascript.options.*" pref-changed callback.
//  nsContentUtils::UnregisterPrefCallback(js_options_dot_str,
//                                         JSOptionChangedCallback,
//                                         this);

    --sContextCount;
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
  NS_WARNING("Python error");
  PyErr_Print(); // for now.
  return PyXPCOM_SetCOMErrorFromPyException();
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
  if (!ret)
    return HandlePythonError();
//  PyObject *obu = PyUnicode_FromObject(obRet);
//  if (obRet)
//    PyUnicode_AS_UNICODE(val_use)
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

  // XXX - todo - specify PyCF_SOURCE_IS_UTF8 in Py_CompileStringFlags
  NS_ConvertUTF16toUTF8 cs(aText, aTextLength);
  // XXX - need to instantiate a tokenizer and set its lineno member.
  // ignore that for now.

  // Python insists on \n between lines and a trailing \n.  Fixup windows/mac
  nsCAutoString source(cs);  // hope this does copy-on-write

  // Windows linebreaks: Map CRLF to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r\n").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // Mac linebreaks: Map any remaining CR to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // trailing \n
  source.Append(NS_LITERAL_CSTRING("\n"));
  CEnterLeavePython _celp;
  PyObject *co = Py_CompileString(source.get(), aURL, Py_file_input);
  if (!co)
    return HandlePythonError();
  *aScriptObject = co;
  PYLEAK_STAT_INCREMENT(ScriptObject);
  return NS_OK;
}

nsresult
nsPythonContext::CompileEventHandler(nsIScriptBinding *aTarget, nsIAtom *aName,
                                 const char *aEventName,
                                 const nsAString& aBody,
                                 const char *aURL, PRUint32 aLineNo,
                                 PRBool aShared, void** aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  // shameless clone of above - let's see what happens before rationalizing
  // XXX - todo - specify PyCF_SOURCE_IS_UTF8 in Py_CompileStringFlags
  nsCAutoString cs;
  CopyUTF16toUTF8(aBody, cs);
  // XXX - need to instantiate a tokenizer and set its lineno member.
  // ignore that for now.

  // Python insists on \n between lines and a trailing \n.  Fixup windows/mac
  nsCAutoString source(cs);  // hope this does copy-on-write

  // Windows linebreaks: Map CRLF to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r\n").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // Mac linebreaks: Map any remaining CR to LF:
  source.ReplaceSubstring(NS_LITERAL_CSTRING("\r").get(),
                          NS_LITERAL_CSTRING("\n").get());

  // trailing \n
  source.Append(NS_LITERAL_CSTRING("\n"));
  CEnterLeavePython _celp;
  PyObject *co = Py_CompileString(source.get(), aURL, Py_file_input);
  if (!co)
    return HandlePythonError();
  PYLEAK_STAT_INCREMENT(EventHandler);
  *aHandler = co;
  // If we were passed a handler, bind to it.
  nsresult rv = NS_OK;
  if (aTarget) {
    NS_ASSERTION(aTarget->GetNativeObject() == nsnull, "Can't already have a native");
    rv = BindCompiledEventHandler(aTarget, aName, *aHandler);
  }
  return rv;
}

nsresult
nsPythonContext::BindCompiledEventHandler(nsIScriptBinding *aTarget, nsIAtom *aName,
                                      void *aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_ASSERTION(aTarget->GetLanguage() == nsIProgrammingLanguage::PYTHON,
               "Must be a Python binder!?");
  nsPyScriptBinding *pyBinding = (nsPyScriptBinding *)aTarget;
  NS_ASSERTION(pyBinding->mCodeObject == nsnull, "Already bound?");
  // XXX - not taking a reference to aHandler - it is assumed we steal the
  // lifetime from the caller.
  pyBinding->mCodeObject = (PyObject *)aHandler;
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
nsPythonContext::CallEventHandler(nsIScriptBinding *aTarget, void* aHandler,
                                    nsIArray *aargv, nsISupports **arv)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    return NS_OK;
  }
  nsPyScriptBinding *pyBinding = (nsPyScriptBinding *)aTarget;
  NS_ENSURE_TRUE(pyBinding->mCodeObject, NS_ERROR_UNEXPECTED);

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
  obTarget = Py_nsISupports::PyObjectFromInterface(pyBinding->mHolder,
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


  ret = PyEval_EvalCode((PyCodeObject *)pyBinding->mCodeObject,
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
nsPythonContext::GetScriptBinding(nsISupports *aObject, void *aScope,
                              nsIScriptBinding **aBinding)
{
    *aBinding = new nsPyScriptBinding(aObject);
    if (!*aBinding)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_IF_ADDREF(*aBinding);
    return NS_OK;
}

nsresult
nsPythonContext::GetScriptBindingHandler(nsIScriptBinding *aBinding,
                                     nsString &name,
                                     void **handler)
{
  NS_ASSERTION(aBinding->GetLanguage() == nsIProgrammingLanguage::PYTHON,
               "Not a Python binding!?");
  nsPyScriptBinding *pyBinding = (nsPyScriptBinding *)aBinding;
  *handler = pyBinding->mCodeObject;
  return NS_OK;
}

nsresult
nsPythonContext::SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal)
{
    NS_ERROR("SetProperty not impl");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::AddGCRoot(void* aScriptObjectRef, const char* aName)
{
    return NS_OK;
}

nsresult
nsPythonContext::RemoveGCRoot(void* aScriptObjectRef)
{
    return NS_OK;
}

nsresult
nsPythonContext::InitClasses(void *aGlobalObj)
{
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
  return NS_OK;
}

NS_IMETHODIMP
nsPythonContext::Notify(nsITimer *timer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

// nsPyScriptBinding
// QueryInterface implementation for nsPyScriptBinding
NS_INTERFACE_MAP_BEGIN(nsPyScriptBinding)
  NS_INTERFACE_MAP_ENTRY(nsIScriptBinding)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsPyScriptBinding)
NS_IMPL_RELEASE(nsPyScriptBinding)

nsPyScriptBinding::nsPyScriptBinding(nsISupports *aObject) :
  mHolder(aObject), mCodeObject(NULL)
{
  PYLEAK_STAT_INCREMENT(Binding);

}

nsPyScriptBinding::~nsPyScriptBinding()
{
  PYLEAK_STAT_XDECREMENT(mCodeObject, EventHandler);
  Py_XDECREF(mCodeObject);
  PYLEAK_STAT_DECREMENT(Binding);
}

void *
nsPyScriptBinding::GetNativeObject()
{
  NS_ERROR("GetNativeObject not impl");
  return nsnull;
}

nsISupports *
nsPyScriptBinding::GetTarget()
{
  return mHolder;
}
