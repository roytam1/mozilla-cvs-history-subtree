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

#include "jsarray.h"

namespace JavaScript {    
namespace JS2Runtime {

JSValue Array_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    if (thisValue->isNull())
        thisValue = new JSValue(Array_Type->newInstance(cx));
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;
    if (argc > 0) {
        if (argc == 1) {
            arrInst->mLength = (uint32)(argv[0].toNumber(cx).f64);
           // arrInst->mInstanceValues = new JSValue[arrInst->mLength];
        }
        else {
            arrInst->mLength = argc;
            //arrInst->mInstanceValues = new JSValue[arrInst->mLength];
            for (uint32 i = 0; i < argc; i++) {
                String *id = numberToString(i);
                arrInst->defineVariable(*id, NULL, Object_Type, argv[i]);
                delete id;
            }
        }
    }
    return *thisValue;
}

JSValue Array_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    if (arrInst->mLength == 0)
        return JSValue(new String(widenCString("")));
    else {
        String *s = new String();
        for (uint32 i = 0; i < arrInst->mLength; i++) {
            String *id = numberToString(i);
            arrInst->getProperty(cx, *id, NULL);
            JSValue result = cx->mStack.back();
            cx->mStack.pop_back();
            s->append(*result.toString(cx).string);
            if (i < (arrInst->mLength - 1))
                s->append(widenCString(","));
        }
        return JSValue(s);
    }
    
}

JSValue Array_toSource(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    if (arrInst->mLength == 0)
        return JSValue(new String(widenCString("[]")));
    else {
        String *s = new String(widenCString("["));
        for (uint32 i = 0; i < arrInst->mLength; i++) {
            String *id = numberToString(i);
            arrInst->getProperty(cx, *id, NULL);
            JSValue result = cx->mStack.back();
            cx->mStack.pop_back();
            if (!result.isUndefined())
                s->append(*result.toString(cx).string);
            if (i < (arrInst->mLength - 1))
                s->append(widenCString(", "));
        }
        s->append(widenCString("]"));
        return JSValue(s);
    }
    
}

JSValue Array_push(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    for (uint32 i = 0; i < argc; i++) {
        String *id = numberToString(i + arrInst->mLength);
        arrInst->defineVariable(*id, NULL, Object_Type, argv[i]);
        delete id;
    }
    arrInst->mLength += argc;
    return JSValue((float64)arrInst->mLength);
}
              
JSValue Array_pop(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    if (arrInst->mLength > 0) {
        String *id = numberToString(arrInst->mLength - 1);
        arrInst->getProperty(cx, *id, NULL);
        JSValue result = cx->mStack.back();
        cx->mStack.pop_back();
        arrInst->deleteProperty(*id, NULL);
        --arrInst->mLength;
        delete id;
        return result;
    }
    else
        return kUndefinedValue;
}

Context::PrototypeFunctions *getArrayProtos()
{
    Context::ProtoFunDef arrayProtos[] = 
    {
        { "toString", String_Type, 0, Array_toString },
        { "toSource", String_Type, 0, Array_toSource },
        { "push",     Number_Type, 1, Array_push },
        { "pop",      Object_Type, 0, Array_pop },
        { NULL }
    };
    return new Context::PrototypeFunctions(&arrayProtos[0]);
}

}
}