/* -*- C -*-
 *
 * Copyright (c) 1998-1999 Innosoft International, Inc.  All Rights Reserved.
 *
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Innosoft International, 
 * Inc. or a written license from an organization licensed by Innosoft
 * International, Inc. to grant such a license.
 *
 *
 *
 *  Copyright (c) 1994 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  getdn.c
 */

/* 
 * Copyright (c) 1996-1997 Critical Angle Inc. All Rights Reserved.
 *
 */

/*
 * $RCSfile$ $Revision$ $Date$ $State$
 *
 * $Log$
 * Revision 1.23  1999/09/08 15:24:40  ac
 * A99090320A - move dn normalize functions into libldap
 *
 * Revision 1.22  1999/08/20 23:12:00  ac
 * A99082020B - Overrun end of string when AVA contains unnecessary spacing
 *
 * Revision 1.21  1999/08/12 22:51:02  sr
 * remove unused static vars
 *
 * Revision 1.20  1999/04/24 03:28:50  nr
 * update dn parsing to handle trailing spaces
 * do not parse backslash hexchar non-hexchar as a hexpair
 * make ldap_explode_ava decompose # encoded values
 *
 * Revision 1.19  1999/03/22 23:58:18  Administrator
 * update Copyright statements in ILC-SDK
 *
 * Revision 1.18  1998/12/04 03:00:48  wahl
 * do not use ctype.h on characters not tested for ASCII
 *
 * Revision 1.17  1998/10/14 22:10:31  wahl
 * A9810084A call ldap_error_memory if malloc,calloc,realloc fail
 *
 * Revision 1.16  1998/06/26 16:35:52  nr
 * fix ldap_dn2ufn so it will work with dns using any attribute type
 *
 * Revision 1.15  1998/06/21 22:05:10  wahl
 * change Debug to ldap_log_message
 *
 * Revision 1.14  1998/04/23 16:45:51  wahl
 * A9804234A improve quoted character handling
 *
 * Revision 1.13  1998/04/09 01:39:03  wahl
 * add ldap_get_entry_controls
 *
 * Revision 1.12  1998/01/08 07:30:16  nr
 * fix bug in ldap_explode_dn when called with the root dn
 *
 * Revision 1.11  1998/01/02 02:49:43  nr
 * fix whitespace problems in ldap_explode_dn and ldap_explode_rdn
 *
 * Revision 1.10  1997/11/04 02:15:19  wahl
 * remove DNS dn test
 *
 * Revision 1.9  1997/10/25 17:06:53  wahl
 * obtain lock before setting ld_errno
 *
 * Revision 1.8  1997/10/18 00:21:48  wahl
 * copyright notice updates
 *
 * Revision 1.7  1997/09/08 10:17:06  wahl
 * remove lint tests
 *
 * Revision 1.6  1997/09/07 08:49:13  wahl
 * C++ type checks
 *
 * Revision 1.5  1997/09/07 07:00:03  wahl
 * include file simplification
 *
 * Revision 1.4  1997/08/07 16:03:49  wahl
 * *** empty log message ***
 *
 * Revision 1.3  1997/08/07 15:31:43  wahl
 * update to ldap-c-api-00
 *
 * Revision 1.2  1997/07/31 06:00:16  wahl
 * prepare for API draft update
 *
 * Revision 1.1  1997/06/05 03:23:51  wahl
 * *** empty log message ***
 *
 * Revision 1.10  1997/03/01 20:52:37  wahl
 * add RCS headers and Copyright notice
 *
 *
 */

#ifndef NETWORK_ALCHEMY
static char copyright[] = "@(#) Copyright 1998-1999 Innosoft International, Inc.\nCopyright 1996-1997 Critical Angle Inc.\nCopyright (c) 1990 Regents of the University of Michigan.\nAll rights reserved.\n";
#endif

#include "ldap-int.h"

/**
*** This function counts the number of trailing space characters in a 
*** string p of length len and returns the length of the string 
*** with those characters removed.  We need to take care not to remove
*** spaces that are escaped with a backslash.  
**/

extern char *ldap_strdup();

static int remove_trailing_spaces(char *p, int len) {
    while ((len>0)&& isascii(p[len-1]) && isspace(p[len-1])) {
        /* count the number of backslashes before the space */
        int count=0;
        while ((count<len-1) && (p[len-count-2] == '\\')) {
            count++;
        }

        /* if there are any backslashes, check if the number  */
        /* is even or odd.  if it is an even number, then the */
        /* space is not escaped and should be dropped.        */
        if (count>0) {
            if ((count%2) == 0)
               len--;
            return len;
        }

        len--;  /* remove trailing space */
    }
    return len;
}



#define INQUOTE                1
#define OUTQUOTE        2


char **ldap_explode_ava(char *rdncomp) {
    char *rp,*rp2;
    int l,i;
    char **tp;

    if (rdncomp == NULL) return NULL;

    rp = strchr(rdncomp,'=');
  
    if (rp == NULL) {
        LDAPDebug(LDAP_DEBUG_TRACE,"ldap_explode_ava: equals missing\n",0,0,0);
        return NULL;
    }
  
    tp = (char **)calloc(3,sizeof(char *));
    if (tp == NULL) {
      return NULL;
    }

    tp[2] = NULL;

    while(isascii(*rdncomp) && isspace(*rdncomp))
        rdncomp++; /* eliminate inital whitespace */

    tp[0] = ldap_strdup(rdncomp);

    rp2 = strchr(tp[0],'=');
    *rp2 = '\0'; 
    rp2--;

    while(isascii(*rp2) && isspace(*rp2)) { /* eliminate trailing whitespace */
        *rp2= '\0'; 
        rp2--;
    }
  
    rp++;
    l = strlen(rp);
    tp[1] = (char *)calloc(l+1,sizeof(char));

    if (tp[1] == NULL) {
        free(tp[0]);
        free(tp);
        return NULL;
    }
  
    while(isascii(*rp) && isspace(*rp)) {
        rp++; /* eliminate inital whitespace */
        l--;
    }

    if (*rp == '"') {
        int j = 0;

        rp++;

        rp2 = strrchr(rp,'"');
        if (rp2 == NULL) {
            LDAPDebug(LDAP_DEBUG_TRACE,"ldap_explode_ava: trailing quote missing\n",0,0,0);
            free(tp[0]);
            free(tp);
            return NULL;
        }

        for (i = 0; rp + i < rp2; i++) {
            if (rp[i] == '\\') {
                i++;
                if ((rp + i) == rp2) {
                    /* break if no characters left */
                    break;

                } else if ((rp+i+1 <= rp2)  &&
                           isascii(rp[i])   && isxdigit(rp[i]) &&
                           isascii(rp[i+1]) && isxdigit(rp[i+1])) {
                    /* check for hexpair */
                    unsigned int ui = 0;
                    sscanf(rp+i,"%02x",&ui); 
                    tp[1][j] = (char)ui;
                    j++;
                    i++;

                } else {
                    /* assume a single character escape.  technically     */
                    /* this should only be special, backslash, or a quote */
                    tp[1][j] = rp[i];
                    j++;
                }
            } else {
                tp[1][j] = rp[i];
                j++;
            }
        } 
        tp[1][j] = '\0';

    } else if (*rp=='#') {
        int j = 0;

        /* we do not really need the full trailing space removal */
        /* but it is equivalent in this case                     */
        l = remove_trailing_spaces(rp,l);

        rp++; l--; /* skip the # */

        for (i = 0; i<l; i+=2) {
            unsigned int ui = 0;

            if ((rp[i]=='\0') || (rp[i+1]=='\0'))
                break;  /* sanity check */ 

            sscanf(rp+i, "%02x", &ui); 
            tp[1][j++] = (char)ui;
        }

        tp[1][j] = '\0';

    } else {
        int j = 0;

        l = remove_trailing_spaces(rp,l);

        for (i = 0; i<l; i++) {
            if (rp[i] == '\\') {
                i++;
                if (rp[i] == '\0') {
                    /* break if no characters left */
                    break;
                } else if ((rp[i+1] != '\0') &&
                            isascii(rp[i])   && isxdigit(rp[i]) &&
                            isascii(rp[i+1]) && isxdigit(rp[i+1])) {
                    unsigned int ui = 0;
                    sscanf(rp+i,"%02x",&ui); 
                    tp[1][j] = (char)ui;
                    j++;
                    i++;

                } else {
                    tp[1][j] = rp[i];
                    j++;
                }
            } else {
                /* assume a single character escape.  technically     */
                /* this should only be special, backslash, or a quote */
                tp[1][j] = rp[i];
                j++;
            }
        }
        tp[1][j] = '\0';
    }
    return tp;
}

/* minimalist dynamic strings set */
struct dn_normalize_dstring {
   char *str;
   int maxlen,curlen;
};

static int dn_normalize_dstring_init(struct dn_normalize_dstring *ds) 
{
    ds->str=ldap_x_malloc(100);
    if (ds->str==NULL)
        return 0;
    ds->str[0]='\0';
    ds->maxlen=100;
    ds->curlen=1;
    return 1;
}

static void dn_normalize_dstring_free(struct dn_normalize_dstring *ds) {
   if (ds->str !=NULL) free(ds->str);
}

static int dn_normalize_dstring_growto(struct dn_normalize_dstring *ds, int len) {
    char *newstr;

    if (len < ds->maxlen+100) {
        len=ds->maxlen+100;
    }

   newstr=ldap_x_realloc(ds->str,len);
   if (newstr==NULL) {
       return 0;
   }
   ds->str=newstr;
   ds->maxlen=len;
   return 1;
}
    
static int dn_normalize_dstring_append(struct dn_normalize_dstring *ds, char *newstr) {
    int len=strlen(newstr);
      
    if (ds->curlen+len>ds->maxlen) {
        if (!dn_normalize_dstring_growto(ds, ds->curlen+len)) {
            return 0;
        }      
    }
    strcat(ds->str,newstr);
    ds->curlen+=len;
    return 1;
}

/*
*** dn_normalize and supporting functions and structures
*/


/* internal rdn list structure for dn_normalize     */
/* used for keeping a sorted list of rdn components */
struct dn_normalize_rdn_list {
    char *attr;
    char *val;
    struct dn_normalize_rdn_list *next;
};
  
/** used by dn_normalize to insert an rdn component into and rdn 
*** list.  the list is ordered on the attribute name. (case      
*** insensitive)                                                 
***
*** return 1 for success, 0 for error (an error being a duplicate attribute
*** component)
**/

static int dn_normalize_rdn_insert(struct dn_normalize_rdn_list **list, struct dn_normalize_rdn_list *newrdn) {
    int cflag=1; /* init to non-zero */

    while(*list && ((cflag=strcasecmp((*list)->attr,newrdn->attr))<0))
        list = &((*list)->next);

    newrdn->next=*list;
    *list=newrdn;
 
    if (cflag==0) {
        return 0;
    }
    return 1;
}

/* escape a value for use within a DN      */
/* always use minimal backslash escaping   */
/* with backslash hex escaping for leading */
/* and trailing spaces.                    */
static char* ldap_dn_normalize_escape_val(char *val) {
    char *newval;
    int i,len,pos;

    if (val==NULL)
        return NULL;

    len=strlen(val);  
    newval=(char *)ldap_x_malloc(3*len+1);  /* the maximum possible length */
    pos=0;

    for (i=0;i<len;i++) {
        int  escape=0; 

        switch(val[i]) {
            case ','  :
            case '+'  :
            case ';'  :
            case '\\' :
            case '\"' :
            case '<' :
            case '>' : { 
                escape=1;
                break;
            }
            case '#'  : {
                if (i==0) {
                    escape=1;
                }
                break;
            }
            default: {
                if (isascii(val[i]) && isspace(val[i])) {
                    if ((i==0) || (i==len-1)) {
                        escape=2;
                    }
                }
            }
        }

        if (escape==0) { 
            /* no escape */
            newval[pos++]=val[i];
	} else if (escape==1) {
            /* backslash escape */
            newval[pos++]='\\';
            newval[pos++]=val[i];
        } else { /* escape == 2 */
            /* hex escape */
            static char *hexchars="0123456789abcdef";

            newval[pos++]='\\';
            newval[pos++]=hexchars[(val[i]/16)&0xf];
            newval[pos++]=hexchars[(val[i]%16)&0xf];
        }
    }
    newval[pos] = '\0';
    return newval;
}

/* normalize an attribute/value and insert it into the rdn list */
static int dn_normalize_set_rdn(
    struct dn_normalize_rdn_list *rdn, 
    char **ava,
    char **(*norm_func)(void*, char**),
    void *norm_data,
    void (*free_func)(void *)
)
{
    char *newattr, *newval;
    char **newava;

    if ((newava = (*norm_func)(norm_data, ava)) == NULL) {
        return 0;
    }

    newattr = ldap_strdup(newava[0]);
    newval = ldap_dn_normalize_escape_val(newava[1]);

    (*free_func)(newava[0]);
    (*free_func)(newava[1]);
    (*free_func)(newava);

    if (newattr == NULL || newval == NULL) {
        if (newattr) free(newattr);
        if (newval) free(newval);
        return 0;
    }

    rdn->attr = newattr; 
    rdn->val  = newval;
    rdn->next = NULL;

    return 1;
}

char *ldap_dn_normalize_count(
    const char *dn,
    int *nrPtr,
    int *naPtr,
    char **(*norm_func)(void*, char**),
    void *norm_data,
    void (*free_func)(void *)
) 
{
    char   **dn_comp,**rdn_comp,**ava_comp;
    int    i,j;
    int    count=1;  /* approx. space requirements  (may overestimate) */
    struct dn_normalize_dstring ds;
    char   *ret;
    int na = 0;

    dn_comp = ldap_explode_dn((char *)dn,0);  /* cast const */
    if (dn_comp == NULL) 
        return NULL;
   
    dn_normalize_dstring_init(&ds);

    for(i=0; dn_comp[i];i++) {
        struct dn_normalize_rdn_list *sorted_rdns=NULL,*tmp,*tmp2;

        rdn_comp=ldap_explode_rdn(dn_comp[i],0);
        if (rdn_comp==NULL) { 
            dn_normalize_dstring_free(&ds);
            ldap_value_free(dn_comp);
            tmp=sorted_rdns;
            while (tmp!=NULL) {
                free(tmp->attr);
                free(tmp->val);

                tmp2=tmp->next;
                free(tmp);
                tmp=tmp2;
            }
            return NULL;
        }

        for(j=0;rdn_comp[j];j++) {

            ava_comp=ldap_explode_ava(rdn_comp[j]);
            if (ava_comp==NULL) {
                dn_normalize_dstring_free(&ds);
                ldap_value_free(dn_comp);
                ldap_value_free(rdn_comp);
                tmp=sorted_rdns;
                while (tmp!=NULL) {
                    free(tmp->attr);
                    free(tmp->val);

                    tmp2=tmp->next;
                    free(tmp);
                    tmp=tmp2;
                }
                return NULL;
            }

            tmp = (struct dn_normalize_rdn_list *) 
                ldap_x_malloc(sizeof(struct dn_normalize_rdn_list));

            if (!dn_normalize_set_rdn(tmp, ava_comp, 
                                      norm_func, norm_data, 
                                      free_func)) {
                free(tmp);
                ldap_value_free(ava_comp);
                dn_normalize_dstring_free(&ds);
                ldap_value_free(dn_comp);
                ldap_value_free(rdn_comp);
                tmp=sorted_rdns;
                while (tmp!=NULL) {
                    free(tmp->attr);
                    free(tmp->val);

                    tmp2=tmp->next;
                    free(tmp);
                    tmp=tmp2;
                }
                return NULL;
            }
            ldap_value_free(ava_comp);

            if (!dn_normalize_rdn_insert(&sorted_rdns, tmp)) {
                free(tmp);
                dn_normalize_dstring_free(&ds);
                ldap_value_free(dn_comp);
                ldap_value_free(rdn_comp);
                tmp=sorted_rdns;
                while (tmp!=NULL) {
                    free(tmp->attr);
                    free(tmp->val);

                    tmp2=tmp->next;
                    free(tmp);
                    tmp=tmp2;
                }
                return NULL;
            }
        }
	if (j > na) na = j;
        count+=j; /* make room for rdn separators */
        ldap_value_free(rdn_comp);

        if (i>0) {
            dn_normalize_dstring_append(&ds,",");
        }

        j=0;
        tmp=sorted_rdns;
        while (tmp!=NULL) {
            if (j!=0) {
                dn_normalize_dstring_append(&ds,"+");            
            } 

            dn_normalize_dstring_append(&ds,tmp->attr);
            dn_normalize_dstring_append(&ds,"=");
            dn_normalize_dstring_append(&ds,tmp->val);

            free(tmp->attr);
            free(tmp->val);

            tmp2=tmp->next;
            free(tmp);
            tmp=tmp2;

            j++;
	}
        
    } 
    ldap_value_free(dn_comp); 
    if (nrPtr) {
      *nrPtr = i;
    }
    if (naPtr) {
      *naPtr = na;
    }

    ret= ldap_strdup(ds.str);
    dn_normalize_dstring_free(&ds);

    return ret;
}

char *ldap_dn_flatten(char **rdns)
{
    struct dn_normalize_dstring ds;
    char *ret, *v;
    int i, j;

    dn_normalize_dstring_init(&ds);

    for (i = 0; rdns[i]; i++) {
        char **avas = ldap_explode_rdn(rdns[i], 0);
        if (i != 0) {
            dn_normalize_dstring_append(&ds, ",");
        }
        for (j = 0; avas[j]; j++) {
            char **tv = ldap_explode_ava(avas[j]);
            if (j != 0) {
                dn_normalize_dstring_append(&ds, "+");
            }
            v = ldap_dn_normalize_escape_val(tv[1]);
            dn_normalize_dstring_append(&ds, tv[0]);
            dn_normalize_dstring_append(&ds, "=");
            dn_normalize_dstring_append(&ds, v);
            free(v);
            ldap_value_free(tv);
        }
        ldap_value_free(avas);
    }

    ret = ldap_strdup(ds.str);
    dn_normalize_dstring_free(&ds);

    return ret;
}

static char **normalize_ava_stub(void *data, char **ava)
{
    return ldap_charray_dup(ava);
}

static char **normalize_ava_cis(void *data, char **ava)
{
    char *newval, *p, *s, **ret = NULL;
    int lastspace,seenfirstchar;

    s = ava[1];

    newval=ldap_x_malloc(strlen(s)+1);
    lastspace=0; seenfirstchar=0;

    for (p=newval; s && *s; s=(char*)ldap_utf8_nextchar((unsigned char*)s)) {
        if (ldap_utf8_isspace((unsigned char*)s)) {
            lastspace=1;
            continue;
        } 

        /* we have a printing character */

        if (lastspace) {
            /* only add a space if we have already seen */
            /* a nonspace character.  trailing spaces   */
	    /* dropped because we never hit this point  */
            /* unless there is a nonspace following it. */

            if (seenfirstchar) {
                *p++=' ';
            }
            lastspace=0;
        }

        seenfirstchar=1;  
        p+=ldap_utf8_toupper((unsigned char*)p,(unsigned char*)s); 
    }
    *p = '\0';

    ldap_charray_add(&ret, ldap_strdup(ava[0]));
    ldap_charray_add(&ret, newval);

    for (s = ret[0]; *s; s++) {
        *s = TOUPPER(*s);
    }

    return ret;
}

char *ldap_dn_normalize_format(const char *dn)
{
    return ldap_dn_normalize_count(dn, NULL, NULL, 
                                   normalize_ava_stub, NULL,
                                   ldap_memfree);
}

char *ldap_dn_normalize(const char *dn)
{
    return ldap_dn_normalize_count(dn, NULL, NULL, 
                                   normalize_ava_cis, NULL,
                                   ldap_memfree);
}
