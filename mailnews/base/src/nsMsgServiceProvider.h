/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Alec Flett <alecf@netscape.com>
 * Seth Spitzer <sspitzer@netscape.com>
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

#ifndef __nsMsgServiceProvider_h
#define __nsMsgServiceProvider_h

#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsCOMPtr.h"

class nsMsgServiceProviderService : public nsIRDFDataSource
{

 public:
  nsMsgServiceProviderService();
  virtual ~nsMsgServiceProviderService();

  nsresult Init();
  
  NS_DECL_ISUPPORTS

  // we can't use NS_FORWARD_NSIRDFDATASOURCE(mInnerDataSource->)
  // since we need to override HasAssertion();
  NS_IMETHOD GetURI(char * *aURI) { return mInnerDataSource->GetURI(aURI); }
  NS_IMETHOD GetSource(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsIRDFResource **_retval) { return mInnerDataSource->GetSource(aProperty, aTarget, aTruthValue, _retval); } 
  NS_IMETHOD GetSources(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsISimpleEnumerator **_retval) { return mInnerDataSource->GetSources(aProperty, aTarget, aTruthValue, _retval); } 
  NS_IMETHOD GetTargets(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsISimpleEnumerator **_retval) { return mInnerDataSource->GetTargets(aSource, aProperty, aTruthValue, _retval); } 
  NS_IMETHOD Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue) { return mInnerDataSource->Assert(aSource, aProperty, aTarget, aTruthValue); } 
  NS_IMETHOD Unassert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget) { return mInnerDataSource->Unassert(aSource, aProperty, aTarget); } 
  NS_IMETHOD Change(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aOldTarget, nsIRDFNode *aNewTarget) { return mInnerDataSource->Change(aSource, aProperty, aOldTarget, aNewTarget); } 
  NS_IMETHOD Move(nsIRDFResource *aOldSource, nsIRDFResource *aNewSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget) { return mInnerDataSource->Move(aOldSource, aNewSource, aProperty, aTarget); } 
  NS_IMETHOD AddObserver(nsIRDFObserver *aObserver) { return mInnerDataSource->AddObserver(aObserver); } 
  NS_IMETHOD RemoveObserver(nsIRDFObserver *aObserver) { return mInnerDataSource->RemoveObserver(aObserver); } 
  NS_IMETHOD ArcLabelsIn(nsIRDFNode *aNode, nsISimpleEnumerator **_retval) { return mInnerDataSource->ArcLabelsIn(aNode, _retval); } 
  NS_IMETHOD ArcLabelsOut(nsIRDFResource *aSource, nsISimpleEnumerator **_retval) { return mInnerDataSource->ArcLabelsOut(aSource, _retval); } 
  NS_IMETHOD GetAllResources(nsISimpleEnumerator **_retval) { return mInnerDataSource->GetAllResources(_retval); } 
  NS_IMETHOD GetAllCommands(nsIRDFResource *aSource, nsIEnumerator **_retval) { return mInnerDataSource->GetAllCommands(aSource, _retval); } 
  NS_IMETHOD IsCommandEnabled(nsISupportsArray *aSources, nsIRDFResource *aCommand, nsISupportsArray *aArguments, PRBool *_retval) { return mInnerDataSource->IsCommandEnabled(aSources, aCommand, aArguments, _retval); } 
  NS_IMETHOD DoCommand(nsISupportsArray *aSources, nsIRDFResource *aCommand, nsISupportsArray *aArguments) { return mInnerDataSource->DoCommand(aSources, aCommand, aArguments); } 
  NS_IMETHOD GetAllCmds(nsIRDFResource *aSource, nsISimpleEnumerator **_retval) { return mInnerDataSource->GetAllCmds(aSource, _retval); } 
  NS_IMETHOD HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *_retval) { return mInnerDataSource->HasArcIn(aNode, aArc, _retval); } 
  NS_IMETHOD HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *_retval) { return mInnerDataSource->HasArcOut(aSource, aArc, _retval); } 
  NS_IMETHOD GetTarget(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **target) { return mInnerDataSource->GetTarget(aSource, aProperty, aTruthValue, target); }

  // override HasAssertion() so I can properly handle the canCreateAccount property
  NS_IMETHOD HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, PRBool *_retval);
  
 private:

  nsCOMPtr<nsIRDFCompositeDataSource> mInnerDataSource;

  nsresult LoadDataSource(const char *aURL);

  nsCOMPtr <nsIRDFResource> mCanCreateAccountProperty;
  nsCOMPtr <nsIRDFResource> mMaxAccountsProperty;
  nsCOMPtr <nsIRDFResource> mRedirectorTypeProperty;
};


#endif
