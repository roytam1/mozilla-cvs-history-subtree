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
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGGElement.h"

typedef nsSVGGraphicElement nsSVGGElementBase;

class nsSVGGElement : public nsSVGGElementBase,
                      public nsIDOMSVGGElement
{
protected:
  friend nsresult NS_NewSVGGElement(nsIContent **aResult,
                                    nsINodeInfo *aNodeInfo);
  nsSVGGElement();
  virtual ~nsSVGGElement();
  virtual nsresult Init();
  
public:
  // interfaces:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGGELEMENT

  // xxx I wish we could use virtual inheritance
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsSVGGElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGGElementBase::)
  
protected:
};

////////////////////////////////////////////////////////////////////////
// implementation

nsresult NS_NewSVGGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo)
{
  *aResult = nsnull;
  nsSVGGElement* it = new nsSVGGElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsGenericElement*,it)->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }
  
  *aResult = NS_STATIC_CAST(nsIContent *, it);

  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGGElement,nsSVGGElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGElement,nsSVGGElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGGElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGElementBase)

//----------------------------------------------------------------------
// Implementation

nsSVGGElement::nsSVGGElement()
{

}

nsSVGGElement::~nsSVGGElement()
{

}

  
nsresult
nsSVGGElement::Init()
{
  nsresult rv;
  rv = nsSVGGElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMNode methods

NS_IMETHODIMP
nsSVGGElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  *aReturn = nsnull;
  nsSVGGElement* it = new nsSVGGElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsGenericElement*,it)->Init(mNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = CopyNode(it, aDeep);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }
 
  *aReturn = NS_STATIC_CAST(nsSVGGElementBase*, it);

  return NS_OK; 
}
