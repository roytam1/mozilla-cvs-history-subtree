*** memcache.c	Tue Feb  8 11:06:59 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/memcache.c	Tue May  9 13:49:29 2000
***************
*** 1,23 ****
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public
!  * License Version 1.1 (the "License"); you may not use this file
!  * except in compliance with the License. You may obtain a copy of
!  * the License at http://www.mozilla.org/NPL/
   *
!  * Software distributed under the License is distributed on an "AS
!  * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
!  * implied. See the License for the specific language governing
!  * rights and limitations under the License.
   *
!  * The Original Code is mozilla.org code.
!  *
!  * The Initial Developer of the Original Code is Netscape
   * Communications Corporation.  Portions created by Netscape are
!  * Copyright (C) 1998 Netscape Communications Corporation. All
!  * Rights Reserved.
!  *
!  * Contributor(s): 
   */
  /*
   *
--- 1,19 ----
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public License
!  * Version 1.0 (the "NPL"); you may not use this file except in
!  * compliance with the NPL.  You may obtain a copy of the NPL at
!  * http://www.mozilla.org/NPL/
   *
!  * Software distributed under the NPL is distributed on an "AS IS" basis,
!  * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
!  * for the specific language governing rights and limitations under the
!  * NPL.
   *
!  * The Initial Developer of this code under the NPL is Netscape
   * Communications Corporation.  Portions created by Netscape are
!  * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
!  * Reserved.
   */
  /*
   *
***************
*** 200,206 ****
--- 196,204 ----
  static int memcache_append(LDAP *ld, int msgid, LDAPMessage *pRes);
  static int memcache_append_last(LDAP *ld, int msgid, LDAPMessage *pRes);
  static int memcache_remove(LDAP *ld, int msgid);
+ #if 0	/* function not used */
  static int memcache_remove_all(LDAP *ld);
+ #endif /* 0 */
  static int memcache_access(LDAPMemCache *cache, int mode, 
  			   void *pData1, void *pData2, void *pData3);
  #ifdef LDAP_DEBUG
***************
*** 358,364 ****
          LDAPMemCache *c = ld->ld_memcache;
  	ldapmemcacheld *pCur = NULL;
  	ldapmemcacheld *pPrev = NULL;
- 	ldapmemcacheld *pNode = NULL;
  
  	/* First dissociate handle from old cache */
  
--- 356,361 ----
***************
*** 475,484 ****
  LDAP_CALL
  ldap_memcache_flush( LDAPMemCache *cache, char *dn, int scope )
  {
-     int nRes = LDAP_SUCCESS;
-     char *basedn = NULL;
-     ldapmemcacheRes *pRes = NULL, *pPrevRes = NULL;
- 
      LDAPDebug( LDAP_DEBUG_TRACE,
  	    "ldap_memcache_flush( cache: 0x%x, dn: %s, scope: %d)\n",
  	    cache, ( dn == NULL ) ? "(null)" : dn, scope );
--- 472,477 ----
***************
*** 679,685 ****
      LDAPMessage *pMsg = NULL;
  
      LDAPDebug( LDAP_DEBUG_TRACE,
! 	    "ldap_memcache_result( ld: 0x%x, msgid: %d, key: 0x%08.8lx)\n",
  	    ld, msgid, key );
  
      if ( !NSLDAPI_VALID_LDAP_POINTER( ld ) || (msgid < 0) ) {
--- 672,678 ----
      LDAPMessage *pMsg = NULL;
  
      LDAPDebug( LDAP_DEBUG_TRACE,
! 	    "ldap_memcache_result( ld: 0x%x, msgid: %d, key: 0x%8.8lx)\n",
  	    ld, msgid, key );
  
      if ( !NSLDAPI_VALID_LDAP_POINTER( ld ) || (msgid < 0) ) {
***************
*** 699,709 ****
  	nRes = memcache_add_to_ld(ld, msgid, pMsg);
  	++ld->ld_memcache->ldmemc_stats.ldmemcstat_hits;
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"ldap_memcache_result: key 0x%08.8lx found in cache\n",
  		key, 0, 0 );
      } else {
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"ldap_memcache_result: key 0x%08.8lx not found in cache\n",
  		key, 0, 0 );
      }
  
--- 692,702 ----
  	nRes = memcache_add_to_ld(ld, msgid, pMsg);
  	++ld->ld_memcache->ldmemc_stats.ldmemcstat_hits;
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"ldap_memcache_result: key 0x%8.8lx found in cache\n",
  		key, 0, 0 );
      } else {
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"ldap_memcache_result: key 0x%8.8lx not found in cache\n",
  		key, 0, 0 );
      }
  
***************
*** 1072,1077 ****
--- 1065,1071 ----
  	                   (void*)&reqid, NULL, NULL);
  }
  
+ #if 0 /* this function is not used */
  /* Wipes out everything in the temporary cache directory. */
  static int
  memcache_remove_all(LDAP *ld)
***************
*** 1082,1087 ****
--- 1076,1082 ----
      return memcache_access(ld->ld_memcache, MEMCACHE_ACCESS_DELETE_ALL,
  	                   NULL, NULL, NULL);
  }
+ #endif /* 0 */
  
  /* Returns TRUE or FALSE */
  static int
***************
*** 1217,1223 ****
  				LDAPMessage **ppResCopy, unsigned long *pSize)
  {
      int nRes = LDAP_SUCCESS;
-     int bEnd = 0;
      unsigned long ber_size;
      LDAPMessage *pCur;
      LDAPMessage **ppCurNew;
--- 1212,1217 ----
***************
*** 1377,1383 ****
      for ( restmp = cache->ldmemc_resHead[index]; restmp != NULL;
  	    restmp = restmp->ldmemcr_next[index] ) {
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"    key: 0x%08.8lx, ld: 0x%x, msgid: %d\n",
  		restmp->ldmemcr_crc_key,
  		restmp->ldmemcr_req_id.ldmemcrid_ld,
  		restmp->ldmemcr_req_id.ldmemcrid_msgid );
--- 1371,1377 ----
      for ( restmp = cache->ldmemc_resHead[index]; restmp != NULL;
  	    restmp = restmp->ldmemcr_next[index] ) {
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"    key: 0x%8.8lx, ld: 0x%x, msgid: %d\n",
  		restmp->ldmemcr_crc_key,
  		restmp->ldmemcr_req_id.ldmemcrid_ld,
  		restmp->ldmemcr_req_id.ldmemcrid_msgid );
***************
*** 1412,1418 ****
      /* Add a new cache header to the cache. */
      if (mode == MEMCACHE_ACCESS_ADD) {
  	unsigned long key = *((unsigned long*)pData1);
- 	ldapmemcacheReqId *pReqId = (ldapmemcacheReqId*)pData2;
  	char *basedn = (char*)pData3;
  	ldapmemcacheRes *pRes = NULL;
  
--- 1406,1411 ----
***************
*** 1494,1500 ****
      /* Search for cached entries for a particular search. */
      else if (mode == MEMCACHE_ACCESS_FIND) {
  
- 	unsigned long key = *((unsigned long*)pData1);
  	ldapmemcacheRes **ppRes = (ldapmemcacheRes**)pData2;
  
  	nRes = htable_get(cache->ldmemc_resLookup, pData1, (void**)ppRes);
--- 1487,1492 ----
***************
*** 1643,1649 ****
  	    return LDAP_NO_SUCH_OBJECT;
  
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"memcache_access FLUSH_LRU: removing key 0x%08.8lx\n",
  		pRes->ldmemcr_crc_key, 0, 0 );
  	nRes = htable_remove(cache->ldmemc_resLookup,
  	              (void*)&(pRes->ldmemcr_crc_key), NULL);
--- 1635,1641 ----
  	    return LDAP_NO_SUCH_OBJECT;
  
  	LDAPDebug( LDAP_DEBUG_TRACE,
! 		"memcache_access FLUSH_LRU: removing key 0x%8.8lx\n",
  		pRes->ldmemcr_crc_key, 0, 0 );
  	nRes = htable_remove(cache->ldmemc_resLookup,
  	              (void*)&(pRes->ldmemcr_crc_key), NULL);
***************
*** 2178,2188 ****
  crc32_convert(char *buf, int len)
  {
      char *p;
      unsigned long crc;	    /* FIXME: this is not 32-bits on all platforms! */
  
      crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
      for (p = buf; len > 0; ++p, --len)
  	crc = ((crc << 8) ^ crc32_table[(crc >> 24) ^ *p]) & 0xffffffff;
  
!     return ~crc;            /* transmit complement, per CRC-32 spec */
  }
--- 2170,2184 ----
  crc32_convert(char *buf, int len)
  {
      char *p;
+ #ifdef OSF1V4D
+     unsigned int crc;
+ #else
      unsigned long crc;	    /* FIXME: this is not 32-bits on all platforms! */
+ #endif /* OSF1V4D */
  
      crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
      for (p = buf; len > 0; ++p, --len)
  	crc = ((crc << 8) ^ crc32_table[(crc >> 24) ^ *p]) & 0xffffffff;
  
!     return (unsigned long) ~crc;    /* transmit complement, per CRC-32 spec */
  }
