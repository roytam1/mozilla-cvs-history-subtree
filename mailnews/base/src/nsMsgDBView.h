/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

#ifndef _nsMsgDBView_H_
#define _nsMsgDBView_H_

#include "nsIMsgDBView.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgHdr.h"
#include "nsMsgLineBuffer.h" // for nsByteArray
#include "nsMsgKeyArray.h"
#include "nsUint8Array.h"
#include "nsIDBChangeListener.h"
#include "nsIOutlinerView.h"
#include "nsIOutlinerBoxObject.h"
#include "nsVoidArray.h"

enum eFieldType {
    kString,
    kU32,
    kPRTime
};

// reserve the top 8 bits in the msg flags for the view-only flags.
#define MSG_VIEW_FLAGS 0xFE000000
#define MSG_VIEW_FLAG_HASCHILDREN 0x40000000
#define MSG_VIEW_FLAG_ISTHREAD 0x8000000

// I think this will be an abstract implementation class.
// The classes that implement the outliner support will probably
// inherit from this class.
class nsMsgDBView : public nsIMsgDBView, public nsIDBChangeListener, public nsIOutlinerView
{
public:
  nsMsgDBView();
  virtual ~nsMsgDBView();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGDBVIEW
  NS_DECL_NSIDBCHANGELISTENER
  NS_DECL_NSIOUTLINERVIEW

protected:

  // mscott: talk to hyatt about making this  a weak reference?
  nsCOMPtr<nsIOutlinerBoxObject> mOutliner;

  // routines used in building up view
  PRBool WantsThisThread(nsIMsgThread * thread);

  nsresult ToggleExpansion(nsMsgViewIndex index, PRUint32 *numChanged);
  nsresult ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded);
  nsresult CollapseByIndex(nsMsgViewIndex index, PRUint32 *pNumCollapsed);
  nsresult ExpandAll();

  // helper routines for thread expanding and collapsing.
  nsresult		GetThreadCount(nsMsgKey messageKey, PRUint32 *pThreadCount);
  nsMsgViewIndex GetIndexOfFirstDisplayedKeyInThread(nsIMsgThread *threadHdr);
  nsMsgViewIndex ThreadIndexOfMsg(nsMsgKey msgKey, 
											  nsMsgViewIndex msgIndex = nsMsgViewIndex_None,
											  PRInt32 *pThreadCount = nsnull,
											  PRUint32 *pFlags = nsnull);
  nsMsgKey GetKeyOfFirstMsgInThread(nsMsgKey key);
  PRInt32 CountExpandedThread(nsMsgViewIndex index);
  nsresult ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta);
  nsresult ReverseSort();
  nsresult ReverseThreads();

	nsMsgKey		GetAt(nsMsgViewIndex index) ;
	nsMsgViewIndex	FindViewIndex(nsMsgKey  key) 
						{return (nsMsgViewIndex) (m_keys.FindIndex(key));}
	virtual nsMsgViewIndex	FindKey(nsMsgKey key, PRBool expand);

  nsresult	ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex viewIndex, PRUint32 *pNumListed);
  PRInt32   FindLevelInThread(nsIMsgDBHdr *msgHdr, nsMsgKey msgKey, nsMsgViewIndex startOfThreadViewIndex);
	PRInt32	  GetSize(void) {return(m_keys.GetSize());}

  // notification api's
	void	EnableChangeUpdates();
	void	DisableChangeUpdates();
	void	NoteChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							   nsMsgViewNotificationCodeValue changeType);
	void	NoteStartChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							   nsMsgViewNotificationCodeValue changeType);
	void	NoteEndChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							   nsMsgViewNotificationCodeValue changeType);

  nsresult GetFieldTypeAndLenForSort(nsMsgViewSortTypeValue sortType, PRUint16 *pMaxLen, eFieldType *pFieldType);
  nsresult GetStringField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUnichar **result);
  nsresult GetLongField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result);
  nsresult GetPRTimeField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRTime *result);

  void FreeAll(nsVoidArray *ptrs);
  
  nsMsgKeyArray m_keys;
  nsUInt32Array m_flags;
  nsUint8Array   m_levels;

  // cache the most recently asked fo header and corresponding msgKey.
  nsCOMPtr <nsIMsgDBHdr>  m_cachedHdr;
  nsMsgKey                m_cachedMsgKey;

  nsCOMPtr <nsIMsgDatabase> m_db;
  PRBool		m_sortValid;
  nsMsgViewSortTypeValue  m_sortType;
  nsMsgViewSortOrderValue m_sortOrder;
  nsMsgDBViewTypeValue m_viewType;
  nsMsgViewFlagsTypeValue m_viewFlags;
};


#endif
