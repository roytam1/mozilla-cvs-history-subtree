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

#include "nsCOMPtr.h"
#include "nsIElementFactory.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsSVGAtoms.h"
#include "nsIXMLContent.h"

extern nsresult NS_NewSVGPolylineElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGPolygonElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGCircleElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGEllipseElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGLineElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGRectElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGSVGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGForeignObjectElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
extern nsresult NS_NewSVGPathElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);


class nsSVGElementFactory : public nsIElementFactory
{
protected:
  nsSVGElementFactory();
  virtual ~nsSVGElementFactory();

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIElementFactory interface
  NS_IMETHOD CreateInstanceByTag(nsINodeInfo *aNodeInfo, nsIContent** aResult);
  
public:
  friend nsresult NS_NewSVGElementFactory(nsIElementFactory** aResult);
};



nsSVGElementFactory::nsSVGElementFactory()
{
  NS_INIT_ISUPPORTS();
}

nsSVGElementFactory::~nsSVGElementFactory()
{
  
}


NS_IMPL_ISUPPORTS1(nsSVGElementFactory, nsIElementFactory);


nsresult
NS_NewSVGElementFactory(nsIElementFactory** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsSVGElementFactory* result = new nsSVGElementFactory();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}



NS_IMETHODIMP
nsSVGElementFactory::CreateInstanceByTag(nsINodeInfo *aNodeInfo,
                                           nsIContent** aResult)
{
  nsCOMPtr<nsIAtom> name;
  aNodeInfo->GetNameAtom(*getter_AddRefs(name));
  
  if (name == nsSVGAtoms::polyline)
    return NS_NewSVGPolylineElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::polygon)
    return NS_NewSVGPolygonElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::circle)
    return NS_NewSVGCircleElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::ellipse)
    return NS_NewSVGEllipseElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::line)
    return NS_NewSVGLineElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::rect)
    return NS_NewSVGRectElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::svg)
    return NS_NewSVGSVGElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::g)
    return NS_NewSVGGElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::foreignObject)
    return NS_NewSVGForeignObjectElement(aResult, aNodeInfo);
  else if (name == nsSVGAtoms::path)
    return NS_NewSVGPathElement(aResult, aNodeInfo);

  // if we don't know what to create, just create a standard xml element:
  nsCOMPtr<nsIXMLContent> xmlContent;
  nsresult rv;
  rv = NS_NewXMLElement(getter_AddRefs(xmlContent), aNodeInfo);
  
  *aResult = xmlContent;
  NS_IF_ADDREF(*aResult);
  
  return rv;
}

