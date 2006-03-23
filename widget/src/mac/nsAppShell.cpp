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

// 
// nsAppShell
//
// This file contains the default implementation of the application shell. Clients
// may either use this implementation or write their own. If you write your
// own, you must create a message sink to route events to. (The message sink
// interface may change, so this comment must be updated accordingly.)
//

#define RUNLOOP_IS_CARBON // RUNLOOP_IS_CARBON or RUNLOOP_IS_CFRUNLOOP
#define RUNLOOP_USES_WNE // Define with RUNLOOP_IS_CARBON to attempt to
                         // pick up EventRecords with WaitNextEvent and
                         // process them via nsMacMessagePump

#if (defined(RUNLOOP_IS_CARBON) && defined(RUNLOOP_IS_CFRUNLOOP)) || \
    (!defined(RUNLOOP_IS_CARBON) && !defined(RUNLOOP_IS_CFRUNLOOP))
#error Exactly one of RUNLOOP_IS_CARBON and RUNLOOP_IS_CFRUNLOOP must be defined
#endif

#include "nsAppShell.h"
#include "nsIAppShell.h"

#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIWidget.h"
#include "nsMacMessagePump.h"
#include "nsToolkit.h"
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>

#include <stdlib.h>

#ifdef RUNLOOP_IS_CARBON
#define kEventClassMoz 'MOZZ'
#define kEventMozLeaveRunLoop 'leev'

static const EventTypeSpec kRunLoopEventList[] = {
  { kEventClassMoz, kEventMozLeaveRunLoop },
};

static pascal OSStatus RunLoopEventHandler(EventHandlerCallRef aHandlerCallRef,
                                           EventRef aEvent,
                                           void* aUserData)
{
  // { kEventClassMoz, kEventMozLeaveRunLoop } is the only event that this
  // handler will process
  ::QuitApplicationEventLoop();
  return noErr;
}
#endif

nsAppShell::~nsAppShell()
{
#ifdef RUNLOOP_IS_CFRUNLOOP
  if (mRunLoop)
    CFRelease(mRunLoop);
#endif
}

NS_IMETHODIMP nsAppShell::Init(int* argc, char ** argv)
{
  nsresult rv = NS_GetCurrentToolkit(getter_AddRefs(mToolkit));
  if (NS_FAILED(rv))
   return rv;

#if defined(RUNLOOP_IS_CARBON) && defined(RUNLOOP_USES_WNE)
  nsIToolkit* toolkit = mToolkit.get();
  mMacPump.reset(new nsMacMessagePump(static_cast<nsToolkit*>(toolkit)));

  if (!mMacPump.get() || !nsMacMemoryCushion::EnsureMemoryCushion())
    return NS_ERROR_OUT_OF_MEMORY;
#else
  if (!nsMacMemoryCushion::EnsureMemoryCushion())
    return NS_ERROR_OUT_OF_MEMORY;
#endif

#ifdef RUNLOOP_IS_CFRUNLOOP
  mRunLoop = CFRunLoopGetCurrent();
  CFRetain(mRunLoop);
#endif

#ifdef RUNLOOP_IS_CARBON
  ::InstallApplicationEventHandler(NewEventHandlerUPP(RunLoopEventHandler),
                                   GetEventTypeCount(kRunLoopEventList),
                                   kRunLoopEventList,
                                   NULL,
                                   NULL);
#endif
  
  return nsBaseAppShell::Init(argc, argv);
}

#if 0
//-------------------------------------------------------------------------
//
// Enter a message handler loop
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Run(void)
{
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  NS_ENSURE_STATE(thread);

  nsToolkit::AppInForeground();

  mExitCalled = PR_FALSE;
  while (!mExitCalled)
    thread->RunNextTask(nsIThread::RUN_NORMAL);

  NS_RunPendingTasks(thread);

  NS_RELEASE_THIS();  // hack: see below
  return NS_OK;

#if 0
	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	mMacPump->StartRunning();
	mMacPump->DoMessagePump();

	if (mExitCalled)	// hack: see below
	{
		--mRefCnt;
		if (mRefCnt == 0)
			delete this;
	}

  return NS_OK;
#endif
}

//-------------------------------------------------------------------------
//
// Exit appshell
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Exit(void)
{
  if (!mMacPump.get())
    return NS_OK;

  Spindown();
  mExitCalled = PR_TRUE;

  NS_ADDREF_THIS(); // hack: since the applications are likely to delete us
                    // after calling this method (see nsViewerApp::Exit()),
                    // we temporarily bump the refCnt to let the message pump
                    // exit properly. The object will delete itself afterwards.
	return NS_OK;
}
#endif

#if 0
//-------------------------------------------------------------------------
//
// respond to notifications that an event queue has come or gone
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen)
{ // unnecessary; handled elsewhere
  return NS_OK;
}
#endif

#if 0
//-------------------------------------------------------------------------
//
// Prepare to process events
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Spinup(void)
{
  NS_ENSURE_TRUE(mMacPump.get(), NS_ERROR_NOT_INITIALIZED);

	mMacPump->StartRunning();
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Stop being prepared to process events.
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Spindown(void)
{
	if (mMacPump.get())
		mMacPump->StopRunning();
	return NS_OK;
}
#endif

//-------------------------------------------------------------------------
//
// nsAppShell constructor
//
//-------------------------------------------------------------------------
nsAppShell::nsAppShell()
{

  //mInitializedToolbox = PR_TRUE;
  //mRefCnt = 0;
  //mExitCalled = PR_FALSE;
}

#if 0
NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
	static EventRecord	theEvent;	// icky icky static (can't really do any better)

	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	aRealEvent = mMacPump->GetEvent(theEvent);
	aEvent = &theEvent;
	return NS_OK;
}

NS_METHOD
nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	mMacPump->DispatchEvent(aRealEvent, (EventRecord *) aEvent);
	return NS_OK;
}
#endif

//-------------------------------------------------------------------------
//
// Thread observer methods
//
//-------------------------------------------------------------------------

#if 0
#if defined(XP_MACOSX)
#if defined(MOZ_WIDGET_COCOA)
#include <CoreFoundation/CoreFoundation.h>
#define MAC_USE_CFRUNLOOPSOURCE
#elif defined(TARGET_CARBON)
/* #include <CarbonEvents.h> */
/* #define MAC_USE_CARBON_EVENT */
#include <CoreFoundation/CoreFoundation.h>
#define MAC_USE_CFRUNLOOPSOURCE
#endif
#endif

#if defined(MAC_USE_CFRUNLOOPSOURCE)
    CFRunLoopSourceRef  mRunLoopSource;
    CFRunLoopRef        mMainRunLoop;
#elif defined(MAC_USE_CARBON_EVENT)
    EventHandlerUPP     eventHandlerUPP;
    EventHandlerRef     eventHandlerRef;
#endif

static void _md_CreateEventQueue( PLEventQueue *eventQueue )
{
#if defined(MAC_USE_CFRUNLOOPSOURCE)
    CFRunLoopSourceContext sourceContext = { 0 };
    sourceContext.version = 0;
    sourceContext.info = (void*)eventQueue;
    sourceContext.perform = _md_EventReceiverProc;

    /* make a run loop source */
    eventQueue->mRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0 /* order */, &sourceContext);
    PR_ASSERT(eventQueue->mRunLoopSource);
    
    eventQueue->mMainRunLoop = CFRunLoopGetCurrent();
    CFRetain(eventQueue->mMainRunLoop);
    
    /* and add it to the run loop */
    CFRunLoopAddSource(eventQueue->mMainRunLoop, eventQueue->mRunLoopSource, kCFRunLoopCommonModes);

#elif defined(MAC_USE_CARBON_EVENT)
    eventQueue->eventHandlerUPP = NewEventHandlerUPP(_md_EventReceiverProc);
    PR_ASSERT(eventQueue->eventHandlerUPP);
    if (eventQueue->eventHandlerUPP)
    {
      EventTypeSpec     eventType;

      eventType.eventClass = kEventClassPL;
      eventType.eventKind  = kEventProcessPLEvents;

      InstallApplicationEventHandler(eventQueue->eventHandlerUPP, 1, &eventType,
                                     eventQueue, &eventQueue->eventHandlerRef);
      PR_ASSERT(eventQueue->eventHandlerRef);
    }
#endif
} /* end _md_CreateEventQueue() */

static PRStatus
_pl_NativeNotify(PLEventQueue* self)
{
#if defined(MAC_USE_CFRUNLOOPSOURCE)
  	CFRunLoopSourceSignal(self->mRunLoopSource);
  	CFRunLoopWakeUp(self->mMainRunLoop);
#elif defined(MAC_USE_CARBON_EVENT)
    OSErr err;
    EventRef newEvent;
    if (CreateEvent(NULL, kEventClassPL, kEventProcessPLEvents,
                    0, kEventAttributeNone, &newEvent) != noErr)
        return PR_FAILURE;
    err = SetEventParameter(newEvent, kEventParamPLEventQueue,
                            typeUInt32, sizeof(PREventQueue*), &self);
    if (err == noErr) {
        err = PostEventToQueue(GetMainEventQueue(), newEvent, kEventPriorityLow);
        ReleaseEvent(newEvent);
    }
    if (err != noErr)
        return PR_FAILURE;
#endif
    return PR_SUCCESS;
}
#endif

NS_IMETHODIMP nsAppShell::OnDispatchedEvent(nsIThreadInternal *thr)
{
  // post a message to the native event queue...

#if 0
  OSErr err;
  EventRef newEvent;
  if (CreateEvent(NULL, kEventClassPL, kEventProcessPLEvents,
                  0, kEventAttributeNone, &newEvent) != noErr)
      return PR_FAILURE;
    err = SetEventParameter(newEvent, kEventParamPLEventQueue,
                            typeUInt32, sizeof(PREventQueue*), &self);
    if (err == noErr) {
        err = PostEventToQueue(GetMainEventQueue(), newEvent, kEventPriorityLow);
        ReleaseEvent(newEvent);
    }
    if (err != noErr)
        return PR_FAILURE;
#endif

  printf("--- nsAppShell::OnDispatchedEvent()\n");

#ifdef RUNLOOP_IS_CFRUNLOOP
  CFRunLoopWakeUp(mRunLoop);
#endif
#ifdef RUNLOOP_IS_CARBON
  // ::QuitApplicationEventLoop(); // doesn't work from non-main thread
  EventRef event;
  ::CreateEvent(NULL, kEventClassMoz, kEventMozLeaveRunLoop,
                0, kEventAttributeNone, &event);
  ::PostEventToQueue(::GetMainEventQueue(),
                     event,
                     kEventPriorityHigh);
#endif
  return NS_OK;
}

PRBool nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  printf("--- nsAppShell::ProcessNextNativeEvent(%d)\n", mayWait);

#ifdef RUNLOOP_IS_CFRUNLOOP
  CFTimeInterval interval = 0.0;
  if (mayWait)
    interval = 10.0;  // seconds

  // @@@mm This is just here to prove that Carbon events are accumulating.
  // The real implementation should dispatch them from the run loop, it's
  // bad to dispatch them like this because the delay intervals in the run
  // loop dictate a delay in processing Carbon events.
  EventQueueRef carbonEventQueue = ::GetCurrentEventQueue();
  while (EventRef carbonEvent =
         ::AcquireFirstMatchingEventInQueue(carbonEventQueue,
                                            0,
                                            NULL,
                                            kEventQueueOptionsNone)) {
    ::SendEventToEventTarget(carbonEvent, ::GetEventDispatcherTarget());
    ::RemoveEventFromQueue(carbonEventQueue, carbonEvent);
    ::ReleaseEvent(carbonEvent);
  }

  SInt32 rv = CFRunLoopRunInMode(kCFRunLoopDefaultMode, interval, true);
  printf("--- CFRunLoopRunInMode returned [%x]\n", rv);

  return rv == kCFRunLoopRunHandledSource;
#endif

#ifdef RUNLOOP_IS_CARBON
  if (!mayWait) {
    // Only process one event (change |if| to |while| to process all pending
    // native events)
#ifndef RUNLOOP_USES_WNE
    EventQueueRef carbonEventQueue = ::GetCurrentEventQueue();
    if (EventRef carbonEvent =
         ::AcquireFirstMatchingEventInQueue(carbonEventQueue,
                                            0,
                                            NULL,
                                            kEventQueueOptionsNone)) {
      ::SendEventToEventTarget(carbonEvent, ::GetEventDispatcherTarget());
      ::RemoveEventFromQueue(carbonEventQueue, carbonEvent);
      ::ReleaseEvent(carbonEvent);
    }
#else
    // Alternate implementation.
    if (::GetNumEventsInQueue(::GetCurrentEventQueue())) {
      EventRecord eventRec;
      PRBool haveEvent = ::WaitNextEvent(everyEvent, &eventRec, 0, NULL);

      mMacPump->DispatchEvent(haveEvent, &eventRec);
    }
#endif
  }
  else {
    ::RunApplicationEventLoop();
  }

  return PR_TRUE;
#endif

}
