// ****************************************************************************
//
// Copyright (c) 2003-2004 America Online, Inc.  All rights reserved.
// This software contains valuable confidential and proprietary information
// of America Online, Inc. and is subject to applicable licensing agreements.
// Unauthorized reproduction, transmission or distribution of this file and
// its contents is a violation of applicable laws.
//
//           A M E R I C A   O N L I N E   C O N F I D E N T I A L
//
// ****************************************************************************
#include "nspr.h"

#include "pk11func.h"
#include "nsNSSComponent.h"
#include "nsSmartCardMonitor.h"

//
// The SmartCard monitoring thread should start up for each module we load
// that has removable tokens. This code calls an NSS function which waits
// until there is a change in the token state. NSS uses the 
// C_WaitForSlotEvent() call in PKCS #11 if  the token implements the call,
// otherwise NSS will pull the token in a loop with a delay of 'latency' 
// between pulls. Note that the C_WaitForSlotEvent() may wake up on any type
// of token event, so it's necessary to filter these events down to just the
// insertion and removal events we are looking for.
//
// Once the event is found, It is passed to nsNSSComponent for dispatching
// on the UI thread, and forwarding to any interested listeners (including
// javascript).
//


static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

#include <assert.h>

// self linking an removing double linked entry
// adopts the thread it is passed.
class SmartCardThreadEntry {
public:
   SmartCardThreadEntry *next;
   SmartCardThreadEntry *prev;
   SmartCardThreadEntry **head;
   SmartCardMonitoringThread *thread;
   SmartCardThreadEntry(SmartCardMonitoringThread *thread_,
    SmartCardThreadEntry *next_, SmartCardThreadEntry *prev_,
    SmartCardThreadEntry **head_) : 
    next(next_), prev(prev_), head(head_), thread(thread_) { 
     if (prev) { prev->next = this; } else { *head = this; }
     if (next) { next->prev = this; }
    }
   ~SmartCardThreadEntry() {
     if (prev) { prev->next = next; } else { *head = next; }
     if (next) { next->prev = prev; }
     // NOTE: automatically stops the thread
     delete thread;
    }
};

//
// SmartCardThreadList is a class to help manage the running threads.
// That way new thread could be started and old ones terminated as we
// load and unload modules.
//
SmartCardThreadList::SmartCardThreadList() : head(0)
{
}

SmartCardThreadList::~SmartCardThreadList()
{
   while (head) {
	delete head;
   }
}

void
SmartCardThreadList::Remove(SECMODModule *aModule)
{
  SmartCardThreadEntry *current;
  for (current = head; current; current=current->next) {
    if (current->thread->GetModule() == aModule) {
      // NOTE: automatically stops the thread and dequeues
      delete current;
      return;
    }
  }
}

// adopts the thread passwd to it. Starts the thread as well
void 
SmartCardThreadList::Add(SmartCardMonitoringThread *thread)
{
  SmartCardThreadEntry *current = new SmartCardThreadEntry(thread, head, NULL,
				&head);
  if (current) {  
    thread->Start();
  }
  // OK to forget current here, it's on the list
}


// We really should have a Unity PL Hash function...
static PR_CALLBACK PLHashNumber
unity(const void *key) { return (PLHashNumber)key; }

SmartCardMonitoringThread::SmartCardMonitoringThread(SECMODModule *module_)
  : mThread(NULL)
{
  mModule = SECMOD_ReferenceModule(module_);
  // simple hash functions, most modules have less than 3 slots, so 10 buckets
  // should be plenty
  mHash = PL_NewHashTable(10, unity, PL_CompareValues, 
					PL_CompareStrings, nsnull, 0);
}

//
// when we shutdown the thread, be sure to stop it first. If not, it just might
// crash when the mModule it is looking at disappears.
//
SmartCardMonitoringThread::~SmartCardMonitoringThread()
{
  Stop();
  SECMOD_DestroyModule(mModule);
  if (mHash) {
    PL_HashTableDestroy(mHash);
  }
}

void SmartCardMonitoringThread::Start()
{
  if (!mThread) {
    mThread = PR_CreateThread(PR_SYSTEM_THREAD, LaunchExecute, this,
                              PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
                              PR_JOINABLE_THREAD, 0);
  }
}

//
// Should only stop if we are through with the module.
// CancelWait has the side effect of losing all the keys and
// current operations on the module!. (See the comment in
// SECMOD_CancelWait for why this is so..).
//
void SmartCardMonitoringThread::Stop()
{
  SECStatus rv;

  rv = SECMOD_CancelWait(mModule);
  if (rv != SECSuccess) {
    // we didn't wake up the Wait, so don't try to join the thread 
    // otherwise we will hang forever...
    return;
  }
  
  if (mThread) {
    PR_JoinThread(mThread);
    mThread = 0; 
  }
}

//
// remember the name and series of a token in a particular slot.
// This is important because the name is no longer available when
// the token is removed. If listeners depended on this information,
// the would be out of luck. It also is a handy way of making sure
// we don't generate spurious insertion and removal events as the slot
// cycles through various states.
//
void
SmartCardMonitoringThread::SetTokenName(CK_SLOT_ID slotid, 
				const char *tokenName, PRUint32 series)
{
  if (mHash) {
    if (tokenName) {
      int len=strlen(tokenName)+1;
      char *entry= (char *)malloc(len+sizeof(PRUint32));
     
      if (entry) {  
	memcpy(entry,&series,sizeof(PRUint32));
	memcpy(&entry[sizeof(PRUint32)],tokenName,len);
	
	PL_HashTableAdd(mHash,(void *)slotid, entry); /* adopt */
	return;
      }
    } 
    // if tokenName was not provided, remove the old one (implicit delete)
    PL_HashTableRemove(mHash,(void *)slotid);
  }
	
}

// retrieve the name saved above
const char *
SmartCardMonitoringThread::GetTokenName(CK_SLOT_ID slotid)
{
  const char *tokenName = NULL;
  const char *entry;

  if (mHash) {
    entry = (const char *)PL_HashTableLookupConst(mHash,(void *)slotid);
    if (entry) {
      tokenName = &entry[sizeof(PRUint32)];
    }
  }
  return tokenName;
}

// retrieve the series saved in SetTokenName above
PRUint32
SmartCardMonitoringThread::GetTokenSeries(CK_SLOT_ID slotid)
{
  PRUint32 series = 0;
  const char *entry;

  if (mHash) {
    entry = (const char *)PL_HashTableLookupConst(mHash,(void *)slotid);
    if (entry) {
      memcpy(&series,entry,sizeof(PRUint32));
    }
  }
  return series;
}

//
// helper function to pass the event off to nsNSSComponent.
//
nsresult
SmartCardMonitoringThread::SendEvent(const nsAString &eventType,
						const char *tokenName)
{
  nsresult rv;
  nsCOMPtr<nsINSSComponent> 
		nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nssComponent->PostEvent(eventType, tokenName);
  return NS_OK;
}

//
// This is the main loop.
//
void SmartCardMonitoringThread::Execute()
{
  PK11SlotInfo *slot;
  const char *tokenName = NULL;

  //
  // populate token names for already inserted tokens.
  //
  PK11SlotList *sl =
	PK11_FindSlotsByNames(mModule->dllName, nsnull, nsnull, PR_TRUE);
  PK11SlotListElement *sle;
 
  if (sl) {
    for (sle=PK11_GetFirstSafe(sl); sle; 
				      sle=PK11_GetNextSafe(sl,sle,PR_FALSE)) {
      SetTokenName(PK11_GetSlotID(sle->slot), 
		PK11_GetTokenName(sle->slot), PK11_GetSlotSeries(sle->slot));
    }
    PK11_FreeSlotList(sl);
  }

  // loop starts..
  do {
    slot = SECMOD_WaitForAnyTokenEvent(mModule, 0, PR_SecondsToInterval(1)  );
    if (slot == NULL) {
      break;
    }

    // now we have a potential insertion or removal event see if the slot
    // is present to determine which it is...
    if (PK11_IsPresent(slot)) {
      // insertion
      CK_SLOT_ID slotID = PK11_GetSlotID(slot);
      PRUint32 series = PK11_GetSlotSeries(slot);

      // skip spurious insertion events...
      if (series != GetTokenSeries(slotID)) {
	// if there's a token name, then we have not yet issued a remove
	// event for the previous token, do so now...
        tokenName = GetTokenName(slotID);
        if (tokenName) {
#ifdef DEBUG
          fprintf (stderr,"Token Virtual Removed (%s) series=%d\n",
			tokenName, GetTokenName(slotID));
#endif
          SendEvent(NS_LITERAL_STRING(SMARTCARD_REMOVE), tokenName);
        }
        tokenName = PK11_GetTokenName(slot);
        // save the token name and series
        SetTokenName(PK11_GetSlotID(slot), tokenName, series);
        SendEvent(NS_LITERAL_STRING(SMARTCARD_INSERT), tokenName);
#ifdef DEBUG
        fprintf (stderr,"Token Inserted (%s) series=%d\n", tokenName, series);
#endif
      }
    } else {
      // retrieve token name 
      tokenName = GetTokenName(PK11_GetSlotID(slot));
      // if there's not a token name, then the software isn't expecting
      // a (or another) remove event.
      if (tokenName) {
	// clear the token name
        SetTokenName(PK11_GetSlotID(slot), NULL, 0);
        SendEvent(NS_LITERAL_STRING(SMARTCARD_REMOVE), tokenName);
#ifdef DEBUG
        fprintf (stderr,"Token Removed (%s)\n", tokenName);
#endif
      }
    }
    PK11_FreeSlot(slot);

  } while (1);
}

// accessor to help searching active Monitoring threads
const SECMODModule * SmartCardMonitoringThread::GetModule() 
{
  return mModule;
}

// C-like calling sequence to glue into PR_CreateThread.
void SmartCardMonitoringThread::LaunchExecute(void *arg)
{
  ((SmartCardMonitoringThread*)arg)->Execute();
}

