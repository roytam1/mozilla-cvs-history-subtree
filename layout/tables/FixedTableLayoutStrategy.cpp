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
 * Algorithms that determine column and table widths used for CSS2's
 * 'table-layout: fixed'.
 */

#include "FixedTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"

FixedTableLayoutStrategy::FixedTableLayoutStrategy(nsTableFrame *aTableFrame)
  : mTableFrame(aTableFrame)
{
    MarkIntrinsicWidthsDirty();
}

/* virtual */
FixedTableLayoutStrategy::~FixedTableLayoutStrategy()
{
}

/* virtual */ nscoord
FixedTableLayoutStrategy::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
    DISPLAY_MIN_WIDTH(mTableFrame, mMinWidth);
    if (mMinWidth != NS_INTRINSIC_WIDTH_UNKNOWN)
        return mMinWidth;

    // It's theoretically possible to do something much better here that
    // depends only on the columns and the first row, but it wouldn't be
    // compatible with other browsers, or with the use of GetMinWidth by
    // nsHTMLReflowState to determine the width of a fixed-layout table,
    // since CSS2.1 says:
    //   The width of the table is then the greater of the value of the
    //   'width' property for the table element and the sum of the
    //   column widths (plus cell spacing or borders).

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    // XXX Should this code do any pixel rounding?

    nscoord coordTotal = 0;
    float percentTotal = 0.0f;

    if (colCount > 0) {
        // XXX Should only add columns that have cells originating in them!
        coordTotal += spacing * (colCount + 1);
    }

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        const nsStyleCoord *styleWidth =
            &colFrame->GetStylePosition()->mWidth;
        if (styleWidth->GetUnit() == eStyleUnit_Coord) {
            coordTotal += styleWidth->GetCoordValue();
        } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
            percentTotal += styleWidth->GetPercentValue();
        } else {
            NS_ASSERTION(styleWidth->GetUnit() == eStyleUnit_Auto, "bad width");

            // The 'table-layout: fixed' algorithm considers only cells
            // in the first row.
            PRBool originates;
            PRInt32 colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->GetStylePosition()->mWidth;
                if (styleWidth->GetUnit() == eStyleUnit_Coord) {
                    nscoord cellWidth = styleWidth->GetCoordValue();
                    // Add in cell's padding and border.
                    cellWidth += cellFrame->GetIntrinsicBorderPadding(
                                  aRenderingContext, nsLayoutUtils::MIN_WIDTH);

                    if (colSpan > 1) {
                        // If a column-spanning cell is in the first
                        // row, split up the space evenly.  (XXX This
                        // isn't quite right if some of the columns it's
                        // in have specified widths.  Should we care?)
                        cellWidth = ((cellWidth + spacing) / colSpan) - spacing;
                    }
                    coordTotal += cellWidth;
                } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
                    percentTotal = styleWidth->GetPercentValue() / float(colSpan);
                    if (colSpan > 1) {
                        coordTotal -= spacing * (colSpan - 1);
                    }
                }
            }
        }
    }

    // XXX It needs to be clamed somewhere less than 1.  The exact value
    // should be tested for interoperability.
    if (percentTotal > 0.99f)
        percentTotal = 0.99f;

    nscoord result = coordTotal / (1.0f - percentTotal);

    return (mMinWidth = result);
}

/* virtual */ nscoord
FixedTableLayoutStrategy::GetPrefWidth(nsIRenderingContext* aRenderingContext)
{
    // It's theoretically possible to do something much better here that
    // depends only on the columns and the first row, but it wouldn't be
    // compatible with other browsers.
    nscoord result = nscoord_MAX;
    DISPLAY_PREF_WIDTH(mTableFrame, result);
    return result;
}

/* virtual */ void
FixedTableLayoutStrategy::MarkIntrinsicWidthsDirty()
{
    mMinWidth = NS_INTRINSIC_WIDTH_UNKNOWN;
    mLastCalcWidth = nscoord_MIN;
}

/* virtual */ void
FixedTableLayoutStrategy::CalcColumnWidths(const nsHTMLReflowState& aReflowState)
{
    nscoord tableWidth = aReflowState.mComputedWidth;

    if (mLastCalcWidth == tableWidth)
        return;
    mLastCalcWidth = tableWidth;

    nsTableCellMap *cellMap = mTableFrame->GetCellMap();
    PRInt32 colCount = cellMap->GetColCount();
    nscoord spacing = mTableFrame->GetCellSpacingX();

    // XXX Should this code do any pixel rounding?

    // border-spacing isn't part of the basis for percentages.
    if (colCount > 0) {
        // XXX Should only add columns that have cells originating in them!
        nscoord subtract = spacing * (colCount + 1);
        tableWidth -= subtract;
    }

    // XXX This ignores the 'min-width' and 'max-width' properties
    // throughout.  Then again, that's what the CSS spec says to do.

    PRUint32 unassignedCount = 0;
    nscoord unassignedSpace = tableWidth;
    const nscoord unassignedMarker = nscoord_MIN;

    for (PRInt32 col = 0; col < colCount; ++col) {
        nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
        const nsStyleCoord *styleWidth =
            &colFrame->GetStylePosition()->mWidth;
        nscoord colWidth;
        if (styleWidth->GetUnit() == eStyleUnit_Coord) {
            colWidth = styleWidth->GetCoordValue();
        } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
            colWidth = NSToCoordFloor(styleWidth->GetPercentValue() *
                                      float(tableWidth));
        } else {
            NS_ASSERTION(styleWidth->GetUnit() == eStyleUnit_Auto, "bad width");

            // The 'table-layout: fixed' algorithm considers only cells
            // in the first row.
            PRBool originates;
            PRInt32 colSpan;
            nsTableCellFrame *cellFrame =
                cellMap->GetCellInfoAt(0, col, &originates, &colSpan);
            if (cellFrame) {
                styleWidth = &cellFrame->GetStylePosition()->mWidth;
                if (styleWidth->GetUnit() == eStyleUnit_Coord) {
                    colWidth = styleWidth->GetCoordValue();
                } else if (styleWidth->GetUnit() == eStyleUnit_Percent) {
                    colWidth = NSToCoordFloor(styleWidth->GetPercentValue() *
                                              float(tableWidth));
                } else {
                    colWidth = unassignedMarker;
                }
                if (colWidth != unassignedMarker) {
                    // Add in cell's padding and border.
                    // XXX This should use real percentage padding, not
                    // the intrinsic width estimate for percentages!
                    colWidth += cellFrame->GetIntrinsicBorderPadding(
                                  aReflowState.rendContext, nsLayoutUtils::MIN_WIDTH);

                    if (colSpan > 1) {
                        // If a column-spanning cell is in the first
                        // row, split up the space evenly.  (XXX This
                        // isn't quite right if some of the columns it's
                        // in have specified widths.  Should we care?)
                        colWidth = ((colWidth + spacing) / colSpan) - spacing;
                    }
                }
            } else {
                colWidth = unassignedMarker;
            }
        }

        colFrame->SetFinalWidth(colWidth);

        if (colWidth == unassignedMarker) {
            ++unassignedCount;
        } else {
            unassignedSpace -= colWidth;
        }
    }

    if (unassignedCount > 0) {
        if (unassignedSpace < 0)
            unassignedSpace = 0;
        nscoord toAssign = unassignedSpace / unassignedCount;
        for (PRInt32 col = 0; col < colCount; ++col) {
            nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
            if (colFrame->GetFinalWidth() == unassignedMarker)
                colFrame->SetFinalWidth(toAssign);
        }
    } else if (unassignedSpace > 0) {
        nscoord toAdd = unassignedSpace / colCount;
        for (PRInt32 col = 0; col < colCount; ++col) {
            nsTableColFrame *colFrame = mTableFrame->GetColFrame(col);
            colFrame->SetFinalWidth(colFrame->GetFinalWidth() + toAdd);
        }
    }
}
