/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the JavaScript 2 Prototype.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#endif

#include <algorithm>

#include "parser.h"
#include "numerics.h"
#include "js2runtime.h"

#include "jsstring.h"

namespace JavaScript {    
namespace JS2Runtime {


JSValue String_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    if (thisValue->isNull())
        thisValue = new JSValue(String_Type->newInstance(cx));
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSStringInstance *>(thisObj));
    JSStringInstance *strInst = (JSStringInstance *)thisObj;

    if (argc > 0)
        thisObj->mPrivate = (void *)(new String(*argv[0].toString(cx).string));
    else
        thisObj->mPrivate = (void *)(new String(widenCString("")));
    strInst->mLength = ((String *)(thisObj->mPrivate))->size();
    return *thisValue;
}

JSValue String_fromCharCode(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    String *resultStr = new String();
    resultStr->reserve(argc);
    for (uint32 i = 0; i < argc; i++)
        *resultStr += (char16)(argv[i].toUInt16(cx).f64);

    return JSValue(resultStr);
}

JSValue String_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    return JSValue((String *)thisObj->mPrivate);
}

struct MatchResult {
    bool failure;
    uint32 endIndex;
    String **captures;
};

void splitMatch(const String *S, uint32 q, const String *R, MatchResult &result)
{
    result.failure = true;
    result.captures = NULL;

    uint32 r = R->size();
    uint32 s = S->size();
    if ((q + r) > s)
        return;
    for (int i = 0; i < r; i++) {
        if ((*S)[q + i] != (*R)[i])
            return;
    }
    result.endIndex = q + r;
    result.failure = false;
}


JSValue String_toLowerCase(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSValue S = thisValue->toString(cx);

    String *result = new String(*S.string);
    for (String::iterator i = result->begin(), end = result->end(); i != end; i++)
        *i = toLower(*i);

    return JSValue(result);
}

JSValue String_toUpperCase(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSValue S = thisValue->toString(cx);

    String *result = new String(*S.string);
    for (String::iterator i = result->begin(), end = result->end(); i != end; i++)
        *i = toUpper(*i);

    return JSValue(result);
}

JSValue String_split(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSValue S = thisValue->toString(cx);

    JSArrayInstance *A = (JSArrayInstance *)Array_Type->newInstance(cx);
    JSValue separatorV;
    JSValue limitV;
    uint32 lim;

    if (argc > 0)
        separatorV = argv[0];
    if (argc > 1)
        limitV = argv[1];
    
    if (limitV.isUndefined())
        lim = (uint32)two32minus1;
    else
        lim = (uint32)(limitV.toUInt32(cx).f64);

    uint32 s = S.string->size();
    uint32 p = 0;

    // if separatorV.isRegExp() -->

    const String *R = separatorV.toString(cx).string;

    if (lim == 0) 
        return JSValue(A);

    if (separatorV.isUndefined()) {
        A->setProperty(cx, widenCString("0"), NULL, S);
        return JSValue(A);
    }

    if (s == 0) {
        MatchResult z;
        splitMatch(S.string, 0, R, z);
        if (!z.failure)
            return JSValue(A);
        A->setProperty(cx, widenCString("0"), NULL, S);
        return JSValue(A);
    }
    
    while (true) {
        uint32 q = p;
step11:
        if (q == s) {
            String *T = new String(*S.string, p, (s - p));
            JSValue v(T);
            A->setProperty(cx, *numberToString(A->mLength), NULL, v);
            return JSValue(A);
        }
        MatchResult z;
        splitMatch(S.string, q, R, z);
        if (z.failure) {
            q = q + 1;
            goto step11;
        }
        uint32 e = z.endIndex;
        if (e == p) {
            q = q + 1;
            goto step11;
        }
        String *T = new String(*S.string, p, (q - p));
        JSValue v(T);
        A->setProperty(cx, *numberToString(A->mLength), NULL, v);
        if (A->mLength == lim)
            return JSValue(A);
        p = e;
        // step 20 --> 27, handle captures array (we know it's empty for non regexp)    
    }

}

JSValue String_valueOf(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_charAt(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_charCodeAt(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_concat(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_indexOf(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_lastIndexOf(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_localeCompare(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_splice(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}

JSValue String_substring(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}


Context::PrototypeFunctions *getStringProtos()
{
    Context::ProtoFunDef stringProtos[] = 
    {
        { "toString",           String_Type, String_toString },
        { "valueof",            String_Type, String_valueOf },
        { "charAt",             String_Type, String_charAt },
        { "charCodeAt",         Number_Type, String_charCodeAt },
        { "concat",             String_Type, String_concat },
        { "indexOf",            Number_Type, String_indexOf },
        { "lastIndexOf",        Number_Type, String_lastIndexOf },
        { "localeCompare",      Number_Type, String_localeCompare },
        { "slice",              String_Type, String_splice },
        { "split",              Array_Type,  String_split },
        { "substring",          String_Type, String_substring },
        { "toSource",           String_Type, String_toString },
        { "toLocaleUpperCase",  String_Type, String_toUpperCase },  // (sic)
        { "toLocaleLowerCase",  String_Type, String_toLowerCase },  // (sic)
        { "toUpperCase",        String_Type, String_toUpperCase },
        { "toLowerCase",        String_Type, String_toLowerCase },
        { NULL }
    };
    return new Context::PrototypeFunctions(&stringProtos[0]);
}

}
}
