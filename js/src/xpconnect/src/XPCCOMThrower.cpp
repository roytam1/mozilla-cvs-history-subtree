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
 *   David Bradley<dbradley@netscape.com> (original author)
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

/* Code for throwing errors into JavaScript. */

#include "xpcprivate.h"

// static
void
XPCCOMThrower::ThrowError(JSContext* cx, HRESULT COMErrorCode)
{
	IErrorInfo * pError;
    // Get the current COM error object
    HRESULT result = GetErrorInfo(0, &pError);
	if (SUCCEEDED(result) && pError)
	{
        // Build an error message from the COM error object
        nsCAutoString msg;
        BSTR bstrDesc = nsnull;
        BSTR bstrSource = nsnull;
        if (SUCCEEDED(pError->GetSource(&bstrSource)) && bstrSource)
        {
            msg = NS_STATIC_CAST(const char *,_bstr_t(bstrSource, FALSE));
            msg += " : ";
        }
        char buffer[9];
        sprintf(buffer, "%0X", COMErrorCode);
        msg += buffer;
        if (SUCCEEDED(pError->GetDescription(&bstrDesc)) && bstrDesc)
        {
            msg += " - ";
            msg += NS_STATIC_CAST(const char *,_bstr_t(bstrDesc, FALSE));
        }
        XPCThrower::BuildAndThrowException(cx, NS_ERROR_XPC_COM_ERROR, msg.get());
	}
    else
    {
        // No error object, so just report the result
        char buffer[48];
        sprintf(buffer, "COM Error Result = %0X", COMErrorCode);
        XPCThrower::BuildAndThrowException(cx, NS_ERROR_XPC_COM_UNKNOWN, buffer);
    }
}

