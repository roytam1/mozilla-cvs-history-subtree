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

#ifndef nspr_cth_defs_h_
#define nspr_cth_defs_h_

#define CTHREAD_ZERO_THR_HANDLE(t)        t = 0
#define CTHREAD_THR_HANDLE_IS_ZERO(t)     t == 0
#define CTHREAD_COPY_THR_HANDLE(st, dt)   (dt) = (st)

#define CT_TRYLOCK_SUCCESS 0
#define CT_TRYLOCK_BUSY    1

#define CTHREAD_MUTEX_IS_LOCKED(m) (0 == mutex_try_lock((m)))

/*
** And when you really wanted hardcore locking w/o any fluff ...
**
**          ... and why would you want that????
*/
#define _PR_LOCK_LOCK(_lock)      mutex_lock(_lock->mutex);
#define _PR_LOCK_UNLOCK(_lock)    mutex_unlock(_lock->mutex);

#if defined(_PR_NO_CTHREAD_KEY_T)

#include <mach/cthreads.h>

typedef int cthread_key_t;

extern int cthread_key_create(cthread_key_t *key, void (*destructor)(any_t));
extern int cthread_setspecific(cthread_key_t key, any_t value);
extern int cthread_getspecific(cthread_key_t key, any_t value);
extern void _cthread_callkeydestructors();

#endif

#ifdef RHAPSODY
#define CT_PRIO_MIN	1
#define CT_PRIO_MAX	10 /* XXX */
#endif
#endif /* nspr_cth_defs_h_ */
