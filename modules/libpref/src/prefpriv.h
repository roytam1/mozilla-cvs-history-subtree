/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/********************************************************************/
 
#ifndef _PREF_PRIVATE_H_
#define _PREF_PRIVATE_H_

#include "prtypes.h"


#ifdef XP_WIN

#include "assert.h"

#define NOT_NULL(X)	X
#ifdef XP_ASSERT
#undef XP_ASSERT
#endif

#define XP_ASSERT(X) assert(X)
#define LINEBREAK "\n"
#endif

/*
 * Creates an iterator over the children of a node.
 */
typedef struct 
{
	char*		 childList;
	char*		 parent;
	unsigned int bufsize;
} PrefChildIter; 


struct CallbackNode {
	char*					domain;
	PrefChangedFunc			func;
	void*					data;
	struct CallbackNode*	next;
};

typedef struct _EntryInfo {
    int     style;
    void    *pData;
} EntryInfo;


#define PREF_IS_LOCKED(pref)			((pref)->flags & PREF_LOCKED)
#define PREF_IS_CONFIG(pref)			((pref)->flags & PREF_CONFIG)
#define PREF_HAS_USER_VALUE(pref)		((pref)->flags & PREF_USERSET)
#define PREF_HAS_LI_VALUE(pref)			((pref)->flags & PREF_LILOCAL) 


PR_EXTERN(int)   pref_CompareStrings (const void *v1, const void *v2);
extern int pref_DoCallback(const char* changed_pref);


/* These constants are borrowed from the mail code, but we can't include
   them directly due to circular dependencies */

#define NEWS_PORT 119
#define SECURE_NEWS_PORT 563   

typedef enum
{
	MSG_Pop3 = 0,
	MSG_Imap4 = 1,
	MSG_MoveMail = 2,
	MSG_Inbox = 3
} MSG_SERVER_TYPE;

#endif
