/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Bradley Baetz
 * Copyright (C) 2001 Bradley Baetz. All Rights Reserved.
 *
 * Contributor(s): 
 *         Bradley Baetz <bbaetz@cs.mcgill.ca> (original author)
 *
 */

#include "nsSVGDocument.h"
#include "nsIDOMClassInfo.h"
#include "nsContentUtils.h"
#include "nsIHttpChannel.h"
#include "nsXPIDLString.h"

NS_INTERFACE_MAP_BEGIN(nsSVGDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGDocument)
NS_INTERFACE_MAP_END_INHERITING(nsXMLDocument)

NS_IMPL_ADDREF_INHERITED(nsSVGDocument, nsXMLDocument)
NS_IMPL_RELEASE_INHERITED(nsSVGDocument, nsXMLDocument)

nsSVGDocument::nsSVGDocument() {

}

nsSVGDocument::~nsSVGDocument() {

}

NS_IMETHODIMP
nsSVGDocument::StartDocumentLoad(const char* aCommand,
                                 nsIChannel* aChannel,
                                 nsILoadGroup* aLoadGroup,
                                 nsISupports* aContainer,
                                 nsIStreamListener **aDocListener,
                                 PRBool aReset) {
  nsresult rv = nsXMLDocument::StartDocumentLoad(aCommand,
                                                 aChannel,
                                                 aLoadGroup,
                                                 aContainer,
                                                 aDocListener,
                                                 aReset);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  if (httpChannel) {
    nsXPIDLCString referrer;
    rv = httpChannel->GetResponseHeader("referrer", getter_Copies(referrer));
    if (NS_SUCCEEDED(rv)) {
      mReferrer.AssignWithConversion(referrer);
    }
  }

  return NS_OK;
}

// nsIDOMSVGDocument

NS_IMETHODIMP
nsSVGDocument::GetTitle(nsAWritableString& aTitle) {
  return nsXMLDocument::GetTitle(aTitle);
}

NS_IMETHODIMP
nsSVGDocument::GetReferrer(nsAWritableString& aReferrer) {
  aReferrer.Assign(mReferrer);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDocument::GetDomain(nsAWritableString& aDomain) {
  if (!mDocumentURL) {
    aDomain.Truncate();
  } else {
    nsXPIDLCString domain;
    nsresult rv = mDocumentURL->GetHost(getter_Copies(domain));
    if (NS_FAILED(rv)) return rv;
    
    aDomain.Assign(NS_ConvertASCIItoUCS2(domain));
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDocument::GetURL(nsAWritableString& aURL) {
  if (!mDocumentURL) {
    aURL.Truncate();
  } else {
    nsXPIDLCString url;
    nsresult rv = mDocumentURL->GetSpec(getter_Copies(url));
    if (NS_FAILED(rv)) return rv;    
    aURL.Assign(NS_ConvertASCIItoUCS2(url));
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDocument::GetRootElement(nsIDOMSVGSVGElement** aRootElement) {
  NS_NOTYETIMPLEMENTED("nsSVGDocument::GetRootElement");
  // XXX - writeme
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_LAYOUT nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult)
{
  nsSVGDocument* doc = new nsSVGDocument();
  if (!doc)
    return NS_ERROR_OUT_OF_MEMORY;
  return doc->QueryInterface(NS_GET_IID(nsIDocument),
                             (void**) aInstancePtrResult);
}
