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

#ifndef _NSSMARTCARDMONITOR_
#define _NSSMARTCARDMONITOR_

#include "prthread.h"
#include "secmod.h"
#include "plhash.h"
#include "pkcs11t.h"

class SmartCardThreadEntry;
class SmartCardMonitoringThread;

//
// manage a group of SmartCardMonitoringThreads
//
class SmartCardThreadList {
public:
  SmartCardThreadList();
  ~SmartCardThreadList();
  void Remove(SECMODModule *module);
  void Add(SmartCardMonitoringThread *thread);
private:
  SmartCardThreadEntry *head;
};

//
// monitor a Module for token insertion and removal
//
// NOTE: this provides the application the ability to dynamically add slots
// on the fly as necessary.
//
class SmartCardMonitoringThread
{
 public:
  SmartCardMonitoringThread(SECMODModule *module);
  ~SmartCardMonitoringThread();
  
  void Start();
  void Stop();
  
  void Execute();
  void Interrupt();
  
  const SECMODModule *GetModule();

 private:

  static void LaunchExecute(void *arg);
  void SetTokenName(CK_SLOT_ID slotid, const char *tokenName, PRUint32 series);
  const char *GetTokenName(CK_SLOT_ID slotid);
  PRUint32 GetTokenSeries(CK_SLOT_ID slotid);
  nsresult SendEvent(const nsAString &type,const char *tokenName);
  
  
  SECMODModule *mModule;
  PLHashTable *mHash;
  PRThread* mThread;
};

#define SMARTCARD_INSERT "smartcard-insert"
#define SMARTCARD_REMOVE "smartcard-remove"

#endif
