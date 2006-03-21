/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Anya server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
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
 */

#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsScriptableRegion.h"
#include "nsRegionImpl.h"
#include "gfxImageFrame.h"
#include "membufFontMetrics.h"
#include "membufDeviceContext.h"
#include "membufRenderingContext.h"
#include "membufImage.h"
#include "membufScreen.h"
#include "membufScreenManager.h"
#include "membufRegion.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufFontMetrics)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufDeviceContext)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufRenderingContext)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufImage)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufScreenManager)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufRegion)

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
  // create an nsRegion and get the scriptable region from it
  nsCOMPtr <nsIRegion> rgn;
  NS_NEWXPCOM(rgn, membufRegion);
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

static const nsModuleComponentInfo components[] =
{
  { "Membuf nsFontMetrics",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    membufFontMetricsConstructor },
  { "Membuf Device Context",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    membufDeviceContextConstructor },
  { "Membuf Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    membufRenderingContextConstructor },
  { "Membuf nsImage",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    membufImageConstructor },
  { "Membuf Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/membuf;1",
    membufRegionConstructor },
  { "Membuf Screen Manager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    membufScreenManagerConstructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Blender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
  { "image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};

PR_STATIC_CALLBACK(void)
nsGfxMembufModuleDtor(nsIModule *self)
{
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxMembufModule,
                              components,
                              nsGfxMembufModuleDtor)


