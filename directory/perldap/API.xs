/*  -*- Mode: C; eval: (c-set-style "GNU") -*-
 ******************************************************************************
 * $Id$
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is PerLDAP. The Initial Developer of the Original
 * Code is Netscape Communications Corp. and Clayton Donley. Portions
 * created by Netscape are Copyright (C) Netscape Communications
 * Corp., portions created by Clayton Donley are Copyright (C) Clayton
 * Donley. All Rights Reserved.
 *
 * Contributor(s):	Leif Hedstrom <leif@perldap.org>
 *			Kevin McCarthy <kevin@perldap.org>
 *
 * DESCRIPTION
 *    This is the XSUB interface for the API.
 *
 * NOTE:
 *   Please read the typemap file in this directory.  It contains
 *   information about the way parameters are mapped and used, and when
 *   things need to be freed.
 *
 *****************************************************************************/

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

/* SSL is only available in Binary */
#ifdef USE_SSL
# include <ldap_ssl.h>
#endif

/* AUTOLOAD methods for LDAP constants */
#include "constant.h"



 /*************
 * Prototypes *
 *************/

static int perldap_init();

static void * perldap_malloc(size_t size);

static void * perldap_calloc(size_t number, size_t size);

static void * perldap_realloc(void *ptr, size_t size);

static void perldap_free(void *ptr);

static int StrCaseCmp(const char *s, const char *t);

static char * StrDup(const char *source);

static char ** avref2charptrptr(SV *avref);

static SV * charptrptr2avref(char **cppval);

static struct berval ** avref2berptrptr(SV *avref);

static SV * berptrptr2avref(struct berval **bval);

static SV * berptr2svpv(struct berval *bval);

static LDAPControl ** sv2ldapcontrolptrptr(SV *ldapcontrol_sv);

static SV * ldapcontrolptrptr2avref(LDAPControl **ldapcontrol);

static LDAPVirtualList * sv2ldapvirtuallistptr(SV *virtuallist_sv);

static LDAPMod *parse1mod(SV *ldap_value_ref,char *ldap_current_attribute,
                int ldap_add_func,int cont);

static int calc_mod_size(HV *ldap_change);

static LDAPMod **hash2mod(SV *ldap_change_ref,int ldap_add_func,const char *func);

static int LDAP_CALL internal_rebind_proc(LDAP *ld,char **dnp,char **pwp,
            int *authmethodp,int freeit,void *arg);

static int LDAP_CALL ldap_default_rebind_proc(LDAP *ld, char **dn, char **pswd,
            int *auth, int freeit, void *arg);


/* Global Definitions and Variables */
SV *ldap_perl_rebindproc = NULL;
static char *ldap_default_rebind_dn = NULL;
static char *ldap_default_rebind_pwd = NULL;
static int ldap_default_rebind_auth = LDAP_AUTH_SIMPLE;

/* Return a Perl List from a char ** in PPCODE */
#define RET_CPP(cppvar) \
	   int cppindex; \
	   if (cppvar) { \
	   for (cppindex = 0; cppvar[cppindex] != NULL; cppindex++) \
	   { \
	      EXTEND(sp,1); \
	      PUSHs(sv_2mortal(newSVpv(cppvar[cppindex],strlen(cppvar[cppindex])))); \
	   } \
	   ldap_value_free(cppvar); }

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


 /***********************
 * Function Definitions *
 ***********************/


/*****************************
* memory allocation routines *
*****************************/

static
int
perldap_init()
{
   struct ldap_memalloc_fns memalloc_fns;

   memalloc_fns.ldapmem_malloc = perldap_malloc;
   memalloc_fns.ldapmem_calloc = perldap_calloc;
   memalloc_fns.ldapmem_realloc = perldap_realloc;
   memalloc_fns.ldapmem_free = perldap_free;

   return (ldap_set_option(NULL,
                           LDAP_OPT_MEMALLOC_FN_PTRS,
                           &memalloc_fns));
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


/******************
* string routines *
******************/

/*
 * StrCaseCmp
 * Replacement for strcasecmp, since it doesn't exist on many
 * systems, including NT...
 */
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


/************************
* T_charptrptr typemaps *
************************/

/*
 * avref2charptrptr
 *
 * INPUT typemap.
 * Return a char ** when passed a reference to an AV 
 * NOTE: This value must be freed using ldap_value_free in CLEANUP.
 */
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


/*
 * charptrptr2avref
 *
 * OUTPUT typemap.
 * Return an AV reference when given a char **
 * Frees the char ** and char * for you.
 */
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
      ldap_value_free(cppval);
   }
   return(tmp_ref);
}


/**************************
* T_bervalptrptr typemaps *
**************************/

/*
 * avref2berptrptr
 *
 * INPUT typemap.
 * Return a struct berval ** when passed a reference to an AV
 * This isn't used as a typemap anymore.  It is only called
 * by parse1mod() to set up the LDAPMod structure for an update/add/delete.
 * hash2mod calls thie for LDAPMod ** conversions.
 * The result of this should be free'd using ldap_mod_free(attrs, 1)
 * in the CLEANUP section.
 */
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

/*
 * berptrptr2avref
 *
 * OUTPUT typemap.
 * Return an AV Reference when given a struct berval **
 * This is not used anywhere anymore, but is kept around
 * for completeness in case we need to use the typemap again.
 */
static
SV *
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


/***********************
* T_bervalptr typemaps *
***********************/

/*
 * berptr2svpv
 *
 * OUTPUT typemap.
 * Converts a struct berval * to a SvPv scalar in Perl
 * Frees the bval for you.
 */
static
SV *
berptr2svpv(struct berval *bval)
{
   SV *SVval;

   SVval = newSVpv(bval->bv_val, bval->bv_len);

   ber_bvfree(bval);

   return(SVval);
}


/*******************************
* T_ldapcontrolptrptr typemaps *
*******************************/

/*
 * sv2ldapcontrolptrptr
 *
 * INPUT typemap.
 * This function converts a SV passed in, into a LDAPControl **
 * The SV passed in should either be a SV (which is a LDAPControl *)
 * or it should be a RV to an AV (each of which is a LDAPControl *)
 *
 * NOTE: be sure the free the LDAPControl ** before we
 *       return back, otherwise it will never be freed.
 */
static
LDAPControl **
sv2ldapcontrolptrptr(SV *ldapcontrol_sv)
{
   LDAPControl **ldapcontrol = NULL;
   AV *ldapcontrol_av;
   SV **ldapcontrol_av_entry;
   I32 ldapcontrol_av_index;

   /*
    * Regular scalar - single entry
    */
   if ( ! SvROK(ldapcontrol_sv) )
   {
      Newz(1, ldapcontrol, 2, LDAPControl *);
      ldapcontrol[0] = (LDAPControl *)SvIV(ldapcontrol_sv);
      ldapcontrol[1] = NULL;
   }
   /*
    * Reference to an array
    */
   else if ( SvTYPE(SvRV(ldapcontrol_sv)) == SVt_PVAV )
   {
      ldapcontrol_av = (AV *)SvRV(ldapcontrol_sv);
      /*
       * av_len + 2 because:
       *    1) av_len is highest index (base 0), so add 1
       *    2) adding 1 for the trailing NULL entry
       */
      Newz(1, ldapcontrol, (av_len(ldapcontrol_av) + 2), LDAPControl *);
      for (ldapcontrol_av_index = 0;
           ldapcontrol_av_index <= av_len(ldapcontrol_av);
           ldapcontrol_av_index++)
      {
         ldapcontrol_av_entry = av_fetch(ldapcontrol_av, ldapcontrol_av_index,
                                         0);
         if ( ldapcontrol_av_entry != NULL )
            ldapcontrol[ldapcontrol_av_index] = (LDAPControl *)SvIV(*ldapcontrol_av_entry);
      }
      ldapcontrol[av_len(ldapcontrol_av) + 1] = NULL;
   }
   /*
    * something else - just return NULL
    */

   return(ldapcontrol);
}


/*
 * ldapcontrolptrptr2avref
 *
 * OUTPUT typemap.
 * This function converts an LDAPControl ** being returned to
 * a reference to an AV
 *
 * NOTE: This function frees the ldapcontrol ** passed in
 *       It does _not_ free the ldapcontrol * entries in the array.
 *       The free is because we are discarding the "container" when
 *       we convert it to a AV
 */
static
SV *
ldapcontrolptrptr2avref(LDAPControl **ldapcontrol)
{
   AV *ldapcontrol_av;
   SV *ldapcontrol_avrv;
   int ldapcontrol_index;
   SV *ldapcontrol_av_entry;

   ldapcontrol_av = newAV();
   ldapcontrol_avrv = newRV_inc((SV*)ldapcontrol_av);

   if (ldapcontrol != NULL)
   {
      for (ldapcontrol_index = 0;
           ldapcontrol[ldapcontrol_index] != NULL;
           ldapcontrol_index++)
      {
         ldapcontrol_av_entry = newSViv((IV)ldapcontrol[ldapcontrol_index]);
         av_push(ldapcontrol_av, ldapcontrol_av_entry);
      }
      Safefree(ldapcontrol);
   }

   return(ldapcontrol_avrv);
}


/********************************
* T_ldapvirtuallistptr typemaps *
********************************/

/*
 * sv2ldapvirtuallistptr
 *
 * INPUT typemap.
 * Converts a SV (RV) passed in into a LDAPVirtualLIst *.
 * The SV can be either a RvAv with 6 entries
 * or it can be a RvHv hash with the entries:
 * ldvlist_before_count, ldvlist_after_count,
 * ldvlist_attrvalue, ldvlist_index, ldvlist_size,
 * ldvlist_extradata
 * The LDAPVirtualList * is malloc()ed in this function.
 * Therefore it needs to be Safefree() in the CLEANUP section.
 */
static
LDAPVirtualList *
sv2ldapvirtuallistptr(SV *virtuallist_sv)
{
   LDAPVirtualList *virtuallist = NULL;
   AV *virtuallist_av;
   HV *virtuallist_hv;
   SV **virtuallist_entry;
   STRLEN len;
   char *before_key = "ldvlist_before_count";
   char *after_key = "ldvlist_after_count";
   char *value_key = "ldvlist_attrvalue";
   char *index_key = "ldvlist_index";
   char *size_key = "ldvlist_size";
   char *data_key = "ldvlist_extradata";

   if ( ! SvROK(virtuallist_sv) )
   {
      return(NULL);
   }

   if ( SvTYPE(SvRV(virtuallist_sv)) == SVt_PVAV )
   {
      virtuallist_av = (AV *)SvRV(virtuallist_sv);
      if ( av_len(virtuallist_av) != 5 )
         return(NULL);

      Newz(1, virtuallist, 1, LDAPVirtualList);

      virtuallist_entry = av_fetch(virtuallist_av, 0, 0);
      virtuallist->ldvlist_before_count = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = av_fetch(virtuallist_av, 1, 0);
      virtuallist->ldvlist_after_count = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = av_fetch(virtuallist_av, 2, 0);
      virtuallist->ldvlist_attrvalue = (char *)
            (virtuallist_entry ? SvPV(*virtuallist_entry, len) : 0);

      virtuallist_entry = av_fetch(virtuallist_av, 3, 0);
      virtuallist->ldvlist_index = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = av_fetch(virtuallist_av, 4, 0);
      virtuallist->ldvlist_size = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = av_fetch(virtuallist_av, 5, 0);
      virtuallist->ldvlist_extradata = (void *)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);
   }
   else if ( SvTYPE(SvRV(virtuallist_sv)) == SVt_PVHV ) 
   {
      virtuallist_hv = (HV *)SvRV(virtuallist_sv);
      Newz(1, virtuallist, 1, LDAPVirtualList);

      virtuallist_entry = hv_fetch(virtuallist_hv, before_key, strlen(before_key), 0);
      virtuallist->ldvlist_before_count = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = hv_fetch(virtuallist_hv, after_key, strlen(after_key), 0);
      virtuallist->ldvlist_after_count = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = hv_fetch(virtuallist_hv, value_key, strlen(value_key), 0);
      virtuallist->ldvlist_attrvalue = (char *)
            (virtuallist_entry ? SvPV(*virtuallist_entry, len) : 0);

      virtuallist_entry = hv_fetch(virtuallist_hv, index_key, strlen(index_key), 0);
      virtuallist->ldvlist_index = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = hv_fetch(virtuallist_hv, size_key, strlen(size_key), 0);
      virtuallist->ldvlist_size = (unsigned long)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);

      virtuallist_entry = hv_fetch(virtuallist_hv, data_key, strlen(data_key), 0);
      virtuallist->ldvlist_extradata = (void *)
            (virtuallist_entry ? SvIV(*virtuallist_entry) : 0);
   }

   return(virtuallist);
}




/**********************
* LDAPMod ** typemaps *
**********************/

/*
 * parse1mod
 *
 * Take a single reference, figure out if it is a HASH,
 * ARRAY, or SCALAR, then extract the values and attributes and
 * return a single LDAPMod pointer to this data.
 */
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


/*
 * calc_mod_size
 *
 * Calculates the number of LDAPMod's buried inside the ldap_change passed
 * in.  This is used by hash2mod to calculate the size to allocate in Newz
 */
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


/*
 * hash2mod
 *
 * INPUT typemap.
 * Cycle through all the keys in the hash and properly call
 * the appropriate functions to build a NULL terminated list of
 * LDAPMod pointers.
 */
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


/********************
* Rebind Procedures *
********************/

/*
 * internal_rebind_proc
 *
 * Wrapper to call a PERL rebind process
 */
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


/*
 * ldap_default_rebind_proc
 *
 * NT and internal_rebind_proc hate each other, so they need this...
 */
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


 /***************
 * Begin Module *
 ***************/

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

#ifdef LDAPV3

int
ldap_abandon_ext(ld,msgid,serverctrls,clientctrls)
	LDAP *		ld
	int		msgid
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_add(ld,dn,attrs)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
	CLEANUP:
	
	if (attrs)
	  ldap_mods_free(attrs, 1);

#ifdef LDAPV3

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
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

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
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_add_s(ld,dn,attrs)
	LDAP *		ld
	const char *	dn
	LDAPMod **	attrs = hash2mod($arg,1,"$func_name");
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
	      ldap_ber_free(ber, freebuf);
	   }
	}

int
ldap_bind(ld,dn,passwd,authmethod)
	LDAP *		ld
	const char *	dn
	const char *	passwd
	int		authmethod

int
ldap_bind_s(ld,dn,passwd,authmethod)
	LDAP *		ld
	const char *	dn
	const char *	passwd
	int		authmethod

int
ldap_compare(ld,dn,attr,value)
	LDAP *		ld
	const char *	dn
	const char *	attr
	const char *	value

#ifdef LDAPV3

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
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

int
ldap_compare_ext_s(ld,dn,attr,bvalue,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	const char *	attr
	struct berval 	&bvalue
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_compare_s(ld,dn,attr,value)
	LDAP *		ld
	const char *	dn
	const char *	attr
	const char *	value

#ifdef LDAPV3

void
ldap_control_free(ctrl)
	LDAPControl *	ctrl

#endif

#ifdef CONTROLS_COUNT_WORKS
int
ldap_controls_count(ctrls)
	LDAPControl **	ctrls
	CLEANUP:
	if (ctrls)
	  Safefree(ctrls);

#endif

#ifdef LDAPV3

 #
 # Note - you don't have to call Safefree(ctrls) b/c
 # ldap_controls_free does this
 #
void
ldap_controls_free(ctrls)
	LDAPControl **	ctrls

#endif

int
ldap_count_entries(ld,result)
	LDAP *		ld
	LDAPMessage *	result

#ifdef LDAPV3

int
ldap_count_messages(ld,result)
	LDAP *		ld
	LDAPMessage *	result

int
ldap_count_references(ld,result)
	LDAP *		ld
	LDAPMessage *	result

#endif

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
	CLEANUP:
	if (valwords)
	  ldap_value_free(valwords);

#ifdef LDAPV3

int
ldap_create_persistentsearch_control(ld,changetypes,changesonly,return_echg_ctrls,ctrl_iscritical,ctrlp)
	LDAP *		ld
	int		changetypes
	int		changesonly
	int		return_echg_ctrls
	char		ctrl_iscritical
	LDAPControl *	&ctrlp = NO_INIT
	OUTPUT:
	RETVAL
	ctrlp

int
ldap_create_sort_control(ld,sortKeyList,ctrl_iscritical,ctrlp)
	LDAP *		ld
	LDAPsortkey **	sortKeyList
	char		ctrl_iscritical
	LDAPControl *	&ctrlp = NO_INIT
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
	LDAPControl *	&ctrlp = NO_INIT
	OUTPUT:
	RETVAL
	ctrlp
	CLEANUP:
	if (ldvlistp)
	  Safefree(ldvlistp);

#endif

int
ldap_delete(ld,dn)
	LDAP *		ld
	const char *	dn

#ifdef LDAPV3

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
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

int
ldap_delete_ext_s(ld,dn,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_delete_s(ld,dn)
	LDAP *		ld
	const char *	dn

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

#ifdef LDAPV3

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
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

int
ldap_extended_operation_s(ld,requestoid,requestdata,serverctrls,clientctrls,retoidp,retdatap)
	LDAP *		ld
	const char *	requestoid
	struct berval 	&requestdata
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	char *		&retoidp = NO_INIT
	struct berval * &retdatap = NO_INIT
	OUTPUT:
	RETVAL
	retoidp
	retdatap
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);
	ldap_memfree(retoidp);

#endif

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

#ifdef LDAPV3

LDAPMessage *
ldap_first_message(ld,res)
	LDAP *		ld
	LDAPMessage *	res

LDAPMessage *
ldap_first_reference(ld,res)
	LDAP *		ld
	LDAPMessage *	res

#endif

void
ldap_free_friendlymap(map)
	FriendlyMap	&map

#ifdef LDAPV3

void
ldap_free_sort_keylist(sortKeyList)
	LDAPsortkey **	sortKeyList

#endif

void
ldap_free_urldesc(ludp)
	LDAPURLDesc *	ludp

 #
 # Documentation doesn't say you need to free the return value
 #
char *
ldap_friendly_name(filename,name,map)
	char *		filename
	char *		name
	FriendlyMap	&map
	OUTPUT:
	map

char *
ldap_get_dn(ld,entry)
	LDAP *		ld
	LDAPMessage *	entry
	CLEANUP:
	ldap_memfree(RETVAL);

#ifdef LDAPV3

int
ldap_get_entry_controls(ld,entry,serverctrlsp)
	LDAP *		ld
	LDAPMessage *	entry
	LDAPControl **	&serverctrlsp = NO_INIT
	OUTPUT:
	RETVAL
	serverctrlsp

#endif

void
ldap_getfilter_free(lfdp)
	LDAPFiltDesc *	lfdp

LDAPFiltInfo *
ldap_getfirstfilter(lfdp,tagpat,value)
	LDAPFiltDesc *	lfdp
	char *		tagpat
	char *		value	

#ifdef LDAPV3

void
ldap_get_lang_values(ld,entry,target,type)
	LDAP *		ld
	LDAPMessage *	entry
	const char *	target
	char *		type
	PPCODE:
	{
	   char ** MOZLDAP_VAL = ldap_get_lang_values(ld,entry,target,&type);
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
	   struct berval ** MOZLDAP_VAL = 
	       ldap_get_lang_values_len(ld,entry,target,&type);
	   RET_BVPP(MOZLDAP_VAL);
	}

#endif


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

	   RETVAL = ldap_get_lderrno(ld, (m && SvROK(m)) ? &match : (char **)NULL,
	                                 (s && SvROK(s)) ? &msg : (char **)NULL);

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
	   char **MOZLDAP_VAL = ldap_get_values(ld,entry,target);
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
	int		port

LDAPFiltDesc *
ldap_init_getfilter(fname)
	char *		fname

LDAPFiltDesc *
ldap_init_getfilter_buf(buf,buflen)
	char *		buf
	long		buflen

int
ldap_is_ldap_url(url)
	char *		url

#ifdef LDAPV3

void
ldap_memcache_destroy(cache)
	LDAPMemCache *	cache

void
ldap_memcache_flush(cache,dn,scope)
	LDAPMemCache *	cache
	char *		dn
	int		scope

int
ldap_memcache_get(ld,cachep)
	LDAP *		ld
	LDAPMemCache *	&cachep = NO_INIT
	OUTPUT:
	RETVAL
	cachep

int
ldap_memcache_init(ttl,size,baseDNs,cachep)
	unsigned long	ttl
	unsigned long	size
	char **		baseDNs
	LDAPMemCache *	&cachep = NO_INIT
	CODE:
	RETVAL = ldap_memcache_init(ttl,size,baseDNs,NULL,&cachep);
	OUTPUT:
	RETVAL
	cachep
	CLEANUP:
	if (baseDNs)
	  ldap_value_free(baseDNs);

int
ldap_memcache_set(ld,cache)
	LDAP *		ld
	LDAPMemCache *	cache

void
ldap_memcache_update(cache)
	LDAPMemCache *	cache

#endif

void
ldap_memfree(p)
	void *		p

int
ldap_modify(ld,dn,mods)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg,0,"$func_name");
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

#ifdef LDAPV3

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
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

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
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_modify_s(ld,dn,mods)
	LDAP *		ld
	const char *	dn
	LDAPMod **	mods = hash2mod($arg, 0, "$func_name");
	CLEANUP:
	if (mods)
	  ldap_mods_free(mods, 1);

int
ldap_modrdn(ld,dn,newrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn

int
ldap_modrdn_s(ld,dn,newrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn

int
ldap_modrdn2(ld,dn,newrdn,deleteoldrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	int		deleteoldrdn

int
ldap_modrdn2_s(ld,dn,newrdn,deleteoldrdn)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	int 		deleteoldrdn

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
ldap_multisort_entries(ld,chain,attr)
	LDAP *		ld
	LDAPMessage *	&chain
	char **		attr
	CODE:
	{
	   RETVAL = ldap_multisort_entries(ld,&chain,attr,StrCaseCmp);
	}
	OUTPUT:
	RETVAL
	chain
	CLEANUP:
	if (attr)
	  ldap_value_free(attr);

char *
ldap_next_attribute(ld,entry,ber)
	LDAP *		ld
	LDAPMessage *	entry
	BerElement *	ber
	OUTPUT:
	RETVAL
	CLEANUP:
	ldap_memfree(RETVAL);

LDAPMessage *
ldap_next_entry(ld,entry)
	LDAP *		ld
	LDAPMessage *	entry

#ifdef LDAPV3

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
	int 		&chgtypep = NO_INIT
	char *		&prevdnp = NO_INIT
	int 		&chgnumpresentp = NO_INIT
	long 		&chgnump = NO_INIT
	OUTPUT:
	RETVAL
	chgtypep
	prevdnp
	chgnumpresentp
	chgnump
	CLEANUP:
	if (ctrls)
	  Safefree(ctrls);
	ldap_memfree(prevdnp);

int
ldap_parse_extended_result(ld,res,retoidp,retdatap,freeit)
	LDAP *		ld
	LDAPMessage *	res
	char *		&retoidp = NO_INIT
	struct berval * &retdatap = NO_INIT
	int		freeit
	OUTPUT:
	RETVAL
	retoidp
	retdatap
	CLEANUP:
	ldap_memfree(retoidp);

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
	CLEANUP:
	ldap_memfree(matcheddnp);
	ldap_memfree(errmsgp);

int
ldap_parse_sasl_bind_result(ld,res,servercredp,freeit)
	LDAP *		ld
	LDAPMessage *	res
	struct berval * &servercredp = NO_INIT
	int freeit
	OUTPUT:
	RETVAL
	servercredp

int
ldap_parse_sort_control(ld,ctrls,result,attribute)
	LDAP *		ld
	LDAPControl **	ctrls
	unsigned long 	&result = NO_INIT
	char *		&attribute = NO_INIT
	OUTPUT:
	RETVAL
	result
	attribute
	CLEANUP:
	if (ctrls)
	  Safefree(ctrls);
	ldap_memfree(attribute);

int
ldap_parse_virtuallist_control(ld,ctrls,target_posp,list_sizep,errcodep)
	LDAP *		ld
	LDAPControl **	ctrls
	unsigned long 	&target_posp = NO_INIT
	unsigned long 	&list_sizep = NO_INIT
	int		&errcodep = NO_INIT
	OUTPUT:
	target_posp
	list_sizep
	errcodep
	CLEANUP:
	if (ctrls)
	  Safefree(ctrls);

#endif

void
ldap_perror(ld,s)
	LDAP *		ld
	const char *	s

#ifdef LDAPV3

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
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

int
ldap_rename_s(ld,dn,newrdn,newparent,deleteoldrdn,serverctrls,clientctrls)
	LDAP *		ld
	const char *	dn
	const char *	newrdn
	const char *	newparent
	int		deleteoldrdn
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

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

#ifdef LDAPV3

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
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

int
ldap_sasl_bind_s(ld,dn,mechanism,cred,serverctrls,clientctrls,servercredp)
	LDAP *		ld
	const char *	dn
	const char *	mechanism
	struct berval 	&cred
	LDAPControl **	serverctrls
	LDAPControl **	clientctrls
	struct berval * &servercredp = NO_INIT
	OUTPUT:
	RETVAL
	servercredp
	CLEANUP:
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_search(ld,base,scope,filter,attrs,attrsonly)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	CLEANUP:
	if (attrs)
	  ldap_value_free(attrs);

#ifdef LDAPV3

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
	  ldap_value_free(attrs);
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

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
	  ldap_value_free(attrs);
	if (serverctrls)
	  Safefree(serverctrls);
	if (clientctrls)
	  Safefree(clientctrls);

#endif

int
ldap_search_s(ld,base,scope,filter,attrs,attrsonly,res)
	LDAP *		ld
	const char *	base
	int		scope
	const char *	filter
	char **		attrs
	int		attrsonly
	LDAPMessage *	&res = NO_INIT
	OUTPUT:
	RETVAL
	res
	CLEANUP:
	if (attrs)
	  ldap_value_free(attrs);

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
	OUTPUT:
	RETVAL
	res
	CLEANUP:
	if (attrs)
	  ldap_value_free(attrs);
	
int
ldap_set_filter_additions(lfdp,prefix,suffix)
	LDAPFiltDesc *	lfdp
	char *		prefix
	char *		suffix

int
ldap_set_lderrno(ld,e,m,s)
	LDAP *		ld
	int		e
	char *		m
	char *		s

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
	      ldap_set_rebind_proc(ld,internal_rebind_proc,NULL);
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

          ldap_set_rebind_proc(ld,
           (LDAP_REBINDPROC_CALLBACK *)&ldap_default_rebind_proc,NULL);
        }

int
ldap_simple_bind(ld,who,passwd)
	LDAP *		ld
	const char *	who
	const char *	passwd

int
ldap_simple_bind_s(ld,who,passwd)
	LDAP *		ld
	const char *	who
	const char *	passwd

int
ldap_sort_entries(ld,chain,attr)
	LDAP *		ld
	LDAPMessage *	&chain
	char *		attr
	CODE:
	{
	   RETVAL = ldap_sort_entries(ld,&chain,attr,StrCaseCmp);
	}
	OUTPUT:
	RETVAL
	chain
	
int
ldap_unbind(ld)
	LDAP *		ld

int
ldap_unbind_s(ld)
	LDAP *		ld

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
	      static char *options_key = "options";
	      SV* options = newSViv(realcomp->lud_options);
	      SV* host = newSVpv(realcomp->lud_host,0);
	      SV* port = newSViv(realcomp->lud_port);
	      SV* dn; /* = newSVpv(realcomp->lud_dn,0); */
	      SV* scope = newSViv(realcomp->lud_scope);
	      SV* filter = newSVpv(realcomp->lud_filter,0);
	      AV* attrarray = newAV();
	      SV* attribref = newRV((SV*) attrarray);

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
	      hv_store(FullHash,host_key,strlen(host_key),host,0);
	      hv_store(FullHash,port_key,strlen(port_key),port,0);
	      hv_store(FullHash,dn_key,strlen(dn_key),dn,0);
	      hv_store(FullHash,attr_key,strlen(attr_key),attribref,0);
	      hv_store(FullHash,scope_key,strlen(scope_key),scope,0);
	      hv_store(FullHash,filter_key,strlen(filter_key),filter,0);
	      hv_store(FullHash,options_key,strlen(options_key),options,0);
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

int
ldap_url_search_s(ld,url,attrsonly,res)
	LDAP *		ld
	char *		url
	int		attrsonly
	LDAPMessage *	&res
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
	OUTPUT:
	RETVAL
	res

int
ldap_version(ver)
	LDAPVersion *	ver

#ifdef USE_SSL

int
ldapssl_client_init(certdbpath,certdbhandle)
	const char *	certdbpath
	void *		certdbhandle

#ifdef LDAPV3

int
ldapssl_clientauth_init(certdbpath,certdbhandle,needkeydb,keydbpath,keydbhandle)
	char *		certdbpath
	void *		certdbhandle
	int		needkeydb
	char *		keydbpath
	void *		keydbhandle

int
ldapssl_enable_clientauth(ld,keynickname,keypasswd,certnickname)
	LDAP *		ld
	char *		keynickname
	char *		keypasswd
	char *		certnickname

#endif

LDAP *
ldapssl_init(host,port,secure)
	const char *	host
	int		port
	int		secure

int
ldapssl_install_routines(ld)
	LDAP *		ld

#endif
