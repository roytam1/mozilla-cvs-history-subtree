/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/* vim:set ts=2 sw=2 sts=2 ci et: */
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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *   Mark Mentovai <mark@moxienet.com>
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

#include "nsAppShell2.h"
#include "nsToolkit.h"

/*static*/ void
nsAppShell::EventReceiverProc(void *info)
{
  nsAppShell *self = NS_STATIC_CAST(nsAppShell *, info);
  self->NativeEventCallback();
  NS_RELEASE(self);
}

nsresult
nsAppShell::Init()
{
  nsresult rv = NS_GetCurrentToolkit(getter_AddRefs(mToolkit));
  if (NS_FAILED(rv))
    return rv;

  nsIToolkit *toolkit = mToolkit.get();
  mMacPump = new nsMacMessagePump(static_cast<nsToolkit*>(toolkit));
  if (!mMacPump.get() || !nsMacMemoryCushion::EnsureMemoryCushion())
    return NS_ERROR_OUT_OF_MEMORY;

  // XXX Mark the pump as running (probably not correct for embedding)
  mMacPump->StartRunning();

  CFRunLoopSourceContext context = {0};
  context.info = this;
  context.perform = EventReceiverProc;
  
  mRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
  NS_ENSURE_STATE(mRunLoopSource);

  mRunLoop = CFRunLoopGetCurrent();
  CFRetain(mRunLoop);

  CFRunLoopAddSource(mRunLoop, mRunLoopSource, kCFRunLoopCommonModes);

  return nsBaseAppShell::Init();
}

void
nsAppShell::CallFromNativeEvent()
{
  NS_ADDREF_THIS();
  CFRunLoopSourceSignal(mRunLoopSource);
  CFRunLoopWakeUp(mRunLoop);
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  NS_ENSURE_STATE(mMacPump);

  EventRecord event;
  PRBool hasEvent = mMacPump->GetEvent(event, mayWait);
  if (!hasEvent && !mayWait)
    return PR_FALSE;

  mMacPump->DispatchEvent(hasEvent, &event);
  return hasEvent;
}

nsAppShell::~nsAppShell()
{
  if (mRunLoopSource) {
    CFRunLoopRemoveSource(mRunLoop, mRunLoopSource, kCFRunLoopCommonModes);
    CFRelease(mRunLoopSource);
    CFRelease(mRunLoop);
  }
}
