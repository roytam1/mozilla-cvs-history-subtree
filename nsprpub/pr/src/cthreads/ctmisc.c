/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

/*
** File:   ctmisc.c
** Descritpion:  Implemenation of miscellaneous methods for cthreads
*/

#if defined(_PR_CTHREADS)

#include "primpl.h"

#include <stdio.h>

#if defined(_PR_NO_CTHREAD_KEY_T)

#define MAX_KEYS 255

static int cur_key;

typedef void (*destructor_t)(any_t);

static destructor_t key_destructors[MAX_KEYS];
static mutex_t key_mutex = NULL;

static void
create_key_mutex()
{
  key_mutex = mutex_alloc();
  PR_ASSERT(key_mutex != NULL);

  mutex_init(key_mutex);
}

int
cthread_key_create(cthread_key_t *key, destructor_t destructor)
{
  if (!key_mutex) create_key_mutex();

  mutex_lock(key_mutex);

  if (cur_key == MAX_KEYS - 1)
  {
    mutex_unlock(key_mutex);
    return 1;
  }

  *key = cur_key;

  key_destructors[cur_key] = destructor;

  cur_key ++;

  mutex_unlock(key_mutex);
  return 0;
}

static any_t *
get_tpd_array()
{
  any_t *key_values = cthread_data(cthread_self());

  if (!key_values)
  {
    key_values = (any_t*)calloc(MAX_KEYS, sizeof(any_t));

    if (!key_values) return 1;

    cthread_set_data(cthread_self(), key_values);
  }

  return key_values;
}

int
cthread_setspecific(cthread_key_t key, any_t value)
{
  any_t *key_values = get_tpd_array();

  if (key_destructors[key] && key_values[key])
    (*key_destructors[key])(key_values[key]);

  key_values[key] = value;

  return 0;
}

int
cthread_getspecific(cthread_key_t key, any_t value)
{
  any_t *key_values = get_tpd_array();

  *((any_t*)value) = key_values[key];
  return 0;
}

void
_cthread_callkeydestructors()
{
  any_t *key_values = cthread_data(cthread_self());
  int i;

  for (i = 0; i < MAX_KEYS; i ++)
    if (key_destructors[i] && key_values[i]) (*key_destructors[i])(key_values[i]);

  free(key_values);
}

#endif /* _PR_CTHREAD_NO_CTHREAD_KEY_T */

#if 0 && defined(DEBUG)
#define CT_LOG(f) \
{ \
    static PRBool didthis = PR_FALSE; \
    if (!didthis) \
    { \
        printf("Empty function called: %s\n", f); \
        didthis = PR_TRUE; \
    } \
}
#else
#define PT_LOG(f)
#endif

PR_IMPLEMENT(void) _MD_INIT_LOCKS() {PT_LOG("_MD_INIT_LOCKS")}
PR_IMPLEMENT(void) _PR_MD_START_INTERRUPTS() {PT_LOG("_PR_MD_START_INTERRUPTS")}

PR_IMPLEMENT(void) _PR_InitCPUs(void) {PT_LOG("_PR_InitCPUs")}
PR_IMPLEMENT(void) _PR_InitStacks(void) {PT_LOG("_PR_InitStacks")}
PR_IMPLEMENT(void) _PR_InitTPD(void) {PT_LOG("_PR_InitTPD")}

PR_IMPLEMENT(void) PR_SetConcurrency(PRUintn numCPUs)  {PT_LOG("PR_SetConcurrency")}

PR_IMPLEMENT(void) PR_SetThreadRecycleMode(PRUint32 flag)
    {PT_LOG("PR_SetThreadRecycleMode")}

#endif /* defined(_PR_CTHREADS) */

/* ctmisc.c */
