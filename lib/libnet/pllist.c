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

#include "pllist.h"
#include "prmem.h"
#include "prlog.h"

static void pl_ListRemoveEntry(PLList *list, PLListEntry *entry);

// For debugging PLList code.
#if 1
#define PL_LIST_CHECK(list)
#else
#define PL_LIST_CHECK(list) pl_ListCheck(list)

static void pl_ListCheck(PLList *list) 
{
  PLListEntry *lastEntry;
  PLListEntry *entry;

  if (!list) {
    return;
  }

  if (list->first) {
    PR_ASSERT(list->first->prev == NULL);
  }

  lastEntry = NULL;
  entry = list->first;
  while (entry) {
    if (entry->next) {
      PR_ASSERT(entry->next->prev == entry);
    }
    else {
      lastEntry = entry;
    }
    entry = entry->next;
  }

  PR_ASSERT(list->last == lastEntry);
}
#endif


PR_IMPLEMENT(PLList *)
PL_ListNew(void)
{
  return PR_NEWZAP(PLList);
}

PR_IMPLEMENT(void)
PL_ListDestroy(PLList *list)
{
  PL_LIST_CHECK(list);
  if (list) {
    while (PL_ListRemoveFirst(list));
    PR_Free(list);
  }
}

PR_IMPLEMENT(void)
PL_ListAdd(PLList *list, void *value)
{
  PLListEntry *entry;
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  
  entry = PR_NEWZAP(PLListEntry);
  entry->value = value;
  entry->next = list->first;
  if (entry->next) {
    entry->next->prev = entry;
  }
  list->first = entry;
  if (list->last == NULL) {
    list->last = entry;
  }
  PL_LIST_CHECK(list);
}

PR_IMPLEMENT(void)
PL_ListAddLast(PLList *list, void *value)
{
  PLListEntry *entry;
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  
  entry = PR_NEWZAP(PLListEntry);
  entry->value = value;
  entry->prev = list->last;
  if (entry->prev) {
    entry->prev->next = entry;
  }
  list->last = entry;
  if (list->first == NULL) {
    list->first = entry;
  }
  PL_LIST_CHECK(list);
}

PR_IMPLEMENT(void)
PL_ListInsertBefore(PLList *list, void *before, void *value)
{
  PLListEntry *entry, *beforeEntry;
  PR_ASSERT(list && before);
  PL_LIST_CHECK(list);
  beforeEntry = PL_ListFindEntry(list, before);

  if (beforeEntry) {
    entry = PR_NEWZAP(PLListEntry);
    entry->value = value;
    entry->prev = beforeEntry->prev;
    entry->next = beforeEntry;
    beforeEntry->prev = entry;
    if (beforeEntry == list->first) {
      list->first = entry;
    }
  }
  PL_LIST_CHECK(list);
}

PR_IMPLEMENT(void)
PL_ListInsertAfter(PLList *list, void *after, void *value)
{
  PLListEntry *entry, *afterEntry;
  PR_ASSERT(list && after);
  PL_LIST_CHECK(list);
  afterEntry = PL_ListFindEntry(list, after);
  
  if (afterEntry) {
    entry = PR_NEWZAP(PLListEntry);
    entry->value = value;
    entry->prev = afterEntry;
    entry->next = afterEntry->next;
    afterEntry->next = entry;
    if (after == list->last) {
      list->last = entry;
    }
  }
  PL_LIST_CHECK(list);
}

static void pl_ListRemoveEntry(PLList *list, PLListEntry *entry)
{
  PL_LIST_CHECK(list);
  if (entry->prev) {
    entry->prev->next = entry->next;
  }
  if (entry->next) {
    entry->next->prev = entry->prev;
  }
  if (list->first == entry) {
    list->first = entry->next;
  }
  if (list->last == entry) {
    list->last = entry->prev;
  }
  PR_Free(entry);
  PL_LIST_CHECK(list);
}

PR_IMPLEMENT(PRBool)
PL_ListRemove(PLList *list, void *value)
{
  PLListEntry *entry = PL_ListFindEntry(list, value);
  PL_LIST_CHECK(list);
  if (entry) {
    pl_ListRemoveEntry(list, entry);
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

PR_IMPLEMENT(PLListEntry *)
PL_ListFindEntry(PLList *list, void *value)
{
  PLListEntry *entry;
  PL_LIST_CHECK(list);
  if (!list)
    return NULL;

  entry = list->first;
  while (entry) {
    if (entry->value == value) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

PR_IMPLEMENT(void *)
PL_ListRemoveFirst(PLList *list)
{
  void *res = NULL;
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  if (list->first) {
    res = list->first->value;
    pl_ListRemoveEntry(list, list->first);
  }
  return res;
}

PR_IMPLEMENT(void *)
PL_ListFirst(PLList *list)
{
  PL_LIST_CHECK(list);
  if (!list)
    return NULL;
  return list->first ? list->first->value : NULL;
}

PR_IMPLEMENT(PLListEntry *)
PL_ListFirstEntry(PLList *list)
{
  PL_LIST_CHECK(list);
  if (!list)
    return NULL;

  return list->first;
}

PR_IMPLEMENT(void *)
PL_ListRemoveLast(PLList *list)
{
  void *res = NULL;
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  if (list->last) {
    res = list->last->value;
    pl_ListRemoveEntry(list, list->last);
  }
  return res;  
}

PR_IMPLEMENT(void *)
PL_ListLast(PLList *list)
{
  PL_LIST_CHECK(list);
  if (!list)
    return NULL;

  return list->last ? list->last->value : NULL;
}

PR_IMPLEMENT(PLListEntry *)
PL_ListLastEntry(PLList *list)
{
  PL_LIST_CHECK(list);
  if (!list)
    return NULL;

  return list->last;
}

PR_IMPLEMENT(PRBool)
PL_ListIsEmpty(PLList *list)
{
  PL_LIST_CHECK(list);
  if (!list)
    return PR_TRUE;

  return list->first ? PR_FALSE : PR_TRUE;
}

PR_IMPLEMENT(PRUint32)
PL_ListCount(PLList *list)
{
  int res = 0;
  PLListEntry *entry;
  PL_LIST_CHECK(list);

  if (!list)
    return 0;

  entry = list->first;
  while (entry) {
    entry = entry->next;
    res++;
  }
  return res;
}

PR_IMPLEMENT(void *)
PL_ListAt(PLList *list, PRUint32 index)
{
  PLListEntry *entry = PL_ListEntryAt(list, index);
  PL_LIST_CHECK(list);
  if (entry) {
    return entry->value;
  }
  return NULL;
}

PR_IMPLEMENT(PLListEntry *)
PL_ListEntryAt(PLList *list, PRUint32 index)
{
  PRUint32 idx = 0;
  PLListEntry *entry;
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  entry = list->first;
  while (entry) {
    if (idx == index)
      return entry;

    entry = entry->next;
    idx++;
  }
  return NULL;
}

PR_IMPLEMENT(PLListEntry *)
PL_ListEntryNext(PLListEntry *entry)
{
  PR_ASSERT(entry);
  return entry->next;
}

PR_IMPLEMENT(PLListEntry *)
PL_ListEntryPrev(PLListEntry *entry)
{
  PR_ASSERT(entry);
  return entry->prev;
}

PR_IMPLEMENT(void *)
PL_ListEntryValue(PLListEntry *entry)
{
  PR_ASSERT(entry);
  return entry->value;
}


/* Round-Robin */
PR_IMPLEMENT(void)
PL_ListMoveFirstToLast(PLList *list)
{
  PR_ASSERT(list);
  PL_LIST_CHECK(list);
  if (list->first && (list->first != list->last)) {
    PLListEntry *entry = list->first;
    list->first = entry->next;
    entry->next->prev = NULL;
    entry->next = NULL;
    entry->prev = list->last;
    list->last->next = entry;
    list->last = entry;
  }
  PL_LIST_CHECK(list);
}

/* Fixed so that calling PL_ListEnumReset() and PL_ListEnumNext() on
 * a NULL list is legal.
 */
PR_IMPLEMENT(void)
PL_ListEnumReset(PLList *list)
{
  if (!list) {
    return;
  }
  PL_LIST_CHECK(list);
  list->enumerate = list->first;
}

PR_IMPLEMENT(void *)
PL_ListEnumNext(PLList *list)
{
  void *res = NULL;
  if (!list) {
    return NULL;
  }
  PL_LIST_CHECK(list);
  if (list->enumerate) {
    res = list->enumerate->value;
    list->enumerate = list->enumerate->next;
  }
  return res;
}
