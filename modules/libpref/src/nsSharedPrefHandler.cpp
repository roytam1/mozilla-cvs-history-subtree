/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSharedPrefHandler.h"
#include "nsPrefService.h"

#include "nsIServiceManagerUtils.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsReadableUtils.h"


#if defined(PR_LOGGING)
// set NSPR_LOG_MODULES=nsSharedPrefHandler:5
PRLogModuleInfo *gPrefsTransactionObserverLog = nsnull;
#define LOG(args) PR_LOG(PrefsTransactionObserver, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

nsSharedPrefHandler *gSharedPrefHandler = nsnull;

// Constants
#define kPrefsTSQueueName NS_LITERAL_CSTRING("prefs")

#define kExceptionListFileName NS_LITERAL_CSTRING("nonshared.txt")
const char kExceptionListCommentChar = '#';

const PRUint32 kCurrentPrefsTransactionDataVersion = 1;

// Static function prototypes
static PRBool PR_CALLBACK enumFind(void* aElement, void *aData);
static PRBool PR_CALLBACK enumFree(void* aElement, void *aData);
static PRInt32 ReadLine(FILE* inStm, nsACString& destString);


//****************************************************************************
// Transaction Format for kCurrentPrefsTransactionDataVersion
//****************************************************************************

/*
  NOTE: All string data is null terminated.
  The data length includes the null character.
  
  PRUint32 dataVersion
  PRUint32 action SET or CLEAR
  PRUint32 prefNameLen
  char prefName[prefNameLen]
  PRUint32 prefValueKind
  PRUint32 prefValueLen
  char prefValue[prefValueLen]
*/

//*****************************************************************************
// nsBufferInputStream
//*****************************************************************************   

class nsBufferInputStream
{
public:
                  nsBufferInputStream(const PRUint8* inBuffer, PRUint32 bufferSize) :
                    mBuf(inBuffer), mBufEnd(inBuffer + bufferSize),
                    mBufPtr(mBuf),
                    mError(NS_OK)
                  { }
                  
                  ~nsBufferInputStream()
                  { }

  nsresult 				GetError()
                  { return mError; }
  
  PRUint8					GetInt8();
  PRUint16				GetInt16();
  PRUint32				GetInt32();
  PRInt32					GetBytes(void* destBuffer, PRInt32 n);
  
  const PRUint8*	GetPtr()  // our buffer never changes
                  { return mBufPtr; }
  PRBool 					AdvancePtr(PRInt32 n);
      
private:
  const PRUint8 	*mBuf, *mBufEnd;
  const PRUint8 	*mBufPtr;
  nsresult 				mError;
};

PRUint8 nsBufferInputStream::GetInt8()
{
  if (mBufPtr < mBufEnd)
    return *mBufPtr++;
  mError = NS_ERROR_FAILURE;
  return 0;
}

PRUint16 nsBufferInputStream::GetInt16()
{
  if (mBufPtr + sizeof(PRUint16) <= mBufEnd) {
    PRUint16 tempInt = *(PRUint16 *)mBufPtr;
    mBufPtr += sizeof(PRUint16);
    return tempInt;
  }
  mError = NS_ERROR_FAILURE;
  return 0;
}

PRUint32 nsBufferInputStream::GetInt32()
{
  if (mBufPtr + sizeof(PRUint32) <= mBufEnd) {
    PRUint32 tempInt = *(PRUint32 *)mBufPtr;
    mBufPtr += sizeof(PRUint32);
    return tempInt;
  }
  mError = NS_ERROR_FAILURE;
  return 0;
}

PRInt32 nsBufferInputStream::GetBytes(void* destBuffer, PRInt32 n)
{
  if (mBufPtr + n <= mBufEnd) {
    memcpy(destBuffer, mBufPtr, n);
    mBufPtr += n;
    return n;
  }
  mError = NS_ERROR_FAILURE;
  return 0;
}

PRBool nsBufferInputStream::AdvancePtr(PRInt32 n)
{
  const PRUint8 *newPtr = mBufPtr + n;
  if (newPtr >= mBuf && newPtr <= mBufEnd) {
    mBufPtr = newPtr;
    return PR_TRUE;
  }
  mError = NS_ERROR_FAILURE;
  return PR_FALSE;
}

//*****************************************************************************
// nsBufferOutputStream
//*****************************************************************************   

class nsBufferOutputStream
{
public:
                  nsBufferOutputStream(PRUint32 initialCapacity) :
                    mBuf(nsnull),
                    mBufPtr(nsnull), mBufEnd(nsnull),
                    mCapacity(initialCapacity),
                    mError(NS_OK)
                  {
                  }
                  
                  ~nsBufferOutputStream();

  nsresult 				GetError()
                  { return mError; }
  
  void 						PutInt8(PRUint8 val);
  void 						PutInt16(PRUint16 val);
  void 						PutInt32(PRUint32 val);  
  PRUint32 				PutBytes(const void* src, PRUint32 n);
  
  PRUint8*				GetBuffer()
                  { return mBuf; }
  
  PRInt32 				GetSize()
                  { return mBufPtr - mBuf; }
  
private:
  PRBool          EnsureCapacity(PRInt32 sizeNeeded)
                  {
                    return (mBuf && ((mBufPtr + sizeNeeded) <= mBufEnd)) ?
                      PR_TRUE : GrowCapacity(sizeNeeded);
                  }
  PRBool					GrowCapacity(PRInt32 sizeNeeded);
  
private:
  PRUint8					*mBuf;
  PRUint8					*mBufPtr, *mBufEnd;
  PRInt32					mCapacity;
  nsresult				mError;
};

nsBufferOutputStream::~nsBufferOutputStream()
{
  if (mBuf)
    nsMemory::Free(mBuf);
}
  
void nsBufferOutputStream::PutInt8(PRUint8 val)
{
  if (EnsureCapacity(sizeof(PRUint8))) 
    *mBufPtr++ = val;
}
  
void nsBufferOutputStream::PutInt16(PRUint16 val)
{
  if (EnsureCapacity(sizeof(PRUint16))) {
    *(PRUint16 *)mBufPtr = val;
    mBufPtr += sizeof(PRUint16);
  }
}
  
void nsBufferOutputStream::PutInt32(PRUint32 val)
{
  if (EnsureCapacity(sizeof(PRUint32))) {
    *(PRUint32 *)mBufPtr = val;
    mBufPtr += sizeof(PRUint32);
  }
}
  
PRUint32 nsBufferOutputStream::PutBytes(const void* src, PRUint32 n)
{
  if (EnsureCapacity(n)) {
    memcpy(mBufPtr, src, n);
    mBufPtr += n;
    return n;
  }
  return 0;
}
    
PRBool nsBufferOutputStream::GrowCapacity(PRInt32 sizeNeeded)
{
  PRInt32 newCapacity = (mBufPtr - mBuf) + sizeNeeded;
  while (newCapacity > mCapacity)
    mCapacity <<= 1;
  
  PRInt32 curPos = mBufPtr - mBuf;
  mBuf = NS_STATIC_CAST(PRUint8*, nsMemory::Realloc(mBuf, mCapacity));
  if (!mBuf) {
    mError = NS_ERROR_OUT_OF_MEMORY;
    return PR_FALSE;
  }
  mBufPtr = mBuf + curPos;
  mBufEnd = mBufPtr + mCapacity;
  return PR_TRUE;
}


//*****************************************************************************
// nsSharedPrefHandler
//*****************************************************************************   

nsSharedPrefHandler::nsSharedPrefHandler() :
  mPrefService(nsnull), mPrefsTSQueueName("prefs"),
  mReadingUserPrefs(PR_FALSE),
  mProcessingTransaction(PR_FALSE)
{
#if defined(PR_LOGGING)
  if (!gPrefsTransactionObserverLog)
    gPrefsTransactionObserverLog = PR_NewLogModule("nsSharedPrefHandler");
#endif
}

nsSharedPrefHandler::~nsSharedPrefHandler()
{
  mExceptionList.EnumerateForwards(enumFree, nsnull);
}
        
nsresult nsSharedPrefHandler::OnSessionBegin()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);

  // Attach. When we receive the reply, we'll read the prefs.
  rv = mTransService->Attach(kPrefsTSQueueName, this);
  NS_ASSERTION(NS_SUCCEEDED(rv), "tmITransactionService::Attach() failed");
  
  return rv;
}

nsresult nsSharedPrefHandler::OnSessionEnd()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mTransService->Detach(kPrefsTSQueueName);
  NS_ASSERTION(NS_SUCCEEDED(rv), "tmITransactionService::Detach() failed");

  return rv;
}

nsresult nsSharedPrefHandler::OnSavePrefs()
{
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);
  
  // When the reply to this comes (synchronously), we'll actually
  // write out our data. The transaction service holds a lock on
  // our data file during our reply.
  rv = mTransService->Flush(kPrefsTSQueueName);
  NS_ASSERTION(NS_SUCCEEDED(rv), "tmITransactionService::Flush() failed");

  return NS_OK;
}
    
nsresult nsSharedPrefHandler::OnPrefChanged(PrefAction action, PrefHashEntry* pref)
{
  if (action != PREF_SETUSER)
    return NS_OK;
  if (!IsPrefShared(pref->key))
    return NS_OK;
  if (mReadingUserPrefs || mProcessingTransaction)
    return NS_OK;
    
  nsresult rv = EnsureTransactionService();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 valueLen, prefNameLen = strlen(pref->key);
  
  nsBufferOutputStream outStm(256);
  outStm.PutInt32(kCurrentPrefsTransactionDataVersion);
  outStm.PutInt32(action);
  outStm.PutInt32(prefNameLen + 1);
  outStm.PutBytes(pref->key, prefNameLen + 1);

  switch (pref->flags & PREF_VALUETYPE_MASK) {
    case PREF_STRING:
      outStm.PutInt32(PREF_STRING);
      valueLen = strlen(pref->userPref.stringVal) + 1;
      outStm.PutInt32(valueLen);
      outStm.PutBytes(pref->userPref.stringVal, valueLen);
      break;
    case PREF_INT:
      outStm.PutInt32(PREF_INT);
      outStm.PutInt32(sizeof(PRInt32));
      outStm.PutInt32(pref->userPref.intVal);
      break;
    case PREF_BOOL:
      outStm.PutInt32(PREF_BOOL);
      outStm.PutInt32(sizeof(PRInt32));
      outStm.PutInt32(pref->userPref.boolVal);
      break;
    default:
      return NS_ERROR_UNEXPECTED;
  }
  
  rv = outStm.GetError();
  NS_ASSERTION(NS_SUCCEEDED(rv), "OnPrefChanged: outStm failed");
  if (NS_SUCCEEDED(rv)) {
    rv = mTransService->PostTransaction(kPrefsTSQueueName, outStm.GetBuffer(), outStm.GetSize());
    NS_ASSERTION(NS_SUCCEEDED(rv), "tmITransactionService::PostTransaction() failed");
  }
  return rv;
}

PRBool nsSharedPrefHandler::IsPrefShared(const char* prefName)
{
  if (!mExceptionList.Count()) // quick check for empty list
    return PR_TRUE;
    
  // returns PR_TRUE if we reached the end without finding it.
  return mExceptionList.EnumerateForwards(enumFind, NS_CONST_CAST(char*, prefName));
}

//*****************************************************************************
// nsSharedPrefHandler Protected Methods
//*****************************************************************************   

nsresult nsSharedPrefHandler::Init(nsPrefService* aOwner)
{
  NS_ENSURE_ARG(aOwner);
  mPrefService = aOwner;
  (void)ReadExceptionFile(); 
  
  return NS_OK;
}

nsresult nsSharedPrefHandler::ReadExceptionFile()
{
  nsresult rv;
  
  nsCOMPtr<nsIProperties> directoryService =
      do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsILocalFile> exceptionFile;
    rv = directoryService->Get(NS_APP_PREF_DEFAULTS_50_DIR, NS_GET_IID(nsILocalFile),
                              getter_AddRefs(exceptionFile));
    if (NS_SUCCEEDED(rv)) {
      rv = exceptionFile->AppendNative(kExceptionListFileName);
      if (NS_SUCCEEDED(rv)) {
        FILE *inStm;
        rv = exceptionFile->OpenANSIFileDesc("r", &inStm);
        if (NS_SUCCEEDED(rv)) {
          nsCAutoString lineStr;
          while (ReadLine(inStm, lineStr) != EOF) {
            lineStr.CompressWhitespace();
            if (lineStr.IsEmpty() || lineStr.CharAt(0) == kExceptionListCommentChar)
              continue;
              
            char *rawStr = ToNewCString(lineStr);
            if (!rawStr) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }
            mExceptionList.AppendElement(rawStr);
          }
          fclose(inStm);
        }
      }
    }
  }
  return rv;
}

nsresult nsSharedPrefHandler::EnsureTransactionService()
{
  if (mTransService)
    return NS_OK;
  nsresult rv;
  mTransService = do_GetService("@mozilla.org/transaction/service;1", &rv);
  return rv;
}

//*****************************************************************************
// nsSharedPrefHandler::nsISupports
//*****************************************************************************   

NS_IMPL_ISUPPORTS1(nsSharedPrefHandler, tmITransactionObserver)

//*****************************************************************************
// nsSharedPrefHandler::tmITransactionObserver
//*****************************************************************************   

NS_IMETHODIMP nsSharedPrefHandler::OnTransactionAvailable(PRUint32 aQueueID, const PRUint8 *aData, PRUint32 aDataLen)
{
    LOG(("nsSharedPrefHandler::OnTransactionAvailable [%s]\n", aData));

    nsBufferInputStream inStm(aData, aDataLen);

    PRUint32 dataVersion, prefAction, dataLen, prefKind, tempInt32;
    const char *stringStart;
    
    dataVersion = inStm.GetInt32();
    NS_ENSURE_TRUE(dataVersion == kCurrentPrefsTransactionDataVersion, NS_ERROR_INVALID_ARG);
    prefAction = inStm.GetInt32();
    dataLen = inStm.GetInt32(); // includes terminating null
    stringStart = (const char *)inStm.GetPtr();
    nsDependentCString prefNameStr(stringStart);
    inStm.AdvancePtr(dataLen);
    prefKind = inStm.GetInt32();
    dataLen = inStm.GetInt32();
    
    mProcessingTransaction = PR_TRUE; // Don't generate transactions for these
    switch (prefKind) {
      case PREF_STRING:
        {
        stringStart = (const char *)inStm.GetPtr();
        nsDependentCString prefStrValueStr(stringStart);
        inStm.AdvancePtr(dataLen);
        NS_ENSURE_SUCCESS(inStm.GetError(), inStm.GetError());
        PREF_SetCharPref(prefNameStr.get(), prefStrValueStr.get());
        }
        break;
      case PREF_INT:
        tempInt32 = inStm.GetInt32();
        NS_ENSURE_SUCCESS(inStm.GetError(), inStm.GetError());
        PREF_SetIntPref(prefNameStr.get(), tempInt32);
        break;
      case PREF_BOOL:
        tempInt32 = inStm.GetInt32();
        NS_ENSURE_SUCCESS(inStm.GetError(), inStm.GetError());
        PREF_SetBoolPref(prefNameStr.get(), tempInt32);
        break;
    }
    mProcessingTransaction = PR_FALSE;
    
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnAttachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("nsSharedPrefHandler::OnAttachReply [%d]\n", aStatus));

    // the transaction service holds a lock on the file during this call.
    mPrefService->ResetUserPrefs();
    mPrefService->ReadUserPrefs(nsnull);
    
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnDetachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("tmModuleTest: nsSharedPrefHandler::OnDetachReply [%d]\n", aStatus));
    return NS_OK;
}

NS_IMETHODIMP nsSharedPrefHandler::OnFlushReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    LOG(("tmModuleTest: nsSharedPrefHandler::OnFlushReply [%d]\n", aStatus));
    
    // Call the internal method to write immediately
    mPrefService->SavePrefFileInternal(nsnull);
    return NS_OK;
}

//*****************************************************************************
// Static functions
//*****************************************************************************   

static PRBool PR_CALLBACK enumFind(void* aElement, void *aData)
{
  char *elemStr = NS_STATIC_CAST(char*, aElement);
  char *searchStr = NS_STATIC_CAST(char*, aData);
  // return PR_FALSE for a match and to stop search
  return (strncmp(elemStr, searchStr, strlen(elemStr)) != 0);
}

static PRBool PR_CALLBACK enumFree(void* aElement, void *aData)
{
  if (aElement)
    nsMemory::Free(aElement);
  return PR_TRUE;
}

static PRInt32 ReadLine(FILE* inStm, nsACString& destString)
{
  char stackBuf[512];
  PRUint32 charsInBuf = 0;
  destString.Truncate();
  int c;
  
  while (1) {
    c = getc(inStm);
    if (c == EOF)
      break;
    else if (c == '\r') {
      c = getc(inStm);
      if (c != '\n')
        ungetc(c, inStm);
      break;
    }
    else if (c == '\n')
      break;
    else {
      if (charsInBuf >= sizeof(stackBuf)) {
        destString.Append(stackBuf, charsInBuf);
        charsInBuf = 0;
      }
      stackBuf[charsInBuf++] = c;
    }
  }
  if (charsInBuf)
    destString.Append(stackBuf, charsInBuf);
  return (c == EOF && destString.IsEmpty()) ? EOF : 1;
}

//*****************************************************************************
// NS_CreateSharedPrefHandler
//*****************************************************************************   

nsresult NS_CreateSharedPrefHandler(nsPrefService *aOwner)
{
  nsSharedPrefHandler *local = new nsSharedPrefHandler;
  if (!local)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = local->Init(aOwner);
  if (NS_FAILED(rv)) {
    delete local;
    return rv;
  }
  NS_ADDREF(gSharedPrefHandler = local);
  return NS_OK;
}

