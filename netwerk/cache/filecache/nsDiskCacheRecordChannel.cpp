/*
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
 * The Original Code is Mozilla Communicator.
 * 
 * The Initial Developer of the Original Code is Intel Corp.
 * Portions created by Intel Corp. are
 * Copyright (C) 1999, 1999 Intel Corp.  All
 * Rights Reserved.
 * 
 * Contributor(s): Yixiong Zou <yixiong.zou@intel.com>
 *                 Carl Wong <carl.wong@intel.com>
 */

#include "nsDiskCacheRecordChannel.h"
//#include "nsFileTransport.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"

#include "nsIOutputStream.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

// This is copied from nsMemCacheChannel, We should consolidate these two.
class WriteStreamWrapper : public nsIOutputStream 
{
  public:
  WriteStreamWrapper(nsDiskCacheRecordChannel* aChannel,
                                         nsIOutputStream *aBaseStream) ;

  virtual ~WriteStreamWrapper() ;

  static nsresult
  Create(nsDiskCacheRecordChannel* aChannel, nsIOutputStream *aBaseStream, nsIOutputStream* *aWrapper) ;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIBASESTREAM
  NS_DECL_NSIOUTPUTSTREAM

  private:
  nsDiskCacheRecordChannel*         mChannel;
  nsCOMPtr<nsIOutputStream>         mBaseStream;
} ;

// implement nsISupports
NS_IMPL_ISUPPORTS(WriteStreamWrapper, NS_GET_IID(nsIOutputStream))

WriteStreamWrapper::WriteStreamWrapper(nsDiskCacheRecordChannel* aChannel, 
                                       nsIOutputStream *aBaseStream) 
  : mChannel(aChannel), mBaseStream(aBaseStream)
{ 
  NS_INIT_REFCNT(); 
}

WriteStreamWrapper::~WriteStreamWrapper()
{

}

nsresult 
WriteStreamWrapper::Create(nsDiskCacheRecordChannel*aChannel, nsIOutputStream *aBaseStream, nsIOutputStream* * aWrapper) 
{
  WriteStreamWrapper *wrapper = new WriteStreamWrapper(aChannel, aBaseStream);
  if (!wrapper) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(wrapper);
  *aWrapper = wrapper;
  return NS_OK;
}

NS_IMETHODIMP
WriteStreamWrapper::Write(const char *aBuffer, PRUint32 aCount, PRUint32 *aNumWritten) 
{
  *aNumWritten = 0;
  nsresult rv = mBaseStream->Write(aBuffer, aCount, aNumWritten);
  mChannel->NotifyStorageInUse(*aNumWritten);
  return rv;
}

NS_IMETHODIMP
WriteStreamWrapper::Flush() 
{ 
  return mBaseStream->Flush(); 
}

NS_IMETHODIMP
WriteStreamWrapper::Close() 
{ 
  return mBaseStream->Close(); 
}

nsDiskCacheRecordChannel::nsDiskCacheRecordChannel(nsDiskCacheRecord *aRecord, 
                                                   nsILoadGroup *aLoadGroup)
  : mRecord(aRecord) ,
    mLoadGroup(aLoadGroup) 
{
  NS_INIT_REFCNT() ;
  mRecord->mNumChannels++ ;
}

nsDiskCacheRecordChannel::~nsDiskCacheRecordChannel()
{
  mRecord->mNumChannels-- ;
}

nsresult 
nsDiskCacheRecordChannel::Init(void) 
{
  char* urlStr ;
  mRecord->mFile->GetURLString(&urlStr) ;

  nsresult rv ;
  NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = serv->NewChannel("load",    // XXX what should this be?
                        urlStr,
                        nsnull,    // no base uri
                        mLoadGroup,
                        nsnull,    // no eventsink getter
                        nsnull,    // original uri same as uri
                        getter_AddRefs(mFileTransport)) ;
  return rv ;
}

nsresult 
nsDiskCacheRecordChannel::NotifyStorageInUse(PRInt32 aBytesUsed)
{
  return mRecord->mDiskCache->m_StorageInUse += aBytesUsed ;
}

// implement nsISupports
NS_IMPL_ISUPPORTS(nsDiskCacheRecordChannel, NS_GET_IID(nsIChannel))

// implement nsIRequest
NS_IMETHODIMP
nsDiskCacheRecordChannel::IsPending(PRBool *aIsPending) 
{
  *aIsPending = PR_FALSE ;
  if(!mFileTransport)
    return NS_OK ;

  return mFileTransport->IsPending(aIsPending) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::Cancel(void)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->Cancel() ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::Suspend(void)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->Suspend() ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::Resume(void)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->Resume() ;
}

// implement nsIChannel
NS_IMETHODIMP
nsDiskCacheRecordChannel::GetURI(nsIURI * *aURI)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->GetURI(aURI) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::OpenInputStream(PRUint32 aStartPosition, 
                                          PRInt32 aReadCount,
                                          nsIInputStream* *aResult)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->OpenInputStream(aStartPosition,
                                         aReadCount,
                                         aResult) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::OpenOutputStream(PRUint32 startPosition, 
                                           nsIOutputStream* *aResult)
{
  nsresult rv ;
  NS_ENSURE_ARG(aResult) ;

  nsCOMPtr<nsIOutputStream> outputStream ;

  PRUint32 oldLength ;
  mRecord->GetStoredContentLength(&oldLength) ;

  if(startPosition < oldLength) {
    NotifyStorageInUse(startPosition - oldLength) ;

    // we should truncate the file at here. 
    rv = mRecord->SetStoredContentLength(startPosition) ;
    if(NS_FAILED(rv)) {
      printf(" failed to truncate\n") ;
      return rv ;
    }
  }

  rv = mFileTransport->OpenOutputStream(startPosition, getter_AddRefs(outputStream)) ;
  if(NS_FAILED(rv)) return rv ;

  return WriteStreamWrapper::Create(this, outputStream, aResult) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::AsyncOpen(nsIStreamObserver *observer, 
                                    nsISupports *ctxt)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->AsyncOpen(observer, ctxt) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::AsyncRead(PRUint32 aStartPosition, 
                                    PRInt32 aReadCount,
                                    nsISupports *aContext, 
                                    nsIStreamListener *aListener)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->AsyncRead(aStartPosition , 
                                   aReadCount , 
                                   aContext , 
                                   aListener) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::AsyncWrite(nsIInputStream *fromStream, 
                                     PRUint32 startPosition,
                                     PRInt32 writeCount, 
                                     nsISupports *ctxt,
                                     nsIStreamObserver *observer)

{
  /*
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->AsyncWrite(fromStream, 
                                    startPosition,
                                    writeCount,
                                    ctxt,
                                    observer) ;
  */

  // I can't do this since the write is not monitored, and I won't be
  // able to updata the storage. 
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->GetLoadAttributes(aLoadAttributes) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->SetLoadAttributes(aLoadAttributes) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetContentType(char * *aContentType)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->GetContentType(aContentType) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetContentLength(PRInt32 *aContentLength)
{
  if(!mFileTransport)
    return NS_ERROR_FAILURE ;

  return mFileTransport->GetContentLength(aContentLength) ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetOwner(nsISupports* *aOwner)
{
  *aOwner = mOwner.get() ;
  NS_IF_ADDREF(*aOwner) ;
  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::SetOwner(nsISupports* aOwner) 
{
  mOwner = aOwner ;
  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
  *aLoadGroup = mLoadGroup ;
  NS_IF_ADDREF(*aLoadGroup) ;
  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecordChannel::GetOriginalURI(nsIURI* *aURI)
{
  return NS_ERROR_NOT_IMPLEMENTED ;
}
