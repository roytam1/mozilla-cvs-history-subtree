/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Travis Bogard <travis@netscape.com> 
 */

#include "nscore.h"
#include "nsScreen.h"
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIOutputDevice.h"
#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentLoader.h"



// XXX pav -- this file is mostly commented out.. please fix it:)





//
//  Screen class implementation 
//
ScreenImpl::ScreenImpl( nsIDocShell* aDocShell): mDocShell(aDocShell)
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
}

ScreenImpl::~ScreenImpl()
{
}

NS_IMPL_ADDREF(ScreenImpl)
NS_IMPL_RELEASE(ScreenImpl)

NS_INTERFACE_MAP_BEGIN(ScreenImpl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptObjectOwner)
   NS_INTERFACE_MAP_ENTRY(nsIScriptObjectOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDOMScreen)
NS_INTERFACE_MAP_END

NS_IMETHODIMP ScreenImpl::SetDocShell(nsIDocShell* aDocShell)
{
   mDocShell = aDocShell; // Weak Reference
   return NS_OK;
}

NS_IMETHODIMP
ScreenImpl::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

NS_IMETHODIMP
ScreenImpl::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
#if 0
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  if (nsnull == mScriptObject) {
    nsIScriptGlobalObject *global = aContext->GetGlobalObject();
    res = NS_NewScriptScreen(aContext, NS_STATIC_CAST(nsIDOMScreen *, this),
                             global, &mScriptObject);
    NS_IF_RELEASE(global);
  }
  
  *aScriptObject = mScriptObject;
#endif
  return res;
}


NS_IMETHODIMP
ScreenImpl::GetTop(PRInt32* aTop)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
		nsRect rect;
		context->GetRect( &rect );
		*aTop = rect.y;
		return NS_OK;
	}
  *aTop = -1;
#endif
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
ScreenImpl::GetLeft(PRInt32* aLeft)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
		nsRect rect;
		context->GetRect( &rect );
		float devUnits;
		*aLeft = rect.x;
		return NS_OK;
	}
  *aLeft = -1;
#endif
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
ScreenImpl::GetWidth(PRInt32* aWidth)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
		nsSize size;
		context->GetSize( &size );
		*aWidth = size.width;
		return NS_OK;
	}

  *aWidth = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetHeight(PRInt32* aHeight)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
    nsSize size;
    context->GetSize( &size );
		*aHeight = size.height;
		return NS_OK;
	}
	
  *aHeight = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetPixelDepth(PRInt32* aPixelDepth)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
		gfx_depth depth;
		context->GetDepth( &depth  );
		*aPixelDepth = depth;
		return NS_OK;
	}
  //XXX not implmented
  *aPixelDepth = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetColorDepth(PRInt32* aColorDepth)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
	if ( context )
	{
		gfx_depth depth;
		context->GetDepth( &depth );
		*aColorDepth = depth;
		return NS_OK;
	}
  
  *aColorDepth = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetAvailWidth(PRInt32* aAvailWidth)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailWidth = NSToIntRound(float( rect.width) / devUnits );
	  return NS_OK;
  }

  *aAvailWidth = -1;
#endif
  return NS_ERROR_FAILURE;

}

NS_IMETHODIMP
ScreenImpl::GetAvailHeight(PRInt32* aAvailHeight)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailHeight = NSToIntRound(float( rect.height) / devUnits );
	  return NS_OK;
  }

  *aAvailHeight = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetAvailLeft(PRInt32* aAvailLeft)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailLeft = NSToIntRound(float( rect.x) / devUnits );
	  return NS_OK;
 }

  *aAvailLeft = -1;
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetAvailTop(PRInt32* aAvailTop)
{
#if 0
	nsCOMPtr<nsIOutputDevice> context ( getter_AddRefs(GetOutputDevice()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailTop = NSToIntRound(float( rect.y) / devUnits );
	  return NS_OK;
  }

  *aAvailTop = -1;
#endif
  return NS_ERROR_FAILURE;
}

nsIOutputDevice* ScreenImpl::GetOutputDevice()
{
#if 0
  if(!mDocShell)
    return nsnull;

	nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));

  nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
  if(!docViewer)
    return nsnull;

  nsCOMPtr<nsIPresContext> presContext;
  docViewer->GetPresContext(*getter_AddRefs(presContext));

	nsIOutputDevice* context = nsnull;
  if(presContext)
    presContext->GetOutputDevice(&context);
  
  return context;
#else
  return nsnull;
#endif
}


