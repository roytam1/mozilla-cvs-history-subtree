/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Author: Eric Vaughan (evaughan@netscape.com)
 * Contributor(s): 
 */

#include "nsGenericAccessible.h"
#include "nsHTMLTextAccessible.h"
#include "nsHTMLImageAccessible.h"
#include "nsReadableUtils.h"
#include "nsAccessible.h"

// --- image -----

nsHTMLImageAccessible::nsHTMLImageAccessible(nsIPresShell* aShell, nsIDOMNode* aDOMNode):
nsHTMLTextAccessible(aShell, aDOMNode)  //, mIsALinkCached(PR_FALSE), mLinkContent(nsnull), mIsLinkVisited(PR_FALSE)
{ 
}

/* wstring getAccName (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetAccName(PRUnichar **_retval)
{
  nsCOMPtr<nsIContent> imageContent(do_QueryInterface(mNode));
  if (imageContent) {
    nsAutoString nameString;
    nsresult rv = AppendFlatStringFromContentNode(imageContent, &nameString);
    if (NS_SUCCEEDED(rv)) {
      nameString.CompressWhitespace();
      *_retval = nameString.ToNewUnicode();
    }
    return rv;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* long GetAccState (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetAccState(PRUint32 *_retval)
{
  *_retval |= STATE_READONLY;
  if (IsALink())
    *_retval |= STATE_FOCUSABLE | STATE_LINKED;
  if (mIsLinkVisited)
    *_retval |= STATE_TRAVERSED;
  // Focused? Do we implement that here or up the chain?
  return NS_OK;
}

/* wstring getAccRole (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetAccRole(PRUnichar **_retval)
{
  *_retval = ToNewUnicode(NS_LITERAL_STRING("graphic"));
  return NS_OK;
}

/* wstring getAccDefaultAction (); */
NS_IMETHODIMP nsHTMLImageAccessible::GetAccDefaultAction(PRUnichar **_retval)
{
  if (mLinkContent) {
    *_retval = ToNewUnicode(NS_LITERAL_STRING("Jump"));
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void accDoDefaultAction (); */
NS_IMETHODIMP nsHTMLImageAccessible::AccDoDefaultAction()
{
  return NS_OK;
}

