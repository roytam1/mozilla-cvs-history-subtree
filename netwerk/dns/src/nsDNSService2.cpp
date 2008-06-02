/* vim:set ts=4 sw=4 sts=4 et cin: */
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
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   IBM Corp.
 *   8x8, Inc.
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

#include <stdlib.h> // for rand()
#include "nsDNSService2.h"
#include "nsIDNSRecord.h"
#include "nsIDNSListener.h"
#include "nsIDNSIterListener.h"
#include "nsIDNSNAPTRRecord.h"
#include "nsIDNSSRVRecord.h"
#include "nsISimpleEnumerator.h"
#include "nsICancelable.h"
#include "nsIProxyObjectManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsNetCID.h"
#include "nsNetError.h"
#include "prsystem.h"
#include "prnetdb.h"
#include "prmon.h"
#include "prio.h"
#include "plstr.h"
#include "prrng.h"

#if defined(XP_WIN32)
#include <WinDef.h>
#include <Windns.h>
#elif defined (XP_MACOSX)
#include <arpa/nameser_compat.h>
#include <resolv.h>
#else
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // For solaris
#include <arpa/nameser.h>
#include <resolv.h>
#endif

static const char kPrefDnsCacheEntries[]    = "network.dnsCacheEntries";
static const char kPrefDnsCacheExpiration[] = "network.dnsCacheExpiration";
static const char kPrefEnableIDN[]          = "network.enableIDN";
static const char kPrefIPv4OnlyDomains[]    = "network.dns.ipv4OnlyDomains";
static const char kPrefDisableIPv6[]        = "network.dns.disableIPv6";

//-----------------------------------------------------------------------------

struct nsSrvRecord
{
    char     host[256];
    PRUint16 port;
    PRUint16 priority;
    PRUint16 weight;
    float running_sum;
    
    void swap(nsSrvRecord *other) {
        nsSrvRecord tmp = *this;
        *this = *other;
        *other = tmp; 
    }
};

class nsDNSSRVRecord : public nsIDNSSRVRecord
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSSRVRECORD

    nsDNSSRVRecord(nsSrvRecord *record)
        : mRecord(record) {}

protected:
    virtual ~nsDNSSRVRecord() {}

private:
    nsSrvRecord *mRecord;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSSRVRecord, nsIDNSSRVRecord)

NS_IMETHODIMP
nsDNSSRVRecord::GetPriority(PRUint16 *aPriority)
{
    NS_ENSURE_ARG(aPriority);
    *aPriority = mRecord->priority;
    return NS_OK;
}

NS_IMETHODIMP
nsDNSSRVRecord::GetWeight(PRUint16 *aWeight)
{
    NS_ENSURE_ARG(aWeight);
    *aWeight = mRecord->weight;
    return NS_OK;
}

NS_IMETHODIMP 
nsDNSSRVRecord::GetHost(nsACString & aHost)
{
    aHost = mRecord->host;      
    return NS_OK;
}

NS_IMETHODIMP
nsDNSSRVRecord::GetPort(PRInt32 *aPort)
{
    NS_ENSURE_ARG(aPort);
    *aPort = mRecord->port;
    return NS_OK;
}

PR_STATIC_CALLBACK(PRBool) 
SrvRecordsEnumFunc(void* aElement, void* aData)
{
    nsSrvRecord* rec = static_cast<nsSrvRecord*>(aElement);
    delete rec;
    return PR_TRUE;
}

class nsDNSSRVIter : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    nsDNSSRVIter(nsVoidArray *srvRecords)
        : mSrvRecords(srvRecords)
        , mPos(-1) 
    {
    }

protected:
    virtual ~nsDNSSRVIter() {
      if (mSrvRecords) {
          mSrvRecords->EnumerateForwards(SrvRecordsEnumFunc, nsnull);
          delete mSrvRecords;
      }
    }

private:

    nsVoidArray *mSrvRecords;
    PRInt16     mPos;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSSRVIter, nsISimpleEnumerator)

NS_IMETHODIMP
nsDNSSRVIter::GetNext(nsISupports **_retval)
{
    NS_ENSURE_ARG(_retval);
    
    if (++mPos > mSrvRecords->Count() - 1)
        return NS_ERROR_NOT_AVAILABLE;
        
    nsSrvRecord* record = 
      static_cast<nsSrvRecord*>(mSrvRecords->ElementAt(mPos));
        
    nsCOMPtr<nsIDNSSRVRecord> rec = new nsDNSSRVRecord(record);
    if (!rec)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*_retval = rec);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSSRVIter::HasMoreElements(PRBool *result)
{
    NS_ENSURE_ARG(result);
    
    *result = mPos + 1 < mSrvRecords->Count();
    return NS_OK;
}

static int 
CompareSRV( const void *a, const void *b, void *o)
{
    nsSrvRecord * aa, * bb;

    if ( !a )
        return 1;
    if ( !b )
        return -1;

    aa  = (nsSrvRecord *) a;
    bb = (nsSrvRecord *) b;

    if ( aa->priority > bb->priority )
        return 1;
    if ( aa->priority < bb->priority )
        return -1;

    if ( aa->weight == 0 )
        return 1;
        
    if ( bb->weight == 0 )
        return -1;

    return 0;
}

static nsresult
ResolveSRV(const nsACString &host, PRUint32 aFlags, nsISimpleEnumerator **result)
{
    nsresult rv = NS_OK;
    nsVoidArray *sorted = new nsVoidArray;
    if (!sorted)
        return NS_ERROR_OUT_OF_MEMORY;  
    
#ifdef XP_WIN32                 
    DNS_RECORD *results = NULL;
    if (DnsQuery_A(PromiseFlatCString(host).get(),
                   DNS_TYPE_SRV,
                   DNS_QUERY_STANDARD,
                   NULL, 
                   &results, 
                   NULL) == 0) { // NOERROR

      for (PDNS_RECORD record = results; record != NULL; record = record->pNext) {
        if (record->wType == DNS_TYPE_SRV) {  
          nsSrvRecord *sr = new nsSrvRecord;
          if (!sr) {
              sorted->EnumerateForwards(SrvRecordsEnumFunc, nsnull);
              delete sorted;
              return NS_ERROR_OUT_OF_MEMORY;
          }
          strncpy(sr->host, (char*)record->Data.Srv.pNameTarget, 
                  sizeof(sr->host)-1);
          sr->host[sizeof(sr->host)-1] = '\0'; 
          sr->port = record->Data.Srv.wPort;
          sr->priority = record->Data.Srv.wPriority;
          sr->weight = record->Data.Srv.wWeight;
          sorted->AppendElement(sr);
        }    
      }

    }
    DnsRecordListFree(results, DnsFreeRecordList);
#else
    unsigned char buffer[NS_PACKETSZ];
    PRInt32 size;
    PRUint16 qdcount, ancount;
    unsigned char *ptr;
    unsigned char *end;
    PRUint32 type; 
    char name[256];

    size = res_query(PromiseFlatCString(host).get(), ns_c_in, ns_t_srv, 
                     buffer, sizeof(buffer));
    if (size < 0) {
        delete sorted;
        return NS_ERROR_FAILURE;
    }
    
    // parse response
    // skip the id and header bits
    ptr = buffer + 4;

    NS_GET16(qdcount, ptr);
    NS_GET16(ancount, ptr);

    ptr = buffer + sizeof(HEADER);
    end = buffer + size; 
    
    // skip question entries
    while (qdcount-- > 0 && ptr < end) {
        size = dn_expand(buffer, end, ptr, name, sizeof(name));
        if (size < 0) {
            NS_WARNING("Failed parsing srv rr name");
            break;
        }
        ptr += size + QFIXEDSZ;
    }

    // parse answer entries
    while (ancount-- > 0 && ptr < end) {
        size = dn_expand(buffer, end, ptr, name, sizeof(name));
        if (size < 0) {
            NS_WARNING("Failed parsing srv rr name");
            break;
        }
        ptr += size;

        NS_GET16(type, ptr);
        // skip rr_class, ttl and rdlength
        ptr += 8; 
    
        if (type == ns_t_srv) {
            nsSrvRecord *sr = new nsSrvRecord;
            if (!sr) {
                sorted->EnumerateForwards(SrvRecordsEnumFunc, nsnull);
                delete sorted;
                return NS_ERROR_OUT_OF_MEMORY;
            }
            NS_GET16(sr->priority, ptr);
            NS_GET16(sr->weight, ptr);
            NS_GET16(sr->port, ptr); 
            size = dn_expand(buffer, end, ptr, name, sizeof(name)); 
            if (size < 0) {
                delete sr;
                sorted->EnumerateForwards(SrvRecordsEnumFunc, nsnull);
                delete sorted;
                NS_ERROR("Failed to parse srv rr name");
                return NS_ERROR_FAILURE;
            }
            strncpy(sr->host, name, sizeof(sr->host)-1);
            sr->host[sizeof(sr->host)-1] = '\0';
            
            sorted->AppendElement(sr);
            ptr += size;
        }
    }
#endif
    
    // sort by priority
    sorted->Sort(CompareSRV, NULL);
    
    // RFC 2782 weighting algorithm
    PRUint32 seed;
    PR_GetRandomNoise(&seed, sizeof(seed));
    srand(seed);
    
    for (PRInt32 ii=0; ii<sorted->Count(); ++ii) {
        float sum = 0;
          
        for (PRInt32 jj=ii; jj<sorted->Count(); ++jj) {
          nsSrvRecord* outer = static_cast<nsSrvRecord*>(sorted->ElementAt(ii));
          nsSrvRecord* inner = static_cast<nsSrvRecord*>(sorted->ElementAt(jj));
          
          if (outer->priority != inner->priority)
            break;
            
          sum += inner->weight;
          inner->running_sum = sum; 
        }
        
        float random = sum * rand()/RAND_MAX;
        
        for (PRInt32 jj=ii; jj<sorted->Count(); ++jj) {
          nsSrvRecord* outer = static_cast<nsSrvRecord*>(sorted->ElementAt(ii));
          nsSrvRecord* inner = static_cast<nsSrvRecord*>(sorted->ElementAt(jj));
          
          if (outer->priority != inner->priority)
            break;
                  
          if (inner->running_sum >= random) {
            // swap
            if (jj != ii)
              inner->swap(outer);
            
            break;
          }
        }
    }
    
    nsDNSSRVIter *iter = new nsDNSSRVIter(sorted);
    if (!iter) {
        sorted->EnumerateForwards(SrvRecordsEnumFunc, nsnull);
        delete sorted;
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        NS_ADDREF(*result = iter);
    }
    
    return rv;
}

//-----------------------------------------------------------------------------

struct nsNaptrRecord
{
    char     replacement[256];
    char     regexp[256];
    char     service[256];
    char     flags[256];
    PRUint16 order;
    PRUint16 preference;
};

class nsDNSNAPTRRecord : public nsIDNSNAPTRRecord
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSNAPTRRECORD

    nsDNSNAPTRRecord(nsNaptrRecord *record)
        : mRecord(record) {}

protected:
    virtual ~nsDNSNAPTRRecord() {}

private:
    nsNaptrRecord *mRecord;
};
    
NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSNAPTRRecord, nsIDNSNAPTRRecord)

NS_IMETHODIMP
nsDNSNAPTRRecord::GetFlag(nsACString &flag)
{
    flag.Assign(mRecord->flags);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSNAPTRRecord::GetService(nsACString &service)
{
    service.Assign(mRecord->service);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSNAPTRRecord::GetRegexp(nsACString &regexp)
{
    regexp.Assign(mRecord->regexp);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSNAPTRRecord::GetReplacement(nsACString &replacement)
{
    replacement.Assign(mRecord->replacement);
    return NS_OK;
}

PR_STATIC_CALLBACK(PRBool) 
NaptrRecordsEnumFunc(void* aElement, void* aData)
{
    nsNaptrRecord* rec = static_cast<nsNaptrRecord*>(aElement);
    delete rec;
    return PR_TRUE;
}
   
class nsDNSNAPTRIter : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    nsDNSNAPTRIter(nsVoidArray *naptrRecords)
        : mNaptrRecords(naptrRecords), mPos(-1) 
    {   
    }

protected:
    virtual ~nsDNSNAPTRIter() {
      if (mNaptrRecords) {
          mNaptrRecords->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
          delete mNaptrRecords;
      }
    }

private:
    
    nsVoidArray *mNaptrRecords;
    PRInt16      mPos;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSNAPTRIter, nsISimpleEnumerator)

NS_IMETHODIMP
nsDNSNAPTRIter::GetNext(nsISupports **_retval)
{
    NS_ENSURE_ARG(_retval);
    
    if (++mPos > mNaptrRecords->Count() - 1)
        return NS_ERROR_NOT_AVAILABLE;

    nsNaptrRecord* record = 
      static_cast<nsNaptrRecord*>(mNaptrRecords->ElementAt(mPos));
        
    nsCOMPtr<nsIDNSNAPTRRecord> rec = new nsDNSNAPTRRecord(record);
    if (!rec)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*_retval = rec);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSNAPTRIter::HasMoreElements(PRBool *result)
{
    NS_ENSURE_ARG(result);
    
    *result = mPos + 1 < mNaptrRecords->Count();
    return NS_OK;
}

static int 
CompareNAPTR( const void *a, const void *b, void *o)
{
    nsNaptrRecord * aa, * bb;

    if ( !a )
        return 1;
    if ( !b )
        return -1;

    aa  = (nsNaptrRecord *) a;
    bb = (nsNaptrRecord *) b;

    if ( aa->order > bb->order )
        return -1;
    if ( aa->order < bb->order )
        return 1;
        
    if ( aa->preference > bb->preference )
        return -1;
    if ( aa->preference < bb->preference )
        return 1;
        
    return 0;
}

static nsresult
ResolveNAPTR(const nsACString &domain, PRUint32 aFlags, nsISimpleEnumerator **result)
{
    NS_ENSURE_ARG(result);
    nsresult rv = NS_OK;
    *result = nsnull;
    nsVoidArray* sorted = new nsVoidArray;
    if (!sorted)
        return NS_ERROR_OUT_OF_MEMORY;

#ifdef XP_WIN32
    DNS_RECORD *results = NULL;
    if (DnsQuery_A(PromiseFlatCString(domain).get(),
                   DNS_TYPE_NAPTR,
                   DNS_QUERY_STANDARD,
                   NULL, 
                   &results, 
                   NULL) == 0) { // NOERROR

      for (PDNS_RECORD record = results; record != NULL; record = record->pNext) {
        if (record->wType == DNS_TYPE_NAPTR) {  
          nsNaptrRecord *sr = new nsNaptrRecord;
          if (!sr) {
              sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
              delete sorted;
              return NS_ERROR_OUT_OF_MEMORY;
          }

          int dataLength = record->wDataLength;

          sr->order = record->Data.NAPTR.wOrder;
          sr->preference = record->Data.NAPTR.wPreference;

          dataLength -= 4; // order + preference
          
          // decode the NAPTR fields from the raw record data
          char *data = (char*)&record->Data.NAPTR.pFlags;
          PRUint32 n, srcIndex = 0;

          if (srcIndex >= dataLength || 
              ((n = data[srcIndex++]) + srcIndex) >= dataLength) {
              sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);  
              delete sorted;
              delete sr;
              NS_ERROR("Failed to parse naptr flags");
              return NS_ERROR_FAILURE;
          }
          memcpy(sr->flags, data+srcIndex, n);
          sr->flags[n] = '\0';
          srcIndex += n;

          if (srcIndex >= dataLength || 
              ((n = data[srcIndex++]) + srcIndex) >= dataLength) {
              sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);  
              delete sorted;
              delete sr;
              NS_ERROR("Failed to parse naptr service");
              return NS_ERROR_FAILURE;
          }
          memcpy(sr->service, data+srcIndex, n);
          sr->service[n] = '\0';
          srcIndex += n;

          if (srcIndex >= dataLength || 
              ((n = data[srcIndex++]) + srcIndex) >= dataLength) {
              sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
              delete sorted;
              delete sr;
              NS_ERROR("Failed to parse naptr regexp");
              return NS_ERROR_FAILURE;
          }
          memcpy(sr->regexp, data+srcIndex, n);
          sr->regexp[n] = '\0';
          srcIndex += n;

          // expand the domain-name field
          int dstIndex = 0;
          while ((n = data[srcIndex++])) {
              if (n + srcIndex >= dataLength || 
                  dstIndex >= sizeof(sr->replacement)-1) {
                  sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                  delete sorted;
                  delete sr;
                  NS_ERROR("Failed to parse naptr replacement");
                  return NS_ERROR_FAILURE;
              }

              memcpy(sr->replacement+dstIndex, data+srcIndex, n);
              srcIndex  += n;
              dstIndex += n;
              if (data[srcIndex])
                  sr->replacement[dstIndex++] = '.';
              else
                  sr->replacement[dstIndex++] = '\0';
          }

          sorted->AppendElement(sr);
        }    
      }

    }
    DnsRecordListFree(results, DnsFreeRecordList);
#else
    unsigned char buffer[NS_PACKETSZ];
    PRInt32 size;
    PRUint16 qdcount, ancount;
    unsigned char *ptr;
    unsigned char *end;
    PRUint32 type; 
    char name[256];

    size = res_query(PromiseFlatCString(domain).get(), ns_c_in, ns_t_naptr,
                     buffer, sizeof(buffer));
    if (size < 0) {
        delete sorted;
        return NS_ERROR_FAILURE;
    }
    
    // parse response
    // skip the id and header bits   
    ptr = buffer + 4;

    NS_GET16(qdcount, ptr);
    NS_GET16(ancount, ptr);

    ptr = buffer + sizeof(HEADER);
    end = buffer + size; 
    
    // skip question entries
    while (qdcount-- > 0 && ptr < end) {
        size = dn_expand(buffer, end, ptr, name, sizeof(name));
        if (size < 0) {
            NS_WARNING("Failed parsing srv rr name");
            break;
        }
        ptr += size + QFIXEDSZ;
    }

    // parse answer entries
    while (ancount-- > 0 && ptr < end) {
        size = dn_expand(buffer, end, ptr, name, sizeof(name));
        if (size < 0) {
            NS_WARNING("Failed parsing naptr rr name");
            break;
        }
        ptr += size;

        NS_GET16(type, ptr);
        // skip rr_class, ttl and rdlength
        ptr += 8; 
       
        if (type == ns_t_naptr) {
            nsNaptrRecord *sr = new nsNaptrRecord;
            if (!sr) {
                sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                delete sorted;
                return NS_ERROR_OUT_OF_MEMORY;
            }
            
            NS_GET16(sr->order, ptr);
            NS_GET16(sr->preference, ptr);
            PRUint32 n;
            
            // get flags
            if (ptr >= end || ((n = *ptr) + ptr) >= end) {
                sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                delete sorted;
                delete sr;
                NS_ERROR("Failed to parse naptr flags");
                return NS_ERROR_FAILURE;
            }

            ptr++;
            if (n < sizeof(sr->regexp) - 1) {
              strncpy(sr->flags, (const char*)ptr, n);
              sr->flags[n] = '\0';
            }
            ptr += n;
            
            // get service
            if (ptr >= end || ((n = *ptr) + ptr) >= end) {
                sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                delete sorted;
                delete sr;
                NS_ERROR("Failed to parse naptr service");
                return NS_ERROR_FAILURE;
            }
            
            ptr++;
            if (n < sizeof(sr->service) - 1) {
              strncpy(sr->service, (const char*)ptr, n);
              sr->service[n] = '\0';
            }
            ptr += n;
          
            // get regexp
            if (ptr >= end || ((n = *ptr) + ptr) >= end) {
                sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                delete sorted;
                delete sr;
                NS_ERROR("Failed to parse naptr regexp");
                return NS_ERROR_FAILURE;
            }

            ptr++;
            if (n < sizeof(sr->regexp) - 1) {
              strncpy(sr->regexp, (const char*)ptr, n);
              sr->regexp[n] = '\0';
            }
            ptr += n;
            
            // get replacement
            size = dn_expand(buffer, end, ptr, name, sizeof(name));
            if (size < 0) {
                sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
                delete sorted;
                delete sr;
                NS_ERROR("Failed to parse naptr replacement");
                return NS_ERROR_FAILURE;
            }
            strncpy(sr->replacement, name, sizeof(sr->replacement)-1);
            sr->replacement[sizeof(sr->replacement)-1] = '\0';
            
            sorted->AppendElement(sr);
            ptr += size;
        }
    }
#endif
    
    sorted->Sort(CompareNAPTR, NULL);
    
    nsDNSNAPTRIter *iter = new nsDNSNAPTRIter(sorted);
    if (!iter) {
        sorted->EnumerateForwards(NaptrRecordsEnumFunc, nsnull);
        delete sorted;
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        NS_ADDREF(*result = iter);
    }
    
    return rv;
}

class nsDNSRecordIter : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    nsDNSRecordIter(nsIDNSRecord *record)
        : mRecord(record) {}

protected:
    virtual ~nsDNSRecordIter() {}

private:
    
    nsCOMPtr<nsIDNSRecord> mRecord;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSRecordIter, nsISimpleEnumerator)

NS_IMETHODIMP
nsDNSRecordIter::GetNext(nsISupports **_retval)
{
    NS_ENSURE_ARG(_retval);

    if (!mRecord)
        return NS_ERROR_NOT_AVAILABLE;
    
    NS_ADDREF(*_retval = mRecord);
    mRecord = nsnull;
    
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRecordIter::HasMoreElements(PRBool *result)
{
    NS_ENSURE_ARG(result);
    
    *result = (mRecord != nsnull);
    return NS_OK;
}
  
//-----------------------------------------------------------------------------

class nsDNSTypeAsyncRequest : public nsICancelable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICANCELABLE

    nsDNSTypeAsyncRequest(nsIDNSIterListener   *listener)
        : mListener(listener), mLock(nsnull)
    {
    }
    ~nsDNSTypeAsyncRequest() 
    {
        if (mLock)
            PR_DestroyLock(mLock);
    }

    nsresult Init();
    void OnLookupComplete(nsISimpleEnumerator *, nsresult);

    nsCOMPtr<nsIDNSIterListener> mListener;
    PRLock *mLock;
};

nsresult
nsDNSTypeAsyncRequest::Init()
{
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK; 
}

void
nsDNSTypeAsyncRequest::OnLookupComplete(nsISimpleEnumerator *iter, nsresult status)
{
    nsAutoLock lock(mLock);
            
    // make sure the request wasn't cancelled
    if (mListener) {
        mListener->OnLookupComplete(this, iter, status);
        mListener = nsnull;
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSTypeAsyncRequest, nsICancelable)

NS_IMETHODIMP
nsDNSTypeAsyncRequest::Cancel(nsresult reason)
{
    NS_ENSURE_ARG(NS_FAILED(reason));
    {
        nsAutoLock lock(mLock);
        mListener = nsnull;
    }
    return NS_OK;
}

//-----------------------------------------------------------------------------

class nsDNSRecord : public nsIDNSRecord
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSRECORD

    nsDNSRecord(nsHostRecord *hostRecord)
        : mHostRecord(hostRecord)
        , mIter(nsnull)
        , mIterGenCnt(-1)
        , mDone(PR_FALSE) {}

protected:
    virtual ~nsDNSRecord() {}

private:

    nsRefPtr<nsHostRecord>  mHostRecord;
    void                   *mIter;
    int                     mIterGenCnt; // the generation count of
                                         // mHostRecord->addr_info when we
                                         // start iterating
    PRBool                  mDone;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSRecord, nsIDNSRecord)

NS_IMETHODIMP
nsDNSRecord::GetCanonicalName(nsACString &result)
{
    // this method should only be called if we have a CNAME
    NS_ENSURE_TRUE(mHostRecord->flags & nsHostResolver::RES_CANON_NAME,
                   NS_ERROR_NOT_AVAILABLE);

    // if the record is for an IP address literal, then the canonical
    // host name is the IP address literal.
    const char *cname;
    PR_Lock(mHostRecord->addr_info_lock);
    if (mHostRecord->addr_info)
        cname = PR_GetCanonNameFromAddrInfo(mHostRecord->addr_info);
    else
        cname = mHostRecord->host;
    result.Assign(cname);
    PR_Unlock(mHostRecord->addr_info_lock);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRecord::GetNextAddr(PRUint16 port, PRNetAddr *addr)
{
    // not a programming error to poke the DNS record when it has no more
    // entries.  just fail without any debug warnings.  this enables consumers
    // to enumerate the DNS record without calling HasMore.
    if (mDone)
        return NS_ERROR_NOT_AVAILABLE;

    PR_Lock(mHostRecord->addr_info_lock);
    if (mHostRecord->addr_info) {
        if (!mIter)
            mIterGenCnt = mHostRecord->addr_info_gencnt;
        else if (mIterGenCnt != mHostRecord->addr_info_gencnt) {
            // mHostRecord->addr_info has changed, so mIter is invalid.
            // Restart the iteration.  Alternatively, we could just fail.
            mIter = nsnull;
            mIterGenCnt = mHostRecord->addr_info_gencnt;
        }
        mIter = PR_EnumerateAddrInfo(mIter, mHostRecord->addr_info, port, addr);
        PR_Unlock(mHostRecord->addr_info_lock);
        if (!mIter) {
            mDone = PR_TRUE;
            return NS_ERROR_NOT_AVAILABLE;
        }
    }
    else {
        PR_Unlock(mHostRecord->addr_info_lock);
        if (!mHostRecord->addr) {
            // Both mHostRecord->addr_info and mHostRecord->addr are null.
            // This can happen if mHostRecord->addr_info expired and the
            // attempt to reresolve it failed.
            return NS_ERROR_NOT_AVAILABLE;
        }
        memcpy(addr, mHostRecord->addr, sizeof(PRNetAddr));
        // set given port
        port = PR_htons(port);
        if (addr->raw.family == PR_AF_INET)
            addr->inet.port = port;
        else
            addr->ipv6.port = port;
        mDone = PR_TRUE; // no iterations
    }
        
    return NS_OK; 
}

NS_IMETHODIMP
nsDNSRecord::GetNextAddrAsString(nsACString &result)
{
    PRNetAddr addr;
    nsresult rv = GetNextAddr(0, &addr);
    if (NS_FAILED(rv)) return rv;

    char buf[64];
    if (PR_NetAddrToString(&addr, buf, sizeof(buf)) == PR_SUCCESS) {
        result.Assign(buf);
        return NS_OK;
    }
    NS_ERROR("PR_NetAddrToString failed unexpectedly");
    return NS_ERROR_FAILURE; // conversion failed for some reason
}

NS_IMETHODIMP
nsDNSRecord::HasMore(PRBool *result)
{
    if (mDone)
        *result = PR_FALSE;
    else {
        // unfortunately, NSPR does not provide a way for us to determine if
        // there is another address other than to simply get the next address.
        void *iterCopy = mIter;
        PRNetAddr addr;
        *result = NS_SUCCEEDED(GetNextAddr(0, &addr));
        mIter = iterCopy; // backup iterator
        mDone = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRecord::Rewind()
{
    mIter = nsnull;
    mIterGenCnt = -1;
    mDone = PR_FALSE;
    return NS_OK;
}

//-----------------------------------------------------------------------------

class nsDNSAsyncRequest : public nsResolveHostCallback
                        , public nsICancelable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICANCELABLE

    nsDNSAsyncRequest(nsHostResolver   *res,
                      const nsACString &host,
                      nsIDNSListener   *listener,
                      PRUint16          flags,
                      PRUint16          af)
        : mResolver(res)
        , mHost(host)
        , mListener(listener)
        , mFlags(flags)
        , mAF(af) {}
    ~nsDNSAsyncRequest() {}

    void OnLookupComplete(nsHostResolver *, nsHostRecord *, nsresult);

    nsRefPtr<nsHostResolver> mResolver;
    nsCString                mHost; // hostname we're resolving
    nsCOMPtr<nsIDNSListener> mListener;
    PRUint16                 mFlags;
    PRUint16                 mAF;
};

void
nsDNSAsyncRequest::OnLookupComplete(nsHostResolver *resolver,
                                    nsHostRecord   *hostRecord,
                                    nsresult        status)
{
    // need to have an owning ref when we issue the callback to enable
    // the caller to be able to addref/release multiple times without
    // destroying the record prematurely.
    nsCOMPtr<nsIDNSRecord> rec;
    if (NS_SUCCEEDED(status)) {
        NS_ASSERTION(hostRecord, "no host record");
        rec = new nsDNSRecord(hostRecord);
        if (!rec)
            status = NS_ERROR_OUT_OF_MEMORY;
    }

    mListener->OnLookupComplete(this, rec, status);
    mListener = nsnull;

    // release the reference to ourselves that was added before we were
    // handed off to the host resolver.
    NS_RELEASE_THIS();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSAsyncRequest, nsICancelable)

NS_IMETHODIMP
nsDNSAsyncRequest::Cancel(nsresult reason)
{
    NS_ENSURE_ARG(NS_FAILED(reason));
    mResolver->DetachCallback(mHost.get(), mFlags, mAF, this, reason);
    return NS_OK;
}

//-----------------------------------------------------------------------------

class nsDNSSyncRequest : public nsResolveHostCallback
{
public:
    nsDNSSyncRequest(PRMonitor *mon)
        : mDone(PR_FALSE)
        , mStatus(NS_OK)
        , mMonitor(mon) {}
    virtual ~nsDNSSyncRequest() {}

    void OnLookupComplete(nsHostResolver *, nsHostRecord *, nsresult);

    PRBool                 mDone;
    nsresult               mStatus;
    nsRefPtr<nsHostRecord> mHostRecord;

private:
    PRMonitor             *mMonitor;
};

void
nsDNSSyncRequest::OnLookupComplete(nsHostResolver *resolver,
                                   nsHostRecord   *hostRecord,
                                   nsresult        status)
{
    // store results, and wake up nsDNSService::Resolve to process results.
    PR_EnterMonitor(mMonitor);
    mDone = PR_TRUE;
    mStatus = status;
    mHostRecord = hostRecord;
    PR_Notify(mMonitor);
    PR_ExitMonitor(mMonitor);
}

//-----------------------------------------------------------------------------

nsDNSService::nsDNSService()
    : mLock(nsnull)
{
}

nsDNSService::~nsDNSService()
{
    if (mLock)
        PR_DestroyLock(mLock);
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDNSService, nsIDNSService, nsPIDNSService,
                              nsIObserver)

NS_IMETHODIMP
nsDNSService::Init()
{
    NS_ENSURE_TRUE(!mResolver, NS_ERROR_ALREADY_INITIALIZED);

    PRBool firstTime = (mLock == nsnull);

    // prefs
    PRUint32 maxCacheEntries  = 20;
    PRUint32 maxCacheLifetime = 1; // minutes
    PRBool   enableIDN        = PR_TRUE;
    PRBool   disableIPv6      = PR_FALSE;
    nsAdoptingCString ipv4OnlyDomains;

    // read prefs
    nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRInt32 val;
        if (NS_SUCCEEDED(prefs->GetIntPref(kPrefDnsCacheEntries, &val)))
            maxCacheEntries = (PRUint32) val;
        if (NS_SUCCEEDED(prefs->GetIntPref(kPrefDnsCacheExpiration, &val)))
            maxCacheLifetime = val / 60; // convert from seconds to minutes

        // ASSUMPTION: pref branch does not modify out params on failure
        prefs->GetBoolPref(kPrefEnableIDN, &enableIDN);
        prefs->GetBoolPref(kPrefDisableIPv6, &disableIPv6);
        prefs->GetCharPref(kPrefIPv4OnlyDomains, getter_Copies(ipv4OnlyDomains));
    }

    if (firstTime) {
        mLock = PR_NewLock();
        if (!mLock)
            return NS_ERROR_OUT_OF_MEMORY;

        // register as prefs observer
        if (prefs) {
            prefs->AddObserver(kPrefDnsCacheEntries, this, PR_FALSE);
            prefs->AddObserver(kPrefDnsCacheExpiration, this, PR_FALSE);
            prefs->AddObserver(kPrefEnableIDN, this, PR_FALSE);
            prefs->AddObserver(kPrefIPv4OnlyDomains, this, PR_FALSE);
            prefs->AddObserver(kPrefDisableIPv6, this, PR_FALSE);
        }
    }

    // we have to null out mIDN since we might be getting re-initialized
    // as a result of a pref change.
    nsCOMPtr<nsIIDNService> idn;
    if (enableIDN)
        idn = do_GetService(NS_IDNSERVICE_CONTRACTID);

    nsRefPtr<nsHostResolver> res;
    nsresult rv = nsHostResolver::Create(maxCacheEntries,
                                         maxCacheLifetime,
                                         getter_AddRefs(res));
    if (NS_SUCCEEDED(rv)) {
        // now, set all of our member variables while holding the lock
        nsAutoLock lock(mLock);
        mResolver = res;
        mIDN = idn;
        mIPv4OnlyDomains = ipv4OnlyDomains; // exchanges buffer ownership
        mDisableIPv6 = disableIPv6;
    }

    return rv;
}

NS_IMETHODIMP
nsDNSService::Shutdown()
{
    nsRefPtr<nsHostResolver> res;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        mResolver = nsnull;
    }
    if (res)
        res->Shutdown();
    return NS_OK;
}

NS_IMETHODIMP
nsDNSService::AsyncResolve(const nsACString  &hostname,
                           PRUint32           flags,
                           nsIDNSListener    *listener,
                           nsIEventTarget    *target,
                           nsICancelable    **result)
{
    // grab reference to global host resolver and IDN service.  beware
    // simultaneous shutdown!!
    nsRefPtr<nsHostResolver> res;
    nsCOMPtr<nsIIDNService> idn;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        idn = mIDN;
    }
    NS_ENSURE_TRUE(res, NS_ERROR_OFFLINE);

    const nsACString *hostPtr = &hostname;

    nsresult rv;
    nsCAutoString hostACE;
    if (idn && !IsASCII(hostname)) {
        if (NS_SUCCEEDED(idn->ConvertUTF8toACE(hostname, hostACE)))
            hostPtr = &hostACE;
    }

    nsCOMPtr<nsIDNSListener> listenerProxy;
    if (target) {
        rv = NS_GetProxyForObject(target,
                                  NS_GET_IID(nsIDNSListener),
                                  listener,
                                  NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(listenerProxy));
        if (NS_FAILED(rv)) return rv;
        listener = listenerProxy;
    }

    PRUint16 af = GetAFForLookup(*hostPtr);

    nsDNSAsyncRequest *req =
            new nsDNSAsyncRequest(res, *hostPtr, listener, flags, af);
    if (!req)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*result = req);

    // addref for resolver; will be released when OnLookupComplete is called.
    NS_ADDREF(req);
    rv = res->ResolveHost(req->mHost.get(), flags, af, req);
    if (NS_FAILED(rv)) {
        NS_RELEASE(req);
        NS_RELEASE(*result);
    }
    return rv;
}

NS_IMETHODIMP
nsDNSService::Resolve(const nsACString &hostname,
                      PRUint32          flags,
                      nsIDNSRecord    **result)
{
    // grab reference to global host resolver and IDN service.  beware
    // simultaneous shutdown!!
    nsRefPtr<nsHostResolver> res;
    nsCOMPtr<nsIIDNService> idn;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        idn = mIDN;
    }
    NS_ENSURE_TRUE(res, NS_ERROR_OFFLINE);

    const nsACString *hostPtr = &hostname;

    nsresult rv;
    nsCAutoString hostACE;
    if (idn && !IsASCII(hostname)) {
        if (NS_SUCCEEDED(idn->ConvertUTF8toACE(hostname, hostACE)))
            hostPtr = &hostACE;
    }

    //
    // sync resolve: since the host resolver only works asynchronously, we need
    // to use a mutex and a condvar to wait for the result.  however, since the
    // result may be in the resolvers cache, we might get called back recursively
    // on the same thread.  so, our mutex needs to be re-entrant.  in other words,
    // we need to use a monitor! ;-)
    //
    
    PRMonitor *mon = PR_NewMonitor();
    if (!mon)
        return NS_ERROR_OUT_OF_MEMORY;

    PR_EnterMonitor(mon);
    nsDNSSyncRequest syncReq(mon);

    PRUint16 af = GetAFForLookup(*hostPtr);

    rv = res->ResolveHost(PromiseFlatCString(*hostPtr).get(), flags, af, &syncReq);
    if (NS_SUCCEEDED(rv)) {
        // wait for result
        while (!syncReq.mDone)
            PR_Wait(mon, PR_INTERVAL_NO_TIMEOUT);

        if (NS_FAILED(syncReq.mStatus))
            rv = syncReq.mStatus;
        else {
            NS_ASSERTION(syncReq.mHostRecord, "no host record");
            nsDNSRecord *rec = new nsDNSRecord(syncReq.mHostRecord);
            if (!rec)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                NS_ADDREF(*result = rec);
        }
    }

    PR_ExitMonitor(mon);
    PR_DestroyMonitor(mon);
    return rv;
}

struct nsDNSTypeThreadInitInfo 
{
    nsCString name;
    PRUint32 type;
    PRUint32 flags;
    nsRefPtr<nsDNSService> dnsService;
    nsRefPtr<nsDNSTypeAsyncRequest> request;
    
    nsDNSTypeThreadInitInfo() {}
    ~nsDNSTypeThreadInitInfo() {}
};

static void
DNSTypeThreadFunc(void *arg)
{
    nsDNSTypeThreadInitInfo *info = (nsDNSTypeThreadInitInfo*) arg;
    nsCOMPtr<nsISimpleEnumerator> iter;
    nsresult rv = info->dnsService->ResolveWithType(info->name,
                                                    info->type,
                                                    info->flags,
                                                    getter_AddRefs(iter));

    info->request->OnLookupComplete(iter, rv);
    delete info;
}

NS_IMETHODIMP 
nsDNSService::ResolveWithType(const nsACString    & aName, 
                              PRUint32              aRecordType, 
                              PRUint32              aFlags, 
                              nsISimpleEnumerator **_retval)
{
    NS_ENSURE_ARG(_retval);
    
    switch (aRecordType) {
        case DNS_RECORD_ADDRESS: {
            nsCOMPtr<nsIDNSRecord> record; 
            nsresult rv = this->Resolve(aName, aFlags, getter_AddRefs(record));
            NS_ENSURE_SUCCESS(rv, rv);
            
            nsCOMPtr<nsISimpleEnumerator> iter = new nsDNSRecordIter(record);
            if (!iter)
                return NS_ERROR_OUT_OF_MEMORY;
            NS_ADDREF(*_retval = iter);
            return NS_OK;
        }
        case DNS_RECORD_SRV:
            return ResolveSRV(aName, aFlags, _retval);
            break;
        case DNS_RECORD_NAPTR:
            return ResolveNAPTR(aName, aFlags, _retval);
            break;      
    }

    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsDNSService::AsyncResolveWithType(const nsACString  & aName, 
                                   PRUint32            aType, 
                                   PRUint32            aFlags, 
                                   nsIDNSIterListener *aListener, 
                                   nsIEventTarget     *aListenerTarget, 
                                   nsICancelable     **_retval)
{
    NS_ENSURE_ARG(aListener);
    NS_ENSURE_ARG(_retval);

    nsresult rv = NS_OK;
    nsCOMPtr<nsIDNSIterListener> listenerProxy;
    if (aListenerTarget) {
        rv = NS_GetProxyForObject(aListenerTarget,
                                  NS_GET_IID(nsIDNSIterListener),
                                  aListener,
                                  NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(listenerProxy));
        NS_ENSURE_SUCCESS(rv, rv);
        aListener = listenerProxy;
    }
    nsRefPtr<nsDNSTypeAsyncRequest> req = new nsDNSTypeAsyncRequest(aListener);
    if (!req)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = req->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    nsDNSTypeThreadInitInfo *info = new nsDNSTypeThreadInitInfo;
    if (!info)
        return NS_ERROR_OUT_OF_MEMORY;
        
    info->name = aName;
    info->type = aType;
    info->flags = aFlags;
    info->dnsService = this;
    info->request = req;
    PRThread *thread = PR_CreateThread(PR_USER_THREAD,
                                      DNSTypeThreadFunc,
                                      info,
                                      PR_PRIORITY_NORMAL,
                                      PR_GLOBAL_THREAD,
                                      PR_UNJOINABLE_THREAD,
                                      0);
                                    
    if (!thread) {
        delete info;
        return NS_ERROR_UNEXPECTED;
    }

    NS_ADDREF(*_retval = req);
        
    return rv;
}

/////////////////////////////////////////////////////////

NS_IMETHODIMP
nsDNSService::GetMyHostName(nsACString &result)
{
    char name[100];
    if (PR_GetSystemInfo(PR_SI_HOSTNAME, name, sizeof(name)) == PR_SUCCESS) {
        result = name;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDNSService::Observe(nsISupports *subject, const char *topic, const PRUnichar *data)
{
    // we are only getting called if a preference has changed. 
    NS_ASSERTION(strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0,
        "unexpected observe call");

    //
    // Shutdown and this function are both only called on the UI thread, so we don't
    // have to worry about mResolver being cleared out from under us.
    //
    // NOTE Shutting down and reinitializing the service like this is obviously
    // suboptimal if Observe gets called several times in a row, but we don't
    // expect that to be the case.
    //

    if (mResolver) {
        Shutdown();
        Init();
    }
    return NS_OK;
}

PRUint16
nsDNSService::GetAFForLookup(const nsACString &host)
{
    if (mDisableIPv6)
        return PR_AF_INET;

    nsAutoLock lock(mLock);

    PRUint16 af = PR_AF_UNSPEC;

    if (!mIPv4OnlyDomains.IsEmpty()) {
        const char *domain, *domainEnd, *end;
        PRUint32 hostLen, domainLen;

        // see if host is in one of the IPv4-only domains
        domain = mIPv4OnlyDomains.BeginReading();
        domainEnd = mIPv4OnlyDomains.EndReading(); 

        nsACString::const_iterator hostStart;
        host.BeginReading(hostStart);
        hostLen = host.Length();

        do {
            // skip any whitespace
            while (*domain == ' ' || *domain == '\t')
                ++domain;

            // find end of this domain in the string
            end = strchr(domain, ',');
            if (!end)
                end = domainEnd;

            // to see if the hostname is in the domain, check if the domain
            // matches the end of the hostname.
            domainLen = end - domain;
            if (domainLen && hostLen >= domainLen) {
                const char *hostTail = hostStart.get() + hostLen - domainLen;
                if (PL_strncasecmp(domain, hostTail, domainLen) == 0) {
                    // now, make sure either that the hostname is a direct match or
                    // that the hostname begins with a dot.
                    if (hostLen == domainLen ||
                            *hostTail == '.' || *(hostTail - 1) == '.') {
                        af = PR_AF_INET;
                        break;
                    }
                }
            }

            domain = end + 1;
        } while (*end);
    }

    return af;
}
