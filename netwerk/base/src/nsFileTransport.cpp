/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

#include "nsFileTransport.h"
#include "nsFileTransportService.h"
#include "nsIInterfaceRequestor.h"
#include "nsAutoLock.h"
#include "netCore.h"
#include "nsIFileStreams.h"
#include "nsCOMPtr.h"
#include "nsIProxyObjectManager.h"
#include "nsNetUtil.h"
#include "nslog.h"

static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);

#define NS_OUTPUT_STREAM_BUFFER_SIZE    (64 * 1024)

NS_IMPL_LOG(nsFileTransportLog)
#define PRINTF NS_LOG_PRINTF(nsFileTransportLog)
#define FLUSH NS_LOG_FLUSH(nsFileTransportLog)

//////////////////////////////////////////////////////////////////////////////////

#define GET_SOURCE_WRAPPER() \
    ((nsInputStreamWrapper *) (nsIInputStream *) mSourceWrapper)
#define GET_SINK_WRAPPER() \
    ((nsOutputStreamWrapper *) (nsIOutputStream *) mSinkWrapper)

//////////////////////////////////////////////////////////////////////////////////

//
// An nsInputStreamWrapper captures the number of bytes read from an
// input stream.
//
class nsInputStreamWrapper : public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS

    nsInputStreamWrapper() : mBytesRead(0) {NS_INIT_ISUPPORTS();}
    virtual ~nsInputStreamWrapper() {}

    //
    // nsIInputStream implementation...
    //
    NS_IMETHOD Close() {
        return mSource->Close();
    }
    NS_IMETHOD Available(PRUint32 *aCount) {
        return mSource->Available(aCount);
    }
    NS_IMETHOD Read(char *aBuf, PRUint32 aCount, PRUint32 *aBytesRead) {
        nsresult rv = mSource->Read(aBuf, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                            PRUint32 aCount, PRUint32 *aBytesRead) {
        nsresult rv = mSource->ReadSegments(aWriter, aClosure, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD GetNonBlocking(PRBool *aValue) {
        return mSource->GetNonBlocking(aValue);
    }
    NS_IMETHOD GetObserver(nsIInputStreamObserver **aObserver) {
        return mSource->GetObserver(aObserver);
    }
    NS_IMETHOD SetObserver(nsIInputStreamObserver *aObserver) {
        return mSource->SetObserver(aObserver);
    }

    //
    // Helper functions
    //
    void SetSource(nsIInputStream *aSource) {
        mSource = aSource;
    }
    PRUint32 GetBytesRead() {
        return mBytesRead;
    }
    void ZeroBytesRead() {
        mBytesRead = 0;
    }

protected:
    // 
    // State variables
    //
    PRUint32                 mBytesRead;
    nsCOMPtr<nsIInputStream> mSource;
};

// This must be threadsafe since different threads can run the same transport
NS_IMPL_THREADSAFE_ISUPPORTS1(nsInputStreamWrapper, nsIInputStream)

//////////////////////////////////////////////////////////////////////////////////

//
// An nsOutputStreamWrapper captures the number of bytes written to an
// output stream.
//
class nsOutputStreamWrapper : public nsIOutputStream
{
public:
    NS_DECL_ISUPPORTS

    nsOutputStreamWrapper() : mBytesWritten(0) {NS_INIT_ISUPPORTS();}
    virtual ~nsOutputStreamWrapper() {}

    //
    // nsIInputStream implementation...
    //
    NS_IMETHOD Close() {
        return mSink->Close();
    }
    NS_IMETHOD Flush() {
        return mSink->Flush();
    }
    NS_IMETHOD Write(const char *aBuf, PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->Write(aBuf, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD WriteFrom(nsIInputStream *aSource, PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->WriteFrom(aSource, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD WriteSegments(nsReadSegmentFun aReader, void *aClosure,
                            PRUint32 aCount, PRUint32 *aBytesWritten) {
        nsresult rv = mSink->WriteSegments(aReader, aClosure, aCount, aBytesWritten);
        if (NS_SUCCEEDED(rv))
            mBytesWritten += *aBytesWritten;
        return rv;
    }
    NS_IMETHOD GetNonBlocking(PRBool *aValue) {
        return mSink->GetNonBlocking(aValue);
    }
    NS_IMETHOD SetNonBlocking(PRBool aValue) {
        return mSink->SetNonBlocking(aValue);
    }
    NS_IMETHOD GetObserver(nsIOutputStreamObserver **aObserver) {
        return mSink->GetObserver(aObserver);
    }
    NS_IMETHOD SetObserver(nsIOutputStreamObserver *aObserver) {
        return mSink->SetObserver(aObserver);
    }

    //
    // Helper functions
    //
    void SetSink(nsIOutputStream *aSink) {
        mSink = aSink;
    }
    PRUint32 GetBytesWritten() {
        return mBytesWritten;
    }
    void ZeroBytesWritten() {
        mBytesWritten = 0;
    }

protected:
    // 
    // State variables
    //
    PRUint32                  mBytesWritten;
    nsCOMPtr<nsIOutputStream> mSink;
};

// This must be threadsafe since different threads can run the same transport
NS_IMPL_THREADSAFE_ISUPPORTS1(nsOutputStreamWrapper, nsIOutputStream)

//////////////////////////////////////////////////////////////////////////////////

nsFileTransport::nsFileTransport()
    : mContentType(nsnull),
      mBufferSegmentSize(NS_FILE_TRANSPORT_DEFAULT_SEGMENT_SIZE),
      mBufferMaxSize(NS_FILE_TRANSPORT_DEFAULT_BUFFER_SIZE),
      mXferState(CLOSED),
      mRunState(RUNNING),
      mCancelStatus(NS_OK),
      mMonitor(nsnull),
      mActive(PR_FALSE),
      mStatus(NS_OK),
      mOffset(0),
      mTotalAmount(-1),
      mTransferAmount(-1),
      mLoadAttributes(LOAD_NORMAL),
      mService(nsnull)
{
    NS_INIT_ISUPPORTS();
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, nsIFile* file, PRInt32 ioFlags, PRInt32 perm)
{
    nsresult rv;
    nsCOMPtr<nsIFileIO> io;
    rv = NS_NewFileIO(getter_AddRefs(io), file, ioFlags, perm);
    if (NS_FAILED(rv)) return rv;

    return Init(aService, io);
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, const char* name, nsIInputStream* inStr,
                      const char* contentType, PRInt32 contentLength)
{
    nsresult rv;
    nsCOMPtr<nsIInputStreamIO> io;
    rv = NS_NewInputStreamIO(getter_AddRefs(io),
                             name, inStr, contentType, contentLength);
    if (NS_FAILED(rv)) return rv;
    return Init(aService, io);
}

nsresult
nsFileTransport::Init(nsFileTransportService *aService, nsIStreamIO* io)
{
    nsresult rv = NS_OK;
    if (mMonitor == nsnull) {
        mMonitor = nsAutoMonitor::NewMonitor("nsFileTransport");
        if (mMonitor == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    mStreamIO = io;
    nsXPIDLCString name;
    rv = mStreamIO->GetName(getter_Copies(name));
    mStreamName = NS_STATIC_CAST(const char*, name);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetName failed");

    mService = aService;
    PR_AtomicIncrement(&mService->mTotalTransports);

    return rv;
}

nsFileTransport::~nsFileTransport()
{
    if (mXferState != CLOSED) {
        DoClose();
    }
    NS_ASSERTION(mSource == nsnull, "transport not closed");
    NS_ASSERTION(mSourceWrapper == nsnull, "transport not closed");
    NS_ASSERTION(mSink == nsnull, "transport not closed");
    NS_ASSERTION(mSinkWrapper == nsnull, "transport not closed");
    //NS_ASSERTION(mPipeIn == nsnull, "transport not closed");
    //NS_ASSERTION(mPipeOut == nsnull, "transport not closed");
    if (mMonitor)
        nsAutoMonitor::DestroyMonitor(mMonitor);
    if (mContentType)
        nsCRT::free(mContentType);

    PR_AtomicDecrement(&mService->mTotalTransports);

}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsFileTransport, 
                              nsIChannel, 
                              nsIRequest, 
                              nsIRunnable)

NS_METHOD
nsFileTransport::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsFileTransport* fc = new nsFileTransport();
    if (fc == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(fc);
    nsresult rv = fc->QueryInterface(aIID, aResult);
    NS_RELEASE(fc);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::GetName(PRUnichar* *result)
{
    nsString name;
    name.AppendWithConversion(mStreamName);
    *result = name.ToNewUnicode();
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsFileTransport::IsPending(PRBool *result)
{
    *result = mXferState != CLOSED;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetStatus(nsresult *status)
{
    *status = mRunState == CANCELED ? mCancelStatus : mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::Cancel(nsresult status)
{
    nsAutoMonitor mon(mMonitor);
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");

    // Cancel takes precidence over Suspend

    nsresult rv = NS_OK;
    if (mRunState == SUSPENDED) {
        rv = Resume();
    }
    if (NS_SUCCEEDED(rv)) {
        // if there's no other error pending, say that we aborted
        mRunState = CANCELED;
        mCancelStatus = status;
    }
    PRINTF("Cancel [this=%x %s]\n", this, mStreamName.GetBuffer());
    return rv;
}

NS_IMETHODIMP
nsFileTransport::Suspend()
{
    nsAutoMonitor mon(mMonitor);
    nsresult rv = NS_OK;
    if (mRunState != SUSPENDED) {
        PRINTF("Suspend [this=%x %s]\n",
                this, mStreamName.GetBuffer());
        mRunState = SUSPENDED;
    }
    return rv;
}

NS_IMETHODIMP
nsFileTransport::Resume()
{
    nsAutoMonitor mon(mMonitor);
    nsresult rv = NS_OK;
    if (mRunState == SUSPENDED) {
        PRINTF("Resume [this=%x %s] status=%x\n",
                this, mStreamName.GetBuffer(), mStatus);
        mRunState = RUNNING;  // set this first before resuming!

        // Only dispatch a new thread, if there isn't one currently active.
        if (!mActive)
            mStatus = mService->DispatchRequest(this);
    }
    else
        PRINTF("Resume ignored [this=%x %s] status=%x\n",
                this, mStreamName.GetBuffer(), mStatus);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// From nsITransport
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::OpenInputStream(nsIInputStream **result)
{
    nsresult rv;
    nsCOMPtr<nsIInputStream> in;
    rv = mStreamIO->GetInputStream(getter_AddRefs(in));
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION(mTransferAmount == -1, "need to wrap input stream in one that truncates");
    if (mOffset > 0) {
        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(in, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, mOffset);
        if (NS_FAILED(rv)) return rv;
    }
    *result = in;
    NS_ADDREF(*result);
    return rv;
}

NS_IMETHODIMP
nsFileTransport::OpenOutputStream(nsIOutputStream **result)
{
    return mStreamIO->GetOutputStream(result);
}

NS_IMETHODIMP
nsFileTransport::AsyncRead(nsIStreamListener *listener, nsISupports *ctxt)
{
    nsresult rv = NS_OK;

    if (mXferState != CLOSED)
        return NS_ERROR_IN_PROGRESS;

    NS_ASSERTION(listener, "need to supply an nsIStreamListener");
    rv = NS_NewStreamListenerProxy(getter_AddRefs(mListener),
                                   listener, nsnull,
                                   mBufferSegmentSize,
                                   mBufferMaxSize);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mContext == nsnull, "context not released");
    mContext = ctxt;
    mXferState = OPEN_FOR_READ;

    PRINTF("AsyncRead [this=%x %s] mOffset=%d mTransferAmount=%d\n",
            this, mStreamName.GetBuffer(), mOffset, mTransferAmount);

    rv = mService->DispatchRequest(this);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::AsyncWrite(nsIStreamProvider *provider,
                            nsISupports *ctxt)
{
    nsresult rv = NS_OK;

    PRINTF("AsyncWrite [this=%x, provider=%x]\n", this, provider);

    if (mXferState != CLOSED)
        return NS_ERROR_IN_PROGRESS;

    NS_ASSERTION(provider, "need to supply an nsIStreamProvider");
    rv = NS_NewStreamProviderProxy(getter_AddRefs(mProvider), provider);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mContext == nsnull, "context not released");
    mContext = ctxt;
    mXferState = OPEN_FOR_WRITE;

    PRINTF("AsyncWrite [this=%x %s] mOffset=%d mTransferAmount=%d\n",
            this, mStreamName.GetBuffer(), mOffset, mTransferAmount);

    rv = mService->DispatchRequest(this);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRunnable methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::Run(void)
{
    nsAutoMonitor mon(mMonitor);
    mActive = PR_TRUE;

    while (mXferState != CLOSED && mRunState != SUSPENDED) {
        if (mRunState == CANCELED) {
            if (mXferState == READING)
                mXferState = END_READ;
            else if (mXferState == WRITING)
                mXferState = END_WRITE;
            else
                mXferState = CLOSING;
            mStatus = mCancelStatus;
        }
        Process();
    }
    mActive = PR_FALSE;
    return NS_OK;
}

/*
static NS_METHOD
nsWriteToFile(nsIInputStream* in,
              void* closure,
              const char* fromRawSegment,
              PRUint32 toOffset,
              PRUint32 count,
              PRUint32 *writeCount)
{
    nsIOutputStream* outStr = (nsIOutputStream*)closure;
    nsresult rv = outStr->Write(fromRawSegment, count, writeCount);
    return rv;
}
*/

void
nsFileTransport::Process(void)
{
    PRINTF("Process [this=%x, state=%x]\n", this, mXferState);FLUSH();

    switch (mXferState) {
      case OPEN_FOR_READ: { 
        PRINTF("OPEN_FOR_READ [this=%x %s]\n",
                this, mStreamName.GetBuffer());FLUSH();
        mStatus = mStreamIO->Open(&mContentType, &mTotalAmount);
        if (mListener) {
            nsresult rv = mListener->OnStartRequest(this, mContext);  // always send the start notification
            if (NS_SUCCEEDED(mStatus))
                mStatus = rv;
        }

        mXferState = NS_FAILED(mStatus) ? END_READ : START_READ;
        break;
      }

      case START_READ: {
        PRINTF("START_READ [this=%x %s]\n",
                this, mStreamName.GetBuffer());FLUSH();

        PR_AtomicIncrement(&mService->mInUseTransports);

        mStatus = mStreamIO->GetInputStream(getter_AddRefs(mSource));
        if (NS_FAILED(mStatus)) {
            mXferState = END_READ;
            return;
        }

        if (mOffset > 0) {
            // if we need to set a starting offset, QI for the nsISeekableStream
            // and set it
            nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(mSource, &mStatus);
            if (NS_FAILED(mStatus)) {
                mXferState = END_READ;
                return;
            }
            // for now, assume the offset is always relative to the start of the
            // file (position 0) so use PR_SEEK_SET
            mStatus = ras->Seek(PR_SEEK_SET, mOffset);
            if (NS_FAILED(mStatus)) {
                mXferState = END_READ;
                return;
            }
        }

        if (!mSourceWrapper) {
            //
            // Allocate an input stream wrapper to capture the number of bytes
            // read from mSource.
            //
            NS_NEWXPCOM(mSourceWrapper, nsInputStreamWrapper);
            if (!mSourceWrapper) {
                mStatus = NS_ERROR_OUT_OF_MEMORY;
                mXferState = END_READ;
                return;
            }
            GET_SOURCE_WRAPPER()->SetSource(mSource);
        }

        // capture the total amount for progress information
        if (mTransferAmount < 0) {
            mTransferAmount = mTotalAmount;
        } else
            mTotalAmount = mTransferAmount;

        mXferState = READING;
        break;
      }

      case READING: {
        //
        // Read at most mBufferMaxSize.
        //
        PRInt32 transferAmt = mBufferMaxSize;
        if (mTransferAmount >= 0)
            transferAmt = PR_MIN(transferAmt, mTransferAmount);

        PRINTF("READING [this=%x %s] transferAmt=%u mBufferMaxSize=%u\n",
                this, mStreamName.GetBuffer(), transferAmt, mBufferMaxSize);

        // Zero the number of bytes read on the source wrapper
        GET_SOURCE_WRAPPER()->ZeroBytesRead();

        //
        // Give the listener a chance to read at most transferAmt bytes from
        // the source input stream.
        //
        nsresult status = mListener->OnDataAvailable(this, mContext,
                                                     mSourceWrapper,
                                                     mOffset, transferAmt);

        //
        // Handle the various return codes.
        //
        if (status == NS_BASE_STREAM_WOULD_BLOCK) {
            PRINTF("READING [this=%x %s] listener would block; suspending self.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = NS_OK;
            Suspend();
        }
        else if (status == NS_BASE_STREAM_CLOSED) {
            PRINTF("READING [this=%x %s] done reading file.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = NS_OK;
            mXferState = END_READ;
        }
        else if (NS_FAILED(status)) {
            PRINTF("READING [this=%x %s] error reading file.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = status;
            mXferState = END_READ;
        }
        else {
            // 
            // get the number of bytes read
            //
            PRUint32 total = GET_SOURCE_WRAPPER()->GetBytesRead();
            mOffset += total;

            if (mTransferAmount > 0)
                mTransferAmount -= total;

            if (0 == total || 0 == mTransferAmount) {
                PRINTF("READING [this=%x %s] done reading file.\n",
                        this, mStreamName.GetBuffer());FLUSH();
                mXferState = END_READ;
            }
            else
                PRINTF("READING [this=%x %s] read %u bytes [offset=%u]\n",
                        this, mStreamName.GetBuffer(), total, mOffset);FLUSH();

            if (mProgress && !(mLoadAttributes & LOAD_BACKGROUND)
                    && (mTransferAmount >= 0)) {
                mProgress->OnProgress(this, mContext,
                                      mTotalAmount - mTransferAmount,
                                      mTotalAmount);
            }
        }
        break;
      }

      case END_READ: {
        
        PR_AtomicDecrement(&mService->mInUseTransports);

        PRINTF("END_READ [this=%x %s] status=%x\n",
                this, mStreamName.GetBuffer(), mStatus);

#if defined (DEBUG_dougt) || defined (DEBUG_warren)
        NS_ASSERTION(mTransferAmount <= 0 || NS_FAILED(mStatus), "didn't transfer all the data");
#endif 
        if (mTransferAmount > 0 && NS_SUCCEEDED(mStatus)) {
            //
            // This happens when the requested read amount is more than the amount
            // of the data in the stream/file, or if the listener returned 
            // NS_BASE_STREAM_CLOSED.
            //
            mStatus = NS_BASE_STREAM_CLOSED;
        }

        // close the data source
        mSource = 0;
        mSourceWrapper = 0;

        if (mListener) {
            mListener->OnStopRequest(this, mContext, mStatus, nsnull);
            mListener = 0;
        }
        if (mProgress && !(mLoadAttributes & LOAD_BACKGROUND)) {
            nsAutoString fileName;
            fileName.AssignWithConversion(mStreamName);
            mProgress->OnStatus(this, mContext, 
                                NS_NET_STATUS_READ_FROM, 
                                fileName.GetUnicode());
        }
        mContext = 0;

        mXferState = CLOSING;
        break;
      }

      case OPEN_FOR_WRITE: {
        PRINTF("OPEN_FOR_WRITE [this=%x %s]\n",
                this, mStreamName.GetBuffer());
        mStatus = mStreamIO->Open(&mContentType, &mTotalAmount);
        if (mProvider) {
            // always send the start notification
            nsresult rv = mProvider->OnStartRequest(this, mContext);
            if (NS_SUCCEEDED(mStatus))
                mStatus = rv;
        }

        mXferState = NS_FAILED(mStatus) ? END_WRITE : START_WRITE;
        break;
      }

      case START_WRITE: {
        PRINTF("START_WRITE [this=%x %s]\n",
                this, mStreamName.GetBuffer());

        PR_AtomicIncrement(&mService->mInUseTransports);

        mStatus = mStreamIO->GetOutputStream(getter_AddRefs(mSink));
        if (NS_FAILED(mStatus)) {
            mXferState = END_WRITE;
            return;
        }

        if (mOffset > 0) {
            // If we need to set a starting offset, QI for the nsISeekableStream
            // and set it.
            nsCOMPtr<nsISeekableStream> ras = do_QueryInterface(mSink, &mStatus);
            if (NS_FAILED(mStatus)) {
                mXferState = END_WRITE;
                return;
            }
            // For now, assume the offset is always relative to the start of the
            // file (position 0) so use PR_SEEK_SET.
            mStatus = ras->Seek(PR_SEEK_SET, mOffset);
            if (NS_FAILED(mStatus)) {
                mXferState = END_WRITE;
                return;
            }
            mOffset = 0;
        }

        if (!mSinkWrapper) {
            //
            // Allocate an output stream wrapper to capture the number of bytes
            // written to mSink.
            //
            NS_NEWXPCOM(mSinkWrapper, nsOutputStreamWrapper);
            if (!mSinkWrapper) {
                mStatus = NS_ERROR_OUT_OF_MEMORY;
                mXferState = END_WRITE;
                return;
            }
            GET_SINK_WRAPPER()->SetSink(mSink);
        }

        mXferState = WRITING;
        break;
      }

      case WRITING: {
        //
        // Write at most mBufferMaxSize
        //
        PRUint32 transferAmt = mBufferMaxSize;
        if (mTransferAmount >= 0)
            transferAmt = PR_MIN(transferAmt, (PRUint32)mTransferAmount);

        // Zero the number of bytes written to the sink wrapper.
        GET_SINK_WRAPPER()->ZeroBytesWritten();

        // 
        // Ask the provider for data
        //
        nsresult status = mProvider->OnProvideData(this, mContext,
                                                   mSinkWrapper,
                                                   mOffset, transferAmt);
        
        //
        // Handle the various return codes.
        //
        if (status == NS_BASE_STREAM_WOULD_BLOCK) {
            PRINTF("WRITING [this=%x %s] provider would block; suspending self.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = NS_OK;
            Suspend();
        }
        else if (status == NS_BASE_STREAM_CLOSED) {
            PRINTF("WRITING [this=%x %s] no more data to be written.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = NS_OK;
            mXferState = END_WRITE;
        }
        else if (NS_FAILED(status)) {
            PRINTF("WRITING [this=%x %s] provider failed.\n",
                    this, mStreamName.GetBuffer());FLUSH();
            mStatus = status;
            mXferState = END_WRITE;
        }
        else {
            //
            // Get the number of bytes written
            //
            PRUint32 total = GET_SINK_WRAPPER()->GetBytesWritten();
            mOffset += total;

            if (mTransferAmount > 0)
                mTransferAmount -= total;

            if (total == 0 || mTransferAmount == 0) {
                PRINTF("WRITING [this=%x %s] done writing file.\n",
                        this, mStreamName.GetBuffer());FLUSH();
                mXferState = END_WRITE;
            }
            else 
                PRINTF("WRITING [this=%x %s] wrote %u bytes [offset=%u]\n",
                        this, mStreamName.GetBuffer(), total, mOffset);FLUSH();

            if (mProgress && !(mLoadAttributes & LOAD_BACKGROUND)
                    && (mTransferAmount >= 0))
                mProgress->OnProgress(this, mContext,
                                      mTotalAmount - mTransferAmount,
                                      mTotalAmount);
        }
        break;
      }

      case END_WRITE: {
        PRINTF("END_WRITE [this=%x %s] status=%x\n",
                this, mStreamName.GetBuffer(), mStatus);

        PR_AtomicDecrement(&mService->mInUseTransports);

#if defined (DEBUG_dougt) || defined (DEBUG_warren)
        NS_ASSERTION(mTransferAmount <= 0 || NS_FAILED(mStatus), "didn't transfer all the data");
#endif 
        if (mTransferAmount > 0 && NS_SUCCEEDED(mStatus)) {
            // This happens when the requested write amount is more than the amount
            // of the data in the stream/file.
            mStatus = NS_BASE_STREAM_CLOSED;
        }

        if (mSink) {
            mSink->Flush();
            mSink = 0;
        }
        mSinkWrapper = 0;
        // Close the pipe...
        //if (mPipeIn)
        //    mPipeIn = 0;
        //if (mPipeOut)
        //    mPipeOut = 0;

        nsresult rv;
        if (mProvider) {
            mProvider->OnStopRequest(this, mContext, mStatus, nsnull);
            mProvider = 0;
        }
        if (mProgress && !(mLoadAttributes & LOAD_BACKGROUND)) {
            nsAutoString fileName; fileName.AssignWithConversion(mStreamName);
            rv = mProgress->OnStatus(this, mContext,
                                     NS_NET_STATUS_WROTE_TO, 
                                     fileName.GetUnicode());
            NS_ASSERTION(NS_SUCCEEDED(rv), "unexpected OnStatus failure");
        }
        mContext = 0;

        mXferState = CLOSING;
        break;
      }

      case CLOSING: {
        DoClose();
        break;
      }

      case CLOSED: {
        NS_NOTREACHED("trying to continue a quiescent file transfer");
        break;
      }
    }
}

void
nsFileTransport::DoClose(void)
{
    PRINTF("CLOSING [this=%x %s] status=%x\n",
            this, mStreamName.GetBuffer(), mStatus);

    if (mStreamIO) {
        nsresult rv = mStreamIO->Close(mStatus);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unexpected Close failure");
        mStreamIO = 0;
    }
    mXferState = CLOSED;

    PR_AtomicDecrement(&mService->mConnectedTransports);
}

////////////////////////////////////////////////////////////////////////////////
// other nsIChannel methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsFileTransport::GetOriginalURI(nsIURI* *aURI)
{
    NS_NOTREACHED("GetOriginalURI");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::SetOriginalURI(nsIURI* aURI)
{
    NS_NOTREACHED("SetOriginalURI");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetURI(nsIURI* *aURI)
{
//    NS_NOTREACHED("GetURI");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::SetURI(nsIURI* aURI)
{
    NS_NOTREACHED("SetURI");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    *aLoadAttributes = mLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    mLoadAttributes = aLoadAttributes;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetContentType(char * *aContentType)
{
    *aContentType = nsCRT::strdup(mContentType);
    if (*aContentType == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetContentType(const char *aContentType)
{
    if (mContentType) {
      nsCRT::free(mContentType);
    }
    mContentType = nsCRT::strdup(aContentType);
    if (!mContentType) return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetContentLength(PRInt32 *aContentLength)
{
    *aContentLength = mTotalAmount;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetContentLength(PRInt32 aContentLength)
{
    NS_NOTREACHED("SetContentLength");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetTransferOffset(PRUint32 *aTransferOffset)
{
    *aTransferOffset = mOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetTransferOffset(PRUint32 aTransferOffset)
{
    mOffset = aTransferOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetTransferCount(PRInt32 *aTransferCount)
{
    *aTransferCount = mTransferAmount;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetTransferCount(PRInt32 aTransferCount)
{
    mTransferAmount = aTransferCount;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetBufferSegmentSize(PRUint32 *aBufferSegmentSize)
{
    *aBufferSegmentSize = mBufferSegmentSize;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetBufferSegmentSize(PRUint32 aBufferSegmentSize)
{
    mBufferSegmentSize = aBufferSegmentSize;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetBufferMaxSize(PRUint32 *aBufferMaxSize)
{
    *aBufferMaxSize = mBufferMaxSize;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetBufferMaxSize(PRUint32 aBufferMaxSize)
{
    mBufferMaxSize = aBufferMaxSize;
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetLocalFile(nsIFile* *file)
{
    nsresult rv;
    nsCOMPtr<nsIFileIO> fileIO = do_QueryInterface(mStreamIO, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = fileIO->GetFile(file);
    if (NS_FAILED(rv)) {
        *file = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::GetPipeliningAllowed(PRBool *aPipeliningAllowed)
{
    *aPipeliningAllowed = PR_FALSE;
    return NS_OK;
}
 
NS_IMETHODIMP
nsFileTransport::SetPipeliningAllowed(PRBool aPipeliningAllowed)
{
    NS_NOTREACHED("SetPipeliningAllowed");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetOwner(nsISupports * *aOwner)
{
    NS_NOTREACHED("GetOwner");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::SetOwner(nsISupports * aOwner)
{
    NS_NOTREACHED("SetOwner");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
    NS_NOTREACHED("GetLoadGroup");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    NS_NOTREACHED("SetLoadGroup");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileTransport::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
    *aNotificationCallbacks = mCallbacks.get();
    NS_IF_ADDREF(*aNotificationCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsFileTransport::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
    mCallbacks = aNotificationCallbacks;

    // Get a nsIProgressEventSink so that we can fire status/progress on it-
    if (mCallbacks) {
        nsCOMPtr<nsISupports> sink;
        nsresult rv = mCallbacks->GetInterface(NS_GET_IID(nsIProgressEventSink),
                                               getter_AddRefs(sink));
        if (NS_FAILED(rv)) return NS_OK;        // don't need a progress event sink

        // Now generate a proxied event sink
        NS_WITH_SERVICE(nsIProxyObjectManager,
                        proxyMgr, kProxyObjectManagerCID, &rv);
        if (NS_FAILED(rv)) return rv;
        
        rv = proxyMgr->GetProxyForObject(NS_UI_THREAD_EVENTQ, // primordial thread - should change?
                                      NS_GET_IID(nsIProgressEventSink),
                                      sink,
                                      PROXY_ASYNC | PROXY_ALWAYS,
                                      getter_AddRefs(mProgress));
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsFileTransport::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    *aSecurityInfo = nsnull;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
