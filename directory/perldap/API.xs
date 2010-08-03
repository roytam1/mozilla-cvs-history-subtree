/******************************************************************************
 * $Id$
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is PerLDAP. 
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation. and Clayton Donley.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   - Leif Hedstrom <leif@perldap.org>
 *   - Kevin McCarthy
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * DESCRIPTION
 *    This is the XSUB interface for the API.
 */
 
#ifdef __cplusplus
extern "C" {
#endif

/* Perl Include Files */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
}
#endif

/* LDAP C SDK Include Files */
#include <lber.h>
#include <ldap.h>

/* need to define ber types for old version of api */
#if LDAP_VENDOR_VERSION < 600
typedef int my_chgtype_t;
typedef long my_chgnum_t;
typedef unsigned long my_result_t;
typedef unsigned long my_vlvint_t;
#else
typedef ber_int_t my_chgtype_t;
typedef ber_int_t my_chgnum_t;
typedef ber_int_t my_result_t;
typedef ber_int_t my_vlvint_t;
#endif /* LDAP_VENDOR_VERSION < 600 */

/* SSL is only available in Binary */
#ifdef USE_SSL
#ifndef USE_OPENLDAP
# include <ldap_ssl.h>
#endif
#endif

/* AUTOLOAD methods for LDAP constants */
#include "constant.h"

/* This stuff is needed when using OpenLDAP.  Most of it is taken
 * from the Mozilla LDAP C SDK. */
#ifdef USE_OPENLDAP
#define LDAP_C
#define LDAP_CALL
#define LDAP_CALLBACK

/* Stuff for sorting entries */
typedef int (LDAP_C LDAP_CALLBACK LDAP_CMP_CALLBACK)(const char *val1, const char *val2);

typedef int (LDAP_C LDAP_CALLBACK LDAP_CHARCMP_CALLBACK)(char*, char*);

static LDAP_CHARCMP_CALLBACK *perldap_et_cmp_fn;

struct entrything {
        char            **et_vals;
        LDAPMessage     *et_msg;
};

typedef LDAPSortKey LDAPsortkey;

#define LDAP_TAG_SK_MATCHRULE   0x80L   /* context specific + primitive + 0 */
#define LDAP_TAG_SK_REVERSE     0x81L   /* context specific + primitive + 1 */

/* VLV structure */
typedef struct ldapvirtuallist {
    ber_int_t   ldvlist_before_count;       /* # entries before target */
    ber_int_t   ldvlist_after_count;        /* # entries after target */
    char        *ldvlist_attrvalue;         /* jump to this value */
    ber_int_t   ldvlist_index;              /* list offset */
    ber_int_t   ldvlist_size;               /* number of items in vlist */
    void        *ldvlist_extradata;         /* for use by application */
} LDAPVirtualList;

/* We don't really need/use these, but we need to define them
 * to something to prevent compiler errors. */
typedef char FriendlyMap;
typedef char LDAPFiltDesc;
typedef char LDAPFiltInfo;
typedef char LDAPMemCache;
typedef char LDAPVersion;
#endif

/* Prototypes */
static int perldap_init();

static void * perldap_malloc(size_t size);

static void * perldap_calloc(size_t number, size_t size);

static void * perldap_realloc(void *ptr, size_t size);

static void perldap_free(void *ptr);

static int perldap_et_cmp(const void  *aa, const void  *bb);

static void perldap_ldap_value_free(char **vals);

static int perldap_ldap_create_persistentsearch_control(LDAP *ld, int changetypes,
    int changesonly, int return_echg_ctrls, char ctrl_iscritical, LDAPControl **ctrlp);

static int perldap_ldap_create_sort_control(LDAP *ld, LDAPsortkey **sortKeyList,
                                            char ctrl_iscritical, LDAPControl **ctrlp);

static int perldap_ldap_create_virtuallist_control(LDAP *ld, LDAPVirtualList *ldvlistp,
                                                   LDAPControl **ctrlp);

static int perldap_ldap_multisort_entries(LDAP *ld, LDAPMessage **chain,
                                          char **attrs, LDAP_CMP_CALLBACK *cmp);

static int perldap_ldap_parse_entrychange_control(LDAP *ld, LDAPControl **ctrls,
                   ber_int_t *chgtypep, char **prevdnp, int *chgnumpresentp, ber_int_t *chgnump);

static int perldap_ldap_parse_sort_control(LDAP *ld, LDAPControl **ctrls,
                                      ber_int_t *result, char **attribute);

static int perldap_ldap_parse_virtuallist_control(LDAP *ld, LDAPControl **ctrls,
                    ber_int_t *target_posp, ber_int_t *list_sizep, int *errcodep);

static void perldap_ldap_perror(LDAP *ld, const char *s);

static int perldap_ldap_set_lderrno(LDAP *ld, int e, char *m, char *s);

static int perldap_ldap_sort_entries(LDAP *ld, LDAPMessage **chain, char *attr,
                                     LDAP_CMP_CALLBACK *cmp);

static int perldap_ldap_url_search(LDAP *ld, char *url, int attrsonly);

static int perldap_ldap_url_search_s(LDAP *ld, char *url, int attrsonly, LDAPMessage **res);

static int perldap_ldap_url_search_st(LDAP *ld, char *url, int attrsonly,
                                      struct timeval *timeout, LDAPMessage **res);

static int perldap_ldapssl_client_init(const char *certdbpath, void *certdbhandle);

static int perldap_ldapssl_enable_clientauth(LDAP *ld, char *keynickname,
                                  char *keypasswd, char *certnickname);

static LDAP * perldap_ldapssl_init(const char *host, const char *port, int secure);

static char ** avref2charptrptr(SV *avref);

static struct berval ** avref2berptrptr(SV *avref);

static SV* charptrptr2avref(char **cppval);

static SV* berptrptr2avref(struct berval **bval);

static LDAPMod *parse1mod(SV *ldap_value_ref,char *ldap_current_attribute,
                int ldap_add_func,int cont);

static int calc_mod_size(HV *ldap_change);

static LDAPMod **hash2mod(SV *ldap_change_ref,int ldap_add_func,const char *func);

static int StrCaseCmp(const char *s, const char *t);

static char * StrDup(const char *source);

#ifdef USE_OPENLDAP
static int LDAP_CALL internal_rebind_proc(LDAP *ld, LDAP_CONST char *url,
            ber_tag_t request, ber_int_t msgid, void *arg);

static int LDAP_CALL ldap_default_rebind_proc(LDAP *ld, LDAP_CONST char *url,
            ber_tag_t request, ber_int_t msgid, void *arg);
#else
static int LDAP_CALL internal_rebind_proc(LDAP *ld,char **dnp,char **pwp,
            int *authmethodp,int freeit,void *arg);

static int LDAP_CALL ldap_default_rebind_proc(LDAP *ld, char **dn, char **pswd,
            int *auth, int freeit, void *arg);
#endif


/* Global Definitions and Variables */
SV *ldap_perl_rebindproc = NULL;

static char *ldap_default_rebind_dn = NULL;
static char *ldap_default_rebind_pwd = NULL;
static int ldap_default_rebind_auth = LDAP_AUTH_SIMPLE;

SV *ldap_perl_sortcmp = NULL;

/* Return a Perl List from a char ** in PPCODE */
#define RET_CPP(cppvar) \
	   int cppindex; \
	   if (cppvar) { \
	   for (cppindex = 0; cppvar[cppindex] != NULL; cppindex++) \
	   { \
	      EXTEND(sp,1); \
	      PUSHs(sv_2mortal(newSVpv(cppvar[cppindex],strlen(cppvar[cppindex])))); \
	   } \
	   perldap_ldap_value_free(cppvar); }

/* Return a Perl List from a berval ** in PPCODE */
#define RET_BVPP(bvppvar) \
	   int bvppindex; \
	   if (bvppvar) { \
	   for (bvppindex = 0; bvppvar[bvppindex] != NULL; bvppindex++) \
	   { \
	      EXTEND(sp,1); \
	      PUSHs(sv_2mortal(newSVpv(bvppvar[bvppindex]->bv_val,bvppvar[bvppindex]->bv_len))); \
	   } \
	   ldap_value_free_len(bvppvar); }


/*
 * Function Definition
 */

static
int
perldap_init()
{
#ifdef USE_OPENLDAP
   /* OpenLDAP doesn't allow us to set our
    * own memory allocation functions. */
   return LDAP_OPT_SUCCESS;
#else
   struct ldap_memalloc_fns memalloc_fns;

   memalloc_fns.ldapmem_malloc = perldap_malloc;
   memalloc_fns.ldapmem_calloc = perldap_calloc;
   memalloc_fns.ldapmem_realloc = perldap_realloc;
   memalloc_fns.ldapmem_free = perldap_free;

   return (ldap_set_option(NULL,
                           LDAP_OPT_MEMALLOC_FN_PTRS,
                           &memalloc_fns));
#endif
}


static
void *
perldap_malloc(size_t size)
{
   void *new_ptr;

   New(1, new_ptr, size, char);

   return (new_ptr);
}

static
void *
perldap_calloc(size_t number, size_t size)
{
   void *new_ptr;

   Newz(1, new_ptr, (number*size), char);

   return (new_ptr);
}

static
void *
perldap_realloc(void *ptr, size_t size)
{
   Renew(ptr, size, char);

   return (ptr);
}

static
void
perldap_free(void *ptr)
{
   Safefree(ptr);
}

/* Helpers for using different LDAP libraries */
static
void
perldap_ldap_value_free(char **vals)
{
#ifdef USE_OPENLDAP
   char    **a;

    if ( vals == NULL ) {
        return;
    }

    for (a = vals; *a != NULL; a++)
    {
        char *tmp= *a;
        ldap_memfree((void *)tmp);
    }
    ldap_memfree((void *)vals);
#else
   ldap_value_free(vals);
#endif
}

static
int
perldap_ldap_create_persistentsearch_control(LDAP *ld, int changetypes,
    int changesonly, int return_echg_ctrls, char ctrl_iscritical, LDAPControl **ctrlp)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    BerElement *        ber = NULL;
    struct berval *     berval = NULL;

    if (ctrlp == NULL || (changetypes & ~(LDAP_CONTROL_PERSIST_ENTRY_CHANGE_ADD |
        LDAP_CONTROL_PERSIST_ENTRY_CHANGE_DELETE | LDAP_CONTROL_PERSIST_ENTRY_CHANGE_MODIFY |
        LDAP_CONTROL_PERSIST_ENTRY_CHANGE_RENAME)) != 0) {
        ret = LDAP_PARAM_ERROR;
        goto bail;
    }

    if ((ber = ber_alloc_t(LBER_USE_DER)) == NULL) {
        ret = LDAP_NO_MEMORY;
        goto bail;
    }

    if (ber_printf(ber, "{ibb}", changetypes, changesonly, return_echg_ctrls) == -1) {
        ret = LDAP_ENCODING_ERROR;
        goto bail;
    }

    if (ber_flatten(ber, &berval) != LDAP_SUCCESS) {
        ret = LDAP_NO_MEMORY;
        goto bail;
    }

    ret = ldap_control_create(LDAP_CONTROL_PERSIST_REQUEST,
                              ctrl_iscritical, berval, 1, ctrlp);

bail:
    ber_free(ber, 1);
    ber_bvfree(berval);
    ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
    ret = ldap_create_persistentsearch_control(ld, changetypes, changesonly,
                                  return_echg_ctrls, ctrl_iscritical, ctrlp);
#endif

    return ret;
}

static
int
perldap_ldap_create_sort_control(LDAP *ld, LDAPsortkey **sortKeyList,
        char ctrl_iscritical, LDAPControl **ctrlp)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    BerElement *        ber = NULL;
    struct berval *     berval = NULL;
    int i = 0;

    if ( sortKeyList == NULL || ctrlp == NULL ) {
        ret = LDAP_PARAM_ERROR;
        goto bail;
    }

    if ((ber = ber_alloc_t(LBER_USE_DER)) == NULL) {
        ret = LDAP_NO_MEMORY;
        goto bail;
    }

    if (ber_printf(ber, "{") == -1) {
        ret = LDAP_ENCODING_ERROR;
        goto bail;
    }

    for(i = 0; sortKeyList[i] != NULL; i++) {
        if (ber_printf(ber, "{s", (sortKeyList[i])->attributeType) == -1) {
            ret = LDAP_ENCODING_ERROR;
            goto bail;
        }

        if ((sortKeyList[i])->orderingRule != NULL ) {
            if (ber_printf(ber, "ts", LDAP_TAG_SK_MATCHRULE,
                            (sortKeyList[i])->orderingRule) == -1) {
                ret = LDAP_ENCODING_ERROR;
                goto bail;
            }
        }

        if ((sortKeyList[i])->reverseOrder) {
            if (ber_printf(ber, "tb}", LDAP_TAG_SK_REVERSE,
                           (sortKeyList[i])->reverseOrder) == -1) {
                ret = LDAP_ENCODING_ERROR;
                goto bail;
            }
        } else {
            if (ber_printf( ber, "}") == -1) {
                ret = LDAP_ENCODING_ERROR;
                goto bail;
            }
        }
    }

    if (ber_printf(ber, "}") == -1) {
        ret = LDAP_ENCODING_ERROR;
        goto bail;
    }

    if (ber_flatten(ber, &berval) != LDAP_SUCCESS) {
        ret = LDAP_NO_MEMORY;
        goto bail;
    }

    ret = ldap_control_create(LDAP_CONTROL_SORTREQUEST,
                              ctrl_iscritical, berval, 1, ctrlp);

bail:
    ber_free(ber, 1);
    ber_bvfree(berval);
    ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
    ret = ldap_create_sort_control(ld, sortKeyList, ctrl_iscritical, ctrlp);
#endif

    return ret;
}

static
int
perldap_ldap_create_virtuallist_control(LDAP *ld, LDAPVirtualList *ldvlistp, LDAPControl **ctrlp)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPVLVInfo ldvlvinfo;
    struct berval attrvalue;

    ldvlvinfo.ldvlv_before_count = ldvlistp->ldvlist_before_count;
    ldvlvinfo.ldvlv_after_count = ldvlistp->ldvlist_after_count;
    ldvlvinfo.ldvlv_offset = ldvlistp->ldvlist_index;
    ldvlvinfo.ldvlv_count = ldvlistp->ldvlist_size;

    if (ldvlistp->ldvlist_attrvalue) {
        attrvalue.bv_val = ldvlistp->ldvlist_attrvalue;
        attrvalue.bv_len = strlen(ldvlistp->ldvlist_attrvalue);
        ldvlvinfo.ldvlv_attrvalue = &attrvalue;
    }

    ret = ldap_create_vlv_control(ld, &ldvlvinfo, ctrlp);
#else
    ret = ldap_create_virtuallist_control(ld, ldvlistp, ctrlp);
#endif

    return ret;
}

static
int
perldap_ldap_multisort_entries(LDAP *ld, LDAPMessage **chain,
                          char **attrs, LDAP_CMP_CALLBACK *cmp)
{
/* OpenLDAP doesn't implement any sort functions.  This code is
 * largely borrowed from MozLDAP. */
#ifdef USE_OPENLDAP
    int i, count;
    struct entrything *et;
    LDAPMessage *e, *last;
    LDAPMessage **ep;

    if (ld == NULL || chain == NULL || cmp == NULL) {
        return( LDAP_PARAM_ERROR );
    }

    count = ldap_count_entries( ld, *chain );

    if (count < 0) { /* error, usually with bad ld or malloc */
        return( LDAP_PARAM_ERROR );
    }

    if (count < 2) { /* nothing to sort */
        return( 0 );
    }

    if ( (et = (struct entrything *)perldap_malloc( count *
        sizeof(struct entrything) )) == NULL ) {
        perldap_ldap_set_lderrno(ld, LDAP_NO_MEMORY, NULL, NULL);
        return( -1 );
    }

    e = ldap_first_message(ld, *chain);
    for ( i = 0; i < count; i++ ) {
        et[i].et_msg = e;
        et[i].et_vals = NULL;
        if ( attrs == NULL ) {
            char    *dn;

            dn = ldap_get_dn( ld, e );
            et[i].et_vals = ldap_explode_dn( dn, 1 );
            perldap_free( dn );
        } else {
            int     attrcnt;
            struct berval    **bvals;

            for ( attrcnt = 0; attrs[attrcnt] != NULL; attrcnt++ ) {
                bvals = ldap_get_values_len( ld, e, attrs[attrcnt] );
                if (bvals && bvals[0]) {
                    int bv_count = 0;
                    int j = 0;

                    /* Count the number of bvals we have. */
                    for (bv_count = 0; bvals[bv_count]; bv_count++) {
                        ;
                    }

                    /* Advance to the end of et_vals. */
                    for (j = 0; et[i].et_vals[j]; j++) {
                        ;
                    }

                    /* Realloc et_vals so we can append values from bvals. */
                    et[i].et_vals = (char **)perldap_realloc((char *)et[i].et_vals,
                                               sizeof(char *) * (j + bv_count + 1));

                    /* Copy vals from bvals to et_vals. */
                    for (bv_count = 0; bvals[bv_count]; bv_count++) {
                        et[i].et_vals[j + bv_count] = StrDup(bvals[bv_count]->bv_val);
                    }

                    /* Null terminate the array. */
                    et[i].et_vals[j + bv_count] = 0;

                    /* The values form bvals have been copied, so we can free bvals now. */
                    ldap_value_free_len(bvals);
                }
            }
        }

        e = ldap_next_message(ld, e);
    }
    last = e;

    perldap_et_cmp_fn = (LDAP_CHARCMP_CALLBACK *)cmp;
    qsort( (void *) et, (size_t) count,
            (size_t) sizeof(struct entrything), perldap_et_cmp );

    ep = chain;

    for ( i = 0; i < count; i++ ) {
        *ep = et[i].et_msg;
        e = ldap_next_message(ld, *ep);
        ep = &e;

        perldap_ldap_value_free( et[i].et_vals );
    }
    *ep = last;
    perldap_free((char *) et);

    return( 0 );
#else
    return(ldap_multisort_entries(ld, chain, attrs, cmp));
#endif
}

static
int
perldap_ldap_parse_entrychange_control(LDAP *ld, LDAPControl **ctrls, ber_int_t *chgtypep,
                                 char **prevdnp, int *chgnumpresentp, ber_int_t *chgnump)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPControl *entchg_control = NULL;

    if ((entchg_control = ldap_control_find(LDAP_CONTROL_PERSIST_ENTRY_CHANGE_NOTICE,
                                            ctrls, NULL)) != NULL) {
        BerElement *ber = NULL;
        ber_int_t changetype;
        ber_len_t len;
        char *previousdn;

        ber = ber_init(&entchg_control->ldctl_value);
        if (ber == NULL) {
            ret = LDAP_NO_MEMORY;
        }

        /* Get the changetype first. */
        if ( ber_scanf( ber, "{e", &changetype ) == LBER_ERROR ) {
            ber_free( ber, 1 );
            ret = LDAP_DECODING_ERROR;
            goto bail;
        }

        /* If this is a MODDN, get the previous DN. */
        if (changetype == LDAP_CONTROL_PERSIST_ENTRY_CHANGE_RENAME) {
            if ( ber_scanf( ber, "a", &previousdn ) == LBER_ERROR ) {
                ber_free( ber, 1 );
                ret = LDAP_DECODING_ERROR;
                goto bail;
            }
        } else {
            previousdn = NULL;
        }

        /* Set the return pointers for the changetype and previous DN. */
        if ( chgtypep != NULL ) {
            *chgtypep = changetype;
        }
        if ( prevdnp != NULL ) {
            *prevdnp = previousdn;
        } else if ( previousdn != NULL ) {
            ber_memfree((void *)previousdn);
        }

        /* Get the optional changenumber if present. */
        if ( chgnump != NULL ) {
            if ( ber_peek_tag( ber, &len ) == LBER_INTEGER
                 && ber_get_int( ber, chgnump ) != LBER_ERROR ) {
                if ( chgnumpresentp != NULL ) {
                    *chgnumpresentp = 1;
                }
            } else {
                if ( chgnumpresentp != NULL ) {
                    *chgnumpresentp = 0;
                }
            }
        }

        ber_free( ber, 1 );
        ret = LDAP_SUCCESS;
    } else {
        ret = LDAP_CONTROL_NOT_FOUND;
    }

bail:
    ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
    ret = ldap_parse_entrychange_control(ld, ctrls, chgtypep, prevdnp
                                         chgnumpresentp, chgnump);
#endif

    return ret;
}

static
int
perldap_ldap_parse_sort_control(LDAP *ld, LDAPControl **ctrls,
                          ber_int_t *result, char **attribute )
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPControl *sort_ctrl = NULL;

    if ((sort_ctrl = ldap_control_find(LDAP_CONTROL_SORTRESPONSE, ctrls, NULL)) != NULL) {
        ret = ldap_parse_sortresponse_control(ld, sort_ctrl, result, attribute);
    } else {
        ret = LDAP_CONTROL_NOT_FOUND;
    }
#else
    ret = ldap_parse_sort_control(ld, ctrls, result, attribute);
#endif

    return ret;
}

static
int
perldap_ldap_parse_virtuallist_control(LDAP *ld, LDAPControl **ctrls,
          ber_int_t *target_posp, ber_int_t *list_sizep, int *errcodep)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPControl *   vlv_ctrl = NULL;

    if ((vlv_ctrl = ldap_control_find(LDAP_CONTROL_VLVRESPONSE, ctrls, NULL)) != NULL) {
        ret = ldap_parse_vlvresponse_control(ld, vlv_ctrl, target_posp,
                                                list_sizep, NULL, errcodep);
    } else {
        ret = LDAP_CONTROL_NOT_FOUND;
    }
#else
    ret = ldap_parse_virtuallist_control(ld, ctrls, target_posp, list_sizep, errcodep);
#endif

    return ret;
}

static
void
perldap_ldap_perror(LDAP *ld, const char *s)
{
#ifdef USE_OPENLDAP
    char *  err_str = NULL;
    char *  separator;
    int     err;

    if ( s == NULL ) {
        s = separator = "";
    } else {
        separator = ": ";
    }

    ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &err);
    err_str = ldap_err2string(err);
    printf("%s%s%s", s, separator, err_str);
#else
    ldap_perror(ld, s);
#endif
}

static
int
perldap_ldap_set_lderrno(LDAP *ld, int e, char *m, char *s)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &e);
    if (m) {
        ldap_set_option(ld, LDAP_OPT_MATCHED_DN, m);
    }

    if (s) {
#ifdef LDAP_OPT_DIAGNOSTIC_MESSAGE
        ldap_set_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, s);
#else
        ldap_set_option(ld, LDAP_OPT_ERROR_STRING, s);
#endif
    }
#else  /* !USE_OPENLDAP */
    ret = ldap_set_lderrno(ld, e, m, s);
#endif

    return ret;
}

static
int
perldap_ldap_sort_entries(LDAP *ld, LDAPMessage **chain,
                          char *attr, LDAP_CMP_CALLBACK *cmp)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    char    *attrs[2];

    attrs[0] = attr;
    attrs[1] = NULL;

    ret = perldap_ldap_multisort_entries(ld, chain, attr ? attrs : NULL, cmp);
#else
    ret = ldap_sort_entries(ld, chain, attr, cmp);
#endif

    return ret;
}

static
int
perldap_ldap_url_search(LDAP *ld, char *url, int attrsonly)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPURLDesc *url_desc = NULL;
    char *old_urls = NULL;

    /* Get old URLs to preserve them. */
    ldap_get_option(ld, LDAP_OPT_URI, &old_urls);

    /* Parse the search URL. */
    ret = ldap_url_parse(url, &url_desc);
    if (ret == LDAP_SUCCESS) {
        /* Set the host/port from the URL. */
        ret = ldap_set_option(ld, LDAP_OPT_URI, url);
        if (ret == LDAP_OPT_SUCCESS) {
            ldap_search_ext(ld, url_desc->lud_dn, url_desc->lud_scope,
                    url_desc->lud_filter, url_desc->lud_attrs, attrsonly,
                    NULL, NULL, NULL, 0, &ret);
        }
    }

    /* Reset the original URLs. */
    ldap_set_option(ld, LDAP_OPT_URI, old_urls);

    /* Free parsed URL structure. */
    ldap_free_urldesc(url_desc);
#else
    ret = ldap_url_search(ld, url, attrsonly);
#endif

    return ret;
}

static
int
perldap_ldap_url_search_s(LDAP *ld, char *url, int attrsonly, LDAPMessage **res)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPURLDesc *url_desc;
    char *old_urls = NULL;

    /* Get old URLs to preserve them. */
    ldap_get_option(ld, LDAP_OPT_URI, &old_urls);

    /* Parse the search URL. */
    ret = ldap_url_parse(url, &url_desc);
    if (ret == LDAP_SUCCESS) {
        /* Set the host/port from the URL. */
        ret = ldap_set_option(ld, LDAP_OPT_URI, url);
        if (ret == LDAP_OPT_SUCCESS) {
            ret = ldap_search_ext_s(ld, url_desc->lud_dn, url_desc->lud_scope,
                         url_desc->lud_filter, url_desc->lud_attrs, attrsonly,
                         NULL, NULL, NULL, 0, res);
        }
    }

    /* Reset the original URLs. */
    ldap_set_option(ld, LDAP_OPT_URI, old_urls);

    /* Free parsed URL structure. */
    ldap_free_urldesc(url_desc);
#else
    ret = ldap_url_search_s(ld, url, attrsonly, res);
#endif

    return ret;
}

static
int
perldap_ldap_url_search_st(LDAP *ld, char *url, int attrsonly,
                           struct timeval *timeout, LDAPMessage **res)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    LDAPURLDesc *url_desc;
    char *old_urls = NULL;

    /* Get old URLs to preserve them. */
    ldap_get_option(ld, LDAP_OPT_URI, &old_urls);
    
    /* Parse the search URL. */
    ret = ldap_url_parse(url, &url_desc);
    if (ret == LDAP_SUCCESS) {
        /* Set the host/port from the URL. */
        ret = ldap_set_option(ld, LDAP_OPT_URI, url);
        if (ret == LDAP_OPT_SUCCESS) {
            ret = ldap_search_ext_s(ld, url_desc->lud_dn, url_desc->lud_scope,
                         url_desc->lud_filter, url_desc->lud_attrs, attrsonly,
                         NULL, NULL, timeout, 0, res);
        }
    }

    /* Reset the original URLs. */
    ldap_set_option(ld, LDAP_OPT_URI, old_urls);

    /* Free parsed URL structure. */
    ldap_free_urldesc(url_desc);
#else
    ret = ldap_url_search_st(ld, url, attrsonly, timeout, res);
#endif

    return ret;
}

static
int
perldap_ldapssl_client_init(const char *certdbpath, void *certdbhandle)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    ret = ldap_set_option(NULL, LDAP_OPT_X_TLS_CACERTDIR, certdbpath);
#else
    ret = ldapssl_client_init(certdbpath, certdbhandle);
#endif

    return ret;
}

static
int
perldap_ldapssl_enable_clientauth(LDAP *ld, char *keynickname,
                                  char *keypasswd, char *certnickname)
{
    int ret = 0;

#ifdef USE_OPENLDAP
    ret = ldap_set_option(ld, LDAP_OPT_X_TLS_KEYFILE, keynickname);
    if (ret == 0) {
        ret = ldap_set_option(ld, LDAP_OPT_X_TLS_CERTFILE, certnickname);
    }
#else
    ret = ldapssl_enable_clientauth(ld, keynickname, keypasswd, certnickname);
#endif

    return ret;
}

static
LDAP *
perldap_ldapssl_init(const char *host, const char *port, int secure)
{
    LDAP *ret = NULL;

#ifdef USE_OPENLDAP
    char *ldapurl;
    int   ldapurl_len;

    if (host && port) {
        ldapurl_len = strlen(host) + strlen(port) + 11;
        ldapurl = (char *)perldap_malloc(ldapurl_len);
        snprintf(ldapurl, ldapurl_len, "ldap%s://%s:%s/",
                 (secure != 0 ? "s" : ""), host, port);
        ldap_initialize(&ret, ldapurl);
        perldap_free((void *)ldapurl);
    }
#else
    ret = ldapssl_init(host, atoi(port), secure);
#endif

    return ret;
}

#ifdef USE_OPENLDAP

static int
perldap_et_cmp( 
    const void  *aa,
    const void  *bb
)
{       
        int                     i, rc;
        struct entrything       *a = (struct entrything *)aa;
        struct entrything       *b = (struct entrything *)bb;

        if ( a->et_vals == NULL && b->et_vals == NULL )
                return( 0 );
        if ( a->et_vals == NULL )
                return( -1 );
        if ( b->et_vals == NULL )
                return( 1 );

        for ( i = 0; a->et_vals[i] && b->et_vals[i]; i++ ) {
                if ( (rc = (*perldap_et_cmp_fn)( a->et_vals[i], b->et_vals[i] ))
                    != 0 ) {
                        return( rc );
                }
        }

        if ( a->et_vals[i] == NULL && b->et_vals[i] == NULL )
                return( 0 );
        if ( a->et_vals[i] == NULL )
                return( -1 );
        return( 1 );
}

#endif


/* Return a char ** when passed a reference to an AV */
static
char **
avref2charptrptr(SV *avref)
{
   I32 avref_arraylen;
   int ix_av;
   SV **current_val;
   char **tmp_cpp;

   if ((! SvROK(avref)) ||
       (SvTYPE(SvRV(avref)) != SVt_PVAV) ||
       ((avref_arraylen = av_len((AV *)SvRV(avref))) < 0))
   {
      return NULL;
   }

   Newz(1,tmp_cpp,avref_arraylen+2,char *);
   for (ix_av = 0;ix_av <= avref_arraylen;ix_av++)
   {
      current_val = av_fetch((AV *)SvRV(avref),ix_av,0);
      tmp_cpp[ix_av] = StrDup(SvPV(*current_val,PL_na));
   }
   tmp_cpp[ix_av] = NULL;

   return (tmp_cpp);
}

/* Return a struct berval ** when passed a reference to an AV */
static
struct berval **
avref2berptrptr(SV *avref)
{
   I32 avref_arraylen;
   int ix_av,val_len;
   SV **current_val;
   char *tmp_char,*tmp2;
   struct berval **tmp_ber;

   if ((! SvROK(avref)) ||
       (SvTYPE(SvRV(avref)) != SVt_PVAV) || 
       ((avref_arraylen = av_len((AV *)SvRV(avref))) < 0))
   {
      return NULL;
   }

   Newz(1,tmp_ber,avref_arraylen+2,struct berval *);
   for (ix_av = 0;ix_av <= avref_arraylen;ix_av++)
   {
      New(1,tmp_ber[ix_av],1,struct berval);
      current_val = av_fetch((AV *)SvRV(avref),ix_av,0);

      tmp_char = SvPV(*current_val,PL_na);
      val_len = SvCUR(*current_val);

      Newz(1,tmp2,val_len+1,char);
      Copy(tmp_char,tmp2,val_len,char);

      tmp_ber[ix_av]->bv_val = tmp2;
      tmp_ber[ix_av]->bv_len = val_len;
   }
   tmp_ber[ix_av] = NULL;

   return(tmp_ber);
}

/* Return an AV reference when given a char ** */

static
SV*
charptrptr2avref(char **cppval)
{
   AV* tmp_av = newAV();
   SV* tmp_ref = newRV((SV*)tmp_av);
   int ix;

   if (cppval != NULL)
   {
      for (ix = 0; cppval[ix] != NULL; ix++)
      {
         SV* SVval = newSVpv(cppval[ix],0);
         av_push(tmp_av,SVval);
      }
      perldap_ldap_value_free(cppval);
   }
   return(tmp_ref);
}

/* Return an AV Reference when given a struct berval ** */

static
SV*
berptrptr2avref(struct berval **bval)
{
   AV* tmp_av = newAV();
   SV* tmp_ref = newRV((SV*)tmp_av);
   int ix;

   if (bval != NULL)
   {
      for(ix = 0; bval[ix] != NULL; ix++)
      {
         SV *SVval = newSVpv(bval[ix]->bv_val,bval[ix]->bv_len);
         av_push(tmp_av,SVval);
      }
      ldap_value_free_len(bval);
   }
   return(tmp_ref);
}


/* parse1mod - Take a single reference, figure out if it is a HASH, */
/*   ARRAY, or SCALAR, then extract the values and attributes and   */
/*   return a single LDAPMod pointer to this data.                  */

static
LDAPMod *
parse1mod(SV *ldap_value_ref,char *ldap_current_attribute,
          int ldap_add_func,int cont)
{
   LDAPMod *ldap_current_mod;
   static HV *ldap_current_values_hv;
   HE *ldap_change_element;
   char *ldap_current_modop;
   SV *ldap_current_value_sv;
   I32 keylen;
   int ldap_isa_ber = 0;

   if (ldap_current_attribute == NULL)
      return(NULL);
   Newz(1,ldap_current_mod,1,LDAPMod);
   ldap_current_mod->mod_type = StrDup(ldap_current_attribute);
   if (SvROK(ldap_value_ref))
   {
     if (SvTYPE(SvRV(ldap_value_ref)) == SVt_PVHV)
     {
      if (!cont)
      {
         ldap_current_values_hv = (HV *) SvRV(ldap_value_ref);
         hv_iterinit(ldap_current_values_hv);
      }
      if ((ldap_change_element = hv_iternext(ldap_current_values_hv)) == NULL)
         return(NULL);
      ldap_current_modop = hv_iterkey(ldap_change_element,&keylen);
      ldap_current_value_sv = hv_iterval(ldap_current_values_hv,
        ldap_change_element);
      if (ldap_add_func == 1)
      {
         ldap_current_mod->mod_op = 0;
      } else {
         if (strchr(ldap_current_modop,'a') != NULL)
         {
            ldap_current_mod->mod_op = LDAP_MOD_ADD;
         } else if (strchr(ldap_current_modop,'r') != NULL)
         {
            ldap_current_mod->mod_op = LDAP_MOD_REPLACE;
         } else if (strchr(ldap_current_modop,'d') != NULL) {
            ldap_current_mod->mod_op = LDAP_MOD_DELETE;
         } else {
            return(NULL);
         }
      }
      if (strchr(ldap_current_modop,'b') != NULL)
      {
         ldap_isa_ber = 1;
         ldap_current_mod->mod_op = ldap_current_mod->mod_op | LDAP_MOD_BVALUES;
      }
      if (SvTYPE(SvRV(ldap_current_value_sv)) == SVt_PVAV)
      {
         if (ldap_isa_ber == 1)
         {
            ldap_current_mod->mod_bvalues =
		avref2berptrptr(ldap_current_value_sv);
         } else {
            ldap_current_mod->mod_values =
		avref2charptrptr(ldap_current_value_sv);
         }
      }
     } else if (SvTYPE(SvRV(ldap_value_ref)) == SVt_PVAV) {
      if (cont)
         return NULL;
      if (ldap_add_func == 1)
         ldap_current_mod->mod_op = 0;
      else
         ldap_current_mod->mod_op = LDAP_MOD_REPLACE;
      ldap_current_mod->mod_values = avref2charptrptr(ldap_value_ref);
      if (ldap_current_mod->mod_values == NULL)
      {
         ldap_current_mod->mod_op = LDAP_MOD_DELETE;
      }
     }
   } else {
      if (cont)
         return NULL;
      if (strcmp(SvPV(ldap_value_ref,PL_na),"") == 0)
      {
         if (ldap_add_func != 1)
         {
            ldap_current_mod->mod_op = LDAP_MOD_DELETE;
            ldap_current_mod->mod_values = NULL;
         } else {
            return(NULL);
         }
      } else {
         if (ldap_add_func == 1)
         {
            ldap_current_mod->mod_op = 0;
         } else {
            ldap_current_mod->mod_op = LDAP_MOD_REPLACE;
         }
         New(1,ldap_current_mod->mod_values,2,char *);
         ldap_current_mod->mod_values[0] = StrDup(SvPV(ldap_value_ref,PL_na));
         ldap_current_mod->mod_values[1] = NULL;
      }
   }
   return(ldap_current_mod);
}

/* calc_mod_size                                                           */
/* Calculates the number of LDAPMod's buried inside the ldap_change passed */
/* in.  This is used by hash2mod to calculate the size to allocate in Newz */
static
int
calc_mod_size(HV *ldap_change)
{
   int mod_size = 0;
   HE *ldap_change_element;
   SV *ldap_change_element_value_ref;
   HV *ldap_change_element_value;

   hv_iterinit(ldap_change);

   while((ldap_change_element = hv_iternext(ldap_change)) != NULL)
   {
      ldap_change_element_value_ref = hv_iterval(ldap_change,ldap_change_element);
      /* Hashes can take up multiple mod slots. */
      if ( (SvROK(ldap_change_element_value_ref)) &&
           (SvTYPE(SvRV(ldap_change_element_value_ref)) == SVt_PVHV) )
      {
         ldap_change_element_value = (HV *)SvRV(ldap_change_element_value_ref);
         hv_iterinit(ldap_change_element_value);
         while ( hv_iternext(ldap_change_element_value) != NULL )
         {
            mod_size++;
         }
      }
      /* scalars and array references only take up one mod slot */
      else
      {
         mod_size++;
      }
   }

   return(mod_size);
}


/* hash2mod - Cycle through all the keys in the hash and properly call */
/*    the appropriate functions to build a NULL terminated list of     */
/*    LDAPMod pointers.                                                */

static
LDAPMod **
hash2mod(SV *ldap_change_ref,int ldap_add_func,const char *func)
{
   LDAPMod **ldapmod = NULL;
   LDAPMod *ldap_current_mod;
   int ldap_attribute_count = 0;
   HE *ldap_change_element;
   char *ldap_current_attribute;
   SV *ldap_current_value_sv;
   I32 keylen;
   HV *ldap_change;

   if (!SvROK(ldap_change_ref) || SvTYPE(SvRV(ldap_change_ref)) != SVt_PVHV)
      croak("Mozilla::LDAP::API::%s needs Hash reference as argument 3.",func);

   ldap_change = (HV *)SvRV(ldap_change_ref);

   Newz(1,ldapmod,1+calc_mod_size(ldap_change),LDAPMod *);
   hv_iterinit(ldap_change);
   while((ldap_change_element = hv_iternext(ldap_change)) != NULL)
   {
      ldap_current_attribute = hv_iterkey(ldap_change_element,&keylen);
      ldap_current_value_sv = hv_iterval(ldap_change,ldap_change_element);
      ldap_current_mod = parse1mod(ldap_current_value_sv,
        ldap_current_attribute,ldap_add_func,0);
      while (ldap_current_mod != NULL)
      {
         ldap_attribute_count++;
         ldapmod[ldap_attribute_count-1] = (LDAPMod *)ldap_current_mod;
         ldap_current_mod = parse1mod(ldap_current_value_sv,
           ldap_current_attribute,ldap_add_func,1);

      }
   }
   ldapmod[ldap_attribute_count] = NULL;
   return ldapmod;
}

/* StrCaseCmp - Replacement for strcasecmp, since it doesn't exist on many
   systems, including NT...  */

static
int
StrCaseCmp(const char *s, const char *t)
{
   while (*s && *t && toupper(*s) == toupper(*t))
   {
      s++; t++;
   }
   return(toupper(*s) - toupper(*t));
}

/*
 * StrDup
 *
 * Duplicates a string, but uses the Perl memory allocation
 * routines (so it can be free by the internal routines
 */
static
char *
StrDup(const char *source)
{
   char *dest;
   STRLEN length;

   if ( source == NULL )
      return(NULL);
   length = strlen(source);
   Newz(1,dest,length+1,char);
   Copy(source,dest,length+1,char);

   return(dest);
}

/* internal_rebind_proc - Wrapper to call a PERL rebind process.  We
 * need separate rebind callbacks for MozLDAP and OpenLDAP. */
#ifdef USE_OPENLDAP
static
int
LDAP_CALL
internal_rebind_proc(LDAP *ld, LDAP_CONST char *url,
                     ber_tag_t request, ber_int_t msgid, void *arg)
{
    int ret;
    struct berval         cred;
    int count = 0;
    int authmethod;
    char *pwd;
    char *dn;
    dSP;

    ENTER ;
    SAVETMPS ;
    count = perl_call_sv(ldap_perl_rebindproc,G_ARRAY|G_NOARGS);

    SPAGAIN;

    if (count != 3)
       croak("ldap_perl_rebindproc: Expected DN, PASSWORD, and AUTHTYPE returned.\n");

    authmethod = POPi;
    pwd = StrDup(POPp);
    dn = StrDup(POPp);

    FREETMPS ;
    LEAVE ;

    cred.bv_val = pwd;
    cred.bv_len = strlen(pwd);

    ret = ldap_sasl_bind_s(ld, dn, LDAP_SASL_SIMPLE,
                          &cred, NULL, NULL, NULL);

    if (dn)
    {
       Safefree(dn);
    }
    if (pwd)
    {
       Safefree(pwd);
    }

    return ret;
}

/* NT and internal_rebind_proc hate each other, so they need this... */
static
int
LDAP_CALL
ldap_default_rebind_proc(LDAP *ld, LDAP_CONST char *url,
                         ber_tag_t request, ber_int_t msgid, void *arg)
{
    int ret = 0;
    struct berval cred;

    if (!ldap_default_rebind_dn || !ldap_default_rebind_pwd)
    {
        return LDAP_OPERATIONS_ERROR;
    }

    cred.bv_val = ldap_default_rebind_pwd;
    cred.bv_len = strlen(ldap_default_rebind_pwd);

    ret = ldap_sasl_bind_s(ld, ldap_default_rebind_dn, LDAP_SASL_SIMPLE,
                              &cred, NULL, NULL, NULL);

   return ret;
}
#else  /* !USE_OPENLDAP */ 
static
int
LDAP_CALL
internal_rebind_proc(LDAP *ld, char **dnp, char **pwp,
                     int *authmethodp, int freeit, void *arg)
{
   if (freeit == 0)
   {
      int count = 0;
      dSP;

      ENTER ;
      SAVETMPS ;
      count = perl_call_sv(ldap_perl_rebindproc,G_ARRAY|G_NOARGS);

      SPAGAIN;

      if (count != 3)
         croak("ldap_perl_rebindproc: Expected DN, PASSWORD, and AUTHTYPE returned.\n");

      *authmethodp = POPi;
      *pwp = StrDup(POPp);
      *dnp = StrDup(POPp);

      PUTBACK ;
      FREETMPS ;
      LEAVE ;
   } else {
      if (dnp && *dnp)
      {
         Safefree(*dnp);
      }
      if (pwp && *pwp)
      {
         Safefree(*pwp);
      }
   }
   return(LDAP_SUCCESS);
}

/* NT and internal_rebind_proc hate each other, so they need this... */
static
int
LDAP_CALL
ldap_default_rebind_proc(LDAP *ld, char **dn, char **pwd,
                         int *auth, int freeit, void *arg)
{
  if (!ldap_default_rebind_dn || !ldap_default_rebind_pwd)
    {
      *dn = NULL;
      *pwd = NULL;
      *auth = 0;

      return LDAP_OPERATIONS_ERROR;
    }

  *dn = ldap_default_rebind_dn;
  *pwd = ldap_default_rebind_pwd;
  *auth = ldap_default_rebind_auth;

  return LDAP_SUCCESS;
}
#endif

/* internal_sortcmp_proc - Wrapper to call a PERL cmp function */
static
int
internal_sortcmp_proc(const char *s1, const char *s2)
{
  int count;
  int res;

  dSP;
  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(newSVpv(s1, 0)));
  XPUSHs(sv_2mortal(newSVpv(s2, 0)));
  PUTBACK;

  count = perl_call_sv(ldap_perl_sortcmp, G_SCALAR);
  SPAGAIN;

  if (count != 1)
     croak("ldap_perl_sortcmp: Expected an INT to be returned.\n");

  res = POPi;

  PUTBACK;
  FREETMPS;
  LEAVE;

  return res;
}


MODULE = Mozilla::LDAP::API		PACKAGE = Mozilla::LDAP::API
PROTOTYPES: ENABLE

BOOT:
if ( perldap_init() != 0)
{
   fprintf(stderr, "Error loading Mozilla::LDAP::API: perldap_init failed\n");
   exit(1);
}

double
constant(name,arg)
	char *		name
	int		arg

int
ldap_abandon(ld,msgid)
	LDAP *		ld
	int		msgid
	CODE:
	RETVAL = ldap_abandon_ext(ld, msgid, NULL, NULL);
	OUTPUT:
	RETVAL

int
ldap_abandon_ext(ld,msgid,serverctrls,clientctrls)
	LDAP *		ld
	int		msgid
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls

int
ldap_add(ld,dn,attrs)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
	CODE:
	ldap_add_ext(ld, dn, attrs, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL
	CLEANUP:
	
	if (attrs)
	  ldap_mods_free(attrs, 1);

int
ldap_add_ext(ld,dn,attrs,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp
	CLEANUP:
	if (attrs)
	  ldap_mods_free(attrs, 1);

int
ldap_add_ext_s(ld,dn,attrs,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (attrs)
	  ldap_mods_free(attrs, 1);

int
ldap_add_s(ld,dn,attrs)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
	CODE:
	RETVAL = ldap_add_ext_s(ld, dn, attrs, NULL, NULL);
	OUTPUT:
	RETVAL
	CLEANUP:
	if (attrs)
	  ldap_mods_free(attrs, 1);

void
ldap_ber_free(ber,freebuf)
	BerElement *	ber
	int		freebuf
	CODE:
	{
	   if (ber)
	   {
	      ber_free(ber, freebuf);
	   }
	}

int
ldap_bind(ld,dn,passwd,authmethod)
	LDAP *		ld
	const char *	dn
	char *		passwd
	PREINIT:
	struct berval	cred;
	CODE:
	cred.bv_val = passwd;
	cred.bv_len = ( passwd == NULL ) ? 0 : strlen( passwd );
	ldap_sasl_bind(ld, dn, LDAP_SASL_SIMPLE,
	               &cred, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_bind_s(ld,dn,passwd,authmethod)
	LDAP *		ld
	const char *	dn
	char *		passwd
	PREINIT:
	struct berval	cred;
	CODE:
	cred.bv_val = passwd;
	cred.bv_len = ( passwd == NULL ) ? 0 : strlen( passwd );
	RETVAL = ldap_sasl_bind_s(ld, dn, LDAP_SASL_SIMPLE,
	                          &cred, NULL, NULL, NULL);
	OUTPUT:
	RETVAL

int
ldap_compare(ld,dn,attr,value)
	LDAP *		ld
	const char *	dn
	const char *	attr
	char *		value
	PREINIT:
	struct berval	bvalue;
	CODE:
	bvalue.bv_val = value;
	bvalue.bv_len = ( value == NULL ) ? 0 : strlen( value );
	ldap_compare_ext(ld, dn, attr, &bvalue, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_compare_ext(ld,dn,attr,bvalue,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	const char *	attr
	struct berval 	&bvalue
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int 		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp

int
ldap_compare_ext_s(ld,dn,attr,bvalue,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	const char *	attr
	struct berval 	&bvalue
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls

int
ldap_compare_s(ld,dn,attr,value)
	LDAP *		ld
	const char *	dn
	const char *	attr
	char *		value
	PREINIT:
	struct berval   bvalue;
	CODE:
	bvalue.bv_val = value;
	bvalue.bv_len = ( value == NULL ) ? 0 : strlen( value );
	RETVAL = ldap_compare_ext_s(ld, dn, attr, &bvalue, NULL, NULL);
	OUTPUT:
	RETVAL

void
ldap_control_free(ctrl)
	LDAPControl *	ctrl

#ifdef CONTROLS_COUNT_WORKS
int
ldap_controls_count(ctrls)
	LDAPControl **	ctrls

#endif

void
ldap_controls_free(ctrls)
	LDAPControl **	ctrls

int
ldap_count_entries(ld,result)
	LDAP *		ld
	LDAPMessage *	result

int
ldap_count_messages(ld,result)
	LDAP *		ld
	LDAPMessage *	result

int
ldap_count_references(ld,result)
	LDAP *		ld
	LDAPMessage *	result

int
ldap_create_filter(buf,buflen,pattern,prefix,suffix,attr,value,valwords)
	char *		buf
	unsigned long	buflen
	char *		pattern
	char *		prefix
	char *		suffix
	char *		attr
	char *		value
	char **		valwords
	CODE:
	/* This is a not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = LDAP_NOT_SUPPORTED;
#else
	RETVAL = ldap_create_filter(buf, buflen, pattern, prefix,
	                            suffix, attr, value, valwords);
#endif
	OUTPUT:
	RETVAL
	buf
	CLEANUP:
	if (valwords)
	  perldap_ldap_value_free(valwords);

int
ldap_create_persistentsearch_control(ld,changetypes,changesonly,return_echg_ctrls,ctrl_iscritical,ctrlp)
	LDAP *		ld
	int		changetypes
	int		changesonly
	int		return_echg_ctrls
	char		ctrl_iscritical
	LDAPControl **	ctrlp = NO_INIT
	CODE:
	RETVAL = perldap_ldap_create_persistentsearch_control(ld, changetypes, changesonly,
	                                         return_echg_ctrls, ctrl_iscritical, ctrlp);
	OUTPUT:
	RETVAL
	ctrlp

int
ldap_create_sort_control(ld,sortKeyList,ctrl_iscritical,ctrlp)
	LDAP *		ld
	LDAPsortkey **	sortKeyList
	char		ctrl_iscritical
	LDAPControl **	ctrlp = NO_INIT
	CODE:
	RETVAL = perldap_ldap_create_sort_control(ld, sortKeyList, ctrl_iscritical, ctrlp);
	OUTPUT:
	RETVAL
	ctrlp

int
ldap_create_sort_keylist(sortKeyList,string_rep)
	LDAPsortkey **	&sortKeyList = NO_INIT
	char *		string_rep
	OUTPUT:
	RETVAL
	sortKeyList

int
ldap_create_virtuallist_control(ld,ldvlistp,ctrlp)
	LDAP *		ld
	LDAPVirtualList	*ldvlistp
	LDAPControl **	ctrlp = NO_INIT
	CODE:
	RETVAL = perldap_ldap_create_virtuallist_control(ld, ldvlistp, ctrlp);
	OUTPUT:
	RETVAL
	ctrlp

int
ldap_delete(ld,dn)
	LDAP *		ld
	const char *	dn
	CODE:
	ldap_delete_ext(ld, dn, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_delete_ext(ld,dn,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp

int
ldap_delete_ext_s(ld,dn,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls

int
ldap_delete_s(ld,dn)
	LDAP *		ld
	const char *	dn
	CODE:
	RETVAL = ldap_delete_ext_s(ld, dn, NULL, NULL);
	OUTPUT:
	RETVAL

char *
ldap_dn2ufn(dn)
	const char *	dn

char *
ldap_err2string(err)
	int err

void
ldap_explode_dn(dn,notypes)
	const char *	dn
	const int	notypes
	PPCODE:
	{
	   char **MOZLDAP_VAL = ldap_explode_dn(dn,notypes);
	   RET_CPP(MOZLDAP_VAL);
	}

void
ldap_explode_rdn(dn,notypes)
	const char *	dn
	int		notypes
	PPCODE:
	{
	   char **MOZLDAP_VAL = ldap_explode_rdn(dn,notypes);
	   RET_CPP(MOZLDAP_VAL);
	}

int
ldap_extended_operation(ld,requestoid,requestdata,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	requestoid
	struct berval 	&requestdata
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp

int
ldap_extended_operation_s(ld,requestoid,requestdata,serverctrls,clientctrls,retoidp,retdatap)
	LDAP *		ld
	const char *	requestoid
	struct berval 	&requestdata
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	char *		&retoidp = NO_INIT
	struct berval *&retdatap = NO_INIT
	OUTPUT:
	RETVAL
	retoidp
	retdatap

char *
ldap_first_attribute(ld,entry,ber)
	LDAP *		ld
	LDAPMessage *	entry
	BerElement *	&ber = NO_INIT
	OUTPUT:
	RETVAL
	ber
	CLEANUP:
	ldap_memfree(RETVAL);


LDAPMessage *
ldap_first_entry(ld,chain)
	LDAP *		ld
	LDAPMessage *	chain

LDAPMessage *
ldap_first_message(ld,res)
	LDAP *		ld
	LDAPMessage *	res

LDAPMessage *
ldap_first_reference(ld,res)
	LDAP *		ld
	LDAPMessage *	res

void
ldap_free_friendlymap(map)
	FriendlyMap *	map
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifndef USE_OPENLDAP
	ldap_free_friendlymap(map);
#endif

void
ldap_free_sort_keylist(sortKeyList)
	LDAPsortkey **	sortKeyList

void
ldap_free_urldesc(ludp)
	LDAPURLDesc *	ludp


char *
ldap_friendly_name(filename,name,map)
	char *		filename
	char *		name
	FriendlyMap *	map
	CODE:
	/* This is not implemented when using OpenLDAP.  We just return
	 * the original "un-friendly" name as MozLDAP does when there is
	 * an error. */
#ifdef USE_OPENLDAP
	RETVAL = name;
#else
	RETVAL = ldap_friendly_name(filename, name, map);
#endif
	OUTPUT:
	RETVAL

char *
ldap_get_dn(ld,entry)
	LDAP *		ld
	LDAPMessage *	entry
	CLEANUP:
	ldap_memfree(RETVAL);

int
ldap_get_entry_controls(ld,entry,serverctrlsp)
	LDAP *		ld
	LDAPMessage *	entry
	LDAPControl **	&serverctrlsp = NO_INIT
	OUTPUT:
	RETVAL
	serverctrlsp

void
ldap_getfilter_free(lfdp)
	LDAPFiltDesc *	lfdp
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifndef USE_OPENLDAP
	ldap_getfilter_free(lfdp);
#endif

LDAPFiltInfo *
ldap_getfirstfilter(lfdp,tagpat,value)
	LDAPFiltDesc *	lfdp
	char *		tagpat
	char *		value	
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = NULL;
#else
	RETVAL = ldap_getfirstfilter(lfdp, tagpat, value);
#endif
	OUTPUT:
	RETVAL

void
ldap_get_lang_values(ld,entry,target,type)
	LDAP *		ld
	LDAPMessage *	entry
	const char *	target
	char *		type
	PPCODE:
	{
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	   int ret = LDAP_NOT_SUPPORTED;
	   char ** MOZLDAP_VAL = NULL;

	   ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
	   char ** MOZLDAP_VAL = ldap_get_lang_values(ld,entry,target,&type);
#endif
	   RET_CPP(MOZLDAP_VAL);
	}

void
ldap_get_lang_values_len(ld,entry,target,type)
	LDAP *		ld
	LDAPMessage *	entry
	const char *	target
	char *		type
	PPCODE:
	{
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	   int ret = LDAP_NOT_SUPPORTED;
	   struct berval ** MOZLDAP_VAL = NULL;

	   ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
	   struct berval ** MOZLDAP_VAL = 
	       ldap_get_lang_values_len(ld,entry,target,&type);
#endif
	   RET_BVPP(MOZLDAP_VAL);
	}

int
ldap_get_lderrno(ld, ...)
	LDAP *		ld
	CODE:
	{
	   char *match = (char *)NULL, *msg = (char *)NULL;
           SV *tmp, *m = (SV *)NULL, *s = (SV *)NULL;

	   if (items > 1)
	   {
	      m = ST(1);
	      if (items > 2)
	         s = ST(2);
           }
#ifdef USE_OPENLDAP
	   ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &RETVAL);
	   if (m && SvROK(m)) {
	      ldap_get_option(ld, LDAP_OPT_MATCHED_DN, &match);
	   }
	   if (s && SvROK(s)) {
#ifdef LDAP_OPT_DIAGNOSTIC_MESSAGE
	      ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, &msg);
#else
	      ldap_get_option(ld, LDAP_OPT_ERROR_STRING, &msg);
#endif
	   }
#else  /* !USE_OPENLDAP */
	   RETVAL = ldap_get_lderrno(ld, (m && SvROK(m)) ? &match : (char **)NULL,
	                                 (s && SvROK(s)) ? &msg : (char **)NULL);
#endif

	   if (match)
	   {
	      tmp = SvRV(m);
	      if (SvTYPE(tmp) <= SVt_PV)
	         sv_setpv(tmp, match);
	   }
	   if (msg)
	   {
	      tmp = SvRV(s);
	      if (SvTYPE(tmp) <= SVt_PV)
	         sv_setpv(tmp, msg);
	   }

	}
	OUTPUT:
	RETVAL

LDAPFiltInfo *
ldap_getnextfilter(lfdp)
	LDAPFiltDesc *lfdp
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = NULL;
#else
	RETVAL = ldap_getnextfilter(lfdp);
#endif
	OUTPUT:
	RETVAL

int
ldap_get_option(ld,option,optdata)
	LDAP *		ld
	int		option
	int		&optdata = NO_INIT
	OUTPUT:
	RETVAL
	optdata

void
ldap_get_values(ld,entry,target)
	LDAP *		ld
	LDAPMessage *	entry
	const char *	target
	PPCODE:
	{
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	   int ret = LDAP_NOT_SUPPORTED;
	   char **MOZLDAP_VAL = NULL;

	   ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &ret);
#else
	   char **MOZLDAP_VAL = ldap_get_values(ld,entry,target);
#endif
	   RET_CPP(MOZLDAP_VAL);
	}

void
ldap_get_values_len(ld,entry,target)
	LDAP *		ld
	LDAPMessage *	entry
	const char *	target
	PPCODE:
	{
	   struct berval **MOZLDAP_VAL = ldap_get_values_len(ld,entry,target);
	   RET_BVPP(MOZLDAP_VAL);
	}

LDAP *
ldap_init(host,port)
	const char *	host
	const char *	port
	PREINIT:
	char *  url = NULL;
	int     url_len = 0;
	CODE:
	RETVAL = NULL;
	/* Create the LDAP URL */
	if (host && port) {
	   url_len = strlen(host) + strlen(port) + 10;
	   url = (char *)perldap_malloc(url_len);
	   snprintf(url, url_len, "ldap://%s:%s/", host, port);
	   ldap_initialize(&RETVAL, url);
	   perldap_free((void *)url);
	}
	OUTPUT:
	RETVAL

LDAPFiltDesc *
ldap_init_getfilter(fname)
	char *		fname
	CODE:
#ifdef USE_OPENLDAP
	RETVAL = NULL;
#else
	RETVAL = ldap_init_getfilter(fname);
#endif
	OUTPUT:
	RETVAL

LDAPFiltDesc *
ldap_init_getfilter_buf(buf,buflen)
	char *		buf
	long		buflen
	CODE:
#ifdef USE_OPENLDAP
	RETVAL = NULL;
#else
	RETVAL = ldap_init_getfilter_buf(buf,buflen);
#endif
	OUTPUT:
	RETVAL

int
ldap_is_ldap_url(url)
	char *		url

void
ldap_memcache_destroy(cache)
	LDAPMemCache *	cache
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifndef USE_OPENLDAP
	ldap_memcache_destroy(cache);
#endif

void
ldap_memcache_flush(cache,dn,scope)
	LDAPMemCache *	cache
	char *		dn
	int		scope
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifndef USE_OPENLDAP
	ldap_memcache_flush(cache, dn, scope);
#endif

int
ldap_memcache_get(ld,cachep)
	LDAP *		ld
	LDAPMemCache **	cachep = NO_INIT
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = LDAP_NOT_SUPPORTED;
#else
	RETVAL = ldap_memcache_get(ld, cachep);
#endif
	OUTPUT:
	RETVAL
	cachep

int
ldap_memcache_init(ttl,size,baseDNs,cachep)
	unsigned long	ttl
	unsigned long	size
	char **		baseDNs
	LDAPMemCache **	cachep = NO_INIT
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = LDAP_NOT_SUPPORTED;
#else
	RETVAL = ldap_memcache_init(ttl,size,baseDNs,NULL,cachep);
#endif
	OUTPUT:
	RETVAL
	cachep
	CLEANUP:
	if (baseDNs)
	  perldap_ldap_value_free(baseDNs);

int
ldap_memcache_set(ld,cache)
	LDAP *		ld
	LDAPMemCache *	cache
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = LDAP_NOT_SUPPORTED;
#else
	RETVAL = ldap_memcache_set(ld, cache);
#endif
	OUTPUT:
	RETVAL


void
ldap_memcache_update(cache)
	LDAPMemCache *	cache
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifndef USE_OPENLDAP
	ldap_memcache_update(cache);
#endif

void
ldap_memfree(p)
	void *		p

int
ldap_modify(ld,dn,mods)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg,0,"$func_name");
	CODE:
	ldap_modify_ext(ld, dn, mods, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

int
ldap_modify_ext(ld,dn,mods,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg,0,"$func_name");
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp
	OUTPUT:
	RETVAL
	msgidp
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

int
ldap_modify_ext_s(ld,dn,mods,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg,0,"$func_name");
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

int
ldap_modify_s(ld,dn,mods)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg, 0, "$func_name");
	CODE:
	RETVAL = ldap_modify_ext_s(ld, dn, mods, NULL, NULL);
	OUTPUT:
	RETVAL
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

int
ldap_modrdn(ld,dn,newrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	CODE:
	ldap_rename(ld, dn, newrdn, NULL, 1, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_modrdn_s(ld,dn,newrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	CODE:
	RETVAL = ldap_rename_s(ld, dn, newrdn, NULL, 1, NULL, NULL);
	OUTPUT:
	RETVAL

int
ldap_modrdn2(ld,dn,newrdn,deleteoldrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	int		deleteoldrdn
	CODE:
	ldap_rename(ld, dn, newrdn, NULL, deleteoldrdn, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_modrdn2_s(ld,dn,newrdn,deleteoldrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	int 		deleteoldrdn
	CODE:
	RETVAL = ldap_rename_s(ld, dn, newrdn, NULL, deleteoldrdn, NULL, NULL);
	OUTPUT:
	RETVAL

void
ldap_mods_free(mods,freemods)
	LDAPMod **	mods
	int		freemods

int
ldap_msgfree(lm)
	LDAPMessage *	lm
	CODE:
	{
	   if (lm)
	   {
	      RETVAL = ldap_msgfree(lm);
	   } else {
	      RETVAL = LDAP_SUCCESS;
	   }
	}
	OUTPUT:
	RETVAL

int
ldap_msgid(lm)
	LDAPMessage *	lm

int
ldap_msgtype(lm)
	LDAPMessage *	lm

int
ldap_multisort_entries(ld,chain,attr,...)
	LDAP *		ld
	LDAPMessage *	&chain
	char **		attr
	CODE:
	{
	   SV		*cmp;
	   LDAP_CMP_CALLBACK		*func = &StrCaseCmp;

	   if (items > 3) {
	      cmp = ST(3);
	      if (SvROK(cmp) &&
	          (SvTYPE(SvRV(cmp)) == SVt_PVCV)) {
		 func = &internal_sortcmp_proc;
		 ldap_perl_sortcmp = cmp;
	      }
	   }
	   RETVAL = perldap_ldap_multisort_entries(ld,&chain,attr,func);
	}
	OUTPUT:
	RETVAL
	chain
	CLEANUP:
	if (attr)
	  perldap_ldap_value_free(attr);

char *
ldap_next_attribute(ld,entry,ber)
	LDAP *		ld
	LDAPMessage *	entry
	BerElement *	ber
	OUTPUT:
	RETVAL
	ber
	CLEANUP:
	ldap_memfree(RETVAL);

LDAPMessage *
ldap_next_entry(ld,entry)
	LDAP *		ld
	LDAPMessage *	entry

LDAPMessage *
ldap_next_message(ld,msg)
	LDAP *		ld
	LDAPMessage *	msg

LDAPMessage *
ldap_next_reference(ld,ref)
	LDAP *		ld
	LDAPMessage *	ref

int
ldap_parse_entrychange_control(ld,ctrls,chgtypep,prevdnp,chgnumpresentp,chgnump)
	LDAP *		ld
	LDAPControl **	ctrls
	my_chgtype_t	&chgtypep = NO_INIT
	char *			&prevdnp = NO_INIT
	int 			&chgnumpresentp = NO_INIT
	my_chgnum_t 	&chgnump = NO_INIT
	CODE:
	RETVAL = perldap_ldap_parse_entrychange_control(ld, ctrls, &chgtypep,
	                                  &prevdnp, &chgnumpresentp, &chgnump);
	OUTPUT:
	RETVAL
	chgtypep
	prevdnp
	chgnumpresentp
	chgnump

int
ldap_parse_extended_result(ld,res,retoidp,retdatap,freeit)
	LDAP *		ld
	LDAPMessage *	res
	char *		&retoidp = NO_INIT
	struct berval *&retdatap = NO_INIT
	int		freeit
	OUTPUT:
	RETVAL
	retoidp
	retdatap

int
ldap_parse_reference(ld,ref,referalsp,serverctrlsp,freeit)
	LDAP *		ld
	LDAPMessage *	ref
	char **		&referalsp = NO_INIT
	LDAPControl **	&serverctrlsp = NO_INIT
	int		freeit
	OUTPUT:
	RETVAL
	referalsp
	serverctrlsp

int
ldap_parse_result(ld,res,errcodep,matcheddnp,errmsgp,referralsp,serverctrlsp,freeit)
	LDAP *		ld
	LDAPMessage *	res
	int 		&errcodep = NO_INIT
	char *		&matcheddnp = NO_INIT
	char *		&errmsgp = NO_INIT
	char **		&referralsp = NO_INIT
	LDAPControl **	&serverctrlsp = NO_INIT
	int		freeit
	OUTPUT:
	RETVAL
	errcodep
	matcheddnp
	errmsgp
	referralsp
	serverctrlsp

int
ldap_parse_sasl_bind_result(ld,res,servercredp,freeit)
	LDAP *		ld
	LDAPMessage *	res
	struct berval * &servercredp
	int freeit

int
ldap_parse_sort_control(ld,ctrls,result,attribute)
	LDAP *			ld
	LDAPControl 	**	ctrls
	my_result_t 	&result = NO_INIT
	char *			&attribute = NO_INIT
	CODE:
	RETVAL = perldap_ldap_parse_sort_control(ld, ctrls, &result, &attribute);
	OUTPUT:
	RETVAL
	result
	attribute

int
ldap_parse_virtuallist_control(ld,ctrls,target_posp,list_sizep,errcodep)
	LDAP *		ld
	LDAPControl **	ctrls
	my_vlvint_t 	&target_posp = NO_INIT
	my_vlvint_t 	&list_sizep = NO_INIT
	int		&errcodep = NO_INIT
	CODE:
	RETVAL = perldap_ldap_parse_virtuallist_control(ld, ctrls, &target_posp,
	                                                &list_sizep, &errcodep);
	OUTPUT:
	RETVAL
	target_posp
	list_sizep
	errcodep

void
ldap_perror(ld,s)
	LDAP *		ld
	const char *	s
	CODE:
	perldap_ldap_perror(ld, s);

int
ldap_rename(ld,dn,newrdn,newparent,deleteoldrdn,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	const char *	newparent
	int		deleteoldrdn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp

int
ldap_rename_s(ld,dn,newrdn,newparent,deleteoldrdn,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	const char *	newparent
	int		deleteoldrdn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls

int
ldap_result(ld,msgid,all,timeout,result)
	LDAP *		ld
	int		msgid
	int		all
	struct timeval	&timeout
	LDAPMessage *	&result = NO_INIT
	OUTPUT:
	RETVAL
	result

int
ldap_result2error(ld,r,freeit)
	LDAP *		ld
	LDAPMessage *	r
	int		freeit
	CODE:
	ldap_parse_result(ld, r, &RETVAL, NULL, NULL, NULL, NULL, freeit);
	OUTPUT:
	RETVAL

int
ldap_sasl_bind(ld,dn,mechanism,cred,serverctrls,clientctrls,msgidp)
	LDAP *		ld
	const char *	dn
	const char *	mechanism
	struct berval 	&cred
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp

int
ldap_sasl_bind_s(ld,dn,mechanism,cred,serverctrls,clientctrls,servercredp)
	LDAP *		ld
	const char *	dn
	const char *	mechanism
	struct berval 	&cred
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	struct berval *&servercredp
	OUTPUT:
	RETVAL
	servercredp

int
ldap_search(ld,base,scope,filter,attrs,attrsonly)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	CODE:
	ldap_search_ext(ld, base, scope, filter, attrs,
	                attrsonly, NULL, NULL, NULL, 0, &RETVAL);
	OUTPUT:
	RETVAL
	CLEANUP:
	if (attrs)
	  perldap_ldap_value_free(attrs);

int
ldap_search_ext(ld,base,scope,filter,attrs,attrsonly,serverctrls,clientctrls,timeoutp,sizelimit,msgidp)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	struct timeval	&timeoutp
	int		sizelimit
	int		&msgidp = NO_INIT
	OUTPUT:
	RETVAL
	msgidp
	CLEANUP:
	if (attrs)
	  perldap_ldap_value_free(attrs);

int
ldap_search_ext_s(ld,base,scope,filter,attrs,attrsonly,serverctrls,clientctrls,timeoutp,sizelimit,res)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	struct timeval	&timeoutp
	int		sizelimit
	LDAPMessage *	&res = NO_INIT
	OUTPUT:
	RETVAL
	res
	CLEANUP:
	if (attrs)
	  perldap_ldap_value_free(attrs);

int
ldap_search_s(ld,base,scope,filter,attrs,attrsonly,res)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	LDAPMessage *	&res = NO_INIT
	CODE:
	RETVAL = ldap_search_ext_s(ld, base, scope, filter, attrs, attrsonly,
	                           NULL, NULL, NULL, 0, &res);
	OUTPUT:
	RETVAL
	res
	CLEANUP:
	if (attrs)
	  perldap_ldap_value_free(attrs);

int
ldap_search_st(ld,base,scope,filter,attrs,attrsonly,timeout,res)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	struct timeval	&timeout
	LDAPMessage *	&res = NO_INIT
	CODE:
	RETVAL = ldap_search_ext_s(ld, base, scope, filter, attrs,
	                           attrsonly, NULL, NULL, &timeout, 0, &res);
	OUTPUT:
	RETVAL
	res
	CLEANUP:
	if (attrs)
	  perldap_ldap_value_free(attrs);
	
int
ldap_set_filter_additions(lfdp,prefix,suffix)
	LDAPFiltDesc *	lfdp
	char *		prefix
	char *		suffix
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = LDAP_NOT_SUPPORTED;
#else
	RETVAL = ldap_set_filter_additions(lfdp, prefix, suffix);
#endif
	OUTPUT:
	RETVAL

int
ldap_set_lderrno(ld,e,m,s)
	LDAP *		ld
	int		e
	char *		m
	char *		s
	CODE:
	RETVAL = perldap_ldap_set_lderrno(ld, e, m, s);
	OUTPUT:
	RETVAL

int
ldap_set_option(ld,option,optdata)
	LDAP *		ld
	int		option
	int		&optdata

void
ldap_set_rebind_proc(ld,rebindproc)
	LDAP *		ld
	SV		*rebindproc
	CODE:
	{
	   if (SvTYPE(SvRV(rebindproc)) != SVt_PVCV)
	   {
	      ldap_set_rebind_proc(ld,NULL,NULL);
	   } else {
	      if (ldap_perl_rebindproc == (SV*)NULL)
	         ldap_perl_rebindproc = newSVsv(rebindproc);
	      else
	         SvSetSV(ldap_perl_rebindproc,rebindproc);
	      ldap_set_rebind_proc(ld,&internal_rebind_proc,NULL);
	   }
	}

void
ldap_set_default_rebind_proc(ld, dn, pwd, auth)
     LDAP *ld
     char *dn
     char *pwd
     int auth
     CODE:
        {
          if ( ldap_default_rebind_dn != NULL )
          {
             Safefree(ldap_default_rebind_dn);
             ldap_default_rebind_dn = NULL;
          }
          if ( ldap_default_rebind_pwd != NULL )
          {
             Safefree(ldap_default_rebind_pwd);
             ldap_default_rebind_pwd = NULL;
          }
          ldap_default_rebind_dn = StrDup(dn);
          ldap_default_rebind_pwd = StrDup(pwd);
          ldap_default_rebind_auth = auth;

          ldap_set_rebind_proc(ld,&ldap_default_rebind_proc,NULL);
        }

int
ldap_simple_bind(ld,who,passwd)
	LDAP *		ld
	const char *	who
	char *		passwd
	PREINIT:
	struct berval	cred;
	CODE:
	cred.bv_val = passwd;
	cred.bv_len = strlen(passwd);

	ldap_sasl_bind(ld, who, LDAP_SASL_SIMPLE,
	               &cred, NULL, NULL, &RETVAL);
	OUTPUT:
	RETVAL

int
ldap_simple_bind_s(ld,who,passwd)
	LDAP *		ld
	const char *	who
	char *		passwd
	PREINIT:
	struct berval	cred;
	CODE:
	cred.bv_val = passwd;
	cred.bv_len = strlen(passwd);

	RETVAL = ldap_sasl_bind_s(ld, who, LDAP_SASL_SIMPLE,
	                          &cred, NULL, NULL, NULL);
	OUTPUT:
	RETVAL

int
ldap_sort_entries(ld,chain,attr,...)
	LDAP *		ld
	LDAPMessage *	&chain
	char *		attr

	CODE:
	{
	   SV		*cmp;
	   LDAP_CMP_CALLBACK		*func = &StrCaseCmp;

	   if (items > 3) {
	      cmp = ST(3);
	      if (SvROK(cmp) &&
	          (SvTYPE(SvRV(cmp)) == SVt_PVCV)) {
	         func = &internal_sortcmp_proc;
	         ldap_perl_sortcmp = cmp;
	      }
	   }
	   RETVAL = perldap_ldap_sort_entries(ld,&chain,attr,func);
	}
	OUTPUT:
	RETVAL
	chain
	
int
ldap_unbind(ld)
	LDAP *		ld
	CODE:
	RETVAL = ldap_unbind_ext(ld, NULL, NULL);
	OUTPUT:
	RETVAL

int
ldap_unbind_s(ld)
	LDAP *		ld
	CODE:
	RETVAL = ldap_unbind_ext_s(ld, NULL, NULL);
	OUTPUT:
	RETVAL

SV *
ldap_url_parse(url)
	char *		url
	CODE:
        {
	   LDAPURLDesc *realcomp;
	   int count,ret;

	   HV*   FullHash = newHV();
	   RETVAL = newRV((SV*)FullHash);

	   ret = ldap_url_parse(url,&realcomp);
	   if (ret == 0)
	   {
	      static char *host_key = "host";
	      static char *port_key = "port";
	      static char *dn_key = "dn";
	      static char *attr_key = "attr";
	      static char *scope_key = "scope";
	      static char *filter_key = "filter";
#ifdef USE_OPENLDAP
              static char *scheme_key = "scheme";
              static char *exts_key = "exts";
              static char *crit_exts_key = "crit_exts";
              SV* scheme = newSVpv(realcomp->lud_scheme,0);
              SV* crit_exts = newSViv(realcomp->lud_crit_exts);
              AV* extsarray = newAV();
              SV* extsref = newRV((SV*) extsarray);
#else
	      static char *options_key = "options";
	      SV* options = newSViv(realcomp->lud_options);
#endif
	      SV* host; /* = newSVpv(realcomp->lud_host,0); */
	      SV* port = newSViv(realcomp->lud_port);
	      SV* dn; /* = newSVpv(realcomp->lud_dn,0); */
	      SV* scope = newSViv(realcomp->lud_scope);
	      SV* filter = newSVpv(realcomp->lud_filter,0);
	      AV* attrarray = newAV();
	      SV* attribref = newRV((SV*) attrarray);

	      if (realcomp->lud_host)
	         host = newSVpv(realcomp->lud_host,0);
	      else
	         host = newSVpv("",0);

	      if (realcomp->lud_dn)
	         dn = newSVpv(realcomp->lud_dn,0);
	      else
	         dn = newSVpv("",0);

	      if (realcomp->lud_attrs != NULL)
	      {
	         for (count=0; realcomp->lud_attrs[count] != NULL; count++)
	         {
	            SV* SVval = newSVpv(realcomp->lud_attrs[count],0);
	            av_push(attrarray, SVval);
	         }
	      }
#ifdef USE_OPENLDAP
	      if (realcomp->lud_exts != NULL)
	      {
	         for (count=0; realcomp->lud_exts[count] != NULL; count++)
	         {
	            SV* SVval = newSVpv(realcomp->lud_exts[count],0);
	            av_push(extsarray, SVval);
	         }
	      }
#endif
	      hv_store(FullHash,host_key,strlen(host_key),host,0);
	      hv_store(FullHash,port_key,strlen(port_key),port,0);
	      hv_store(FullHash,dn_key,strlen(dn_key),dn,0);
	      hv_store(FullHash,attr_key,strlen(attr_key),attribref,0);
	      hv_store(FullHash,scope_key,strlen(scope_key),scope,0);
	      hv_store(FullHash,filter_key,strlen(filter_key),filter,0);
#ifdef USE_OPENLDAP
	      hv_store(FullHash,scheme_key,strlen(scheme_key),scheme,0);
	      hv_store(FullHash,exts_key,strlen(exts_key),extsref,0);
	      hv_store(FullHash,crit_exts_key,strlen(crit_exts_key),crit_exts,0);
#else
	      hv_store(FullHash,options_key,strlen(options_key),options,0);
#endif
	      ldap_free_urldesc(realcomp);
	   } else {
	      RETVAL = &PL_sv_undef;
	   }
	}
	OUTPUT:
	RETVAL

int
ldap_url_search(ld,url,attrsonly)
	LDAP *		ld
	char *		url
	int		attrsonly
	CODE:
	RETVAL = perldap_ldap_url_search(ld, url, attrsonly);
	OUTPUT:
	RETVAL

int
ldap_url_search_s(ld,url,attrsonly,res)
	LDAP *		ld
	char *		url
	int		attrsonly
	LDAPMessage *	&res
	CODE:
	RETVAL = perldap_ldap_url_search_s(ld, url, attrsonly, &res);
	OUTPUT:
	RETVAL
	res

int
ldap_url_search_st(ld,url,attrsonly,timeout,res)
	LDAP *		ld
	char *		url
	int		attrsonly
	struct timeval	&timeout
	LDAPMessage *	&res
	CODE:
	RETVAL = perldap_ldap_url_search_st(ld, url, attrsonly, &timeout, &res);
	OUTPUT:
	RETVAL
	res

int
ldap_version(ver)
	LDAPVersion *	ver
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = 0;
#else
	RETVAL = ldap_version(ver);
#endif
	OUTPUT:
	RETVAL

#ifdef USE_SSL

int
ldapssl_client_init(certdbpath,certdbhandle)
	const char *	certdbpath
	void *		certdbhandle
	CODE:
	RETVAL = perldap_ldapssl_client_init(certdbpath, certdbhandle);
	OUTPUT:
	RETVAL

int
ldapssl_clientauth_init(certdbpath,certdbhandle,needkeydb,keydbpath,keydbhandle)
	char *		certdbpath
	void *		certdbhandle
	int		needkeydb
	char *		keydbpath
	void *		keydbhandle
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = 0;
#else
	RETVAL = ldapssl_clientauth_init(certdbpath,certdbhandle,needkeydb,keydbpath,keydbhandle);
#endif
	OUTPUT:
	RETVAL

int
ldapssl_enable_clientauth(ld,keynickname,keypasswd,certnickname)
	LDAP *		ld
	char *		keynickname
	char *		keypasswd
	char *		certnickname
	CODE:
	RETVAL = perldap_ldapssl_enable_clientauth(ld, keynickname, keypasswd, certnickname);
	OUTPUT:
	RETVAL

LDAP *
ldapssl_init(host,port,secure)
	const char *	host
	const char *	port
	int		secure
	CODE:
	RETVAL = perldap_ldapssl_init(host,port,secure);
	OUTPUT:
	RETVAL

int
ldapssl_install_routines(ld)
	LDAP *		ld
	CODE:
	/* This is a no-op when using OpenLDAP. */
#ifdef USE_OPENLDAP
	RETVAL = 0;
#else
	RETVAL = ldapssl_install_routines(ld);
#endif

int
ldap_start_tls_s(ld,serverctrls,clientctrls)
    LDAP *ld
	LDAPControl **serverctrls
	LDAPControl **clientctrls

const char *
ldapssl_err2string(prerrno)
	const int prerrno
	CODE:
	/* We just return "Unknown error." when using OpenLDAP.  This
	 * is the same string that MozLDAP returns when it has a problem
	 * getting the error string. */
#ifdef USE_OPENLDAP
	RETVAL = "Unknown error.";
#else
	RETVAL = ldapssl_err2string(prerrno);
#endif
	OUTPUT:
	RETVAL

int
ldapssl_set_strength(ld,sslstrength)
	LDAP *ld
	int sslstrength
	CODE:
	/* This is not implemented when using OpenLDAP. */
#ifdef USE_OPENLDAP
        RETVAL = -1;
#else
	RETVAL = ldapssl_set_strength(ld, sslstrength);
#endif
	OUTPUT:
	RETVAL

#endif


#ifdef PRLDAP

#ifndef USE_OPENLDAP
#include <ldappr.h>
#endif

int
prldap_install_routines(ld, shared)
	LDAP *		ld
	int		shared
	CODE:
#ifdef USE_OPENLDAP
	RETVAL = 0;
#else
	RETVAL = prldap_install_routines(ld, shared);
#endif
	OUTPUT:
	RETVAL

int
prldap_set_session_option(ld, sessionarg, option, optdata)
	LDAP *		ld
	void *		sessionarg
	int		option
	int		optdata
	CODE:
#ifdef USE_OPENLDAP
	RETVAL = 0;
#else
	RETVAL = prldap_set_session_option(ld, sessionarg, option, optdata);
#endif
	OUTPUT:
	RETVAL

#endif  /* PRLDAP */



#
# local variables:
# mode: c
# c-basic-offset: 3
# indent-tabs-mode:t
# end:
