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

NS_IMPL_ISUPPORTS_INHERITED2(nsFastLoadFileReader,
                             nsBinaryInputStream,
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

struct nsDocumentMapEntry : public PLDHashEntryHdr {
    const char* mURISpec;               // key, must come first
};

struct nsDocumentMapReadEntry : public nsDocumentMapEntry {
    PRUint32    mNextSegmentOffset;     // offset of URI's next segment to read
    PRUint32    mBytesLeft;             // bytes remaining in current segment
    PRUint32    mSaveOffset;            // in case demux schedule differs from
                                        // mux schedule
};

PR_STATIC_CALLBACK(PRBool)
docmap_MatchEntry(PLDHashTable *aTable,
                  const PLDHashEntryHdr *aHdr,
                  const void *aKey)
{
    const nsDocumentMapEntry* entry =
        NS_STATIC_CAST(const nsDocumentMapEntry*, aHdr);
    const char* spec = NS_REINTERPRET_CAST(const char*, aKey);

    return strcmp(entry->mURISpec, spec) == 0;
}

PR_STATIC_CALLBACK(void)
docmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsDocumentMapEntry* entry = NS_STATIC_CAST(nsDocumentMapEntry*, aHdr);

    nsMemory::Free((void*) entry->mURISpec);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static PLDHashTableOps docmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashGetKeyStub,
    PL_DHashStringKey,
    docmap_MatchEntry,
    PL_DHashMoveEntryStub,
    docmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};

nsresult
nsFastLoadFileReader::SelectMuxedDocument(const char* aURISpec)
{
    nsresult rv;

    // If we're interrupting another document's segment, save its offset so
    // we can seek back when it's reselected.
    nsDocumentMapReadEntry* entry = mCurrentDocumentMapEntry;
    if (entry && entry->mBytesLeft) {
        rv = Tell(&entry->mSaveOffset);
        if (NS_FAILED(rv)) return rv;
    }

    // Find the given URI's entry and select it for more reading.
    entry = NS_STATIC_CAST(nsDocumentMapReadEntry*,
                           PL_DHashTableOperate(&mFooter.mMuxedDocumentMap,
                                                aURISpec,
                                                PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        // XXXbe no such URI in mux -- invalidate/remove FastLoad file?
        return NS_ERROR_UNEXPECTED;
    }

    // Invariant: entry->mBytesLeft implies entry->mSaveOffset has been set
    // non-zero by the Tell call above.
    if (entry->mBytesLeft) {
        NS_ASSERTION(entry->mSaveOffset != 0, "reselecting at unsaved offset");

        rv = Seek(nsISeekableStream::NS_SEEK_SET, entry->mSaveOffset);
        if (NS_FAILED(rv)) return rv;
    }

    mCurrentDocumentMapEntry = entry;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    nsresult rv;

    nsDocumentMapReadEntry* entry = mCurrentDocumentMapEntry;
    if (entry && entry->mBytesLeft == 0) {
        rv = Seek(nsISeekableStream::NS_SEEK_SET, entry->mNextSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = Read32(&entry->mNextSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = Read32(&entry->mBytesLeft);
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(entry->mBytesLeft >= 8, "demux segment length botch!");
        entry->mBytesLeft -= 8;
    }

    rv = mInputStream->Read(aBuffer, aCount, aBytesRead);

    if (NS_SUCCEEDED(rv) && entry) {
        NS_ASSERTION(entry->mBytesLeft >= *aBytesRead, "demux underflow!");
        entry->mBytesLeft -= *aBytesRead;

#ifdef NS_DEBUG
        // Invariant: !entry->mBytesLeft implies entry->mSaveOffset == 0.
        if (entry->mBytesLeft == 0)
            entry->mSaveOffset = 0;
#endif
    }
    return rv;
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
        nsFastLoadSharpObjectEntry* entry = &aFooter->mSharpObjectMap[i];

        rv = ReadSharpObjectInfo(entry);
        if (NS_FAILED(rv)) return rv;

        entry->mObject = nsnull;
    }

    if (!PL_DHashTableInit(&aFooter->mMuxedDocumentMap, &docmap_DHashTableOps,
                           (void *)this, sizeof(nsDocumentMapReadEntry),
                           PL_DHASH_MIN_SIZE)) {
        aFooter->mMuxedDocumentMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0, n = aFooter->mNumMuxedDocuments; i < n; i++) {
        nsFastLoadMuxedDocumentInfo info;

        rv = ReadMuxedDocumentInfo(&info);
        if (NS_FAILED(rv)) return rv;

        nsDocumentMapReadEntry* entry =
            NS_STATIC_CAST(nsDocumentMapReadEntry*,
                           PL_DHashTableOperate(&aFooter->mMuxedDocumentMap,
                                                info.mURISpec,
                                                PL_DHASH_ADD));
        if (!entry) {
            nsMemory::Free((void*) info.mURISpec);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        NS_ASSERTION(!entry->mURISpec, "duplicate URISpec in MuxedDocumentMap");
        entry->mURISpec = info.mURISpec;
        entry->mNextSegmentOffset = info.mInitialSegmentOffset;
        entry->mBytesLeft = 0;
        entry->mSaveOffset = 0;
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

    rv = Read32(&aFooterPrefix->mNumMuxedDocuments);
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
nsFastLoadFileReader::ReadFastID(NSFastLoadID *aID)
{
    nsresult rv = Read32(aID);
    if (NS_SUCCEEDED(rv))
        *aID ^= MFL_ID_XOR_KEY;
    return rv;
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
nsFastLoadFileReader::ReadMuxedDocumentInfo(nsFastLoadMuxedDocumentInfo *aInfo)
{
    nsresult rv;

    char *spec;
    rv = ReadStringZ(&spec);
    if (NS_FAILED(rv)) return rv;

    rv = Read32(&aInfo->mInitialSegmentOffset);
    if (NS_FAILED(rv)) {
        nsMemory::Free((void*) spec);
        return rv;
    }

    aInfo->mURISpec = spec;
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

    if (mHeader.mVersion != MFL_FILE_VERSION)
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

    rv = ReadFastID(&fastCID);
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
    oid ^= MFL_OID_XOR_KEY;

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
            nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));
            PRUint32 saveOffset;

            rv = seekable->Tell(&saveOffset);
            if (NS_FAILED(rv)) return rv;

            if (entry->mCIDOffset != saveOffset) {
                // We skipped deserialization of this object from its position
                // earlier in the input stream, presumably due to the reference
                // there being an nsFastLoadPtr or some such thing.  Seek back
                // and read it now.
                NS_ASSERTION(entry->mCIDOffset < saveOffset,
                             "out of order object?!");

                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                    entry->mCIDOffset);
                if (NS_FAILED(rv)) return rv;
            }

            rv = DeserializeObject(getter_AddRefs(object));
            if (NS_FAILED(rv)) return rv;

            if (entry->mCIDOffset != saveOffset) {
                // Restore offset in case we're still reading forward to get
                // object definitions eagerly.
                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
                if (NS_FAILED(rv)) return rv;
            }

            // Save object until all refs have been deserialized.
            entry->mObject = object;
        }

        if (aIsStrongRef) {
            NS_ASSERTION(entry->mStrongRefCnt != 0, "mStrongRefCnt underflow!");
            entry->mStrongRefCnt--;
        } else {
            NS_ASSERTION(entry->mWeakRefCnt != 0, "mWeakRefCnt underflow!");
            entry->mWeakRefCnt--;
        }

        if (entry->mStrongRefCnt == 0 && entry->mWeakRefCnt == 0)
            entry->mObject = nsnull;
    }

    if (oid & MFL_QUERY_INTERFACE_TAG) {
        NSFastLoadID iid;
        rv = ReadFastID(&iid);
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

    rv = ReadFastID(&fastID);
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

NS_IMPL_ISUPPORTS_INHERITED2(nsFastLoadFileWriter,
                             nsBinaryOutputStream,
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
    nsIDMapEntry* entry =
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

struct nsDocumentMapWriteEntry : public nsDocumentMapEntry {
    PRUint32    mInitialSegmentOffset;      // offset of URI's first segment
    PRUint32    mCurrentSegmentOffset;      // last written segment's offset
};

nsresult
nsFastLoadFileWriter::SelectMuxedDocument(const char* aURISpec)
{
    // Avoid repeatedly QI'ing to nsISeekableStream as we tell and seek.
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));

    // Capture the current file offset (XXXbe maintain our own via Write?)
    nsresult rv;
    PRUint32 currentSegmentOffset;
    rv = seekable->Tell(&currentSegmentOffset);
    if (NS_FAILED(rv)) return rv;

    // Look for an existing entry keyed by aURISpec, adding one if needed.
    nsDocumentMapWriteEntry* entry =
        NS_STATIC_CAST(nsDocumentMapWriteEntry*,
                       PL_DHashTableOperate(&mDocumentMap, aURISpec,
                                            PL_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    // If there is a muxed document segment open, close it now by setting its
    // length, stored in the second PRUint32 of the segment.
    nsDocumentMapWriteEntry* previousEntry = mCurrentDocumentMapEntry;
    if (previousEntry) {
        NS_ASSERTION(previousEntry != entry, "redundant SelectMuxedDocument!");
        if (previousEntry == entry)
            return NS_OK;

        PRUint32 previousSegmentOffset = previousEntry->mCurrentSegmentOffset;
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            previousSegmentOffset + 4);
        if (NS_FAILED(rv)) return rv;

        // The length counts all bytes in the segment, including the header
        // that contains [nextSegmentOffset, length].
        rv = Write32(currentSegmentOffset - previousSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        // Seek back to the current offset only if we are not going to seek
        // back to *this* entry's last "current" segment offset and write its
        // next segment offset at the first PRUint32 of the segment.
        if (!entry->mURISpec) {
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                currentSegmentOffset);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // If this entry was newly added, set its key and initial segment offset.
    // Otherwise, seek back to write the next segment offset of the previous
    // segment for this document in the multiplex.
    if (!entry->mURISpec) {
        void *spec = nsMemory::Clone(aURISpec, strlen(aURISpec) + 1);
        if (!spec)
            return NS_ERROR_OUT_OF_MEMORY;

        entry->mURISpec = NS_REINTERPRET_CAST(const char*, spec);
        entry->mInitialSegmentOffset = currentSegmentOffset;
    } else {
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            entry->mCurrentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = Write32(currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;
    }

    // Update this document's current segment offset so we can later fix its
    // next segment offset (unless it is last, in which case we leave the zero
    // placeholder as a terminator).
    entry->mCurrentSegmentOffset = currentSegmentOffset;

    rv = Write32(0);    // nextSegmentOffset placeholder
    if (NS_FAILED(rv)) return rv;

    rv = Write32(0);    // length placeholder
    if (NS_FAILED(rv)) return rv;

    mCurrentDocumentMapEntry = entry;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteFooterPrefix(const nsFastLoadFooterPrefix& aFooterPrefix)
{
    nsresult rv;

    rv = Write32(aFooterPrefix.mNumIDs);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumSharpObjects);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumMuxedDocuments);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aFooterPrefix.mNumDependencies);
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
nsFastLoadFileWriter::WriteFastID(NSFastLoadID aID)
{
    return Write32(aID ^ MFL_ID_XOR_KEY);
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

nsresult
nsFastLoadFileWriter::WriteMuxedDocumentInfo(const nsFastLoadMuxedDocumentInfo& aInfo)
{
    nsresult rv;

    rv = WriteStringZ(aInfo.mURISpec);
    if (NS_FAILED(rv)) return rv;

    rv = Write32(aInfo.mInitialSegmentOffset);
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

PLDHashOperator PR_CALLBACK
nsFastLoadFileWriter::DocumentMapEnumerate(PLDHashTable *aTable,
                                           PLDHashEntryHdr *aHdr,
                                           PRUint32 aNumber,
                                           void *aData)
{
    nsFastLoadFileWriter* writer =
        NS_REINTERPRET_CAST(nsFastLoadFileWriter*, aTable->data);
    nsDocumentMapWriteEntry* entry =
        NS_STATIC_CAST(nsDocumentMapWriteEntry*, aHdr);
    nsresult* rvp = NS_REINTERPRET_CAST(nsresult*, aData);

    nsFastLoadMuxedDocumentInfo info;
    info.mURISpec = entry->mURISpec;
    info.mInitialSegmentOffset = entry->mInitialSegmentOffset;
    *rvp = writer->WriteMuxedDocumentInfo(info);

    return NS_FAILED(*rvp) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileWriter::WriteFooter()
{
    nsresult rv;
    PRUint32 i, count;

    nsFastLoadFooterPrefix footerPrefix;
    footerPrefix.mNumIDs = mIDMap.entryCount;
    footerPrefix.mNumSharpObjects = mObjectMap.entryCount;
    footerPrefix.mNumMuxedDocuments = mDocumentMap.entryCount;
    footerPrefix.mNumDependencies = mDependencies.Count();

    rv = WriteFooterPrefix(footerPrefix);
    if (NS_FAILED(rv)) return rv;

    // Enumerate mIDMap into a vector indexed by mFastID and write it.
    nsID* idvec = new nsID[footerPrefix.mNumIDs];
    if (!idvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mIDMap, IDMapEnumerate, idvec);
    NS_ASSERTION(count == footerPrefix.mNumIDs, "bad mIDMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSlowID(idvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] idvec;
    if (NS_FAILED(rv)) return rv;

    // Enumerate mObjectMap into a vector indexed by mOID and write it.
    nsFastLoadSharpObjectInfo* objvec =
        new nsFastLoadSharpObjectInfo[footerPrefix.mNumSharpObjects];
    if (!objvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mObjectMap, ObjectMapEnumerate, objvec);
    NS_ASSERTION(count == footerPrefix.mNumSharpObjects,
                 "bad mObjectMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSharpObjectInfo(objvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] objvec;
    if (NS_FAILED(rv)) return rv;

    // Enumerate mDocumentMap, writing nsFastLoadMuxedDocumentInfo records
    count = PL_DHashTableEnumerate(&mDocumentMap, DocumentMapEnumerate, &rv);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(count == footerPrefix.mNumMuxedDocuments,
                 "bad mDocumentMap enumeration!");

    // Write out make-like file dependencies.
    count = footerPrefix.mNumDependencies;
    for (i = 0; i < count; i++) {
        const char* s =
          NS_REINTERPRET_CAST(const char*, mDependencies.ElementAt(PRInt32(i)));
        rv = WriteStringZ(s);
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

    if (!PL_DHashTableInit(&mDocumentMap, &docmap_DHashTableOps, (void *)this,
                           sizeof(nsDocumentMapWriteEntry),
                           PL_DHASH_MIN_SIZE)) {
        PL_DHashTableFinish(&mIDMap);
        PL_DHashTableFinish(&mObjectMap);
        mIDMap.ops = mObjectMap.ops = mDocumentMap.ops = nsnull;
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
    header.mVersion = MFL_FILE_VERSION;

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mOutputStream));

    rv = seekable->Tell(&header.mFooterOffset);
    if (NS_FAILED(rv)) return rv;

    // If there is a muxed document segment open, close it now by setting its
    // length, stored in the second PRUint32 of the segment.
    if (mCurrentDocumentMapEntry) {
        PRUint32 currentSegmentOffset =
            mCurrentDocumentMapEntry->mCurrentSegmentOffset;
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            currentSegmentOffset + 4);
        if (NS_FAILED(rv)) return rv;

        rv = Write32(header.mFooterOffset - currentSegmentOffset);
        if (NS_FAILED(rv)) return rv;

        // Seek back to the current offset to write the footer.
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            header.mFooterOffset);
        if (NS_FAILED(rv)) return rv;

        mCurrentDocumentMapEntry = nsnull;
    }

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
            PRUint32 thisOffset;
            rv = Tell(&thisOffset);
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

    rv = Write32(oid ^ MFL_OID_XOR_KEY);
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

        rv = WriteFastID(fastCID);
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

    return WriteFastID(iid);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteID(const nsID& aID)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = MapID(aID, &fastID);
    if (NS_FAILED(rv)) return rv;

    return WriteFastID(fastID);
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
