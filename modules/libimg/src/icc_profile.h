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

/* -*- Mode: C; tab-width: 4 -*-
 *   icc_profile.h ---	Routines to handle icc profiles
 *
 */
#if defined (COLORSYNC)

#ifndef _icc_profile_h
#define _icc_profile_h


#include "xp_mcom.h"            /* XP definitions and types. */
#include "ntypes.h"             /* typedefs for commonly used Netscape data
                                   structures */
#include "net.h"
#include "if.h"
#include "jmc.h"				/* For JMC_ macros */

/*	---------------------------------------------------------------------------
	Constants
*/
typedef enum
{
	kFirstICCDataBlock		= 1,
	kNthICCDataBlock,
	kICCDataBlocksComplete,
	kICCProfileData
}	ICCDataNotification;

typedef enum			/* Levels of checking */
{
	kCheckState		= 0,		/* Don't read from disk */
	kReadPref					/* Read pref from disk */
}	ColorSyncPrefsCheck;

enum					/* These are values in the container's matching flags*/
{
	kNoMatching			= 0,	/* No profile, no matching */
	kColorSyncOFF		= 1,	/* User has turned ColorSync OFF */
	kProfileNONE		= 2,	/* The IMG profile tag = NONE */
	kProfileInvalid		= 4,	/* An associated or embedded profile was bad */
	kProfilePending		= 8,	/* An associated or embedded profile has yet to complete */
	kProfilePresent		= 16,	/* Match if a profile is present */
	kProfileDefault		= 32,	/* The image has no profile, but prefs say match */
	kProfileFromURL		= 64,	/* The IMG profile tag has a valid url */
	kProfileEmbedded	= 128,	/* The image has an embedded profile */
	kContainerMatched	= 256	/* The image was matched */
};

enum					/* Values of our preferences */
{
	kColorMatchingOff	= 0,
	kColorMatchIfProfile,
	kColorMatchAllImages
};
	
/*	---------------------------------------------------------------------------
	Macros to make IMGCBIF nicer to deal with
*/
#ifdef		STANDALONE_IMAGE_LIB

#define	JMC_EXCP							void*
#define	ADD_IMGCBIF_REF(img_cbif,jmc_excp)	NS_ADDREF(img_cbif)
#define	CHECK_JMC_EXCP(jmc_excp)			(0)
#define	DELETE_JMC_EXCP(jmc_excp)
#define	DELETE_IMGCBIF(img_cbif,jmc_excp)	NS_IF_RELEASE(img_cbif)

#else	/*	!STANDALONE_IMAGE_LIB */

#define	JMC_EXCP							JMCException*
#define	ADD_IMGCBIF_REF(img_cbif,jmc_excp)	IMGCBIF_addRef(img_cbif,&jmc_excp)
#define	CHECK_JMC_EXCP(jmc_excp)			(jmc_excp)
#define	DELETE_JMC_EXCP(jmc_excp)			JMC_DELETE_EXCEPTION(&jmc_excp)
#define	DELETE_IMGCBIF(img_cbif,jmc_excp)	if(img_cbif)IMGCBIF_release(img_cbif,&jmc_excp)

#endif	/*	!STANDALONE_IMAGE_LIB */

#define	CONTAINER_HAS_PROFILE(c)	((c->icc_profile_flags&kProfileFromURL)|| \
									(c->icc_profile_flags&kProfileEmbedded))
#define	CONTAINER_WAS_MATCHED(c)	(c->icc_profile_flags&kContainerMatched)

/*	---------------------------------------------------------------------------
	Public Functions for imglib
*/
IL_ProfileReq *	IL_GetICCProfile			(	const char*			profile_url,
									            IL_GroupContext		*img_cx,
									            ilINetContext		*net_cx,
									            il_container		*ic );
									            
PRBool			il_color_matching_available	(	void );

int32			il_color_matching_on		(	ColorSyncPrefsCheck	check_level );

uint8			il_get_container_color_matching( il_container		*ic );

void			il_stop_icc_profile_request	(	IL_ProfileReq		*pr );

void			il_remove_icc_profile_request(	IL_ProfileReq		*pr );

PRBool			il_waiting_for_icc_profile	(	il_container		*ic );

void			il_setup_color_matching_session( il_container		*ic );

void			il_icc_matching_complete	(	il_container		*ic );

extern	void	il_icc_profile_data_notify	(	il_container		*ic,
												unsigned char		*icc_data_ptr,
												uint32				icc_data_len,
												ICCDataNotification	message );

void			IL_Init_ColorSync			(	void );

/*	---------------------------------------------------------------------------
	Called by ilINetReader()
*/
int				IL_ProfileStreamFirstWrite	(	ip_container		*ip,
												const unsigned char	*str,
												int32				len );
												
int				IL_ProfileStreamWrite		(	ip_container		*ip,
												const unsigned char	*str,
												int32				len );
												
void			IL_ProfileStreamComplete	(	ip_container		*ip );

void			IL_ProfileStreamAbort		(	ip_container		*ip,
												int					status );
												
void			IL_NetProfileRequestDone	(	ip_container 		*ip,
												ilIURL				*url,
												int					status );
												
PRBool			IL_ProfileStreamCreated		(	ip_container		*ic,
												ilIURL				*url,
												int					type );

#endif	/*	_icc_profile_h */

#endif	/* (COLORSYNC) */
