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
#include "prefapi.h"
#include "xp_str.h"
#include "np.h"
#include "prio.h"
#include "prmem.h"
#include "prthread.h"
#include "pprthred.h"
#include "plstr.h"
#include "jni.h"
#ifdef MOCHA
#include "libmocha.h"
#endif

#include "xpgetstr.h"
extern "C" int XP_PROGRESS_STARTING_JAVA;
extern "C" int XP_PROGRESS_STARTING_JAVA_DONE;
extern "C" int XP_JAVA_NO_CLASSES;
extern "C" int XP_JAVA_GENERAL_FAILURE;
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

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIJVMManagerIID, NS_IJVMMANAGER_IID);
static NS_DEFINE_IID(kIJVMPluginIID, NS_IJVMPLUGIN_IID);
static NS_DEFINE_IID(kISymantecDebugManagerIID, NS_ISYMANTECDEBUGMANAGER_IID);
static NS_DEFINE_IID(kIJVMPluginTagInfoIID, NS_IJVMPLUGINTAGINFO_IID);
static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID);
static NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
static NS_DEFINE_IID(kIJVMConsoleIID, NS_IJVMCONSOLE_IID);
static NS_DEFINE_IID(kISymantecDebuggerIID, NS_ISYMANTECDEBUGGER_IID);

////////////////////////////////////////////////////////////////////////////////

NS_IMPL_AGGREGATED(nsJVMMgr);

NS_METHOD
nsJVMMgr::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?
    nsJVMMgr* jvmmgr = new nsJVMMgr(outer);
    if (jvmmgr == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    jvmmgr->AddRef();
    *aInstancePtr = (outer != NULL ? (void*) jvmmgr->GetInner() : (void*) jvmmgr);
    return NS_OK;
}

nsJVMMgr::nsJVMMgr(nsISupports* outer)
    : fJVM(NULL), fStatus(nsJVMStatus_Enabled),
      fRegisteredJavaPrefChanged(PR_FALSE), fDebugManager(NULL), fJSJavaVM(NULL),
      fClassPathAdditions(new nsVector())
{
    extern PRUintn tlsIndex_g;
    NS_INIT_AGGREGATED(outer);
    PR_NewThreadPrivateIndex(&tlsIndex_g, NULL);
}

nsJVMMgr::~nsJVMMgr()
{
    int count = fClassPathAdditions->GetSize();
    for (int i = 0; i < count; i++) {
        PR_Free((*fClassPathAdditions)[i]);
    }
    delete fClassPathAdditions;
    if (fJVM) {
        /*nsrefcnt c =*/ fJVM->Release();   // Release for QueryInterface in GetJVM
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

#if 0
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
#endif

void
nsJVMMgr::ReportJVMError(nsresult err)
{
    MWContext* cx = XP_FindSomeContext();
    char *s = NULL;
    switch (err) {
      case NS_JVM_ERROR_NO_CLASSES: {
          s = PR_smprintf(XP_GetString(XP_JAVA_NO_CLASSES));
          break;
      }

      case NS_JVM_ERROR_JAVA_ERROR: {
          nsIJVMPlugin* plugin = GetJVMPlugin();
          PR_ASSERT(plugin != NULL);
          if (plugin == NULL) break;
          JNIEnv* env;
          plugin->GetJNIEnv(&env);
          const char* msg = GetJavaErrorString(env);
          plugin->ReleaseJNIEnv(env);
#ifdef DEBUG
		  env->ExceptionDescribe();
#endif
          s = PR_smprintf(XP_GetString(XP_JAVA_STARTUP_FAILED), 
                          (msg ? msg : ""));
          if (msg) free((void*)msg);
          break;
      }

      case NS_JVM_ERROR_NO_DEBUGGER: {
          s = PR_smprintf(XP_GetString(XP_JAVA_DEBUGGER_FAILED));
          break;
      }

      default: {
          s = PR_smprintf(XP_GetString(XP_JAVA_GENERAL_FAILURE), err);
          break;
      }
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
nsJVMMgr::GetJavaErrorString(JNIEnv* env)
{
    /* XXX javah is a pain wrt mixing JRI and JDK native methods. 
       Since we need to call a method on Object, we'll do it the hard way 
       to avoid using javah for this.
       Maybe we need a new JRI entry point for toString. Yikes! */

#if 1 //def XP_MAC
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

////////////////////////////////////////////////////////////////////////////////

PRLogModuleInfo* NSJAVA = NULL;

nsJVMStatus
nsJVMMgr::StartupJVM(void)
{
    // Be sure to check the prefs first before asking java to startup.
    switch (GetJVMStatus()) {
      case nsJVMStatus_Disabled:
        return nsJVMStatus_Disabled;
      case nsJVMStatus_Running:
        return nsJVMStatus_Running;
      default:
        break;
    }

#ifdef DEBUG
    PRIntervalTime start = PR_IntervalNow();
    if (NSJAVA == NULL)
        NSJAVA = PR_NewLogModule("NSJAVA");
    PR_LOG(NSJAVA, PR_LOG_ALWAYS, ("Starting java..."));
#endif

	MWContext* someRandomContext = XP_FindSomeContext();
	if (someRandomContext) {
		FE_Progress(someRandomContext, XP_GetString(XP_PROGRESS_STARTING_JAVA));
	}

    PR_ASSERT(fJVM == NULL);
    nsIPlugin* plugin = NPL_LoadPluginByType(NPJVM_MIME_TYPE);
    if (plugin == NULL) {
        fStatus = nsJVMStatus_Failed;
        return fStatus;
    }

    nsresult rslt = plugin->QueryInterface(kIJVMPluginIID, (void**)&fJVM);
    if (rslt != NS_OK) {
        PR_ASSERT(fJVM == NULL);
        fStatus = nsJVMStatus_Failed;
        return fStatus;
    }
    
    nsJVMInitArgs initargs;
    int count = fClassPathAdditions->GetSize();
    char* classpathAdditions = NULL;
    for (int i = 0; i < count; i++) {
        const char* path = (const char*)(*fClassPathAdditions)[i];
        char* oldPath = classpathAdditions;
        if (oldPath) {
            classpathAdditions = PR_smprintf("%s%c%s", oldPath, PR_PATH_SEPARATOR, path);
            PR_Free(oldPath);
        }
        else
            classpathAdditions = PL_strdup(path);
    }
    initargs.version = nsJVMInitArgs_Version;
    initargs.classpathAdditions = classpathAdditions;

    nsresult err = fJVM->StartupJVM(&initargs);
    if (err == NS_OK) {
        /* assume the JVM is running. */
        fStatus = nsJVMStatus_Running;
    }
    else {
        ReportJVMError(err);
        fStatus = nsJVMStatus_Failed;
    }

#if 0
    JSContext* crippledContext = LM_GetCrippledContext();
    MaybeStartupLiveConnect(crippledContext, JS_GetGlobalObject(crippledContext));
#endif

    fJVM->Release();

#ifdef DEBUG
    PRIntervalTime end = PR_IntervalNow();
    PRInt32 d = PR_IntervalToMilliseconds(end - start);
    PR_LOG(NSJAVA, PR_LOG_ALWAYS,
           ("Starting java...%s (%ld ms)",
            (fStatus == nsJVMStatus_Running ? "done" : "failed"), d));
#endif

	if (someRandomContext) {
		FE_Progress(someRandomContext, XP_GetString(XP_PROGRESS_STARTING_JAVA_DONE));
	}
    return fStatus;
}

nsJVMStatus
nsJVMMgr::ShutdownJVM(PRBool fullShutdown)
{
    if (fStatus == nsJVMStatus_Running) {
        PR_ASSERT(fJVM != NULL);
        (void)MaybeShutdownLiveConnect();
        nsresult err = fJVM->ShutdownJVM(fullShutdown);
        if (err == NS_OK)
            fStatus = nsJVMStatus_Enabled;
        else {
            ReportJVMError(err);
            fStatus = nsJVMStatus_Disabled;
        }
        fJVM = NULL;
    }
    PR_ASSERT(fJVM == NULL);
    return fStatus;
}

////////////////////////////////////////////////////////////////////////////////

static int PR_CALLBACK
JavaPrefChanged(const char *prefStr, void* data)
{
    nsJVMMgr* mgr = (nsJVMMgr*)data;
    XP_Bool prefBool;
#if defined(XP_MAC)
	// beard: under Mozilla, no way to enable this right now.
	prefBool = TRUE;
#else
    PREF_GetBoolPref(prefStr, &prefBool);
#endif
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

#ifdef XP_MAC
#define JSJDLL "LiveConnect"
#endif

PRBool
nsJVMMgr::MaybeStartupLiveConnect()
{
    if (fJSJavaVM)
        return PR_TRUE;

	do {
		static PRBool registeredLiveConnectFactory = PR_FALSE;
		if (!registeredLiveConnectFactory) {
            NS_DEFINE_CID(kCLiveconnectCID, NS_CLIVECONNECT_CID);
			registeredLiveConnectFactory = 
	            (nsRepository::RegisterFactory(kCLiveconnectCID, (const char *)JSJDLL,
	                                        PR_FALSE, PR_FALSE) == NS_OK);
		}
	    if (IsLiveConnectEnabled() && StartupJVM() == nsJVMStatus_Running) {
            extern JSJCallbacks jsj_callbacks;
	        JSJ_Init(&jsj_callbacks);
	        nsIJVMPlugin* plugin = GetJVMPlugin();
	        if (plugin) {
#if 0
	            const char* classpath = NULL;
	            nsresult err = plugin->GetClassPath(&classpath);
	            if (err != NS_OK) break;

	            JavaVM* javaVM = NULL;
	            err = plugin->GetJavaVM(&javaVM);
	            if (err != NS_OK) break;
#endif
	            fJSJavaVM = JSJ_ConnectToJavaVM(NULL, NULL);
	            if (fJSJavaVM != NULL)
	                return PR_TRUE;
	            // plugin->Release(); // GetJVMPlugin no longer calls AddRef
	        }
	    }
	} while (0);
    return PR_FALSE;
}

PRBool
nsJVMMgr::MaybeShutdownLiveConnect(void)
{
    if (fJSJavaVM) {
        JSJ_DisconnectFromJavaVM(fJSJavaVM);
        fJSJavaVM = NULL;
        return PR_TRUE; 
    }
    return PR_FALSE;
}

PRBool
nsJVMMgr::IsLiveConnectEnabled(void)
{
	if (LM_GetMochaEnabled()) {
		nsJVMStatus status = GetJVMStatus();
		return (status == nsJVMStatus_Enabled || status == nsJVMStatus_Running);
	}
	return PR_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
nsJVMMgr::AddToClassPath(const char* dirPath)
{
    nsIJVMPlugin* jvm = GetJVMPlugin();

    /* Add any zip or jar files in this directory to the classpath: */
    PRDir* dir = PR_OpenDir(dirPath);
    if (dir != NULL) {
        PRDirEntry* dirent;
        while ((dirent = PR_ReadDir(dir, PR_SKIP_BOTH)) != NULL) {
            PRFileInfo info;
            char* path = PR_smprintf("%s%c%s", dirPath, PR_DIRECTORY_SEPARATOR, PR_DirName(dirent));
			if (path != NULL) {
        		PRBool freePath = PR_TRUE;
	            if ((PR_GetFileInfo(path, &info) == PR_SUCCESS)
	                && (info.type == PR_FILE_FILE)) {
	                int len = PL_strlen(path);

	                /* Is it a zip or jar file? */
	                if ((len > 4) && 
	                    ((PL_strcasecmp(path+len-4, ".zip") == 0) || 
	                     (PL_strcasecmp(path+len-4, ".jar") == 0))) {
	                    fClassPathAdditions->Add((void*)path);
	                    if (jvm) {
	                        /* Add this path to the classpath: */
	                        jvm->AddToClassPath(path);
	                    }
	                    freePath = PR_FALSE;
	                }
	            }
	            
	            // Don't leak the path!
	            if (freePath)
		            PR_smprintf_free(path);
	        }
        }
        PR_CloseDir(dir);
    }

    /* Also add the directory to the classpath: */
    fClassPathAdditions->Add((void*)dirPath);
    if (jvm) {
        jvm->AddToClassPath(dirPath);
        // jvm->Release(); // GetJVMPlugin no longer calls AddRef
    }
    return NS_OK;
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

NS_METHOD
nsJVMPluginTagInfo::GetCode(const char* *result)
{
    if (fSimulatedCode) {
        *result = fSimulatedCode;
        return NS_OK;
    }

    const char* code;
    nsresult err = fPluginTagInfo->GetAttribute("code", &code);
    if (err == NS_OK && code) {
        fSimulatedCode = PL_strdup(code);
        oji_StandardizeCodeAttribute(fSimulatedCode);
        *result = fSimulatedCode;
        return NS_OK;
    }

    const char* classid;
    err = fPluginTagInfo->GetAttribute("classid", &classid);
    if (err == NS_OK && classid && PL_strncasecmp(classid, "java:", 5) == 0) {
        fSimulatedCode = PL_strdup(classid + 5); // skip "java:"
        oji_StandardizeCodeAttribute(fSimulatedCode);
        *result = fSimulatedCode;
        return NS_OK;
    }

    // XXX what about "javaprogram:" and "javabean:"?
    return NS_ERROR_FAILURE;
}

NS_METHOD
nsJVMPluginTagInfo::GetCodeBase(const char* *result)
{
    // If we've already cached and computed the value, use it...
    if (fSimulatedCodebase) {
        *result = fSimulatedCodebase;
        return NS_OK;
    }

    // See if it's supplied as an attribute...
    const char* codebase;
    nsresult err = fPluginTagInfo->GetAttribute("codebase", &codebase);
    if (err == NS_OK && codebase != NULL) {
        *result = fSimulatedCodebase;
        return NS_OK;
    }

    // Okay, we'll need to simulate it from the layout tag's base URL.
    const char* docBase;
    err = fPluginTagInfo->GetDocumentBase(&docBase);
    if (err != NS_OK) return err;
    PA_LOCK(codebase, const char*, docBase);

    if ((fSimulatedCodebase = PL_strdup(codebase)) != NULL) {
        char* lastSlash = PL_strrchr(fSimulatedCodebase, '/');

        // chop of the filename from the original document base URL to
        // generate the codebase.
        if (lastSlash != NULL)
            *(lastSlash + 1) = '\0';
    }
    
    PA_UNLOCK(docBase);
    *result = fSimulatedCodebase;
    return NS_OK;
}

NS_METHOD
nsJVMPluginTagInfo::GetArchive(const char* *result)
{
    return fPluginTagInfo->GetAttribute("archive", result);
}

NS_METHOD
nsJVMPluginTagInfo::GetName(const char* *result)
{
    const char* attrName;
    nsPluginTagType type;
    nsresult err = fPluginTagInfo->GetTagType(&type);
    if (err != NS_OK) return err;
    switch (type) {
      case nsPluginTagType_Applet:
        attrName = "name";
        break;
      default:
        attrName = "id";
        break;
    }
    return fPluginTagInfo->GetAttribute(attrName, result);
}

NS_METHOD
nsJVMPluginTagInfo::GetMayScript(PRBool *result)
{
    const char* attr;
    *result = PR_FALSE;

    nsresult err = fPluginTagInfo->GetAttribute("mayscript", &attr);
    if (err) return err;

    if (PL_strcasecmp(attr, "true") == 0)
    {
       *result = PR_TRUE;
    }
    return NS_OK;
}

NS_METHOD
nsJVMPluginTagInfo::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
                           nsIPluginTagInfo2* info)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?

    nsJVMPluginTagInfo* jvmTagInfo = new nsJVMPluginTagInfo(outer, info);
    if (jvmTagInfo == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    jvmTagInfo->AddRef();
    *aInstancePtr = jvmTagInfo->GetInner();

    nsresult result = outer->QueryInterface(kIPluginTagInfo2IID,
                                            (void**)&jvmTagInfo->fPluginTagInfo);
    if (result != NS_OK) goto error;
    outer->Release();   // no need to AddRef outer
    return result;

  error:
    delete jvmTagInfo;
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Convenience Routines

PR_BEGIN_EXTERN_C

extern nsPluginManager* thePluginManager;

PR_IMPLEMENT(nsJVMMgr*)
JVM_GetJVMMgr(void)
{
	nsresult result = NS_OK;
    if (thePluginManager == NULL) {
        result = nsPluginManager::Create(NULL, kIPluginManagerIID, (void**)&thePluginManager);
		if (result != NS_OK)
			return NULL;
    }
    nsJVMMgr* mgr = NULL;
    result = thePluginManager->QueryInterface(kIJVMManagerIID, (void**)&mgr);
    if (result != NS_OK)
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
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        status = mgr->ShutdownJVM();
        mgr->Release();
    }
    return status;
}

PR_IMPLEMENT(nsJVMStatus)
JVM_GetJVMStatus(void)
{
    nsJVMStatus status = nsJVMStatus_Disabled;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        status = mgr->GetJVMStatus();
        mgr->Release();
    }
    return status;
}

PR_IMPLEMENT(PRBool)
JVM_AddToClassPath(const char* dirPath)
{
    nsresult err = NS_ERROR_FAILURE;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        err = mgr->AddToClassPath(dirPath);
        mgr->Release();
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
JVM_ToggleConsole(void)
{
    nsIJVMConsole* console = GetConsole();
    if (console) {
        PRBool visible = PR_FALSE;
        nsresult err = console->IsConsoleVisible(&visible);
        PR_ASSERT(err != NS_OK ? visible == PR_FALSE : PR_TRUE);
        if (visible)
            console->HideConsole();
        else
            console->ShowConsole();
        console->Release();
    }
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
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->MaybeStartupLiveConnect();
        mgr->Release();
    }
    return result;
}


PR_IMPLEMENT(PRBool)
JVM_MaybeShutdownLiveConnect(void)
{
    PRBool result = PR_FALSE;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->MaybeShutdownLiveConnect();
        mgr->Release();
    }
    return result;
}

PR_IMPLEMENT(PRBool)
JVM_IsLiveConnectEnabled(void)
{
    PRBool result = PR_FALSE;
    nsJVMMgr* mgr = JVM_GetJVMMgr();
    if (mgr) {
        result = mgr->IsLiveConnectEnabled();
        mgr->Release();
    }
    return result;
}

PR_END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////

