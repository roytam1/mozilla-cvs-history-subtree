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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
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

#include <windows.h>
#include <Gdiplus.h>
using namespace Gdiplus;

#include "nsCOMPtr.h"
#include "nsSVGGDIPlusPathGeometry.h"
#include "nsISVGRendererPathGeometry.h"
#include "nsISVGGDIPlusRenderContext.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGGDIPlusRegion.h"
#include "nsISVGRendererRegion.h"
#include "nsSVGGDIPlusPathBuilder.h"
#include "nsISVGPathGeometrySource.h"
#include "nsISVGRendererPathBuilder.h"
#include "nsMemory.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGDIPlusPathGeometry class

class nsSVGGDIPlusPathGeometry : public nsISVGRendererPathGeometry
{
protected:
  friend nsresult NS_NewSVGGDIPlusPathGeometry(nsISVGRendererPathGeometry **result,
                                               nsISVGPathGeometrySource *src);

  nsSVGGDIPlusPathGeometry();
  ~nsSVGGDIPlusPathGeometry();
  nsresult Init(nsISVGPathGeometrySource* src);

public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS
  
  // nsISVGRendererPathGeometry interface:
  NS_DECL_NSISVGRENDERERPATHGEOMETRY
  
protected:
  void ClearPath() { if (mPath) { delete mPath; mPath=nsnull; } }
  void ClearFill() { if (mFill) { delete mFill; mFill=nsnull; } }
  void ClearStroke() { if (mStroke) {delete mStroke; mStroke=nsnull; } }
  void ClearCoveredRegion() { mCoveredRegion = nsnull; }
  void ClearHitTestRegion() { if (mHitTestRegion) {delete mHitTestRegion; mHitTestRegion=nsnull; } }
  GraphicsPath *GetPath();
  GraphicsPath *GetFill();
  GraphicsPath *GetStroke();
  Region *GetHitTestRegion();
  
  void GetGlobalTransform(Matrix *matrix);
  void RenderPath(GraphicsPath *path, nscolor color, float opacity,
                  nsISVGGDIPlusRenderContext *ctx);
private:
  nsCOMPtr<nsISVGPathGeometrySource> mSource;
  GraphicsPath *mPath; // untransformed path
  GraphicsPath *mFill;
  GraphicsPath *mStroke;
  Region *mHitTestRegion;
  nsCOMPtr<nsISVGRendererRegion> mCoveredRegion;
};

//----------------------------------------------------------------------
// implementation:

nsSVGGDIPlusPathGeometry::nsSVGGDIPlusPathGeometry()
    : mPath(nsnull), mFill(nsnull), mStroke(nsnull), mHitTestRegion(nsnull)
{
  NS_INIT_ISUPPORTS();
}

nsSVGGDIPlusPathGeometry::~nsSVGGDIPlusPathGeometry()
{
  ClearPath();
  ClearFill();
  ClearStroke();
  ClearCoveredRegion();
  ClearHitTestRegion();
}

nsresult nsSVGGDIPlusPathGeometry::Init(nsISVGPathGeometrySource* src)
{
  mSource = src;
  return NS_OK;
}


nsresult
NS_NewSVGGDIPlusPathGeometry(nsISVGRendererPathGeometry **result,
                             nsISVGPathGeometrySource *src)
{
  nsSVGGDIPlusPathGeometry* pg = new nsSVGGDIPlusPathGeometry();
  if (!pg) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(pg);

  nsresult rv = pg->Init(src);

  if (NS_FAILED(rv)) {
    NS_RELEASE(pg);
    return rv;
  }
  
  *result = pg;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGGDIPlusPathGeometry)
NS_IMPL_RELEASE(nsSVGGDIPlusPathGeometry)

NS_INTERFACE_MAP_BEGIN(nsSVGGDIPlusPathGeometry)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererPathGeometry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------

GraphicsPath *
nsSVGGDIPlusPathGeometry::GetPath()
{
  if (mPath) return mPath;

  mPath = new GraphicsPath();
  nsCOMPtr<nsISVGRendererPathBuilder> builder;
  NS_NewSVGGDIPlusPathBuilder(getter_AddRefs(builder), mSource, mPath);
  mSource->ConstructPath(builder);
  return mPath;
}

GraphicsPath *
nsSVGGDIPlusPathGeometry::GetFill()
{
  if (mFill) return mFill;
  GraphicsPath *path = GetPath();
  if (!path) return nsnull;

  mFill = path->Clone();
  Matrix m;
  GetGlobalTransform(&m);
  mFill->Transform(&m);

  return mFill;
}

GraphicsPath *
nsSVGGDIPlusPathGeometry::GetStroke()
{
  if (mStroke) return mStroke;
  GraphicsPath *path = GetPath();
  if (!path) return nsnull;

  float width;
  mSource->GetStrokeWidth(&width);
  if (width==0.0f) return nsnull;
  
  mStroke = path->Clone();

  // construct the pen:

  // gdi+ seems to widen paths to >1 width unit. If our pen width is
  // smaller, we need to widen an appropriately enlarged path and then
  // scale it down after widening:
  PRBool scaleToUnitPen = width<1.0f;
  
  Pen pen(Color(), scaleToUnitPen ? 1.0f : width);

  // set linecap style
  PRUint16 capStyle;
  LineCap lcap;
  DashCap dcap;
  mSource->GetStrokeLinecap(&capStyle);
  switch (capStyle) {
    case nsISVGGeometrySource::STROKE_LINECAP_BUTT:
      lcap = LineCapFlat;
      dcap = DashCapFlat;
      break;
    case nsISVGGeometrySource::STROKE_LINECAP_ROUND:
      lcap = LineCapRound;
      dcap = DashCapRound;
      break;
    case nsISVGGeometrySource::STROKE_LINECAP_SQUARE:
      lcap = LineCapSquare;
      dcap = DashCapFlat;
      break;
  }
  pen.SetLineCap(lcap, lcap, dcap);

  // set miterlimit:
  float miterlimit;
  mSource->GetStrokeMiterlimit(&miterlimit);
  pen.SetMiterLimit(miterlimit);

  // set linejoin style:
  PRUint16 joinStyle;
  LineJoin join;
  mSource->GetStrokeLinejoin(&joinStyle);
  switch(joinStyle) {
    case nsISVGGeometrySource::STROKE_LINEJOIN_MITER:
      join = LineJoinMiterClipped;
      break;
    case nsISVGGeometrySource::STROKE_LINEJOIN_ROUND:
      join = LineJoinRound;
      break;
    case nsISVGGeometrySource::STROKE_LINEJOIN_BEVEL:
      join = LineJoinBevel;
      break;
  }
  pen.SetLineJoin(join);
  
  // set pen dashpattern
  float *dashArray;
  PRUint32 count;
  mSource->GetStrokeDashArray(&dashArray, &count);
  if (count>0) {
    if (!scaleToUnitPen) {
      for (PRUint32 i=0; i<count; ++i)
        dashArray[i]/=width;
    }
    
    pen.SetDashPattern(dashArray, count);
    nsMemory::Free(dashArray);

    float offset;
    mSource->GetStrokeDashoffset(&offset);
    if (!scaleToUnitPen)
      offset/=width;
    pen.SetDashOffset(offset);
  }
  
  Matrix m;
  GetGlobalTransform(&m);

  if (scaleToUnitPen) {
    Matrix enlarge(1/width, 0, 0, 1/width, 0, 0);
    mStroke->Transform(&enlarge);
    m.Scale(width,width);
  }
  
  mStroke->Widen(&pen);

  
//  mStroke->Outline();
  mStroke->Transform(&m);
  
  return mStroke;
}

Region *
nsSVGGDIPlusPathGeometry::GetHitTestRegion()
{
  if (mHitTestRegion) return mHitTestRegion;

  mHitTestRegion = new Region();
  if (!mHitTestRegion) {
    NS_ERROR("could not construct region");
    return nsnull;
  }

  mHitTestRegion->MakeEmpty();

  PRUint16 mask=0;
  mSource->GetHittestMask(&mask);
  if (mask & nsISVGPathGeometrySource::HITTEST_MASK_FILL)
    mHitTestRegion->Union(GetFill());
  if (mask & nsISVGPathGeometrySource::HITTEST_MASK_STROKE)
    mHitTestRegion->Union(GetStroke());

  return mHitTestRegion;
}


void
nsSVGGDIPlusPathGeometry::GetGlobalTransform(Matrix *matrix)
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm;
  mSource->GetCTM(getter_AddRefs(ctm));
  NS_ASSERTION(ctm, "graphic source didn't specify a ctm");
  
  float m[6];
  float val;
  ctm->GetA(&val);
  m[0] = val;
  
  ctm->GetB(&val);
  m[1] = val;
  
  ctm->GetC(&val);  
  m[2] = val;  
  
  ctm->GetD(&val);  
  m[3] = val;  
  
  ctm->GetE(&val);
  m[4] = val;
  
  ctm->GetF(&val);
  m[5] = val;

  matrix->SetElements(m[0],m[1],m[2],m[3],m[4],m[5]);
}

void
nsSVGGDIPlusPathGeometry::RenderPath(GraphicsPath *path, nscolor color, float opacity,
                                     nsISVGGDIPlusRenderContext *ctx)
{
  SolidBrush brush(Color((BYTE)(opacity*255),NS_GET_R(color),NS_GET_G(color),NS_GET_B(color)));
  ctx->GetGraphics()->FillPath(&brush, path);
}

//----------------------------------------------------------------------
// nsISVGRendererPathGeometry methods:

/* void render (in nsISVGRendererRenderContext ctx); */
NS_IMETHODIMP
nsSVGGDIPlusPathGeometry::Render(nsISVGRendererRenderContext *ctx)
{
  nsCOMPtr<nsISVGGDIPlusRenderContext> gdiplusCtx = do_QueryInterface(ctx);
  NS_ASSERTION(gdiplusCtx, "wrong svg render context for geometry!");
  if (!gdiplusCtx) return NS_ERROR_FAILURE;
  gdiplusCtx->GetGraphics()->SetSmoothingMode(SmoothingModeAntiAlias);
//  gdiplusCtx->GetGraphics()->SetPixelOffsetMode(PixelOffsetModeHalf);

  nscolor color;
  float opacity;
  PRUint16 type;
  
  // paint fill:
  mSource->GetFillPaintType(&type);
  if (type == nsISVGGeometrySource::PAINT_TYPE_SOLID_COLOR && GetFill()) {
    mSource->GetFillPaint(&color);
    mSource->GetFillOpacity(&opacity);
    RenderPath(GetFill(), color, opacity, gdiplusCtx);
  }

  // paint stroke:
  mSource->GetStrokePaintType(&type);
  if (type == nsISVGGeometrySource::PAINT_TYPE_SOLID_COLOR && GetStroke()) {
    mSource->GetStrokePaint(&color);
    mSource->GetStrokeOpacity(&opacity);
    RenderPath(GetStroke(), color, opacity, gdiplusCtx);
  }
  
  return NS_OK;
}

/* nsISVGRendererRegion update (in unsigned long updatemask); */
NS_IMETHODIMP
nsSVGGDIPlusPathGeometry::Update(PRUint32 updatemask, nsISVGRendererRegion **_retval)
{
  *_retval = nsnull;
#ifdef DEBUG
  //printf("nsSVGGDIPlusPathGeometry::Update %p\n", this);
#endif
  nsCOMPtr<nsISVGRendererRegion> before;
  GetCoveredRegion(getter_AddRefs(before));

  
  if (updatemask & UPDATEMASK_GEOMETRY_DATA) {
    ClearPath();
    ClearFill();
    ClearStroke();
    ClearCoveredRegion();
    ClearHitTestRegion();
    goto finished;
  }
  
  if (updatemask & UPDATEMASK_CTM) {
    // retain path
    ClearFill();
    ClearStroke();
    ClearCoveredRegion();
    ClearHitTestRegion();
    goto finished;
  }

  if (updatemask & UPDATEMASK_STROKE_STYLE) {
    ClearStroke();
    ClearCoveredRegion();
    ClearHitTestRegion();
  }

  if (updatemask & UPDATEMASK_FILL_STYLE) {
    ClearFill();
    ClearCoveredRegion();
    ClearHitTestRegion();
  }

  if (updatemask & UPDATEMASK_HITTEST_MASK) {
    ClearHitTestRegion();
  }
  
  if (updatemask & UPDATEMASK_FILL_PAINT) {
    // XXX suboptimal
  }

  if (updatemask & UPDATEMASK_STROKE_PAINT) {
    // XXX suboptimal
  }

  finished:
  nsCOMPtr<nsISVGRendererRegion> after;
  GetCoveredRegion(getter_AddRefs(after));
  if (after)
    after->Combine(before, _retval);
    
  return NS_OK;
}

/* nsISVGRendererRegion getCoveredRegion (); */
NS_IMETHODIMP
nsSVGGDIPlusPathGeometry::GetCoveredRegion(nsISVGRendererRegion **_retval)
{
  *_retval = nsnull;
  
  if (mCoveredRegion) {
    *_retval = mCoveredRegion;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  
  PRUint16 type;  
  mSource->GetFillPaintType(&type);
  bool hasCoveredFill = (type!=nsISVGGeometrySource::PAINT_TYPE_NONE) && GetFill();
  
  mSource->GetStrokePaintType(&type);
  bool hasCoveredStroke = (type!=nsISVGGeometrySource::PAINT_TYPE_NONE) && GetStroke();

  if (!hasCoveredFill && !hasCoveredStroke) return NS_OK;

  if (hasCoveredFill) {
    nsCOMPtr<nsISVGRendererRegion> reg1;
    NS_NewSVGGDIPlusPathRegion(getter_AddRefs(reg1), GetFill());
    if (hasCoveredStroke) {
      nsCOMPtr<nsISVGRendererRegion> reg2;
      NS_NewSVGGDIPlusPathRegion(getter_AddRefs(reg2), GetStroke());
      reg1->Combine(reg2, _retval);
    }
    else {
      *_retval = reg1;
      NS_ADDREF(*_retval);
    }
  } // covered stroke only
  else
    NS_NewSVGGDIPlusPathRegion(_retval, GetStroke());

  mCoveredRegion = *_retval;
  return NS_OK;
}

/* boolean containsPoint (in float x, in float y); */
NS_IMETHODIMP
nsSVGGDIPlusPathGeometry::ContainsPoint(float x, float y, PRBool *_retval)
{
  *_retval = PR_FALSE;
  
   if (GetHitTestRegion()->IsVisible(x,y)) {
     *_retval = PR_TRUE;
   }
  
  return NS_OK;
}
