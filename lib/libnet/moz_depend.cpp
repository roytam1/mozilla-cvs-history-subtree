/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "depend.h"

#include "nspr.h"
#include "libi18n.h"
#include "secnav.h"
#include "xp_hash.h"
#include "libimg.h"
#include "il_strm.h"
#include "jsapi.h"

/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/intl_csi.c
 *---------------------------------------------------------------------------
 */

extern "C" {

/*  Meta charset is weakest. Only set doc_csid if no http or override */
void
INTL_SetCSIDocCSID (INTL_CharSetInfo c, int16 doc_csid)
{
    MOZ_FUNCTION_STUB;
}


int16
INTL_GetCSIDocCSID(INTL_CharSetInfo c)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


void
INTL_SetCSIWinCSID(INTL_CharSetInfo c, int16 win_csid)
{
    MOZ_FUNCTION_STUB;
}


int16
INTL_GetCSIWinCSID(INTL_CharSetInfo c)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


INTL_CharSetInfo 
LO_GetDocumentCharacterSetInfo(MWContext *context)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/acptlang.c
 *---------------------------------------------------------------------------
 */

/* INTL_GetAcceptCharset()                          */
/* return the AcceptCharset from XP Preference      */
/* this should be a C style NULL terminated string  */
char* INTL_GetAcceptCharset()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* INTL_GetAcceptLanguage()                         */
/* return the AcceptLanguage from XP Preference     */
/* this should be a C style NULL terminated string  */
char* INTL_GetAcceptLanguage()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}




/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/net_junk.c
 *---------------------------------------------------------------------------
 */
PUBLIC Stream *
INTL_ConvCharCode (int         format_out,
                   void       *data_obj,
                   URL_Struct *URL_s,
                   MWContext  *mwcontext)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/cvchcode.c
 *---------------------------------------------------------------------------
 */

/* INTL_GetCharCodeConverter:
 * RETURN: 1 if converter found, else 0
 * Also, sets:
 *              obj->cvtfunc:   function handle for chararcter
 *                              code set streams converter
 *              obj->cvtflag:   (Optional) flag to converter
 *                              function
 *              obj->from_csid: Code set converting from
 *              obj->to_csid:   Code set converting to
 * If the arg to_csid==0, then use the the conversion  for the
 * first conversion entry that matches the from_csid.
 */
int
INTL_GetCharCodeConverter(register int16  from_csid,
                          register int16  to_csid,
                          CCCDataObject   obj)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


CCCDataObject
INTL_CreateCharCodeConverter()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


unsigned char *
INTL_CallCharCodeConverter(CCCDataObject obj, const unsigned char *buf,
                           int32 bufsz)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


void
INTL_DestroyCharCodeConverter(CCCDataObject obj)
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/fe_ccc.c
 *---------------------------------------------------------------------------
 */
/*
 INTL_DocToWinCharSetID,
   Based on DefaultDocCSID, it determines which Win CSID to use for Display
*/
/*

        To Do: (ftang)

        We should seperate the DocToWinCharSetID logic from the cscvt_t table
        for Cyrillic users. 

*/
int16 INTL_DocToWinCharSetID(int16 csid)
{
    MOZ_FUNCTION_STUB;
    return CS_FE_ASCII;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/doc_ccc.c
 *---------------------------------------------------------------------------
 */

PUBLIC void INTL_CCCReportMetaCharsetTag(MWContext *context, char *charset_tag)
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/intl_csi.c
 *---------------------------------------------------------------------------
 */
/* ----------- Mime CSID ----------- */
char *
INTL_GetCSIMimeCharset (INTL_CharSetInfo c)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libparse/pa_hash.c
 *---------------------------------------------------------------------------
 */

/*************************************
 * Function: pa_tokenize_tag
 *
 * Description: This function maps the passed in string
 * 		to one of the valid tag element tokens, or to
 *		the UNKNOWN token.
 *
 * Params: Takes a \0 terminated string.
 *
 * Returns: a 32 bit token to describe this tag element.  On error,
 * 	    which means it was not passed an unknown tag element string,
 * 	    it returns the token P_UNKNOWN.
 *
 * Performance Notes:
 * Profiling on mac revealed this routine as a big (5%) time sink.
 * This function was stolen from pa_mdl.c and merged with the perfect
 * hashing code and the tag comparison code so it would be flatter (fewer
 * function calls) since those are still expensive on 68K and x86 machines.
 *************************************/

intn
pa_tokenize_tag(char *str)
{
    MOZ_FUNCTION_STUB;
    return -1; /* P_UNKNOWN */;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libparse/pa_parse.c
 *---------------------------------------------------------------------------
 */

/*************************************
 * Function: PA_BeginParseMDL
 *
 * Description: The outside world's main access to the parser.
 *      call this when you are going to start parsing
 *      a new document to set up the parsing stream.
 *      This function cannot be called successfully
 *      until PA_ParserInit() has been called.
 *
 * Params: Takes lots of document information that is all
 *     ignored right now, just used the window_id to create
 *     a unique document id.
 *
 * Returns: a pointer to a new NET_StreamClass structure, set up to
 *      give the caller a parsing stream into the parser.
 *      Returns NULL on error.
 *************************************/
NET_StreamClass *
PA_BeginParseMDL(FO_Present_Types format_out,
                 void *init_data, URL_Struct *anchor, MWContext *window_id)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/cert/pcertdb.c
 *---------------------------------------------------------------------------
 */

/*
 * Decode a certificate and enter it into the temporary certificate database.
 * Deal with nicknames correctly
 *
 * nickname is only used if isperm == PR_TRUE
 */
CERTCertificate *
CERT_NewTempCertificate(CERTCertDBHandle *handle, SECItem *derCert,
                        char *nickname, PRBool isperm, PRBool copyDER)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/cert/certhtml.c
 *---------------------------------------------------------------------------
 */
char *
CERT_HTMLCertInfo(CERTCertificate *cert, PRBool showImages, PRBool showIssuer)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/hash/algmd5.c
 *---------------------------------------------------------------------------
 */
SECStatus
MD5_HashBuf(unsigned char *dest, const unsigned char *src, uint32 src_length)
{
    MOZ_FUNCTION_STUB;
    return SECFailure;
}


void
SECNAV_HandleInternalSecURL(URL_Struct *url, MWContext *cx)
{
    MOZ_FUNCTION_STUB;
}


char *
SECNAV_MakeCertButtonString(CERTCertificate *cert)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/modules/security/nav/securl.c
 *---------------------------------------------------------------------------
 */

 /*
 * send the data for the given about:security url to the given stream
 */
int
SECNAV_SecURLData(char *which, NET_StreamClass *stream, MWContext *cx)
{
    MOZ_FUNCTION_STUB;
    return SECFailure;
}


char *
SECNAV_SecURLContentType(char *which)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


int
SECNAV_SecHandleSecurityAdvisorURL(MWContext *cx, const char *which)
{
    MOZ_FUNCTION_STUB;
    return -1;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/authdll.cpp
 *---------------------------------------------------------------------------
 */
char * WFE_BuildCompuserveAuthString(URL_Struct *URL_s)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


int
WFE_DoCompuserveAuthenticate(MWContext *context,
                             URL_Struct *URL_s, 
                             char *authenticate_header_value)
{
    MOZ_FUNCTION_STUB;
    return NET_AUTH_FAILED_DONT_DISPLAY;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/regproto.cpp
 *---------------------------------------------------------------------------
 */

//  Purpose:    See if we're supposed to handle a specific protocol differently.
//  Arguments:  pContext        The context.
//              iFormatOut      The format out (possibly saving).
//              pURL            The URL to load.
//              pExitFunc       The URL exit routine.  If we handle the protocol, we use the function.
//Returns:      BOOL            TRUE    We want to handle the protocol here.
//                              FALSE   Netlib continues to handle the protocol.
//Comments:     To work with certain DDE topics which can cause URLs of a specific protocol type to be handled by another
//              application.
//Revision History:
//      01-17-95        created GAB
//

XP_Bool FE_UseExternalProtocolModule(MWContext *pContext, FO_Present_Types iFormatOut, URL_Struct *pURL,
                                     Net_GetUrlExitFunc *pExitFunc)	
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/femess.cpp
 *---------------------------------------------------------------------------
 */
//
// Prompt the user for a local file name
//
int FE_PromptForFileName(MWContext *context,
                         const char *prompt_string,
                         const char *default_path,
                         XP_Bool file_must_exist_p,
                         XP_Bool directories_allowed_p,
                         ReadFileNameCallbackFunction fn,
                         void *closure)
{
    MOZ_FUNCTION_STUB;
    return -1;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/fegui.cpp
 *---------------------------------------------------------------------------
 */
//  Return some spanked out full path on the mac.
//  For windows, we just return what was passed in.
//  We must provide it in a seperate buffer, otherwise they might change
//      the original and change also what they believe to be saved.
char *WH_FilePlatformName(const char *pName)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


//  Purpose:    Set a delayed load timer for a window for a particular URL.
//  Arguments:  pContext    The context that we're in.  We are only interested on wether or not there appears to be a Frame/hence document.
//              ulSeconds   The number of seconds that will pass before we attempt the reload.
//  Returns:    void
//  Comments:
//  Revision History:
//  02-17-95    created GAB
//  07-22-95    modified to use new context.
//  05-03-96    modified to use new API outside of idle loop.
//

void FE_SetRefreshURLTimer(MWContext *pContext, uint32 ulSeconds, char *pRefreshUrl) 
{
    MOZ_FUNCTION_STUB;
}


PUBLIC void 
FE_ConnectToRemoteHost(MWContext * context, int url_type, char *
    hostname, char * port, char * username) 
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/feutil.cpp
 *---------------------------------------------------------------------------
 */

//      The purpose of FEU_AhAhAhAhStayingAlive is to house the one and only
//      saturday night fever function; named after Chouck's idol.
//      This function will attempt to do all that is necessary in order
//      to keep the application's messages flowing and idle loops
//      going when we need to finish an asynchronous operation
//      synchronously.
//      The current cases that cause this are RPC calls into the
//      application where we need to return a value or produce output
//      from only one entry point before returning to the caller.
//
//      If and when you modify this function, get your changes reviewed.
//      It is too vital that this work, always.
//
//      The function only attempts to look at one message at a time, or
//      propigate one idle call at a time, keeping it's own idle count.
//      This is not a loop.  YOU must provide the loop which calls this function.
//
//      Due to the nature and order of which we process windows messages, this
//      can seriously mess with the flow of control through the client.
//      If there is any chance at all that you can ensure that you are at the
//      bottom of the message queue before doing this, then please take those
//      measures.
void FEU_StayingAlive()
{
    MOZ_FUNCTION_STUB;
}




/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/cfe.cpp
 *---------------------------------------------------------------------------
 */
void FE_RunNetcaster(MWContext *context) 
{
    MOZ_FUNCTION_STUB;
}




/*
 *---------------------------------------------------------------------------
 * From ns/lib/libdbm/db.c
 *---------------------------------------------------------------------------
 */
DB *
dbopen(const char *fname, int flags,int mode, DBTYPE type, const void *openinfo)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_hash.c
 *---------------------------------------------------------------------------
 */

/* create a hash list, which isn't really a table.
 */
PUBLIC XP_HashList *
XP_HashListNew (int size, 
                XP_HashingFunction  hash_func, 
                XP_HashCompFunction comp_func)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


 /* free a hash list, which isn't really a table.
 */
PUBLIC void
XP_HashListDestroy (XP_HashList * hash_struct)
{
    MOZ_FUNCTION_STUB;
}


/* add an element to a hash list, which isn't really a table.
 *
 * returns positive on success and negative on failure
 *
 * ERROR return codes
 *
 *  XP_HASH_DUPLICATE_OBJECT
 */
PUBLIC int
XP_HashListAddObject (XP_HashList * hash_struct, void * new_ele)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/* removes an object by name from the hash list, which isn't really a table,
 * and returns the object if found
 */
PUBLIC void *
XP_HashListRemoveObject (XP_HashList * hash_struct, void * ele)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* finds an object by name in the hash list, which isn't really a table.
 */
PUBLIC void *
XP_HashListFindObject (XP_HashList * hash_struct, void * ele)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PUBLIC uint32
XP_StringHash (const void *xv)
{ 
    MOZ_FUNCTION_STUB;
    return 0;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_cntxt.c
 *---------------------------------------------------------------------------
 */

 /*
 * Finds a context that should be loaded with the URL, given
 * a type and current (refering) context.  Return NULL if there is none.
 */
MWContext * XP_FindContextOfType (MWContext * context, MWContextType type)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 * if the passed context is in the global context list
 * TRUE is returned.  Otherwise false
 */
Bool XP_IsContextInList(MWContext *context)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xplocale.c
 *---------------------------------------------------------------------------
 */

size_t XP_StrfTime(MWContext* context, char *result, size_t maxsize, int format,
                   const struct tm *timeptr)
{
    MOZ_FUNCTION_STUB;
    return 0;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_sec.c
 *---------------------------------------------------------------------------
 */

 /*
** Take basic security key information and return an allocated string
** that contains a "pretty printed" version.
*/
char *XP_PrettySecurityStatus(int level, char *cipher, int keySize,
                              int secretKeySize)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/layutil.c
 *---------------------------------------------------------------------------
 */

 /*
 * Return the width of the window for this context in chars in the default
 * fixed width font.  Return -1 on error.
 */
int16
LO_WindowWidthInFixedChars(MWContext *context)
{
    MOZ_FUNCTION_STUB;
    return -1;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/layinfo.c
 *---------------------------------------------------------------------------
 */

/*
 * Prepare a bunch of information about the content of this
 * document and prints the information as HTML down
 * the passed in stream.
 *
 * Returns:
 *      -1      If the context passed does not correspond to any currently
 *              laid out document.
 *      0       If the context passed corresponds to a document that is
 *              in the process of being laid out.
 *      1       If the context passed corresponds to a document that is
 *              completly laid out and info can be found.
 */
PUBLIC intn
LO_DocumentInfo(MWContext *context, NET_StreamClass *stream)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/layobj.c
 *---------------------------------------------------------------------------
 */

/*
 * Create a new stream handler for dealing with a stream of
 * object data.  We don't really want to do anything with
 * the data, we just need to check the type to see if this
 * is some kind of object we can handle.  If it is, we can
 * format the right kind of object, clear the layout blockage,
 * and connect this stream up to its rightful owner.
 * NOTE: Plug-ins are the only object type supported here now.
 */
NET_StreamClass*
LO_NewObjectStream(FO_Present_Types format_out, void* type,
                   URL_Struct* urls, MWContext* context)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/editor.cpp
 *---------------------------------------------------------------------------
 */

//
// Hooked into the GetURL state machine.  We do intermitent processing to
//  let the layout engine to the initial processing and fetch all the nested
//  images.
//
// Returns: 1 - Done ok, continuing.
//    0 - Done ok, stopping.
//   -1 - not done, error.
//
intn EDT_ProcessTag(void *data_object, PA_Tag *tag, intn status)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/*
 *---------------------------------------------------------------------------
 * From ns/modules/libimg/src/if.c
 *---------------------------------------------------------------------------
 */

/*
 *  determine what kind of image data we are dealing with
 */
int
IL_Type(const char *buf, int32 len)
{
    MOZ_FUNCTION_STUB;
    return IL_NOTFOUND;
}


PRBool
IL_PreferredStream(URL_Struct *urls)
{
    MOZ_FUNCTION_STUB;
    return PR_FALSE;
}


NET_StreamClass *
IL_NewStream (FO_Present_Types format_out,
              void *type,
              URL_Struct *urls,
              OPAQUE_CONTEXT *cx)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/modules/libimg/src/ilclient.c
 *---------------------------------------------------------------------------
 */

/*
 * Create an HTML stream and generate HTML describing
 * the image cache.  Use "about:memory-cache" URL to acess.
 */
int 
IL_DisplayMemCacheInfoAsHTML(FO_Present_Types format_out, URL_Struct *urls,
                             OPAQUE_CONTEXT *cx)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


char *
IL_HTMLImageInfo(char *url_address)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* Set limit on approximate size, in bytes, of all pixmap storage used
   by the imagelib.  */
void
IL_SetCacheSize(uint32 new_size)
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/modules/libimg/src/external.c
 *---------------------------------------------------------------------------
 */
NET_StreamClass *
IL_ViewStream(FO_Present_Types format_out, void *newshack, URL_Struct *urls,
              OPAQUE_CONTEXT *cx)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmisc/glhist.c
 *---------------------------------------------------------------------------
 */

PUBLIC void
NET_RegisterEnableUrlMatchCallback(void)
{
    MOZ_FUNCTION_STUB;
}


/* start global history tracking
 */
PUBLIC void
GH_InitGlobalHistory(void)
{
    MOZ_FUNCTION_STUB;
}


/* save the global history to a file while leaving the object in memory
 */
PUBLIC void
GH_SaveGlobalHistory(void)
{
    MOZ_FUNCTION_STUB;
}



/* create an HTML stream and push a bunch of HTML about
 * the global history
 */
MODULE_PRIVATE int
NET_DisplayGlobalHistoryInfoAsHTML(MWContext *context,
                                   URL_Struct *URL_s,
                                   int format_out)
{
    MOZ_FUNCTION_STUB;
    return MK_UNABLE_TO_CONVERT;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmisc/shist.c
 *---------------------------------------------------------------------------
 */
PUBLIC int
SHIST_GetIndex(History * hist, History_entry * entry)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/plugin/npglue.c
 *---------------------------------------------------------------------------
 */

XP_Bool
NPL_HandleURL(MWContext *cx, FO_Present_Types iFormatOut, URL_Struct *pURL, Net_GetUrlExitFunc *pExitFunc)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/lm_taint.c
 *---------------------------------------------------------------------------
 */

const char *
LM_SkipWysiwygURLPrefix(const char *url_string)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/htmldlgs/htmldlgs.c
 *---------------------------------------------------------------------------
 */

void
XP_HandleHTMLPanel(URL_Struct *url)
{
    MOZ_FUNCTION_STUB;
}


void
XP_HandleHTMLDialog(URL_Struct *url)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/nav-java/netscape/net/netStubs.c
 *---------------------------------------------------------------------------
 */


/*
 *---------------------------------------------------------------------------
 * From ns/modules/softupdt/src/softupdt.c
 *---------------------------------------------------------------------------
 */

/* New stream callback */
/* creates the stream, and a opens up a temporary file */
NET_StreamClass * SU_NewStream (int format_out, void * registration,
                                URL_Struct * request, MWContext *context)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/modules/libpref/src/prefapi.c
 *---------------------------------------------------------------------------
 */

/* This is more recent than the below 3 routines which should be obsoleted */
PR_IMPLEMENT(JSBool)
PREF_EvaluateConfigScript(const char * js_buffer, size_t length,
                          const char* filename, XP_Bool bGlobalContext, XP_Bool bCallbacks)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}



}; /* end of extern "C" */
