/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsCOMPtr.h"
#include "nsIXTFService.h"
#include "nsIXTFElementFactory.h"
#include "nsIXTFElement.h"
#include "nsIXTFGenericElement.h"
#include "nsIXTFSVGVisual.h"
#include "nsIXTFXMLVisual.h"
#include "nsIXTFXULVisual.h"
#include "nsIElementFactory.h"
#include "nsString.h"
#include "nsINodeInfo.h"
#include "nsIXMLContent.h"
#include "nsXTFGenericElementWrapper.h"
#include "nsXTFSVGVisualWrapper.h"
#include "nsXTFXMLVisualWrapper.h"
#include "nsXTFXULVisualWrapper.h"

////////////////////////////////////////////////////////////////////////
// nsXTFElementFactoryWrapper class
class nsXTFElementFactoryWrapper : public nsIElementFactory
{
protected:
  friend nsresult
  NS_NewXTFElementFactoryWrapper(nsIXTFElementFactory* xtfFactory,
                                 nsIElementFactory** aResult);

  nsXTFElementFactoryWrapper(nsIXTFElementFactory* xtfFactory);
  
public:
  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIElementFactory interface
  NS_IMETHOD CreateInstanceByTag(nsINodeInfo *aNodeInfo,
                                 nsIContent** aResult);
private:
  nsCOMPtr<nsIXTFElementFactory> mXTFFactory;
};

//----------------------------------------------------------------------
// implementation:

nsXTFElementFactoryWrapper::nsXTFElementFactoryWrapper(nsIXTFElementFactory* xtfFactory)
    : mXTFFactory(xtfFactory)
{
}

nsresult
NS_NewXTFElementFactoryWrapper(nsIXTFElementFactory* xtfFactory,
                               nsIElementFactory** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  if (!xtfFactory) {
    NS_ERROR("can't construct an xtf element factory wrapper without a factory");
    return NS_ERROR_FAILURE;
  }
  
  nsXTFElementFactoryWrapper* result = new nsXTFElementFactoryWrapper(xtfFactory);
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports implementation

NS_IMPL_ISUPPORTS1(nsXTFElementFactoryWrapper, nsIElementFactory);

//----------------------------------------------------------------------
// nsIElementFactory implementation

NS_IMETHODIMP
nsXTFElementFactoryWrapper::CreateInstanceByTag(nsINodeInfo *aNodeInfo,
                                                nsIContent** aResult)
{
  nsCOMPtr<nsIXTFElement> elem;
  nsAutoString tagName;
  aNodeInfo->GetName(tagName);
#ifdef DEBUG
  printf("nsXTFElementFactoryWrapper::CreateInstanceByTag(%s)\n",NS_ConvertUTF16toUTF8(tagName).get());
#endif
  mXTFFactory->CreateElement(tagName, getter_AddRefs(elem));
  if (elem) {
    // we've got an xtf element. create an appropriate wrapper for it:
    PRUint32 elementType;
    elem->GetElementType(&elementType);
    switch (elementType) {
      case nsIXTFElement::ELEMENT_TYPE_GENERIC_ELEMENT:
      {
        nsCOMPtr<nsIXTFGenericElement> elem2 = do_QueryInterface(elem);
        return NS_NewXTFGenericElementWrapper(elem2, aNodeInfo, aResult);
      break;
      }
      case nsIXTFElement::ELEMENT_TYPE_SVG_VISUAL:
      {
        nsCOMPtr<nsIXTFSVGVisual> elem2 = do_QueryInterface(elem);
        return NS_NewXTFSVGVisualWrapper(elem2, aNodeInfo, aResult);
        break;
      }
      case nsIXTFElement::ELEMENT_TYPE_XML_VISUAL:
      {
        nsCOMPtr<nsIXTFXMLVisual> elem2 = do_QueryInterface(elem);
        return NS_NewXTFXMLVisualWrapper(elem2, aNodeInfo, aResult);
        break;
      }
      case nsIXTFElement::ELEMENT_TYPE_XUL_VISUAL:
      {
        nsCOMPtr<nsIXTFXULVisual> elem2 = do_QueryInterface(elem);
        return NS_NewXTFXULVisualWrapper(elem2, aNodeInfo, aResult);
        break;
      }
      default:
        NS_ERROR("unknown xtf element type");
        break;
    }
  }
#ifdef DEBUG
  printf("nsXTFElementFactoryWrapper::CreateInstanceByTag: element could not be created\n");
#endif
  // if we don't know what to create, just create a standard xml element:
  return NS_NewXMLElement(aResult, aNodeInfo);
}

////////////////////////////////////////////////////////////////////////
// nsXTFService class 
class nsXTFService : public nsIXTFService
{
protected:
  friend nsresult NS_NewXTFService(nsIXTFService** aResult);
  
  nsXTFService();

public:
  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIXTFService interface
  NS_IMETHOD WrapXTFElementFactory(nsIXTFElementFactory* xtfFactory,
                                   nsIElementFactory** wrapper);

private:
};

//----------------------------------------------------------------------
// implementation:

nsXTFService::nsXTFService()
{
}

nsresult
NS_NewXTFService(nsIXTFService** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsXTFService* result = new nsXTFService();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ISUPPORTS1(nsXTFService, nsIXTFService);

//----------------------------------------------------------------------
// nsIXTFService methods

NS_IMETHODIMP
nsXTFService::WrapXTFElementFactory(nsIXTFElementFactory* xtfFactory,
                                    nsIElementFactory** wrapper)
{
#ifdef DEBUG
  printf("nsXTFService::WrapXTFElementFactory\n");
#endif
  return NS_NewXTFElementFactoryWrapper(xtfFactory, wrapper);
}
