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

#ifndef _nsStorageStream_h_
#define _nsStorageStream_h_

#include "nsIStorageStream.h"
#include "nsIOutputStream.h"

class nsSegmentedBuffer;

class NS_COM nsStorageStream : public nsIStorageStream,
                               public nsIOutputStream
{
public:
    nsStorageStream();
    ~nsStorageStream();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTORAGESTREAM
    NS_DECL_NSIBASESTREAM
    NS_DECL_NSIOUTPUTSTREAM

    friend class nsStorageInputStream;

private:
    nsSegmentedBuffer* mSegmentedBuffer;
    PRUint32           mSegmentSize;
    PRUint32           mSegmentSizeLog2;
    bool               mWriteInProgress;
    PRInt32            mLastSegmentNum;    // Last segment in use, -1 initially
    char*              mWriteCursor;       /* Pointer within segment to next to
                                              next byte to be written */
    char*              mSegmentEnd;        // Pointer to one byte after segment end
    PRUint32           mLogicalLength;     // Number of bytes written to stream

    NS_METHOD Seek(PRInt32 aPosition);
    PRUint32 SegNum(PRUint32 aPosition)    {return aPosition >> mSegmentSizeLog2;}
    PRUint32 SegOffset(PRUint32 aPosition) {return aPosition & (mSegmentSize - 1);}
};

#endif //  _nsStorageStream_h_
