/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *   Robert John Churchill    <rjc@netscape.com>
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
  The remote bookmarks service.
 */



#ifndef remotebookmarks___h___
#define remotebookmarks___h___

#include "nsIRemoteBookmarks.h"
#include "nsString.h"
#include "nsIRDFResource.h"
#include "nsIRDFLiteral.h"
#include "nsIRDFContainer.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFNode.h"
#include "nsISupportsArray.h"
#include "nsILDAPURL.h"
#include "nsILDAPConnection.h"
#include "nsILDAPOperation.h"
#include "nsILDAPMessageListener.h"
#include "nsIWindowWatcher.h"
#include "nsITimer.h"
#include "nsITimerCallback.h"



class nsRemoteBookmarks : public nsIRemoteBookmarks,
                          public nsIRDFDataSource,
                          public nsIRDFObserver,
                          public nsILDAPMessageListener,
                          public nsITimerCallback
                          // public nsIRDFRemoteDataSource,
                          // public nsIStreamListener,
                          // public nsIObserver,
                          // public nsSupportsWeakReference
{
protected:
  nsIRDFDataSource           *mInner;
  nsCOMPtr<nsISupportsArray> mObservers;
  PRInt32                    mUpdateBatchNest;

  nsCOMPtr<nsIWindowWatcher> mWindowWatcher;

  static nsIRDFResource      *kRDF_type;
  static nsIRDFResource      *kNC_Bookmark;
  static nsIRDFResource      *kNC_BookmarkSeparator;
  static nsIRDFResource      *kNC_Folder;
  static nsIRDFResource      *kNC_FolderGroup;
  static nsIRDFResource      *kNC_Parent;
  static nsIRDFResource      *kNC_Child;
  static nsIRDFResource      *kNC_URL;
  static nsIRDFResource      *kNC_Name;
  static nsIRDFResource      *kNC_ShortcutURL;
  static nsIRDFResource      *kNC_Description;
  static nsIRDFResource      *kWEB_LastCharset;
  static nsIRDFLiteral       *kTrueLiteral;

  static nsIRDFResource      *kNC_BookmarkCommand_NewBookmark;
  static nsIRDFResource      *kNC_BookmarkCommand_NewFolder;
  static nsIRDFResource      *kNC_BookmarkCommand_NewSeparator;
  static nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmark;
  static nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmarkFolder;
  static nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmarkSeparator;


  // XXX XXX XXX hack TO DO for now testing only!
  nsCOMPtr<nsILDAPConnection> mConnection;
  nsCOMPtr<nsILDAPOperation>  mLDAPOperation;
  nsCOMPtr<nsILDAPURL>        mLDAPURL;
  nsCOMPtr<nsIRDFContainer>   mContainer;
  nsCOMPtr<nsIRDFResource>    mNode;
  nsCOMPtr<nsIRDFResource>    mProperty;
  nsCOMPtr<nsIRDFNode>        mOldTarget;
  nsCOMPtr<nsIRDFNode>        mNewTarget;
  nsCOMPtr<nsIRDFLiteral>     mURLLiteral;
  nsCOMPtr<nsIRDFLiteral>     mNameLiteral;
  nsCOMPtr<nsIRDFLiteral>     mShortcutLiteral;
  nsCOMPtr<nsIRDFLiteral>     mDescLiteral;
  
  nsCOMPtr<nsITimer>          mTimer;
  nsString                    mPassword;
  PRUint32                    mOpcode;


  PRBool    isRemoteBookmarkURI(nsIRDFResource *r);
  nsresult  showLDAPError(nsILDAPMessage *aMessage);
  nsresult  doLDAPRebind();
  nsresult  doLDAPQuery(nsILDAPConnection *ldapConnection, nsIRDFResource *aParent, nsIRDFResource *aNode, nsString bindDN, nsString password, PRUint32 ldapOpcode);
  nsresult  doAuthentication(nsIRDFResource *aNode, nsString &bindDN, nsString &password);
  PRBool    GetLDAPMsgAttrValue(nsILDAPMessage *aMessage, const char *aAttrib, nsString &aValue);
  nsresult  GetLDAPExtension(nsIRDFResource *aNode, const char *name, nsCString &value, PRBool *important);
  nsresult  insertLDAPBookmarkItem(nsIRDFResource *aRelativeNode, nsISupportsArray *aArguments, nsIRDFResource *aItemType);
  nsresult  deleteLDAPBookmarkItem(nsIRDFResource *aNode, nsISupportsArray *aArguments, PRInt32 parentArgIndex, nsIRDFResource *aItemType);
  nsresult  updateLDAPBookmarkItem(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aOldTarget, nsIRDFNode *aNewTarget);
  nsresult  getArgumentN(nsISupportsArray *arguments, nsIRDFResource *res, PRInt32 offset, nsIRDFNode **argValue);
  PRBool    isMutableProperty(nsIRDFResource *aProperty);
  const char *getPropertySchemaName(nsIRDFResource *aProperty);
  nsCOMPtr<nsIRDFResource> getLDAPUrl(nsIRDFResource *aSource);
  PRBool    isRemoteContainer(nsIRDFResource *aNode);

  enum { LDAP_READY=0, LDAP_SEARCH, LDAP_ADD, LDAP_DELETE, LDAP_MODIFY };

public:
  nsRemoteBookmarks();
  virtual ~nsRemoteBookmarks();
  nsresult Init();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIRemoteBookmarks
  NS_DECL_NSIREMOTEBOOKMARKS

	// nsIRDFDataSource
	NS_DECL_NSIRDFDATASOURCE

  // nsIRDFObserver
  NS_DECL_NSIRDFOBSERVER

  // nsIRDFRemoteDataSource
//  NS_DECL_NSIRDFREMOTEDATASOURCE

  // nsILDAPMessageListener
  NS_DECL_NSILDAPMESSAGELISTENER

  // nsITimerCallback
  NS_IMETHOD_(void) Notify(nsITimer *timer);
};


#endif // remotebookmarks___h___
