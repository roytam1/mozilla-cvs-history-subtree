/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=80: */
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
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsIScriptGlobalObject_h__
#define nsIScriptGlobalObject_h__

#include "nsISupports.h"
#include "nsEvent.h"

class nsIScriptContext;
class nsIDOMDocument;
class nsIDOMEvent;
class nsPresContext;
class nsIDocShell;
class nsIDOMWindowInternal;
class nsIScriptGlobalObjectOwner;
class nsIArray;
class nsScriptErrorEvent;
class nsIScriptGlobalObject;
enum nsEventStatus;
struct JSObject; // kill me!

// Some helpers for working with integer "language IDs", and specifically for
// working with arrays of such objects.  For example, it is common for
// implementations supporting multiple script languages to keep each
// language's nsIScriptContext in an array indexed by the language ID.

// Implementation note: We always ignore nsIProgrammingLanguage::UNKNOWN and
// nsIProgrammingLanguage::CPLUSPLUS - this gives javascript slot 0.  We also
// have no need to go all the way to nsIProgrammingLanguage::MAX; restricting
// the upper bound to reduce the number of bits we need may end up useful.

#define NS_SL_FIRST nsIProgrammingLanguage::JAVASCRIPT
// last is kinda arbitrary.
#define NS_SL_LAST (nsIProgrammingLanguage::JAVASCRIPT+3)

// Use to declare the array size
#define NS_SL_ARRAY_UBOUND (NS_SL_LAST-NS_SL_FIRST+1)

// Is a language ID valid?
#define NS_SL_VALID(langID) (langID >= NS_SL_FIRST && langID <= NS_SL_LAST)

// Return an index for a given ID.
#define NS_SL_INDEX(langID) (langID-NS_SL_FIRST)

// Create a 'for' loop iterating over all possible language IDs (*not* indexes)
#define NS_SL_FOR_ID(varName) \
          for (PRUint32 varName=NS_SL_FIRST;varName<=NS_SL_LAST;varName++)

// Create a 'for' loop iterating over all indexes (when you don't need to know
// what language it is)
#define NS_SL_FOR_INDEX(varName) \
          for (PRUint32 varName=0;varName<=NS_SL_INDEX(NS_SL_LAST);varName++)

// A helper function for nsIScriptGlobalObject implementations to use
// when handling a script error.  Generally called by the global when a context
// notifies it of an error via nsIScriptGlobalObject::HandleScriptError.
// Returns PR_TRUE if HandleDOMEvent was actually called, in which case
// aStatus will be filled in with the status.
PRBool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     nsScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus);


#define NS_ISCRIPTGLOBALOBJECT_IID \
{ /* {6E7EF978-47D0-47c9-9649-CDCDB1E4CCEC} */ \
  0x6e7ef978, 0x47d0, 0x47c9, \
  { 0x96, 0x49, 0xcd, 0xcd, 0xb1, 0xe4, 0xcc, 0xec } }

/**
 * The global object which keeps a script context for each supported script
 * language. This often used to store per-window global state.
 */

class nsIScriptGlobalObject : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISCRIPTGLOBALOBJECT_IID)

  virtual nsresult SetNewDocument(nsIDOMDocument *aDocument,
                                  nsISupports *aState,
                                  PRBool aRemoveEventListeners,
                                  PRBool aClearScope) = 0;
  virtual void SetDocShell(nsIDocShell *aDocShell) = 0;
  virtual nsIDocShell *GetDocShell() = 0;
  virtual void SetOpenerWindow(nsIDOMWindowInternal *aOpener)=0;

  /**
   * Let the script global object know who its owner is.
   * The script global object should not addref the owner. It
   * will be told when the owner goes away.
   * @return NS_OK if the method is successful
   */
  virtual void SetGlobalObjectOwner(nsIScriptGlobalObjectOwner* aOwner) = 0;

  /**
   * Get the owner of the script global object. The method
   * addrefs the returned reference according to regular
   * XPCOM rules, even though the internal reference itself
   * is a "weak" reference.
   */
  virtual nsIScriptGlobalObjectOwner *GetGlobalObjectOwner() = 0;

  virtual nsresult HandleDOMEvent(nsPresContext* aPresContext, 
                                  nsEvent* aEvent, 
                                  nsIDOMEvent** aDOMEvent,
                                  PRUint32 aFlags,
                                  nsEventStatus* aEventStatus)=0;

  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID) = 0;
  /**
   * Get a script context (WITHOUT added reference) for the specified language.
   */
  virtual nsIScriptContext *GetLanguageContext(PRUint32 lang) = 0;
  
  /**
   * Get the opaque "global" object for the specified lang.
   */
  virtual void *GetLanguageGlobal(PRUint32 lang) = 0;

  // Set/GetContext deprecated methods - use GetLanguageContext/Global
  virtual JSObject *GetGlobalJSObject() {
        return (JSObject *)GetLanguageGlobal(nsIProgrammingLanguage::JAVASCRIPT);
  }

  virtual nsIScriptContext *GetContext() {
        return GetLanguageContext(nsIProgrammingLanguage::JAVASCRIPT);
  }

  /**
   * Set a new language context for this global.  The native global for the
   * context is created by the context's GetNativeGlobal() method.
   */

  virtual nsresult SetLanguageContext(PRUint32 lang, nsIScriptContext *aContext) = 0;

  /**
   * Called when the global script is finalized.  Finalizes all contexts and globals
   * for all languages.
   * XXXmarkh - that is not the original intent - OnFinalize generally called
   * as part of js class destruction.
   */

  virtual void OnFinalize() = 0;

  /**
   * Called to enable/disable scripts.
   */
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts) = 0;

  /** Set a new arguments object for this window. This will be set on
   * the window right away (if there's an existing document) and it
   * will also be installed on the window when the next document is
   * loaded.  Each language impl is responsible for converting to
   * an array of args as appropriate for that language.
    */
  virtual nsresult SetNewArguments(nsIArray *aArguments) = 0;

  /** Handle a script error.  Generally called by a script context.
   */
  virtual nsresult HandleScriptError(nsScriptErrorEvent *aErrorEvent,
                                     nsEventStatus *aEventStatus) {
    return NS_HandleScriptError(this, aErrorEvent, aEventStatus);
  }
};

#endif
