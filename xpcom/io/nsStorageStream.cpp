/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/**
 * The storage stream provides a single output stream that can be used
 * by a client to fill an internal buffer with data.  One or more
 * input streams can be created to read the data out non-destructively.
 */
#include "nsStorageStream.h"
#include "nsSegmentedBuffer.h"
#include "nsCOMPtr.h"
#include "prbit.h"

nsStorageStream::nsStorageStream()
    : mSegmentedBuffer(0), mSegmentSize(0), mWriteInProgress(false),
      mWriteCursor(0), mSegmentEnd(0), mLogicalLength(0), mLastSegmentNum(-1)
{
    NS_INIT_REFCNT();
}

nsStorageStream::~nsStorageStream()
{
	if (mSegmentedBuffer)
	    delete mSegmentedBuffer;
}

NS_IMPL_ADDREF(nsStorageStream)
NS_IMPL_RELEASE(nsStorageStream)

// Multiple inheritance requires customized QI
NS_IMETHODIMP
nsStorageStream::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (aInstancePtr == nsnull)
        return NS_ERROR_NULL_POINTER;
    if (aIID.Equals(NS_GET_IID(nsIStorageStream)) ||
        aIID.Equals(NS_GET_IID(nsISupports))) {
        NS_ADDREF(this);
        *aInstancePtr = NS_STATIC_CAST(nsIStorageStream*, this);
        return NS_OK;
    } else if (aIID.Equals(NS_GET_IID(nsIOutputStream))) {
        NS_ADDREF(this);
        *aInstancePtr = NS_STATIC_CAST(nsIOutputStream*, this);
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsStorageStream::Initialize(PRUint32 segmentSize, PRUint32 maxSize,
                            nsIAllocator *segmentAllocator)
{
    mSegmentedBuffer = new nsSegmentedBuffer();
    if (!mSegmentedBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
    
    mSegmentSize = segmentSize;
    mSegmentSizeLog2 = PR_FloorLog2(segmentSize);

    // Segment size must be a power of two
    if (mSegmentSize != ((PRUint32)1 << mSegmentSizeLog2))
        return NS_ERROR_INVALID_ARG;

    return mSegmentedBuffer->Init(segmentSize, maxSize, segmentAllocator);
}

NS_IMETHODIMP
nsStorageStream::GetOutputStream(PRInt32 aStartingOffset, 
                                 nsIOutputStream * *aOutputStream)
{
    NS_ENSURE_ARG(aOutputStream);
    if (mWriteInProgress)
        return NS_ERROR_NOT_AVAILABLE;

    if (mLastSegmentNum >= 0)
        mSegmentedBuffer->ReallocLastSegment(mSegmentSize);
    
    nsresult rv = Seek(aStartingOffset);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(this);
    *aOutputStream = NS_STATIC_CAST(nsIOutputStream*, this);
    mWriteInProgress = true;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Close()
{
    mWriteInProgress = false;
    
    PRInt32 segmentOffset = SegOffset(mLogicalLength);

    if (segmentOffset)
        mSegmentedBuffer->ReallocLastSegment(segmentOffset);
    
    Seek(0);

    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Flush()
{
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Write(const char *aBuffer, PRUint32 aCount, PRUint32 *aNumWritten)
{
    const char* readCursor;
    PRUint32 count, availableInSegment, remaining;
    nsresult rv = NS_OK;

    NS_ENSURE_ARG(aNumWritten);
    NS_ENSURE_ARG(aBuffer);

    remaining = aCount;
    readCursor = aBuffer;
    while (remaining) {
        availableInSegment = mSegmentEnd - mWriteCursor;
        if (!availableInSegment) {
            mWriteCursor = mSegmentedBuffer->AppendNewSegment();
            mLastSegmentNum++;
            if (!mWriteCursor) {
                rv = NS_ERROR_OUT_OF_MEMORY;
                goto out;
            }
            mSegmentEnd = mWriteCursor + mSegmentSize;
            availableInSegment = mSegmentEnd - mWriteCursor;
        }
	
        count = PR_MIN(availableInSegment, remaining);
        memcpy(mWriteCursor, readCursor, count);
        remaining -= count;
        readCursor += count;
        mWriteCursor += count;
    };

 out:
    *aNumWritten = aCount - remaining;
    mLogicalLength += *aNumWritten;

    return rv;
}

NS_IMETHODIMP
nsStorageStream::GetLength(PRUint32 *aLength)
{
    NS_ENSURE_ARG(aLength);
    *aLength = mLogicalLength;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::SetLength(PRUint32 aLength)
{
    if (mWriteInProgress)
        return NS_ERROR_NOT_AVAILABLE;

    if (aLength > mLogicalLength)
        return NS_ERROR_INVALID_ARG;

    PRInt32 newLastSegmentNum = SegNum(aLength);
    PRInt32 segmentOffset = SegOffset(aLength);
    if (segmentOffset == 0)
        newLastSegmentNum--;

    while (newLastSegmentNum < mLastSegmentNum) {
        mSegmentedBuffer->DeleteLastSegment();
        mLastSegmentNum--;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::GetWriteInProgress(PRBool *aWriteInProgress)
{
    NS_ENSURE_ARG(aWriteInProgress);

    *aWriteInProgress = mWriteInProgress;
    return NS_OK;
}

NS_METHOD
nsStorageStream::Seek(PRInt32 aPosition)
{
    // An argument of -1 means "seek to end of stream"
    if (aPosition == -1)
        aPosition = mLogicalLength;
    if ((PRUint32)aPosition > mLogicalLength)
        return NS_ERROR_INVALID_ARG;

    if (aPosition == 0) {
        mWriteCursor = 0;
        mSegmentEnd = 0;
        return NS_OK;
    }

    PRUint32 segmentNum = SegNum(aPosition);
    PRUint32 segmentOffset = SegOffset(aPosition);
    if (!segmentOffset) {
        segmentNum--;
        segmentOffset = mSegmentSize;
    }

    mWriteCursor = mSegmentedBuffer->GetSegment(mLastSegmentNum);
    mSegmentEnd = mWriteCursor + mSegmentSize;
    mWriteCursor += segmentOffset;
    
    return NS_OK;
}

class nsStorageInputStream : public nsIInputStream
{
public:
    nsStorageInputStream(nsStorageStream *aStorageStream, PRUint32 aSegmentSize)
        : mStorageStream(aStorageStream), mReadCursor(0),
          mSegmentEnd(0), mLogicalCursor(0), mSegmentNum(0),
          mSegmentSize(aSegmentSize)
	{
        NS_ADDREF(mStorageStream);
	    NS_INIT_REFCNT();
	}

    virtual ~nsStorageInputStream()
    {
        NS_IF_RELEASE(mStorageStream);
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIBASESTREAM
    NS_DECL_NSIINPUTSTREAM

protected:
    
    NS_METHOD Seek(PRUint32 aPosition);

    friend class nsStorageStream;

private:
    const char*      mReadCursor;    // Next memory location to read byte or NULL
    const char*      mSegmentEnd;    // One byte past end of current buffer segment
    PRUint32         mSegmentNum;
    PRUint32         mSegmentSize;   // All segments, except the last, are of this size
    PRUint32         mLogicalCursor;
    nsStorageStream* mStorageStream;

    PRUint32 SegNum(PRUint32 aPosition)    {return aPosition >> mStorageStream->mSegmentSizeLog2;}
    PRUint32 SegOffset(PRUint32 aPosition) {return aPosition & (mSegmentSize - 1);}
};

static NS_DEFINE_IID(kIInputStream, NS_IINPUTSTREAM_IID);
NS_IMPL_ISUPPORTS(nsStorageInputStream, kIInputStream)

NS_IMETHODIMP
nsStorageStream::NewInputStream(PRInt32 aStartingOffset, nsIInputStream* *aInputStream)
{
    nsStorageInputStream *inputStream = new nsStorageInputStream(this, mSegmentSize);
    if (!inputStream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inputStream);

    if (aStartingOffset) {
        nsresult rv = inputStream->Seek(aStartingOffset);
        if (NS_FAILED(rv)) {
            NS_RELEASE(inputStream);
            return rv;
        }
    }

    *aInputStream = inputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Close()
{
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Available(PRUint32 *aAvailable)
{
    *aAvailable = mStorageStream->mLogicalLength - mLogicalCursor;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aNumRead)
{
    char* writeCursor;
    PRUint32 count, availableInSegment, remainingCapacity;

    remainingCapacity = aCount;
    writeCursor = aBuffer;
    while (remainingCapacity) {
	availableInSegment = mSegmentEnd - mReadCursor;
	if (!availableInSegment) {
	    PRUint32 available = mStorageStream->mLogicalLength - mLogicalCursor;
	    if (!available)
            goto out;
	    
	    mReadCursor = mStorageStream->mSegmentedBuffer->GetSegment(mSegmentNum++);
	    mSegmentEnd = mReadCursor + PR_MIN(mSegmentSize, available);
	}
	
	count = PR_MIN(availableInSegment, remainingCapacity);
	memcpy(writeCursor, mReadCursor, count);
	remainingCapacity -= count;
	mReadCursor += count;
	writeCursor += count;
	mLogicalCursor += count;
    };

 out:
    *aNumRead = aCount - remainingCapacity;

    if (*aNumRead == 0)
	return NS_BASE_STREAM_WOULD_BLOCK;
    else
	return NS_OK;
}

NS_METHOD
nsStorageInputStream::Seek(PRUint32 aPosition)
{
    PRUint32 length = mStorageStream->mLogicalLength;
    if (aPosition >= length)
        return NS_ERROR_INVALID_ARG;

    mSegmentNum = SegNum(aPosition);
    PRUint32 segmentOffset = SegOffset(aPosition);
    mReadCursor = mStorageStream->mSegmentedBuffer->GetSegment(mSegmentNum) +
        segmentOffset;
    PRUint32 available = length - aPosition;
    mSegmentEnd = mReadCursor + PR_MIN(mSegmentSize - segmentOffset, available);
    mLogicalCursor = aPosition;
    return NS_OK;
}
