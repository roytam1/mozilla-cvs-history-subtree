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
    MarkIntrinsicWidthsDirty();
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
    nscoord min = 0, pref = 0, max_small_pct_pref = 0, pct_running_nonpct = 0;
    float pct_running_pct = 0.0f;
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        min += colFrame->GetMinCoord() + spacing;
        pref += colFrame->GetPrefCoord() + spacing;

        // Percentages are of the table, so we have to reverse them for
        // intrinsic widths.
        float p = colFrame->GetPrefPercent();
        if (0.0f < p && p < 1.0f) {
            nscoord new_small_pct_expand =
                nscoord(float(colFrame->GetPrefCoord()) / p);
            if (new_small_pct_expand > max_small_pct_pref) {
                max_small_pct_pref = new_small_pct_expand;
            }
            pct_running_pct += p;
            pct_running_nonpct += colFrame->GetPrefCoord();
        }
    }

    // Account for small percentages expanding the preferred width of
    // *other* columns.
    if (max_small_pct_pref > pref)
        pref = max_small_pct_pref;

    // Account for large percentages expanding the preferred width of
    // themselves.
    if (0.0f < pct_running_pct && pct_running_pct < 1.0f) {
        nscoord large_pct_pref = nscoord(float(pref - pct_running_nonpct) /
                                         (1.0f - pct_running_pct));
        if (large_pct_pref > pref)
            pref = large_pct_pref;
    }

    // XXX Should this go before or after the percent business?
    if (colCount > 0) {
        pref += spacing;
        min += spacing;
    }

    float p2t = mTableFrame->GetPresContext()->ScaledPixelsToTwips();
    min = nsTableFrame::RoundToPixel(min, p2t);
    pref = nsTableFrame::RoundToPixel(pref, p2t);

    mMinWidth = min;
    mPrefWidth = pref;
}

/* virtual */ void
BasicTableLayoutStrategy::MarkIntrinsicWidthsDirty()
{
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mPrefWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcWidth = nscoord_MIN;
}

/* virtual */ void
BasicTableLayoutStrategy::CalcColumnWidths(const nsHTMLReflowState& aReflowState)
{
    nscoord width = aReflowState.mComputedWidth;

    if (mLastCalcWidth == width)
        return;
    mLastCalcWidth = width;

    if (mTableFrame->IsBorderCollapse()) {
        // The border and padding that was subtracted by the reflow
        // state counts as part of the column widths.
        width += aReflowState.mComputedBorderPadding.LeftRight();
    }

    // XXX Is this needed?
    NS_ASSERTION((mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aReflowState.rendContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();

    float p2t = mTableFrame->GetPresContext()->ScaledPixelsToTwips();

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
    } else if (colCount > 0) {
        u.e = width / cellMap->GetColCount();
    }

    // XXX This needs to consider percentage widths!

    for (PRInt32 col = 0; col < colCount; ++col) {
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
