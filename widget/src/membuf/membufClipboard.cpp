/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Hartshorn <peter@igelaus.com.au>
 *   Ken Faulkner <faulkner@igelaus.com.au>
 *   Dan Rosen <dr@netscape.com>
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* TODO:
 * Currently this only supports the transfer of TEXT! FIXME
 */

#include "membufAppShell.h"
#include "membufClipboard.h"

#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

#include "nsTextFormatter.h"
#include "nsVoidArray.h"

#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
// unicode conversion
#  include "nsIPlatformCharset.h"


// The class statics:
nsITransferable            *membufClipboard::mTransferable = nsnull;

#if defined(DEBUG_mcafee) || defined(DEBUG_pavlov)
#define DEBUG_CLIPBOARD
#endif

NS_IMPL_ISUPPORTS1(membufClipboard, nsIClipboard)

membufClipboard::membufClipboard() {
  Init();
}

membufClipboard::~membufClipboard() {
}

/*static*/ void membufClipboard::Shutdown() {
  NS_IF_RELEASE(mTransferable);
}

// Initialize the clipboard and create a nsWidget for communications

void membufClipboard::Init() {
}

// This is the callback function for our nsWidget. It is given the
// XEvent from nsAppShell.
// FIXME: We _should_ assign mTransferable here depending on if its a
// selectionrequest
nsEventStatus PR_CALLBACK membufClipboard::Callback(nsGUIEvent *event) {
  return nsEventStatus_eIgnore;
}

nsITransferable *membufClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
  return 0;
}

// Ripped from GTK. Does the writing to the appropriate transferable.
// Does *some* flavour stuff, but not all!!!
// FIXME: Needs to be completed.
NS_IMETHODIMP membufClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  return NS_OK;
}

NS_IMETHODIMP membufClipboard::SetData(nsITransferable *aTransferable,
                                   nsIClipboardOwner *anOwner,
                                   PRInt32 aWhichClipboard)
{
  return NS_OK;
}

NS_IMETHODIMP membufClipboard::GetData(nsITransferable *aTransferable,
                                   PRInt32 aWhichClipboard)
{
  return NS_OK;
}

NS_IMETHODIMP membufClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
  return NS_OK;
}

NS_IMETHODIMP membufClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList,
                                                  PRInt32 aWhichClipboard,
                                                  PRBool *_retval) {
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP membufClipboard::SupportsSelectionClipboard(PRBool *_retval) {
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE; // we support the selection clipboard on unix.
  return NS_OK;
}
