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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

////////////////////////////////////////////////////////////////////////////////
// XXX What a mess we have here w.r.t. our prompting interfaces. As far as I
// can tell, the situation looks something like this:
//
//      - clients get the nsIPrompt from the web shell window
//      - the web shell window passes control to nsCommonDialogs
//      - nsCommonDialogs calls into js with the current dom window
//      - the dom window gets the nsIPrompt of its tree owner
//      - somewhere along the way a real dialog comes up
// 
// This little transducer maps the nsIPrompt interface to the nsICommonDialogs
// interface. Ideally, nsIPrompt would be implemented by nsIDOMWindowInternal which 
// would eliminate the need for this.

#include "nsDOMWindowPrompter.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"

#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsICommonDialogs.h"
#include "nsIStringBundle.h"

static NS_DEFINE_CID (kCommonDialogsCID, NS_CommonDialog_CID );
static NS_DEFINE_CID (kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define kCommonDialogsProperties "chrome://global/locale/commonDialogs.properties"

//*****************************************************************************
// nsDOMWindowPrompter
//*****************************************************************************   

class nsDOMWindowPrompter : public nsIPrompt
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROMPT

  nsDOMWindowPrompter(nsIDOMWindowInternal* window);
  virtual ~nsDOMWindowPrompter() {}

  nsresult Init();

protected:
  nsCOMPtr<nsIDOMWindowInternal>        mDOMWindow;
  nsCOMPtr<nsICommonDialogs>    mCommonDialogs;

  nsresult GetLocaleString(const PRUnichar*, PRUnichar**);
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWindowPrompter, nsIPrompt)

nsDOMWindowPrompter::nsDOMWindowPrompter(nsIDOMWindowInternal* window)
  : mDOMWindow(window)
{
  NS_INIT_REFCNT();
}

nsresult
nsDOMWindowPrompter::Init()
{
  nsresult rv;
  mCommonDialogs = do_GetService(kCommonDialogsCID, &rv);
  return rv;
}

nsresult
nsDOMWindowPrompter::GetLocaleString(const PRUnichar* aString, PRUnichar** aResult)
{
  nsresult rv;

  nsCOMPtr<nsIStringBundleService> stringService = do_GetService(kStringBundleServiceCID);
  nsCOMPtr<nsIStringBundle> stringBundle;
  nsILocale *locale = nsnull;
 
  rv = stringService->CreateBundle(kCommonDialogsProperties, locale, getter_AddRefs(stringBundle));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  rv = stringBundle->GetStringFromName(aString, aResult);

  return rv;
}

//*****************************************************************************
// nsDOMWindowPrompter::nsIPrompt
//*****************************************************************************   

NS_IMETHODIMP
nsDOMWindowPrompter::Alert(const PRUnichar* dialogTitle, 
                           const PRUnichar* text)
{
  nsresult rv;
 
  if (dialogTitle == nsnull) {
    PRUnichar *title;
    rv = GetLocaleString(NS_ConvertASCIItoUCS2("Alert").get(), &title);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    rv = mCommonDialogs->Alert(mDOMWindow, title, text);
    nsCRT::free(title);
    title = nsnull;
  }
  else {
    rv = mCommonDialogs->Alert(mDOMWindow, dialogTitle, text);
  }
  
  return rv; 
}

NS_IMETHODIMP
nsDOMWindowPrompter::AlertCheck(const PRUnichar* dialogTitle, 
                                  const PRUnichar* text,
                                  const PRUnichar* checkMsg,
                                  PRBool *checkValue)
{
  nsresult rv;

  if (dialogTitle == nsnull) {
    PRUnichar *title;
    rv = GetLocaleString(NS_LITERAL_STRING("Alert").get(), &title);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    rv = mCommonDialogs->AlertCheck(mDOMWindow, title, text, checkMsg, checkValue);
    nsCRT::free(title);
    title = nsnull;
  }
  else {
    rv = mCommonDialogs->AlertCheck(mDOMWindow, dialogTitle, text, checkMsg, checkValue);
  }

  return rv;  
}

NS_IMETHODIMP
nsDOMWindowPrompter::Confirm(const PRUnichar* dialogTitle, 
                             const PRUnichar* text,
                             PRBool *_retval)
{
  nsresult rv;
  if (dialogTitle == nsnull) {
    PRUnichar *title;
    rv = GetLocaleString(NS_ConvertASCIItoUCS2("Confirm").get(), &title);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    rv = mCommonDialogs->Confirm(mDOMWindow, title, text, _retval);
    nsCRT::free(title);
    title = nsnull;
  }  
  else {
    rv = mCommonDialogs->Confirm(mDOMWindow, dialogTitle, text, _retval);
  }

  return rv; 
}

NS_IMETHODIMP
nsDOMWindowPrompter::ConfirmCheck(const PRUnichar* dialogTitle, 
                                  const PRUnichar* text,
                                  const PRUnichar* checkMsg,
                                  PRBool *checkValue,
                                  PRBool *_retval)
{
  nsresult rv;

  if (dialogTitle == nsnull) {
    PRUnichar *title;
    rv = GetLocaleString(NS_ConvertASCIItoUCS2("ConfirmCheck").get(), &title);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    rv = mCommonDialogs->ConfirmCheck(mDOMWindow, title, text, checkMsg, checkValue, _retval);
    nsCRT::free(title);
    title = nsnull;
  }
  else {
    rv = mCommonDialogs->ConfirmCheck(mDOMWindow, dialogTitle, text, checkMsg, checkValue, _retval);
  }

  return rv;  
}

NS_IMETHODIMP
nsDOMWindowPrompter::Prompt(const PRUnichar *dialogTitle,
                            const PRUnichar *text,
                            PRUnichar **result,
                            const PRUnichar *checkMsg,
                            PRBool *checkValue,
                            PRBool *_retval)
{
  nsresult rv;
  PRUnichar *localTitle = nsnull;
  
  if (dialogTitle == nsnull) {
    rv = GetLocaleString(NS_LITERAL_STRING("Prompt").get(), &localTitle);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  }
  
  PRInt32 buttonPressed = 1;
  rv = mCommonDialogs->UniversalDialog(mDOMWindow,
                                       nsnull,
                                       dialogTitle ? dialogTitle : localTitle, /* e.g., alert, confirm, prompt, prompt password */
                                       text, /* main message for dialog */
                                       checkMsg, /* message for checkbox */
                                       nsnull, /* text for first button */
                                       nsnull, /* text for second button */
                                       nsnull, /* text for third button */
                                       nsnull, /* text for fourth button */
                                       nsnull, /*message for first edit field */
                                       nsnull, /* message for second edit field */
                                       result, /* initial and final value for first edit field */
                                       nsnull, /* initial and final value for second edit field */
                                       nsnull, /* url of icon to be displayed in dialog */
                                       checkValue, /* initial and final state of check box */
                                       2, /* total number of buttons (0 to 4) */
                                       1, /* total number of edit fields (0 to 2) */
                                       0, /* is first edit field a password field */
                                       &buttonPressed);
  if (localTitle)
    nsCRT::free(localTitle);
  if (NS_FAILED(rv)) return rv;
  *_retval = (buttonPressed == 0);
  return rv; 
}

NS_IMETHODIMP
nsDOMWindowPrompter::PromptUsernameAndPassword(const PRUnichar *dialogTitle,
                                               const PRUnichar *text,
                                               PRUnichar **username,
                                               PRUnichar **password,
                                               const PRUnichar *checkMsg,
                                               PRBool *checkValue,
                                               PRBool *_retval)
{	
  nsresult rv;
  PRUnichar *localTitle = nsnull;
  PRUnichar *usernameLabel, *passwordLabel;

  rv = GetLocaleString(NS_LITERAL_STRING("Username").get(), &usernameLabel);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  rv = GetLocaleString(NS_LITERAL_STRING("Password").get(), &passwordLabel);
  if (NS_FAILED(rv)) {
    nsCRT::free(usernameLabel);
    return NS_ERROR_FAILURE;
  }
  
  if (dialogTitle == nsnull) {
    rv = GetLocaleString(NS_LITERAL_STRING("PromptUsernameAndPassword").get(), &localTitle);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  }
  
  PRInt32 buttonPressed = 1;
  rv = mCommonDialogs->UniversalDialog(mDOMWindow,
                                       nsnull,
                                       dialogTitle ? dialogTitle : localTitle, /* e.g., alert, confirm, prompt, prompt password */
                                       text, /* main message for dialog */
                                       checkMsg, /* message for checkbox */
                                       nsnull, /* text for first button */
                                       nsnull, /* text for second button */
                                       nsnull, /* text for third button */
                                       nsnull, /* text for fourth button */
                                       usernameLabel, /*message for first edit field */
                                       passwordLabel, /* message for second edit field */
                                       username, /* initial and final value for first edit field */
                                       password, /* initial and final value for second edit field */
                                       nsnull, /* url of icon to be displayed in dialog */
                                       checkValue, /* initial and final state of check box */
                                       2, /* total number of buttons (0 to 4) */
                                       2, /* total number of edit fields (0 to 2) */
                                       0, /* is first edit field a password field */
                                       &buttonPressed);
  nsCRT::free(usernameLabel);
  nsCRT::free(passwordLabel);
  if (localTitle)
    nsCRT::free(localTitle);
  if (NS_FAILED(rv)) return rv;
  *_retval = (buttonPressed == 0);
  return rv; 
}

NS_IMETHODIMP
nsDOMWindowPrompter::PromptPassword(const PRUnichar *dialogTitle,
                                    const PRUnichar *text,
                                    PRUnichar **password,
                                    const PRUnichar *checkMsg,
                                    PRBool *checkValue,
                                    PRBool *_retval)
{
  nsresult rv;
  PRUnichar *localTitle = nsnull;
  
  if (dialogTitle == nsnull) {
    rv = GetLocaleString(NS_LITERAL_STRING("PromptPassword").get(), &localTitle);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  }
  
  PRInt32 buttonPressed = 1;
  rv = mCommonDialogs->UniversalDialog(mDOMWindow,
                                       nsnull,
                                       dialogTitle ? dialogTitle : localTitle, /* e.g., alert, confirm, prompt, prompt password */
                                       text, /* main message for dialog */
                                       checkMsg, /* message for checkbox */
                                       nsnull, /* text for first button */
                                       nsnull, /* text for second button */
                                       nsnull, /* text for third button */
                                       nsnull, /* text for fourth button */
                                       nsnull, /*message for first edit field */
                                       nsnull, /* message for second edit field */
                                       password, /* initial and final value for first edit field */
                                       nsnull, /* initial and final value for second edit field */
                                       nsnull, /* url of icon to be displayed in dialog */
                                       checkValue, /* initial and final state of check box */
                                       2, /* total number of buttons (0 to 4) */
                                       1, /* total number of edit fields (0 to 2) */
                                       1, /* is first edit field a password field */
                                       &buttonPressed);
  if (localTitle)
    nsCRT::free(localTitle);
  if (NS_FAILED(rv)) return rv;
  *_retval = (buttonPressed == 0);
  return rv; 
}

NS_IMETHODIMP
nsDOMWindowPrompter::Select(const PRUnichar *dialogTitle,
                            const PRUnichar* inMsg,
                            PRUint32 inCount, 
                            const PRUnichar **inList,
                            PRInt32 *outSelection,
                            PRBool *_retval)
{
  nsresult rv; 

  if (dialogTitle == nsnull) {
    PRUnichar *title;
    rv = GetLocaleString(NS_ConvertASCIItoUCS2("Select").get(), &title);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    rv = mCommonDialogs->Select(mDOMWindow, title, inMsg, 
                                inCount, inList, outSelection, _retval);
    nsCRT::free(title);
    title = nsnull;
  }
  else {
    rv = mCommonDialogs->Select(mDOMWindow, dialogTitle, inMsg,
                                inCount, inList, outSelection, _retval);
  }

  return rv;
}

NS_IMETHODIMP
nsDOMWindowPrompter::UniversalDialog(const PRUnichar *inTitleMessage,
                                     const PRUnichar *inDialogTitle, /* e.g., alert, confirm, prompt, prompt password */
                                     const PRUnichar *inMsg, /* main message for dialog */
                                     const PRUnichar *inCheckboxMsg, /* message for checkbox */
                                     const PRUnichar *inButton0Text, /* text for first button */
                                     const PRUnichar *inButton1Text, /* text for second button */
                                     const PRUnichar *inButton2Text, /* text for third button */
                                     const PRUnichar *inButton3Text, /* text for fourth button */
                                     const PRUnichar *inEditfield1Msg, /*message for first edit field */
                                     const PRUnichar *inEditfield2Msg, /* message for second edit field */
                                     PRUnichar **inoutEditfield1Value, /* initial and final value for first edit field */
                                     PRUnichar **inoutEditfield2Value, /* initial and final value for second edit field */
                                     const PRUnichar *inIConURL, /* url of icon to be displayed in dialog */
                                     /* examples are
                                        "chrome://global/skin/question-icon.gif" for question mark,
                                        "chrome://global/skin/alert-icon.gif" for exclamation mark
                                     */
                                     PRBool *inoutCheckboxState, /* initial and final state of check box */
                                     PRInt32 inNumberButtons, /* total number of buttons (0 to 4) */
                                     PRInt32 inNumberEditfields, /* total number of edit fields (0 to 2) */
                                     PRInt32 inEditField1Password, /* is first edit field a password field */
                                     PRInt32 *outButtonPressed) /* number of button that was pressed (0 to 3) */
{
  nsresult rv;
  NS_ASSERTION(inDialogTitle, "UniversalDialog must have a dialog title supplied");
  rv = mCommonDialogs->UniversalDialog(mDOMWindow, 
                                       inTitleMessage,
                                       inDialogTitle,
                                       inMsg,
                                       inCheckboxMsg,
                                       inButton0Text,
                                       inButton1Text,
                                       inButton2Text,
                                       inButton3Text,
                                       inEditfield1Msg,
                                       inEditfield2Msg,
                                       inoutEditfield1Value,
                                       inoutEditfield2Value,
                                       inIConURL,
                                       inoutCheckboxState,
                                       inNumberButtons,
                                       inNumberEditfields,
                                       inEditField1Password,
                                       outButtonPressed);
  return rv;
}

//*****************************************************************************
// NS_NewDOMWindowPrompter
//*****************************************************************************   

nsresult NS_NewDOMWindowPrompter(nsIPrompt* *result, nsIDOMWindowInternal* window)
{
  nsresult rv;
  nsDOMWindowPrompter* prompter = new nsDOMWindowPrompter(window);
  if (prompter == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(prompter);
  rv = prompter->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(prompter);
    return rv;
  }
  *result = prompter;
  return NS_OK;
}
