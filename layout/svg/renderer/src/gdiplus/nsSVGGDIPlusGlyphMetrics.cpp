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

// unknwn.h is needed to build with WIN32_LEAN_AND_MEAN
#include <unknwn.h>

#include <Gdiplus.h>
using namespace Gdiplus;

#include "nsCOMPtr.h"
#include "nsISVGGlyphMetricsSource.h"
#include "nsPromiseFlatString.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"
#include "nsIPresContext.h"
#include "nsDeviceContextWin.h"
#include "nsISVGGDIPlusGlyphMetrics.h"
#include "nsSVGGDIPlusGlyphMetrics.h"
#include "float.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGRect.h"
#include "nsSVGTypeCIDs.h"
#include "nsIComponentManager.h"

////////////////////////////////////////////////////////////////////////
// nsWindowsDC helper class

class nsWindowsDC {
public:
  nsWindowsDC(nsIPresContext* presContext);
  ~nsWindowsDC();
  operator HDC() { return mHDC; }
private:
  bool isWndDC;
  HWND mWND;
  HDC mHDC;
};

nsWindowsDC::nsWindowsDC(nsIPresContext* presContext)
{
  nsCOMPtr<nsIDeviceContext> devicecontext;
  presContext->GetDeviceContext(getter_AddRefs(devicecontext));
  NS_ASSERTION(devicecontext, "no device context");

  isWndDC=((nsDeviceContextWin *)(devicecontext.get()))->mDC==nsnull;
  if (isWndDC) {
    mWND = (HWND)((nsDeviceContextWin *)(devicecontext.get()))->mWidget;
    NS_ASSERTION(mWND, "no window and no handle in devicecontext (continuing with screen dc)");
    mHDC = ::GetDC(mWND);
  }
  else
    mHDC = ((nsDeviceContextWin *)(devicecontext.get()))->mDC;
}

nsWindowsDC::~nsWindowsDC()
{
  if (isWndDC)
    ::ReleaseDC(mWND, mHDC);
}

////////////////////////////////////////////////////////////////////////
// nsSVGGDIPlusGlyphMetrics class

class nsSVGGDIPlusGlyphMetrics : public nsISVGGDIPlusGlyphMetrics
{
protected:
  friend nsresult NS_NewSVGGDIPlusGlyphMetrics(nsISVGRendererGlyphMetrics **result,
                                               nsISVGGlyphMetricsSource *src);
  nsSVGGDIPlusGlyphMetrics(nsISVGGlyphMetricsSource *src);
  ~nsSVGGDIPlusGlyphMetrics();
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererGlyphMetrics interface:
  NS_DECL_NSISVGRENDERERGLYPHMETRICS

  // nsISVGGDIPlusGlyphMetrics interface:
  NS_IMETHOD_(const RectF*) GetBoundingRect();
  NS_IMETHOD_(void) GetSubBoundingRect(PRUint32 charoffset, PRUint32 count, RectF* retval);
  NS_IMETHOD_(const Font*) GetFont();
  NS_IMETHOD_(TextRenderingHint) GetTextRenderingHint();
  
protected:
  void MarkRectForUpdate() { mRectNeedsUpdate = PR_TRUE; }
  void ClearFontInfo() { if (mFont) { delete mFont; mFont = nsnull; }}
  void InitializeFontInfo();
  void GetGlobalTransform(Matrix *matrix);
  void PrepareGraphics(Graphics &g);
  float GetPixelScale();
  
private:
  PRBool mRectNeedsUpdate;
  RectF mRect;
  Font *mFont;
  nsCOMPtr<nsISVGGlyphMetricsSource> mSource;
  
};

//----------------------------------------------------------------------
// implementation:

nsSVGGDIPlusGlyphMetrics::nsSVGGDIPlusGlyphMetrics(nsISVGGlyphMetricsSource *src)
    : mRectNeedsUpdate(PR_TRUE), mFont(nsnull), mSource(src)
{
}

nsSVGGDIPlusGlyphMetrics::~nsSVGGDIPlusGlyphMetrics()
{
  ClearFontInfo();
}

nsresult
NS_NewSVGGDIPlusGlyphMetrics(nsISVGRendererGlyphMetrics **result,
                             nsISVGGlyphMetricsSource *src)
{
  *result = new nsSVGGDIPlusGlyphMetrics(src);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*result);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGGDIPlusGlyphMetrics)
NS_IMPL_RELEASE(nsSVGGDIPlusGlyphMetrics)

NS_INTERFACE_MAP_BEGIN(nsSVGGDIPlusGlyphMetrics)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererGlyphMetrics)
  NS_INTERFACE_MAP_ENTRY(nsISVGGDIPlusGlyphMetrics)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGRendererGlyphMetrics methods:

/* float getBaselineOffset (in unsigned short baselineIdentifier); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::GetBaselineOffset(PRUint16 baselineIdentifier, float *_retval)
{
  if (!GetFont()) {
    NS_ERROR("no font");
    return NS_ERROR_FAILURE;
  }
  NS_ASSERTION(GetFont()->GetUnit()==UnitWorld, "font unit is not in world units");

  switch (baselineIdentifier) {
    case BASELINE_TEXT_BEFORE_EDGE:
      *_retval = GetBoundingRect()->Y;
      break;
    case BASELINE_TEXT_AFTER_EDGE:
      *_retval = (float)(UINT16)(GetBoundingRect()->Y + GetBoundingRect()->Height + 0.5);
      break;
    case BASELINE_CENTRAL:
    case BASELINE_MIDDLE:
      *_retval = (float)(UINT16)(GetBoundingRect()->Y + GetBoundingRect()->Height/2.0 + 0.5);
      break;
    case BASELINE_ALPHABETIC:
    default:
    {
      FontFamily family;
      GetFont()->GetFamily(&family);
      INT style = GetFont()->GetStyle();
      // alternatively to rounding here, we could set the
      // pixeloffsetmode to 'PixelOffsetModeHalf' on painting
      *_retval = (float)(UINT16)(GetBoundingRect()->Y
                                 + GetFont()->GetSize()
                                   *family.GetCellAscent(style)/family.GetEmHeight(style)
                                 + 0.5);
#ifdef DEBUG
//       printf("Glyph Metrics:\n"
//              "--------------\n"
//              "ascent:%d\n"
//              "boundingheight:%f\n"
//              "fontheight:%f\n"
//              "fontsize:%f\n"
//              "emheight:%d\n"
//              "linespacing:%d\n"
//              "calculated offset:%f\n"
//              "descent:%d\n"
//              "fh-bh:%f\n"
//              "fontsize/emheight:%f\n"
//              "fontheight/linespacing:%f\n"
//              "boundingheight/(ascent+descent):%f\n"
//              "boundingbox.Y:%f\n"
//              "--------------\n",
//              family.GetCellAscent(style),
//              GetBoundingRect()->Height,
//              GetFontHeight(),
//              GetFont()->GetSize(),
//              family.GetEmHeight(style),
//              family.GetLineSpacing(style),
//              *_retval,
//              family.GetCellDescent(style),
//              GetFontHeight()-GetBoundingRect()->Height,
//              GetFont()->GetSize()/family.GetEmHeight(style),
//              GetFontHeight()/family.GetLineSpacing(style),
//              GetBoundingRect()->Height/(family.GetCellAscent(style)+family.GetCellDescent(style)),
//              GetBoundingRect()->Y);
#endif
    }
    break;
  }
  
  return NS_OK;
}


/* readonly attribute float advance; */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::GetAdvance(float *aAdvance)
{
  // XXX
  *aAdvance = GetBoundingRect()->Width;
  return NS_OK;
}

/* readonly attribute nsIDOMSVGRect boundingBox; */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::GetBoundingBox(nsIDOMSVGRect * *aBoundingBox)
{
  *aBoundingBox = nsnull;

  nsCOMPtr<nsIDOMSVGRect> rect = do_CreateInstance(NS_SVGRECT_CONTRACTID);

  NS_ASSERTION(rect, "could not create rect");
  if (!rect) return NS_ERROR_FAILURE;
  
  rect->SetX(GetBoundingRect()->X);
  rect->SetY(GetBoundingRect()->Y);
  rect->SetWidth(GetBoundingRect()->Width);
  rect->SetHeight(GetBoundingRect()->Height);

  *aBoundingBox = rect;
  NS_ADDREF(*aBoundingBox);
  
  return NS_OK;
}

/* [noscript] nsIDOMSVGRect getExtentOfChar (in unsigned long charnum); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  RectF bounds;
  GetSubBoundingRect(charnum, 1, &bounds);
  
  nsCOMPtr<nsIDOMSVGRect> rect = do_CreateInstance(NS_SVGRECT_CONTRACTID);

  NS_ASSERTION(rect, "could not create rect");
  if (!rect) return NS_ERROR_FAILURE;
  
  rect->SetX(bounds.X);
  rect->SetY(bounds.Y);
  rect->SetWidth(bounds.Width);
  rect->SetHeight(bounds.Height);

  *_retval = rect;
  NS_ADDREF(*_retval);
  
  return NS_OK;
}

/* boolean update (in unsigned long updatemask); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::Update(PRUint32 updatemask, PRBool *_retval)
{
  *_retval = PR_FALSE;
  
  if (updatemask & nsISVGGlyphMetricsSource::UPDATEMASK_CHARACTER_DATA) {
    MarkRectForUpdate();
    *_retval = PR_TRUE;
  }

  if (updatemask & nsISVGGlyphMetricsSource::UPDATEMASK_FONT) {
    ClearFontInfo();
    MarkRectForUpdate();
    *_retval = PR_TRUE;
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGGDIPlusGlyphMetrics methods:

NS_IMETHODIMP_(const RectF*)
nsSVGGDIPlusGlyphMetrics::GetBoundingRect()
{
  if (!mRectNeedsUpdate) return &mRect;
  mRectNeedsUpdate = PR_FALSE;
  
  nsAutoString text;
  mSource->GetCharacterData(text);

  GetSubBoundingRect(0, text.Length(), &mRect);

  return &mRect;

}

NS_IMETHODIMP_(void)
nsSVGGDIPlusGlyphMetrics::GetSubBoundingRect(PRUint32 charoffset, PRUint32 count,
                                             RectF* retval)
{
  nsCOMPtr<nsIPresContext> presContext;
  mSource->GetPresContext(getter_AddRefs(presContext));
  NS_ASSERTION(presContext, "null prescontext");

  nsWindowsDC devicehandle(presContext);
  Graphics graphics(devicehandle);
  PrepareGraphics(graphics);
  
  nsAutoString text;
  mSource->GetCharacterData(text);
  
  StringFormat stringFormat(StringFormat::GenericTypographic());
  stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() |
                              StringFormatFlagsMeasureTrailingSpaces);
  
  CharacterRange charRange(charoffset, count);
  stringFormat.SetMeasurableCharacterRanges(1, &charRange);
  
  Region region;
  region.MakeEmpty();
  
  // we measure in the transformed coordinate system...
  GraphicsState state = graphics.Save();
  
  Matrix m;
  GetGlobalTransform(&m);
  graphics.MultiplyTransform(&m);
  
  graphics.MeasureCharacterRanges(PromiseFlatString(text).get(), -1, GetFont(),
                                  RectF(0.0f, 0.0f, FLT_MAX, FLT_MAX), &stringFormat, 1, &region);
  
  graphics.Restore(state);
  
  // ... and obtain the bounds in our local coord system
  region.GetBounds(retval, &graphics);  
}

NS_IMETHODIMP_(const Font*)
nsSVGGDIPlusGlyphMetrics::GetFont()
{
  InitializeFontInfo();
  return mFont;
}

NS_IMETHODIMP_(TextRenderingHint)
nsSVGGDIPlusGlyphMetrics::GetTextRenderingHint()
{
  // when the text is stroked, we have to turn off hinting so that
  // stroke and fill match up exactely:
  bool forceUnhinted = PR_FALSE;
  {
    PRUint16 type;
    mSource->GetStrokePaintType(&type);
    forceUnhinted = (type != nsISVGGeometrySource::PAINT_TYPE_NONE); 
  }

  PRUint16 textRendering;
  mSource->GetTextRendering(&textRendering);
  switch (textRendering) {
    case nsISVGGlyphMetricsSource::TEXT_RENDERING_OPTIMIZESPEED:
      return TextRenderingHintSingleBitPerPixel;
      break;
    case nsISVGGlyphMetricsSource::TEXT_RENDERING_OPTIMIZELEGIBILITY:
      return forceUnhinted ?
        TextRenderingHintAntiAlias :
        TextRenderingHintAntiAliasGridFit;
      break;
    case nsISVGGlyphMetricsSource::TEXT_RENDERING_GEOMETRICPRECISION:
    case nsISVGGlyphMetricsSource::TEXT_RENDERING_AUTO:
    default:
      return TextRenderingHintAntiAlias;
      break;
  }
}

void
nsSVGGDIPlusGlyphMetrics::InitializeFontInfo()
{
  if (mFont) return; // already initialized

  nsCOMPtr<nsIPresContext> presContext;
  mSource->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    NS_ERROR("null prescontext");
    return;
  }
  
  nsFontHandle fonthandle;
  {
    nsFont font;
    mSource->GetFont(&font);

#ifdef DEBUG
//    printf("font.name=%s\n", NS_ConvertUCS2toUTF8(font.name).get());
#endif

    // XXX The size of the font is in device units. Since we are going
    // to use the font in a logical pixel coordinate system with dev
    // unit = pixel*scale, we need to devide the size by scale:
    font.size/=GetPixelScale();

    nsCOMPtr<nsIFontMetrics> metrics;

    presContext->GetMetricsFor(font, getter_AddRefs(metrics));
    if (!metrics) {
      NS_ERROR("could not get fontmetrics");
      return;
    }
    
    metrics->GetFontHandle(fonthandle);
  }

  mFont = new Font(nsWindowsDC(presContext), (HFONT)fonthandle);
  NS_ASSERTION(mFont->IsAvailable(),"font not available");
}

void
nsSVGGDIPlusGlyphMetrics::GetGlobalTransform(Matrix *matrix)
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
nsSVGGDIPlusGlyphMetrics::PrepareGraphics(Graphics &g)
{
  g.SetPageUnit(UnitPixel);
  // XXX for some reason the Graphics object that we derive from our
  // measurement dcs is never correctly scaled for devices like
  // printers. Hence we scale manually here:
  g.SetPageScale(GetPixelScale());
  g.SetSmoothingMode(SmoothingModeAntiAlias);
  //g.SetPixelOffsetMode(PixelOffsetModeHalf);
  g.SetTextRenderingHint(GetTextRenderingHint());
}

float
nsSVGGDIPlusGlyphMetrics::GetPixelScale()
{
  nsCOMPtr<nsIPresContext> presContext;
  mSource->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    NS_ERROR("null prescontext");
    return 1.0f;
  }

  nsCOMPtr<nsIDeviceContext> devicecontext;
  presContext->GetDeviceContext(getter_AddRefs(devicecontext));

  float scale;
  devicecontext->GetCanonicalPixelScale(scale);
  return scale;
}  
