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
#include "nsGUIEvent.h"

#include "nsPyDOM.h"
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

nsPythonContext::nsPythonContext() :
    mIsInitialized(PR_FALSE),
    mOwner(nsnull),
    mScriptGlobal(nsnull),
    mScriptsEnabled(PR_TRUE),
    mProcessingScriptTag(PR_FALSE),
    mDelegate(NULL)
{
  PYLEAK_STAT_INCREMENT(ScriptContext);
}

nsPythonContext::~nsPythonContext()
{
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


nsresult nsPythonContext::HandlePythonError()
{
  // It looks like we first need to call the DOM.  Depending on the result
  // of the DOM error handler, we may then just treat it as unhandled.
  if (!PyErr_Occurred())
    return NS_OK;

  nsScriptErrorEvent errorevent(PR_TRUE, NS_SCRIPT_ERROR);
  nsAutoString strFilename;

  PyObject *exc, *typ, *tb;
  PyErr_Fetch(&exc, &typ, &tb);
  PyErr_NormalizeException( &exc, &typ, &tb);
  // We must have a traceback object to report the filename/lineno.
  // *sob* - PyTracebackObject not in a header file in 2.3.  Use getattr etc
  if (tb) {
    PyObject *frame = PyObject_GetAttrString(tb, "tb_frame");
    if (frame) {
      PyObject *obLineNo = PyObject_GetAttrString(frame, "f_lineno");
      if (obLineNo) {
        errorevent.lineNr = PyInt_AsLong(obLineNo);
        Py_DECREF(obLineNo);
      } else {
        NS_ERROR("Traceback had no lineNo attribute?");
        PyErr_Clear();
      }
      PyObject *code = PyObject_GetAttrString(frame, "f_code");
      if (code) {
        PyObject *filename = PyObject_GetAttrString(code, "co_filename");
        if (filename && PyString_Check(filename)) {
          CopyUTF8toUTF16(PyString_AsString(filename), strFilename);
          errorevent.fileName = strFilename.get();
        }
        Py_XDECREF(filename);
        Py_DECREF(code);
      }
      Py_DECREF(frame);
    }
  }
  PRBool outOfMem = PyErr_GivenExceptionMatches(exc, PyExc_MemoryError);
  
  nsCAutoString cerrMsg;
  PyXPCOM_FormatGivenException(cerrMsg, exc, typ, tb);
  nsAutoString errMsg;
  CopyUTF8toUTF16(cerrMsg, errMsg);
  NS_ASSERTION(!errMsg.IsEmpty(), "Failed to extract a Python error message");
  errorevent.errorMsg = errMsg.get();
  nsEventStatus status = nsEventStatus_eIgnore;

  // Handle the script error before we restore the exception.
  if (mScriptGlobal != nsnull && !outOfMem) {
    mScriptGlobal->HandleScriptError(&errorevent, &status);
  }

  PyErr_Restore(exc, typ, tb);

  if (status != nsEventStatus_eConsumeNoDefault) {
    // report it 'normally'.  We probably should use nsIScriptError
    // (OTOH, pyxpcom itself is probably what should)
    NS_WARNING("Python error calling DOM script");
    PyXPCOM_LogError("Python error");
  }
  // We always want the hresult and exception state cleared.
  nsresult ret = PyXPCOM_SetCOMErrorFromPyException();
  PyErr_Clear();
  return ret;
}

nsresult
nsPythonContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::InitContext");
  // Make sure callers of this use
  // WillInitializeContext/DidInitializeContext around this call?
  // We don't really care though.
  NS_ENSURE_TRUE(!mIsInitialized, NS_ERROR_ALREADY_INITIALIZED);
  // Load our delegate.
  CEnterLeavePython _celp;
  // this assertion blowing means we need to create the delegate elsewhere...
  NS_ASSERTION(mDelegate == NULL, "Init context called multiple times!");
  Py_XDECREF(mDelegate);
  PyObject *mod = PyImport_ImportModule("nsdom.context");
  if (mod==NULL)
    return HandlePythonError();
  PyObject *klass = PyObject_GetAttrString(mod, "ScriptContext");
  Py_DECREF(mod);
  if (klass == NULL)
    return HandlePythonError();
  mDelegate = PyObject_Call(klass, NULL, NULL);
  Py_DECREF(klass);

  if (mDelegate == NULL)
    return HandlePythonError();

  PyObject *obGlobal;
  if (aGlobalObject) {
    obGlobal = PyObject_FromNSDOMInterface(mDelegate, aGlobalObject,
                                           NS_GET_IID(nsIScriptGlobalObject));
    if (!obGlobal)
      return HandlePythonError();
  } else {
    obGlobal = Py_None;
    Py_INCREF(Py_None);
  }

  PyObject *ret = PyObject_CallMethod(mDelegate, "InitContext", "N", obGlobal);
  if (ret == NULL)
    return HandlePythonError();
  // stash the global away so we can fetch it locally.
  mScriptGlobal = aGlobalObject;
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
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::ExecuteScript");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (aIsUndefined) {
    *aIsUndefined = PR_TRUE;
  }
  if (!mScriptsEnabled) {
    if (aRetValue) {
      aRetValue->Truncate();
    }
    return NS_OK;
  }
  NS_ENSURE_TRUE(aScriptObject, NS_ERROR_NULL_POINTER);
  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);
  CEnterLeavePython _celp;

  PyObject *ret = PyObject_CallMethod(mDelegate, "ExecuteScript", "OO",
                                      aScriptObject, aScopeObject);
  // We always want to return OK here - any errors are "just" script errors.
  if (!ret) {
    HandlePythonError();
    if (aRetValue)
      aRetValue->Truncate();
  } else if (ret == Py_None) {
    if (aRetValue)
      aRetValue->Truncate();
  } else {
    if (aRetValue) {
      PyObject_AsNSString(ret, *aRetValue);
    }
    if (aIsUndefined) {
      *aIsUndefined = PR_FALSE;
    }
  }
  Py_XDECREF(ret);
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

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obCode = PyObject_FromNSString(aText, aTextLength);
  if (!obCode)
    return HandlePythonError();
  PyObject *obPrincipal = Py_None; // fix this when we can use it!

  PyObject *ret = PyObject_CallMethod(mDelegate, "CompileScript",
                                      "NOOsii",
                                      obCode,
                                      aScopeObject ? aScopeObject : Py_None,
                                      obPrincipal,
                                      aURL,
                                      aLineNo,
                                      aVersion);
  if (!ret) {
    *aScriptObject = nsnull;
    return HandlePythonError();
  }
  PYLEAK_STAT_INCREMENT(ScriptObject);
  *aScriptObject = ret;
  return NS_OK;
}

nsresult
nsPythonContext::CompileEventHandler(nsIPrincipal *aPrincipal, nsIAtom *aName,
                                 PRUint32 aArgCount,
                                 const char** aArgNames,
                                 const nsAString& aBody,
                                 const char *aURL, PRUint32 aLineNo,
                                 void** aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileEventHandler");

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *argNames = PyList_New(aArgCount);
  if (!argNames)
    return HandlePythonError();
  for (PRUint32 i=0;i<aArgCount;i++) {
    PyList_SET_ITEM(argNames, i, PyString_FromString(aArgNames[i]));
  }
  PyObject *obPrincipal = Py_None; // fix this when we can use it!

  PyObject *ret = PyObject_CallMethod(mDelegate, "CompileEventHandler",
                                      "OsNNsi",
                                      obPrincipal,
                                      AtomToEventHandlerName(aName),
                                      argNames,
                                      PyObject_FromNSString(aBody),
                                      aURL, aLineNo);
  if (!ret) {
    *aHandler = nsnull;
    return HandlePythonError();
  }
  PYLEAK_STAT_INCREMENT(EventHandler);
  *aHandler = ret;
  return NS_OK;
}

nsresult
nsPythonContext::BindCompiledEventHandler(nsISupports *aTarget, void *aScope,
                                          nsIAtom *aName, void *aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obTarget;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "BindCompiledEventHandler",
                                      "NOsO",
                                      obTarget, aScope,
                                      AtomToEventHandlerName(aName),
                                      aHandler);
  Py_XDECREF(ret);
  return HandlePythonError();
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
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileFunction");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ERROR("CompileFunction not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::CallEventHandler(nsISupports *aTarget, void *aScope, void* aHandler,
                                    nsIArray *aargv, nsISupports **arv)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CallEventHandler");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obTarget, *obArgv;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  obArgv = PyObject_FromNSDOMInterface(mDelegate, aargv);
  if (!obArgv) {
    Py_DECREF(obTarget);
    return HandlePythonError();
  }


  PyObject *ret = PyObject_CallMethod(mDelegate, "CallEventHandler",
                                      "NOON",
                                      obTarget, aScope, aHandler,
                                      obArgv);
  if (ret) {
    Py_nsISupports::InterfaceFromPyObject(ret, NS_GET_IID(nsIVariant),
                                               arv, PR_TRUE, PR_TRUE);
    Py_DECREF(ret);
  }
  return HandlePythonError();
}

nsresult
nsPythonContext::GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                      nsIAtom* aName,
                                      void** aHandler)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::GetBoundEventHandler");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obTarget;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "GetBoundEventHandler",
                                      "NOs",
                                      obTarget, aScope,
                                      AtomToEventHandlerName(aName));
  if (!ret) {
    *aHandler = nsnull;
    return HandlePythonError();
  }
  if (ret == Py_None) {
    *aHandler = nsnull;
    return NS_OK;
  }
  PYLEAK_STAT_INCREMENT(ScriptObject);
  *aHandler = ret;
  return NS_OK;
}

nsresult
nsPythonContext::SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::SetProperty");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;
  PyObject *obVal;
  obVal = PyObject_FromNSDOMInterface(mDelegate, aVal);
  if (!obVal)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "SetProperty",
                                      "OsN",
                                      aTarget, aPropName, obVal);
  Py_XDECREF(ret);
  return HandlePythonError();
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
  PyObject *pyScriptObject = (PyObject *)aScriptObject;
  if (!PyCode_Check(pyScriptObject) && !PyFunction_Check(pyScriptObject)) {
    NS_ERROR("aScriptObject is not a code or function object");
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

  if (magic != (PRUint32)PyImport_GetMagicNumber()) {
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
  NS_ASSERTION(PyCode_Check(codeObject) || PyFunction_Check(codeObject),
               "unmarshal returned non code/functions");
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
  return mScriptGlobal;
}

void *
nsPythonContext::GetNativeContext()
{
  return nsnull;
}

void *
nsPythonContext::GetNativeGlobal()
{
  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, nsnull);
  CEnterLeavePython _celp;
  PyObject *ret = PyObject_CallMethod(mDelegate, "GetNativeGlobal", NULL);
  if (!ret) {
    HandlePythonError();
    return nsnull;
  }
  // This is assumed a borrowed reference.
  NS_ASSERTION(ret->ob_refcnt > 1, "Can't have a new object here!?");
  Py_DECREF(ret);
  return ret;
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
nsPythonContext::FinalizeContext()
{
  ;
}

void
nsPythonContext::GC()
{
  ;
}

void
nsPythonContext::FinalizeClasses(void *aGlobalObj, PRBool aWhatever)
{
  CEnterLeavePython _celp;
  if (mDelegate) {
    PyObject *ret = PyObject_CallMethod(mDelegate, "FinalizeClasses",
                                        "O",
                                        aGlobalObj);
    Py_XDECREF(ret);
  }
  mScriptGlobal = nsnull;
  HandlePythonError();
}

NS_IMETHODIMP
nsPythonContext::Notify(nsITimer *timer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
