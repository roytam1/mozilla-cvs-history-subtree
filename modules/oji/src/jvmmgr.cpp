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

#include "nsJVMManager.h"
#include "nsIServiceManager.h"

static NS_DEFINE_CID(kJVMManagerCID, NS_JVMMANAGER_CID);
static NS_DEFINE_IID(kIJVMManagerIID, NS_IJVMMANAGER_IID);
static NS_DEFINE_IID(kIJVMConsoleIID, NS_IJVMCONSOLE_IID);
static NS_DEFINE_IID(kISymantecDebuggerIID, NS_ISYMANTECDEBUGGER_IID);

PR_BEGIN_EXTERN_C

#ifdef PRE_SERVICE_MANAGER
extern nsPluginManager* thePluginManager;
#endif

PR_IMPLEMENT(nsJVMManager*)
JVM_GetJVMMgr(void)
{
#ifdef PRE_SERVICE_MANAGER
	nsresult result = NS_OK;
    if (thePluginManager == NULL) {
        result = nsPluginManager::Create(NULL, kIPluginManagerIID, (void**)&thePluginManager);
		if (result != NS_OK)
			return NULL;
    }
    nsJVMManager* mgr = NULL;
    result = thePluginManager->QueryInterface(kIJVMManagerIID, (void**)&mgr);
    if (result != NS_OK)
        return NULL;
    return mgr;
#else
    nsJVMManager* mgr = NULL;
    nsresult err = nsServiceManager::GetService(kJVMManagerCID, kIJVMManagerIID, 
                                                (nsISupports**)&mgr);
    if (err != NS_OK)
        return NULL;
    return mgr;
#endif
}

PR_IMPLEMENT(void)
JVM_ReleaseJVMMgr(nsJVMManager* mgr)
{
    nsresult err = nsServiceManager::ReleaseService(kJVMManagerCID, mgr);
    PR_ASSERT(err == NS_OK);
}

static nsIJVMPlugin*
GetRunningJVM(void)
{
    nsIJVMPlugin* jvm = NULL;
    nsJVMManager* jvmMgr = JVM_GetJVMMgr();
    if (jvmMgr) {
        nsJVMStatus status = jvmMgr->GetJVMStatus();
        if (status == nsJVMStatus_Enabled)
            status = jvmMgr->StartupJVM();
        if (status == nsJVMStatus_Running) {
            jvm = jvmMgr->GetJVMPlugin();
        }
//        jvmMgr->Release();
    }
    return jvm;
}

PR_IMPLEMENT(nsJVMStatus)
JVM_StartupJVM(void)
{
    nsIJVMPlugin* jvm = GetRunningJVM();
    return JVM_GetJVMStatus();
}

PR_IMPLEMENT(nsJVMStatus)
JVM_ShutdownJVM(void)
{
    nsJVMStatus status = nsJVMStatus_Failed;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        status = mgr->ShutdownJVM();
//        mgr->Release();
    }
    return status;
}

PR_IMPLEMENT(nsJVMStatus)
JVM_GetJVMStatus(void)
{
    nsJVMStatus status = nsJVMStatus_Disabled;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        status = mgr->GetJVMStatus();
//        mgr->Release();
    }
    return status;
}

PR_IMPLEMENT(PRBool)
JVM_AddToClassPath(const char* dirPath)
{
    nsresult err = NS_ERROR_FAILURE;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        err = mgr->AddToClassPath(dirPath);
//        mgr->Release();
    }
    return err == NS_OK;
}

// This will get the JVMConsole if one is available. You have to Release it 
// when you're done with it.
static nsIJVMConsole*
GetConsole(void)
{
    nsIJVMConsole* console = NULL;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        jvm->QueryInterface(kIJVMConsoleIID, (void**)&console);
        // jvm->Release(); // GetRunningJVM no longer calls AddRef
    }
    return console;
}

PR_IMPLEMENT(void)
JVM_ShowConsole(void)
{
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->ShowConsole();
        console->Release();
    }
}

PR_IMPLEMENT(void)
JVM_HideConsole(void)
{
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMConsole* console = GetConsole();
        if (console) {
            console->HideConsole();
            console->Release();
        }
    }
}

PR_IMPLEMENT(PRBool)
JVM_IsConsoleVisible(void)
{
    PRBool result = PR_FALSE;
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status == nsJVMStatus_Running) {
        nsIJVMConsole* console = GetConsole();
        if (console) {
            nsresult err = console->IsConsoleVisible(&result);
            PR_ASSERT(err != NS_OK ? result == PR_FALSE : PR_TRUE);
            console->Release();
        }
    }
    return result;
}

PR_IMPLEMENT(void)
JVM_PrintToConsole(const char* msg)
{
    nsJVMStatus status = JVM_GetJVMStatus();
    if (status != nsJVMStatus_Running)
        return;
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->Print(msg);
        console->Release();
    }
}

PR_IMPLEMENT(void)
JVM_StartDebugger(void)
{
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        nsISymantecDebugger* debugger;
        if (jvm->QueryInterface(kISymantecDebuggerIID, (void**)&debugger) == NS_OK) {
            // XXX should we make sure the vm is started first?
            debugger->StartDebugger(nsSymantecDebugPort_SharedMemory);
            debugger->Release();
        }
        // jvm->Release(); // GetRunningJVM no longer calls AddRef
    }
}

#if 0
static void PR_CALLBACK detach_JNIEnv(void* env)
{
	JNIEnv* jenv = (JNIEnv*)env;
	JavaVM* vm = NULL;
	jenv->GetJavaVM(&vm);
	vm->DetachCurrentThread();
}
#endif

PR_IMPLEMENT(JNIEnv*)
JVM_GetJNIEnv(void)
{
    JNIEnv* env = NULL;
	/* Use NSPR thread private data to manage the per-thread JNIEnv* association. */
#ifdef NOT_YET /* Talked to patrick about it and he said it was ok to remove it
                  for now. This code should be in the jvm plugin code which should also*/
	static ThreadLocalStorage<JNIEnv*> localEnv(&detach_JNIEnv);

    env = localEnv.get();
	if (env != NULL)
		return env;
#endif
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        (void)jvm->GetJNIEnv(&env);
        // jvm->Release(); // GetRunningJVM no longer calls AddRef
    }

#ifdef NOT_YET
	/* Associate the JNIEnv with the current thread. */
	localEnv.set(env);
#endif
    return env;
}

PR_IMPLEMENT(void)
JVM_ReleaseJNIEnv(JNIEnv* env)
{
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        (void)jvm->ReleaseJNIEnv(env);
        // jvm->Release(); // GetRunningJVM no longer calls AddRef
    }
}

PR_IMPLEMENT(nsresult)
JVM_SpendTime(PRUint32 timeMillis)
{
#ifdef XP_MAC
	nsresult result = NS_ERROR_NOT_INITIALIZED;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm != NULL)
		result = jvm->SpendTime(timeMillis);
	return result;
#else
	return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

PR_IMPLEMENT(PRBool)
JVM_MaybeStartupLiveConnect()
{
    PRBool result = PR_FALSE;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->MaybeStartupLiveConnect();
//        mgr->Release();
    }
    return result;
}


PR_IMPLEMENT(PRBool)
JVM_MaybeShutdownLiveConnect(void)
{
    PRBool result = PR_FALSE;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->MaybeShutdownLiveConnect();
//        mgr->Release();
    }
    return result;
}

PR_IMPLEMENT(PRBool)
JVM_IsLiveConnectEnabled(void)
{
    PRBool result = PR_FALSE;
    nsJVMManager* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->IsLiveConnectEnabled();
//        mgr->Release();
    }
    return result;
}

PR_END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////
