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

#include "membufFontMetrics.h"
#include "nsFont.h"

#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsString.h"

#define FT_FLOOR(X) ((X & -64) >> 6)
#define FT_CEIL(X)  (((X + 63) & -64) >> 6)

static FT_Library ftlib = nsnull;

NS_IMPL_ISUPPORTS1(membufFontMetrics, nsIFontMetrics)

membufFontMetrics::membufFontMetrics() :
    mFont(nsnull),
    mMaxAscent(0),
    mMaxDescent(0),
    mMaxAdvance(0),
    mUnderlineOffset(0),
    mUnderlineHeight(0)
{
    NS_INIT_ISUPPORTS();
    if (!ftlib) {
        FT_Init_FreeType(&ftlib);
    }
}

membufFontMetrics::~membufFontMetrics()
{
    FT_Done_Face(mFace);
    delete mFont;
}

/*
Vera.ttf    VeraBd.ttf  VeraMoBI.ttf  VeraMoIt.ttf  VeraSe.ttf    fonts.cache-1
VeraBI.ttf  VeraIt.ttf  VeraMoBd.ttf  VeraMono.ttf  VeraSeBd.ttf
*/

#define NORMAL_FONT             "VeraSe.ttf"
#define BOLD_FONT               "VeraSeBd.ttf"
#define ITALIC_FONT             "VeraIt.ttf"
#define BOLD_ITALIC_FONT        "VeraBI.ttf"
#define MONO_FONT               "VeraMono.ttf"
#define MONO_BOLD_FONT          "VeraMoBd.ttf"
#define MONO_ITALIC_FONT        "VeraMoIt.ttf"
#define MONO_BOLD_ITALIC_FONT   "VeraMoBI.ttf"

static char *
GetFontPath(bool isitalic, bool isbold)
{
    nsCOMPtr<nsIFile> aFile;
    // XXXjgaunt - requirs the fonts to be installed in the process dir.
    //             might consider creating a font directory within the user
    //             profile dir instead. Or using Freetype's ability to find
    //             system fonts on it won (if that exists).
    NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, getter_AddRefs(aFile));

    if(isitalic) {
        if(isbold) aFile->Append(NS_LITERAL_STRING(BOLD_ITALIC_FONT));
        else aFile->Append(NS_LITERAL_STRING(ITALIC_FONT));
    } else {
        if(isbold) aFile->Append(NS_LITERAL_STRING(BOLD_FONT));
        else aFile->Append(NS_LITERAL_STRING(NORMAL_FONT));
    }
    nsAutoString pathBuf;
    aFile->GetPath(pathBuf);
    NS_LossyConvertUCS2toASCII pathCBuf(pathBuf);

    return strdup(pathCBuf.get());
}

NS_IMETHODIMP
membufFontMetrics::Init(const nsFont& aFont,
                        nsIAtom* aLangGroup,
                        nsIDeviceContext *aContext)
{
    mFont = new nsFont(aFont);
    mLangGroup = aLangGroup;

    mDeviceContext = aContext;

    mDev2App = mDeviceContext->DevUnitsToAppUnits();

    float app2dev = mDeviceContext->AppUnitsToDevUnits();

    mPixelSize = NSToIntRound(app2dev * mFont->size);

    mStretchIndex = 4; // normal

    bool isitalic = (mFont->style != NS_FONT_STYLE_NORMAL);
    bool isbold = (mFont->weight > NS_FONT_WEIGHT_NORMAL);

    char *fontPath = GetFontPath(isitalic, isbold);
    //printf("membufFontMetrics: Loading font from path %s\n", fontPath);
    int r = FT_New_Face(ftlib, fontPath, 0, &mFace);

    if(r != 0) {
        printf("Loading font from %s failed!\n", fontPath);
        return NS_ERROR_FILE_INVALID_PATH;
    }

    free(fontPath);

    FT_Set_Char_Size(mFace, 0, mPixelSize << 6, 72, 72);

    mMaxAscent  = mFace->bbox.yMax;
    mMaxDescent = mFace->bbox.yMin;
    mMaxAdvance = mFace->max_advance_width;

    mUnderlineOffset = mFace->underline_position;
    mUnderlineHeight = mFace->underline_thickness;

    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::Destroy()
{
    delete mFont;
    mFont = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetXHeight(nscoord& aResult)
{
    aResult = NSToCoordRound(14 * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetSuperscriptOffset(nscoord& aResult)
{
    aResult = 0;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetSubscriptOffset(nscoord& aResult)
{
    aResult = 0;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
    aOffset = 0;
    aSize = NSToCoordRound(1 * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
    const FT_Fixed scale = mFace->size->metrics.y_scale;

    aOffset = FT_FLOOR(FT_MulFix(mUnderlineOffset, scale));
    aOffset = NSToCoordRound(aOffset * mDev2App);

    aSize = FT_CEIL(FT_MulFix(mUnderlineHeight, scale));
    if (aSize > 1)
        aSize = 1;
    aSize = NSToCoordRound(aSize * mDev2App);

    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetHeight(nscoord &aHeight)
{
    aHeight = NSToCoordRound((mFace->size->metrics.height >> 6) * mDev2App);

    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetInternalLeading(nscoord &aLeading)
{
    aLeading = 0 * mDev2App;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetExternalLeading(nscoord &aLeading)
{
    aLeading = 0 * mDev2App;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetEmHeight(nscoord &aHeight)
{
    return GetHeight(aHeight);
}

NS_IMETHODIMP
membufFontMetrics::GetEmAscent(nscoord &aAscent)
{
    return GetMaxAscent(aAscent);
}

NS_IMETHODIMP
membufFontMetrics::GetEmDescent(nscoord &aDescent)
{
    return GetMaxDescent(aDescent);
}

NS_IMETHODIMP
membufFontMetrics::GetMaxHeight(nscoord &aHeight)
{
    /* ascent + descent */
    aHeight = FT_CEIL(FT_MulFix(mMaxAscent, mFace->size->metrics.y_scale))
              - FT_CEIL(FT_MulFix(mMaxDescent, mFace->size->metrics.y_scale))
              + 1;

    aHeight = NSToCoordRound(aHeight * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetMaxAscent(nscoord &aAscent)
{
    /* units above the base line */
    aAscent = FT_CEIL(FT_MulFix(mMaxAscent, mFace->size->metrics.y_scale));
    aAscent = NSToCoordRound(aAscent * mDev2App);

    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetMaxDescent(nscoord &aDescent)
{
    /* units below the base line */
    aDescent = -FT_CEIL(FT_MulFix(mMaxDescent, mFace->size->metrics.y_scale));
    aDescent = NSToCoordRound(aDescent * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetMaxAdvance(nscoord &aAdvance)
{
    aAdvance = FT_CEIL(FT_MulFix(mMaxAdvance, mFace->size->metrics.x_scale));
    aAdvance = NSToCoordRound(aAdvance * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetFont(const nsFont *&aFont)
{
    aFont = mFont;
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetLangGroup(nsIAtom** aLangGroup)
{
    *aLangGroup = mLangGroup;
    NS_IF_ADDREF(*aLangGroup);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetFontHandle(nsFontHandle &aHandle)
{
    aHandle = (nsFontHandle)mFace;
}

NS_IMETHODIMP
membufFontMetrics::GetAveCharWidth(nscoord& aAveCharWidth)
{
    aAveCharWidth = NSToCoordRound(7 * mDev2App);
    return NS_OK;
}

NS_IMETHODIMP
membufFontMetrics::GetSpaceWidth(nscoord& aSpaceCharWidth)
{
    FT_Load_Char(mFace, ' ', FT_LOAD_DEFAULT | FT_LOAD_NO_AUTOHINT);

    aSpaceCharWidth = NSToCoordRound((mFace->glyph->advance.x >> 6) * mDev2App);

    return NS_OK;
}

nscoord
membufFontMetrics::MeasureString(const char *aString, PRUint32 aLength)
{
    nscoord width = 0;
    PRUint32 i;
    int error;

    FT_UInt glyph_index;
    FT_UInt previous = 0;

    for (i=0; i < aLength; ++i) {
        glyph_index = FT_Get_Char_Index(mFace, aString[i]);
/*
        if (previous && glyph_index) {
            FT_Vector delta;
            FT_Get_Kerning(mFace, previous, glyph_index,
                           FT_KERNING_DEFAULT, &delta);
            width += delta.x >> 6;
        }
*/
        error = FT_Load_Glyph(mFace, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_AUTOHINT);
        if (error)
            continue;

        width += mFace->glyph->advance.x;
        previous = glyph_index;
    }

    return NSToCoordRound((width >> 6) * mDev2App);
}

nscoord
membufFontMetrics::MeasureString(const PRUnichar *aString, PRUint32 aLength)
{
    nscoord width = 0;
    PRUint32 i;
    int error;

    FT_UInt glyph_index;
    FT_UInt previous = 0;

    for (i=0; i < aLength; ++i) {
        glyph_index = FT_Get_Char_Index(mFace, aString[i]);
/*
        if (previous && glyph_index) {
            FT_Vector delta;
            FT_Get_Kerning(mFace, previous, glyph_index,
                           FT_KERNING_DEFAULT, &delta);
            width += delta.x >> 6;
        }
*/
        error = FT_Load_Glyph(mFace, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_AUTOHINT);
        if (error)
            continue;

        width += mFace->glyph->advance.x;
        previous = glyph_index;
    }

    return NSToCoordRound((width >> 6) * mDev2App);
}

NS_IMETHODIMP
membufFontMetrics::GetLeading(nscoord& aLeading) {
    aLeading = 0;
    return NS_OK;
}
NS_IMETHODIMP
membufFontMetrics::GetNormalLineHeight(nscoord& aHeight){
    return GetMaxHeight(aHeight);
}

