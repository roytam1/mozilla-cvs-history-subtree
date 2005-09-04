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
#include "nsITimer.h"
#include "PyXPCOM.h"
class nsIScriptObjectOwner;
class nsIArray;

class nsPythonContext : public nsIScriptContext,
                        public nsITimerCallback
{
public:
  nsPythonContext();
  virtual ~nsPythonContext();

  NS_DECL_ISUPPORTS

  virtual PRUint32 GetLanguage() { return nsIProgrammingLanguage::PYTHON; }

  virtual nsresult EvaluateString(const nsAString& aScript,
                                  void *aScopeObject,
                                  nsIPrincipal *principal,
                                  const char *aURL,
                                  PRUint32 aLineNo,
                                  PRUint32 aVersion,
                                  nsAString *aRetValue,
                                  PRBool* aIsUndefined);
  virtual nsresult EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined);

  virtual nsresult CompileScript(const PRUnichar* aText,
                                 PRInt32 aTextLength,
                                 void *aScopeObject,
                                 nsIPrincipal *principal,
                                 const char *aURL,
                                 PRUint32 aLineNo,
                                 PRUint32 aVersion,
                                 void** aScriptObject);
  virtual nsresult ExecuteScript(void* aScriptObject,
                                 void *aScopeObject,
                                 nsAString* aRetValue,
                                 PRBool* aIsUndefined);
  virtual nsresult CompileEventHandler(nsIScriptBinding *aTarget,
                                       nsIAtom *aName,
                                       const char *aEventName,
                                       const nsAString& aBody,
                                       const char *aURL,
                                       PRUint32 aLineNo,
                                       PRBool aShared,
                                       void** aHandler);
  virtual nsresult CallEventHandler(nsIScriptBinding* aTarget, void* aHandler,
                                    nsIArray *argv, nsISupports **rv);
  virtual nsresult BindCompiledEventHandler(nsIScriptBinding *aTarget,
                                            nsIAtom *aName,
                                            void *aHandler);
  virtual nsresult CompileFunction(void* aTarget,
                                   const nsACString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRBool aShared,
                                   void** aFunctionObject);

  virtual nsresult GetScriptBinding(nsISupports *aObject, void *aScope,
                                    nsIScriptBinding **aBinding);

  virtual nsresult GetScriptBindingHandler(nsIScriptBinding *aBinding,
                                           nsString &name,
                                           void **handler);

  virtual nsresult AddGCRoot(void *object, const char *desc);
  virtual nsresult RemoveGCRoot(void *object);

  virtual void SetDefaultLanguageVersion(PRUint32 aVersion);
  virtual nsIScriptGlobalObject *GetGlobalObject();
  virtual void *GetNativeContext();
  virtual void *GetNativeGlobal();
  virtual nsresult InitContext(nsIScriptGlobalObject *aGlobalObject);
  virtual PRBool IsContextInitialized();
  virtual void GC();

  virtual void ScriptEvaluated(PRBool aTerminated);
  virtual void SetOwner(nsIScriptContextOwner* owner);
  virtual nsIScriptContextOwner *GetOwner();
  virtual nsresult SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                          nsISupports* aRef);
  virtual PRBool GetScriptsEnabled();
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);

  virtual nsresult SetProperty(void *aTarget, const char *aPropName,
                               nsISupports *aVal);

  virtual PRBool GetProcessingScriptTag();
  virtual void SetProcessingScriptTag(PRBool aResult);

  virtual void SetGCOnDestruction(PRBool aGCOnDestruction) {;}

  virtual nsresult InitClasses(void *aGlobalObj);
  virtual nsresult FinalizeClasses(void* aGlobalObj);

  virtual void WillInitializeContext();
  virtual void DidInitializeContext();

  NS_DECL_NSITIMERCALLBACK
protected:
  PRPackedBool mIsInitialized;
  PRPackedBool mScriptsEnabled;
  PRPackedBool mProcessingScriptTag;
  PRUint32 mNumEvaluations;
  PyObject *mGlobal;

  nsIScriptContextOwner* mOwner;  /* NB: weak reference, not ADDREF'd */

  static nsresult HandlePythonError();
};


class nsPyScriptBinding : public nsIScriptBinding {
public:
  nsPyScriptBinding(nsISupports *aObject);
  virtual ~nsPyScriptBinding();

  NS_DECL_ISUPPORTS


  virtual PRUint32 GetLanguage() {return nsIProgrammingLanguage::PYTHON;}
  void *GetNativeObject();
  nsISupports *GetTarget();

  nsCOMPtr<nsISupports> mHolder;
  PyObject *mCodeObject;
};
