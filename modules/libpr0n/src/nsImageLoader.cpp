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

#include "nsImageLoader.h"

#include "nsIImageRequest2.h"

#include "nsIServiceManager.h"

#include "nsIChannel.h"
#include "nsIIOService.h"
#include "nsIRunnable.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"

#include "nsPIImageRequest.h"
#include "nsPIImageRequestProxy.h"

#include "nsXPIDLString.h"

NS_IMPL_ISUPPORTS1(nsImageLoader, nsIImageLoader)



class nsIURIKey : public nsHashKey {
protected:
  nsCOMPtr<nsIURI> mKey;

public:
  nsIURIKey(nsIURI* key) : mKey(key) {}
  ~nsIURIKey(void) {}

  PRUint32 HashCode(void) const {
    nsXPIDLCString spec;
    mKey->GetSpec(getter_Copies(spec));
    return (PRUint32) PL_HashString(spec);
  }

  PRBool Equals(const nsHashKey *aKey) const {
    PRBool eq;
    mKey->Equals( ((nsIURIKey*) aKey)->mKey, &eq );
    return eq;
  }

  nsHashKey *Clone(void) const {
    return new nsIURIKey(mKey);
  }
};



nsImageLoader::nsImageLoader()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsImageLoader::~nsImageLoader()
{
  /* destructor code */
}


//#define IMAGE_THREADPOOL 1

/* nsIImageRequest loadImage (in nsIURI uri, in nsIImageDecoderObserver aObserver, in nsISupports cx); */
NS_IMETHODIMP nsImageLoader::LoadImage(nsIURI *aURI, nsIImageDecoderObserver *aObserver, nsISupports *cx, nsIImageRequest **_retval)
{

#ifdef IMAGE_THREADPOOL
  if (!mThreadPool) {
    NS_NewThreadPool(getter_AddRefs(mThreadPool),
                     1, 4,
                     512,
                     PR_PRIORITY_NORMAL,
                     PR_GLOBAL_THREAD);
  }
#endif



  nsCOMPtr<nsPIImageRequest> imgRequest;

  nsIURIKey key(aURI);
  nsISupports *sup = mRequests.Get(&key);
  if (sup) {
    imgRequest = do_QueryInterface(sup);
    NS_RELEASE(sup);
  } else {

    nsCOMPtr<nsIIOService> ioserv(do_GetService("@mozilla.org/network/io-service;1"));
    if (!ioserv) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIChannel> newChannel;
    ioserv->NewChannelFromURI(aURI, getter_AddRefs(newChannel));
    if (!newChannel) return NS_ERROR_FAILURE;

    newChannel->SetOwner(this); // the channel is now holding a strong ref to 'this'

    imgRequest = do_CreateInstance("@mozilla.org/image/request/real;1");
    imgRequest->Init(newChannel);

    mRequests.Put(&key, imgRequest);

    nsCOMPtr<nsIStreamListener> streamList(do_QueryInterface(imgRequest));
    newChannel->AsyncRead(streamList, cx);  // XXX are we calling this too early?
  }

  nsCOMPtr<nsPIImageRequestProxy> proxyRequest(do_CreateInstance("@mozilla.org/image/request/proxy;1"));
  proxyRequest->Init(imgRequest, aObserver, cx); // init adds itself to imgRequest's list of observers


#ifdef IMAGE_THREADPOOL
  nsCOMPtr<nsIRunnable> run(do_QueryInterface(imgRequest));
  mThreadPool->DispatchRequest(run);
#endif

  nsCOMPtr<nsIImageRequest> ret(do_QueryInterface(proxyRequest));
  *_retval = ret;
  NS_ADDREF(*_retval);

  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator requests; */
NS_IMETHODIMP nsImageLoader::GetRequests(nsISimpleEnumerator * *aRequests)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

