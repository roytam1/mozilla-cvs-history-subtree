/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Christopher
 * Blizzard.  Portions created by Christopher Blizzard are
 * Copyright (C) 2000 Christopher Blizzard. All Rights Reserved.
 *
 * Contributor(s): 
 *   Christopher Blizzzard <blizzard@mozilla.org>
 */

#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsFontMetricsMac.h"
#include "nsRegionMac.h"
#include "nsRenderingContextMac.h"
#include "nsDeviceContextSpecFactoryM.h"
#if TARGET_CARBON
#include "nsDeviceContextSpecX.h"
#else
#include "nsDeviceContextSpecMac.h"
#endif
#include "nsScreenManagerMac.h"
#include "nsScriptableRegion.h"
#include "nsIImageManager.h"
#include "nsDeviceContextMac.h"
#include "nsImageMac.h"
#if !TARGET_CARBON
#include "nsPrintOptionsMac.h"
#endif
#include "nsFontList.h"

// objects that just require generic constructors

NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionMac)
#if TARGET_CARBON
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecX)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecMac)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecFactoryMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrintOptionsMac)

// our custom constructors

static NS_IMETHODIMP nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst;

  if ( !aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = nsnull;
  if (aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }
  // create an nsRegionMac and get the scriptable region from it
  nsCOMPtr <nsIRegion> rgn;
  NS_NEWXPCOM(rgn, nsRegionMac);
  nsCOMPtr<nsIScriptableRegion> scriptableRgn;
  if (rgn != nsnull)
  {
    scriptableRgn = new nsScriptableRegion(rgn);
    inst = scriptableRgn;
  }
  if (!inst)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
  }
  NS_ADDREF(inst);
  // release our variable above now that we have created our owning
  // reference - we don't want this to go out of scope early!
  scriptableRgn = nsnull;
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

static nsresult nsImageManagerConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;

  if ( !aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = nsnull;
  if (aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }
  // this will return an image manager with a count of 1
  rv = NS_NewImageManager((nsIImageManager **)aResult);
  return rv;
}

static nsModuleComponentInfo components[] =
{
  { "Mac Font Metrics",
    NS_FONT_METRICS_CID,
    //    "@mozilla.org/gfx/font_metrics/mac;1",
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsMacConstructor },
  { "Mac Device Context",
    NS_DEVICE_CONTEXT_CID,
    //    "@mozilla.org/gfx/device_context/mac;1",
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextMacConstructor },
  { "Mac Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    //    "@mozilla.org/gfx/rendering_context/mac;1",
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextMacConstructor },
  { "Mac Image",
    NS_IMAGE_CID,
    //    "@mozilla.org/gfx/image/mac;1",
    "@mozilla.org/gfx/image;1",
    nsImageMacConstructor },
  { "Mac Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/mac;1",
    nsRegionMacConstructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    //    "@mozilla.org/gfx/scriptable_region;1",
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Blender",
    NS_BLENDER_CID,
    //    "@mozilla.org/gfx/blender;1",
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
#if TARGET_CARBON
  { "Mac Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    //    "@mozilla.org/gfx/device_context_spec/mac;1",
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecXConstructor },
#else
  { "Mac Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    //    "@mozilla.org/gfx/device_context_spec/mac;1",
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecMacConstructor },
#endif
  { "Mac Device Context Spec Factory",
    NS_DEVICE_CONTEXT_SPEC_FACTORY_CID,
    //    "@mozilla.org/gfx/device_context_spec_factory/mac;1",
    "@mozilla.org/gfx/devicecontextspecfactory;1",
    nsDeviceContextSpecFactoryMacConstructor },
  { "Image Manager",
    NS_IMAGEMANAGER_CID,
    //    "@mozilla.org/gfx/image_manager;1",
    "@mozilla.org/gfx/imagemanager;1",
    nsImageManagerConstructor },
#if !TARGET_CARBON
  { "Print Options",
    NS_PRINTOPTIONS_CID,
    //    "@mozilla.org/gfx/printoptions;1",
    "@mozilla.org/gfx/printoptions;1",
    nsPrintOptionsMacConstructor },
#endif
  { "Mac Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    //    "@mozilla.org/gfx/font_enumerator/mac;1",
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorMacConstructor },
  { "Font List",  
    NS_FONTLIST_CID,
    //    "@mozilla.org/gfx/fontlist;1"
    NS_FONTLIST_CONTRACTID,
    nsFontListConstructor },
  { "Mac Screen Manager",
    NS_SCREENMANAGER_CID,
    //    "@mozilla.org/gfx/screenmanager/mac;1",
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerMacConstructor }
};

PR_STATIC_CALLBACK(void)
nsGfxModuleDtor(nsIModule *self)
{
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxModule, components, nsGfxModuleDtor)

