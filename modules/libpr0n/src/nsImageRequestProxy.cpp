/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsImageRequestProxy.h"

#include "nsXPIDLString.h"

#include "nsIInputStream.h"
#include "nsIImageLoader.h"
#include "nsIComponentManager.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsPIImageRequest.h"

#include "nsString.h"

#include "nspr.h"

NS_IMPL_THREADSAFE_ISUPPORTS3(nsImageRequestProxy, nsIImageRequest, nsPIImageRequestProxy,
                              nsIImageDecoderObserver)

nsImageRequestProxy::nsImageRequestProxy()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsImageRequestProxy::~nsImageRequestProxy()
{
  /* destructor code */
  nsCOMPtr<nsPIImageRequest> pr(do_QueryInterface(mOwner));
  pr->RemoveObserver(this, NS_OK);
}



/* void init (in nsPIImageRequest request, in nsIImageDecoderObserver aObserver, in nsISupports cx); */
NS_IMETHODIMP nsImageRequestProxy::Init(nsPIImageRequest *request, nsIImageDecoderObserver *aObserver, nsISupports *cx)
{
  mOwner = do_QueryInterface(request);

  mObserver = aObserver;
  // XXX we should save off the thread we are getting called on here so that we can proxy all calls to mDecoder to it.

  mContext = cx;

  request->AddObserver(this);

  return NS_OK;
}


/* void cancel (in nsresult status); */
NS_IMETHODIMP nsImageRequestProxy::Cancel(nsresult status)
{
  nsCOMPtr<nsPIImageRequest> pr(do_QueryInterface(mOwner));
  return pr->RemoveObserver(this, status);
}

/* readonly attribute nsIImage image; */
NS_IMETHODIMP nsImageRequestProxy::GetImage(nsIImageContainer * *aImage)
{
  return mOwner->GetImage(aImage);
}

/* readonly attribute unsigned long imageStatus; */
NS_IMETHODIMP nsImageRequestProxy::GetImageStatus(PRUint32 *aStatus)
{
  return mOwner->GetImageStatus(aStatus);
}


/** nsIImageDecoderObserver methods **/

/* void onStartDecode (in nsIImageRequest request, in nsISupports cx); */
NS_IMETHODIMP nsImageRequestProxy::OnStartDecode(nsIImageRequest *request, nsISupports *cx)
{
  if (mObserver)
    mObserver->OnStartDecode(this, mContext);

  return NS_OK;
}

/* void onStartContainer (in nsIImageRequest request, in nsISupports cx, in nsIImageContainer image); */
NS_IMETHODIMP nsImageRequestProxy::OnStartContainer(nsIImageRequest *request, nsISupports *cx, nsIImageContainer *image)
{
  if (mObserver)
    mObserver->OnStartContainer(this, mContext, image);

  return NS_OK;
}

/* void onStartFrame (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame); */
NS_IMETHODIMP nsImageRequestProxy::OnStartFrame(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame)
{
  if (mObserver)
    mObserver->OnStartFrame(this, mContext, frame);

  return NS_OK;
}

/* [noscript] void onDataAvailable (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame, [const] in nsRect rect); */
NS_IMETHODIMP nsImageRequestProxy::OnDataAvailable(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame, const nsRect * rect)
{
  if (mObserver)
    mObserver->OnDataAvailable(this, mContext, frame, rect);

  return NS_OK;
}

/* void onStopFrame (in nsIImageRequest request, in nsISupports cx, in nsIImageFrame frame); */
NS_IMETHODIMP nsImageRequestProxy::OnStopFrame(nsIImageRequest *request, nsISupports *cx, nsIImageFrame *frame)
{
  if (mObserver)
    mObserver->OnStopFrame(this, mContext, frame);

  return NS_OK;
}

/* void onStopContainer (in nsIImageRequest request, in nsISupports cx, in nsIImageContainer image); */
NS_IMETHODIMP nsImageRequestProxy::OnStopContainer(nsIImageRequest *request, nsISupports *cx, nsIImageContainer *image)
{
  if (mObserver)
    mObserver->OnStopContainer(this, mContext, image);

  return NS_OK;
}

/* void onStopDecode (in nsIImageRequest request, in nsISupports cx, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP nsImageRequestProxy::OnStopDecode(nsIImageRequest *request, nsISupports *cx, nsresult status, const PRUnichar *statusArg)
{
  if (mObserver)
    mObserver->OnStopDecode(this, mContext, status, statusArg);

  return NS_OK;
}



