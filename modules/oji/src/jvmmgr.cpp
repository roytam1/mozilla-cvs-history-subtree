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
// Plugin Manager Methods to support the JVM Plugin API
////////////////////////////////////////////////////////////////////////////////

#include "jvmmgr.h"
#include "npglue.h"
#include "xp.h"
#include "net.h"
#include "prefapi.h"
#include "xp_str.h"
#include "libmocha.h"
#include "np.h"
#include "plstr.h"
#include "prio.h"
#include "jni.h"
#include "jsjava.h"
#ifdef MOCHA
#include "libmocha.h"
#include "libevent.h"
#endif

#include "xpgetstr.h"
extern "C" int XP_PROGRESS_STARTING_JAVA;
extern "C" int XP_PROGRESS_STARTING_JAVA_DONE;
extern "C" int XP_JAVA_NO_CLASSES;
extern "C" int XP_JAVA_WRONG_CLASSES;
extern "C" int XP_JAVA_STARTUP_FAILED;
extern "C" int XP_JAVA_DEBUGGER_FAILED;

// FIXME -- need prototypes for these functions!!! XXX
#ifdef XP_MAC
extern "C" {
OSErr ConvertUnixPathToMacPath(const char *, char **);
void startAsyncCursors(void);
void stopAsyncCursors(void);
}
#endif // XP_MAC

////////////////////////////////////////////////////////////////////////////////

NS_IMPL_AGGREGATED(nsJVMMgr);

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIJVMManagerIID, NS_IJVMMANAGER_IID);
static NS_DEFINE_IID(kIJVMPluginIID, NS_IJVMPLUGIN_IID);
static NS_DEFINE_IID(kIJNIPluginIID, NS_IJNIPLUGIN_IID);
static NS_DEFINE_IID(kISymantecDebugManagerIID, NS_ISYMANTECDEBUGMANAGER_IID);

NS_METHOD
nsJVMMgr::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?
    nsJVMMgr* jvmmgr = new nsJVMMgr(outer);
    nsresult result = jvmmgr->QueryInterface(aIID, aInstancePtr);
    if (result != NS_OK) {
        delete jvmmgr;
    }
    return result;
}

nsIJVMPlugin*
nsJVMMgr::GetJVMPlugin(void)
{
    if (fJVM) {
        fJVM->AddRef();
        return fJVM;
    }
    nsIPlugin* plugin = NPL_LoadPluginByType(NPJVM_MIME_TYPE);
    if (plugin) {
        nsresult rslt = plugin->QueryInterface(kIJVMPluginIID, (void**)&fJVM);
        if (rslt == NS_OK) return fJVM;
    }
    return NULL;   
}

nsJVMMgr::nsJVMMgr(nsISupports* outer)
    : fJVM(NULL), fStatus(nsJVMStatus_Enabled),
      fRegisteredJavaPrefChanged(PR_FALSE), fDebugManager(NULL), fJSJavaVM(NULL)
{
    NS_INIT_AGGREGATED(outer);
}

nsJVMMgr::~nsJVMMgr()
{
    if (fJVM) {
        nsrefcnt c = fJVM->Release();   // Release for QueryInterface in GetJVM
        // XXX unload plugin if c == 1 ? (should this be done inside Release?)
    }
}

NS_METHOD
nsJVMMgr::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (aIID.Equals(kIJVMManagerIID)) {
        *aInstancePtr = this;
        AddRef();
        return NS_OK;
    }
#ifdef XP_PC
    // Aggregates...
    if (fDebugManager == NULL) {
        nsresult rslt =
            nsSymantecDebugManager::Create(this, kISupportsIID,
                                           (void**)&fDebugManager, this);
        if (rslt != NS_OK) return rslt;
    }
    return fDebugManager->QueryInterface(aIID, aInstancePtr);
#else
    return NS_NOINTERFACE;
#endif
}

////////////////////////////////////////////////////////////////////////////////

NS_METHOD_(void)
nsJVMMgr::NotifyJVMStatusChange(nsJVMError error)
{
    if (error != nsJVMError_Ok) {
        // Should have been running before this:
        PR_ASSERT(fStatus == nsJVMStatus_Running);

        // XXX report error?

        // The JVM should have shut down itself in this case so there's no
        // need to call ShutdownJVM here.
        fStatus = nsJVMStatus_Failed;
    }
}


///////////////////////////LiveConnect callbacks//////////////////////////////////////////////////////
PR_BEGIN_EXTERN_C

#include "jscntxt.h"

#ifdef MOCHA

PR_IMPLEMENT(JSContext *)
map_jsj_thread_to_js_context_impl(JNIEnv *env, char **errp)
{
    JSContext *cx    = NULL;
    jobject   loader;
    PRBool    mayscript = PR_FALSE;
    PRBool    jvmMochaPrefsEnabled = PR_FALSE;

    *errp = NULL;
    nsJVMMgr* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      jvmMochaPrefsEnabled = pJVMMgr->IsJVMAndMochaPrefsEnabled();
      if (pJVMPI != NULL) {
       jobject javaObject = pJVMPI->AttachThreadToJavaObject(env);
        nsIPluginInstance* pPIT = 
          pJVMPI->GetPluginInstance(javaObject);
          if (pPIT != NULL) {
               nsIJVMPluginInstance* pJVMPIT;
NS_DEFINE_IID(kIJVMPluginInstanceIID, NS_IJVMPLUGININSTANCE_IID);
               if (pPIT->QueryInterface(kIJVMPluginInstanceIID,
                           (void**)&pJVMPIT) == NS_OK) {
                 nsPluginInstancePeer* pPITP = 
                      (nsPluginInstancePeer*)pJVMPIT->GetPeer(); 
                 if (pPITP != NULL) {
                   cx = pPITP->GetJSContext();
                   pPITP->Release();
                 }
                 pJVMPIT->Release();
               }
           pPIT->Release();
          }
        pJVMPI->Release();
      }
      pJVMMgr->Release();
    }
    if (jvmMochaPrefsEnabled == PR_FALSE)
    {
        *errp = strdup("Java preference is disabled");
        return NULL;
    }

    if ( cx == NULL )
    {
       *errp = strdup("Java thread could not be associated with a JSContext");
       return NULL;
    }

    return cx;
}

/*
** This callback is called to map a JSContext to a JSJavaThreadState which
** is a wrapper around JNIEnv. Hence this callback essentially maps a JSContext
** to a java thread. JSJ_AttachCurrentThreadToJava just calls AttachCurrentThread
** on the java vm.
*/
PR_IMPLEMENT(JSJavaThreadState *)
map_js_context_to_jsj_thread_impl(JSContext *cx, char **errp)
{
    int ret = ET_InitMoja(0);
    *errp = NULL;
    if (ret != LM_MOJA_OK)
    {
       *errp = strdup("ET_InitMoja(0) failed.");
	      return NULL;
    }
    nsJVMMgr* pJVMMgr = JVM_GetJVMMgr();

    if (pJVMMgr != NULL) {
      if (pJVMMgr->GetJSJavaVM() == NULL)
      {
         *errp = strdup("Failed to attach to a java vm");
	        return NULL;
      }
      pJVMMgr->Release();
    }

    return JSJ_AttachCurrentThreadToJava(pJVMMgr->GetJSJavaVM(), NULL, NULL);
}

/*
** This callback is called to map a applet,bean to its corresponding JSObject
** created on javascript side and then map that to a java wrapper JSObject class.
** This callback is called in JSObject.getWindow implementation to get
** a java wrapper JSObject class for a applet only once.
** Note that once a mapping between applet -> javascript JSObject -> Java wrapper JSObject 
** is made, all subsequent method calls via JSObject use the internal field
** to get to the javascript JSObject.
*/
PR_IMPLEMENT(JSObject *)
map_java_object_to_js_object_impl(JNIEnv *env, jobject applet, char **errp)
{
    LJAppletData    *ad;
    MWContext       *cx;
    JSObject        *window;
    MochaDecoder    *decoder; 
    PRBool           mayscript = PR_FALSE;
    PRBool           jvmMochaPrefsEnabled = PR_FALSE;

    *errp = NULL;
    /* XXX assert JS is locked */

    if (!applet) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"),0);
        return 0;
    }
    if (!env->IsInstanceOf(applet,
                           env->FindClass("java/applet/Applet"))) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                     "JSObject.getWindow() requires a java.applet.Applet argument");
        return 0;
    }

    nsJVMMgr* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      jvmMochaPrefsEnabled = pJVMMgr->IsJVMAndMochaPrefsEnabled();
      if (pJVMPI != NULL) {
       jobject javaObject = applet;
        nsIPluginInstance* pPIT = 
          pJVMPI->GetPluginInstance(javaObject);
          if (pPIT != NULL) {
               nsIJVMPluginInstance* pJVMPIT;
NS_DEFINE_IID(kIJVMPluginInstanceIID, NS_IJVMPLUGININSTANCE_IID);
               if (pPIT->QueryInterface(kIJVMPluginInstanceIID,
                           (void**)&pJVMPIT) == NS_OK) {
                 nsPluginInstancePeer* pPITP = 
                      (nsPluginInstancePeer*)pJVMPIT->GetPeer(); 
                 if (pPITP != NULL) {
                   nsIJVMPluginTagInfo* pJVMTagInfo;
NS_DEFINE_IID(kIJVMPluginTagInfoIID, NS_IJVMPLUGINTAGINFO_IID);
                   if (pPITP->QueryInterface(kIJVMPluginTagInfoIID,
                               (void**)&pJVMTagInfo) == NS_OK) {
                      mayscript = pJVMTagInfo->GetMayScript();
                      pJVMTagInfo->Release();
                   }
                   cx = pPITP->GetMWContext();
                   pPITP->Release();
                 }
                 pJVMPIT->Release();
               }
           pPIT->Release();
          }
        pJVMPI->Release();
      }
      pJVMMgr->Release();
    }

    if (  (mayscript            == PR_FALSE)
        ||(jvmMochaPrefsEnabled == PR_FALSE)
       )
    {
        *errp = strdup("JSObject.getWindow() requires mayscript attribute on this Applet or java preference is disabled");
        goto except;
    }


    if (!cx || (cx->type != MWContextBrowser && cx->type != MWContextPane))
        *errp = strdup("JSObject.getWindow() can only be called in MWContextBrowser or MWContextPane");
        return 0;

    decoder = LM_GetMochaDecoder(cx);

    /* if there is a decoder now, reflect the window */
    if (decoder && (jvmMochaPrefsEnabled == PR_TRUE)) {
        window = decoder->window_object;
    }

    LM_PutMochaDecoder(decoder);

    return window;
  except:
    return 0;
}

PR_IMPLEMENT(JavaVM *)
get_java_vm_impl(char **errp)
{
    *errp = NULL;
    JavaVM *pJavaVM = NULL;
    
    nsJVMMgr* pJVMMgr = JVM_GetJVMMgr();
    if (pJVMMgr != NULL) {
      nsIJVMPlugin* pJVMPI = pJVMMgr->GetJVMPlugin();
      if (pJVMPI != NULL) {
        pJavaVM = pJVMPI->GetJavaVM();
        pJVMPI->Release();
      }
      pJVMMgr->Release();
    }
    if ( pJavaVM == NULL )
    {
       *errp = strdup("Could not find a JavaVM to attach to.");
    }
    return pJavaVM;
}

#endif /* MOCHA */

PR_END_EXTERN_C


/*
 * Callbacks for client-specific jsjava glue
 */
static JSJCallbacks jsj_callbacks = {
    map_jsj_thread_to_js_context_impl,
    map_js_context_to_jsj_thread_impl,
    map_java_object_to_js_object_impl,
    NULL,
    LM_LockJS,
    LM_UnlockJS,
    NULL,
    get_java_vm_impl
};

void
nsJVMMgr::JSJInit()
{
    nsIJVMPlugin* pJVMPI = NULL;
    
    if(fJSJavaVM == NULL)
    {
      JSJ_Init(&jsj_callbacks);

      if( (pJVMPI = GetJVMPlugin()) != NULL)
      {
        fJSJavaVM = JSJ_ConnectToJavaVM(JVM_GetJavaVM(), pJVMPI->GetClassPath());
        pJVMPI->Release();
      }
    }
}


////////////////////////////////////////////////////////////////////////////////

#if 0
NS_METHOD_(PRBool)
nsJVMMgr::HandOffJSLock(PRThread* oldOwner, PRThread* newOwner)
{
    return LM_HandOffJSLock(oldOwner, newOwner);
}

////////////////////////////////////////////////////////////////////////////////

static const char*
ConvertToPlatformPathList(const char* cp)
{
    const char* c = strdup(cp);
    if (c == NULL)
        return NULL;
#ifdef XP_MAC
    {
        const char* path;
        const char* strtok_path;
		
        char* result = (char*) malloc(1);
        if (result == NULL)
            return NULL;
        result[0] = '\0';
        path = c;
        strtok_path = path;
#ifndef NSPR20
        while ((path = XP_STRTOK(strtok_path, PATH_SEPARATOR_STR)))
#else
        while ((path = XP_STRTOK(strtok_path, PR_PATH_SEPARATOR_STR)))
#endif
        {
            const char* macPath;
            OSErr err = ConvertUnixPathToMacPath(path, &macPath);
            char* r = PR_smprintf("%s\r%s", result, macPath);
			
            strtok_path = NULL;

            if (r == NULL)
                return result;	/* return what we could come up with */
            free(result);
            result = r;
        }
        free(c);
        return result;
    }
#else
    return c;
#endif
}

NS_METHOD_(void)
nsJVMMgr::ReportJVMError(void* env, nsJVMError err)
{
    MWContext* cx = XP_FindSomeContext();
    char *s;
    switch (err) {
      case nsJVMError_NoClasses: {
          const char* cp = fJVM->GetClassPath();
          const char* jarName = "<jar path>"; // XXX fJVM->GetSystemJARPath();
          cp = ConvertToPlatformPathList(cp);
          s = PR_smprintf(XP_GetString(XP_JAVA_NO_CLASSES),
                          jarName, jarName,
                          (cp ? cp : "<none>"));
          free((void*)cp);
          break;
      }

      case nsJVMError_JavaError: {
          const char* msg = GetJavaErrorString((JRIEnv*)env);
#ifdef DEBUG
# ifdef XP_MAC
		  ((JRIEnv*)env)->ExceptionDescribe();
# else
          JRI_ExceptionDescribe((JRIEnv*)env);
# endif
#endif
          s = PR_smprintf(XP_GetString(XP_JAVA_STARTUP_FAILED), 
                          (msg ? msg : ""));
          if (msg) free((void*)msg);
          break;
      }
      case nsJVMError_NoDebugger: {
          s = PR_smprintf(XP_GetString(XP_JAVA_DEBUGGER_FAILED));
          break;
      }
      default:
        return;	/* don't report anything */
    }
    if (s) {
        FE_Alert(cx, s);
        free(s);
    }
}

/* stolen from java_lang_Object.h (jri version) */
#define classname_java_lang_Object	"java/lang/Object"
#define name_java_lang_Object_toString	"toString"
#define sig_java_lang_Object_toString 	"()Ljava/lang/String;"

const char*
nsJVMMgr::GetJavaErrorString(JRIEnv* env)
{
    /* XXX javah is a pain wrt mixing JRI and JDK native methods. 
       Since we need to call a method on Object, we'll do it the hard way 
       to avoid using javah for this.
       Maybe we need a new JRI entry point for toString. Yikes! */

#ifdef XP_MAC
    jthrowable exc = env->ExceptionOccurred();
    if (exc == NULL) {
        return strdup("");	/* XXX better "no error" message? */
    }

    /* Got to do this before trying to find a class (like Object). 
       This is because the runtime refuses to do this with a pending 
       exception! I think it's broken. */

    env->ExceptionClear();

    jclass classObject = env->FindClass(classname_java_lang_Object);
    jmethodID toString = env->GetMethodID(classObject, name_java_lang_Object_toString, sig_java_lang_Object_toString);
    jstring excString = (jstring) env->CallObjectMethod(exc, toString);

	jboolean isCopy;
    const char* msg = env->GetStringUTFChars(excString, &isCopy);
    if (msg != NULL) {
	    const char* dupmsg = (msg == NULL ? NULL : strdup(msg));
    	env->ReleaseStringUTFChars(excString, msg);
    	msg = dupmsg;
    }
    return msg;
#else
    const char* msg;
    struct java_lang_Class* classObject;
    JRIMethodID toString;
    struct java_lang_String* excString;
    struct java_lang_Throwable* exc = JRI_ExceptionOccurred(env);

    if (exc == NULL) {
        return strdup("");	/* XXX better "no error" message? */
    }

    /* Got to do this before trying to find a class (like Object). 
       This is because the runtime refuses to do this with a pending 
       exception! I think it's broken. */

    JRI_ExceptionClear(env);

    classObject = (struct java_lang_Class*)
        JRI_FindClass(env, classname_java_lang_Object);
    toString = JRI_GetMethodID(env, classObject,
                               name_java_lang_Object_toString,
                               sig_java_lang_Object_toString);
    excString = (struct java_lang_String *)
        JRI_CallMethod(env)(env, JRI_CallMethod_op,
                            (struct java_lang_Object*)exc, toString);

    /* XXX change to JRI_GetStringPlatformChars */
    msg = JRI_GetStringPlatformChars(env, excString, NULL, 0);
    if (msg == NULL) return NULL;
    return strdup(msg);
#endif
}
#endif // 0

////////////////////////////////////////////////////////////////////////////////

#ifdef XP_MAC
static NS_DEFINE_IID(kIJNIPluginIID, NS_IJVMPLUGIN_IID);
#else
static NS_DEFINE_IID(kIJRIPluginIID, NS_IJRIPLUGIN_IID);        // XXX change later to JNI
#endif

// Should be in a header; must solve build-order problem first
extern "C" void SU_Initialize(JRIEnv * env);

nsJVMStatus
nsJVMMgr::StartupJVM(void)
{
    // Be sure to check the prefs first before asking java to startup.
    if (GetJVMStatus() == nsJVMStatus_Disabled) {
        return nsJVMStatus_Disabled;
    }

    nsIJVMPlugin* jvm = GetJVMPlugin();
    if (jvm == NULL) {
        fStatus = nsJVMStatus_Failed;
        return fStatus;
    }
    
    JDK1_1InitArgs initargs;
    jvm->GetDefaultJVMInitArgs(&initargs);

    // XXX munge initargs

    nsJVMError err = fJVM->StartupJVM(&initargs);
    if (err == nsJVMError_Ok) {
#if defined(XP_MAC)
		// On the MacOS, MRJ uses JNI exclusively.
        nsIJNIPlugin* jniJVM;
        if (fJVM->QueryInterface(kIJNIPluginIID, (void**)&jniJVM) == NS_OK) {
            JNIEnv* env = jniJVM->GetJNIEnv();
            if (env) {
                if (env->ExceptionOccurred()) {
                    env->ExceptionDescribe();
                    env->ExceptionClear();
                }
                jniJVM->ReleaseJNIEnv(env);
            }
            fStatus = nsJVMStatus_Running;
            jniJVM->Release();
        }
        else {
            // Non-JNI JVM -- bail.
            jvm->Release();
            return ShutdownJVM();
        }
#else
        nsIJRIPlugin* jriJVM;
        if (fJVM->QueryInterface(kIJRIPluginIID, (void**)&jriJVM) == NS_OK) {
            JRIEnv* env = jriJVM->GetJRIEnv();
            if (env) {
                SU_Initialize(env);
                if (JRI_ExceptionOccurred(env)) {
#ifdef DEBUG
                    JVM_PrintToConsole("LJ:  SU_Initialize failed.  Bugs to atotic.\n");
#endif	
                    JRI_ExceptionDescribe(env);
                    JRI_ExceptionClear(env);
                }
                jriJVM->ReleaseJRIEnv(env);
            }
            fStatus = nsJVMStatus_Running;
            jriJVM->Release();
        }
        else {
            // Non-JRI JVM -- bail.
            jvm->Release();
            return ShutdownJVM();
        }
#endif
    }
    jvm->Release();
    return fStatus;
}

nsJVMStatus
nsJVMMgr::ShutdownJVM(PRBool fullShutdown)
{
    if (fJVM) {
        // XXX we should just make nsPluginError and nsJVMStatus be nsresult
        nsJVMError err = fJVM->ShutdownJVM(fullShutdown);
        if (err == nsJVMError_Ok)
            return nsJVMStatus_Enabled;
        else {
            // XXX report error?
            return nsJVMStatus_Disabled;
        }
    }
    return nsJVMStatus_Enabled;   // XXX what if there's no plugin available?
}

////////////////////////////////////////////////////////////////////////////////

static int PR_CALLBACK
JavaPrefChanged(const char *prefStr, void* data)
{
    nsJVMMgr* mgr = (nsJVMMgr*)data;
    XP_Bool prefBool;
    PREF_GetBoolPref(prefStr, &prefBool);
    mgr->SetJVMEnabled(prefBool);
    return 0;
} 

void
nsJVMMgr::SetJVMEnabled(PRBool enabled)
{
    if (enabled) {
        if (fStatus != nsJVMStatus_Running) 
            fStatus = nsJVMStatus_Enabled;
        // don't start the JVM here, do it lazily
    }
    else {
        if (fStatus == nsJVMStatus_Running) 
            (void)ShutdownJVM();
        else
            fStatus = nsJVMStatus_Disabled;
    }
}

void
nsJVMMgr::EnsurePrefCallbackRegistered(void)
{
    if (fRegisteredJavaPrefChanged != PR_TRUE) {
        fRegisteredJavaPrefChanged = PR_TRUE;
        PREF_RegisterCallback("security.enable_java", JavaPrefChanged, this);
        JavaPrefChanged("security.enable_java", this);
    }
}

nsJVMStatus
nsJVMMgr::GetJVMStatus(void)
{
    EnsurePrefCallbackRegistered();
    return fStatus;
}

PRBool 
nsJVMMgr::IsJVMAndMochaPrefsEnabled(void)
{
    if (GetJVMStatus() == nsJVMStatus_Disabled) {
        return PR_FALSE;
    }
    if (!LM_GetMochaEnabled()) {
        return PR_FALSE;
    }
    return PR_TRUE;
}


////////////////////////////////////////////////////////////////////////////////

nsPluginError
nsJVMMgr::AddToClassPathRecursively(const char* dirPath)
{
    PRDir* dir;
    PRDirEntry* ptr;
    nsIJVMPlugin* jvm = GetJVMPlugin();
    if (jvm == NULL) 
        return nsPluginError_GenericError;      // XXX better error?

    /* Add this path to the classpath: */
    jvm->AddToClassPath(dirPath);

    /* Also add any zip or jar files in this directory to the classpath: */
    dir = PR_OpenDir(dirPath);
    if (dir == NULL)
        return nsPluginError_InvalidParam;
    while ((ptr = PR_ReadDir(dir, PR_SKIP_NONE)) != NULL) {
        if (PL_strcmp(PR_DirName(ptr), ".") && PL_strcmp(PR_DirName(ptr), "..")) {
            char* path = PR_smprintf("%s%c%s", dirPath, PR_DIRECTORY_SEPARATOR, PR_DirName(ptr));
            if (path == NULL)
                return nsPluginError_OutOfMemoryError;
            PRFileInfo info;
            if (PR_GetFileInfo(path, &info) == PR_SUCCESS
                && info.type == PR_FILE_FILE) {
                int len = PL_strlen(path);
                /* Is it a zip or jar file? */
                if ((len > 4) && 
                    ((PL_strcasecmp(path+len-4, ".zip") == 0) || 
                     (PL_strcasecmp(path+len-4, ".jar") == 0))) {	
                    jvm->AddToClassPath(path);
                }
            }
            PR_smprintf_free(path);
        }
    }
    PR_CloseDir(dir);
    jvm->Release();
    return nsPluginError_NoError;
}

////////////////////////////////////////////////////////////////////////////////
// nsJVMPluginTagInfo
////////////////////////////////////////////////////////////////////////////////

nsJVMPluginTagInfo::nsJVMPluginTagInfo(nsISupports* outer, nsIPluginTagInfo2* info)
    : fPluginTagInfo(info), fSimulatedCodebase(NULL), fSimulatedCode(NULL)
{
    NS_INIT_AGGREGATED(outer);
}

nsJVMPluginTagInfo::~nsJVMPluginTagInfo(void)
{
    if (fSimulatedCodebase)
        PL_strfree(fSimulatedCodebase);

    if (fSimulatedCode)
        PL_strfree(fSimulatedCode);
}

NS_IMPL_AGGREGATED(nsJVMPluginTagInfo);

static NS_DEFINE_IID(kIJVMPluginTagInfoIID, NS_IJVMPLUGINTAGINFO_IID);

NS_METHOD
nsJVMPluginTagInfo::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (aIID.Equals(kIJVMPluginTagInfoIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


static void
oji_StandardizeCodeAttribute(char* buf)
{
    // strip off the ".class" suffix
    char* cp;

    if ((cp = PL_strrstr(buf, ".class")) != NULL)
        *cp = '\0';

    // Convert '/' to '.'
    cp = buf;
    while ((*cp) != '\0') {
        if ((*cp) == '/')
            (*cp) = '.';

        ++cp;
    }
}

NS_METHOD_(const char *) 
nsJVMPluginTagInfo::GetCode(void)
{
    if (fSimulatedCode)
        return fSimulatedCode;

    const char* code = fPluginTagInfo->GetAttribute("code");
    if (code) {
        fSimulatedCode = PL_strdup(code);
        oji_StandardizeCodeAttribute(fSimulatedCode);
        return fSimulatedCode;
    }

    const char* classid = fPluginTagInfo->GetAttribute("classid");
    if (classid && PL_strncasecmp(classid, "java:", 5) == 0) {
        fSimulatedCode = PL_strdup(classid + 5); // skip "java:"
        oji_StandardizeCodeAttribute(fSimulatedCode);
        return fSimulatedCode;
    }

    // XXX what about "javaprogram:" and "javabean:"?
    return NULL;
}

NS_METHOD_(const char *) 
nsJVMPluginTagInfo::GetCodeBase(void)
{
    // If we've already cached and computed the value, use it...
    if (fSimulatedCodebase)
        return fSimulatedCodebase;

    // See if it's supplied as an attribute...
    const char* codebase = fPluginTagInfo->GetAttribute("codebase");
    if (codebase != NULL)
        return codebase;

    // Okay, we'll need to simulate it from the layout tag's base URL.
    const char* docBase = fPluginTagInfo->GetDocumentBase();
    PA_LOCK(codebase, const char*, docBase);

    if ((fSimulatedCodebase = PL_strdup(codebase)) != NULL) {
        char* lastSlash = PL_strrchr(fSimulatedCodebase, '/');

        // chop of the filename from the original document base URL to
        // generate the codebase.
        if (lastSlash != NULL)
            *(lastSlash + 1) = '\0';
    }
    
    PA_UNLOCK(docBase);
    return fSimulatedCodebase;
}

NS_METHOD_(const char *) 
nsJVMPluginTagInfo::GetArchive(void)
{
    return fPluginTagInfo->GetAttribute("archive");
}

NS_METHOD_(const char *) 
nsJVMPluginTagInfo::GetName(void)
{
    const char* attrName;
    switch (fPluginTagInfo->GetTagType()) {
      case nsPluginTagType_Applet:
        attrName = "name";
        break;
      default:
        attrName = "id";
        break;
    }
    return fPluginTagInfo->GetAttribute(attrName);
}

NS_METHOD_(PRBool) 
nsJVMPluginTagInfo::GetMayScript(void)
{
    return (fPluginTagInfo->GetAttribute("mayscript") != NULL);
}

static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID);

NS_METHOD
nsJVMPluginTagInfo::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
                           nsIPluginTagInfo2* info)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?

    nsJVMPluginTagInfo* jvmInstPeer = new nsJVMPluginTagInfo(outer, info);
    if (jvmInstPeer == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    nsresult result = jvmInstPeer->QueryInterface(aIID, aInstancePtr);
    if (result != NS_OK) goto error;

    result = outer->QueryInterface(kIPluginTagInfo2IID,
                                   (void**)&jvmInstPeer->fPluginTagInfo);
    if (result != NS_OK) goto error;
    outer->Release();   // no need to AddRef outer
    return result;

  error:
    delete jvmInstPeer;
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Convenience Routines

PR_BEGIN_EXTERN_C

PR_IMPLEMENT(nsJVMMgr*)
JVM_GetJVMMgr(void)
{
    extern nsPluginManager* thePluginManager;
    if (thePluginManager == NULL)
        return NULL;
    nsJVMMgr* mgr;
    nsresult res =
        thePluginManager->QueryInterface(kIJVMManagerIID, (void**)&mgr);
    if (res != NS_OK)
        return NULL;
    return mgr;
}

static nsIJVMPlugin*
GetRunningJVM(void)
{
    nsIJVMPlugin* jvm = NULL;
    nsJVMMgr* jvmMgr = JVM_GetJVMMgr();
    if (jvmMgr) {
        nsJVMStatus status = jvmMgr->GetJVMStatus();
        if (status == nsJVMStatus_Enabled)
            status = jvmMgr->StartupJVM();
        if (status == nsJVMStatus_Running) {
            jvm = jvmMgr->GetJVMPlugin();
        }
        jvmMgr->Release();
    }
    return jvm;
}

PR_IMPLEMENT(PRBool)
JVM_IsJVMAvailable(void)
{
    PRBool result = PR_FALSE;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        nsJVMStatus status = mgr->GetJVMStatus();
        result = status != nsJVMStatus_Failed
              && status != nsJVMStatus_Disabled;
        mgr->Release();
    }
    return result;
}

PR_IMPLEMENT(nsPluginError)
JVM_AddToClassPathRecursively(const char* dirPath)
{
    nsPluginError err = nsPluginError_GenericError;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        err = mgr->AddToClassPathRecursively(dirPath);
        mgr->Release();
    }
    return err;
}

static NS_DEFINE_IID(kIJVMConsoleIID, NS_IJVMCONSOLE_IID);

// This will get the JVMConsole if one is available. You have to Release it 
// when you're done with it.
static nsIJVMConsole*
GetConsole(void)
{
    nsIJVMConsole* console = NULL;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        jvm->QueryInterface(kIJVMConsoleIID, (void**)&console);
        jvm->Release();
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
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->HideConsole();
        console->Release();
    }
}

PR_IMPLEMENT(PRBool)
JVM_IsConsoleVisible(void)
{
    PRBool result = PR_FALSE;
    nsIJVMConsole* console = GetConsole();
    if (console) {
        result = console->IsConsoleVisible();
        console->Release();
    }
    return result;
}

PR_IMPLEMENT(void)
JVM_ToggleConsole(void)
{
    nsIJVMConsole* console = GetConsole();
    if (console) {
        if (console->IsConsoleVisible())
            console->HideConsole();
        else
            console->ShowConsole();
        console->Release();
    }
}

PR_IMPLEMENT(void)
JVM_PrintToConsole(const char* msg)
{
    nsIJVMConsole* console = GetConsole();
    if (console) {
        console->Print(msg);
        console->Release();
    }
}

static NS_DEFINE_IID(kISymantecDebuggerIID, NS_ISYMANTECDEBUGGER_IID);

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
        jvm->Release();
    }
}

PR_IMPLEMENT(JavaVM*)
JVM_GetJavaVM(void)
{
    JavaVM* jnijvm = NULL;
    nsIJVMPlugin* jvm = GetRunningJVM();
    if (jvm) {
        jnijvm = jvm->GetJavaVM();
        jvm->Release();
    }
    return jnijvm;
}

PR_IMPLEMENT(JNIEnv*)
JVM_GetJNIEnv(void)
{
    JNIEnv* env = NULL;
    nsIJVMPlugin* jvm = GetRunningJVM();
    nsIJNIPlugin* jnijvm;
    if (jvm) {
        if (jvm->QueryInterface(kIJNIPluginIID, (void**)&jnijvm) == NS_OK) {
            env = jnijvm->GetJNIEnv();
            jnijvm->Release();
        }
        jvm->Release();
    }
    return env;
}

PR_END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////

