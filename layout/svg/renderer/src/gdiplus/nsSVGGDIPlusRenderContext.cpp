/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include <windows.h>
#include <Gdiplus.h>
using namespace Gdiplus;

#include "nsCOMPtr.h"
#include "nsSVGGDIPlusRenderContext.h"
#include "nsISVGGDIPlusRenderContext.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIPresContext.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGDIPlusRenderContext class

class nsSVGGDIPlusRenderContext : public nsISVGGDIPlusRenderContext
{
public:
  nsSVGGDIPlusRenderContext();
  ~nsSVGGDIPlusRenderContext();
  nsresult Init(nsIRenderingContext* ctx, nsIPresContext* presContext);

  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererRenderContext interface:
  NS_DECL_NSISVGRENDERERRENDERCONTEXT

  // nsISVGGDIPlusRenderContext interface:
  NS_IMETHOD_(Graphics*) GetGraphics();
  

private:
  nsCOMPtr<nsIRenderingContext> mMozContext;
  nsCOMPtr<nsIPresContext> mPresContext;
  Graphics *mGraphics;
};

//----------------------------------------------------------------------
// implementation:

nsSVGGDIPlusRenderContext::nsSVGGDIPlusRenderContext() : mGraphics(nsnull)
{
  NS_INIT_ISUPPORTS();
}

nsSVGGDIPlusRenderContext::~nsSVGGDIPlusRenderContext()
{
  if (mGraphics)
    delete mGraphics;
  mMozContext = nsnull;
}

nsresult
nsSVGGDIPlusRenderContext::Init(nsIRenderingContext* ctx,
                                nsIPresContext* presContext)
{
  mPresContext = presContext;
  mMozContext = ctx;
  NS_ASSERTION(mMozContext, "empty rendering context");

  HDC hdc;
  // this ctx better be what we think it is...
  mMozContext->RetrieveCurrentNativeGraphicData((PRUint32 *)(&hdc));
    
  mGraphics = new Graphics(hdc);
  if (!mGraphics) return NS_ERROR_FAILURE;

  nsTransform2D* xform;
  mMozContext->GetCurrentTransform(xform);
  float dx, dy;
  xform->GetTranslation(&dx, &dy);
  mGraphics->TranslateTransform(dx, dy);
  
  return NS_OK;
}

nsresult
NS_NewSVGGDIPlusRenderContext(nsISVGRendererRenderContext **result,
                              nsIRenderingContext *ctx,
                              nsIPresContext *presContext)
{
  nsSVGGDIPlusRenderContext* pg = new nsSVGGDIPlusRenderContext();
  if (!pg) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(pg);

  nsresult rv = pg->Init(ctx, presContext);

  if (NS_FAILED(rv)) {
    NS_RELEASE(pg);
    return rv;
  }
  
  *result = pg;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGGDIPlusRenderContext)
NS_IMPL_RELEASE(nsSVGGDIPlusRenderContext)

NS_INTERFACE_MAP_BEGIN(nsSVGGDIPlusRenderContext)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererRenderContext)
  NS_INTERFACE_MAP_ENTRY(nsISVGGDIPlusRenderContext)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
//  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGRendererRenderContext)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGRendererRenderContext methods:

/* nsIRenderingContext lockMozRenderingContext (); */
NS_IMETHODIMP
nsSVGGDIPlusRenderContext::LockMozRenderingContext(nsIRenderingContext **_retval)
{
  Flush();
  *_retval = mMozContext;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* void unlockMozRenderingContext (); */
NS_IMETHODIMP
nsSVGGDIPlusRenderContext::UnlockMozRenderingContext()
{
  // nothing to do
  return NS_OK;
}

/* nsIPresContext getPresContext (); */
NS_IMETHODIMP
nsSVGGDIPlusRenderContext::GetPresContext(nsIPresContext **_retval)
{
  *_retval = mPresContext;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

/* void clear (in nscolor color); */
NS_IMETHODIMP
nsSVGGDIPlusRenderContext::Clear(nscolor color)
{
  mGraphics->Clear(Color(NS_GET_R(color),
                         NS_GET_G(color),
                         NS_GET_B(color)));
  return NS_OK;
}

/* void flush (); */
NS_IMETHODIMP
nsSVGGDIPlusRenderContext::Flush()
{
  mGraphics->Flush(FlushIntentionSync);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGGDIPlusRenderContext methods:

NS_IMETHODIMP_(Graphics*)
nsSVGGDIPlusRenderContext::GetGraphics()
{
  return mGraphics;
}
