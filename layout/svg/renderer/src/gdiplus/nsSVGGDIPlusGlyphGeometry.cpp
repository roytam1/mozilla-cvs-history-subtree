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
#include "nsSVGGDIPlusGlyphGeometry.h"
#include "nsISVGRendererGlyphGeometry.h"
#include "nsISVGGDIPlusRenderContext.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGGDIPlusRegion.h"
#include "nsISVGRendererRegion.h"
#include "nsISVGPosGlyphGeometrySrc.h"
#include "nsPromiseFlatString.h"
#include "nsSVGGDIPlusGlyphMetrics.h"
#include "nsISVGGDIPlusGlyphMetrics.h"
#include "nsIPresContext.h"
#include "nsMemory.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGDIPlusGlyphGeometry class

class nsSVGGDIPlusGlyphGeometry : public nsISVGRendererGlyphGeometry
{
protected:
  friend nsresult NS_NewSVGGDIPlusGlyphGeometry(nsISVGRendererGlyphGeometry **result,
                                                nsISVGPositionedGlyphGeometrySource *src);

  nsSVGGDIPlusGlyphGeometry();
  ~nsSVGGDIPlusGlyphGeometry();
  nsresult Init(nsISVGPositionedGlyphGeometrySource* src);

public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererGlyphGeometry interface:
  NS_DECL_NSISVGRENDERERGLYPHGEOMETRY
  
protected:
  void GetGlobalTransform(Matrix *matrix);
  void UpdateStroke();
  void UpdateRegions(); // update covered region & hit-test region
  
  nsCOMPtr<nsISVGPositionedGlyphGeometrySource> mSource;
  nsCOMPtr<nsISVGRendererRegion> mCoveredRegion;
  Region *mHitTestRegion;
  GraphicsPath *mStroke;
};

//----------------------------------------------------------------------
// implementation:

nsSVGGDIPlusGlyphGeometry::nsSVGGDIPlusGlyphGeometry()
    : mStroke(nsnull), mHitTestRegion(nsnull)
{
  NS_INIT_ISUPPORTS();
}

nsSVGGDIPlusGlyphGeometry::~nsSVGGDIPlusGlyphGeometry()
{
  if (mStroke) delete mStroke;
  if (mHitTestRegion) delete mHitTestRegion;
}

nsresult
nsSVGGDIPlusGlyphGeometry::Init(nsISVGPositionedGlyphGeometrySource* src)
{
  mSource = src;
  return NS_OK;
}


nsresult
NS_NewSVGGDIPlusGlyphGeometry(nsISVGRendererGlyphGeometry **result,
                              nsISVGPositionedGlyphGeometrySource *src)
{
  *result = nsnull;
  
  nsSVGGDIPlusGlyphGeometry* gg = new nsSVGGDIPlusGlyphGeometry();
  if (!gg) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(gg);

  nsresult rv = gg->Init(src);

  if (NS_FAILED(rv)) {
    NS_RELEASE(gg);
    return rv;
  }
  
  *result = gg;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGGDIPlusGlyphGeometry)
NS_IMPL_RELEASE(nsSVGGDIPlusGlyphGeometry)

NS_INTERFACE_MAP_BEGIN(nsSVGGDIPlusGlyphGeometry)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererGlyphGeometry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// nsISVGRendererGlyphGeometry methods:

/* void render (in nsISVGRendererRenderContext ctx); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphGeometry::Render(nsISVGRendererRenderContext *ctx)
{
  nsCOMPtr<nsISVGGDIPlusRenderContext> gdiplusCtx = do_QueryInterface(ctx);
  NS_ASSERTION(gdiplusCtx, "wrong svg render context for geometry!");
  if (!gdiplusCtx) return NS_ERROR_FAILURE;
  gdiplusCtx->GetGraphics()->SetSmoothingMode(SmoothingModeAntiAlias);
  //gdiplusCtx->GetGraphics()->SetPixelOffsetMode(PixelOffsetModeHalf);
  nsCOMPtr<nsISVGGDIPlusGlyphMetrics> metrics;
  {
    nsCOMPtr<nsISVGRendererGlyphMetrics> xpmetrics;
    mSource->GetMetrics(getter_AddRefs(xpmetrics));
    metrics = do_QueryInterface(xpmetrics);
    NS_ASSERTION(metrics, "wrong metrics object!");
    if (!metrics) return NS_ERROR_FAILURE;
  }

  if (!mCoveredRegion) {
    nsCOMPtr<nsISVGRendererRegion> region;
    Update(UPDATEMASK_ALL, getter_AddRefs(region));
  }
  
  nscolor color;
  float opacity;
  PRUint16 type;
  
  // paint fill:
  mSource->GetFillPaintType(&type);

  if (type == nsISVGGeometrySource::PAINT_TYPE_SOLID_COLOR) {
    mSource->GetFillPaint(&color);
    mSource->GetFillOpacity(&opacity);
     
    SolidBrush brush(Color((BYTE)(opacity*255),NS_GET_R(color),NS_GET_G(color),NS_GET_B(color)));

    // prepare graphics object:
    GraphicsState state = gdiplusCtx->GetGraphics()->Save();
        
    Matrix m;
    GetGlobalTransform(&m);
    gdiplusCtx->GetGraphics()->MultiplyTransform(&m);
    gdiplusCtx->GetGraphics()->SetTextRenderingHint(metrics->GetTextRenderingHint());

    // paint:
    nsAutoString text;
    mSource->GetCharacterData(text);
    
    float x,y;
    mSource->GetX(&x);
    mSource->GetY(&y);

    StringFormat stringFormat(StringFormat::GenericTypographic());
    stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() |
                                StringFormatFlagsMeasureTrailingSpaces);
    stringFormat.SetLineAlignment(StringAlignmentNear);
    
    gdiplusCtx->GetGraphics()->DrawString(PromiseFlatString(text).get(), -1,
                                          metrics->GetFont(), PointF(x,y),
                                          &stringFormat, &brush);
#ifdef DEBUG
//  gdiplusCtx->GetGraphics()->TranslateTransform(x, y);
//  gdiplusCtx->GetGraphics()->DrawRectangle(&Pen(Color(255,0,0,0)), *(metrics->GetBoundingRect()));
#endif
    // restore state:
    gdiplusCtx->GetGraphics()->Restore(state);
  }

  // paint stroke
  mSource->GetStrokePaintType(&type);
  if (type == nsISVGGeometrySource::PAINT_TYPE_SOLID_COLOR && mStroke) {
    mSource->GetStrokePaint(&color);
    mSource->GetStrokeOpacity(&opacity);
    SolidBrush brush(Color((BYTE)(opacity*255), NS_GET_R(color), NS_GET_G(color), NS_GET_B(color)));
    gdiplusCtx->GetGraphics()->SetPageUnit(UnitWorld);
    gdiplusCtx->GetGraphics()->FillPath(&brush, mStroke);
  }
  
  return NS_OK;
}

/* nsISVGRendererRegion update (in unsigned long updatemask); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphGeometry::Update(PRUint32 updatemask, nsISVGRendererRegion **_retval)
{
  nsCOMPtr<nsISVGRendererRegion> regionBefore = mCoveredRegion;
  
  if ((updatemask & UPDATEMASK_FILL_PAINT) == updatemask) {
    mCoveredRegion = nsnull;
  }
  else if (updatemask != UPDATEMASK_NOTHING) {
    mCoveredRegion = nsnull;
    UpdateStroke();
  }

  if (!mCoveredRegion) {
    // either the coveredRegion might have changed or this is the
    // first update.
    
    UpdateRegions();
    if (regionBefore) {
      regionBefore->Combine(mCoveredRegion, _retval);
    }
    else {
      *_retval = mCoveredRegion;
      NS_IF_ADDREF(*_retval);
    }
  }
  else {
    // region hasn't changed
    *_retval = mCoveredRegion;
    NS_IF_ADDREF(*_retval);
  }    
      
  return NS_OK;
}

/* nsISVGRendererRegion getCoveredRegion (); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphGeometry::GetCoveredRegion(nsISVGRendererRegion **_retval)
{
  *_retval = mCoveredRegion;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

/* boolean containsPoint (in float x, in float y); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphGeometry::ContainsPoint(float x, float y, PRBool *_retval)
{
  *_retval = PR_FALSE;

   if (mHitTestRegion && mHitTestRegion->IsVisible(x,y)) {
     *_retval = PR_TRUE;
   }

  return NS_OK;
}

//----------------------------------------------------------------------
//

void
nsSVGGDIPlusGlyphGeometry::GetGlobalTransform(Matrix *matrix)
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
nsSVGGDIPlusGlyphGeometry::UpdateStroke()
{
  if (mStroke) {
    delete mStroke;
    mStroke = nsnull;
  }

  PRUint16 type;
  mSource->GetStrokePaintType(&type);
  if (type != nsISVGGeometrySource::PAINT_TYPE_SOLID_COLOR)
    return;

  float width;
  mSource->GetStrokeWidth(&width);
  
  if (width==0.0f) return;

  nsCOMPtr<nsISVGGDIPlusGlyphMetrics> metrics;
  {
    nsCOMPtr<nsISVGRendererGlyphMetrics> xpmetrics;
    mSource->GetMetrics(getter_AddRefs(xpmetrics));
    metrics = do_QueryInterface(xpmetrics);
    NS_ASSERTION(metrics, "wrong metrics object!");
    if (!metrics) return;
  }

  
  mStroke = new GraphicsPath();
  if (!mStroke) {
    NS_ERROR("couldn't construct graphicspath");
    return;
  }

  FontFamily fontFamily;
  metrics->GetFont()->GetFamily(&fontFamily);
  
  nsAutoString text;
  mSource->GetCharacterData(text);

  float x,y;
  mSource->GetX(&x);
  mSource->GetY(&y);

  
  mStroke->AddString(PromiseFlatString(text).get(), -1, &fontFamily, metrics->GetFont()->GetStyle(),
                     metrics->GetFont()->GetSize(),
                     PointF(x,y), StringFormat::GenericTypographic());
  
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
  
  // set miterlimit:
  float miterlimit;
  mSource->GetStrokeMiterlimit(&miterlimit);
  pen.SetMiterLimit(miterlimit);

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
  mStroke->Transform(&m);
//  mStroke->Outline();
}

void
nsSVGGDIPlusGlyphGeometry::UpdateRegions()
{
  if (mHitTestRegion) {
    delete mHitTestRegion;
    mHitTestRegion = nsnull;
  }

  nsCOMPtr<nsISVGGDIPlusGlyphMetrics> metrics;
  {
    nsCOMPtr<nsISVGRendererGlyphMetrics> xpmetrics;
    mSource->GetMetrics(getter_AddRefs(xpmetrics));
    metrics = do_QueryInterface(xpmetrics);
    NS_ASSERTION(metrics, "wrong metrics object!");
    if (!metrics) return;
  }
  
  // the hit-test region is the bounding box transformed into our
  // local coord system:
  mHitTestRegion = new Region(*metrics->GetBoundingRect());

  if (!mHitTestRegion) {
    NS_ERROR("could not construct region");
    return;
  }
  
  float x,y;
  mSource->GetX(&x);
  mSource->GetY(&y);
  
  mHitTestRegion->Translate(x, y);

  Matrix m;
  GetGlobalTransform(&m);
  
  mHitTestRegion->Transform(&m);

  
  // clone the covered region from the hit-test region:

  nsCOMPtr<nsIPresContext> presContext;
  mSource->GetPresContext(getter_AddRefs(presContext));

  NS_NewSVGGDIPlusClonedRegion(getter_AddRefs(mCoveredRegion),
                               mHitTestRegion,
                               presContext);
}

