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
#include "nsISVGGlyphGeometrySource.h"
#include "nsPromiseFlatString.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"
#include "nsIPresContext.h"
#include "nsDeviceContextWin.h"
#include "nsISVGGDIPlusGlyphMetrics.h"
#include "nsSVGGDIPlusGlyphMetrics.h"
#include "float.h"
#include "nsIDOMSVGMatrix.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGDIPlusGlyphMetrics class

class nsSVGGDIPlusGlyphMetrics : public nsISVGGDIPlusGlyphMetrics
{
protected:
  friend nsresult NS_NewSVGGDIPlusGlyphMetrics(nsISVGRendererGlyphMetrics **result,
                                               nsISVGGlyphGeometrySource *src);
  nsSVGGDIPlusGlyphMetrics(nsISVGGlyphGeometrySource *src);
  ~nsSVGGDIPlusGlyphMetrics();
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererGlyphMetrics interface:
  NS_DECL_NSISVGRENDERERGLYPHMETRICS

  // nsISVGGDIPlusGlyphMetrics interface:
  NS_IMETHOD_(const RectF*) GetBoundingRect();
  NS_IMETHOD_(const Font*) GetFont();
  NS_IMETHOD_(TextRenderingHint) GetTextRenderingHint();
  
protected:
  void MarkRectForUpdate() { mRectNeedsUpdate = PR_TRUE; }
  void ClearFontInfo() { if (mFont) { delete mFont; mFont = nsnull; }}
  void InitializeFontInfo();
  float GetFontHeight() { InitializeFontInfo(); return mFontHeight; }
  void GetGlobalTransform(Matrix *matrix);
  
private:
  PRBool mRectNeedsUpdate;
  RectF mRect;
  Font *mFont;
  float mFontHeight;
  nsCOMPtr<nsISVGGlyphGeometrySource> mSource;
  
};

//----------------------------------------------------------------------
// implementation:

nsSVGGDIPlusGlyphMetrics::nsSVGGDIPlusGlyphMetrics(nsISVGGlyphGeometrySource *src)
    : mRectNeedsUpdate(PR_TRUE), mFont(nsnull), mSource(src)
{
  NS_INIT_ISUPPORTS();
}

nsSVGGDIPlusGlyphMetrics::~nsSVGGDIPlusGlyphMetrics()
{
  ClearFontInfo();
}

nsresult
NS_NewSVGGDIPlusGlyphMetrics(nsISVGRendererGlyphMetrics **result,
                             nsISVGGlyphGeometrySource *src)
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

/* boolean update (in unsigned long updatemask); */
NS_IMETHODIMP
nsSVGGDIPlusGlyphMetrics::Update(PRUint32 updatemask, PRBool *_retval)
{
  *_retval = PR_FALSE;
  
  if (updatemask & UPDATEMASK_CHARACTER_DATA) {
    MarkRectForUpdate();
    *_retval = PR_TRUE;
  }

  if (updatemask & UPDATEMASK_FONT) {
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

  nsCOMPtr<nsIPresContext> presContext;
  mSource->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    NS_ERROR("null prescontext");
    return &mRect;
  }

  HWND win;
  HDC devicehandle;
  {
    nsCOMPtr<nsIDeviceContext> devicecontext;
    presContext->GetDeviceContext(getter_AddRefs(devicecontext));
    // this better be what we think it is:
    win = (HWND)((nsDeviceContextWin *)(devicecontext.get()))->mWidget;
    devicehandle = ::GetDC(win);
  }

  {
    // wrap this in a block so that the Graphics-object goes out of
    // scope before we release the HDC.
    
    Graphics graphics(devicehandle);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    //graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

    graphics.SetTextRenderingHint(GetTextRenderingHint());
    
    nsAutoString text;
    mSource->GetCharacterData(text);

    StringFormat stringFormat(StringFormat::GenericTypographic());
    stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() |
                                StringFormatFlagsMeasureTrailingSpaces);
    
//     graphics.MeasureString(PromiseFlatString(text).get(), -1, GetFont(),
//                            PointF(0.0f, 0.0f),&stringFormat, &mRect);
    
    CharacterRange charRange(0, text.Length());
    stringFormat.SetMeasurableCharacterRanges(1, &charRange);

    Region region;

    // we measure in the transformed coordinate system...
    GraphicsState state = graphics.Save();
    Matrix m;
    GetGlobalTransform(&m);
    graphics.MultiplyTransform(&m);

    graphics.MeasureCharacterRanges(PromiseFlatString(text).get(), -1, GetFont(),
                                    RectF(0.0f, 0.0f, FLT_MAX, FLT_MAX), &stringFormat, 1, &region);

    graphics.Restore(state);
    
    // ... and obtain the bounds in our local coord system
    region.GetBounds(&mRect, &graphics);
    
  }

  ::ReleaseDC(win, devicehandle);

  
  return &mRect;
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
  PRUint16 type;
  mSource->GetStrokePaintType(&type);

  if (type != nsISVGGeometrySource::PAINT_TYPE_NONE)
    return TextRenderingHintAntiAlias;
  
  return TextRenderingHintAntiAliasGridFit;
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

  HWND win;
  HDC devicehandle;
  {
    nsCOMPtr<nsIDeviceContext> devicecontext;
    presContext->GetDeviceContext(getter_AddRefs(devicecontext));
    // this better be what we think it is:
    win = (HWND)((nsDeviceContextWin *)(devicecontext.get()))->mWidget;
    devicehandle = ::GetDC(win);
  }
  
  nsFontHandle fonthandle;
  {
    nsFont font;
    mSource->GetFont(&font);
#ifdef DEBUG
//    printf("font.name=%s\n", NS_ConvertUCS2toUTF8(font.name).get());
#endif
    nsCOMPtr<nsIFontMetrics> metrics;
    presContext->GetMetricsFor(font, getter_AddRefs(metrics));
    if (!metrics) {
      NS_ERROR("could not get fontmetrics");
      return;
    }
    
    metrics->GetFontHandle(fonthandle);
  }

  mFont = new Font(devicehandle, (HFONT)fonthandle);
  NS_ASSERTION(mFont->IsAvailable(),"font not available");

  {
    // wrap this in a block so that the Graphics-object goes out of
    // scope before we release the HDC.
    
    Graphics graphics(devicehandle);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    //graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
    graphics.SetTextRenderingHint(GetTextRenderingHint());
    Matrix m;
    GetGlobalTransform(&m);
    graphics.MultiplyTransform(&m);

    mFontHeight = mFont->GetHeight(&graphics);
  }
  
  ::ReleaseDC(win, devicehandle);
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




