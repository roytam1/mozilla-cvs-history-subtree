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
 * Contributor(s): 
 */

#include "nsAccessible.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIScrollableView.h"
#include "nsRootAccessible.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIEventStateManager.h"

// IFrame Helpers
#include "nsIDocShell.h"
#include "nsIWebShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDocShellTreeItem.h"

#ifdef NS_DEBUG
#include "nsIFrameDebug.h"
#include "nsIDOMCharacterData.h"
#endif

//#define DEBUG_LEAKS

#ifdef DEBUG_LEAKS
static gnsAccessibles = 0;
#endif

class nsFrameTreeWalker {
public:
  nsFrameTreeWalker(nsIPresContext* aPresContext, nsAccessible* aOwner);
  nsIFrame* GetNextSibling(nsIFrame* aFrame);
  nsIFrame* GetPreviousSibling(nsIFrame* aFrame);
  nsIFrame* GetParent(nsIFrame* aFrame);
  nsIFrame* GetFirstChild(nsIFrame* aFrame);
  nsIFrame* GetLastChild(nsIFrame* aFrame);
  nsIFrame* GetChildBefore(nsIFrame* aParent, nsIFrame* aChild);
  PRInt32 GetCount(nsIFrame* aFrame);

  static PRBool ShouldSkip(nsIPresContext* aContext, nsIAtom* aList, nsIFrame* aStart, nsIFrame* aNext);
  static void GetAccessible(nsIFrame* aFrame, nsCOMPtr<nsIAccessible>& aAccessible, nsCOMPtr<nsIContent>& aContent);

  nsCOMPtr<nsIPresContext> mPresContext;
  nsCOMPtr<nsIAccessible> mAccessible;
  nsCOMPtr<nsIContent> mContent;
  nsAccessible* mOwner;
};

nsFrameTreeWalker::nsFrameTreeWalker(nsIPresContext* aPresContext, nsAccessible* aOwner)
{
  mPresContext = aPresContext;
  mOwner = aOwner;
}

nsIFrame* nsFrameTreeWalker::GetParent(nsIFrame* aFrame)
{
  //printf("Get parent\n");

  nsIFrame* parent = nsnull;
  aFrame->GetParent(&parent);

  // if no parent then we hit the root
  // just return that top frame
  if (!parent) {
    mAccessible = nsnull;
    mContent = nsnull;
    return aFrame;
  }

  GetAccessible(parent, mAccessible, mContent);
  if (mAccessible)
    return parent;
  
  return GetParent(parent);
}


nsIFrame* nsFrameTreeWalker::GetNextSibling(nsIFrame* aFrame)
{
  //printf("Get next\n");

  // get next sibling
  nsIFrame* next = nsnull;
  aFrame->GetNextSibling(&next);
  nsIAtom* list = nsnull;
  mOwner->GetListAtomForFrame(aFrame, list);

  
  // skip any frames with the same content node
  while(ShouldSkip(mPresContext, list, aFrame, next)) 
    next->GetNextSibling(&next);
  

  // if failed
  if (!next)
  {
    // if parent has content
    nsIFrame* parent = nsnull;
    aFrame->GetParent(&parent);
    
    // if no parent fail
    if (!parent) {
      mAccessible = nsnull;
      mContent = nsnull;
      return nsnull;
    }

    // fail if we reach a parent that is accessible
    GetAccessible(parent, mAccessible, mContent);
    if (mAccessible)
    {
      // fail
      mAccessible = nsnull;
      mContent = nsnull;
      return nsnull;
    } else {
      // next on parent
      nsIFrame* n = GetNextSibling(parent);
      if (ShouldSkip(mPresContext, list, aFrame, n))
        return GetNextSibling(n);
      else 
        return n;
    }
  }

  // if next has content
  GetAccessible(next, mAccessible, mContent);
  if (mAccessible)
  {
    // done
    return next;
  }

  // if next doesn't have node

  // call first on next
  nsIFrame* first = GetFirstChild(next);

  // if found
  if (first) {
    if (ShouldSkip(mPresContext, list, aFrame, first))
      return GetNextSibling(first);
    else 
      return first;
  }

  // call next on next
  nsIFrame* n =  GetNextSibling(next);
  if (ShouldSkip(mPresContext, list, aFrame, next))
    return GetNextSibling(n);
  else 
    return n;
}

nsIFrame* nsFrameTreeWalker::GetFirstChild(nsIFrame* aFrame)
{

  //printf("Get first\n");

  // get first child
  nsIFrame* child = nsnull;
  nsIAtom* list = nsnull;
  mOwner->GetListAtomForFrame(aFrame, list);
  aFrame->FirstChild(mPresContext, list, &child);

  while(child)
  {
    // if first has a content node
    GetAccessible(child, mAccessible, mContent);
    if (mAccessible)
    {
      // done
      return child;
    } else {
      // call first on child
      nsIFrame* first = GetFirstChild(child);

      // if succeeded
      if (first)
      {
        // return child
        return first;
      }
    }

    // get next sibling
    nsIFrame* next;
    child->GetNextSibling(&next);

    // skip children with duplicate content nodes
    nsIAtom* list = nsnull;
    mOwner->GetListAtomForFrame(child, list);

    while(ShouldSkip(mPresContext, list, child, next)) 
      next->GetNextSibling(&next);

    child = next;
  }

  // fail
  mAccessible = nsnull;
  mContent = nsnull;
  return nsnull;
}

nsIFrame* nsFrameTreeWalker::GetChildBefore(nsIFrame* aParent, nsIFrame* aChild)
{
  nsIFrame* child = GetFirstChild(aParent);

  // if the child is not us
  if (child == aChild) {
    mAccessible = nsnull;
    mContent = nsnull;
    return nsnull;
  }

  nsIFrame* prev = child;
  nsCOMPtr<nsIContent> prevContent = mContent;
  nsCOMPtr<nsIAccessible> prevAccessible = mAccessible;

  while(child) 
  {
    child = GetNextSibling(child);

    if (child == aChild)
      break;

    prev = child;
    prevContent = mContent;
    prevAccessible = mAccessible;
  }

  mAccessible = prevAccessible;
  mContent = prevContent;
  return prev;
}

nsIFrame* nsFrameTreeWalker::GetPreviousSibling(nsIFrame* aFrame)
{
  //printf("Get previous\n");

  nsIFrame* parent = GetParent(aFrame);
  
  return GetChildBefore(parent, aFrame);
}

nsIFrame* nsFrameTreeWalker::GetLastChild(nsIFrame* aFrame)
{
  //printf("Get last\n");

  return GetChildBefore(aFrame, nsnull);
}

PRInt32 nsFrameTreeWalker::GetCount(nsIFrame* aFrame)
{

  //printf("Get count\n");
  nsIFrame* child = GetFirstChild(aFrame);

  PRInt32 count = 0;
  while(child) 
  {
    count++;
    child = GetNextSibling(child);
  }

  return count;
}

void nsFrameTreeWalker::GetAccessible(nsIFrame* aFrame, nsCOMPtr<nsIAccessible>& aAccessible, nsCOMPtr<nsIContent>& aContent)
{
  aContent = nsnull;
  aAccessible = nsnull;

  aFrame->GetContent(getter_AddRefs(aContent));

  if (!aContent)
    return;

#if 1
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));
#else
  nsCOMPtr<nsIDocument> document;
  aContent->GetDocument(*getter_AddRefs(document));
  if (!document)
    return;

  PRInt32 shells = document->GetNumberOfShells();
  NS_ASSERTION(shells > 0,"Error no shells!");
  nsIPresShell* shell = document->GetShellAt(0);
#endif
  nsIFrame* frame = nsnull;
  shell->GetPrimaryFrameFor(aContent, &frame);

  if (!frame)
    return;

  aAccessible = do_QueryInterface(aFrame);

  if (!aAccessible)
    aAccessible = do_QueryInterface(aContent);

 // if (aAccessible)
 //   printf("Found accessible!\n");
}

PRBool nsFrameTreeWalker::ShouldSkip(nsIPresContext* aContext, nsIAtom* aList, nsIFrame* aStart, nsIFrame* aNext)
{
  if (!aStart || !aNext)
    return PR_FALSE;

  // is content the same? If so skip it
  nsCOMPtr<nsIContent> content1;
  nsCOMPtr<nsIContent> content2;

  aStart->GetContent(getter_AddRefs(content1));
  aNext->GetContent(getter_AddRefs(content2));

  if (content1 == content2 && content1 != nsnull) {
    // does it have childen? It it does then don't skip it
    nsIFrame* child = nsnull;
    aNext->FirstChild(aContext, aList, &child);
    if (child)
      return PR_FALSE;

    return PR_TRUE;
  }
  
  return PR_FALSE;
}

/*
 * Class nsAccessible
 */

//-----------------------------------------------------
// construction 
//-----------------------------------------------------
nsAccessible::nsAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  NS_INIT_REFCNT();

  // get frame and node
  mContent = aContent;
  mAccessible = aAccessible;
  mPresShell = aShell;
     
  nsCOMPtr<nsIDocument> document;
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mPresShell));
  if (shell)
    shell->GetDocument(getter_AddRefs(document));
  if (document) {
    nsCOMPtr<nsIScriptGlobalObject> ourGlobal;
    document->GetScriptGlobalObject(getter_AddRefs(ourGlobal));
    nsCOMPtr<nsPIDOMWindow> ourWindow(do_QueryInterface(ourGlobal));
    if(ourWindow) 
      ourWindow->GetRootFocusController(getter_AddRefs(mFocusController));
  }
#ifdef NS_DEBUG_X
   {
     nsCOMPtr<nsIPresShell> shell = do_QueryReferent(aShell);
     printf(">>> %p Created Acc - Con: %p  Acc: %p  PS: %p", 
             (nsIAccessible*)this, aContent, aAccessible, shell.get());
     if (shell && aContent != nsnull) {
       nsIFrame* frame;
       shell->GetPrimaryFrameFor(aContent, &frame);
       char * name;
       if (GetNameForFrame(frame, &name)) {
         printf(" Name:[%s]", name);
         nsMemory::Free(name);
       }
     }
     printf("\n");
   }
#endif
   // get frame and node
   mContent = aContent;
   mAccessible = aAccessible;
   mPresShell = aShell;

#ifdef DEBUG_LEAKS
  printf("nsAccessibles=%d\n", ++gnsAccessibles);
#endif

}

//-----------------------------------------------------
// destruction
//-----------------------------------------------------
nsAccessible::~nsAccessible()
{
#ifdef DEBUG_LEAKS
  printf("nsAccessibles=%d\n", --gnsAccessibles);
#endif
}

//NS_IMPL_ISUPPORTS2(nsAccessible, nsIAccessible, nsIAccessibleWidgetAccess);
NS_IMPL_ISUPPORTS1(nsAccessible, nsIAccessible);

nsresult nsAccessible::GetAccParent(nsIPresContext*   aPresContext,
                                    nsIWeakReference* aPresShell,
                                    nsIFrame*         aFrame,
                                    nsIAccessible **  aAccParent)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aAccParent);

  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccParent(aAccParent);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (aPresContext) {
    nsFrameTreeWalker walker(aPresContext, this); 

    // failed? Lets do some default behavior
    walker.GetParent(aFrame);

    // if no content or accessible then we hit the root
    if (!walker.mContent || !walker.mAccessible)
    {

      if (aPresContext) {
        nsCOMPtr<nsIPresShell> presShell;
        aPresContext->GetShell(getter_AddRefs(presShell));
        if (presShell) {
          nsCOMPtr<nsIPresShell> parentPresShell;
          nsCOMPtr<nsIContent> content;
          if (NS_SUCCEEDED(GetParentPresShellAndContent(presShell,
                                                        getter_AddRefs(parentPresShell),
                                                        getter_AddRefs(content)))) {
           nsIFrame* frame;
           parentPresShell->GetPrimaryFrameFor(content, &frame);
           nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(frame));
            if (!accessible)
              accessible = do_QueryInterface(content);
            if (accessible) {
              nsCOMPtr<nsIWeakReference> wr = getter_AddRefs(NS_GetWeakReference(parentPresShell));
              *aAccParent = CreateNewParentAccessible(accessible, content, wr);
              NS_ADDREF(*aAccParent);
              return NS_OK;
            }
          }
        }
      }

      *aAccParent = new nsRootAccessible(aPresShell);
      NS_ADDREF(*aAccParent);
      return NS_OK;
    }

    *aAccParent = CreateNewParentAccessible(walker.mAccessible, walker.mContent, aPresShell);
    NS_ADDREF(*aAccParent);
    return NS_OK;
  }

  *aAccParent = nsnull;
  return NS_OK;
}

  /* readonly attribute nsIAccessible accParent; */
NS_IMETHODIMP nsAccessible::GetAccParent(nsIAccessible * *aAccParent) 
{ 

  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(presContext);

  return GetAccParent(presContext, mPresShell, GetFrame(), aAccParent);
}

  /* readonly attribute nsIAccessible accNextSibling; */
NS_IMETHODIMP nsAccessible::GetAccNextSibling(nsIAccessible * *aAccNextSibling) 
{ 
  // delegate
 
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccNextSibling(aAccNextSibling);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  // failed? Lets do some default behavior

  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (context) {
    nsFrameTreeWalker walker(context, this);

    nsIFrame* next = walker.GetNextSibling(GetFrame());
  
    if (next && walker.mAccessible && walker.mContent) 
    {
      *aAccNextSibling = CreateNewNextAccessible(walker.mAccessible, walker.mContent, mPresShell);
      NS_ADDREF(*aAccNextSibling);
      return NS_OK;
    }
  }

  *aAccNextSibling = nsnull;

  return NS_OK;  
}

  /* readonly attribute nsIAccessible accPreviousSibling; */
NS_IMETHODIMP nsAccessible::GetAccPreviousSibling(nsIAccessible * *aAccPreviousSibling) 
{ 
// delegate
 
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccPreviousSibling(aAccPreviousSibling);
    if (NS_SUCCEEDED(rv))
      return rv;
  }
 
  // failed? Lets do some default behavior
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (context) {
    nsFrameTreeWalker walker(context, this); 
    nsIFrame* prev = walker.GetPreviousSibling(GetFrame());

    if (prev && walker.mAccessible && walker.mContent) 
    {
      *aAccPreviousSibling = CreateNewPreviousAccessible(walker.mAccessible, walker.mContent, mPresShell);
      NS_ADDREF(*aAccPreviousSibling);
      return NS_OK;
    }
  }

  *aAccPreviousSibling = nsnull;

  return NS_OK;  
}

  /* readonly attribute nsIAccessible accFirstChild; */
NS_IMETHODIMP nsAccessible::GetAccFirstChild(nsIAccessible * *aAccFirstChild) 
{ 
    
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccFirstChild(aAccFirstChild);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (context) {

    nsFrameTreeWalker walker(context, this); 
    nsIFrame* child = walker.GetFirstChild(GetFrame());

    if (child && walker.mAccessible && walker.mContent) 
    {
      *aAccFirstChild = CreateNewFirstAccessible(walker.mAccessible, walker.mContent, mPresShell);
      NS_ADDREF(*aAccFirstChild);
      return NS_OK;
    }
  }

  *aAccFirstChild = nsnull;

  return NS_OK;  
}

  /* readonly attribute nsIAccessible accFirstChild; */
NS_IMETHODIMP nsAccessible::GetAccLastChild(nsIAccessible * *aAccLastChild)
{  
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccLastChild(aAccLastChild);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (context) {
    nsFrameTreeWalker walker(context, this); 
    nsIFrame* last = walker.GetLastChild(GetFrame());

    if (last && walker.mAccessible && walker.mContent) 
    {
      *aAccLastChild = CreateNewLastAccessible(walker.mAccessible, walker.mContent, mPresShell);
      NS_ADDREF(*aAccLastChild);
      return NS_OK;
    }
  }

  *aAccLastChild = nsnull;

  return NS_OK;
}

/* readonly attribute long accChildCount; */
NS_IMETHODIMP nsAccessible::GetAccChildCount(PRInt32 *aAccChildCount) 
{
  
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccChildCount(aAccChildCount);
    if (NS_SUCCEEDED(rv))
      return rv;
  }
  

  // failed? Lets do some default behavior
  nsCOMPtr<nsIPresContext> context;
  GetPresContext(context);

  if (context) {
    nsFrameTreeWalker walker(context, this); 
    *aAccChildCount = walker.GetCount(GetFrame());
  } else 
    *aAccChildCount = 0;

  return NS_OK;  
}


  /* attribute wstring accName; */
NS_IMETHODIMP nsAccessible::GetAccName(PRUnichar * *aAccName) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccName(aAccName);
    if (NS_SUCCEEDED(rv) && *aAccName != nsnull)
      return rv;
  }

  *aAccName = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
}

  /* attribute wstring accName; */
NS_IMETHODIMP nsAccessible::GetAccDefaultAction(PRUnichar * *aDefaultAction) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccDefaultAction(aDefaultAction);
    if (NS_SUCCEEDED(rv) && *aDefaultAction != nsnull)
      return rv;
  }

  *aDefaultAction = 0;
  return NS_ERROR_NOT_IMPLEMENTED;  
}

NS_IMETHODIMP nsAccessible::SetAccName(const PRUnichar * aAccName) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->SetAccName(aAccName);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_NOT_IMPLEMENTED;  
}

  /* attribute wstring accValue; */
NS_IMETHODIMP nsAccessible::GetAccValue(PRUnichar * *aAccValue) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccValue(aAccValue);

    if (NS_SUCCEEDED(rv) && *aAccValue != nsnull)
      return rv;
  }

  *aAccValue = 0;
  return NS_ERROR_NOT_IMPLEMENTED;  
}

NS_IMETHODIMP nsAccessible::SetAccValue(const PRUnichar * aAccValue) { return NS_ERROR_NOT_IMPLEMENTED;  }

  /* readonly attribute wstring accDescription; */
NS_IMETHODIMP nsAccessible::GetAccDescription(PRUnichar * *aAccDescription) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccDescription(aAccDescription);
    if (NS_SUCCEEDED(rv) && *aAccDescription != nsnull)
      return rv;
  }

  return NS_ERROR_NOT_IMPLEMENTED;  
}

  /* readonly attribute wstring accRole; */
NS_IMETHODIMP nsAccessible::GetAccRole(PRUnichar * *aAccRole) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccRole(aAccRole);
    if (NS_SUCCEEDED(rv) && *aAccRole != nsnull)
      return rv;
  }

  return NS_ERROR_NOT_IMPLEMENTED;  
}

  /* readonly attribute wstring accState; */
NS_IMETHODIMP nsAccessible::GetAccState(PRUint32 *aAccState) 
{ 
  nsresult rv = NS_OK; 
  *aAccState = 0;

  // delegate
  if (mAccessible) 
    rv = mAccessible->GetAccState(aAccState);

  if (NS_SUCCEEDED(rv) && mFocusController) {
    nsCOMPtr<nsIDOMElement> focusedElement, currElement(do_QueryInterface(mContent));
    mFocusController->GetFocusedElement(getter_AddRefs(focusedElement));
    if (focusedElement == currElement)
      *aAccState |= STATE_FOCUSED;
  }

  return rv;
}

NS_IMETHODIMP nsAccessible::GetAccExtState(PRUint32 *aAccExtState) 
{ 
  // delegate
  if (mAccessible) 
    return mAccessible->GetAccExtState(aAccExtState);

  return NS_ERROR_NOT_IMPLEMENTED;  
}

  /* readonly attribute wstring accHelp; */
NS_IMETHODIMP nsAccessible::GetAccHelp(PRUnichar * *aAccHelp) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->GetAccHelp(aAccHelp);
    if (NS_SUCCEEDED(rv) && *aAccHelp != nsnull)
      return rv;
  }

  // failed? Lets do some default behavior
  return NS_ERROR_NOT_IMPLEMENTED;  
}

  /* readonly attribute boolean accFocused; */
NS_IMETHODIMP nsAccessible::GetAccFocused(PRBool *aAccFocused) { return NS_OK;  }

  /* nsIAccessible accGetChildAt (in long x, in long y); */
NS_IMETHODIMP nsAccessible::AccGetAt(PRInt32 tx, PRInt32 ty, nsIAccessible **_retval)
{
  PRInt32 x,y,w,h;
  AccGetBounds(&x,&y,&w,&h);
  if (tx > x && tx < x + w && ty > y && ty < y + h)
  {
    nsCOMPtr<nsIAccessible> child;
    nsCOMPtr<nsIAccessible> next;
    GetAccFirstChild(getter_AddRefs(child));
    PRInt32 cx,cy,cw,ch;

    while(child) {
      child->AccGetBounds(&cx,&cy,&cw,&ch);
      if (tx > cx && tx < cx + cw && ty > cy && ty < cy + ch) 
      {
        *_retval = child;
        NS_ADDREF(*_retval);
        return NS_OK;
      }
      child->GetAccNextSibling(getter_AddRefs(next));
      child = next;
    }


    *_retval = this;
    NS_ADDREF(this);
    return NS_OK;
  }

  *_retval = nsnull;
  return NS_OK;
}

  /* void accNavigateRight (); */
NS_IMETHODIMP nsAccessible::AccNavigateRight(nsIAccessible **_retval) { return NS_OK;  }

  /* void navigateLeft (); */
NS_IMETHODIMP nsAccessible::AccNavigateLeft(nsIAccessible **_retval) { return NS_OK;  }

  /* void navigateUp (); */

NS_IMETHODIMP nsAccessible::AccNavigateUp(nsIAccessible **_retval) { return NS_OK;  }

  /* void navigateDown (); */
NS_IMETHODIMP nsAccessible::AccNavigateDown(nsIAccessible **_retval) { return NS_OK;  }



  /* void addSelection (); */
NS_IMETHODIMP nsAccessible::AccAddSelection(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccAddSelection();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}

  /* void removeSelection (); */
NS_IMETHODIMP nsAccessible::AccRemoveSelection(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccRemoveSelection();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}

  /* void extendSelection (); */
NS_IMETHODIMP nsAccessible::AccExtendSelection(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccExtendSelection();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}

  /* void takeSelection (); */
NS_IMETHODIMP nsAccessible::AccTakeSelection(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccTakeSelection();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}

  /* void takeFocus (); */
NS_IMETHODIMP nsAccessible::AccTakeFocus(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccTakeFocus();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}

  /* void doDefaultAction (); */
NS_IMETHODIMP nsAccessible::AccDoDefaultAction(void) 
{ 
  // delegate
  if (mAccessible) {
    nsresult rv = mAccessible->AccDoDefaultAction();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_FAILURE;  
}


// Calculate a frame's position in screen coordinates
static nsresult
GetAbsoluteFramePosition(nsIPresContext* aPresContext,
                         nsIFrame *aFrame, 
                         nsRect& aAbsoluteTwipsRect, 
                         nsRect& aAbsolutePixelRect)
{
  //XXX: This code needs to take the view's offset into account when calculating
  //the absolute coordinate of the frame.
  nsresult rv = NS_OK;
 
  aFrame->GetRect(aAbsoluteTwipsRect);
  // zero these out, 
  // because the GetOffsetFromView figures them out
  aAbsoluteTwipsRect.x = 0;
  aAbsoluteTwipsRect.y = 0;

    // Get conversions between twips and pixels
  float t2p;
  float p2t;
  aPresContext->GetTwipsToPixels(&t2p);
  aPresContext->GetPixelsToTwips(&p2t);
  
   // Add in frame's offset from it it's containing view
  nsIView *containingView = nsnull;
  nsPoint offset;
  rv = aFrame->GetOffsetFromView(aPresContext, offset, &containingView);
  if (NS_SUCCEEDED(rv) && (nsnull != containingView)) {
    aAbsoluteTwipsRect.x += offset.x;
    aAbsoluteTwipsRect.y += offset.y;

    nsPoint viewOffset;
    containingView->GetPosition(&viewOffset.x, &viewOffset.y);
    nsIView * parent;
    containingView->GetParent(parent);

    // if we don't have a parent view then 
    // check to see if we have a widget and adjust our offset for the widget
    if (parent == nsnull) {
      nsIWidget * widget;
      containingView->GetWidget(widget);
      if (nsnull != widget) {
        // Add in the absolute offset of the widget.
        nsRect absBounds;
        nsRect lc;
        widget->WidgetToScreen(lc, absBounds);
        // Convert widget coordinates to twips   
        //aAbsoluteTwipsRect.x += NSIntPixelsToTwips(absBounds.x, p2t);
        //aAbsoluteTwipsRect.y += NSIntPixelsToTwips(absBounds.y, p2t);   
        NS_RELEASE(widget);
      }
      rv = NS_OK;
    } else {

      while (nsnull != parent) {
        nsPoint po;
        parent->GetPosition(&po.x, &po.y);
        viewOffset.x += po.x;
        viewOffset.y += po.y;
        nsIScrollableView * scrollView;
        if (NS_OK == containingView->QueryInterface(NS_GET_IID(nsIScrollableView), (void **)&scrollView)) {
          nscoord x;
          nscoord y;
          scrollView->GetScrollPosition(x, y);
          viewOffset.x -= x;
          viewOffset.y -= y;
        }
        nsIWidget * widget;
        parent->GetWidget(widget);
        if (nsnull != widget) {
          // Add in the absolute offset of the widget.
          nsRect absBounds;
          nsRect lc;
          widget->WidgetToScreen(lc, absBounds);
          // Convert widget coordinates to twips   
          aAbsoluteTwipsRect.x += NSIntPixelsToTwips(absBounds.x, p2t);
          aAbsoluteTwipsRect.y += NSIntPixelsToTwips(absBounds.y, p2t);   
          NS_RELEASE(widget);
          break;
        }
        parent->GetParent(parent);
      }
      aAbsoluteTwipsRect.x += viewOffset.x;
      aAbsoluteTwipsRect.y += viewOffset.y;
    }
  }

   // convert to pixel coordinates
  if (NS_SUCCEEDED(rv)) {
   aAbsolutePixelRect.x = NSTwipsToIntPixels(aAbsoluteTwipsRect.x, t2p);
   aAbsolutePixelRect.y = NSTwipsToIntPixels(aAbsoluteTwipsRect.y, t2p);
   aAbsolutePixelRect.width = NSTwipsToIntPixels(aAbsoluteTwipsRect.width, t2p);
   aAbsolutePixelRect.height = NSTwipsToIntPixels(aAbsoluteTwipsRect.height, t2p);
  }

  return rv;
}

nsresult nsAccessible::GetDocShellFromPS(nsIPresShell* aPresShell, nsIDocShell** aDocShell)
{
  *aDocShell = nsnull;
  if (aPresShell) {
    nsCOMPtr<nsIDocument> doc;
    aPresShell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      nsCOMPtr<nsIScriptGlobalObject> scriptObj;
      doc->GetScriptGlobalObject(getter_AddRefs(scriptObj));
      if (scriptObj) {
        scriptObj->GetDocShell(aDocShell);
        return *aDocShell != nsnull?NS_OK:NS_ERROR_FAILURE;
      }
    }
  }
  return NS_ERROR_FAILURE;
}
  
//-------------------------------------------------------
// This gets ref counted copies of the PresShell, PresContext, 
// and Root Content for a given nsIDocShell
nsresult 
nsAccessible::GetDocShellObjects(nsIDocShell*     aDocShell,
                                 nsIPresShell**   aPresShell, 
                                 nsIPresContext** aPresContext, 
                                 nsIContent**     aContent)
{
  NS_ENSURE_ARG_POINTER(aDocShell);
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aPresContext);
  NS_ENSURE_ARG_POINTER(aContent);


  aDocShell->GetPresShell(aPresShell); // this addrefs
  if (*aPresShell == nsnull) return NS_ERROR_FAILURE;

  aDocShell->GetPresContext(aPresContext); // this addrefs
  if (*aPresContext == nsnull) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc;
  (*aPresShell)->GetDocument(getter_AddRefs(doc));
  if (!doc) return NS_ERROR_FAILURE;

  *aContent = doc->GetRootContent(); // this addrefs
  return NS_OK;
}

//-------------------------------------------------------
// 
nsresult 
nsAccessible::GetDocShells(nsIPresShell* aPresShell,
                           nsIDocShell** aDocShell,
                           nsIDocShell** aParentDocShell)
{
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aDocShell);
  NS_ENSURE_ARG_POINTER(aParentDocShell);

  *aDocShell = nsnull;

  // Start by finding our PresShell and from that
  // we get our nsIDocShell in order to walk the DocShell tree
  if (NS_SUCCEEDED(GetDocShellFromPS(aPresShell, aDocShell))) {
    // Now that we have the DocShell QI 
    // it to a tree item to find it's parent
    nsCOMPtr<nsIDocShell> docShell = *aDocShell;
    nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(docShell));
    if (item) {
      nsCOMPtr<nsIDocShellTreeItem> itemParent;
      item->GetParent(getter_AddRefs(itemParent));
      // QI to get the WebShell for the parent document
      nsCOMPtr<nsIDocShell> pDocShell(do_QueryInterface(itemParent));
      if (pDocShell) {
        *aParentDocShell = pDocShell.get();
        NS_ADDREF(*aParentDocShell);
        return NS_OK;
      }
    }
  }

  NS_IF_RELEASE(*aDocShell);
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------
// 
nsresult 
nsAccessible::GetParentPresShellAndContent(nsIPresShell*  aPresShell,
                                           nsIPresShell** aParentPresShell,
                                           nsIContent**   aSubShellContent)
{
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aParentPresShell);
  NS_ENSURE_ARG_POINTER(aSubShellContent);

  *aParentPresShell = nsnull;
  *aSubShellContent = nsnull;

  nsCOMPtr<nsIDocShell> docShell;
  nsCOMPtr<nsIDocShell> parentDocShell;
  if (NS_FAILED(GetDocShells(aPresShell, 
                             getter_AddRefs(docShell), 
                             getter_AddRefs(parentDocShell)))) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPresContext> parentPresContext;
  nsCOMPtr<nsIContent> parentRootContent;
  if (NS_FAILED(GetDocShellObjects(parentDocShell, 
                                   aParentPresShell,
                                   getter_AddRefs(parentPresContext),
                                   getter_AddRefs(parentRootContent)))) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(docShell));
  if (FindContentForWebShell(*aParentPresShell, parentRootContent, 
                              webShell, aSubShellContent)) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}
 
PRBool 
nsAccessible::FindContentForWebShell(nsIPresShell* aParentPresShell,
                                     nsIContent*   aParentContent,
                                     nsIWebShell*  aWebShell,
                                     nsIContent**  aFoundContent)
{
  NS_ASSERTION(aWebShell, "Pointer is null!");
  NS_ASSERTION(aParentPresShell, "Pointer is null!");
  NS_ASSERTION(aParentContent, "Pointer is null!");
  NS_ASSERTION(aFoundContent, "Pointer is null!");

  nsCOMPtr<nsIDOMHTMLIFrameElement> iFrame(do_QueryInterface(aParentContent));
#ifdef NS_DEBUG_X
  {
    printf("** FindContent - Content %p",aParentContent);
    nsIFrame* frame;
    aParentPresShell->GetPrimaryFrameFor(aParentContent, &frame);
    if (frame) {
      char * name;
      GetNameForFrame(frame, &name);
      printf("  [%s]", name?name:"<no name>");
      if (name) nsMemory::Free(name);
    }
    printf("\n");
  }
#endif

  if (iFrame) {
    //printf("********* Found IFrame %p\n", aParentContent);
    nsCOMPtr<nsISupports> supps;
    aParentPresShell->GetSubShellFor(aParentContent, getter_AddRefs(supps));
    if (supps) {
      nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(supps));
      //printf("********* Checking %p == %p (parent)\n", webShell.get(), aWebShell);
      if (webShell.get() == aWebShell) {
        //printf("********* Found WebShell %p \n", aWebShell);
        *aFoundContent = aParentContent;
        NS_ADDREF(aParentContent);
        return PR_TRUE;
      }
    }
  }

  // walk children content
  PRInt32 count;
  aParentContent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsIContent* child;
    aParentContent->ChildAt(i, child);
    if (FindContentForWebShell(aParentPresShell, child, aWebShell, aFoundContent)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

// Calculate a frame's position in screen coordinates
nsresult
nsAccessible::GetAbsoluteFramePosition(nsIPresContext* aPresContext,
                                       nsIFrame *aFrame, 
                                       nsRect& aAbsoluteTwipsRect, 
                                       nsRect& aAbsolutePixelRect)
{
  //XXX: This code needs to take the view's offset into account when calculating
  //the absolute coordinate of the frame.
  nsresult rv = NS_OK;
 
  aFrame->GetRect(aAbsoluteTwipsRect);
  // zero these out, 
  // because the GetOffsetFromView figures them out
  aAbsoluteTwipsRect.x = 0;
  aAbsoluteTwipsRect.y = 0;

    // Get conversions between twips and pixels
  float t2p;
  float p2t;
  aPresContext->GetTwipsToPixels(&t2p);
  aPresContext->GetPixelsToTwips(&p2t);
  
   // Add in frame's offset from it it's containing view
  nsIView *containingView = nsnull;
  nsPoint offset(0,0);
  rv = aFrame->GetOffsetFromView(aPresContext, offset, &containingView);
  if (containingView == nsnull) {
    aFrame->GetView(aPresContext, &containingView);
    nsRect r;
    aFrame->GetRect(r);
    offset.x = r.x;
    offset.y = r.y;
  }
  if (NS_SUCCEEDED(rv) && (nsnull != containingView)) {
    aAbsoluteTwipsRect.x += offset.x;
    aAbsoluteTwipsRect.y += offset.y;

    nsPoint viewOffset;
    containingView->GetPosition(&viewOffset.x, &viewOffset.y);
    nsIView * parent;
    containingView->GetParent(parent);

    // if we don't have a parent view then 
    // check to see if we have a widget and adjust our offset for the widget
    if (parent == nsnull) {
      nsIWidget * widget;
      containingView->GetWidget(widget);
      if (nsnull != widget) {
        // Add in the absolute offset of the widget.
        nsRect absBounds;
        nsRect lc;
        widget->WidgetToScreen(lc, absBounds);
        // Convert widget coordinates to twips   
        aAbsoluteTwipsRect.x += NSIntPixelsToTwips(absBounds.x, p2t);
        aAbsoluteTwipsRect.y += NSIntPixelsToTwips(absBounds.y, p2t);   
        NS_RELEASE(widget);
      }
      rv = NS_OK;
    } else {

      while (nsnull != parent) {
        nsPoint po;
        parent->GetPosition(&po.x, &po.y);
        viewOffset.x += po.x;
        viewOffset.y += po.y;
        nsIScrollableView * scrollView;
        if (NS_OK == containingView->QueryInterface(NS_GET_IID(nsIScrollableView), (void **)&scrollView)) {
          nscoord x;
          nscoord y;
          scrollView->GetScrollPosition(x, y);
          viewOffset.x -= x;
          viewOffset.y -= y;
        }
        nsIWidget * widget;
        parent->GetWidget(widget);
        if (nsnull != widget) {
          // Add in the absolute offset of the widget.
          nsRect absBounds;
          nsRect lc;
          widget->WidgetToScreen(lc, absBounds);
          // Convert widget coordinates to twips   
          aAbsoluteTwipsRect.x += NSIntPixelsToTwips(absBounds.x, p2t);
          aAbsoluteTwipsRect.y += NSIntPixelsToTwips(absBounds.y, p2t);   
          NS_RELEASE(widget);
          break;
        }
        parent->GetParent(parent);
      }
      aAbsoluteTwipsRect.x += viewOffset.x;
      aAbsoluteTwipsRect.y += viewOffset.y;
    }
  }

   // convert to pixel coordinates
  if (NS_SUCCEEDED(rv)) {
   aAbsolutePixelRect.x = NSTwipsToIntPixels(aAbsoluteTwipsRect.x, t2p);
   aAbsolutePixelRect.y = NSTwipsToIntPixels(aAbsoluteTwipsRect.y, t2p);
   aAbsolutePixelRect.width = NSTwipsToIntPixels(aAbsoluteTwipsRect.width, t2p);
   aAbsolutePixelRect.height = NSTwipsToIntPixels(aAbsoluteTwipsRect.height, t2p);
  }

  return rv;
}


nsresult  
nsAccessible::GetAbsPosition(nsIPresShell* aPresShell, nsPoint& aPoint)
{
  NS_ENSURE_ARG_POINTER(aPresShell);

  nsCOMPtr<nsIPresShell> parentPresShell;
  nsCOMPtr<nsIContent> content;
  if (NS_FAILED(GetParentPresShellAndContent(aPresShell,
                                             getter_AddRefs(parentPresShell),
                                             getter_AddRefs(content)))) {
    return NS_ERROR_FAILURE;
  }

  nsIFrame* frame;
  parentPresShell->GetPrimaryFrameFor(content, &frame);
  if (frame == nsnull) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPresContext> parentPresContext;
  parentPresShell->GetPresContext(getter_AddRefs(parentPresContext));
  nsRect rect;
  if (!parentPresContext || NS_FAILED(CalcOffset(frame, parentPresContext, rect))) {
    return NS_ERROR_FAILURE;
  }

#ifdef NS_DEBUG
  printf("Frame: %p  Offset: %d,%d\n", frame, rect.x, rect.y);
#endif

  aPoint.x += rect.x;
  aPoint.y += rect.y;

  // return code here doesn't matter
  // because the last call to this will always fail 
  // when you are at the root.
  GetAbsPosition(parentPresShell, aPoint);

  return NS_OK;
}

nsresult 
nsAccessible::CalcOffset(nsIFrame* aFrame,
                         nsIPresContext * aPresContext,
                         nsRect& aRect)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  NS_ENSURE_ARG_POINTER(aPresContext);

  aRect.SetRect(0,0,0,0);

   // sum up all rects of frames with the same content node
  nsIFrame* start = aFrame;
  nsIFrame* next  = nsnull;
  start->GetNextSibling(&next);

  nsRect r;
  start->GetRect(r);
  
  while (nsFrameTreeWalker::ShouldSkip(context,nsnull, start, next))
  {
    nsRect r2;
    next->GetRect(r2);
    r.UnionRect(r,r2);
    next->GetNextSibling(&next);
  }

  nsPoint offset(r.x, r.y);

  nsIFrame* parent;
  aFrame->GetParent(&parent);

  nsPoint pos(0,0);
  while (parent) {
    // XXX hack
    nsIView* view;
    parent->GetView(aPresContext, &view);
    if (view) {
      nsIScrollableView* scrollingView;
      nsresult result = view->QueryInterface(NS_GET_IID(nsIScrollableView), (void**)&scrollingView);
      if (NS_SUCCEEDED(result)) {
        nscoord xoff = 0;
        nscoord yoff = 0;
        scrollingView->GetScrollPosition(xoff, yoff);
        offset.x -= xoff;
        offset.y -= yoff;
      }
    }

    parent->GetOrigin(pos);
    offset += pos;
    parent->GetParent(&parent);
  }

  float t2p;
  aPresContext->GetTwipsToPixels(&t2p);

  aRect.x      = PRInt32(offset.x*t2p);
  aRect.y      = PRInt32(offset.y*t2p);
  aRect.width  = PRInt32(r.width*t2p);
  aRect.height = PRInt32(r.height*t2p);

  return NS_OK;
}

  /* void accGetBounds (out long x, out long y, out long width, out long height); */
NS_IMETHODIMP nsAccessible::AccGetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(presContext);

  nsIFrame* frame = GetBoundsFrame();

  float t2p;
  presContext->GetTwipsToPixels(&t2p);

  nsIFrame* start = frame;
  nsIFrame* next  = nsnull;
  start->GetNextSibling(&next);

  nsRect rect;
  nsRect twips;

  nsRect orgRectTwips;
  nsRect unionRectTwips;
  nsRect orgRectPixels;

  start->GetRect(orgRectTwips);
  unionRectTwips = orgRectTwips;

  if (presContext && NS_SUCCEEDED(GetAbsoluteFramePosition(presContext, start, twips, rect))) {
    orgRectPixels = rect;
    orgRectPixels.x -= NSTwipsToIntPixels(orgRectTwips.x, t2p);
    orgRectPixels.y -= NSTwipsToIntPixels(orgRectTwips.y, t2p);
  }

  while (nsFrameTreeWalker::IsSameContent(start, next))
  {
    nsRect r2;
    next->GetRect(r2);
    unionRectTwips.UnionRect(unionRectTwips, r2);
    next->GetNextSibling(&next);
  }

  nsRect rectPixels;

  rectPixels.x      = NSTwipsToIntPixels(unionRectTwips.x, t2p) + orgRectPixels.x; 
  rectPixels.y      = NSTwipsToIntPixels(unionRectTwips.y, t2p) + orgRectPixels.y;
  rectPixels.width  = NSTwipsToIntPixels(unionRectTwips.width, t2p);
  rectPixels.height = NSTwipsToIntPixels(unionRectTwips.height, t2p);

  *x      = rectPixels.x;
  *y      = rectPixels.y;
  *width  = rectPixels.width;
  *height = rectPixels.height;


  return NS_OK;
}

// helpers

nsIFrame* nsAccessible::GetBoundsFrame()
{
   return GetFrame();
}

nsIFrame* nsAccessible::GetFrame()
{
   nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShell);
   nsIFrame* frame = nsnull;
   shell->GetPrimaryFrameFor(mContent, &frame);
   return frame;
}

void nsAccessible::GetPresContext(nsCOMPtr<nsIPresContext>& aContext)
{
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShell);

  if (shell) {
    shell->GetPresContext(getter_AddRefs(aContext));
  } else
    aContext = nsnull;
}

nsIAccessible* nsAccessible::CreateNewNextAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  return CreateNewAccessible(aAccessible, aContent, aShell);
}

nsIAccessible* nsAccessible::CreateNewPreviousAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  return CreateNewAccessible(aAccessible, aContent, aShell);
}

nsIAccessible* nsAccessible::CreateNewParentAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  return CreateNewAccessible(aAccessible, aContent, aShell);
}

nsIAccessible* nsAccessible::CreateNewFirstAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  return CreateNewAccessible(aAccessible, aContent, aShell);
}

nsIAccessible* nsAccessible::CreateNewLastAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  return CreateNewAccessible(aAccessible, aContent, aShell);
}

nsIAccessible* nsAccessible::CreateNewAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  NS_ASSERTION(aAccessible && aContent,"Error not accessible or content");
  return new nsAccessible(aAccessible, aContent, aShell);
}

// ------- nsHTMLBlockAccessible ------

nsHTMLBlockAccessible::nsHTMLBlockAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell):nsAccessible(aAccessible, aContent, aShell)
{

}

nsIAccessible* nsHTMLBlockAccessible::CreateNewAccessible(nsIAccessible* aAccessible, nsIContent* aContent, nsIWeakReference* aShell)
{
  NS_ASSERTION(aAccessible && aContent,"Error not accessible or content");
  return new nsHTMLBlockAccessible(aAccessible, aContent, aShell);
}

/* nsIAccessible accGetAt (in long x, in long y); */
NS_IMETHODIMP nsHTMLBlockAccessible::AccGetAt(PRInt32 tx, PRInt32 ty, nsIAccessible **_retval)
{
  PRInt32 x,y,w,h;
  AccGetBounds(&x,&y,&w,&h);
  if (tx > x && tx < x + w && ty > y && ty < y + h)
  {
    nsCOMPtr<nsIAccessible> child;
    nsCOMPtr<nsIAccessible> smallestChild;
    PRInt32 smallestArea = -1;
    nsCOMPtr<nsIAccessible> next;
    GetAccFirstChild(getter_AddRefs(child));
    PRInt32 cx,cy,cw,ch;

    while(child) {
      child->AccGetBounds(&cx,&cy,&cw,&ch);
      
      // ok if there are multiple frames the contain the point 
      // and they overlap then pick the smallest. We need to do this
      // for text frames.
      if (tx > cx && tx < cx + cw && ty > cy && ty < cy + ch) 
      {
        if (smallestArea == -1 || cw*ch < smallestArea) {
          smallestArea = cw*ch;
          smallestChild = child;
        }
      }
      child->GetAccNextSibling(getter_AddRefs(next));
      child = next;
    }

    if (smallestChild != nsnull)
    {
      *_retval = smallestChild;
      NS_ADDREF(*_retval);
      return NS_OK;
    }

    *_retval = this;
    NS_ADDREF(this);
    return NS_OK;
  }

  *_retval = nsnull;
  return NS_OK;
}





