/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla FastLoad code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (original author)
 */

#include <string.h>
#include "prtypes.h"
#include "nsDebug.h"
#include "nsMemory.h"

#include "nsIComponentManager.h"
#include "nsISeekableStream.h"
#include "nsISerializable.h"
#include "nsIStreamBufferAccess.h"

#include "nsBinaryStream.h"
#include "nsFastLoadFile.h"

/*
 * Fletcher's 16-bit checksum, using 32-bit two's-complement arithmetic.
 */
PR_IMPLEMENT(PRUint32)
NS_AccumulateFastLoadChecksum(PRUint32 aChecksum,
                              const char* aBuffer,
                              PRUint32 aLength)
{
    PRUint32 A = aChecksum & 0xffff;
    PRUint32 B = aChecksum >> 16;

#define FOLD_ONES_COMPLEMENT_CARRY(X)   ((X) = ((X) & 0xffff) + ((X) >> 16))
#define ONES_COMPLEMENT_ACCUMULATE(X,Y) (X) += (Y); if ((X) & 0x80000000)     \
                                        FOLD_ONES_COMPLEMENT_CARRY(X)

    PRUint16 U;
    if (aLength && ((PRWord)aBuffer & 1)) {
        U = *aBuffer++;        // Big endian: higher address, lower order
        ONES_COMPLEMENT_ACCUMULATE(A, U);
        ONES_COMPLEMENT_ACCUMULATE(B, A);
        aLength--;
    }

    while (aLength > 1) {
        U = *NS_REINTERPRET_CAST(const PRUint16 *, aBuffer);
        U = NS_SWAP16(U);
        ONES_COMPLEMENT_ACCUMULATE(A, U);
        ONES_COMPLEMENT_ACCUMULATE(B, A);
        aBuffer += 2;
        aLength -= 2;
    }

    if (aLength) {
        U = *aBuffer << 8;     // Big endian: lower address, higher order
        ONES_COMPLEMENT_ACCUMULATE(A, U);
        ONES_COMPLEMENT_ACCUMULATE(B, A);
    }

    while (A >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(A);
    while (B >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(B);

#undef FOLD_ONES_COMPLEMENT_CARRY
#undef ONES_COMPLEMENT_ACCUMULATE

    return (B << 16) | A;
}

static const char magic[] = MFL_FILE_MAGIC;

// -------------------------- nsFastLoadFileReader --------------------------

NS_IMPL_ISUPPORTS2(nsFastLoadFileReader,
                   nsIObjectInputStream,
                   nsISeekableStream)

nsresult
nsFastLoadFileReader::ReadHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesRead;

    rv = Read(aHeader->mMagic, MFL_FILE_MAGIC_SIZE, &bytesRead);
    if (NS_FAILED(rv)) return rv;

    if (bytesRead != MFL_FILE_MAGIC_SIZE ||
        memcmp(aHeader->mMagic, magic, MFL_FILE_MAGIC_SIZE)) {
        return NS_ERROR_FAILURE;
    }

    rv = Read32(&aHeader->mChecksum);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aHeader->mVersion);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aHeader->mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aHeader->mFileSize);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFooter(nsFastLoadFooter *aFooter)
{
    nsresult rv;

    rv = ReadFooterPrefix(aFooter);
    if (NS_FAILED(rv)) return rv;

    aFooter->mIDMap = new nsID[aFooter->mNumIDs];
    if (!aFooter->mIDMap)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 i, n;
    for (i = 0, n = aFooter->mNumIDs; i < n; i++) {
        rv = ReadSlowID(&aFooter->mIDMap[i]);
        if (NS_FAILED(rv)) return rv;
    }

    aFooter->mSharpObjectMap =
        new nsFastLoadSharpObjectEntry[aFooter->mNumSharpObjects];
    if (!aFooter->mSharpObjectMap)
        return NS_ERROR_OUT_OF_MEMORY;

    for (i = 0, n = aFooter->mNumSharpObjects; i < n; i++) {
        nsFastLoadSharpObjectEntry& entry = aFooter->mSharpObjectMap[i];

        rv = ReadSharpObjectInfo(&entry);
        if (NS_FAILED(rv)) return rv;

        entry.mObject = nsnull;
    }

    for (i = 0, n = aFooter->mNumDependencies; i < n; i++) {
        char* s;
        rv = ReadStringZ(&s);
        if (NS_FAILED(rv)) return rv;

        if (!aFooter->AppendDependency(s, PR_FALSE)) {
            nsMemory::Free(s);
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFooterPrefix(nsFastLoadFooterPrefix *aFooterPrefix)
{
    nsresult rv;

    rv = Read32(&aFooterPrefix->mNumIDs);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aFooterPrefix->mNumSharpObjects);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aFooterPrefix->mNumDependencies);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadSlowID(nsID *aID)
{
    nsresult rv;

    rv = Read32(&aID->m0);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aID->m1);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aID->m2);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bytesRead;
    rv = Read(NS_REINTERPRET_CAST(char*, aID->m3), sizeof aID->m3, &bytesRead);
    if (NS_FAILED(rv)) return rv;

    if (bytesRead != sizeof aID->m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadSharpObjectInfo(nsFastLoadSharpObjectInfo *aInfo)
{
    nsresult rv;

    rv = Read32(&aInfo->mCIDOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aInfo->mStrongRefCnt);
    if (NS_FAILED(rv)) return rv;

    rv = Read16(&aInfo->mWeakRefCnt);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::Open()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    if (!seekable)
        return NS_ERROR_FAILURE;

    nsresult rv;
    
    rv = ReadHeader(&mHeader);
    if (NS_FAILED(rv)) return rv;

    if (mHeader.mVersion != MFL_FILE_VERSION_0)
        return NS_ERROR_FAILURE;

    PRUint32 dataOffset;
    rv = seekable->Tell(&dataOffset);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_END, 0);
    if (NS_FAILED(rv)) return rv;

    PRUint32 fileSize;
    rv = seekable->Tell(&fileSize);
    if (NS_FAILED(rv)) return rv;

    if (fileSize != mHeader.mFileSize)
        return NS_ERROR_FAILURE;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        PRInt32(mHeader.mFooterOffset));
    if (NS_FAILED(rv)) return rv;

    rv = ReadFooter(&mFooter);
    if (NS_FAILED(rv)) return rv;

    return seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                          PRInt32(dataOffset));
}

NS_IMETHODIMP
nsFastLoadFileReader::Close()
{
    // Give up any dangling strong refs, after asserting there aren't any.
    for (PRUint32 i = 0, n = mFooter.mNumSharpObjects; i < n; i++) {
        nsFastLoadSharpObjectEntry* entry = &mFooter.mSharpObjectMap[i];

        NS_ASSERTION(entry->mStrongRefCnt == 0,
                     "failed to deserialize all strong refs!");
        NS_ASSERTION(entry->mWeakRefCnt == 0,
                     "failed to deserialize all weak refs!");

        entry->mObject = nsnull;
    }

    return mInputStream->Close();
}

nsresult
nsFastLoadFileReader::DeserializeObject(nsISupports* *aObject)
{
    nsresult rv;
    NSFastLoadID fastCID;

    rv = Read32(&fastCID);
    if (NS_FAILED(rv)) return rv;

    const nsID& slowCID = mFooter.GetID(fastCID);
    nsCOMPtr<nsISupports> object(do_CreateInstance(slowCID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISerializable> serializable(do_QueryInterface(object));
    if (!serializable)
        return NS_ERROR_FAILURE;

    rv = serializable->Read(this);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aObject = object);
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadObject(PRBool aIsStrongRef, nsISupports* *aObject)
{
    nsresult rv;

    NSFastLoadOID oid;
    rv = Read32(&oid);
    if (NS_FAILED(rv)) return rv;

    nsFastLoadSharpObjectEntry* entry = (oid != MFL_DULL_OBJECT_OID)
                                        ? &mFooter.GetSharpObjectEntry(oid)
                                        : nsnull;
    nsCOMPtr<nsISupports> object;

    if (!entry) {
        // A very dull object, defined at point of single (strong) reference.
        NS_ASSERTION(aIsStrongRef, "dull object read via weak ref!");

        rv = DeserializeObject(getter_AddRefs(object));
        if (NS_FAILED(rv)) return rv;
    } else {
        NS_ASSERTION((oid & MFL_WEAK_REF_TAG) ==
                     (aIsStrongRef ? 0 : MFL_WEAK_REF_TAG),
                     "strong vs. weak ref deserialization mismatch!");

        // Check whether we've already deserialized the object for this OID.
        object = entry->mObject;

        if (!object) {
            // We skipped deserialization of this object from its position
            // earlier in the input stream, presumably due to the reference
            // there being an nsFastLoadPtr or some such thing.  Seek back
            // and read it now.
            nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
            PRUint32 saveOffset;

            rv = seekable->Tell(&saveOffset);
            if (NS_FAILED(rv)) return rv;
            NS_ASSERTION(entry->mCIDOffset < saveOffset, "out of order object?!");

            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                entry->mCIDOffset);
            if (NS_FAILED(rv)) return rv;

            rv = DeserializeObject(getter_AddRefs(object));
            if (NS_FAILED(rv)) return rv;

            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
            if (NS_FAILED(rv)) return rv;

            // Save object until all refs have been deserialized.
            entry->mObject = object;
        }

        if (aIsStrongRef) {
            NS_ASSERTION(entry->mStrongRefCnt != 0, "mStrongRefCnt underflow!");
            entry->mStrongRefCnt--;
            nsISupports* rawPtr = object.get();
            NS_ADDREF(rawPtr);
        } else {
            NS_ASSERTION(entry->mWeakRefCnt != 0, "mWeakRefCnt underflow!");
            entry->mWeakRefCnt--;
        }

        if (entry->mStrongRefCnt == 0 && entry->mWeakRefCnt == 0)
            entry->mObject = nsnull;
    }

    if (oid & MFL_QUERY_INTERFACE_TAG) {
        NSFastLoadID iid;
        rv = Read32(&iid);
        if (NS_FAILED(rv)) return rv;

        rv = object->QueryInterface(mFooter.GetID(iid),
                                    NS_REINTERPRET_CAST(void**, aObject));
        if (NS_FAILED(rv)) return rv;
    } else {
        *aObject = object;
        NS_ADDREF(*aObject);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ReadID(nsID *aResult)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = Read32(&fastID);
    if (NS_FAILED(rv)) return rv;

    *aResult = mFooter.GetID(fastID);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Seek(PRInt32 aWhence, PRInt32 aOffset)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    return seekable->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFastLoadFileReader::Tell(PRUint32 *aResult)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
    return seekable->Tell(aResult);
}

NS_COM nsresult
NS_NewFastLoadFileReader(nsIObjectInputStream* *aResult,
                         nsIInputStream* aSrcStream)
{
    nsFastLoadFileReader* reader = new nsFastLoadFileReader(aSrcStream);
    if (!reader)
        return NS_ERROR_OUT_OF_MEMORY;

    // Stabilize reader's refcnt.
    nsCOMPtr<nsIObjectInputStream> stream(reader);

    nsresult rv = reader->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}

// -------------------------- nsFastLoadFileWriter --------------------------

NS_IMPL_ISUPPORTS2(nsFastLoadFileWriter,
                   nsIObjectOutputStream,
                   nsISeekableStream)

struct nsIDMapEntry : public PLDHashEntryHdr {
    NSFastLoadID    mFastID;            // 1 + nsFastLoadFooter::mIDMap index
    nsID            mSlowID;            // key, used by PLDHashTableOps below
};

PR_STATIC_CALLBACK(const void *)
idmap_GetKey(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsIDMapEntry* entry = NS_STATIC_CAST(nsIDMapEntry*, aHdr);

    return &entry->mSlowID;
}

PR_STATIC_CALLBACK(PLDHashNumber)
idmap_HashKey(PLDHashTable *aTable, const void *aKey)
{
    const nsID *idp = NS_REINTERPRET_CAST(const nsID*, aKey);
    return idp->m0;
}

PR_STATIC_CALLBACK(PRBool)
idmap_MatchEntry(PLDHashTable *aTable,
                const PLDHashEntryHdr *aHdr,
                const void *aKey)
{
    const nsIDMapEntry* entry = NS_STATIC_CAST(const nsIDMapEntry*, aHdr);
    const nsID *idp = NS_REINTERPRET_CAST(const nsID*, aKey);

    return memcmp(&entry->mSlowID, idp, sizeof(nsID)) == 0;
}

static PLDHashTableOps idmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    idmap_GetKey,
    idmap_HashKey,
    idmap_MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    NULL
};

nsresult
nsFastLoadFileWriter::MapID(const nsID& aSlowID, NSFastLoadID *aResult)
{
    nsIDMapEntry *entry =
        NS_STATIC_CAST(nsIDMapEntry*,
                       PL_DHashTableOperate(&mIDMap, &aSlowID, PL_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    if (entry->mFastID == 0) {
        entry->mFastID = mIDMap.entryCount;
        entry->mSlowID = aSlowID;
    }

    *aResult = entry->mFastID;
    return NS_OK;
}

struct nsObjectMapEntry : public PLDHashEntryHdr {
    nsISupports*                mObject;        // key, must come first
    NSFastLoadOID               mOID;
    nsFastLoadSharpObjectInfo   mInfo;
};

PR_STATIC_CALLBACK(void)
objmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsObjectMapEntry* entry = NS_STATIC_CAST(nsObjectMapEntry*, aHdr);

    NS_IF_RELEASE(entry->mObject);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static PLDHashTableOps objmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    objmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};

nsresult
nsFastLoadFileWriter::WriteHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write(aHeader->mMagic, MFL_FILE_MAGIC_SIZE, &bytesWritten);
    if (NS_FAILED(rv)) return rv;

    if (bytesWritten != MFL_FILE_MAGIC_SIZE)
        return NS_ERROR_FAILURE;

    rv = Write32(aHeader->mChecksum);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mVersion);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aHeader->mFileSize);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteSlowID(const nsID& aID)
{
    nsresult rv;

    rv = Write32(aID.m0);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aID.m1);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aID.m2);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bytesWritten;
    rv = Write(NS_REINTERPRET_CAST(const char*, aID.m3), sizeof aID.m3,
               &bytesWritten);
    if (NS_FAILED(rv)) return rv;

    if (bytesWritten != sizeof aID.m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteSharpObjectInfo(const nsFastLoadSharpObjectInfo& aInfo)
{
    nsresult rv;

    rv = Write32(aInfo.mCIDOffset);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aInfo.mStrongRefCnt);
    if (NS_FAILED(rv)) return rv;

    rv = Write16(aInfo.mWeakRefCnt);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::IDMapEnumerate(PLDHashTable *aTable,
                                     PLDHashEntryHdr *aHdr,
                                     PRUint32 aNumber,
                                     void *aData)
{
    nsIDMapEntry* entry = NS_STATIC_CAST(nsIDMapEntry*, aHdr);
    PRUint32 index = entry->mFastID - 1;
    nsID* vector = NS_REINTERPRET_CAST(nsID*, aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsIDMap index!");
    vector[index] = entry->mSlowID;
    return PL_DHASH_NEXT;
}

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::ObjectMapEnumerate(PLDHashTable *aTable,
                                         PLDHashEntryHdr *aHdr,
                                         PRUint32 aNumber,
                                         void *aData)
{
    nsObjectMapEntry* entry = NS_STATIC_CAST(nsObjectMapEntry*, aHdr);
    PRUint32 index = MFL_OID_TO_SHARP_INDEX(entry->mOID);
    nsFastLoadSharpObjectInfo* vector =
        NS_REINTERPRET_CAST(nsFastLoadSharpObjectInfo*, aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsObjectMap index!");
    vector[index] = entry->mInfo;

#ifdef NS_DEBUG
    NS_ASSERTION(entry->mInfo.mStrongRefCnt, "no strong ref in serialization!");

    nsrefcnt rc = entry->mObject->AddRef();
    NS_ASSERTION(entry->mInfo.mStrongRefCnt <= rc - 2,
                 "too many strong refs in serialization");
    entry->mObject->Release();
#endif
    NS_RELEASE(entry->mObject);

    return PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileWriter::WriteFooter()
{
    nsresult rv;
    PRUint32 i, length, count;

    // Enumerate mIDMap into a vector indexed by mFastID and write it.
    length = mIDMap.entryCount;
    nsID* idvec = new nsID[length];
    if (!idvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mIDMap, IDMapEnumerate, idvec);
    NS_ASSERTION(count == length, "bad mIDMap enumeration!");

    rv = Write32(length);
    if (NS_SUCCEEDED(rv)) {
        for (i = 0; i < length; i++) {
            rv = WriteSlowID(idvec[i]);
            if (NS_FAILED(rv)) break;
        }
    }

    delete idvec;
    if (NS_FAILED(rv)) return rv;

    // Enumerate mObjectMap into a vector indexed by mOID and write it.
    length = mObjectMap.entryCount;
    nsFastLoadSharpObjectInfo* infovec = new nsFastLoadSharpObjectInfo[length];
    if (!infovec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mObjectMap, ObjectMapEnumerate, infovec);
    NS_ASSERTION(count == length, "bad mObjectMap enumeration!");

    rv = Write32(length);
    if (NS_SUCCEEDED(rv)) {
        for (i = 0; i < length; i++) {
            rv = WriteSharpObjectInfo(infovec[i]);
            if (NS_FAILED(rv)) break;
        }
    }

    delete infovec;
    if (NS_FAILED(rv)) return rv;

    count = mDependencies.Count();
    rv = Write32(count);
    if (NS_FAILED(rv)) return rv;

    for (i = 0; i < count; i++) {
        rv = WriteStringZ(NS_REINTERPRET_CAST(const char*,
                                              mDependencies.ElementAt(PRInt32(i))));
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::Open()
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    if (!seekable)
        return NS_ERROR_FAILURE;

    nsresult rv;
    
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        sizeof(nsFastLoadHeader));
    if (NS_FAILED(rv)) return rv;

    if (!PL_DHashTableInit(&mIDMap, &idmap_DHashTableOps, (void *)this,
                           sizeof(nsIDMapEntry), PL_DHASH_MIN_SIZE)) {
        mIDMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mObjectMap, &objmap_DHashTableOps, (void *)this,
                           sizeof(nsObjectMapEntry), PL_DHASH_MIN_SIZE)) {
        PL_DHashTableFinish(&mIDMap);
        mIDMap.ops = mObjectMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::Close()
{
    nsresult rv;
    nsFastLoadHeader header;

    memcpy(header.mMagic, magic, MFL_FILE_MAGIC_SIZE);
    header.mChecksum = 0;
    header.mVersion = MFL_FILE_VERSION_0;
    
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));

    rv = seekable->Tell(&header.mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    rv = WriteFooter();
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Tell(&header.mFileSize);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;

    rv = WriteHeader(&header);
    if (NS_FAILED(rv)) return rv;

    return mOutputStream->Close();
}

// Psuedo-tag used as flag between WriteSingleRefObject and WriteCommon.
#define MFL_SINGLE_REF_PSEUDO_TAG       PR_BIT(MFL_OBJECT_TAG_BITS)

nsresult
nsFastLoadFileWriter::WriteObjectCommon(nsISupports* aObject,
                                        PRBool aIsStrongRef,
                                        PRUint32 aTags)
{
    nsrefcnt rc;
    nsresult rv;
    
    // Here be manual refcounting dragons!
    rc = aObject->AddRef();
    NS_ASSERTION(rc != 0, "bad refcnt when writing aObject!");

    NSFastLoadOID oid;

    if (rc == 2 && (aTags & MFL_SINGLE_REF_PSEUDO_TAG)) {
        // Dull object: only one strong ref and no weak refs in serialization.
        // Conservative: we don't trust the caller if there are more than two
        // refs (one from the AddRef above, one from the data structure that's
        // being serialized).
        oid = MFL_DULL_OBJECT_OID;
        aObject->Release();
    } else {
        // Object is presumed to be multiply connected through some combo of
        // strong and weak refs.  Hold onto it via mObjectMap.
        nsObjectMapEntry* entry =
            NS_STATIC_CAST(nsObjectMapEntry*,
                           PL_DHashTableOperate(&mObjectMap, aObject,
                                                PL_DHASH_ADD));
        if (!entry) {
            aObject->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }

        if (!entry->mObject) {
            // First time we've seen this object address: add it to mObjectMap
            // and serialize the object at the current stream offset.
            nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
            PRUint32 thisOffset;

            rv = seekable->Tell(&thisOffset);
            if (NS_FAILED(rv)) {
                aObject->Release();
                return rv;
            }

            // NB: aObject was already held, and mObject is a raw nsISupports*.
            entry->mObject = aObject;

            oid = (mObjectMap.entryCount << MFL_OBJECT_TAG_BITS);
            entry->mOID = oid;

            // NB: the (32-bit, fast) CID and object data follow the OID.
            entry->mInfo.mCIDOffset = thisOffset + sizeof(oid);
            entry->mInfo.mStrongRefCnt = aIsStrongRef ? 1 : 0;
            entry->mInfo.mWeakRefCnt   = aIsStrongRef ? 0 : 1;

            oid |= MFL_OBJECT_DEF_TAG;
        } else {
            // Already serialized, recover oid and update the desired refcnt.
            oid = entry->mOID;
            if (aIsStrongRef)
                entry->mInfo.mStrongRefCnt++;
            else
                entry->mInfo.mWeakRefCnt++;

            aObject->Release();
        }
    }

    if (!aIsStrongRef)
        oid |= MFL_WEAK_REF_TAG;
    oid |= (aTags & MFL_QUERY_INTERFACE_TAG);

    rv = Write32(oid);
    if (NS_FAILED(rv)) return rv;

    if (oid & MFL_OBJECT_DEF_TAG) {
        nsCOMPtr<nsISerializable> serializable(do_QueryInterface(aObject));
        if (!serializable)
            return NS_ERROR_FAILURE;

        nsCID slowCID;
        rv = serializable->GetCID(&slowCID);
        if (NS_FAILED(rv)) return rv;

        NSFastLoadID fastCID;
        rv = MapID(slowCID, &fastCID);
        if (NS_FAILED(rv)) return rv;

        rv = Write32(fastCID);
        if (NS_FAILED(rv)) return rv;

        rv = serializable->Write(this);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteObject(nsISupports* aObject, PRBool aIsStrongRef)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, aIsStrongRef, 0);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteSingleRefObject(nsISupports* aObject)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, PR_TRUE, MFL_SINGLE_REF_PSEUDO_TAG);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteCompoundObject(nsISupports* aObject,
                                          const nsIID& aIID,
                                          PRBool aIsStrongRef)
{
    nsresult rv;
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> roundtrip;
    rootObject->QueryInterface(aIID, getter_AddRefs(roundtrip));

    NS_ASSERTION(rootObject.get() != aObject,
                 "wasteful call to WriteCompoundObject -- call WriteObject!");
    NS_ASSERTION(roundtrip.get() == aObject,
                 "bad aggregation or multiple inheritance detected by call to WriteCompoundObject!");
#endif

    rv = WriteObjectCommon(rootObject, aIsStrongRef, MFL_QUERY_INTERFACE_TAG);
    if (NS_FAILED(rv)) return rv;

    NSFastLoadID iid;
    rv = MapID(aIID, &iid);
    if (NS_FAILED(rv)) return rv;

    return Write32(iid);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteID(const nsID& aID)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = MapID(aID, &fastID);
    if (NS_FAILED(rv)) return rv;

    return Write32(fastID);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Seek(PRInt32 aWhence, PRInt32 aOffset)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    return seekable->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Tell(PRUint32 *aResult)
{
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));
    return seekable->Tell(aResult);
}

NS_COM nsresult
NS_NewFastLoadFileWriter(nsIObjectOutputStream* *aResult,
                         nsIOutputStream* aDestStream)
{
    nsFastLoadFileWriter* writer = new nsFastLoadFileWriter(aDestStream);
    if (!writer)
        return NS_ERROR_OUT_OF_MEMORY;

    // Stabilize writer's refcnt.
    nsCOMPtr<nsIObjectOutputStream> stream(writer);

    nsresult rv = writer->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}
