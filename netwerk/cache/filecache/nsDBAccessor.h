/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef _NSIDBACCESSOR_H_
#define _NSIDBACCESSOR_H_

#include "nsIDBAccessor.h"
#include "mcom_db.h"

class nsDBAccessor : public nsIDBAccessor
{
  public:
  NS_DECL_ISUPPORTS

  nsDBAccessor() ;
  virtual ~nsDBAccessor() ;
  
  NS_IMETHOD Init(nsIFileSpec* dbfile) ;
  NS_IMETHOD Shutdown(void) ; 

  NS_IMETHOD Put(PRInt32 aID, void* anEntry, PRUint32 aLength) ;

  NS_IMETHOD Get(PRInt32 aID, void** anEntry, PRUint32 *aLength) ;

  NS_IMETHOD Del(PRInt32 aID) ;

  NS_IMETHOD GenID(const char* key, PRUint32 length, PRInt32* aID) ;

  NS_IMETHOD EnumEntry(void* *anEntry, PRUint32* aLength, PRBool bReset) ;

  protected:

  private:
  DB *                          mDB ;
  char *                        mDBFilename ;
  PRLock *                      m_Lock ;
} ;

#endif // _NSIDBACCESSOR_H_
