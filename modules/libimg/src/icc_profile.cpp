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
 *   icc_profile.c ---	Routines to deal with icc profiles in images
 *						and in the html stream.  Apple's ColorSync is
 *						used for color management tasks.
 *
 */

#include "if.h"
#include "icc_profile.h"
#include "dummy_nc.h"				/* For GetURLStruct, etc. */
#include "prefapi.h"				/* For PREF_GetIntPref */
#include "merrors.h"
#include "xp_str.h"					/* For BlockAllocCopy, BlockAllocCat */
#include "il_strm.h"            	/* Stream converters. */

extern int MK_UNABLE_TO_LOCATE_FILE;
extern int MK_OUT_OF_MEMORY;

#ifdef PROFILE
#pragma profile on
#endif


/* Global list of profile containers. */
static ip_container *il_global_profile_container_list = NULL;
static PRBool		isColorSyncInstalled = FALSE;
static int32		isColorSyncOn = kColorMatchingOff;

#if	CACHE_COLOR_WORLDS
typedef	struct
{
	void	*matching_session;
	void	*src_profile_ref;
	void	*next;
}
il_cached_matching_session;

/*
	Support up to 'kNumDisplaysToCache' sets of color-world caches,
	each set being defined by the display profile, with an entry
	for any given source profile.  Each set can have a max of
	'kMaxCachedMatchingSessions' sessions.
*/
#define	kMaxCachedMatchingSessions	4
#define	kNumDisplaysToCache			2
static il_cached_matching_session *il_color_matching_session_cache[kNumDisplaysToCache] = NULL;

#endif /* CACHE_COLOR_WORLDS */

/***************************** Routine Prototypes ****************************
******************************************************************************/
												
static void		il_remove_icc_profile_ref		(	void				*ref,
													ip_container		*ip );

static void		*il_get_icc_profile_ref			(	ip_container		*ip );

static ip_container *il_get_icc_profile_container(	IL_GroupContext		*img_cx,
		            								ilINetContext		*net_cx,
													NET_ReloadMethod	cache_reload_policy,
													const char*			profile_url );

static ip_container *il_find_icc_profile_container(	ilIURL				*url );

static void		il_delete_icc_profile_container	(	ip_container		*ip );

static void		il_delete_icc_profile_request	(	IL_ProfileReq		*pr );

static PRBool	il_icc_profile_stopped			(	ip_container		*ip );

static void		il_icc_profile_complete_notify	(	ip_container		*ip );

static int		il_add_icc_profile_block		(	ip_container		*ip,
													uint32				buffer_size,
													unsigned char		*data,
													uint32				data_size );

static void		il_set_container_color_matching	(	il_container		*ic,
													uint16				matching_flag );
#if	CACHE_COLOR_WORLDS
static void		*il_find_color_matching_session	(	void				*src_ref,
													void				*dst_ref );
static PRBool	il_cache_color_matching_session	(	il_container		*ic );

#endif /* CACHE_COLOR_WORLDS */

/*	******************************************************************************
*/												

/******************************* Main Entry Points *******************************/

/*	******************************************************************************
	IL_Init_ColorSync
	
	Desc:		We need to create an imgcb interface here, to call the
				Front End and ask it whether ColorSync exists.
*/
void			IL_Init_ColorSync			(	void )
{
/*
	Define a local JMC callback interface pointer.
*/
#ifdef STANDALONE_IMAGE_LIB
    ilIImageRenderer	*img_cb;
#else
    IMGCB 				*img_cb;
#endif

	JMCException *		exc = NULL;
	
    img_cb = IMGCBFactory_Create( &exc );
    if (exc)
    {
    	/* XXXM12N Should really return exception */
        JMC_DELETE_EXCEPTION( &exc );
    }
    else
    {
#ifdef STANDALONE_IMAGE_LIB
		isColorSyncInstalled = img_cb->IsColorSyncAvailable(NULL);
#else
    	isColorSyncInstalled = IMGCBIF_IsColorSyncAvailable((IMGCBIF*) img_cb, NULL);
#endif /* STANDALONE_IMAGE_LIB */
    }
    
    /*
    	Get the prefs value - store it in a global.
    */
    (void) il_color_matching_on(kReadPref);
}

/*	******************************************************************************
	IL_GetICCProfile
	
	Desc:	The main entry point.  Looks for the profile in the disk cache
			and if it wasn't found, generates a request to the Netlib
			to get it.
*/
IL_ProfileReq *
IL_GetICCProfile	(	const char		*profile_url,
						IL_GroupContext	*img_cx,
						ilINetContext	*net_cx,
						il_container	*ic  )
{
	NET_ReloadMethod		cache_reload_policy = NET_NORMAL_RELOAD;
    IL_ProfileReq			*profile_req;
	ilINetReader			*reader = NULL;
	ip_container			*ip = NULL;
	int						err = 0;

    ILTRACE(1, ("il: IL_GetICCProfile, url=%s\n", profile_url));

	/*
		If we got passed a URL of "NONE"
		that means we want to turn off matching for
		this container completely, regardless of any
		embedded profiles and of the preferences.
	*/
	if (strcasecomp(profile_url,"NONE") == 0)
	{
		il_set_container_color_matching(ic,kProfileNONE);
		return NULL;
	}

    /*
    	Set up the other matching flags cases, based on prefs.
		We don't need to reload the prefs, because they were
		just read in il_get_container.  For the prefs cases
		kColorMatchIfProfile and kColorMatchAllImages
		we set an additional flag kProfilePending, to denote that
		we're actually trying to obtain a profile.  This will help
		us interpret what to do later when we look to see if we
		should match.
    */
    switch (il_color_matching_on(kCheckState))
    {
    	case	kColorMatchingOff:
			il_set_container_color_matching(ic,kColorSyncOFF);
    		break;
    	default:
    	case	kColorMatchIfProfile:
			il_set_container_color_matching(ic, kProfilePending | kProfilePresent);
    		break;
    	case	kColorMatchAllImages:
			il_set_container_color_matching(ic, kProfilePending | kProfileDefault);
    		break;
    }

    /*
		Create a new instance for this profile request.
    */
    profile_req = PR_NEWZAP(IL_ProfileReq);
    if (!profile_req) return NULL;
    
    /*
		Copy the image context, and the net context.
		The net context is really an object with methods
		for creating and using urls.
    */
    profile_req->net_cx = net_cx->Clone();
	if (!profile_req->net_cx)
	{
		PR_FREEIF(profile_req);
		return NULL;
	}
    
	/*
		Get a container - could have been previously obtained,
		so we'll look for it in the cache first.
	*/
    ip = il_get_icc_profile_container(	img_cx,
	    								net_cx,
	    								cache_reload_policy,
	    								profile_url );
    if (!ip)
    {
        ILTRACE(0,("il: MEM ip_container"));
        
        il_delete_icc_profile_request(profile_req);
        return NULL;
    }

    /*
		Give the client a reference to this container.
		The client is the request.  It gets returned
		from here, and stored in the *real* client: the caller.
    */
    profile_req->ip = ip;
    profile_req->ic = ic;

    /*
		Add the client (the request) to the end of the container's
		client list. Once all clients detach from this container,
		it can be deleted.
    */
	if (!ip->lclient)
		ip->clients = profile_req;
	else
		ip->lclient->next = profile_req;
		
	ip->lclient = profile_req;
	
    /*
    	If the profile has already been requested
    */
	if (ip->state != ICCP_VIRGIN)
	{
        switch (ip->state)
        {
	        case ICCP_BAD:
	        case ICCP_INCOMPLETE:
	        case ICCP_ACCUMULATING:
	        case ICCP_MISSING:
	            break;

	        case ICCP_COMPLETE:
	            /*
	        		This is a cached profile that has already
	        		been stored.  Share the wealth.
	            */
	            il_icc_profile_complete_notify(ip);

	            break;

	        case ICCP_START:
	        case ICCP_STREAM:
	        case ICCP_ABORT_PENDING:
	        case ICCP_NOCACHE:
	            break;
	            
	        default:
	            PR_ASSERT(0);
	            NS_IF_RELEASE(profile_req->net_cx);
	            PR_FREEIF(profile_req);
	            return NULL;
        }

		/* NOCACHE falls through to be tried again */
        if (ip->state != ICCP_NOCACHE)
             return profile_req;
    }

    
    /*
    	Set the content type in the URL struct, for the request.
    */
	ip->url->SetContentType("*");
	
	/*
		Record the fact that we are calling NetLib to load a URL.
	*/
	ip->state			= ICCP_START;
    ip->is_url_loading	= PR_TRUE;
	#ifdef DEBUG
    ip->start_time		= PR_Now();
	#endif
    ILTRACE(1,("il: net request for %s", profile_url));

    /*
    	Create a new NetReader, giving it our container.
    */
	reader = IL_NewNetReader( NULL, ip);
	if (!reader)
	{
		il_delete_icc_profile_request(profile_req);
        return NULL;
	}
	
    err = ip->net_cx->GetURL(ip->url, cache_reload_policy, reader);
    
    /*
		Set the container's flag to reflect that we've gotten this far.
	*/
    if (!err)
		il_set_container_color_matching(ic,kProfileFromURL);

    /*
    	Release reader, GetURL will keep a ref to it.
    */
    NS_RELEASE(reader);
    
    /*
    	If there's a NetLib error, we'll find out later.
    	If we try to clean up now, we may pull the rug
    	out from under our feet.
    */
	return profile_req;
}

/*	******************************************************************************
	il_color_matching_available
	
	Desc:	Return whether ColorSync is installed.
			This yields optimizations for image decoders, so they
			know whether to keep track of icc profile data.
*/
PRBool
il_color_matching_available	(	void )
{
	return (isColorSyncInstalled);
}

/*	******************************************************************************
	il_color_matching_on
	
	Desc:	The user can turn off and on color matching, by visiting the
			preferences panel.  We should do our best to repsect this
			setting, checking at key entrypoints.
			
			One way to avoid unnecessary disk accesses is for us to
			accept two levels of checking:
				1.	Read the prefs from disk, set the global value.
				2.	Just check to global value.
			
			Callers can use the first level of checking at the beginning of
			a series of operations, and then simply use the second level
			for each operation.
*/
int32
il_color_matching_on	(	ColorSyncPrefsCheck		check_level )
{
	int		got_prefs;
	int32	pref_value;

	/*
		We can check another global first, to determine
		whether ColorSync is even active.  If it's not
		then the "isColorSyncOn" global will be off.
	*/
    if (isColorSyncInstalled)
    {
	    /*
	    	If we're being asked to reads the prefs, do so.
	    */
    	if (check_level == kReadPref)
    	{
	    	got_prefs = PREF_GetIntPref("browser.color_management", &pref_value);
	    	if (got_prefs == PREF_OK)
	    	{
	    		if (pref_value > kColorMatchAllImages ||
	    			pref_value < kColorMatchingOff)
	    			got_prefs = PREF_NOT_INITIALIZED;
	    		else
	    			isColorSyncOn = pref_value;
			}
			
			/* Deal with the error case */
	    	if (got_prefs != PREF_OK)
	    	{
	    		/*
	    			This happens the first time we try to read the
	    			pref and it's never been set.  Try setting it
	    			to the preferred default value, which is to
	    			match images with profiles.
	    		*/
	    		pref_value = kColorMatchIfProfile;
	    		got_prefs = PREF_SetIntPref("browser.color_management", pref_value);
				
	    		/*
	    			If we set it ok, then return what we set.
	    			If not, turn it off.
	    		*/
	    		if (got_prefs >= PREF_OK)
	    			isColorSyncOn = pref_value;
	    		else
	    			isColorSyncOn = kColorMatchingOff;
	    	}
	    }
    }
    else isColorSyncOn = kColorMatchingOff;
    
    return (isColorSyncOn);
}

/*	******************************************************************************
	il_get_container_color_matching
	
	Desc:	The passed param is an image container,
			into which we may have previously stored flags.

			We want to return a non-zero value if matching
			should occur.  This involves inspecting the flags
			and returning the correct value depending on
			whic flags are set.  If certain flags are set,
			we want to return a zero value, indicating that
			no matching should take place on this container's image.
			
*/
uint8
il_get_container_color_matching	(	 il_container			*ic )
{
	PR_ASSERT(ic);
	
	/*
		Take care of obvious negative case(s).
		NOTE that if color matching is turned off, we
		still should reflect what the container thinks,
		this helps interpret the state of cached images.
	*/
	if (ic == NULL) return 0;
    
	/*
		The order of operations is important - these are in
		order of precedence.
	*/
    if (ic->icc_profile_flags & (kColorSyncOFF | kProfileNONE | kProfileInvalid))
    	return 0;
	if (ic->icc_profile_flags & kProfileEmbedded) return kProfileEmbedded;
	if (ic->icc_profile_flags & kProfileFromURL) return kProfileFromURL;
	
	/*
		This is a wierd one, because we'll return true but we don't
		have a profile yet.  This is ok, because the final act is to
		check the profile reference anyway, and if we haven't got a
		valid profile by then we don't attempt to set up a match.
	*/
	if (ic->icc_profile_flags & kProfilePending) return kProfilePending;

	/*
		When the container was created, we set whether the prefs had
		specified "match images with No profiles with the default profile".
		We don't want to go reading the prefs now, so rely on that setting.
	*/
	if (ic->icc_profile_flags & kProfileDefault) return kProfileDefault;
	if (ic->icc_profile_flags & kProfilePresent) return kProfilePresent;
	
    return 0;
}

/*	******************************************************************************
	il_stop_icc_profile_request
	
	Desc:	We're being told that the request has been suspended.
			We could attempt to actually remove the request and delete
			the profile container if no requests remain active, but
			that shouldn't be necessary if the image container is
			going away at some point anyway.
*/
void
il_stop_icc_profile_request	(	IL_ProfileReq		*pr )
{
	PR_ASSERT(pr);

    pr->stopped = TRUE;
}

/*	******************************************************************************
	il_remove_icc_profile_request
	
	Desc:
			A request (client) has been either satisfied or diminished.
			If this is the last client, we should delete ourselves,
			and make sure the profile reference, if any, is closed.
			We need to:
			
			¥ delete the data structures allocated by the IL_ProfileReq:
				net_cx
			¥ tell the profile container to remove us as a client
			¥ hook up the next IL_ProfileReq to the one that's pointing to us?
			  (why do we have a list of these anyway?)
			¥ point back at the image container that called us and remove it's
			  profile reference?  Or should we let the profile container do this?
			  Probably, since it can tell whether there are any more refs to the
			  actual profile, and if not, close it.
			¥ kill ourselves.
*/
void
il_remove_icc_profile_request	(	IL_ProfileReq		*pr )
{
	PRBool			found = FALSE;
	ip_container	*ip;
	il_container	*ic;
	IL_ProfileReq	*requests_outstanding = NULL;
	IL_ProfileReq	*previous_request;
	
	/*
		First, back-link to the image container and the profile container.
	*/
	PR_ASSERT(pr);
	if (pr == NULL)
		return;
	
	/*
		If there is an image container, that means the request was
		successfully made.  If the image container has a profile
		reference, we need to decrement the reference count and
		potentially close the profile;
	*/	
	ic = pr->ic;
	ip = pr->ip;
	if (ic && ic->icc_profile_ref)
	{
		il_remove_icc_profile_ref(ic->icc_profile_ref, ip);
	}

	/*
		Check for a profile container and search it for active requests.
	*/
	if (ip != NULL)
	{
		requests_outstanding = ip->clients;
		previous_request = NULL;
		
		while (requests_outstanding)
		{
			if (pr == requests_outstanding)
			{
				if (previous_request)
				{
					/*
						Remove the link by pointing the current
						at the next one, and then fixing the
						previous's link to jump over the current.
					*/
					requests_outstanding = requests_outstanding->next;
					previous_request->next = requests_outstanding;
				}
				else
				{
					ip->clients = requests_outstanding = requests_outstanding->next;
				}
				
				found = TRUE;
				break;
			}
			else
			{
				previous_request = requests_outstanding;
				requests_outstanding = requests_outstanding->next;
			}
		}
		
		/*
			If the request wasn't found, there's something wrong.
			Otherwise, delete it.
		*/
		PR_ASSERT(found);
	}
	
	/*
		Delete the request, whether or not it was "found".
		It is possible that the profile container never existed.
	*/
	il_delete_icc_profile_request(pr);
	
	/*
		If we didn't find the container, or if
		there are still requests, then we're done.
	*/
	if (!found) return;
	if (requests_outstanding) return;
	
	/*
		We can delete the entire container.
	*/
	il_delete_icc_profile_container(ip);
}

/*	******************************************************************************
	il_waiting_for_icc_profile
	
	Desc:	During image download, the NetReader will call a WriteReady
			function.  If the reader isn't ready, it should return 0, in
			which case the netlib should poll.  We need this function to
			check to see if a pending profile for the image has been obtained,
			otherwise if it arrives after the image starts decoding it's too
			late.  (This has happened in practice - for small images that
			reference larger profiles.)
			
	In:		Image Container, from which we'll reference:
				icc_profile_req				if a profile request was made.
				icc_profile_req->ip			then we check the container
	
	Out:	If the image container's profile request has a profile container
			which is still loading, we return TRUE, otherwise, FALSE.
*/
PRBool
il_waiting_for_icc_profile	(	il_container	*ic )
{
	PRBool			waiting = FALSE;
	PR_ASSERT(ic);
	/*
		Check for icc profile request
	*/
	if (ic->icc_profile_req)
	{
		/*
			Check if the request has been stopped
		*/
		if (!ic->icc_profile_req->stopped)
		{
			/*
				Check for icc profile request's profile container
			*/
			if (ic->icc_profile_req->ip)
			{
				/*
					Check if the profile container is still loading,
					AND that it's not aborting.  We don't need to
					wait if the profile isn't going to exist.
				*/
				waiting = (ic->icc_profile_req->ip->is_url_loading);
				if (waiting)
					waiting = ic->icc_profile_req->ip->state != ICCP_ABORT_PENDING;
			}
		}
	}
	
	return (waiting);
}

/*	******************************************************************************
	il_setup_color_matching_session
	
	Desc:	Called to establish the color matching session betweem the image's
			color space and the 'display' color space, which is stored in the
			image->header.  This is done when the image's pixmap is allocated
			in the front end.  Anyway, we'll call the front end here as well,
			until ColorSync is across more than just Mac, Windows, and Mach.
			
	In:		Image Container, from which we'll reference:
				img_cb						to make the call to the front end
				src_header->color_space		set by image decoder
				image->header.color_space	set by IMGCBIF_NewPixmap
	
	Out:	Nothing returned, but if the call is successful,
			the image container's icc_matching_session field will be set.
*/
void
il_setup_color_matching_session	(	il_container	*ic )
{
    NI_ColorSpace	*src_space;
    NI_ColorSpace	*dst_space;
#if	CACHE_COLOR_WORLDS
    void			*cached_session = NULL;
#endif /* CACHE_COLOR_WORLDS */

	PR_ASSERT(ic);
	
    src_space = ic->src_header->color_space;
    dst_space = ic->image->header.color_space;
    
    if ( src_space->icc_profile_ref && dst_space->icc_profile_ref )
    {
#if	CACHE_COLOR_WORLDS
		cached_session = il_find_color_matching_session(src_space->icc_profile_ref,
														dst_space->icc_profile_ref);
		if (cached_session == NULL)
#endif /* CACHE_COLOR_WORLDS */
    
#ifdef STANDALONE_IMAGE_LIB
    	ic->icc_matching_session = ic->img_cb->SetupICCColorMatching( NULL,
    										src_space->icc_profile_ref,
    										dst_space->icc_profile_ref );
    										 
#else
    	ic->icc_matching_session = IMGCBIF_SetupICCColorMatching(ic->img_cb, NULL,
    										src_space->icc_profile_ref,
    										dst_space->icc_profile_ref );
#endif /* STANDALONE_IMAGE_LIB */
	
	}
}

/*	******************************************************************************
	il_icc_matching_complete
	
	Desc:
			The image container previously established a matching session,
			which is now complete.  Since the data is ready for display
			and must be reloaded and re-matched if it is kicked out of the
			image cache, we should free the memory taken up by the matching
			session now, to keep as many images in the cache.
			
			Alternatively, since there may be several instances of
			common color worlds, we could cache them by source and dest
			profiles, as long as the dest profiles are limited to a small
			number - such as the system profile and perhaps a few others.
			(By AVID.)  Since this is the case, we can enable caching to
			see if that helps performance.
*/
void
il_icc_matching_complete	(	il_container	*ic )
{
	PRBool			found = FALSE;
	
	/*
		First, check the image container and the matching session.
	*/
	PR_ASSERT(ic);
	if (ic->icc_matching_session == NULL)
		return;
		
#if	CACHE_COLOR_WORLDS
	if (!il_cache_color_matching_session(ic))
#else

#ifdef STANDALONE_IMAGE_LIB
	ic->img_cb->DisposeICCColorMatching( NULL, ic->icc_matching_session );						 
#else
	IMGCBIF_DisposeICCColorMatching(ic->img_cb, NULL, ic->icc_matching_session );
#endif /* STANDALONE_IMAGE_LIB */
	
#endif /* CACHE_COLOR_WORLDS */

	/* Zero the session reference */
	ic->icc_matching_session = NULL;
	
	/* Flag that the image was matched */
	il_set_container_color_matching(ic, kContainerMatched);
}

/*	******************************************************************************
	il_icc_profile_data_notify
	
	Desc:
			An image decoder is calling us with icc profile data.
			We may get called more than once per profile, in the
			case of GIF.  We have to either accumulate data or
			take it all at once.  Once accumulated data is complete
			or a single complete block arrives, we can open the
			profile, which is done via il_get_icc_profile_ref.
			
			This is a separate path for getting profiles than
			from a NetReader, and the data must be transferred to
			the appropriate profile container.
			
			The message param tells us whether we are getting
			1.	The first of a series of blocks
			2.	The nth of a series of blocks - COULD BE FIRST!
			3.	The last or a series, or no block, but there are
				no more blocks to accumulate.
			4.	A single block - no accumulation necessary.
*/
void
il_icc_profile_data_notify	(	il_container		*ic,
								unsigned char		*icc_data_block,
								uint32				icc_data_len,
								ICCDataNotification	message )
{
	ip_container	*ip						= NULL;
	IL_ProfileReq	*profile_req			= NULL;
	void			*profile_ref			= NULL;
	unsigned char	*icc_data_ptr			= NULL;
	PRBool			get_profile_ref			= PR_FALSE;
	PRBool			get_profile_container	= PR_FALSE;
	PRBool			add_to_container		= PR_FALSE;
	int				err						= 0;
	#define			kICCDataIncrement		1024

	/*
		First, check the image container.
	*/
	PR_ASSERT(ic);
	
	/*
		Let's record the fact that this image contains a profile.
		That way, we can tell later on whether we need to kick
		it out of the cache if it WAS NOT matched.
    */
	il_set_container_color_matching(ic,kProfileEmbedded);

	/*
		If we're not supposed to match this guy, just return.
		BUT WAIT!  If this is a notification that icc profile
		data has been accumulated for us in memory, we need to
		deallocate it now - or else it just stays around.
    */
    if (!il_color_matching_on(kCheckState) || !il_get_container_color_matching(ic))
    {
    	if (message == kICCProfileData)
    		PR_FREEIF(icc_data_block);
    	return;
    }

	/*
		Next, check if the profile request and container already exist.
	*/
	profile_req = ic->icc_profile_req;
	if (profile_req) ip = profile_req->ip;
		
	/*
		Figure out what we have to do.
	*/
	switch (message)
	{
		case	kFirstICCDataBlock:
			/* Alloc container & request */
			get_profile_container = PR_TRUE;
			break;
		case	kNthICCDataBlock:
			/* Realloc data ptr, potentially get request & container */
			add_to_container = PR_TRUE;
			get_profile_container = (ip == NULL);
			break;
		case	kICCDataBlocksComplete:
			/* Container & request exist, just get profile ref. */
			get_profile_ref = PR_TRUE;
			add_to_container = (icc_data_block != NULL && icc_data_len != 0);
			break;
		case	kICCProfileData:
			/* Alloc container & request, then get profile ref. */
			get_profile_container = PR_TRUE;
			get_profile_ref = PR_TRUE;
			/*
				This data is 'ours', by convention.
				WE must free it when we're done.
			*/
			icc_data_ptr = icc_data_block;
			break;
	}
	
	/*
		It's possible that we're being called with
		NULL for data, but only in the case that
		we're being told that the profile is complete.
	*/
	if (get_profile_container || add_to_container)
	{
		PR_ASSERT(icc_data_block);
		PR_ASSERT(icc_data_len);
		
		if (icc_data_block == NULL || icc_data_len == 0)
			return;
	}
	
	if (get_profile_container)
	{
		/*
			Create a new profile container, first check if there is one already.
			This can happen in the case when an image contains an embedded
			profile and also has an associated profile.  If this happens we stop
			accumulating the netborne data and give preference to the embedded data.
		*/
		if (ip != NULL)
		{
			il_remove_icc_profile_request(profile_req);
			ic->icc_profile_req = NULL;
			profile_req = NULL;
		}

		/*
			Get a profile container for the embedded profile.
		*/
		ip = il_get_icc_profile_container(ic->img_cx, NULL, NET_DONT_RELOAD, NULL);
		
		if (ip)
		{
			/*
				If this ptr is set, it means we own it.
			*/ 
			if (icc_data_ptr != NULL)
			{
				ip->profile_data		= icc_data_ptr;
				ip->profile_data_size	= icc_data_len;
				ip->profile_buffer_size	= icc_data_len;
				ip->state				= ICCP_COMPLETE;
			}
			
			/*
				Allocate a profile request.
				This is how the image container is able to
				get to the profile container.  For netborne
				profiles, there can be multiple requests for
				a single profile container.
			*/
			
	    	profile_req = PR_NEWZAP(IL_ProfileReq);
	    	if (profile_req)
	    	{
	    		profile_req->ip = ip;			/* This request's container */
	    		profile_req->ic = ic;			/* The request's client */
	    		profile_req->net_cx = NULL;		/* There was no net request */
	    		profile_req->stopped = FALSE;	/* Never started... */
	    		profile_req->next = NULL;		/* This isn't part of the list */
	    		
	    		ic->icc_profile_req = profile_req;	/* Complete the link */
	    		ic->icc_profile_ref = profile_ref;	/* Set this NULL for now */
	    		
			    /*
					Add the client (the request) to the end of the container's
					client list. Once all clients detach from this container,
					it can be deleted.
			    */
				ip->clients = profile_req;
				ip->lclient = profile_req;
	    	}
	    	else err = NS_ERROR_OUT_OF_MEMORY;
		}
		else
		{
			/*
				We're kind of in a bad way here, because we may have to keep
				eating data and we won't have a container for it.  In this
				case, we just do nothing, and future calls here will do nothing.
			*/
			err = NS_ERROR_OUT_OF_MEMORY;
		}
	}
	
	/*
		If we're supposed to accumulate this data,
		AND there's a container for it... go ahead.
	*/
	if (add_to_container && ip && !err)
	{
		err = il_add_icc_profile_block(	ip, kICCDataIncrement,
										icc_data_block,
										icc_data_len );
	}
	
	/*
		If we're supposed to get a profile ref,
		AND there's a container for it....
	*/
	if (get_profile_ref && ip && !err)
	{
		/*
			Open the profile - get a reference to it.
		*/
		profile_ref = il_get_icc_profile_ref(ip);
		
		if (profile_ref)
		{
	    	ic->icc_profile_ref = profile_ref;
		}
		else
		{	/*
				This could be a profile error, but
				what's important is that we flag it.
			*/
			err = NS_ERROR_OUT_OF_MEMORY;
		}
	}
	
	/*
		Check for errors - get rid of the container
		and/or data if they exist.
	*/
	if (err)
	{
		if (ip)
		{
			/*
				This routine frees the request,
				the data, and the ref.
			*/
			il_delete_icc_profile_container(ip);
		}
		else if (icc_data_ptr)
		{
			/*
				Get rid of our own data if we couldn't
				allocate a container for it.
			*/
			PR_FREEIF(icc_data_ptr);
		}
		
		/*
			We should treat this as an image with
			a specific instruction not to match.
			This will keep the default matching from
			occurring.
	    */
		il_set_container_color_matching(ic,kProfileInvalid);
	}
}

/***************************** Private Entry Points ******************************/
#pragma mark -
/*************************** Profile Container Routines **************************

	These functions are mostly housekeeping routines to ensure that image
	containers (1), profile requests (2), and profile containers (3) all have
	the right information about each other.

	(1)	il_container_struct
	(2) IL_ProfileReq.
	(3)	ip_container_struct

*/

/*	******************************************************************************
	il_remove_icc_profile_ref
	
	Desc:
			A client of the profile container doesn't need the profile ref
			any more.  So we can close the profile if nobody else has
			a reference.
*/
static void
il_remove_icc_profile_ref	(	void	*ref,	ip_container	*ip)
{
	PR_ASSERT(ip);
	PR_ASSERT(ip->profile_ref);
	PR_ASSERT(ip->profile_ref_count);
    /*

    */
    if (ip->profile_ref && ip->profile_ref_count > 0)
    {
    	ip->profile_ref_count -= 1;
    	
    	if (ip->profile_ref_count <= 0)
    	{   	
#ifdef STANDALONE_IMAGE_LIB
	    	ip->img_cb->CloseICCProfileRef( NULL, ip->profile_ref );
#else
	    	IMGCBIF_CloseICCProfileRef(ip->img_cb, NULL, ip->profile_ref );
#endif /* STANDALONE_IMAGE_LIB */

        	ip->profile_ref = 0;
        	ip->profile_ref_count = 0;
        }
    }
}

/*	******************************************************************************
	il_get_icc_profile_ref
	
	Desc:
			Given a profile container, create a "reference"
			to the profile contents.  In ColorSync parlance,
			this is called Opening the profile and getting
			a CMProfileRef.  We will make this type opaqe
			for now.
			
	In:		The profile container to create a profile ref for
	
			Whether the profile came as a result of a NetReader.
			If not, it is a memory-based profile, from an image decoder.
*/
static void*
il_get_icc_profile_ref	(	ip_container		*ip )
{
	char			*filename	= NULL;
	URL_Struct		*url_struct;
	void*			profile_ref = NULL;
    
    /*
    	We don't want to look in the cache more than once (if we've found it),
    	because FindURLInCache allocates memory and stores it in the URL_Struct.
    	It doesn't deallocate whatever's there first.  So if we've already
    	opened a profile ref, we should just clone it.
    */
    if (ip->profile_ref)
    {
    	ip->profile_ref_count += 1;
#ifdef STANDALONE_IMAGE_LIB
	    ip->img_cb->CloneICCProfileRef( NULL, ip->profile_ref );
#else
	    IMGCBIF_CloneICCProfileRef(ip->img_cb, NULL, ip->profile_ref );
#endif /* STANDALONE_IMAGE_LIB */
        return (ip->profile_ref);
    }
    
	if (ip->is_file_based)
	{
    	url_struct = ip->url->GetURLStruct();
    	filename = url_struct->cache_file;
    	 
		if (filename)
		{
			/*
				Get a profile ref - tell the IMGCB Interface it's disk based.
			*/
#ifdef STANDALONE_IMAGE_LIB
		    profile_ref = ip->img_cb->OpenICCProfileFromDisk( NULL, filename );
#else
		    profile_ref = IMGCBIF_OpenICCProfileFromDisk(ip->img_cb, NULL, filename );
#endif /* STANDALONE_IMAGE_LIB */
		}
	}
	/*
		If this is not a file-based profile,
		it must be an in-memory profile.
	*/
	else if (ip->profile_data != NULL)
	{
		/*
			Get a profile ref - tell the IMGCB Interface it's memory based.
		*/
#ifdef STANDALONE_IMAGE_LIB
	    profile_ref = ip->img_cb->OpenICCProfileFromMem( NULL, ip->profile_data );
#else
	    profile_ref = IMGCBIF_OpenICCProfileFromMem(ip->img_cb, NULL, ip->profile_data );
#endif /* STANDALONE_IMAGE_LIB */
	}
	
	/*
		If we got it, store it in our structure, and increment the ref count.
	*/
	if (profile_ref)
	{
		ip->profile_ref_count = 1;
    	ip->profile_ref = profile_ref;
    } 
	
	return (profile_ref);
}

/*	******************************************************************************
	il_get_icc_profile_container
	
	Desc:	Looks for the profile url in the disk cache.

*/
static ip_container*
il_get_icc_profile_container (	IL_GroupContext		*img_cx,
		            			ilINetContext		*net_cx,
								NET_ReloadMethod	cache_reload_policy,
								const char*			profile_url )
{
	ilIURL					*url = NULL;
	ip_container			*ip	= NULL;
	JMC_EXCP				jmc_excp = NULL;	/* Macro def covers both environments */
	
	/*
		If this is a netborne profile, we should
		handle it differently than an embedded profile.
	*/
	if (net_cx)
	{
		url = net_cx->CreateURL(profile_url, cache_reload_policy);
	    if (!url)
	    {
	    	return NULL;
	    }
	    
	    /*
	    	If this profile has already been loaded,
	    	then we can use it.  We have to recover
	    	the container from our list of container
	    	pointers.
	    */      
		if (net_cx->FindURLInCache(url))
		{
			ip = il_find_icc_profile_container(url);
		}
		
	    if (ip != NULL)
	    {
	    	/*
	    		What if the img_cx isn't the same?
	    		Will we have not gotten a hit?
	    	*/
	    	return (ip);
	    }
    } 
    
    /*
    	If we didn't find it, it may have been deleted
    	even though the file is still in the cache.
    	Or it may be an embedded profile. That's ok.
    	We just allocate a new container.
    */
	ip = PR_NEWZAP(ip_container);

	if (ip == NULL) 
	{
		ILTRACE(0,("il: MEM il_get_profile_container"));
    	NS_RELEASE(url);
		return 0;
	}
	
    /*
    	Before we assign all the other fields, copy the
    	JMC CallBack Interface.  This can yield an exception,
    	so we have to check for that and close up shop if thrown.
    	If we succeed, we increment the CBIF's reference count.
     */
	ip->img_cb = img_cx->img_cb;
	ADD_IMGCBIF_REF(ip->img_cb,jmc_excp);
	if (CHECK_JMC_EXCP(jmc_excp))
	{
		DELETE_JMC_EXCP(jmc_excp);
		PR_FREEIF(ip);
		return NULL;
	}

    /*
    	Assign the url object to the profile container.
    	and store the context that actually
    	initiates and controls the transfer.
     */
	ip->url					= url;
	ip->is_url_loading		= FALSE;
	ip->is_file_based		= FALSE;
    ip->state				= ICCP_VIRGIN;
	ip->profile_data		= NULL;			/* The actual data. */
	ip->profile_data_size	= 0;			/* How much accumulated */
	ip->profile_buffer_size = 0;			/* How big the accumulation buffer is */
	ip->profile_ref			= NULL;			/* Opened profile. */
    ip->profile_ref_count	= 0;			/* How many il_container's use us? */
	ip->expires				= 0; 			/* no expiration date - yet */
    ip->net_cx				= (net_cx ? net_cx->Clone() : NULL); /* Context initiating this transfer. */
    ip->clients				= NULL;			/* List of clients of this container. */
    ip->lclient				= NULL;			/* Last client in the client list. */
    ip->next				= NULL;			/* For global profile container list. */
    ip->prev				= NULL;			/* For global profile container list. */   
    
    /*
    	Now add this to the global profile container list.
    	BUT only if it is a netborne profile, since the
    	only way we have of uniquely id'ing them is by url.
    */
    if (url)
    {
	    if (il_global_profile_container_list)
	    {
	    	il_global_profile_container_list->prev = ip;
	    	ip->next = il_global_profile_container_list;
	    	il_global_profile_container_list = ip;
	    }
	    else
	    {
	    	il_global_profile_container_list = ip;
	    }
    }
    
    /*
    	Return the container.
    */
    return (ip);
}

/*	******************************************************************************
	il_find_profile_container
	
	Desc:
			We're supposed to be able to locate a profile container that
			has this url.  Look through our list and compare the url structs.
*/
static ip_container*
il_find_icc_profile_container(	ilIURL			*url )
{
	ip_container	*ip;
//	URL_Struct		*urls;
	
	PR_ASSERT(url);
	
	/*
		Make sure we're not comparing NULL.
	*/	
	if (url->GetAddress() == 0)
		return (NULL);

	/*
		Go through a static list of pointers and see which
		one matches on the url address.
	*/	
	ip = il_global_profile_container_list;
	
	while (ip)
	{
		if (ip->url) 
		{   	
	    	/*
	    		Compare what?  Address I guess.
	    	*/
			if (strcasecomp(ip->url->GetAddress(), url->GetAddress()) == 0)
				return(ip);
		}
		ip = ip->next;
	}
	
	return (NULL);
}

/*	******************************************************************************
	il_delete_icc_profile_container
	
	Desc:
			We're all washed up.
			
			The only tricky thing here is not to do anything if we're
			still being pumped to by NetLib.  This can happen if the
			request is removed while we're still loading.  As long as
			the state flag is set to ICCP_ABORT_PENDING, then eventually
			the stream functions will call us again.
*/
static void
il_delete_icc_profile_container	(	ip_container		*ip )
{
	ip_container	*container_in_list;
	IL_ProfileReq	*request;
	PRBool			found		= FALSE;
	PRBool			net_borne	= FALSE;
	JMC_EXCP		jmc_excp	= NULL;;
	
	PR_ASSERT(ip);
	
	/*
		Wait until the stream is done.  We should get called again.
	*/
	if (ip->is_url_loading)
	{
		ip->state = ICCP_ABORT_PENDING;
		return;
	}
		
	/*
		¥	If there are opened profile refs, we have to close them.
			This should be done first, by linking back to the
			clients (requests) and the image containers that made them.

		¥	Free all structures we reference
			ilIURL			url
			ilINetContext	net_cx
			IL_ProfileReq	*clients - a list
    		IMGCBIF			*img_cb
			
		¥	Kill all clients (IL_ProfileReqs) in list
	*/

	/*
		Get rid of the actual profile reference.
	*/
    while(ip->profile_ref && ip->profile_ref_count)
    	il_remove_icc_profile_ref(ip->profile_ref, ip);
	
	/*
		Get rid of the url object and the net context.
	*/
	net_borne = (ip->net_cx != NULL);
	if (net_borne)
	{
		NS_IF_RELEASE(ip->url);
		NS_IF_RELEASE(ip->net_cx);
	}

	/*
		While we are probably being called as a result of
		the last request (client) having been removed,
		it is still safer to assume nothing and go ahead
		and delete (NOT remove) any outstanding requests.
	*/		
	while (ip->clients)
	{
		request = ip->clients;
		ip->clients = ip->clients->next;
		il_delete_icc_profile_request(request);	
	}
	
	/*
		Use our macros to resolve differences between
		the way the img callback interface is handled
		when executing a standalone version of the imglib
		vs. a linked-in version.
	*/
	DELETE_IMGCBIF(ip->img_cb,jmc_excp);
	if (CHECK_JMC_EXCP(jmc_excp))
		DELETE_JMC_EXCP(jmc_excp);
		
    /*
    	Remove ourselves from the global profile container list.
    	NOTE that only net-borne profiles are cached.
    	This is because the only way we have of uniquely id'ing
    	them is by url (short of opening, checksumming, etc.).
    */
    if (net_borne)
    {
	    container_in_list = il_global_profile_container_list;
	    while (container_in_list)
	    {
		    if (container_in_list == ip)
		    {
		    	found = TRUE;
		    	container_in_list = ip->next;

	    		/* If there is a prev, we're somewhere in the middle */
	    		if (ip->prev)
	    		{
	    			(ip->prev)->next = container_in_list;
	    		}
	    		else	/* If there is no prev, we're at the beginning */
	    		{
	    			il_global_profile_container_list = container_in_list;
	    		}
	    		/*
	    			If there is one beyond the container we're deleting
	    			hook it's back pointer up.
	    		*/
	    		if (container_in_list)
	    			container_in_list->prev = ip->prev;
		    }
		    else
		    {
		    	container_in_list = container_in_list->next;
		    }
		}
		PR_ASSERT(found);
	}
	
	/*
		Deallocate any in-memory profile data.
	*/
	PR_FREEIF(ip->profile_data);

	/*
		Finally, we can shovel dirt on top of ourselves.
	*/
	PR_FREEIF(ip);
}

/*	******************************************************************************
	il_delete_icc_profile_request
	
	Desc:
			The request is no longer needed - either it's been satisfied
			or it doesn't need to be.
			We assume that the profile container that this request points
			to has already had this request removed from it's client list.
			Our job is simply to delete the request structure and it's
			allocated substructures:
			
				ilINetContext	*net_cx
*/
static void
il_delete_icc_profile_request	(	IL_ProfileReq		*pr )
{
	PR_ASSERT(pr);
	/*
		We have to delete an object, and then ourselves.
	*/
	NS_IF_RELEASE(pr->net_cx);
	PR_FREEIF(pr);
}

/*	******************************************************************************
	il_icc_profile_stopped
	
	Desc:	Answer the question:
			Have all profile requests for this container been stopped?
*/
static PRBool
il_icc_profile_stopped (ip_container *ip)
{
    IL_ProfileReq *profile_req;
    for (profile_req = ip->clients; profile_req; profile_req = profile_req->next)
    {
        if (!profile_req->stopped)
            return FALSE;
    }

    /* All clients must be stopped for profile container to become dormant. */
    return TRUE;
}

/*	******************************************************************************
	il_icc_profile_complete_notify
	
	Desc:
			The profile has finished decoding.
			The 'clients', or requests, of this profile
			need to know that is is done.
*/
static void
il_icc_profile_complete_notify(ip_container *ip)
{
    IL_ProfileReq	*profile_req;
    il_container	*ic;
    
    ip->expires = (ip->url)->GetExpires(); /* get the URL's expiration date */

	PR_ASSERT(ip->clients);
    for (profile_req = ip->clients; profile_req; profile_req = profile_req->next)
    {
    	/*
    		Each profile request has an associated image container:
    		the guy who made the request for the profile in the first place.
    		Let's set these containers' profile data references to
    		the same one in the passed profile containers'.
    	*/
    	ic = profile_req->ic;
		PR_ASSERT(ic);
		if (ic)
		{
    		ic->icc_profile_ref = il_get_icc_profile_ref(ip);
    		/*
				Tell the image container it has an associated profile.
			*/
			if (ic->icc_profile_ref != NULL)
				il_set_container_color_matching(ic, kProfileFromURL);
		}
    }
}

/*	******************************************************************************
	il_add_icc_profile_block
	
	Desc:
			There are several scenarios where we have to accumulate
			profile data.  Thus a subroutine to handle the allocation
			and reallocations involved, updating buffer sizes, etc.
			
			ip					the profile container
			buffer size			size of each alloc
			data				data to add
			data size			size of data being added
*/
static int
il_add_icc_profile_block(	ip_container	*ip,
							uint32			buffer_size,
							unsigned char	*data,
							uint32			data_size )
{
	uint8			*buffer = NULL;
	int				err = 0;

	PR_ASSERT(ip);
	
	if (ip == NULL) return -1;
	
	/*
		First time through - easy.  Just alloc and go.
	*/
	if (ip->profile_data == NULL)
	{
		buffer = (uint8*) XP_ALLOC(buffer_size);
		if (buffer != NULL)
		{
			ip->profile_data = buffer;
			ip->profile_data_size = data_size;
			ip->profile_buffer_size = buffer_size;
			XP_MEMCPY(buffer, data, data_size);
			ip->state = ICCP_ACCUMULATING;
		}
		else	err = NS_ERROR_OUT_OF_MEMORY;	// boomer!
	}
	else
	{
		/*
			Realloc if we're going to exceed our buffer size.
		*/
		if (data_size + ip->profile_data_size > ip->profile_buffer_size)
		{
			buffer_size += ip->profile_buffer_size;
			buffer = (uint8*) XP_REALLOC(ip->profile_data, buffer_size);
			if (buffer != NULL)
			{
				ip->profile_data = buffer;
				ip->profile_buffer_size = buffer_size;
				buffer += ip->profile_data_size;
				XP_MEMCPY(buffer, data, data_size);
				ip->profile_data_size += data_size;
			}
			else	err = NS_ERROR_OUT_OF_MEMORY;	// boomer!
		}
		else
		/*
			Just copy it in.
		*/
		{
			buffer = ip->profile_data + ip->profile_data_size;
			XP_MEMCPY(buffer, data, data_size);
			ip->profile_data_size += data_size;
		}
	}
	
	return err;
}

/*	******************************************************************************
	il_set_container_color_matching
	
	Desc:	Throughout it's journey through our code, this container
			may experience many evolutions.  These are recorded here,
			for final interrogation at the point where matching is set up.
*/
static void
il_set_container_color_matching (	il_container	*ic,
									uint16			matching_flag )
{
	if (ic)
		ic->icc_profile_flags |= matching_flag;
}


#if	CACHE_COLOR_WORLDS
/*	******************************************************************************
	il_find_color_matching_session
	
	Desc:	Called to search the color matching session cache by source
			color space and the 'display' color space, which we find in the
			image->header.
			
	In:		icc_profiles associates with these two color spaces:	
				
		    src_space = ic->src_header->color_space			set by image decoder
		    dst_space = ic->image->header.color_space		set by IMGCBIF_NewPixmap
	
	Out:	If we find a matching session (ColorWorld in ColorSync parlance)
			we return it.
*/
static void*
il_find_color_matching_session	(	void				*src_ref,
									void				*dst_ref )
{
	void						*session = NULL;
	uint32						index = 0;
	il_cached_matching_session	*cached_session = NULL;
	/*
		Since the destination is the display, and there are a limited
		number of displays, we'll index by our known display contants.
		First do a range-check.
	*/
    if (dst_ref < kNumDisplaysToCache)
    {
	    index = dst_ref;
	}
	
	if (index)
	{
		if (index == kICCProfileRef_SystemProfile)
			index = 0;
		
		/*
			Look in the cache.
		*/
		cached_session = il_color_matching_session_cache[index];
		while (cached_session)
		{
			if (cached_session->src_profile_ref == src_ref)
				return (cached_session->matching_session);
			
			cached_session = cached_session->next;
		}
	}
	
	return (NULL);
}

/*	******************************************************************************
	il_find_color_matching_session
	
	Desc:	Called to search the color matching session cache by source
			color space and the 'display' color space, which we find in the
			image->header.
			
	In:		icc_profiles assocaites with these two color spaces:	
				
		    src_space = ic->src_header->color_space			set by image decoder
		    dst_space = ic->image->header.color_space		set by IMGCBIF_NewPixmap
	
	Out:	If we find a matching session (ColorWorld in ColorSync parlance)
			we return it.
*/
static PRBool
il_cache_color_matching_session	(	il_container	*ic )
{
	PRBool						cached = FALSE;
	int							num_in_cache = 0;
	uint32						index = 0;
    void						*src_space;
    void						*dst_space;
	il_cached_matching_session	*cached_session = NULL;
	il_cached_matching_session	*last_session = NULL;

	PR_ASSERT(ic);
	
    src_ref = ic->src_header->color_space->icc_profile_ref;
    dst_ref = ic->image->header.color_space->icc_profile_ref;
    
	/*
		Range-check the dst_ref - see if it's within our known
		constants.
	*/
    if ( src_ref && dst_ref && dst_ref < kNumDisplaysToCache)
    {
	    index = dst_ref;
	}
	
	if (index)
	{
		if (index == kICCProfileRef_SystemProfile)
			index = 0;
		
		/*
			Look in the cache.
			See if it's already there.
			If it is, then we want to return TRUE,
			otherwise it will be deleted.
		*/
		cached_session = il_color_matching_session_cache[index];
		while (cached_session)
		{
			if (cached_session->src_profile_ref == src_ref)
				return (TRUE);
			
			last_session = cached_session;
			cached_session = cached_session->next;
			num_in_cache += 1;
		}
		
		/*
			Not found, so add it.
			But first max out on a certain number.
		*/
		if (num_in_cache > kMaxCachedMatchingSessions)
			return (FALSE);

		cached_session = PR_NEWZAP(il_cached_matching_session);
		if (!cached_session)
			return (FALSE);
		
		last_session->next = cached_session;
		cached_session->src_profile_ref = src_ref;
		cached_session->matching_session = ic->icc_matching_session;
		cached_session->next = NULL;
		
		cached = TRUE;
	}
	
	return (cached);
}

#endif	/* CACHE_COLOR_WORLDS */
/*************************** Called by NetReaderImpl *************************/
#pragma mark -

/*	******************************************************************************
	IL_ProfileStreamFirstWrite

	Desc:	This function exists only because we can't rely on the normal
			cahce converter to do the right thing and put the stream on
			disk for us.  So we have to do it ourselves.
*/
int
IL_ProfileStreamFirstWrite	(	ip_container			*ip,
								const unsigned char		*str,
								int32					len )
{
	URL_Struct		*url_struct;
	int				err = 0;

	if (ip->state != ICCP_ABORT_PENDING && ip->url)
	{
	    url_struct = ip->url->GetURLStruct();
	    	
		/*
			If the file name exists, that's just fine.
			We'll open it later.
		*/
	    if (url_struct->cache_file != NULL)
	    {
		    ip->is_file_based = TRUE;
		}
		else
		{
			/*
				This is a net-borne profile that is not local.
				It will have to be stored in memory (until I
				can get someone at NetScape to tell me how to
				direct urls to the cache for later access).
		    */
		}
	}
	
    return err;
}

/*	******************************************************************************
	IL_ProfileStreamWrite

	Desc:	This is called by the stream object's write function.
			We use it to accumulate profile data in the profile container.
*/
int 
IL_ProfileStreamWrite		(	ip_container		*ip,
								const unsigned char	*str,
								int32				len )
{
	uint32			size;
	int				err = 0;
	URL_Struct		*url_struct;

	/*
	   	Someone may have decided to abort this image in mid-stream,
		but netlib doesn't know about it yet and keeps sending
		us data.  Force the netlib to abort.
    */
    if (ip->state == ICCP_ABORT_PENDING)
        return -1;

    /* Has user hit the stop button ? */
    if (il_icc_profile_stopped(ip))
    {
    	ip->state = ICCP_ABORT_PENDING;
        return -1;
	}
    
    /*
    	If we are storing this in memory, we have some work to do.
    */
	if (!ip->is_file_based)
	{
	    url_struct = ip->url->GetURLStruct();
		/*
			Get the nominal max amount to allocate.
		*/
	    size = url_struct->content_length;
	    if (size < url_struct->real_content_length)
	    	size = url_struct->real_content_length;
	    if (size < len)
	    	size = len;
		
		/*
			Add blocks to the data in the profile container.
		*/
		err = il_add_icc_profile_block(ip, size, (unsigned char*) str, len);
	}
		    
	if (err)
	{
		ILTRACE(0,("il: alloc for profile failed"));
		return err;
	}

	return len;
}


/*	******************************************************************************
	IL_ProfileStreamComplete

	Desc:	This is the stream object's stream_complete function
*/
void
IL_ProfileStreamComplete(ip_container *ip)
{
#ifdef DEBUG
	PRTime cur_time;
	PRInt32 difference;
#endif /* DEBUG */

	PR_ASSERT(ip);

#ifdef DEBUG
	cur_time = PR_Now();
	LL_SUB(cur_time, cur_time, ip->start_time);
	difference = LL_L2I(difference, cur_time);
    ILTRACE(1, ("il: complete: %d seconds for %s\n",
                difference, ip->url->GetAddress()));
#endif /* DEBUG */

	/* Record the fact that NetLib is done loading. */
	ip->is_url_loading = PR_FALSE;

	if (ip->state == ICCP_ABORT_PENDING)
	{
        /* It could be that someone aborted loading
           before the netlib finished transferring data.
           Don't do anything.
        */
    }
    else if (ip->state == ICCP_STREAM || ip->state == ICCP_ACCUMULATING)
    {
        /* This is the stream telling us we're finished loading,
           so let's assume the profile is fine.
        */
        ip->state = ICCP_COMPLETE;
	}
    
    /*
   		Notify observers that we are done decoding.
   	*/
    if (ip->state == ICCP_COMPLETE)
        il_icc_profile_complete_notify(ip);
}

/*	******************************************************************************
	IL_ProfileStreamAbort

	Desc:	This is the stream object's abort function
*/
void 
IL_ProfileStreamAbort (ip_container *ip, int status)
{
	PR_ASSERT(ip);

	ILTRACE(4,("il: abort, status=%d ip->state=%d", status, ip->state));

	if (ip->state != ICCP_ABORT_PENDING)
	{
	    if (status == MK_INTERRUPTED)
	        ip->state = ICCP_INCOMPLETE;
	    else
	        ip->state = ICCP_BAD;
    }
}

/*	******************************************************************************
	IL_NetProfileRequestDone
	
	Desc:	NET_GetURL completion callback.
			In here, we're supposed to determine what went wrong
			and signal the right cleanup mechanisms.
*/
void
IL_NetProfileRequestDone (ip_container *ip, ilIURL *url, int status)
{
	PR_ASSERT(ip);

	/* Record the fact that NetLib is done loading. */
	ip->is_url_loading = PR_FALSE;

    if (status < 0)
	{
		ILTRACE(2,("il:net done ip=0x%08x, status=%d, ip->state=0x%02x\n",
                   ip, status, ip->state));

        /*
        	Netlib detected failure before a stream was even created.
        */
		if (ip->state < ICCP_BAD)
		{
			if (status == MK_OBJECT_NOT_IN_CACHE)
				ip->state = ICCP_NOCACHE;
			else if (status == MK_UNABLE_TO_LOCATE_FILE)
                ip->state = ICCP_MISSING;
            else
            {
                /*
                	status is probably MK_INTERRUPTED
                	try again on reload
                */
				ip->state = ICCP_INCOMPLETE;
            }
        }
	}
	else
	{
        /*
        	It may be possible for NetLib to call the exit routine
        	with a success status.  Flip our state to complete if
        	we are currently in the stream state.
        */ 
	    if (ip->state < ICCP_STREAM)
	    {
		    ip->state = ICCP_MISSING;
	    }
	    else ip->state = ICCP_COMPLETE;
    }

	if (ip->state != ICCP_COMPLETE)
	{
		/*
			Since this function is always called last, we can delete
			the data here instead of all the different places
			where we might know that the state is ICCP_ABORT_PENDING.
		*/
		if (ip->state == ICCP_ABORT_PENDING || ip->state == ICCP_BAD)
		{
			il_delete_icc_profile_container(ip);
		}
    }

	/*
		I don't particularly want to free this now.
		I can free it when I'm deleting this profile container.
		UNLESS - NETLib makes it's own copy?
	*/
	
//	NS_RELEASE(urls);
//	or
//	NET_FreeURLStruct(urls);

}

/*	******************************************************************************
	IL_ProfileStreamCreated
	
	Desc:	Stream verification function called by NetReaderImpl.
*/
PRBool	IL_ProfileStreamCreated	(	ip_container	*ip,
									ilIURL			*url,
					 				int			/*	type */)
{
	PR_ASSERT(ip);

	/*
    	If there's a pending axe, sit tight.
    */
	if (ip->state == ICCP_ABORT_PENDING)
		return FALSE;

	ILTRACE(4,("il: new stream %s", url->GetAddress()));
	ip->state = ICCP_STREAM;

	return TRUE;
}

#ifdef PROFILE
#pragma profile off
#endif


