/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsBoxFrame.h"
#include "nsIXTFXULVisual.h"
#include "nsIDOMElement.h"

typedef nsBoxFrame nsXTFXULDisplayFrameBase;

class nsXTFXULDisplayFrame : public nsXTFXULDisplayFrameBase
{
public:
  friend nsresult
  NS_NewXTFXULDisplayFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
  
  nsXTFXULDisplayFrame(nsIPresShell* aPresShell);

  virtual nsIFrame* GetContentInsertionFrame();
  
private:
  nsIPresShell* mPresShell; // XXX should get rid of this; make
                            // GetContentInsertionFrame take as arg
                            // instead
};


//----------------------------------------------------------------------

nsresult
NS_NewXTFXULDisplayFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  nsXTFXULDisplayFrame* it = new (aPresShell) nsXTFXULDisplayFrame(aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;

  return NS_OK;
}

nsXTFXULDisplayFrame::nsXTFXULDisplayFrame(nsIPresShell* aPresShell)
    : nsXTFXULDisplayFrameBase(aPresShell, PR_FALSE, nsnull),
      mPresShell(aPresShell)
{}

nsIFrame*
nsXTFXULDisplayFrame::GetContentInsertionFrame()
{
  nsCOMPtr<nsIXTFXULVisual> visual = do_QueryInterface(mContent);
  if (!visual) {
    NS_ERROR("xtf element doesn't implement required interface");
    return nsnull;
  }
  
  nsCOMPtr<nsIDOMElement> childInsertionPoint;
  visual->GetInsertionPoint(getter_AddRefs(childInsertionPoint));
  if (!childInsertionPoint) return nsnull; // we don't take visual child content
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(childInsertionPoint);
  NS_ASSERTION(content, "element not implementing nsIContent!?");

  nsIFrame* insertionFrame = nsnull;
  mPresShell->GetPrimaryFrameFor(content, &insertionFrame);
  return insertionFrame;
}
