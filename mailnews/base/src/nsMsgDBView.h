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

enum eFieldType {
    kString,
    kU16,
    kU32,
    kU64
};

// reserve the top 8 bits in the msg flags for the view-only flags.
#define MSG_VIEW_FLAGS 0xFE000000
#define MSG_VIEW_FLAG_HASCHILDREN 0x40000000
#define MSG_VIEW_FLAG_ISTHREAD 0x8000000

typedef PRInt32 nsMsgDBViewFlags;
	// flags for GetViewFlags
const int kOutlineDisplay = 0x1;
const int kFlatDisplay = 0x2;
const int kShowIgnored = 0x8;
const int kUnreadOnly = 0x10;

// I think this will be an abstract implementation class.
// The classes that implement the outliner support will probably
// inherit from this class.
class nsMsgDBView : public nsIMsgDBView, public nsIDBChangeListener
{
public:
  nsMsgDBView();
  virtual ~nsMsgDBView();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGDBVIEW
  NS_DECL_NSIDBCHANGELISTENER
protected:
  // routines used in building up view
  PRBool WantsThisThread(nsIMsgThread * thread);

  nsresult ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded);
  nsresult ExpandAll();
  nsresult ReverseSort();
  nsresult ReverseThreads();

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
  nsresult GetStringField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, char **result);
  nsresult GetLongField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result);
  
  nsMsgKeyArray m_keys;
  nsUInt32Array m_flags;
  nsUint8Array   m_levels;

  nsCOMPtr <nsIMsgDatabase> m_db;
  PRBool		m_sortValid;
  nsMsgViewSortTypeValue  m_sortType;
  nsMsgViewSortOrderValue m_sortOrder;
  nsMsgDBViewTypeValue m_viewType;
	nsMsgDBViewFlags	m_viewFlags;
};


#endif
