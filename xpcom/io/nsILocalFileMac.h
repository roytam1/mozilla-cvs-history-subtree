/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Steve Dagley <sdagley@netscape.com>
 */

// Mac specific interfaces for nsLocalFileMac

#ifndef _nsILocalFileMAC_H_
#define _nsILocalFileMAC_H_

#include "nsISupports.h"

#include <Files.h>


#define NS_ILOCALFILEMAC_IID_STR "614c3010-1dd2-11b2-be04-bcd57a64ffc9"

#define NS_ILOCALFILEMAC_IID \
  {0x614c3010, 0x1dd2, 0x11b2, \
    { 0xbe, 0x04, 0xbc, 0xd5, 0x7a, 0x64, 0xcc, 0xc9 }}


class nsILocalFileMac : public nsISupports
{
 public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ILOCALFILEMAC_IID)
  
  // Since the OS native way to represent a file on the Mac is an FSSpec
  // we provide a way to initialize an nsLocalFile with one
  NS_IMETHOD InitWithFSSpec(const FSSpec *fileSpec) = 0;
  
  // In case we need to get the FSSpec at the heart of an nsLocalFIleMac
  NS_IMETHOD GetFSSpec(FSSpec *fileSpec) = 0;

  // Get/Set methods for the file type
  NS_IMETHOD GetType(OSType *type) = 0;
  NS_IMETHOD SetType(OSType type) = 0;

  // Get/Set methods for the file creator
  NS_IMETHOD GetCreator(OSType *creator) = 0;
  NS_IMETHOD SetCreator(OSType creator) = 0;
};

#endif
