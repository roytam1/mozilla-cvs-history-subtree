/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2002 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   David Bradley <dbradley@netscape.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/** \file XPCIDArray.cpp
 * XPCIDArray implementation
 * This file contains the implementation of the XPCIDArray class
 */

#include "XPCPrivate.h"

XPCIDArray::XPCIDArray(XPCCallContext& ccx, JSIdArray* array) : mMarked(JS_FALSE), mIDArray(array->length)
{
    for (jsint index = 0; index < array->length; ++index)
    {
        jsval val;
        if (JS_IdToValue(ccx, array->vector[index], &val))
        {
            mIDArray.ReplaceElementAt(NS_REINTERPRET_CAST(void*,val), index);
        }
    }   
}

void XPCIDArray::Mark()
{
    if (!IsMarked())
    {
        mMarked = JS_TRUE;
        XPCCallContext ccx(NATIVE_CALLER);
        if (ccx.IsValid())
        {
            PRUint32 size = Length();

            for (PRUint32 index = 0 ; index < size ; ++index)
            {
                void * pVal = mIDArray.ElementAt(index);
                if (JSVAL_IS_GCTHING(NS_REINTERPRET_CAST(jsval,pVal)))
                    JS_MarkGCThing(ccx, pVal, nsnull, nsnull);
            }
        }
    }
}

