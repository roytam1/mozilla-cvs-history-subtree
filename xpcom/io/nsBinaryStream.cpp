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
 *
 */

#include "nsBinaryStream.h"
#include "nsIAllocator.h"

#ifdef IS_LITTLE_ENDIAN
#    define SWAP16(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#    define SWAP32(x) (SWAP16(((x) & 0xffff) << 16) | SWAP16(((x) >> 16) & 0xffff))
#else
#    ifndef IS_BIG_ENDIAN
#        error "Unknown endianness"
#    endif
#    define SWAP16(x) (x)
#    define SWAP32(x) (x)
#endif

nsBinaryOutputStream::nsBinaryOutputStream(nsIOutputStream* aStream): mOutputStream(aStream) {}

NS_IMPL_ISUPPORTS(nsBinaryOutputStream, NS_GET_IID(nsIBinaryOutputStream))

NS_IMETHODIMP
nsBinaryOutputStream::Flush() { return mOutputStream->Flush(); }

NS_IMETHODIMP
nsBinaryOutputStream::Close() { return mOutputStream->Close(); }

NS_IMETHODIMP
nsBinaryOutputStream::Write(const char *aBuf, PRUint32 aCount, PRUint32 *aActualBytes)
{
    return mOutputStream->Write(aBuf, aCount, aActualBytes);
}

NS_IMETHODIMP
nsBinaryOutputStream::SetOutputStream(nsIOutputStream *aOutputStream)
{
    NS_ENSURE_ARG_POINTER(aOutputStream);
    mOutputStream = aOutputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteBoolean(PRBool aBoolean)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write((const char*)&aBoolean, sizeof aBoolean, &bytesWritten);
    if (bytesWritten != sizeof aBoolean)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::Write8(PRUint8 aByte)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write((const char*)&aByte, sizeof aByte, &bytesWritten);
    if (bytesWritten != sizeof aByte)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::Write16(PRUint16 a16)
{
    nsresult rv;
    PRUint32 bytesWritten;

    a16 = SWAP16(a16);
    rv = Write((const char*)&a16, sizeof a16, &bytesWritten);
    if (bytesWritten != sizeof a16)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::Write32(PRUint32 a32)
{
    nsresult rv;
    PRUint32 bytesWritten;

    a32 = SWAP32(a32);
    rv = Write((const char*)&a32, sizeof a32, &bytesWritten);
    if (bytesWritten != sizeof a32)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryOutputStream::Write64(PRUint64 a64)
{
    nsresult rv;
    PRUint32* raw32 = (PRUint32*)&a64;

#ifdef IS_BIG_ENDIAN
    rv = Write32(raw32[0]);
    if (NS_FAILED(rv)) return rv;
    return Write32(raw32[1]);
#else
    rv = Write32(raw32[1]);
    if (NS_FAILED(rv)) return rv;
    return Write32(raw32[0]);
#endif
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteFloat(float aFloat)
{
    NS_ASSERTION(sizeof(float) == sizeof (PRUint32),
                 "False assumption about sizeof(float)");
    return Write32(*NS_REINTERPRET_CAST(PRUint32*, &aFloat));
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteDouble(double aDouble)
{
    NS_ASSERTION(sizeof(double) == sizeof(PRUint64),
                 "False assumption about sizeof(double)");
    return Write64(*NS_REINTERPRET_CAST(PRUint64*,&aDouble));
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteString(const char *aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteWString(const PRUnichar* aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteUtf8(const PRUnichar* aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryOutputStream::WriteBytes(const char *aString, PRUint32 aLength)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write(aString, aLength, &bytesWritten);
    if (bytesWritten != aLength)
        return NS_ERROR_FAILURE;
    return rv;
}

nsBinaryInputStream::nsBinaryInputStream(nsIInputStream* aStream): mInputStream(aStream) {}

NS_IMPL_ISUPPORTS(nsBinaryInputStream, NS_GET_IID(nsIBinaryInputStream))

NS_IMETHODIMP
nsBinaryInputStream::Available(PRUint32* aResult) { return mInputStream->Available(aResult); }

NS_IMETHODIMP
nsBinaryInputStream::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aNumRead)
{
    return mInputStream->Read(aBuffer, aCount, aNumRead);
}

NS_IMETHODIMP
nsBinaryInputStream::Close() { return mInputStream->Close(); }

NS_IMETHODIMP
nsBinaryInputStream::SetInputStream(nsIInputStream *aInputStream)
{
    NS_ENSURE_ARG_POINTER(aInputStream);
    mInputStream = aInputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadBoolean(PRBool* aBoolean)
{
    return Read8((PRUint8*)aBoolean);
}

NS_IMETHODIMP
nsBinaryInputStream::Read8(PRUint8* aByte)
{
    nsresult rv;
    PRBool byte;
    PRUint32 bytesRead;
    
    rv = Read((char*)&byte, sizeof byte, &bytesRead);
    if (bytesRead != sizeof byte)
        return NS_ERROR_FAILURE;
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read16(PRUint16* a16)
{
    nsresult rv;
    PRUint32 bytesRead;

    rv = Read((char*)a16, sizeof *a16, &bytesRead);
    if (bytesRead != sizeof *a16)
        return NS_ERROR_FAILURE;
    *a16 = SWAP16(*a16);
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read32(PRUint32* a32)
{
    nsresult rv;
    PRUint32 bytesRead;

    rv = Read((char*)a32, sizeof *a32, &bytesRead);
    if (bytesRead != sizeof *a32)
        return NS_ERROR_FAILURE;
    *a32 = SWAP32(*a32);
    return rv;
}

NS_IMETHODIMP
nsBinaryInputStream::Read64(PRUint64* a64)
{
    nsresult rv;
    PRUint32* raw32 = (PRUint32*)a64;

#ifdef IS_BIG_ENDIAN
    rv = Read32(&raw32[0]);
    if (NS_FAILED(rv)) return rv;
    return Read32(&raw32[1]);
#else
    rv = Read32(&raw32[1]);
    if (NS_FAILED(rv)) return rv;
    return Read32(&raw32[0]);
#endif
}

NS_IMETHODIMP
nsBinaryInputStream::ReadFloat(float* aFloat)
{
    NS_ASSERTION(sizeof(float) == sizeof (PRUint32),
		 "False assumption about sizeof(float)");
    return Read32(NS_REINTERPRET_CAST(PRUint32*, aFloat));
}

NS_IMETHODIMP
nsBinaryInputStream::ReadDouble(double* aDouble)
{
    NS_ASSERTION(sizeof(double) == sizeof(PRUint64),
		 "False assumption about sizeof(double)");
    return Read64(NS_REINTERPRET_CAST(PRUint64*, aDouble));
}

NS_IMETHODIMP
nsBinaryInputStream::ReadString(char* *aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadWString(PRUnichar* *aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadUtf8(PRUnichar* *aString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinaryInputStream::ReadBytes(char* *aString, PRUint32 aLength)
{
    nsresult rv;
    PRUint32 bytesRead;
    char* s;

    s = (char*)nsAllocator::Alloc(aLength);
    if (!s)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = Read(s, aLength, &bytesRead);
    if (NS_FAILED(rv)) return rv;
    if (bytesRead != aLength)
	return NS_ERROR_FAILURE;

    *aString = s;
    return NS_OK;
}

NS_COM nsresult
NS_NewBinaryOutputStream(nsIBinaryOutputStream* *aResult, nsIOutputStream* aDestStream)
{
    NS_ENSURE_ARG_POINTER(aResult);
    nsIBinaryOutputStream *stream = new nsBinaryOutputStream(aDestStream);
    if (!stream)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    *aResult = stream;
    return NS_OK;
}

NS_COM nsresult
NS_NewBinaryInputStream(nsIBinaryInputStream* *aResult, nsIInputStream* aSrcStream)
{
    NS_ENSURE_ARG_POINTER(aResult);
    nsIBinaryInputStream *stream = new nsBinaryInputStream(aSrcStream);
    if (!stream)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    *aResult = stream;
    return NS_OK;
}
