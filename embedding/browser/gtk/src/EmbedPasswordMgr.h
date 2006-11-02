/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Password Manager.
 *
 * The Initial Developer of the Original Code is
 * Brian Ryner.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
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
#include "nsCPasswordManager.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIFormSubmitObserver.h"
#include "nsIWebProgressListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIStringBundle.h"
#include "nsIPrefBranch.h"
#include "nsIAuthPromptWrapper.h"
#include "nsCOMPtr.h"
#include "nsIPrompt.h"
#include "EmbedPrivate.h"
#define EMBED_PASSWORDMANAGER_DESCRIPTION "MicroB PSM Dialog Impl"
/* 360565c4-2ef3-4f6a-bab9-94cca891b2a7 */
#define EMBED_PASSWORDMANAGER_CID \
{0x360565c4, 0x2ef3, 0x4f6a, {0xba, 0xb9, 0x94, 0xcc, 0xa8, 0x91, 0xb2, 0xa7}}
class nsIFile;
class nsIStringBundle;
class nsIComponentManager;
class nsIContent;
class nsIDOMWindowInternal;
class nsIURI;
class nsIDOMHTMLInputElement;
struct nsModuleComponentInfo;
class EmbedPasswordMgr : public nsIPasswordManager,
                          public nsIPasswordManagerInternal,
                          public nsIObserver,
                          public nsIFormSubmitObserver,
                          public nsIWebProgressListener,
                          public nsIDOMFocusListener,
                          public nsIDOMLoadListener,
        public nsSupportsWeakReference
{
public:
  class SignonDataEntry;
  class SignonHashEntry;
  class PasswordEntry;
  EmbedPasswordMgr();
  virtual ~EmbedPasswordMgr();
  static EmbedPasswordMgr* GetInstance();
  static EmbedPasswordMgr* GetInstance(EmbedPrivate *aOwner);
  nsresult Init();
  static PRBool SingleSignonEnabled();
  static NS_METHOD Register(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* aRegistryLocation,
                            const char* aComponentType,
                            const nsModuleComponentInfo* aInfo);
  static NS_METHOD Unregister(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* aRegistryLocation,
                              const nsModuleComponentInfo* aInfo);
  static void Shutdown();
  static void GetLocalizedString(const nsAString& key,
                                 nsAString& aResult,
                                 PRBool aFormatted = PR_FALSE,
                                 const PRUnichar** aFormatArgs = nsnull,
                                 PRUint32 aFormatArgsLength = 0);
  static nsresult DecryptData(const nsAString& aData, nsAString& aPlaintext);
  static nsresult EncryptData(const nsAString& aPlaintext,
                              nsACString& aEncrypted);
  static nsresult EncryptDataUCS2(const nsAString& aPlaintext,
                                  nsAString& aEncrypted);
  nsresult InsertLogin(const char* username, const char* password = nsnull);
  nsresult RemovePasswords(const char *aHostName, const char *aUserName);
  nsresult RemovePasswordsByIndex(PRInt32 aIndex);
  nsresult GetNumberOfSavedPassword(PRInt32 *aNum);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPASSWORDMANAGER
  NS_DECL_NSIPASSWORDMANAGERINTERNAL
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER
  // nsIFormSubmitObserver
  NS_IMETHOD Notify(nsIContent* aFormNode,
                    nsIDOMWindowInternal* aWindow,
                    nsIURI* aActionURL,
                    PRBool* aCancelSubmit);
  // nsIDOMFocusListener
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  // nsIDOMEventListener
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  // nsIDOMLoadListener
  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD Unload(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent);
  NS_IMETHOD Abort(nsIDOMEvent* aEvent);
  NS_IMETHOD Error(nsIDOMEvent* aEvent);
protected:
  void WritePasswords(nsIFile* aPasswordFile);
  void AddSignonData(const nsACString& aRealm, SignonDataEntry* aEntry);
  nsresult FindPasswordEntryInternal(const SignonDataEntry* aEntry,
                                     const nsAString&  aUser,
                                     const nsAString&  aPassword,
                                     const nsAString&  aUserField,
                                     SignonDataEntry** aResult);
  nsresult FillPassword(nsIDOMEvent* aEvent = nsnull);
  void AttachToInput(nsIDOMHTMLInputElement* aElement);
  PRBool GetPasswordRealm(nsIURI* aURI, nsACString& aRealm);
  static PLDHashOperator PR_CALLBACK FindEntryEnumerator(const nsACString& aKey,
                                                         SignonHashEntry* aEntry,
                                                         void* aUserData);
  static PLDHashOperator PR_CALLBACK WriteRejectEntryEnumerator(const nsACString& aKey,
                                                                PRInt32 aEntry,
                                                                void* aUserData);
  static PLDHashOperator PR_CALLBACK WriteSignonEntryEnumerator(const nsACString& aKey,
                                                                SignonHashEntry* aEntry,
                                                                void* aUserData);
  static PLDHashOperator PR_CALLBACK BuildArrayEnumerator(const nsACString& aKey,
                                                          SignonHashEntry* aEntry,
                                                          void* aUserData);
  static PLDHashOperator PR_CALLBACK BuildRejectArrayEnumerator(const nsACString& aKey,
                                                                PRInt32 aEntry,
                                                                void* aUserData);
  static PLDHashOperator PR_CALLBACK RemoveForDOMDocumentEnumerator(nsISupports* aKey,
                                                                    PRInt32& aEntry,
                                                                    void* aUserData);
  static void EnsureDecoderRing();
  nsClassHashtable<nsCStringHashKey,SignonHashEntry> mSignonTable;
  nsDataHashtable<nsCStringHashKey,PRInt32> mRejectTable;
  nsDataHashtable<nsISupportsHashKey,PRInt32> mAutoCompleteInputs;
  nsCOMPtr<nsIFile> mSignonFile;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsIDOMHTMLInputElement* mAutoCompletingField;
  nsIDOMHTMLInputElement* mGlobalUserField;
  nsIDOMHTMLInputElement* mGlobalPassField;
  SignonHashEntry * mLastSignonHashEntry;
  int lastIndex;
  nsCAutoString mLastHostQuery;
  EmbedCommon* mCommonObject;
//  nsAString mLastHostQuery;
};
/* 1baf3398-f759-4a72-a21f-0abdc9cc9960 */
#define NS_SINGLE_SIGNON_PROMPT_CID \
{0x1baf3398, 0xf759, 0x4a72, {0xa2, 0x1f, 0x0a, 0xbd, 0xc9, 0xcc, 0x99, 0x60}}
// Our wrapper for username/password prompts - this allows us to prefill
// the password dialog and add a "remember this password" checkbox.
class EmbedSignonPrompt : public nsIAuthPromptWrapper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHPROMPT
  NS_DECL_NSIAUTHPROMPTWRAPPER
  EmbedSignonPrompt() { }
  virtual ~EmbedSignonPrompt() { }
protected:
  void GetLocalizedString(const nsAString& aKey, nsAString& aResult);
  nsCOMPtr<nsIPrompt> mPrompt;
};
/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIPassword.idl
 */
#ifndef __gen_nsIPassword_h__
#define __gen_nsIPassword_h__
#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif
/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
/* starting interface:    nsIPassword */
#define NS_IPASSWORD_IID_STR "cf39c2b0-1e4b-11d5-a549-0010a401eb10"
#define NS_IPASSWORD_IID \
  {0xcf39c2b0, 0x1e4b, 0x11d5, \
    { 0xa5, 0x49, 0x00, 0x10, 0xa4, 0x01, 0xeb, 0x10 }}
/** 
 * An optional interface for clients wishing to access a
 * password object
 * 
 * @status FROZEN
 */
class NS_NO_VTABLE nsIPassword : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPASSWORD_IID)
  /**
     * the name of the host corresponding to the login being saved
     *
     * The form of the host depends on how the nsIPassword object was created
     *
     * - if it was created as a result of submitting a form to a site, then the
     *   host is the url of the site, as obtained from a call to GetSpec
     *
     * - if it was created as a result of another app (e.g., mailnews) calling a
     *   prompt routine such at PromptUsernameAndPassword, then the host is whatever
     *   arbitrary string the app decided to pass in.
     *
     * Whatever form it is in, it will be used by the password manager to uniquely
     * identify the login realm, so that "newsserver:119" is not the same thing as
     * "newsserver".
     */
  /* readonly attribute AUTF8String host; */
  NS_IMETHOD GetHost(nsACString & aHost) = 0;
  /**
     * the user name portion of the login
     */
  /* readonly attribute AString user; */
  NS_IMETHOD GetUser(nsAString & aUser) = 0;
  /**
     * the password portion of the login
     */
  /* readonly attribute AString password; */
  NS_IMETHOD GetPassword(nsAString & aPassword) = 0;
};
  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPassword, NS_IPASSWORD_IID)
/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPASSWORD \
  NS_IMETHOD GetHost(nsACString & aHost); \
  NS_IMETHOD GetUser(nsAString & aUser); \
  NS_IMETHOD GetPassword(nsAString & aPassword); 
/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPASSWORD(_to) \
  NS_IMETHOD GetHost(nsACString & aHost) { return _to GetHost(aHost); } \
  NS_IMETHOD GetUser(nsAString & aUser) { return _to GetUser(aUser); } \
  NS_IMETHOD GetPassword(nsAString & aPassword) { return _to GetPassword(aPassword); } 
/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPASSWORD(_to) \
  NS_IMETHOD GetHost(nsACString & aHost) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetHost(aHost); } \
  NS_IMETHOD GetUser(nsAString & aUser) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUser(aUser); } \
  NS_IMETHOD GetPassword(nsAString & aPassword) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPassword(aPassword); } 
#endif /* __gen_nsIPassword_h__ */
/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIPasswordInternal.idl
 */
#ifndef __gen_nsIPasswordInternal_h__
#define __gen_nsIPasswordInternal_h__
#ifndef __gen_nsIPassword_h__
#include "nsIPassword.h"
#endif
/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
/* starting interface:    nsIPasswordInternal */
#define NS_IPASSWORDINTERNAL_IID_STR "2cc35c67-978f-42a9-a958-16e97ad2f4c8"
#define NS_IPASSWORDINTERNAL_IID \
  {0x2cc35c67, 0x978f, 0x42a9, \
    { 0xa9, 0x58, 0x16, 0xe9, 0x7a, 0xd2, 0xf4, 0xc8 }}
/**
 * This interface is supported by password manager entries to expose the
 * fieldnames passed to nsIPasswordManagerInternal's addUserFull method.
 */
class NS_NO_VTABLE nsIPasswordInternal : public nsIPassword {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPASSWORDINTERNAL_IID)
  /**
   * The name of the field that contained the username.
   */
  /* readonly attribute AString userFieldName; */
  NS_IMETHOD GetUserFieldName(nsAString & aUserFieldName) = 0;
  /**
   * The name of the field that contained the password.
   */
  /* readonly attribute AString passwordFieldName; */
  NS_IMETHOD GetPasswordFieldName(nsAString & aPasswordFieldName) = 0;
};
  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPasswordInternal, NS_IPASSWORDINTERNAL_IID)
/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPASSWORDINTERNAL \
  NS_IMETHOD GetUserFieldName(nsAString & aUserFieldName); \
  NS_IMETHOD GetPasswordFieldName(nsAString & aPasswordFieldName); 
/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPASSWORDINTERNAL(_to) \
  NS_IMETHOD GetUserFieldName(nsAString & aUserFieldName) { return _to GetUserFieldName(aUserFieldName); } \
  NS_IMETHOD GetPasswordFieldName(nsAString & aPasswordFieldName) { return _to GetPasswordFieldName(aPasswordFieldName); } 
/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPASSWORDINTERNAL(_to) \
  NS_IMETHOD GetUserFieldName(nsAString & aUserFieldName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUserFieldName(aUserFieldName); } \
  NS_IMETHOD GetPasswordFieldName(nsAString & aPasswordFieldName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPasswordFieldName(aPasswordFieldName); } 
#endif /* __gen_nsIPasswordInternal_h__ */
