/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "xp.h"
#include "mkutils.h"
#include "netutils.h"
#include "mkselect.h"
#include "mktcp.h"
#include "mkgeturl.h"
#include "plstr.h"
#include "prmem.h"

#include "fileurl.h"
#include "httpurl.h"
#include "ftpurl.h"
#include "abouturl.h"
#include "gophurl.h"
#include "jsurl.h"
#include "fileurl.h"
#include "remoturl.h"
#include "dataurl.h"
#include "netcache.h"

#if defined(JAVA)
#include "marimurl.h"
#endif

/* For random protocols */
#include "htmldlgs.h"
#include "libevent.h"
#include "libi18n.h"
#include "secnav.h"
extern int MK_MALFORMED_URL_ERROR;

/* For about handlers */
#include "il_strm.h"
#include "glhist.h"

PRIVATE void net_InitTotallyRandomStuffPeopleAddedProtocols(void);
PRIVATE void net_InitAboutURLs();

PUBLIC void
NET_ClientProtocolInitialize(void)
{

    NET_InitFileProtocol();
    NET_InitHTTPProtocol();
    NET_InitMemCacProtocol();
    NET_InitFTPProtocol();
    NET_InitAboutProtocol();
    NET_InitGopherProtocol();
    NET_InitMochaProtocol();
    NET_InitRemoteProtocol();
    NET_InitDataURLProtocol();

#ifdef JAVA
    NET_InitMarimbaProtocol();
#endif

	net_InitTotallyRandomStuffPeopleAddedProtocols();

    net_InitAboutURLs();
}

/* print out security URL
 */
PRIVATE int net_output_security_url(ActiveEntry * cur_entry, MWContext *cx)
{
    NET_StreamClass * stream;
	char * content_type;
	char * which = cur_entry->URL_s->address;
	char * colon = PL_strchr (which, ':');

	if (colon)
	  {
		/* found the first colon; now find the question mark
		   (as in "about:security?certs"). */
		which = colon + 1;
		colon = PL_strchr (which, '?');
		if (colon)
		  which = colon + 1;
		else
		  which = which + PL_strlen (which); /* give it "" */
	  }

	content_type = SECNAV_SecURLContentType(which);
	if (!content_type) {
		cur_entry->status = MK_MALFORMED_URL_ERROR;

	} else if (!PL_strcasecmp(content_type, "advisor")) {
	    cur_entry->status = SECNAV_SecHandleSecurityAdvisorURL(cx, which);

	} else {
		int status;

		StrAllocCopy(cur_entry->URL_s->content_type, content_type);

		cur_entry->format_out = CLEAR_CACHE_BIT(cur_entry->format_out);

		stream = NET_StreamBuilder(cur_entry->format_out,
								   cur_entry->URL_s, cur_entry->window_id);

		if (!stream)
			return(MK_UNABLE_TO_CONVERT);

		status = SECNAV_SecURLData(which, stream, cx);

		if (status >= 0) {
			(*stream->complete) (stream);
		} else {
			(*stream->abort) (stream, status);
		}

		cur_entry->status = status;

		FREE(stream);
	}

    return(-1);
}

PRIVATE int32
net_SecurityURLLoad(ActiveEntry *ce)
{
	if(ce->URL_s)
		StrAllocCopy(ce->URL_s->charset, INTL_ResourceCharSet());
	return net_output_security_url(ce, ce->window_id);
}

PRIVATE int32
net_SeclibURLLoad(ActiveEntry *ce)
{
	if(ce->URL_s)
		StrAllocCopy(ce->URL_s->charset, INTL_ResourceCharSet());
	SECNAV_HandleInternalSecURL(ce->URL_s, ce->window_id);
	return -1;
}

PRIVATE int32
net_HTMLPanelLoad(ActiveEntry *ce)
{
	if(ce->URL_s)
		StrAllocCopy(ce->URL_s->charset, INTL_ResourceCharSet());
	XP_HandleHTMLPanel(ce->URL_s);
	return -1;
}

PRIVATE int32
net_HTMLDialogLoad(ActiveEntry *ce)
{
	if(ce->URL_s)
		StrAllocCopy(ce->URL_s->charset, INTL_ResourceCharSet());
	XP_HandleHTMLDialog(ce->URL_s);
	return -1;
}

PRIVATE int32
net_WysiwygLoad(ActiveEntry *ce)
{
	const char *real_url = LM_SkipWysiwygURLPrefix(ce->URL_s->address);
	char *new_address;

	/* XXX can't use StrAllocCopy because it frees dest first */
	if (real_url && (new_address = PL_strdup(real_url)) != NULL)
	{
		PR_Free(ce->URL_s->address);
		ce->URL_s->address = new_address;
		FREE_AND_CLEAR(ce->URL_s->wysiwyg_url);
	}

	/* no need to free real_url */

	ce->status = MK_DO_REDIRECT;
	return MK_DO_REDIRECT;
}

PRIVATE int32
net_ProtoMainStub(ActiveEntry *ce)
{
#ifdef DO_ANNOYING_ASSERTS_IN_STUBS
	PR_ASSERT(0);
#endif
	return -1;
}

PRIVATE void
net_ProtoCleanupStub(void)
{
}

PRIVATE void
net_reg_random_protocol(NET_ProtoInitFunc *LoadRoutine, const char *proto)
{
    NET_ProtoImpl *random_proto_impl;

	random_proto_impl = PR_NEW(NET_ProtoImpl);

	if(!random_proto_impl)
		return;
	
    random_proto_impl->init = LoadRoutine;
    random_proto_impl->process = net_ProtoMainStub;
    random_proto_impl->interrupt = net_ProtoMainStub;
    random_proto_impl->cleanup = net_ProtoCleanupStub;

    NET_RegisterProtocolImplementation(random_proto_impl, proto);
}

/* don't you just hate it when people come along and hack this
 * kind of stuff into your code.
 *
 * @@@ clean this up some time
 */
PRIVATE void
net_InitTotallyRandomStuffPeopleAddedProtocols(void)
{
	net_reg_random_protocol(net_SecurityURLLoad, SECURITY_PROTOCOL);
	net_reg_random_protocol(net_SeclibURLLoad, INTERNAL_SECLIB_PROTOCOL);
	net_reg_random_protocol(net_HTMLPanelLoad, HTML_PANEL_HANDLER_PROTOCOL);
	net_reg_random_protocol(net_HTMLDialogLoad, HTML_DIALOG_HANDLER_PROTOCOL);
	net_reg_random_protocol(net_WysiwygLoad, WYSIWYG_PROTOCOL);
}

#ifdef XP_UNIX
extern void FE_ShowMinibuffer(MWContext *);
  
PRBool net_AboutMinibuffer(const char *token,
                           FO_Present_Types format_out,
                           URL_Struct *URL_s,
                           MWContext *window_id)
{
    FE_ShowMinibuffer(window_id);

    return PR_TRUE;
}
#endif

#ifdef WEBFONTS
PRBool net_AboutFonts(const char *token,
                      FO_Present_Types format_out,
                      URL_Struct *URL_s,
                      MWContext *window_id)
{
    NF_AboutFonts(window_id, which);
    return PR_TRUE;
}
#endif /* WEBFONTS */

PRBool net_AboutImageCache(const char *token,
                           FO_Present_Types format_out,
                           URL_Struct *URL_s,
                           MWContext *window_id)
{
    IL_DisplayMemCacheInfoAsHTML(format_out, URL_s, window_id);
    return PR_TRUE;
}

PRBool net_AboutGlobalHistory(const char *token,
                              FO_Present_Types format_out,
                              URL_Struct *URL_s,
                              MWContext *window_id)
{
    NET_DisplayGlobalHistoryInfoAsHTML(window_id, URL_s, format_out);
    
    return PR_TRUE;
}

PRIVATE void net_InitAboutURLs()
{
    NET_RegisterAboutProtocol("image-cache?", net_AboutImageCache);
    NET_RegisterAboutProtocol("global", net_AboutGlobalHistory);
#ifdef XP_UNIX
    NET_RegisterAboutProtocol("minibuffer", net_AboutMinibuffer);
#endif
#ifdef WEBFONTS
    NET_RegisterAboutProtocol("fonts?", net_AboutFonts);
#endif
}
