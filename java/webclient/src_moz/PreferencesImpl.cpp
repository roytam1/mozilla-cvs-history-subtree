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

#include "ns_util.h"

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

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

