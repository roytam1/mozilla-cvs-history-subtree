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

#include "nsHTMLTextAccessible.h"
#include "nsWeakReference.h"
#include "nsIFrame.h"
#include "nsILink.h"
#include "nsILinkHandler.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIPresContext.h"
#include "nsReadableUtils.h"

nsHTMLTextAccessible::nsHTMLTextAccessible(nsIPresShell* aShell, nsIDOMNode* aDomNode):
nsLeafDOMAccessible(aShell, aDomNode), mIsALinkCached(PR_FALSE), mLinkContent(nsnull), mIsLinkVisited(PR_FALSE)
{ 
}

/* wstring getAccName (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccName(PRUnichar **_retval)
{ 

  nsAutoString nameString;
  nsresult rv = NS_OK;
  if (IsALink()) {
    rv = AppendFlatStringFromSubtree(mLinkContent, &nameString);
  }
  else mNode->GetNodeValue(nameString);
  nameString.CompressWhitespace();
  *_retval = nameString.ToNewUnicode();
  return rv;
}

/* wstring getAccRole (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccRole(PRUnichar **_retval)
{
  *_retval = ToNewUnicode(IsALink()? NS_LITERAL_STRING("link"): 
    NS_LITERAL_STRING("static text"));

  // we used to say editable text like IE!! That seems like a bug of theirs
  return NS_OK;
}

/* long GetAccState (); */
NS_IMETHODIMP nsHTMLTextAccessible::GetAccState(PRUint32 *_retval)
{
  *_retval |= STATE_READONLY | STATE_SELECTABLE;
  if (IsALink())
    *_retval |= STATE_FOCUSABLE | STATE_LINKED;
  
  // Get current selection and find out if current node is in it
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mPresShell));
  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  nsCOMPtr<nsIContent> content(do_QueryInterface(mNode));
  nsIFrame *frame;
  if (content && NS_SUCCEEDED(shell->GetPrimaryFrameFor(content, &frame))) {
    nsCOMPtr<nsISelectionController> selCon;
    frame->GetSelectionController(context,getter_AddRefs(selCon));
    if (selCon) {
      nsCOMPtr<nsISelection> domSel;
      selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
      if (domSel) {
        PRBool isSelected = PR_FALSE, isCollapsed = PR_TRUE;
        domSel->ContainsNode(mNode, PR_TRUE, &isSelected);
        domSel->GetIsCollapsed(&isCollapsed);
        if (isSelected && !isCollapsed)
          *_retval |=STATE_SELECTED;
      }
    }
  }

  // Focused? Do we implement that here or up the chain?
  return NS_OK;
}


NS_IMETHODIMP nsHTMLTextAccessible::GetAccDefaultAction(PRUnichar **_retval) 
{
  if (IsALink()) {
    *_retval = ToNewUnicode(NS_LITERAL_STRING("jump")); 
    return NS_OK;
  }
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHTMLTextAccessible::AccDoDefaultAction()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


PRBool nsHTMLTextAccessible::IsALink()
{
  if (mIsALinkCached)  // Cached answer?
    return mLinkContent? PR_TRUE: PR_FALSE;

  nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mNode));
  if (walkUpContent) {
    nsCOMPtr<nsIContent> tempContent;
    while (walkUpContent) {
      nsCOMPtr<nsILink> link(do_QueryInterface(walkUpContent));
      if (link) {
        mLinkContent = tempContent;
        mIsALinkCached = PR_TRUE;
        nsLinkState linkState;
        link->GetLinkState(linkState);
        if (linkState == eLinkState_Visited)
          mIsLinkVisited = PR_TRUE;
        return PR_TRUE;
      }
      walkUpContent->GetParent(*getter_AddRefs(tempContent));
      walkUpContent = tempContent;
    }
  }
  mIsALinkCached = PR_TRUE;  // Cached that there is no link
  return PR_FALSE;
}
