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

#if defined(MOZ_MAIL_NEWS)

#ifdef FEATURE_KATAKANA
#include "katakana.i"
#else
#include "intlpriv.h"
#endif
#include "prefapi.h"

#ifndef FEATURE_KATAKANA
static void han2zen(XP_Bool insjis, unsigned char *inbuf, uint32 inlen, 
					unsigned char *outbuf, XP_Bool *composite)
{
	*composite = FALSE;
	XP_STRCPY(outbuf, inbuf);
}

/*
 * Half to full Katakana conversion for SJIS. Caller need to allocate outbuf (x2 of inbuf).
 */
MODULE_PRIVATE void INTL_SjisHalf2FullKana(unsigned char *inbuf, uint32 inlen, unsigned char *outbuf, uint32 *byteused)
{
	XP_Bool composite;
		
	han2zen(TRUE, inbuf, inlen, outbuf, &composite);
	*byteused = composite ? 2 : 1;
}

/*
 * Half to full Katakana conversion for EUC. Caller need to allocate outbuf (x3 of inbuf).
 */
MODULE_PRIVATE void INTL_EucHalf2FullKana(unsigned char *inbuf, uint32 inlen, unsigned char *outbuf, uint32 *byteused)
{
	XP_Bool composite;
		
	han2zen(FALSE, inbuf, inlen, outbuf, &composite);
	*byteused = composite ? 3 : 1;		/* 2 chars plus SS2 or 1 char */
}

#endif /* FEATURE_KATAKANA */

/* pref related prototype and variables */
PUBLIC int PR_CALLBACK intl_SetSendHankakuKana(const char * newpref, void * data);

static XP_Bool pref_callback_installed = FALSE;
static XP_Bool send_hankaku_kana = FALSE;
static const char *pref_send_hankaku_kana = "mailnews.send_hankaku_kana";


/* callback routine invoked by prefapi when the pref value changes */
PUBLIC int PR_CALLBACK intl_SetSendHankakuKana(const char * newpref, void * data)
{
	return PREF_GetBoolPref(pref_send_hankaku_kana, &send_hankaku_kana);
}

MODULE_PRIVATE XP_Bool INTL_GetSendHankakuKana()
{
	if (!pref_callback_installed)
	{
		PREF_GetBoolPref(pref_send_hankaku_kana, &send_hankaku_kana);
		PREF_RegisterCallback(pref_send_hankaku_kana, intl_SetSendHankakuKana, NULL);
		pref_callback_installed = TRUE;
	}
	
	return send_hankaku_kana;
}

#endif /* MOZ_MAIL_NEWS */
