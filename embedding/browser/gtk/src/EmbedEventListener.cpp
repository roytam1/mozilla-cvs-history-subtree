/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 tw=80 et cindent: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Christopher Blizzard. Portions created by Christopher Blizzard are Copyright (C) Christopher Blizzard.  All Rights Reserved.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Oleg Romashin <romaxa@gmail.com>
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

#include <nsCOMPtr.h>
#include <nsIDOMMouseEvent.h>

#include <nsIDOMNSEvent.h>
#include "nsIDOMKeyEvent.h"
#include "nsIDOMUIEvent.h"

#include "EmbedEventListener.h"
#include "EmbedPrivate.h"
#include "gtkmozembed_internal.h"

static PRInt32 sLongPressTimer = 0, mLongMPressDelay = 1000;
static PRInt32 sX = 0, sY = 0;
static PRBool  sMPressed = PR_FALSE, sIsScrolling = PR_FALSE;

EmbedEventListener::EmbedEventListener(void)
{
  mOwner = nsnull;
}

EmbedEventListener::~EmbedEventListener()
{
}

NS_IMPL_ADDREF(EmbedEventListener)
NS_IMPL_RELEASE(EmbedEventListener)
NS_INTERFACE_MAP_BEGIN(EmbedEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
NS_INTERFACE_MAP_END

nsresult
EmbedEventListener::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  mCtxInfo = new EmbedContextMenuInfo(aOwner);
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::HandleEvent(nsIDOMEvent* aDOMEvent)
{
  nsString eventType;
  aDOMEvent->GetType(eventType);
  if (eventType.EqualsLiteral ("focus"))
    if (mCtxInfo->GetFormControlType(aDOMEvent)) {
      if (mCtxInfo->mEmbedCtxType & GTK_MOZ_EMBED_CTX_INPUT)
      {
        gint return_val = FALSE;
        gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                        moz_embed_signals[DOM_FOCUS],
                        (void *)aDOMEvent, &return_val);
        if (return_val) {
          aDOMEvent->StopPropagation();
          aDOMEvent->PreventDefault();
        }
      }
    }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyDown(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // Return FALSE to this function to mark the event as not
  // consumed...
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_DOWN],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyUp(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // return FALSE to this function to mark this event as not
  // consumed...
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_UP],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } else {
    //mCtxInfo->UpdateContextData(aDOMEvent);
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::KeyPress(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMKeyEvent> keyEvent;
  keyEvent = do_QueryInterface(aDOMEvent);
  if (!keyEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_KEY_PRESS],
                  (void *)keyEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

static gboolean
sLongMPress(void *aOwningWidget)
{
  // Return TRUE from your signal handler to mark the event as consumed.
  if (!sMPressed || sIsScrolling)
    return FALSE;
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(aOwningWidget),
                  moz_embed_signals[DOM_MOUSE_LONG_PRESS],
                  (void *)0, &return_val);
  if (return_val) {
    sMPressed = PR_FALSE;
  }
  return FALSE;
}

NS_IMETHODIMP
EmbedEventListener::MouseDown(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  sMPressed = PR_TRUE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_DOWN],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    sMPressed = PR_FALSE;
    if (sLongPressTimer)
      g_source_remove (sLongPressTimer);
//FIXME
//    aDOMEvent->StopPropagation();
//    aDOMEvent->PreventDefault();
  } else {
    sLongPressTimer = g_timeout_add(mLongMPressDelay, sLongMPress, mOwner->mOwningWidget);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenX(&sX);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenY(&sY);        
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseUp(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  if (sLongPressTimer)
    g_source_remove (sLongPressTimer);
  sMPressed = PR_FALSE;
  mOwner->mOpenBlock = sIsScrolling;
  sIsScrolling = sMPressed;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_UP],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseClick(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_CLICK],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseDblClick(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  if (sLongPressTimer)
    g_source_remove (sLongPressTimer);
  sMPressed = PR_FALSE;
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_DBL_CLICK],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } 
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOver(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_OVER],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  } else {
    //mCtxInfo->UpdateContextData(aDOMEvent);
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseOut(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMMouseEvent> mouseEvent;
  mouseEvent = do_QueryInterface(aDOMEvent);
  if (!mouseEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_MOUSE_OUT],
                  (void *)mouseEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::Activate(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_ACTIVATE],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusIn(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_FOCUS_IN],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::FocusOut(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr <nsIDOMUIEvent> uiEvent = do_QueryInterface(aDOMEvent);
  if (!uiEvent)
    return NS_OK;
  // Return TRUE from your signal handler to mark the event as consumed.
  gint return_val = FALSE;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[DOM_FOCUS_OUT],
                  (void *)uiEvent, &return_val);
  if (return_val) {
    aDOMEvent->StopPropagation();
    aDOMEvent->PreventDefault();
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::MouseMove(nsIDOMEvent* aDOMEvent)
{
  if (sMPressed && 
      gtk_signal_handler_pending(GTK_OBJECT(mOwner->mOwningWidget),
                                 moz_embed_signals[DOM_MOUSE_SCROLL],
				 TRUE)) {
    // Return TRUE from your signal handler to mark the event as consumed.
    nsCOMPtr <nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aDOMEvent);
    if (!mouseEvent)
      return NS_OK;
    PRInt32  newX, newY, subX, subY;
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenX(&newX);
    ((nsIDOMMouseEvent*)mouseEvent)->GetScreenY(&newY);
    subX = newX - sX;
    subY = newY - sY;
    nsresult rv = NS_OK;
    if (ABS(subX) > 10 || ABS(subY) > 10 || (sIsScrolling && sMPressed)) {
      if (!sIsScrolling) {
        gint return_val = FALSE;
        gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                        moz_embed_signals[DOM_MOUSE_SCROLL],
                        (void *)mouseEvent, &return_val);
        if (!return_val) {
          sIsScrolling = PR_TRUE;
          if (mCtxInfo)
            rv = mCtxInfo->GetElementForScroll(aDOMEvent);
        } else {
          sMPressed = PR_FALSE;
          sIsScrolling = PR_FALSE;
        }
      }
      if (sIsScrolling) 
      {
        if (sLongPressTimer)
          g_source_remove (sLongPressTimer);
        if (mCtxInfo->mNSHHTMLElementSc) {
          PRInt32 x, y;
          mCtxInfo->mNSHHTMLElementSc->GetScrollTop(&y);
          mCtxInfo->mNSHHTMLElementSc->GetScrollLeft(&x);
#ifdef MOZ_SCROLL_TOP_LEFT_HACK
          rv = mCtxInfo->mNSHHTMLElementSc->ScrollTopLeft (y - subY, x - subX);
#endif
        } else {
          rv = NS_ERROR_UNEXPECTED;
        }
        if (rv == NS_ERROR_UNEXPECTED) {
          nsCOMPtr<nsIDOMWindow> DOMWindow;
          nsIWebBrowser *webBrowser = nsnull;
          gtk_moz_embed_get_nsIWebBrowser(mOwner->mOwningWidget, &webBrowser);
          webBrowser->GetContentDOMWindow(getter_AddRefs(DOMWindow));
          DOMWindow->ScrollBy(-subX, -subY);
        }
      }
      sX = newX;
      sY = newY;
      sIsScrolling = sMPressed;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedEventListener::DragMove(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}
