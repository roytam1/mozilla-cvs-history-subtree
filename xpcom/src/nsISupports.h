/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef nsISupports_h___
#define nsISupports_h___

#include "nsDebug.h"
#include "nsID.h"
#include "nsError.h"

// An "interface id" which can be used to uniquely identify a given
// interface.

typedef nsID nsIID;

#define REFNSIID const nsIID&

// Define an IID
#define NS_DEFINE_IID(_name, _iidspec) \
  const nsIID _name = _iidspec

#ifdef NS_IMPL_IDS
#define NS_DECLARE_ID(_name,m0,m1,m2,m30,m31,m32,m33,m34,m35,m36,m37) \
  extern "C" const nsID _name = {m0,m1,m2,{m30,m31,m32,m33,m34,m35,m36,m37}}
#else
#define NS_DECLARE_ID(_name,m0,m1,m2,m30,m31,m32,m33,m34,m35,m36,m37) \
  extern "C" const nsID _name
#endif

//----------------------------------------------------------------------

// IID for the nsISupports interface
// {00000000-0000-0000-c000-000000000046}
#define NS_ISUPPORTS_IID      \
{ 0x00000000, 0x0000, 0x0000, \
  {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} }

// Generic result data type
typedef PRUint32 nsresult;

// Reference count values
typedef PRUint32 nsrefcnt;

// Basic component object model interface. Objects which implement
// this interface support runtime interface discovery (QueryInterface)
// and a reference counted memory model (AddRef/Release). This is
// modelled after the win32 IUnknown API.
class nsISupports {
public:
  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr) = 0;
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

//----------------------------------------------------------------------

// Some convenience macros for implementing AddRef and Release

// Declare the reference count variable and the implementations of the
// AddRef and QueryInterface methods.
#define NS_DECL_ISUPPORTS                                                   \
public:                                                                     \
  NS_IMETHOD QueryInterface(REFNSIID aIID,                                  \
                            void** aInstancePtr);                           \
  NS_IMETHOD_(nsrefcnt) AddRef(void);                                       \
  NS_IMETHOD_(nsrefcnt) Release(void);                                      \
protected:                                                                  \
  nsrefcnt mRefCnt;                                                         \
public:

// Initialize the reference count variable. Add this to each and every
// constructor you implement.
#define NS_INIT_REFCNT() mRefCnt = 0

// Use this macro to implement the AddRef method for a given _class
#define NS_IMPL_ADDREF(_class)                               \
nsrefcnt _class::AddRef(void)                                \
{                                                            \
  return ++mRefCnt;                                          \
}

#define NS_ADDREF(_ptr) \
 (_ptr)->AddRef()

#define NS_IF_ADDREF(_ptr)  \
((0 != (_ptr)) ? (_ptr)->AddRef() : 0);

#define NS_RELEASE(_ptr) \
 (_ptr)->Release();      \
 (_ptr) = NULL

#define NS_IF_RELEASE(_ptr)             \
 ((0 != (_ptr)) ? (_ptr)->Release() : 0); \
 (_ptr) = NULL

// Use this macro to implement the Release method for a given _class
#define NS_IMPL_RELEASE(_class)                        \
nsrefcnt _class::Release(void)                         \
{                                                      \
  if (--mRefCnt == 0) {                                \
    delete this;                                       \
    return 0;                                          \
  }                                                    \
  return mRefCnt;                                      \
}

//----------------------------------------------------------------------

// Some convenience macros for implementing QueryInterface

// This implements query interface with two assumptions: First, the
// class in question implements nsISupports and it's own interface and
// nothing else. Second, the implementation of the class's primary
// inheritance chain leads to it's own interface.
//
// _class is the name of the class implementing the method
// _classiiddef is the name of the #define symbol that defines the IID
// for the class (e.g. NS_ISUPPORTS_IID)
#define NS_IMPL_QUERY_INTERFACE(_class,_classiiddef)                     \
nsresult _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
{                                                                        \
  if (NULL == aInstancePtr) {                                            \
    return NS_ERROR_NULL_POINTER;                                        \
  }                                                                      \
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 \
  static NS_DEFINE_IID(kClassIID, _classiiddef);                         \
  if (aIID.Equals(kClassIID)) {                                          \
    *aInstancePtr = (void*) this;                                        \
    AddRef();                                                            \
    return NS_OK;                                                        \
  }                                                                      \
  if (aIID.Equals(kISupportsIID)) {                                      \
    *aInstancePtr = (void*) ((nsISupports*)this);                        \
    AddRef();                                                            \
    return NS_OK;                                                        \
  }                                                                      \
  return NS_NOINTERFACE;                                                 \
}

#define NS_IMPL_ISUPPORTS(_class,_classiiddef) \
  NS_IMPL_ADDREF(_class)                       \
  NS_IMPL_RELEASE(_class)                      \
  NS_IMPL_QUERY_INTERFACE(_class,_classiiddef)

#endif /* nsISupports_h___ */
