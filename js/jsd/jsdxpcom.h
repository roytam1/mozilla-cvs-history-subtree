/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*
 * Header for JavaScript Debugging support - All xpcom interfaces
 */

#ifndef jsdxpcom_h___
#define jsdxpcom_h___
#ifndef __cplusplus
#error jsdxpcom.h is a C++ only header file
#endif

#include "nsISupports.h"
#include "nsIFactory.h"

/* Get jstypes.h included first. After that we can use JS macros for doing
*  this extern "C" stuff!
*/
extern "C"
{
#include "jstypes.h"
}

JS_BEGIN_EXTERN_C
#include "jsdenums.h"
#include "jsapi.h"
JS_END_EXTERN_C


// forward declarations...

class nsIJSDRuntimeClass;
class nsIJSDRuntime;
class nsIJSDScript;
class nsIJSDScriptHook;
class nsIJSDScriptIterator;
class nsIJSDRuntimeIterator;

class nsIJSDThreadState;


typedef void JSDThread;


///////////////////////////////////
// XXX add uuids
///////////////////////////////////


class nsIJSDRuntimeClass : nsIFactory {
public:

    NS_IMETHOD
    GetMajorVersion(uintN* version) = 0;

    NS_IMETHOD
    GetMinorVersion(uintN* version) = 0;

    NS_IMETHOD
    CreateJSDRuntime(/* out */ nsIJSDRuntime** jsdrt) = 0;

    NS_IMETHOD
    LockRuntimes(/* in */ JSBool lock) = 0;

    NS_IMETHOD
    GetRuntimeIterator(/* out */  nsIJSDRuntimeIterator** iter) = 0;

    NS_IMETHOD
    FindJSDRuntimeForJSContext(/* in */  JSContext* cx, 
                               /* out */ nsIJSDRuntime** jsdrt) = 0;

    NS_IMETHOD
    FindJSDRuntimeForJSRuntime(/* in */  JSRuntime* jsrt, 
                               /* out */ nsIJSDRuntime** jsdrt) = 0;
};

class nsIJSDRuntime : public nsISupports {
public:

    /////////////////////////////////////
    // High level methods

    // XXX use C JSAPI or plan for XPCOM JSAPI?
    NS_IMETHOD
    Attach(/* in */ JSRuntime* jsrt) = 0;

    NS_IMETHOD
    Detach(void) = 0;

    NS_IMETHOD
    GetJSRuntime(/* out */ JSRuntime** jsrt) = 0;

    NS_IMETHOD
    GetDefaultJSContext(/* out */ JSContext** cx) = 0;

    /////////////////////////////////////
    // script methods

    NS_IMETHOD
    LockScriptSubsystem(/* in */ JSBool lock) = 0;

    NS_IMETHOD
    GetScriptIterator(/* out */  nsIJSDScriptIterator** iter) = 0;

    NS_IMETHOD
    SetScriptHook(/* in */ nsIJSDScriptHook* hook) = 0;

    NS_IMETHOD
    GetScriptHook(/* out */ nsIJSDScriptHook** hook) = 0;


};

class nsIJSDScript : public nsISupports {
public:

    NS_IMETHOD
    IsActive(/* out */ JSBool* active) = 0;
    
    NS_IMETHOD
    GetFilename(/* out */ const char** name) = 0;

    NS_IMETHOD
    GetFunctionName(/* out */ const char** name) = 0;
    
    NS_IMETHOD
    GetBaseLineNumber(/* out */ uintN* lineno) = 0;

    NS_IMETHOD
    GetLineExtent(/* out */ uintN* extent) = 0;

    NS_IMETHOD
    GetClosestPC(/* in */  uintN line,
                 /* out */ jsuword* pc) = 0;

    NS_IMETHOD
    GetClosestLine(/* in */  jsuword pc,
                   /* out */ uintN* line) = 0;
};


class nsIJSDThreadState : public nsISupports {
public:

    NS_IMETHOD

};

/////////////////////////////////////////////////////////////////////////////
// hooks

enum JSDThreadStatus
    JSD_THREAD_RUNNING;
    JSD_THREAD_RUNNING;
};

class nsIJSDScriptHook : public nsISupports {
public:

    NS_IMETHOD
    ScriptAloha(/* in */ nsIJSDRuntime** jsdrt,
                /* in */ nsIJSDScript* jsdscript,
                /* in */ JSBool created) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// iterators

class nsIJSDScriptIterator : public nsISupports {
public:

    NS_IMETHOD
    Next(/* out */ nsIJSDScript** jsdscript) = 0;

    NS_IMETHOD
    Reset(void) = 0;
};

class nsIJSDRuntimeIterator : public nsISupports {
public:

    NS_IMETHOD
    Next(/* out */ nsIJSDRuntime** jsdrt) = 0;

    NS_IMETHOD
    Reset(void) = 0;
};






#endif /* jsdxpcom_h___ */
