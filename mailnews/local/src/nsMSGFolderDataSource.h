/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIRDFMSGFolderDataSource.h"
#include "nsIMsgFolder.h"
#include "nsISupports.h"    
#include "nsVoidArray.h"
#include "nsIRDFNode.h"
#include "nsIRDFCursor.h"
#include "nsFileSpec.h"
#include "nsIFolderListener.h"

/**
 * The mail data source.
 */
class nsMSGFolderDataSource : public nsIRDFMSGFolderDataSource, public nsIFolderListener
{
private:
  char*         mURI;
  nsVoidArray*  mObservers;
	PRBool				mInitialized;

public:
  
  NS_DECL_ISUPPORTS

  nsMSGFolderDataSource(void);
  virtual ~nsMSGFolderDataSource (void);


  // nsIRDFDataSource methods
  NS_IMETHOD Init(const char* uri);

  NS_IMETHOD GetURI(const char* *uri) const;

  NS_IMETHOD GetSource(nsISupports* property,
                       nsISupports* target,
                       PRBool tv,
                       nsISupports** source /* out */);

  NS_IMETHOD GetTarget(nsISupports* source,
                       nsISupports* property,
                       PRBool tv,
                       nsISupports** target);

  NS_IMETHOD GetSources(nsISupports* property,
                        nsISupports* target,
                        PRBool tv,
                        nsIRDFAssertionCursor** sources);

  NS_IMETHOD GetTargets(nsISupports* source,
                        nsISupports* property,    
                        PRBool tv,
                        nsIRDFAssertionCursor** targets);

  NS_IMETHOD Assert(nsISupports* source,
                    nsISupports* property, 
                    nsISupports* target,
                    PRBool tv);

  NS_IMETHOD Unassert(nsISupports* source,
                      nsISupports* property,
                      nsISupports* target);

  NS_IMETHOD HasAssertion(nsISupports* source,
                          nsISupports* property,
                          nsISupports* target,
                          PRBool tv,
                          PRBool* hasAssertion);

  NS_IMETHOD AddObserver(nsIRDFObserver* n);

  NS_IMETHOD RemoveObserver(nsIRDFObserver* n);

  NS_IMETHOD ArcLabelsIn(nsISupports* node,
                         nsIRDFArcsInCursor** labels);

  NS_IMETHOD ArcLabelsOut(nsISupports* source,
                          nsIRDFArcsOutCursor** labels); 

  NS_IMETHOD GetAllResources(nsIRDFResourceCursor** aCursor);

  NS_IMETHOD Flush();

  NS_IMETHOD GetAllCommands(nsISupports* source,
                            nsIEnumerator** commands);

  NS_IMETHOD IsCommandEnabled(nsISupportsArray* aSources,
                              nsISupports*   aCommand,
                              nsISupportsArray* aArguments);

  NS_IMETHOD DoCommand(nsISupportsArray* aSources,
                       nsISupports*   aCommand,
                       nsISupportsArray* aArguments);

  NS_IMETHOD OnItemAdded(nsIFolder *parentFolder, nsISupports *item);

  NS_IMETHOD OnItemRemoved(nsIFolder *parentFolder, nsISupports *item);

  NS_IMETHOD OnItemPropertyChanged(nsISupports *item, const char *property, const char *value);

  // caching frequently used resources
protected:

	nsresult  NotifyObservers(nsISupports *subject, nsISupports *property,
														nsISupports *object, PRBool assert);
	nsresult  GetSenderName(nsAutoString& sender, nsAutoString *senderUserName);
	nsresult  GetFolderFromMessage(nsIMessage *message, nsIMsgFolder** folder);


  static nsISupports* kNC_Child;
  static nsISupports* kNC_MessageChild;
  static nsISupports* kNC_Folder;
  static nsISupports* kNC_Name;
  static nsISupports* kNC_Columns;
  static nsISupports* kNC_MSGFolderRoot;
  static nsISupports* kNC_SpecialFolder;

  static nsISupports* kNC_Subject;
  static nsISupports* kNC_Sender;
  static nsISupports* kNC_Date;
  static nsISupports* kNC_Status;

  // commands
  static nsISupports* kNC_Delete;
  static nsISupports* kNC_Reply;
  static nsISupports* kNC_Forward;

};

