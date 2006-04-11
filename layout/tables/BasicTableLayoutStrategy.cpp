/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// vim:cindent:ts=4:et:sw=4:
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla's table layout code.
 *
 * The Initial Developer of the Original Code is the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   L. David Baron <dbaron@dbaron.org> (original author)
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
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * Web-compatible algorithms that determine column and table widths,
 * used for CSS2's 'table-layout: auto'.
 */

#include "BasicTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsLayoutUtils.h"

BasicTableLayoutStrategy::BasicTableLayoutStrategy(nsTableFrame *aTableFrame)
  : mTableFrame(aTableFrame)
{
}

/* virtual */
BasicTableLayoutStrategy::~BasicTableLayoutStrategy()
{
}

/* virtual */ nscoord
BasicTableLayoutStrategy::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aRenderingContext);
    return mMinWidth;
}

/* virtual */ nscoord
BasicTableLayoutStrategy::GetPrefWidth(nsIRenderingContext* aRenderingContext)
{
    DISPLAY_PREF_WIDTH(mTableFrame, mPrefWidth);
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aRenderingContext);
    return mPrefWidth;
}

void
BasicTableLayoutStrategy::ComputeIntrinsicWidths(nsIRenderingContext* aRenderingContext)
{
    mTableFrame->ComputeColumnIntrinsicWidths(aRenderingContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    nscoord min = 0, pref = 0, pref_pct = 0;

    for (PRInt32 col = 0, col_end = cellMap->GetColCount();
         col < col_end; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        min += colFrame->GetMinCoord();
        pref += colFrame->GetPrefCoord();

        // Percentages are of the table, so we have to reverse them for
        // intrinsic widths.
        float p = colFrame->GetPrefPercent();
        if (0.0f < p && p < 1.0f) {
            float new_pref_pct = colFrame->GetPrefCoord() / (1.0f - p);
            if (new_pref_pct > pref_pct)
                pref_pct = new_pref_pct;
        }
    }

    if (pref_pct > pref)
        pref = pref_pct;

    float p2t = mTableFrame->GetPresContext()->PixelsToTwips();
    min = nsTableFrame::RoundToPixel(min, p2t);
    pref = nsTableFrame::RoundToPixel(pref, p2t);

    if (mTableFrame->IsBorderCollapse()) {
        // subtract these so the caller can add them back again!
        min -= nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                 mTableFrame, nsLayoutUtils::MIN_WIDTH,
                 nsLayoutUtils::IntrinsicWidthPart(nsLayoutUtils::BORDER |
                                                   nsLayoutUtils::PADDING));
        pref -= nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                  mTableFrame, nsLayoutUtils::PREF_WIDTH,
                  nsLayoutUtils::IntrinsicWidthPart(nsLayoutUtils::BORDER |
                                                    nsLayoutUtils::PADDING));
    }

    mMinWidth = min;
    mPrefWidth = pref;
}

/* virtual */ void
BasicTableLayoutStrategy::MarkIntrinsicWidthsDirty()
{
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
}

/* virtual */ void
BasicTableLayoutStrategy::CalcColumnWidths(const nsHTMLReflowState& aReflowState)
{
    // XXX Is this needed?
    NS_ASSERTION((mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aReflowState.rendContext);

    // XXX Does mComputedWidth include border and padding?
    nscoord width = aReflowState.mComputedWidth;
    nsTableCellMap *cellMap = mTableFrame->GetCellMap();

    float p2t = mTableFrame->GetPresContext()->PixelsToTwips();

    // Hold previous to avoid accumulating rounding error.
    nscoord prev_x = 0, prev_x_round = 0;

    union {
        float f;
        nscoord d;
        nscoord e;
    } u;
    if (mPrefWidth != mMinWidth) {
        u.f = float(width - mMinWidth) / float(mPrefWidth - mMinWidth);
    } else if (mMinWidth != 0) {
        u.d = width - mMinWidth;
    } else {
        u.e = width / cellMap->GetColCount();
    }

    for (PRInt32 col = 0, col_end = cellMap->GetColCount();
         col < col_end; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);

        nscoord colWidth = colFrame->GetMinCoord();
        if (mPrefWidth != mMinWidth) {
            colWidth += nscoord(u.f * (colFrame->GetPrefCoord() -
                                       colFrame->GetMinCoord()));
        } else if (mMinWidth != 0) {
            colWidth += u.d * colFrame->GetMinCoord() / mMinWidth;
        } else {
            colWidth += u.e;
            NS_ASSERTION(colFrame->GetMinCoord() == 0, "yikes");
        }

        nscoord new_x = prev_x + colWidth;
        nscoord new_x_round = nsTableFrame::RoundToPixel(new_x, p2t);

        colFrame->SetFinalWidth(new_x_round - prev_x_round);

        prev_x = new_x;
        prev_x_round = new_x_round;
    }
}
