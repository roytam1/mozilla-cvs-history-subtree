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

////////////////////////////////////////////////////////////////////////////////
// NETSCAPE JAVA VM PLUGIN EXTENSIONS
// 
// This interface allows a Java virtual machine to be plugged into
// Communicator to implement the APPLET tag and host applets.
// 
// Note that this is the C++ interface that the plugin sees. The browser
// uses a specific implementation of this, nsJVMPlugin, found in jvmmgr.h.
////////////////////////////////////////////////////////////////////////////////

#ifndef nsIJVM_h___
#define nsIJVM_h___

#include "nsIPlug.h"
#include "jni.h"
#include "jri.h"
#include "prthread.h"

////////////////////////////////////////////////////////////////////////////////

#define NPJVM_MIME_TYPE         "application/x-java-vm" // XXX application/java

enum nsJVMError {
    nsJVMError_Ok                 = nsPluginError_NoError,
    nsJVMError_Base               = 0x1000,
    nsJVMError_InternalError      = nsJVMError_Base,
    nsJVMError_NoClasses,
    nsJVMError_WrongClasses,
    nsJVMError_JavaError,
    nsJVMError_NoDebugger
};

////////////////////////////////////////////////////////////////////////////////
// Java VM Plugin Manager
// This interface defines additional entry points that are available
// to JVM plugins for browsers that support JVM plugins.

class nsIJVMPlugin;

class nsIJVMManager : public nsISupports {
public:

    // This method may be called by the JVM to indicate that an error has
    // occurred, e.g. that the JVM has failed or is shutting down spontaneously.
    // This allows the browser to clean up any JVM-specific state.
    NS_IMETHOD_(void)
    NotifyJVMStatusChange(nsJVMError error) = 0;

#if 0   // junk?
    NS_IMETHOD_(PRBool)
    HandOffJSLock(PRThread* oldOwner, PRThread* newOwner) = 0;

    NS_IMETHOD_(void)
    ReportJVMError(void* env, nsJVMError err) = 0;      // XXX JNIEnv*
#endif

};

#define NS_IJVMMANAGER_IID                           \
{ /* a1e5ed50-aa4a-11d1-85b2-00805f0e4dfe */         \
    0xa1e5ed50,                                      \
    0xaa4a,                                          \
    0x11d1,                                          \
    {0x85, 0xb2, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

////////////////////////////////////////////////////////////////////////////////
// Java VM Plugin Interface
// This interface defines additional entry points that a plugin developer needs
// to implement in order to implement a Java virtual machine plugin. 

class nsIJVMPlugin : public nsIPlugin {
public:

    // This method fills out an initargs struct defined by jni.h
    // according to what the JVM thinks is appropriate. It basically
    // corresponds to JNI_GetDefaultJavaVMInitArgs.
    NS_IMETHOD_(nsJVMError)
    GetDefaultJVMInitArgs(void* initargs) = 0;

    // This method us used to start the Java virtual machine.
    // It sets up any global state necessary to host Java programs.
    // Note that calling this method is distinctly separate from 
    // initializing the nsIJVMPlugin object (done by the Initialize
    // method).
    NS_IMETHOD_(nsJVMError)
    StartupJVM(void* initargs) = 0;

    // This method us used to stop the Java virtual machine.
    // It tears down any global state necessary to host Java programs.
    // The fullShutdown flag specifies whether the browser is quitting
    // (PR_TRUE) or simply whether the JVM is being shut down (PR_FALSE).
    NS_IMETHOD_(nsJVMError)
    ShutdownJVM(PRBool fullShutdown) = 0;

    // Causes the JVM to append a new directory to its classpath.
    // If the JVM doesn't support this operation, an error is returned.
    NS_IMETHOD_(nsJVMError)
    AddToClassPath(const char* dirPath) = 0;

    // Causes the JVM to remove a directory from its classpath.
    // If the JVM doesn't support this operation, an error is returned.
    NS_IMETHOD_(nsJVMError)
    RemoveFromClassPath(const char* dirPath) = 0;

    // Returns the current classpath in use by the JVM.
    NS_IMETHOD_(const char*)
    GetClassPath(void) = 0;
    
    NS_IMETHOD_(nsIPluginInstance*)
    GetPluginInstance(jobject applet) = 0;

    NS_IMETHOD_(jobject)
    AttachThreadToJavaObject(JNIEnv *jenv) = 0;

    NS_IMETHOD_(JavaVM *)
    GetJavaVM(void) = 0;
};

#define NS_IJVMPLUGIN_IID                            \
{ /* da6f3bc0-a1bc-11d1-85b1-00805f0e4dfe */         \
    0xda6f3bc0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

////////////////////////////////////////////////////////////////////////////////
// JNI Plugin Class Interface

class nsIJNIPlugin : public nsISupports {
public:

    // QueryInterface on nsIJVMPlugin to get this.

    // Find or create a JNIEnv for the current thread.
    // Returns NULL if an error occurs.
    NS_IMETHOD_(JNIEnv*)
    GetJNIEnv(void) = 0;

    // This method must be called when the caller is done using the JNIEnv.
    // This decrements a refcount associated with it may free it.
    NS_IMETHOD_(nsrefcnt)
    ReleaseJNIEnv(JNIEnv* env) = 0;

};

#define NS_IJNIPLUGIN_IID                            \
{ /* 79fa03d0-0164-11d2-815b-006008119d7a */         \
    0x79fa03d0,                                      \
    0x0164,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// JRI Plugin Class Interface
// This interface is provided for backward compatibility for the Netscape JVM.

class nsIJRIPlugin : public nsISupports {
public:

    // QueryInterface on nsIJVMPlugin to get this.

    // Find or create a JRIEnv for the current thread. 
    // Returns NULL if an error occurs.
    NS_IMETHOD_(JRIEnv*)
    GetJRIEnv(void) = 0;

    // This method must be called when the caller is done using the JRIEnv.
    // This decrements a refcount associated with it may free it.
    NS_IMETHOD_(nsrefcnt)
    ReleaseJRIEnv(JRIEnv* env) = 0;

};

#define NS_IJRIPLUGIN_IID                            \
{ /* bfe2d7d0-0164-11d2-815b-006008119d7a */         \
    0xbfe2d7d0,                                      \
    0x0164,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// JVM Console Interface
// This interface defines the API the browser needs to show and hide the JVM's
// Java console, and to send text to it.

class nsIJVMConsole : public nsISupports {
public:
    
    // QueryInterface on nsIJVMPlugin to get this.

    NS_IMETHOD_(void)
    ShowConsole(void) = 0;

    NS_IMETHOD_(void)
    HideConsole(void) = 0;

    NS_IMETHOD_(PRBool)
    IsConsoleVisible(void) = 0;

    // Prints a message to the Java console. The encodingName specifies the
    // encoding of the message, and if NULL, specifies the default platform
    // encoding.
    NS_IMETHOD_(void)
    Print(const char* msg, const char* encodingName = NULL) = 0;
    
};

#define NS_IJVMCONSOLE_IID                           \
{ /* 85344580-01c1-11d2-815b-006008119d7a */         \
    0x85344580,                                      \
    0x01c1,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Java VM Plugin Instance Interface

class nsIJVMPluginInstance : public nsIPluginInstance {
public:

    // This method is called when LiveConnect wants to find the Java object
    // associated with this plugin instance, e.g. the Applet or JavaBean object.
    NS_IMETHOD_(jobject) 
    GetJavaObject(void) = 0;

    NS_IMETHOD_(nsIPluginInstancePeer2 *)
    GetPeer(void) = 0;

    /* =-= sudu: Ask Eric Bina, what is GetText used for in layform.c
                 Check to see if this should be more general api applicaple to
                 applets/beans and plugins.
    */
    NS_IMETHOD_(char *)
    GetText(void) = 0;
};

#define NS_IJVMPLUGININSTANCE_IID                    \
{ /* a0c057d0-01c1-11d2-815b-006008119d7a */         \
    0xa0c057d0,                                      \
    0x01c1,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Java VM Plugin Instance Peer Interface
// This interface provides additional hooks into the plugin manager that allow 
// a plugin to implement the plugin manager's Java virtual machine.

class nsIJVMPluginTagInfo : public nsISupports {
public:

    NS_IMETHOD_(const char *) 
    GetCode(void) = 0;

    NS_IMETHOD_(const char *) 
    GetCodeBase(void) = 0;

    NS_IMETHOD_(const char *) 
    GetArchive(void) = 0;

    NS_IMETHOD_(const char *) 
    GetName(void) = 0;

    NS_IMETHOD_(PRBool) 
    GetMayScript(void) = 0;

};

#define NS_IJVMPLUGINTAGINFO_IID                     \
{ /* 27b42df0-a1bd-11d1-85b1-00805f0e4dfe */         \
    0x27b42df0,                                      \
    0xa1bd,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

class nsIJVMPluginInstanceJSContext : public nsISupports {
public:

};

#define NS_IJVMPLUGINTAGINFO_IID                     \
{ /* 27b42df0-a1bd-11d1-85b1-00805f0e4dfe */         \
    0x27b42df0,                                      \
    0xa1bd,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}


////////////////////////////////////////////////////////////////////////////////
#endif /* nsIJVM_h___ */
