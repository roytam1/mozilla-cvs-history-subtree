/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsIAccessibilityService.h"
#include "nsAccessibilityService.h"
#include "nsAccessible.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsRootAccessible.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsIDOMNode.h"
#include "nsHTMLTextAccessible.h"
#include "nsHTMLTableAccessible.h"
#include "nsHTMLImageAccessible.h"
#include "nsHTMLAreaAccessible.h"
#include "nsHTMLLinkAccessible.h"
#include "nsHTMLSelectAccessible.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsHTMLFormControlAccessible.h"
#include "nsILink.h"

// IFrame
#include "nsIDocShell.h"
#include "nsHTMLIFrameRootAccessible.h"

//--------------------


nsAccessibilityService::nsAccessibilityService()
{
  NS_INIT_REFCNT();
  //printf("################################## nsAccessibilityService\n");
}

nsAccessibilityService::~nsAccessibilityService()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsAccessibilityService, nsIAccessibilityService);




////////////////////////////////////////////////////////////////////////////////
// nsIAccessibilityService methods:

NS_IMETHODIMP 
nsAccessibilityService::CreateRootAccessible(nsISupports* aPresContext, nsISupports* aFrame, nsIAccessible **_retval)
{
  nsIFrame* f = NS_STATIC_CAST(nsIFrame*, aFrame);

  nsCOMPtr<nsIPresContext> c(do_QueryInterface(aPresContext));
  NS_ASSERTION(c,"Error non prescontext passed to accessible factory!!!");

  nsCOMPtr<nsIPresShell> s;
  c->GetShell(getter_AddRefs(s));

  NS_ASSERTION(s,"Error not presshell!!");

  nsCOMPtr<nsIWeakReference> wr (getter_AddRefs(NS_GetWeakReference(s)));

  //printf("################################## CreateRootAccessible\n");
  *_retval = new nsRootAccessible(wr);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP 
nsAccessibilityService::CreateHTMLSelectAccessible(nsIDOMNode* node, nsISupports* aPresContext, nsIAccessible **_retval)
{
  nsCOMPtr<nsIPresContext> c(do_QueryInterface(aPresContext));
  NS_ASSERTION(c,"Error non prescontext passed to accessible factory!!!");

  nsCOMPtr<nsIPresShell> s;
  c->GetShell(getter_AddRefs(s)); 

  nsCOMPtr<nsIWeakReference> wr = getter_AddRefs(NS_GetWeakReference(s));

  *_retval = new nsHTMLSelectAccessible(node, wr);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_ERROR_FAILURE;
}

/* nsIAccessible createHTMLCheckboxAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLCheckboxAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLCheckboxAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMRadioButtonAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLRadioButtonAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLRadioButtonAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMLButtonAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLButtonAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLButtonAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTML4ButtonAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTML4ButtonAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTML4ButtonAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMLTextAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLTextAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  //printf("################################## CreateHTMLTextAccessible\n");
  *_retval = new nsHTMLTextAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}


/* nsIAccessible createHTMLTableAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLTableAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLTableAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMLTableCellAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLTableCellAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLTableCellAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMLImageAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLImageAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;
  nsIImageFrame* imageFrame = nsnull;
  
  // not using a nsCOMPtr frames don't support them.
  aFrame->QueryInterface(NS_GET_IID(nsIImageFrame), (void**)&imageFrame);

  if (!imageFrame)
    return NS_ERROR_FAILURE;

  *_retval = new nsHTMLImageAccessible(node, imageFrame, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

/* nsIAccessible createHTMLAreaAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLAreaAccessible(nsIWeakReference *aShell, nsIDOMNode *aDOMNode, nsIAccessible *aAccParent, 
                                                               nsIAccessible **_retval)
{
  *_retval = new nsHTMLAreaAccessible(aDOMNode, aAccParent, aShell);

  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;

}

/* nsIAccessible createHTMLTextFieldAccessible (in nsISupports aPresShell, in nsISupports aFrame); */
NS_IMETHODIMP nsAccessibilityService::CreateHTMLTextFieldAccessible(nsISupports *aFrame, nsIAccessible **_retval)
{
  nsIFrame* frame;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIWeakReference> shell;
  nsresult rv = GetInfo(aFrame, &frame, getter_AddRefs(shell), getter_AddRefs(node));
  if (NS_FAILED(rv))
    return rv;

  *_retval = new nsHTMLTextFieldAccessible(node, shell);
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  } else 
    return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsAccessibilityService::GetInfo(nsISupports* aFrame, nsIFrame** aRealFrame, nsIWeakReference** aShell, nsIDOMNode** aNode)
{
  *aRealFrame = NS_STATIC_CAST(nsIFrame*, aFrame);
  nsCOMPtr<nsIContent> content;
  (*aRealFrame)->GetContent(getter_AddRefs(content));
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
  *aNode = node;
  NS_ADDREF(*aNode);

  nsCOMPtr<nsIDocument> document;
  content->GetDocument(*getter_AddRefs(document));
  if (!document)
    return NS_ERROR_FAILURE;
  if (!document)
     return NS_ERROR_FAILURE;

#ifdef DEBUG
  PRInt32 shells = document->GetNumberOfShells();
  NS_ASSERTION(shells > 0,"Error no shells!");
#endif

  *aShell = NS_GetWeakReference(document->GetShellAt(0));
  NS_IF_ADDREF(*aShell);

  return NS_OK;
}

NS_IMETHODIMP 
nsAccessibilityService::CreateAccessible(nsIDOMNode* node, nsISupports* document, nsIAccessible **_retval)
{
  nsCOMPtr<nsIContent> content (do_QueryInterface(node));

  nsCOMPtr<nsIDocument> d (do_QueryInterface(document));
  if (!document)
    return NS_ERROR_FAILURE;

#ifdef DEBUG
  PRInt32 shells = d->GetNumberOfShells();
  NS_ASSERTION(shells > 0,"Error no shells!");
#endif

  nsCOMPtr<nsIWeakReference> wr (getter_AddRefs(NS_GetWeakReference(d->GetShellAt(0))));

  *_retval = new nsAccessible(node, wr);
  NS_IF_ADDREF(*_retval);

  return NS_OK;  
}

NS_IMETHODIMP 
nsAccessibilityService::CreateHTMLBlockAccessible(nsIDOMNode* node, nsISupports* document, nsIAccessible **_retval)
{
  nsCOMPtr<nsIContent> content (do_QueryInterface(node));

  nsCOMPtr<nsIDocument> d (do_QueryInterface(document));
  if (!document)
    return NS_ERROR_FAILURE;

#ifdef DEBUG
  PRInt32 shells = d->GetNumberOfShells();
  NS_ASSERTION(shells > 0,"Error no shells!");
#endif

  nsCOMPtr<nsIWeakReference> wr (getter_AddRefs(NS_GetWeakReference(d->GetShellAt(0))));

  *_retval = new nsAccessible(node, wr);
  NS_IF_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP 
nsAccessibilityService::CreateHTMLIFrameAccessible(nsIDOMNode* node, nsISupports* aPresContext, nsIAccessible **_retval)
{
  *_retval = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  NS_ASSERTION(content,"Error non nsIContent passed to accessible factory!!!");

  nsCOMPtr<nsIPresContext> presContext(do_QueryInterface(aPresContext));
  NS_ASSERTION(presContext,"Error non prescontext passed to accessible factory!!!");

  nsCOMPtr<nsIPresShell> presShell;
  presContext->GetShell(getter_AddRefs(presShell)); 
  NS_ASSERTION(presShell,"Error non PresShell passed to accessible factory!!!");

  nsCOMPtr<nsIWeakReference> weakRef (getter_AddRefs(NS_GetWeakReference(presShell)));

  nsCOMPtr<nsIDocument> doc;
  if (NS_SUCCEEDED(content->GetDocument(*getter_AddRefs(doc))) && doc) {
    nsCOMPtr<nsIPresShell> presShell = getter_AddRefs(doc->GetShellAt(0));
    if (presShell) {
      nsCOMPtr<nsISupports> supps;
      presShell->GetSubShellFor(content, getter_AddRefs(supps));
      if (supps) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(supps));
        if (docShell) {
          nsCOMPtr<nsIPresShell> ps;
          docShell->GetPresShell(getter_AddRefs(ps));
          if (ps) {
            nsCOMPtr<nsIWeakReference> wr (getter_AddRefs(NS_GetWeakReference(ps)));
            //printf("################################## CreateHTMLIFrameAccessible\n");

            nsCOMPtr<nsIAccessible> root = new nsHTMLIFrameRootAccessible(node,wr);
            *_retval = new nsHTMLIFrameAccessible(node, root, weakRef);
            NS_ADDREF(*_retval);
            return NS_OK;
          }
        }
      }
    }
  }
  return NS_ERROR_FAILURE;
}

/*
PRBool nsDOMTreeWalker::GetAccessible()
{
  mAccessible = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  if (!content)
    return PR_FALSE;

  nsCOMPtr<nsIDOMHTMLAreaElement> areaContent(do_QueryInterface(mDOMNode));
  if (areaContent)   // Area elements are implemented in nsHTMLImageAccessible as children of the image
    return PR_FALSE; // Return, otherwise the image frame looks like an accessible object in the wrong place

  nsIFrame* frame = GetPrimaryFrame();
  if (!frame)
    return PR_FALSE;

  frame->GetAccessible(getter_AddRefs(mAccessible));

  if (!mAccessible)
    mAccessible = do_QueryInterface(mDOMNode);

  if (!mAccessible) {
    // is it a link?
    nsCOMPtr<nsILink> link(do_QueryInterface(mDOMNode));
    if (link) {
       mAccessible = new nsHTMLLinkAccessible(mDOMNode, mPresShell);
    }
  }

  if (mAccessible)
    return PR_TRUE;
  else
    return PR_FALSE;
}
*/

/* nsIAccessible GetAccessibleFor (in nsISupports aPresShell, in nsIDOMNode aNode); */
NS_IMETHODIMP nsAccessibilityService::GetAccessibleFor(nsIWeakReference *aPresShell, nsIDOMNode *aNode, 
                                                       nsIAccessible **_retval) 
{
  *_retval = nsnull;

  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(aPresShell));

  if (!shell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

  if (!content)
    return PR_FALSE;

  nsCOMPtr<nsIDOMHTMLAreaElement> areaContent(do_QueryInterface(aNode));
  if (areaContent)   // Area elements are implemented in nsHTMLImageAccessible as children of the image
    return PR_FALSE; // Return, otherwise the image frame looks like an accessible object in the wrong place

  nsIFrame* frame = nsnull;
  if (!shell) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  shell->GetPrimaryFrameFor(content, &frame);
  if (!frame)
    return PR_FALSE;

  nsCOMPtr<nsIAccessible> newAcc;
  frame->GetAccessible(getter_AddRefs(newAcc));

  if (!newAcc)
    newAcc = do_QueryInterface(aNode);

  if (!newAcc) {
    // is it a link?
    nsCOMPtr<nsILink> link(do_QueryInterface(aNode));
    if (link) {
      *_retval = new nsHTMLLinkAccessible(aNode, aPresShell);
      NS_IF_ADDREF(*_retval);
      return NS_OK;
    }
  }

  if (!newAcc)
    return NS_ERROR_FAILURE;

  *_retval = newAcc;
  NS_ADDREF(*_retval);

  return NS_OK;
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

nsresult
NS_NewAccessibilityService(nsIAccessibilityService** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
      return NS_ERROR_NULL_POINTER;

  nsAccessibilityService* a = new nsAccessibilityService();
  if (a == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(a);
  *aResult = a;
  return NS_OK;
}

