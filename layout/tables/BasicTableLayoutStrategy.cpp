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
#include "nsTableCellFrame.h"
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
BasicTableLayoutStrategy::ComputeColumnIntrinsicWidths(nsIRenderingContext* aRenderingContext)
{
    nsTableFrame *tableFrame = mTableFrame;
    float p2t = tableFrame->GetPresContext()->ScaledPixelsToTwips();
    nsTableCellMap *cellMap = tableFrame->GetCellMap();

    // We need to consider the width on the table since a specified width
    // on the table prevents widths on cells that would make the table
    // larger than that specified width from being honored.  In this case,
    // we shrink the min widths proportionally, in proportion to the
    // difference between the width specified and the min width of the
    // content.
    const nsStylePosition *tablePos = tableFrame->GetStylePosition();
    PRBool tableHasWidth = tablePos->mWidth.GetUnit() == eStyleUnit_Coord ||
                           tablePos->mMaxWidth.GetUnit() == eStyleUnit_Coord;

    nscoord spacing = tableFrame->GetCellSpacingX();

    // Prevent percentages from adding to more than 100% by (to be
    // compatible with other browsers) treating any percentages that would
    // increase the total percentage to more than 100% as the number that
    // would increase it to only 100% (which is 0% if we've already hit
    // 100%).
    float pct_used = 0.0f;

    for (PRInt32 col = 0, col_end = cellMap->GetColCount();
         col < col_end; ++col) {
        nsTableColFrame *colFrame = tableFrame->GetColFrame(col);
        colFrame->ResetMinCoord();
        colFrame->ResetPrefCoord();
        colFrame->ResetPrefPercent();

        // XXX Consider col frame!
        // XXX Should it get Border/padding considered?

        for (PRInt32 row = 0, row_end = cellMap->GetRowCount();
             row < row_end; ++row) {
            PRBool originates;
            PRInt32 colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(row, col, &originates, &colSpan);
            if (!cellFrame)
                continue;

            const nsStylePosition *pos = cellFrame->GetStylePosition();
            nscoord minCoord = cellFrame->GetMinWidth(aRenderingContext);
            nscoord prefCoord = cellFrame->GetPrefWidth(aRenderingContext);
            float prefPercent = 0.0f;

            PRBool hasSpecifiedWidth = PR_FALSE;

            switch (pos->mWidth.GetUnit()) {
                case eStyleUnit_Coord: {
                      hasSpecifiedWidth = PR_TRUE;
                      nscoord w = pos->mWidth.GetCoordValue();
                      if (!tableHasWidth && w > minCoord)
                          minCoord = w;
                      prefCoord = PR_MAX(w, minCoord);
                    }
                    break;
                case eStyleUnit_Percent: {
                        prefPercent = pos->mWidth.GetPercentValue();
                    }
                    break;
                default:
                    break;
            }

            switch (pos->mMaxWidth.GetUnit()) {
                case eStyleUnit_Coord: {
                        hasSpecifiedWidth = PR_TRUE;
                        nscoord w = pos->mMaxWidth.GetCoordValue();
                        if (w < minCoord)
                            minCoord = w;
                        if (w < prefCoord)
                            prefCoord = w;
                    }
                    break;
                case eStyleUnit_Percent: {
                        float p = pos->mMaxWidth.GetPercentValue();
                        if (p < prefPercent)
                            prefPercent = p;
                    }
                    break;
                default:
                    break;
            }

            switch (pos->mMinWidth.GetUnit()) {
                case eStyleUnit_Coord: {
                        nscoord w = pos->mMinWidth.GetCoordValue();
                        if (w > minCoord)
                            minCoord = w;
                        if (w > prefCoord)
                            prefCoord = w;
                    }
                    break;
                case eStyleUnit_Percent: {
                        float p = pos->mMinWidth.GetPercentValue();
                        if (p > prefPercent)
                            prefPercent = p;
                    }
                    break;
                default:
                    break;
            }

            // Add the cell-spacing (and take it off again below) so that we don't
            // require extra room for the omitted cell-spacing of column-spanning
            // cells.
            minCoord += cellFrame->GetIntrinsicBorderPadding(
                    aRenderingContext, nsLayoutUtils::MIN_WIDTH) +
                spacing;
            prefCoord += cellFrame->GetIntrinsicBorderPadding(
                    aRenderingContext, nsLayoutUtils::PREF_WIDTH) +
                spacing;

            // XXX Rounding errors due to RoundToPixel below!
            minCoord = minCoord / colSpan;
            prefCoord = prefCoord / colSpan;
            prefPercent = prefPercent / colSpan;

            minCoord -= spacing;
            prefCoord -= spacing;

            minCoord = nsTableFrame::RoundToPixel(minCoord, p2t);
            prefCoord = nsTableFrame::RoundToPixel(prefCoord, p2t);

            if (colSpan > 1)
                hasSpecifiedWidth = PR_FALSE;

            colFrame->AddMinCoord(minCoord);
            colFrame->AddPrefCoord(prefCoord, hasSpecifiedWidth);
            colFrame->AddPrefPercent(prefPercent);
        }

        colFrame->AdjustPrefPercent(&pct_used);
    }
}

void
BasicTableLayoutStrategy::ComputeIntrinsicWidths(nsIRenderingContext* aRenderingContext)
{
    ComputeColumnIntrinsicWidths(aRenderingContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    nscoord min = 0, pref = 0, max_small_pct_pref = 0, pct_running_nonpct = 0;
    float pct_running_pct = 0.0f; // always from 0.0f - 1.0f
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        min += colFrame->GetMinCoord();
        pref += colFrame->GetPrefCoord();

        // Percentages are of the table, so we have to reverse them for
        // intrinsic widths.
        float p = colFrame->GetPrefPercent();
        if (p > 0.0f) {
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
    if (max_small_pct_pref > pref) {
        pct_running_nonpct += (max_small_pct_pref - pref);
        pref = max_small_pct_pref;
    }

    // Account for large percentages expanding the preferred width of
    // themselves.  This needs to happen *after* accounting for small
    // percentages; otherwise we'd need to iterate over the columns
    // multiple times.  (I'm not entirely convinced that we don't need
    // to even so, but it seems ok.)
    NS_ASSERTION(0.0f <= pct_running_pct && pct_running_pct <= 1.0f,
                 "column percentage widths not adjusted down to 100%");
    if (pct_running_pct == 1.0f) {
        if (pref - pct_running_nonpct > 0) {
            pref = nscoord_MAX;
            // XXX Or should I use some smaller value?  (Test this using
            // nested tables!)
        }
    } else {
        nscoord large_pct_pref = nscoord(float(pref - pct_running_nonpct) /
                                         (1.0f - pct_running_pct));
        if (large_pct_pref > pref)
            pref = large_pct_pref;
    }

    // border-spacing isn't part of the basis for percentages
    if (colCount > 0) {
        nscoord add = spacing * (colCount + 1);
        pref += add;
        min += add;
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

    // XXX Is this needed?
    NS_ASSERTION((mMinWidth == NS_INTRINSIC_WIDTH_UNKNOWN) ==
                 (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN),
                 "dirtyness out of sync");
    if (mPrefWidth == NS_INTRINSIC_WIDTH_UNKNOWN)
        ComputeIntrinsicWidths(aReflowState.rendContext);

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();
    float p2t = mTableFrame->GetPresContext()->ScaledPixelsToTwips();

    nscoord min = mMinWidth;

    // border-spacing isn't part of the basis for percentages.
    if (colCount > 0) {
        // XXX Should only add columns that have cells originating in them!
        nscoord subtract = spacing * (colCount + 1);
        width -= subtract;
        min -= subtract;
    }

    // XXX is |width| the right basis for percentage widths?

    // Loop #1 over the columns, initially assigning them the larger of
    // their percentage pref width or min width.
    nscoord assigned = 0;
    nscoord grow_basis = 0;
    nscoord zoom_basis = 0;
    nscoord zoom_fixed_basis = 0;
    float pct_basis = 0.0f; // 0.0f through 1.0f
    PRInt32 col;
    for (col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        nscoord pct_width = nscoord(float(width) * colFrame->GetPrefPercent());
        nscoord min_width = colFrame->GetMinCoord();
        // XXX Is it OK that pct_width isn't rounded to pixel?
        // (What's the point anyway?)
        nscoord val = PR_MAX(pct_width, min_width);
        // Note that we re-compute |val| in the loop below.

        if (colFrame->GetPrefPercent() == 0.0f) {
            nscoord pref_width = colFrame->GetPrefCoord();
            grow_basis += pref_width - min_width;
            if (!colFrame->GetHasSpecifiedCoord()) {
              zoom_basis += pref_width;
            }
            zoom_fixed_basis += pref_width;
        } else {
            pct_basis += colFrame->GetPrefPercent();
        }
        assigned += val;
    }

    // Loop #2 over the columns:
    // 1. If we have too little space, by shrinking the percentage width
    //    columns based on how far they are above their min width.
    // *  If we have extra space, give it all to the columns previously
    //    given their min width:
    //    2. in proportion to the difference between their min width and
    //       pref width up until all columns reach pref width,
    //    3. once columns reach pref width, in proportion to pref width
    //    4. if those are all zero, instead increase the percentage
    //       width columns in proportion to their percentages
    //    5. if there are no percentage width columns, equally
    enum Loop2Type {
        SHRINK, GROW, ZOOM_GROW, ZOOM_FIXED_GROW, PCT_GROW, EQUAL_GROW, NOOP
    };

    Loop2Type l2t;
    if (assigned > width) {
        if (assigned != min)
            l2t = SHRINK;
        else
            l2t = NOOP;
    } else {
        if (grow_basis > 0 && assigned + grow_basis > width)
            l2t = GROW;
        else if (zoom_basis > 0)
            l2t = ZOOM_GROW;
        else if (zoom_fixed_basis > 0)
            l2t = ZOOM_FIXED_GROW;
        else if (pct_basis > 0.0f)
            l2t = PCT_GROW;
        else if (colCount > 0)
            l2t = EQUAL_GROW;
        else
            l2t = NOOP;
    }
 
    // Hold previous to avoid accumulating rounding error.
    nscoord prev_x = 0, prev_x_round = 0;

    union {
        float f;
        nscoord c;
    } u;
    switch (l2t) {
        case SHRINK: // u.f is negative
            u.f = float(width - assigned) / float(assigned - min);
            break;
        case GROW:
            u.f = float(width - assigned) / float(grow_basis);
            break;
        case ZOOM_GROW:
            u.f = 1.0f + (float(width - assigned - grow_basis) /
                          float(zoom_basis));
            break;
        case ZOOM_FIXED_GROW:
            u.f = 1.0f + (float(width - assigned - grow_basis) /
                          float(zoom_fixed_basis));
            break;
        case PCT_GROW:
            u.f = float(width - assigned) / float(pct_basis);
            break;
        case EQUAL_GROW:
            u.c = width / cellMap->GetColCount();
            break;
        case NOOP:
            break;
    }

    for (col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        nscoord pct_width = nscoord(float(width) * colFrame->GetPrefPercent());
        nscoord min_width = colFrame->GetMinCoord();
        // recompute |val| from the loop above
        nscoord col_width = PR_MAX(pct_width, min_width);

        switch (l2t) {
            case SHRINK: // u.f is negative
                col_width += NSToCoordRound(u.f * (col_width - min_width));
                break;
            case GROW:
                if (colFrame->GetPrefPercent() == 0.0f)
                    col_width += NSToCoordRound(u.f *
                                                (colFrame->GetPrefCoord() -
                                                 min_width));
                break;
            case ZOOM_GROW:
                if (colFrame->GetPrefPercent() == 0.0f) {
                    if (colFrame->GetHasSpecifiedCoord())
                        // XXX It's usually already the pref coord; is
                        // it always?
                        col_width = colFrame->GetPrefCoord();
                    else
                        col_width = NSToCoordRound(u.f *
                                                   colFrame->GetPrefCoord());
                }
                break;
            case ZOOM_FIXED_GROW:
                if (colFrame->GetPrefPercent() == 0.0f)
                    col_width = NSToCoordRound(u.f *
                                               colFrame->GetPrefCoord());
                break;
            case PCT_GROW:
                col_width += NSToCoordRound(u.f * colFrame->GetPrefPercent());
                break;
            case EQUAL_GROW:
                col_width += u.c;
                NS_ASSERTION(min_width == 0, "yikes");
                break;
            case NOOP:
                break;
        }

        nscoord new_x = prev_x + col_width;
        nscoord new_x_round = nsTableFrame::RoundToPixel(new_x, p2t);

        nscoord old_final = colFrame->GetFinalWidth();
        nscoord new_final = new_x_round - prev_x_round;
        colFrame->SetFinalWidth(new_final);

        if (old_final != new_final)
            mTableFrame->DidResizeColumns();

        prev_x = new_x;
        prev_x_round = new_x_round;
    }
}
