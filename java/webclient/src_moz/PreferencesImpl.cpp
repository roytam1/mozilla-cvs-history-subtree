/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

#include "PreferencesImpl.h"
#include "nsIPref.h"
#include "nsCRT.h" // for memset

#include "ns_util.h"
#include "nsActions.h"

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);




//
// Local functions
//

typedef struct _peStruct {
    WebShellInitContext *cx;
    jobject obj;
    jobject callback;
} peStruct;

class wsPrefChangedEvent : public nsActionEvent {
public:
    wsPrefChangedEvent (const char *prefName, peStruct *yourPeStruct);
    void    *       handleEvent    (void);
    
protected:
    const char *mPrefName;
    peStruct *mPeStruct;
};

wsPrefChangedEvent::wsPrefChangedEvent(const char *prefName, 
                                       peStruct *yourPeStruct) :
    mPrefName(prefName), mPeStruct(yourPeStruct)
{
    
}

void *wsPrefChangedEvent::handleEvent()
{
    JNIEnv *env = (JNIEnv *) JNU_GetEnv(gVm, JNI_VERSION);
    jint result;
    jstring prefName;
#ifdef BAL_INTERFACE
#else
    jclass pcClass = nsnull;
    jmethodID pcMID = nsnull;
    
    if (!(pcClass = env->GetObjectClass(mPeStruct->callback))) {
        return (void*) NS_ERROR_FAILURE;
    }
    if (!(pcMID =env->GetMethodID(pcClass, "prefChanged",
                                  "(Ljava/lang/String;Ljava/lang/Object;)I"))){
        return (void*) NS_ERROR_FAILURE;
    }
    if (!(prefName = ::util_NewStringUTF(env, mPrefName))) {
        return (void*) NS_ERROR_FAILURE;
    }
    result = env->CallIntMethod(mPeStruct->callback, pcMID, prefName, 
                                mPeStruct->obj);
    
#endif
    return (void *) result;
}


void prefEnumerator(const char *name, void *closure);

static int PR_CALLBACK prefChanged(const char *name, void *closure);

void prefEnumerator(const char *name, void *closure)
{
    JNIEnv *env = (JNIEnv *) JNU_GetEnv(gVm, JNI_VERSION);
    if (nsnull == closure) {
        return;
    }
    peStruct *pes = (peStruct *) closure;
    WebShellInitContext *mInitContext =  pes->cx;
    jobject props = pes->obj;
    PRInt32 prefType, intVal;
    PRBool boolVal;
    nsresult rv = NS_ERROR_FAILURE;
    jstring prefName = nsnull;
    jstring prefValue = nsnull;
    PRUnichar *prefValueUni = nsnull;
    nsAutoString prefValueAuto;
    const PRInt32 bufLen = 20;
    char buf[bufLen];
    nsCRT::memset(buf, 0, bufLen);
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));

    if (nsnull == props || !prefs) {
        return;
    }
    if (NS_FAILED(prefs->GetPrefType(name, &prefType))) {
        return;
    }
    
    if (nsnull == (prefName = ::util_NewStringUTF(env, name))) {
        return;
    }

    switch(prefType) {
    case nsIPref::ePrefInt:
        if (NS_SUCCEEDED(prefs->GetIntPref(name, &intVal))) {
            WC_ITOA(intVal, buf, 10);
            prefValue = ::util_NewStringUTF(env, buf);
        }
        break;
    case nsIPref::ePrefBool:
        if (NS_SUCCEEDED(prefs->GetBoolPref(name, &boolVal))) {
            if (boolVal) {
                prefValue = ::util_NewStringUTF(env, "true");
            }
            else {
                prefValue = ::util_NewStringUTF(env, "false");
            }
        }
        break;
    case nsIPref::ePrefString:
        if (NS_SUCCEEDED(prefs->CopyUnicharPref(name, &prefValueUni))) {
            prefValueAuto = prefValueUni;
            prefValue = ::util_NewString(env, (const jchar *) prefValueUni,
                                         prefValueAuto.Length());
            delete [] prefValueUni;
        }
        break;
    default:
        PR_ASSERT(PR_TRUE);
        break;
    }
    if (nsnull == prefValue) {
        prefValue = ::util_NewStringUTF(env, "");
    }
    ::util_StoreIntoPropertiesObject(env, props, prefName, prefValue, 
                                     (jobject) &(mInitContext->shareContext));
}

static int PR_CALLBACK prefChanged(const char *name, void *closure)
{
    if (nsnull == name || nsnull == closure) {
        return NS_ERROR_NULL_POINTER;
    }
    nsresult rv;
    JNIEnv *env = (JNIEnv *) JNU_GetEnv(gVm, JNI_VERSION);
    wsPrefChangedEvent *actionEvent = nsnull;
    peStruct *pes = (peStruct *) closure;
    void *voidResult = nsnull;
    jstring prefName;

    if (!(prefName = ::util_NewStringUTF(env, name))) {
        rv = NS_ERROR_NULL_POINTER;
        goto PC_CLEANUP;
    }

    if (!(actionEvent = new wsPrefChangedEvent(name, (peStruct *) closure))) {
        rv = NS_ERROR_NULL_POINTER;
        goto PC_CLEANUP;
    }
    
    voidResult = ::util_PostSynchronousEvent(pes->cx,(PLEvent *) *actionEvent);
    rv = (nsresult) voidResult;
 PC_CLEANUP:

    return rv;
}



JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_wrapper_1native_PreferencesImpl_nativeSetUnicharPref
(JNIEnv *env, jobject obj, jstring prefName, jstring prefValue)
{
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    const char	*	prefNameChars = (char *) ::util_GetStringUTFChars(env, 
                                                                      prefName);
    const jchar	*	prefValueChars = (jchar *) ::util_GetStringChars(env, 
                                                                     prefValue);
    nsresult rv = NS_ERROR_FAILURE;
    
    if (!prefs) {
        ::util_ThrowExceptionToJava(env, "nativeSetUnicharPref: can't get prefs service");
        return;
    }
    if (nsnull == prefNameChars) {
        ::util_ThrowExceptionToJava(env, "nativeSetUnicharPref: unable to extract Java string for pref name");
        return;
    }
    if (nsnull == prefValueChars) {
        ::util_ThrowExceptionToJava(env, "nativeSetUnicharPref: unable to extract Java string for pref value");
        return;
    }
    rv = prefs->SetUnicharPref(prefNameChars, (const PRUnichar *) prefValueChars);
    ::util_ReleaseStringUTFChars(env, prefName, prefNameChars);
    ::util_ReleaseStringChars(env, prefName, prefValueChars);
    
    if (NS_FAILED(rv)) {
        // PENDING(edburns): set a more specific kind of pref
        ::util_ThrowExceptionToJava(env, "nativeSetUnicharPref: can't set pref");
    }
    
    return;
}


JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_wrapper_1native_PreferencesImpl_nativeSetIntPref
(JNIEnv *env, jobject obj, jstring prefName, jint prefValue)
{
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    const char	*	prefNameChars = (char *) ::util_GetStringUTFChars(env, 
                                                                      prefName);
    nsresult rv = NS_ERROR_FAILURE;
    
    if (!prefs) {
        ::util_ThrowExceptionToJava(env, "nativeSetIntPref: can't get prefs service");
        return;
    }
    if (nsnull == prefNameChars) {
        ::util_ThrowExceptionToJava(env, "nativeSetIntPref: unable to extract Java string");
        return;
    }
    rv = prefs->SetIntPref(prefNameChars, (PRInt32) prefValue);
    ::util_ReleaseStringUTFChars(env, prefName, prefNameChars);
    
    if (NS_FAILED(rv)) {
        // PENDING(edburns): set a more specific kind of pref
        ::util_ThrowExceptionToJava(env, "nativeSetIntPref: can't set pref");
    }
    
    return;
}


JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_wrapper_1native_PreferencesImpl_nativeSetBoolPref
(JNIEnv *env, jobject obj, jstring prefName, jboolean prefValue)
{
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    const char	*	prefNameChars = (char *) ::util_GetStringUTFChars(env, 
                                                                      prefName);
    nsresult rv = NS_ERROR_FAILURE;

    if (!prefs) {
        ::util_ThrowExceptionToJava(env, "nativeSetBoolPref: can't get prefs service");
        return;
    }
    if (nsnull == prefNameChars) {
        ::util_ThrowExceptionToJava(env, "nativeSetBoolPref: unable to extract Java string");
        return;
    }
    rv = prefs->SetBoolPref(prefNameChars, (PRBool) prefValue);
    ::util_ReleaseStringUTFChars(env, prefName, prefNameChars);

    if (NS_FAILED(rv)) {
        // PENDING(edburns): set a more specific kind of pref
        ::util_ThrowExceptionToJava(env, "nativeSetBoolPref: can't set pref");
    }

    return;
}

JNIEXPORT jobject JNICALL 
Java_org_mozilla_webclient_wrapper_1native_PreferencesImpl_nativeGetPrefs
(JNIEnv *env, jobject obj, jint webShellPtr, jobject props)
{
    nsresult rv = NS_ERROR_FAILURE;
    jobject newProps;
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    WebShellInitContext* initContext = (WebShellInitContext *) webShellPtr;

    if (!prefs) {
        ::util_ThrowExceptionToJava(env, "nativeGetPrefs: can't get prefs service");
        return props;
    }

    if (initContext == nsnull) {
      ::util_ThrowExceptionToJava(env, "Exception: null webShellPtr passed to netiveGetPrefs");
      return props;
    }

    PR_ASSERT(initContext->initComplete);

    // step one: create or clear props
    if (nsnull == props) {
        if (nsnull == 
            (newProps = 
             ::util_CreatePropertiesObject(env, (jobject)
                                           &(initContext->shareContext)))) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeGetPrefs: can't create prefs.");
            return props;
        }
        if (nsnull == (props = ::util_NewGlobalRef(env, newProps))) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeGetPrefs: can't create global ref for prefs.");
            return props;
        }
    }
    else {
        ::util_ClearPropertiesObject(env, props, (jobject) 
                                     &(initContext->shareContext));
        
    }
    PR_ASSERT(props);

    // step two, call the magic enumeration function, to populate the
    // properties
    peStruct pes;
    pes.cx = initContext;
    pes.obj = props;
    prefs->EnumerateChildren("", prefEnumerator, &pes);
            
    return props;
}

JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_wrapper_1native_PreferencesImpl_nativeRegisterPrefChangedCallback
(JNIEnv *env, jobject obj, jint webShellPtr, 
 jobject callback, jstring prefName, jobject closure)
{
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
    WebShellInitContext* initContext = (WebShellInitContext *) webShellPtr;
    const char *prefNameChars;
    
    if (!prefs) {
        ::util_ThrowExceptionToJava(env, "nativeGetPrefs: can't get prefs service");
        return;
    }
    
    if (initContext == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null webShellPtr passed to nativeRegisterPrefChangedCallback");
        return;
    }
    
    PR_ASSERT(initContext->initComplete);

    if (nsnull == (callback = ::util_NewGlobalRef(env, callback))) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeRegisterPrefChangedCallback: can't global ref for callback");
        return;
    }
    
    if (nsnull == (closure = ::util_NewGlobalRef(env, closure))) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeRegisterPrefChangedCallback: can't global ref for closure");
        return;
    }
    
    // step one, set up our struct
    peStruct *pes;
    
    if (nsnull == (pes = new peStruct())) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeRegisterPrefChangedCallback: can't get peStruct");
        return;
    }
    
    pes->cx = initContext;
    pes->obj = closure;
    pes->callback = callback;

    // step two: create a const char * from the prefName
    if (nsnull == (prefNameChars = ::util_GetStringUTFChars(env, prefName))) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeRegisterPrefChangedCallback: can't get string for prefName");
        return;
    }
    prefs->RegisterCallback(prefNameChars, prefChanged, pes);

    ::util_ReleaseStringUTFChars(env, prefName, prefNameChars);
            
    return;
}
