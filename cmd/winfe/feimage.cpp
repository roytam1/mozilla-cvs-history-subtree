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

#define JMC_INIT_IMGCB_ID	1
#ifndef NSPR20
#include "coremem.h"
#include "stdafx.h"
#else
#include "stdafx.h"
#include "coremem.h"
#endif
#include "feimage.h"
#include "il_types.h"
#include "cxdc.h"
#include "cxicon.h"

JMC_PUBLIC_API(void)
_IMGCB_NewPixmap(struct IMGCB* self, jint op, void* displayContext, jint width, jint height, IL_Pixmap* image, IL_Pixmap* mask)
{
    MWContext *pContext = (MWContext *)displayContext;
	CAbstractCX  *dispCxt = (CAbstractCX *) pContext->fe.cx;

	FEBitmapInfo *imageInfo;

	imageInfo = new FEBitmapInfo;

	if (!imageInfo) return;
	NI_PixmapHeader* imageHeader = &image->header;  
	imageInfo->targetWidth = CASTINT(width);
	imageInfo->targetHeight = CASTINT(height);
	imageInfo->width = CASTINT(width);
	imageHeader->width = width;

	imageInfo->height = CASTINT(height);
	imageHeader->height = height;
	imageInfo->hBitmap = NULL;
	image->client_data = imageInfo;

	
	if ((imageInfo->bmpInfo  = dispCxt->NewPixmap(image, FALSE)) == NULL) {// error
		delete imageInfo;
		image->client_data = NULL;
		return;
	}
	if (mask) {
		FEBitmapInfo * maskInfo;

		NI_PixmapHeader* maskHeader = &mask->header;  
		maskInfo = new FEBitmapInfo;
		if (!maskInfo) {
			delete imageInfo;
			return;
		}
		maskHeader->width = imageHeader->width;
		maskHeader->height = imageHeader->height;

		mask->client_data = maskInfo;
		maskInfo->hBitmap = NULL;
		if ((maskInfo->bmpInfo = dispCxt->NewPixmap(mask, TRUE)) == NULL) {// error
#ifndef USE_DIB_SECTION
			if (image->bits) {
				CDCCX::HugeFree(image->bits);
				image->bits = NULL;
			}
#endif
			delete imageInfo;
			delete maskInfo;
			mask->client_data = NULL;
			return;
		}

		maskInfo->width = CASTINT(maskHeader->width);
		maskInfo->height = CASTINT(maskHeader->height);
		maskInfo->pContext = dispCxt;	// Not used.
		maskInfo->IsMask = TRUE;
	}

	imageInfo->pContext = dispCxt;		// Not used.
	imageInfo->IsMask = FALSE;
}

JMC_PUBLIC_API(void)
_IMGCB_UpdatePixmap(struct IMGCB* self, jint op, void* a, IL_Pixmap* b, jint c, jint d, jint e, jint f)
{
}

JMC_PUBLIC_API(void)
_IMGCB_ControlPixmapBits(struct IMGCB* self, jint op, void* displayContext,
						IL_Pixmap* image, IL_PixmapControl c)
{
	if (c == IL_RELEASE_BITS) {
		MWContext *pContext = (MWContext *)displayContext;

		XP_ASSERT(pContext);
		if (!pContext)
			return;

		ABSTRACTCX(pContext)->ImageComplete(image);
	}
}

JMC_PUBLIC_API(void)
_IMGCB_DestroyPixmap(struct IMGCB* self, jint op, void* displayContext, IL_Pixmap* pixmap)
{
	FEBitmapInfo *imageInfo;

	imageInfo = (FEBitmapInfo*)pixmap->client_data;

	if (imageInfo)
		delete imageInfo;
#ifndef USE_DIB_SECTION
	if (pixmap->bits) {
		CDCCX::HugeFree(pixmap->bits);
		pixmap->bits = NULL;
	}
#endif
	pixmap->client_data = NULL;
}

JMC_PUBLIC_API(void)
_IMGCB_DisplayPixmap(struct IMGCB* self, jint op, void* displayContext, IL_Pixmap* image, IL_Pixmap* mask, 
					 jint x, jint y, jint x_offset, jint y_offset, jint width, jint height,
                                         jint scalewidth, jint scaleheight)
{
    MWContext *pContext = (MWContext *)displayContext;
	CDCCX  *dispCxt = (CDCCX *) pContext->fe.cx;
	LTRB Rect;
	dispCxt->DisplayPixmap( image, mask, x, y, x_offset, y_offset, width, height, scalewidth, scaleheight, Rect);
}

JMC_PUBLIC_API(void)
_IMGCB_DisplayIcon(struct IMGCB* self, jint op, void* displayContext, jint x, jint y, jint iconNumber)
{
    MWContext *pContext = (MWContext *)displayContext;
	CDCCX  *dispCxt = (CDCCX *) pContext->fe.cx;

	dispCxt->DisplayIcon( x, y, CASTINT(iconNumber));
}

// The width and height are *write-only* we don't care what the original
//   values are.  But, CDCCX::GetIconDimensions() is going to give us
//   x and y as int32's not int's so jump through a few hoops to get the
//   sizes right.
//
extern JMC_PUBLIC_API(void)
_IMGCB_GetIconDimensions(struct IMGCB* self, jint op, void* displayContext, int* width, int* height, jint iconNumber)
{
    int32 lWidth = 0;
    int32 lHeight = 0;

    MWContext *pContext = (MWContext *)displayContext;
	CDCCX  *dispCxt = (CDCCX *)pContext->fe.cx;
	dispCxt->GetIconDimensions( &lWidth,  &lHeight, CASTINT(iconNumber));

    if(width)   {
        *width = CASTINT(lWidth);
    }
    if(height)  {
        *height = CASTINT(lHeight);
    }
}

JMC_PUBLIC_API(void)
_IMGCB_init(struct IMGCB* self, struct JMCException* *exceptionThrown)
{
}

JMC_PUBLIC_API(void*)
_IMGCB_getBackwardCompatibleInterface(struct IMGCB* self, const JMCInterfaceID* iid,
	struct JMCException* *exceptionThrown)
{
	return NULL;
}

#if defined (COLORSYNC)
/*	******************************************************************************
	OpenICCProfileFromMem
	
	Desc:		This is the most common way of 'opening' an icc profile,
				which really means loading it's table of contents and
				indexing its tags.  If the passed memory doesn't contain
				a valid profile, we'll retun NULL.
*/
JMC_PUBLIC_API(void*)
_IMGCB_OpenICCProfileFromMem(	struct IMGCB*	/*self*/,
								jint			/*op*/,
								void*			a,
								unsigned char*	profile_data )
{
	return ((void*) 0);
}

/*	******************************************************************************
	OpenICCProfileFromDisk
	
	Desc:		This is the most efficient (in terms of memory) way of 'opening'
	 			an icc profile, vs. having ColorSync create an in-memory copy
	 			of some of the contents.  The filename param must contain a full
	 			path name, wehich will be converted internally to a Mac FSSpec.
				If the resulting FSSpec isn't valid or the file doesn't contain
				a valid profile, we'll retun NULL.
*/
JMC_PUBLIC_API(void*)
_IMGCB_OpenICCProfileFromDisk(	struct IMGCB*	/*self*/,
								jint			/*op*/,
								void*			a,
								char*			filename )
{
	return ((void*) 0);
}

/*	******************************************************************************
	CloneICCProfileRef
	
	Desc:		If more than one 'client' wants to have a profile open, the
				profile reference can be cloned, rather than having two
				separate instances of the profile's data loaded in memory.
				ColorSync bumps an internal reference count and will only
				actually close the profile when it reaches zero.
*/
JMC_PUBLIC_API(void)
_IMGCB_CloneICCProfileRef(	struct IMGCB*	/*self*/,
							jint			/*op*/,
							void*			a,
							void*			profile_ref )
{
}

/*	******************************************************************************
	CloneICCProfileRef
	
	Desc:		Releases the internal memory allocated when a profile is opened.
				ColorSync will close the profile when there are no more clients
				referencing the opened profile.				
*/
JMC_PUBLIC_API(void)
_IMGCB_CloseICCProfileRef(	struct IMGCB*	/*self*/,
							jint			/*op*/,
							void*			a,
							void*			profile_ref )
{
}

/*	******************************************************************************
	SetupICCColorMatching
	
	Desc:		Given two profiles, a color matching session can be established
				which moves colors from one device space (specified in the
				profile) to another.  This is done by moving the colors through
				a device independent space, the instructions on how to do this
				inherent in the profile.
				
				ColorSync supports the specification of a special "system space"
				by accepting NULL in either the source or dest profile ref
				(but not both).  In this case - we require the caller to pass a
				special value "kICCProfileRef_SystemProfile" to designate this.
				
				If a color matching session can be established (may fail due to
				memory requirements, components not being installed, bad profile
				references, etc)  it is returned to the caller - who then may
				close the profile references if no more matching sessions using
				them are required.
*/
JMC_PUBLIC_API(void*)
_IMGCB_SetupICCColorMatching(	struct IMGCB*	/*self*/,
								jint			/*op*/,
								void*			a,
								void*			src_profile_ref,
								void*			dst_profile_ref )
{
	return ((void*) 0);
}

/*	******************************************************************************
	ColorMatchRGBPixels
	
	Desc:		This is the pixel matching interface.  It is NOT generalized,
				rather it expects the following:
				
			¥	pixels are in RGB 888 format (8 bits per channel - 24 bits total)
			¥	pixels are in a consecutive buffer - no padding at intervals
			
				This happens to be the way the imagelib sends pixels to the
				front end for display.  If in the future this changes we can
				generalize the interface or create new entrypoints. 			
*/
JMC_PUBLIC_API(void)
_IMGCB_ColorMatchRGBPixels(	struct IMGCB*	/*self*/,
							jint			/*op*/,
							void*			a,
							void*			color_world,
							const uint8*	pixels,
							int				column_pixels )
/*							uint32			rows ) */
{
}

/*	******************************************************************************
	DisposeICCColorMatching
	
	Desc:		Releases the memory (may be upwards of 200K) used by a color
				matching session - previously created by SetupICCColorMatching.
				
*/
JMC_PUBLIC_API(void)
_IMGCB_DisposeICCColorMatching(	struct IMGCB*	/*self*/,
								jint			/*op*/,
								void*			a,
								void* color_world )
{
}

/*	******************************************************************************
	IsColorSyncAvailable
	
	Desc:		This simply looks for the existence of a known symbol - which
				will have been resolved by CFM at load time.  If we find it
				we return TRUE.
				
*/
JMC_PUBLIC_API(jbool)
_IMGCB_IsColorSyncAvailable(	struct IMGCB*	/*self*/,
								jint			/*op*/,
								void*			a )
{
	return (false);
}
#endif /* (COLORSYNC) */

void
ImageGroupObserver(XP_Observable observable,
	      XP_ObservableMsg message,
	      void *message_data,
	      void *closure)
{
	MWContext *pXPCX = (MWContext *)closure;

	switch(message) {
	case IL_STARTED_LOADING:
		ABSTRACTCX(pXPCX)->SetImagesLoading(TRUE);
		break;

	case IL_ABORTED_LOADING:
		ABSTRACTCX(pXPCX)->SetImagesDelayed(TRUE);
		break;

	case IL_FINISHED_LOADING:
		ABSTRACTCX(pXPCX)->SetImagesLoading(FALSE);
		break;

	case IL_STARTED_LOOPING:
		ABSTRACTCX(pXPCX)->SetImagesLooping(TRUE);
		break;

	case IL_FINISHED_LOOPING:
		ABSTRACTCX(pXPCX)->SetImagesLooping(FALSE);
		break;

	default:
		break;
	}
	FE_UpdateStopState(pXPCX);
}

void
FE_MochaImageGroupObserver(XP_Observable observable,
	      XP_ObservableMsg message,
	      void *message_data,
	      void *closure)
{
	IL_GroupMessageData *data = (IL_GroupMessageData *)message_data;
	MWContext *pXPCX = (MWContext *)data->display_context;

	// If we are passed a NULL display context, the MWContext has been
	// destroyed.
	if (!pXPCX)
		return;
	
	switch(message) {
	case IL_STARTED_LOADING:
		ABSTRACTCX(pXPCX)->SetMochaImagesLoading(TRUE);
		break;

	case IL_ABORTED_LOADING:
		ABSTRACTCX(pXPCX)->SetMochaImagesDelayed(TRUE);
		break;

	case IL_FINISHED_LOADING:
		ABSTRACTCX(pXPCX)->SetMochaImagesLoading(FALSE);
		break;

	case IL_STARTED_LOOPING:
		ABSTRACTCX(pXPCX)->SetMochaImagesLooping(TRUE);
		break;

	case IL_FINISHED_LOOPING:
		ABSTRACTCX(pXPCX)->SetMochaImagesLooping(FALSE);
		break;

	default:
		break;
	}
	FE_UpdateStopState(pXPCX);
}
