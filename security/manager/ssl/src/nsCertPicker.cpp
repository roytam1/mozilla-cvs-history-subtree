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
 * The Original Code is __________________________________________.
 *
 * The Initial Developer of the Original Code is
 * ____________________________________________.
 * Portions created by the Initial Developer are Copyright (C) 2___
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsCertPicker.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"
#include "nsIProxyObjectManager.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsNSSComponent.h"
#include "nsNSSCertificate.h"
#include "nsINSSDialogs.h"
#include "nsNSSAutoPtr.h"

#include "cert.h"

/* strings for marking invalid user cert nicknames */
#define NICKNAME_EXPIRED_STRING " (expired)"
#define NICKNAME_NOT_YET_VALID_STRING " (not yet valid)"


NS_IMPL_ISUPPORTS1(nsCertPicker, nsIUserCertPicker)

nsCertPicker::nsCertPicker()
{
  NS_INIT_ISUPPORTS();
}

nsCertPicker::~nsCertPicker()
{
}

/* nsIX509Cert pick (in nsIInterfaceRequestor ctx, in wstring title, in wstring infoPrompt, in PRInt32 certUsage, in boolean allowInvalid, in boolean allowDuplicateNicknames, out boolean canceled); */
NS_IMETHODIMP nsCertPicker::PickByUsage(nsIInterfaceRequestor *ctx, const PRUnichar *title, const PRUnichar *infoPrompt, PRInt32 certUsage, PRBool allowInvalid, PRBool allowDuplicateNicknames, PRBool *canceled, nsIX509Cert **_retval)
{
  PRInt32 i = 0;
  PRInt32 selectedIndex = -1;
  PRUnichar **certNicknameList = nsnull;
  PRUnichar **certDetailsList = nsnull;
  CERTCertListNode* node = nsnull;
  CERTCertificate* cert = nsnull;
  nsresult rv;

  {
    CERTCertList *allcerts = nsnull;
    nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
    allcerts = PK11_ListCerts(PK11CertListUnique, ctx);
    CERT_DestroyCertList(allcerts);
  }


  /* find all user certs that are valid and for SSL */
  /* note that we are allowing expired certs in this list */

  nsNSSAutoCPtr<CERTCertList> certList(
    CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                              (SECCertUsage)certUsage,
                              !allowDuplicateNicknames,
                              !allowInvalid,
                              ctx),
    CERT_DestroyCertList
  );
  
  if (!certList) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  nsNSSAutoCPtr<CERTCertNicknames> nicknames(
    CERT_NicknameStringsFromCertList(certList,
                                     NICKNAME_EXPIRED_STRING,
                                     NICKNAME_NOT_YET_VALID_STRING),
    CERT_FreeNicknames
  );

  if (!certList) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  certNicknameList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);
  certDetailsList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);

  nsCOMPtr<nsIProxyObjectManager> proxyman(do_GetService(NS_XPCOMPROXY_CONTRACTID));
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));

  if (proxyman && nssComponent)
  for (i = 0, node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       ++i, node = CERT_LIST_NEXT(node)
      )
  {
    nsNSSCertificate *tempCert = new nsNSSCertificate(node->cert);
    NS_ADDREF(tempCert);

    nsCOMPtr<nsIX509Cert> x509 = do_QueryInterface(tempCert);

    nsCOMPtr<nsIX509Cert> x509Proxy;
    proxyman->GetProxyForObject( NS_UI_THREAD_EVENTQ,
                                 nsIX509Cert::GetIID(),
                                 x509,
                                 PROXY_SYNC | PROXY_ALWAYS,
                                 getter_AddRefs(x509Proxy));

    if (x509Proxy) {
      nsAutoString nickWithSerial;
      nsAutoString str;
      nsAutoString info;
      PRUnichar *temp1 = 0;

      nickWithSerial.Append(NS_ConvertUTF8toUCS2(nicknames->nicknames[i]));

      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoIssuedFor").get(), info))) {
        str.Append(info);
        str.Append(NS_LITERAL_STRING("\n"));
      }

      if (NS_SUCCEEDED(x509Proxy->GetSubjectName(&temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
        str.Append(NS_LITERAL_STRING("  "));
        if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubject").get(), info))) {
          str.Append(info);
          str.Append(NS_LITERAL_STRING(": "));
        }
        str.Append(temp1);
        nsMemory::Free(temp1);
        str.Append(NS_LITERAL_STRING("\n"));
      }

      if (NS_SUCCEEDED(x509Proxy->GetSerialNumber(&temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
        str.Append(NS_LITERAL_STRING("  "));
        if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSerialNo").get(), info))) {
          str.Append(info);
          str.Append(NS_LITERAL_STRING(": "));
        }
        str.Append(temp1);

        nickWithSerial.Append(NS_LITERAL_STRING(" ["));
        nickWithSerial.Append(temp1);
        nickWithSerial.Append(NS_LITERAL_STRING("]"));

        nsMemory::Free(temp1);
        str.Append(NS_LITERAL_STRING("\n"));
      }


      {
        nsCOMPtr<nsIX509CertValidity> validity;
        nsCOMPtr<nsIX509CertValidity> originalValidity;
        rv = x509Proxy->GetValidity(getter_AddRefs(originalValidity));
        if (NS_SUCCEEDED(rv) && originalValidity) {
          proxyman->GetProxyForObject( NS_UI_THREAD_EVENTQ,
                                       nsIX509CertValidity::GetIID(),
                                       originalValidity,
                                       PROXY_SYNC | PROXY_ALWAYS,
                                       getter_AddRefs(validity));
        }

        if (validity) {
          str.Append(NS_LITERAL_STRING("  "));
          if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoValid").get(), info))) {
            str.Append(info);
          }

          if (NS_SUCCEEDED(validity->GetNotBeforeLocalTime(&temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
            str.Append(NS_LITERAL_STRING(" "));
            if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoFrom").get(), info))) {
              str.Append(info);
            }
            str.Append(NS_LITERAL_STRING(" "));
            str.Append(temp1);
            nsMemory::Free(temp1);
          }

          if (NS_SUCCEEDED(validity->GetNotAfterLocalTime(&temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
            str.Append(NS_LITERAL_STRING(" "));
            if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoTo").get(), info))) {
              str.Append(info);
            }
            str.Append(NS_LITERAL_STRING(" "));
            str.Append(temp1);
            nsMemory::Free(temp1);
          }

          str.Append(NS_LITERAL_STRING("\n"));
        }
      }

      PRUint32 tempInt = 0;
      if (NS_SUCCEEDED(x509Proxy->GetPurposes(&tempInt, &temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
        str.Append(NS_LITERAL_STRING("  "));
        if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoPurposes").get(), info))) {
          str.Append(info);
        }
        str.Append(NS_LITERAL_STRING(": "));
        str.Append(temp1);
        nsMemory::Free(temp1);
        str.Append(NS_LITERAL_STRING("\n"));
      }

      if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertInfoIssuedBy").get(), info))) {
        str.Append(info);
        str.Append(NS_LITERAL_STRING("\n"));
      }

      if (NS_SUCCEEDED(x509Proxy->GetIssuerName(&temp1)) && temp1 && nsCharTraits<PRUnichar>::length(temp1)) {
        str.Append(NS_LITERAL_STRING("  "));
        if (NS_SUCCEEDED(nssComponent->GetPIPNSSBundleString(NS_LITERAL_STRING("CertDumpSubject").get(), info))) {
          str.Append(info);
          str.Append(NS_LITERAL_STRING(": "));
        }
        str.Append(temp1);
        nsMemory::Free(temp1);
        str.Append(NS_LITERAL_STRING("\n"));
      }

      /*
        the above produces output the following output:

        Issued to: 
          Subject: $subjectName
          Serial number: $serialNumber
          Valid from: $starting_date to $expriation_date
          Purposes: $purposes
        Issued by:
          Subject: $issuerName
      */

      certNicknameList[i] = nickWithSerial.ToNewUnicode();
      certDetailsList[i] = str.ToNewUnicode();
    }

    NS_RELEASE(tempCert);
  }

  nsICertPickDialogs *dialogs = nsnull;
  rv = getNSSDialogs((void**)&dialogs, NS_GET_IID(nsICertPickDialogs));

  if (NS_SUCCEEDED(rv)) {
    /* Throw up the cert picker dialog and get back the index of the selected cert */
    rv = dialogs->PickCertificate(ctx, title, infoPrompt,
      (const PRUnichar**)certNicknameList, (const PRUnichar**)certDetailsList,
      nicknames->numnicknames, &selectedIndex, canceled);

    for (i = 0; i < nicknames->numnicknames; ++i) {
      nsMemory::Free(certNicknameList[i]);
      nsMemory::Free(certDetailsList[i]);
    }
    nsMemory::Free(certNicknameList);
    nsMemory::Free(certDetailsList);

    NS_RELEASE(dialogs);
  }

  if (NS_SUCCEEDED(rv) && !*canceled) {
    for (i = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         ++i, node = CERT_LIST_NEXT(node)) {

      if (i == selectedIndex) {
        nsNSSCertificate *cert = new nsNSSCertificate(node->cert);
        if (!cert) {
          return NS_ERROR_OUT_OF_MEMORY;
        }

        nsIX509Cert *x509 = 0;
        nsresult rv = cert->QueryInterface(NS_GET_IID(nsIX509Cert), (void**)&x509);
        if (NS_FAILED(rv)) {
          return rv;
        }
        
        NS_ADDREF(x509);
        *_retval = x509;
        NS_RELEASE(cert);
        break;
      }
    }
  }

  return NS_OK;
}
