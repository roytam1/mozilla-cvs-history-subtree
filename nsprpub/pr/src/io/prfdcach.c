/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "primpl.h"

#include <string.h>

/*****************************************************************************/
/*****************************************************************************/
/************************** File descriptor caching **************************/
/*****************************************************************************/
/*****************************************************************************/

/*
** This code is built into debuggable versions of NSPR to assist in
** finding misused file descriptors. Since file descritors (PRFileDesc)
** are identified by a pointer to their structure, they can be the
** target of dangling references. Furthermore, NSPR caches and tries
** to aggressively reuse file descriptors, leading to more ambiguity.
** The following code will allow a debugging client to set environment
** variables and control the number of file descriptors that will be
** preserved before they are recycled. The environment variables are
** NSPR_FD_CACHE_SIZE_LOW and NSPR_FD_CACHE_SIZE_HIGH. The former sets
** the number of descriptors NSPR will allocate before beginning to
** recycle. The latter is the maximum number permitted in the cache
** (exclusive of those in use) at a time.
*/
typedef struct _PR_Fd_Cache
{
    PRLock *ml;
    PRIntn count;
#ifdef SOLARIS
    /* 
     * For now Solaris, but eventually all platforms without atomic
     * stack instructions, will utilize an array of freelists for
     * performance.  This replaces the LIFO stack implementation, but
     * not the FIFO cache implementation used by default in DEBUG
     * builds.  To engage with a DEBUG build, set the environment
     * variable NSPR_FD_CACHE_SIZE_HIGH to 0.
     */
    PRFileDesc **freelist;
    pthread_mutex_t *freelist_lock;
#else
    PRStack *stack;
#endif
    PRFileDesc *head, *tail;
    PRIntn limit_low, limit_high;
} _PR_Fd_Cache;

static _PR_Fd_Cache _pr_fd_cache;
static PRFileDesc **stack2fd = &(((PRFileDesc*)NULL)->higher);

#ifdef SOLARIS
/* 
 * The number of freelists can be set by the environment variable
 * NSPR_NUM_FD_FREELISTS
 */
#define DEFAULT_NUM_FD_FREELISTS 32
static _pr_num_fd_freelists = DEFAULT_NUM_FD_FREELISTS;

static PRFileDesc *_PR_GetFdFromFreelist()
{
    int i;
    int m = (PRUptrdiff)pthread_self() % _pr_num_fd_freelists;
    PRFileDesc *rv = NULL;
    
    for (i = m; i < _pr_num_fd_freelists; i++) {
        if (0 == pthread_mutex_trylock(&_pr_fd_cache.freelist_lock[i])) {
            if ((rv = _pr_fd_cache.freelist[i]) != NULL) {
                _pr_fd_cache.freelist[i] = rv->higher;
            }
            pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[i]);
        }
        if (NULL != rv) {
            return rv;
        }
    }
    for (i = 0; i < m; i++) {
        if (0 == pthread_mutex_trylock(&_pr_fd_cache.freelist_lock[i])) {
            if ((rv = _pr_fd_cache.freelist[i]) != NULL) {
                _pr_fd_cache.freelist[i] = rv->higher;
            }
            pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[i]);
        }
        if (NULL != rv) {
            return rv;
        }
    }
    pthread_mutex_lock(&_pr_fd_cache.freelist_lock[m]);
    if ((rv = _pr_fd_cache.freelist[m]) != NULL) {
        _pr_fd_cache.freelist[m] = rv->higher;
    }
    pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[m]);
    return rv;
}

static void _PR_PutFdToFreelist(PRFileDesc *fd)
{
    int i;
    int m = (PRUptrdiff)pthread_self() % _pr_num_fd_freelists;

    for (i = m; i < _pr_num_fd_freelists; i++) {
        if (0 == pthread_mutex_trylock(&_pr_fd_cache.freelist_lock[i])) {
            fd->higher = _pr_fd_cache.freelist[i];
            _pr_fd_cache.freelist[i] = fd;
            pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[i]);
            return;
        }
    }
    for (i = 0; i < m; i++) {
        if (0 == pthread_mutex_trylock(&_pr_fd_cache.freelist_lock[i])) {
            fd->higher = _pr_fd_cache.freelist[i];
            _pr_fd_cache.freelist[i] = fd;
            pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[i]);
            return;
        }
    }
    pthread_mutex_lock(&_pr_fd_cache.freelist_lock[m]);
    fd->higher = _pr_fd_cache.freelist[m];
    _pr_fd_cache.freelist[m] = fd;
    pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[m]);
}
#endif


/*
** Get a FileDescriptor from the cache if one exists. If not allocate
** a new one from the heap.
*/
PRFileDesc *_PR_Getfd()
{
    PRFileDesc *fd;
    /*
    ** $$$
    ** This may look a little wasteful. We'll see. Right now I want to
    ** be able to toggle between caching and not at runtime to measure
    ** the differences. If it isn't too annoying, I'll leave it in.
    ** $$$$
    **
    ** The test is against _pr_fd_cache.limit_high. If that's zero,
    ** we're not doing the extended cache but going for performance.
    */
    if (0 == _pr_fd_cache.limit_high)
    {
#ifdef SOLARIS
        fd = _PR_GetFdFromFreelist();
        if (NULL == fd) goto allocate;
#else
        PRStackElem *pop;
        PR_ASSERT(NULL != _pr_fd_cache.stack);
        pop = PR_StackPop(_pr_fd_cache.stack);
        if (NULL == pop) goto allocate;
        fd = (PRFileDesc*)((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
#endif
    }
    else
    {
        do
        {
            if (NULL == _pr_fd_cache.head) goto allocate;  /* nothing there */
            if (_pr_fd_cache.count < _pr_fd_cache.limit_low) goto allocate;

            /* we "should" be able to extract an fd from the cache */
            PR_Lock(_pr_fd_cache.ml);  /* need the lock to do this safely */
            fd = _pr_fd_cache.head;  /* protected extraction */
            if (NULL == fd)  /* unexpected, but not fatal */
            {
                PR_ASSERT(0 == _pr_fd_cache.count);
                PR_ASSERT(NULL == _pr_fd_cache.tail);
            }
            else
            {
                _pr_fd_cache.count -= 1;
                _pr_fd_cache.head = fd->higher;
                if (NULL == _pr_fd_cache.head)
                {
                    PR_ASSERT(0 == _pr_fd_cache.count);
                    _pr_fd_cache.tail = NULL;
                }
                PR_ASSERT(&_pr_faulty_methods == fd->methods);
                PR_ASSERT(PR_INVALID_IO_LAYER == fd->identity);
                PR_ASSERT(_PR_FILEDESC_FREED == fd->secret->state);
            }
            PR_Unlock(_pr_fd_cache.ml);

        } while (NULL == fd);  /* then go around and allocate a new one */
    }

finished:
    fd->dtor = NULL;
    fd->lower = fd->higher = NULL;
    fd->identity = PR_NSPR_IO_LAYER;
    memset(fd->secret, 0, sizeof(PRFilePrivate));
    return fd;

allocate:
    fd = PR_NEW(PRFileDesc);
    if (NULL != fd)
    {
        fd->secret = PR_NEW(PRFilePrivate);
        if (NULL == fd->secret) PR_DELETE(fd);
    }
    if (NULL != fd) goto finished;
    else return NULL;

}  /* _PR_Getfd */

/*
** Return a file descriptor to the cache unless there are too many in
** there already. If put in cache, clear the fields first.
*/
void _PR_Putfd(PRFileDesc *fd)
{
    PR_ASSERT(PR_NSPR_IO_LAYER == fd->identity);
    fd->methods = &_pr_faulty_methods;
    fd->identity = PR_INVALID_IO_LAYER;
    fd->secret->state = _PR_FILEDESC_FREED;

    if (0 == _pr_fd_cache.limit_high)
    {
#ifdef SOLARIS
        _PR_PutFdToFreelist(fd);
#else
        PR_StackPush(_pr_fd_cache.stack, (PRStackElem*)(&fd->higher));
#endif
    }
    else
    {
        if (_pr_fd_cache.count > _pr_fd_cache.limit_high)
        {
            PR_Free(fd->secret);
            PR_Free(fd);
        }
        else
        {
            PR_Lock(_pr_fd_cache.ml);
            if (NULL == _pr_fd_cache.tail)
            {
                PR_ASSERT(0 == _pr_fd_cache.count);
                PR_ASSERT(NULL == _pr_fd_cache.head);
                _pr_fd_cache.head = _pr_fd_cache.tail = fd;
            }
            else
            {
                PR_ASSERT(NULL == _pr_fd_cache.tail->higher);
                _pr_fd_cache.tail->higher = fd;
                _pr_fd_cache.tail = fd;  /* new value */
            }
            fd->higher = NULL;  /* always so */
            _pr_fd_cache.count += 1;  /* count the new entry */
            PR_Unlock(_pr_fd_cache.ml);
        }
    }
}  /* _PR_Putfd */

PR_IMPLEMENT(PRStatus) PR_SetFDCacheSize(PRIntn low, PRIntn high)
{
    /*
    ** This can be called at any time, may adjust the cache sizes,
    ** turn the caches off, or turn them on. It is not dependent
    ** on the compilation setting of DEBUG.
    */
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (low > high) low = high;  /* sanity check the params */
    
    PR_Lock(_pr_fd_cache.ml);
    if (0 == high)  /* shutting down or staying down */
    {
        if (0 != _pr_fd_cache.limit_high)  /* shutting down */
        {
            _pr_fd_cache.limit_high = 0;  /* stop use */
            /*
            ** Hold the lock throughout - nobody's going to want it
            ** other than another caller to this routine. Just don't
            ** let that happen.
            **
            ** Put all the cached fds onto the new cache.
            */
            while (NULL != _pr_fd_cache.head)
            {
                PRFileDesc *fd = _pr_fd_cache.head;
                _pr_fd_cache.head = fd->higher;
#ifdef SOLARIS
                _PR_PutFdToFreelist(fd);
#else
                PR_StackPush(_pr_fd_cache.stack, (PRStackElem*)(&fd->higher));
#endif
            }
            _pr_fd_cache.limit_low = 0;
            _pr_fd_cache.tail = NULL;
            _pr_fd_cache.count = 0;
        }
    }
    else  /* starting up or just adjusting parameters */
    {
        PRBool was_using_stack = (0 == _pr_fd_cache.limit_high);
        _pr_fd_cache.limit_low = low;
        _pr_fd_cache.limit_high = high;
        if (was_using_stack)  /* was using stack - feed into cache */
        {
#ifdef SOLARIS
            int i;
            for (i = 0; i < _pr_num_fd_freelists; i++)
            {
                PRFileDesc *fd;
                pthread_mutex_lock(&_pr_fd_cache.freelist_lock[i]);
                while (NULL != (fd = _pr_fd_cache.freelist[i])) {
                    _pr_fd_cache.freelist[i] = fd->higher;
                    if (NULL == _pr_fd_cache.tail) _pr_fd_cache.tail = fd;
                    fd->higher = _pr_fd_cache.head;
                    _pr_fd_cache.head = fd;
                    _pr_fd_cache.count += 1;
                }
                pthread_mutex_unlock(&_pr_fd_cache.freelist_lock[i]);
            }
#else
            PRStackElem *pop;
            while (NULL != (pop = PR_StackPop(_pr_fd_cache.stack)))
            {
                PRFileDesc *fd = (PRFileDesc*)
                    ((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
                if (NULL == _pr_fd_cache.tail) _pr_fd_cache.tail = fd;
                fd->higher = _pr_fd_cache.head;
                _pr_fd_cache.head = fd;
                _pr_fd_cache.count += 1;
            }
#endif
        }
    }
    PR_Unlock(_pr_fd_cache.ml);
    return PR_SUCCESS;
}  /* PR_SetFDCacheSize */

void _PR_InitFdCache()
{
    /*
    ** The fd caching is enabled by default for DEBUG builds,
    ** disabled by default for OPT builds. That default can
    ** be overridden at runtime using environment variables
    ** or a super-wiz-bang API.
    */
    const char *low = PR_GetEnv("NSPR_FD_CACHE_SIZE_LOW");
    const char *high = PR_GetEnv("NSPR_FD_CACHE_SIZE_HIGH");
#ifdef SOLARIS
    const char *num_freelists = PR_GetEnv("NSPR_NUM_FD_FREELISTS");
    int i;
#endif

    /* 
    ** _low is allowed to be zero, _high is not.
    ** If _high is zero, we're not doing the caching.
    */

    _pr_fd_cache.limit_low = 0;
#if defined(DEBUG)
    _pr_fd_cache.limit_high = FD_SETSIZE;
#else
    _pr_fd_cache.limit_high = 0;
#endif  /* defined(DEBUG) */

    if (NULL != low) _pr_fd_cache.limit_low = atoi(low);
    if (NULL != high) _pr_fd_cache.limit_high = atoi(high);

    if (_pr_fd_cache.limit_high < _pr_fd_cache.limit_low)
        _pr_fd_cache.limit_high = _pr_fd_cache.limit_low;

    _pr_fd_cache.ml = PR_NewLock();
    PR_ASSERT(NULL != _pr_fd_cache.ml);
#ifdef SOLARIS
    if (NULL != num_freelists) {
        _pr_num_fd_freelists = atoi(num_freelists);
    }
    if (_pr_num_fd_freelists <= 0) {
        _pr_num_fd_freelists = DEFAULT_NUM_FD_FREELISTS;
    }
    _pr_fd_cache.freelist =  (PRFileDesc **)
        PR_Malloc(sizeof(PRFileDesc *) * _pr_num_fd_freelists);
    PR_ASSERT(NULL != _pr_fd_cache.freelist);
    _pr_fd_cache.freelist_lock = (pthread_mutex_t *) 
        PR_Malloc(sizeof(pthread_mutex_t) * _pr_num_fd_freelists);
    PR_ASSERT(_pr_fd_cache.freelist_lock != NULL );
    for (i = 0; i < _pr_num_fd_freelists; i++) {
        int rv = pthread_mutex_init(&_pr_fd_cache.freelist_lock[i], NULL);
        PR_ASSERT(0 == rv);
        _pr_fd_cache.freelist[i] = NULL;
    }
#else
    _pr_fd_cache.stack = PR_CreateStack("FD");
    PR_ASSERT(NULL != _pr_fd_cache.stack);
#endif

}  /* _PR_InitFdCache */

void _PR_CleanupFdCache(void)
{
    PRFileDesc *fd, *next;
#ifdef SOLARIS
    int i;
#else
    PRStackElem *pop;
#endif

    for (fd = _pr_fd_cache.head; fd != NULL; fd = next)
    {
        next = fd->higher;
        PR_DELETE(fd->secret);
        PR_DELETE(fd);
    }
    PR_DestroyLock(_pr_fd_cache.ml);
#ifdef SOLARIS
    for (i = 0; i < _pr_num_fd_freelists; i++) {
        while ((fd = _pr_fd_cache.freelist[i]) != NULL) {
            _pr_fd_cache.freelist[i] = fd->higher;
            PR_DELETE(fd->secret);
            PR_DELETE(fd);
        }
        pthread_mutex_destroy(&_pr_fd_cache.freelist_lock[i]);
    }
    PR_DELETE(_pr_fd_cache.freelist);
    PR_DELETE(_pr_fd_cache.freelist_lock);
#else
    while ((pop = PR_StackPop(_pr_fd_cache.stack)) != NULL)
    {
        fd = (PRFileDesc*)((PRPtrdiff)pop - (PRPtrdiff)stack2fd);
        PR_DELETE(fd->secret);
        PR_DELETE(fd);
    }
    PR_DestroyStack(_pr_fd_cache.stack);
#endif
}  /* _PR_CleanupFdCache */

/* prfdcach.c */
