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
#include "nsDataHashtable.h"
 
#include "PyXPCOM.h"
class nsIScriptObjectOwner;
class nsIArray;

#ifdef NS_DEBUG
class nsPyDOMObjectLeakStats
  {
    public:
      nsPyDOMObjectLeakStats()
        : mEventHandlerCount(0), mScriptObjectCount(0), mBindingCount(0),
          mScriptContextCount(0) {}

      ~nsPyDOMObjectLeakStats()
        {
          printf("pydom leaked objects:\n");
          PRBool leaked = PR_FALSE;
#define CHECKLEAK(attr) \
          if (attr) { \
            printf(" => %-20s % 6d\n", #attr ":", attr); \
            leaked = PR_TRUE; \
          }
      
          CHECKLEAK(mScriptObjectCount);
          CHECKLEAK(mBindingCount);
          CHECKLEAK(mScriptContextCount);
          if (_PyXPCOM_GetInterfaceCount()) {
            printf(" => %-20s % 6d\n",
                   "pyxpcom interfaces:", _PyXPCOM_GetInterfaceCount());
            leaked = PR_TRUE;
          }
          if (_PyXPCOM_GetGatewayCount()) {
            printf(" => %-20s % 6d\n",
                   "pyxpcom gateways:", _PyXPCOM_GetGatewayCount());
            leaked = PR_TRUE;
          }
          if (!leaked)
            printf("No leaks.objects");
        }

      PRInt32 mEventHandlerCount;
      PRInt32 mScriptObjectCount;
      PRInt32 mBindingCount;
      PRInt32 mScriptContextCount;
  };
extern nsPyDOMObjectLeakStats gLeakStats;
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
  virtual nsresult CompileEventHandler(nsIPrincipal *aPrincipal,
                                       nsIAtom *aName,
                                       const char *aEventName,
                                       const nsAString& aBody,
                                       const char *aURL,
                                       PRUint32 aLineNo,
                                       void** aHandler);
  virtual nsresult CallEventHandler(nsISupports* aTarget, void *aScope,
                                    void* aHandler,
                                    nsIArray *argv, nsISupports **rv);
  virtual nsresult BindCompiledEventHandler(nsISupports*aTarget, void *aScope,
                                            nsIAtom *aName,
                                            void *aHandler);
  virtual nsresult GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                        nsIAtom* aName,
                                        void** aHandler);
  virtual nsresult CompileFunction(void* aTarget,
                                   const nsACString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRBool aShared,
                                   void** aFunctionObject);

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

  virtual nsresult Serialize(nsIObjectOutputStream* aStream, void *aScriptObject);
  virtual nsresult Deserialize(nsIObjectInputStream* aStream, void **aResult);

  NS_DECL_NSITIMERCALLBACK
protected:

  // covert utf-16 source code into utf8 with normalized line endings.
  nsCAutoString FixSource(const nsAString &aSource);
  PyObject *InternalCompile(const nsAString &source, const char *url,
                            PRUint32 lineNo);

  PRPackedBool mIsInitialized;
  PRPackedBool mScriptsEnabled;
  PRPackedBool mProcessingScriptTag;
  PRUint32 mNumEvaluations;
  PyObject *mGlobal;

  nsIScriptContextOwner* mOwner;  /* NB: weak reference, not ADDREF'd */

  static nsresult HandlePythonError();
  
  // Implement our concept of a 'wrapped native'.  No concept of 'expandos'
  // yet - we explicitly know when we need to create a new permanent one.

  PyObject *GetPyNamespaceFor(nsISupports *pThing, PRBool bCreate);
  void CleanPyNamespaces();
  
  // and the data.
  nsDataHashtable<nsISupportsHashKey, PyObject *> mMapPyObjects;
  // And other to keep an explicit strong ref to the target for as long as our
  // context is alive.
//  nsClassHashtable<nsISupportsHashKey, nsISupports> mMapNatives; 
};
