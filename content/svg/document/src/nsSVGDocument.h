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

#ifndef nsSVGDocument_h__
#define nsSVGDocument_h__

#include "nsXMLDocument.h"
#include "nsIDOMSVGDocument.h"
#include "nsString.h"
#include "nsIURI.h"

class nsSVGDocument : public nsXMLDocument,
                      public nsIDOMSVGDocument
{
 public:
  nsSVGDocument();
  virtual ~nsSVGDocument();

  // Most methods will just fall through to the XMLDocument
  
  NS_IMETHOD StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener,
                               PRBool aReset = PR_TRUE);

  NS_DECL_NSIDOMSVGDOCUMENT
  NS_FORWARD_NSIDOMDOCUMENT(nsXMLDocument::)
  NS_FORWARD_NSIDOMNODE(nsXMLDocument::)
  NS_FORWARD_NSIDOMDOCUMENTEVENT(nsXMLDocument::)
  NS_DECL_ISUPPORTS_INHERITED

protected:
  nsString mReferrer;
};

#endif
