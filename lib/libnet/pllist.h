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

#ifndef pllist_h__
#define pllist_h__

/*
 * API to portable list code.
 */
#include <stddef.h>
#include <stdio.h>
#include "prtypes.h"

typedef struct PLList PLList;
typedef struct PLListEntry PLListEntry;

struct PLList {
  PLListEntry *first;
  PLListEntry *last;
  PLListEntry *enumerate;
};

struct PLListEntry {
  PLListEntry    *prev;
  PLListEntry    *next;
  void           *value;
};

PR_BEGIN_EXTERN_C

PR_EXTERN(PLList *)
PL_ListNew(void);

PR_EXTERN(void)
PL_ListDestroy(PLList *list);

PR_EXTERN(void)
PL_ListAdd(PLList *list, void *value);

PR_EXTERN(void)
PL_ListAddLast(PLList *list, void *value);

PR_EXTERN(void)
PL_ListInsertBefore(PLList *list, void *before, void *value);

PR_EXTERN(void)
PL_ListInsertAfter(PLList *list, void *after, void *value);

PR_EXTERN(PRBool)
PL_ListRemove(PLList *list, void *value);

PR_EXTERN(PLListEntry *)
PL_ListFindEntry(PLList *list, void *value);

PR_EXTERN(void *)
PL_ListRemoveFirst(PLList *list);

PR_EXTERN(void *)
PL_ListFirst(PLList *list);

PR_EXTERN(PLListEntry *)
PL_ListFirstEntry(PLList *list);

PR_EXTERN(void *)
PL_ListRemoveLast(PLList *list);

PR_EXTERN(void *)
PL_ListLast(PLList *list);

PR_EXTERN(PLListEntry *)
PL_ListLastEntry(PLList *list);

PR_EXTERN(PRBool)
PL_ListIsEmpty(PLList *list);

PR_EXTERN(PRUint32)
PL_ListCount(PLList *list);

PR_EXTERN(void *)
PL_ListAt(PLList *list, PRUint32 index);

PR_EXTERN(PLListEntry *)
PL_ListEntryAt(PLList *list, PRUint32 index);

PR_EXTERN(PLListEntry *)
PL_ListEntryNext(PLListEntry *entry);

PR_EXTERN(PLListEntry *)
PL_ListEntryPrev(PLListEntry *entry);

PR_EXTERN(void *)
PL_ListEntryValue(PLListEntry *entry);

PR_EXTERN(void)
PL_ListEnumReset(PLList *list);

PR_EXTERN(void *)
PL_ListEnumNext(PLList *list);

/* Round-Robin */
PR_EXTERN(void)
PL_ListMoveFirstToLast(PLList *list);

PR_END_EXTERN_C

#endif /* pllist_h__ */
