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
#pragma warning(disable: 4711)
#pragma warning(disable: 4710)
#endif

#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "parser.h"
#include "numerics.h"
#include "js2runtime.h"
#include "bytecodegen.h"

#include "jsstring.h"
#include "jsarray.h"
#include "jsmath.h"

#include "fdlibm_ns.h"

// this is the AttributeList passed to the name lookup routines
#define CURRENT_ATTR    (NULL)

namespace JavaScript {    
namespace JS2Runtime {


//
// XXX don't these belong in the context? But don't
// they need to compare equal across contexts?
//
JSType *Object_Type = NULL;
JSType *Number_Type;
JSType *Integer_Type;
JSType *Function_Type;
JSStringType *String_Type;
JSType *Boolean_Type;
JSType *Type_Type;
JSType *Void_Type;
JSType *Unit_Type;
JSType *Attribute_Type;
JSArrayType *Array_Type;

JSInstance *NullAttribute;


NamespaceList *Context::buildNamespaceList(AttributeStmtNode *attr)
{
    if (attr == NULL)
        return NULL;

    JSArrayInstance *names = getNamesArray(attr->attributeValue);
    if (names->mLength == 0)
        return NULL;

    NamespaceList *result = NULL;
    for (uint32 k = 0; k < names->mLength; k++) {
        String *id = numberToString(k);
        names->getProperty(this, *id, NULL);
        JSValue name = popValue();
        ASSERT(name.isString());
        result = new NamespaceList(name.string, result);
    }
    return result;
}

JSInstance *Context::executeAttributes(ExprNode *attr)
{
    if (attr == NULL)
        return NullAttribute;

    ByteCodeGen bcg(this, mScopeChain);
    ByteCodeModule *bcm = bcg.genCodeForExpression(attr);
//    stdOut << *bcm;
    JSValue result = interpret(bcm, NULL, JSValue(getGlobalObject()), NULL, 0);

    ASSERT(result.isObject() && (result.object->getType() == Attribute_Type));
    return static_cast<JSInstance *>(result.object);
}


JSType *Context::getExtendArg(JSObject *attributeValue)
{
    attributeValue->getProperty(this, widenCString("extendArg"), (NamespaceList *)(NULL));
    JSValue extArg = popValue();
    ASSERT(extArg.isType());
    return extArg.type;
}

JSArrayInstance *Context::getNamesArray(JSObject *attributeValue)
{
    attributeValue->getProperty(this, widenCString("names"), (NamespaceList *)(NULL));
    JSValue names = popValue();
    ASSERT(names.isObject() && (names.object->getType() == Array_Type));
    return static_cast<JSArrayInstance *>(names.object);
}

// find a property by the given name, and then check to see if there's any
// overlap between the supplied attribute list and the property's list.
// ***** REWRITE ME -- matching attribute lists for inclusion is a bad idea.
// XXX it's re-written a little bit, but is still doing linear searching (o^2) ...
PropertyIterator JSObject::findNamespacedProperty(const String &name, NamespaceList *names)
{
    for (PropertyIterator i = mProperties.lower_bound(name), 
                    end = mProperties.upper_bound(name); (i != end); i++) {
        NamespaceList *propNames = PROPERTY_NAMESPACELIST(i);
        if (names) {
            if (propNames == NULL)
                continue;       // a namespace list was specified, no match
            while (names) {
                NamespaceList *propNameEntry = propNames;
                while (propNameEntry) {
                    if (names->mName == propNameEntry->mName)
                        return i;
                    propNameEntry = propNameEntry->mNext;
                }
                names = names->mNext;
            }
        }
        else {
            if (propNames)  // entry is in a namespace, but none called for, no match
                continue;
            return i;
        }
    }
    return mProperties.end();
}



/*---------------------------------------------------------------------------------------------*/











// see if the property exists by a specific kind of access
bool JSObject::hasOwnProperty(const String &name, NamespaceList *names, Access acc, PropertyIterator *p)
{
    *p = findNamespacedProperty(name, names);
    if (*p != mProperties.end()) {
        Property *prop = PROPERTY(*p);
        if (prop->mFlag == FunctionPair)
            return (acc == Read) ? (prop->mData.fPair.getterF != NULL)
                                 : (prop->mData.fPair.setterF != NULL);
        else
            if (prop->mFlag == IndexPair)
                return (acc == Read) ? (prop->mData.iPair.getterI != toUInt32(-1))
                                     : (prop->mData.iPair.setterI != toUInt32(-1));
            else
                return true;
    }
    else
        return false;
}

bool JSObject::hasProperty(const String &name, NamespaceList *names, Access acc, PropertyIterator *p)
{
    if (hasOwnProperty(name, names, acc, p))
        return true;
    else
        if (mPrototype)
            return mPrototype->hasProperty(name, names, acc, p);
        else
            return false;
}


// get a property value
JSValue JSObject::getPropertyValue(PropertyIterator &i)
{
    Property *prop = PROPERTY(i);
    ASSERT(prop->mFlag == ValuePointer);
    return *prop->mData.vp;
}


void JSObject::defineGetterMethod(Context *cx, const String &name, AttributeStmtNode *attr, JSFunction *f)
{
    NamespaceList *names = cx->buildNamespaceList(attr);
    PropertyIterator i;
    if (hasProperty(name, names, Write, &i)) {
        ASSERT(PROPERTY_KIND(i) == FunctionPair);
        ASSERT(PROPERTY_GETTERF(i) == NULL);
        PROPERTY_GETTERF(i) = f;
    }
    else {
        const PropertyMap::value_type e(name, new NamespacedProperty(new Property(Function_Type, f, NULL), names));
        mProperties.insert(e);
    }
}
void JSObject::defineSetterMethod(Context *cx, const String &name, AttributeStmtNode *attr, JSFunction *f)
{
    NamespaceList *names = cx->buildNamespaceList(attr);
    PropertyIterator i;
    if (hasProperty(name, names, Read, &i)) {
        ASSERT(PROPERTY_KIND(i) == FunctionPair);
        ASSERT(PROPERTY_SETTERF(i) == NULL);
        PROPERTY_SETTERF(i) = f;
    }
    else {
        const PropertyMap::value_type e(name, new NamespacedProperty(new Property(Function_Type, NULL, f), names));
        mProperties.insert(e);
    }
}

// add a property
Property *JSObject::defineVariable(Context *cx, const String &name, AttributeStmtNode *attr, JSType *type)
{
    Property *prop = new Property(new JSValue(), type);
    const PropertyMap::value_type e(name, new NamespacedProperty(prop, cx->buildNamespaceList(attr)));
    mProperties.insert(e);
    return prop;
}
Property *JSObject::defineVariable(Context * /*cx*/, const String &name, NamespaceList *names, JSType *type)
{
    Property *prop = new Property(new JSValue(), type);
    const PropertyMap::value_type e(name, new NamespacedProperty(prop, names));
    mProperties.insert(e);
    return prop;
}

// add a property (with a value)
Property *JSObject::defineVariable(Context *cx, const String &name, AttributeStmtNode *attr, JSType *type, JSValue v)
{
    Property *prop = new Property(new JSValue(v), type);
    const PropertyMap::value_type e(name, new NamespacedProperty(prop, cx->buildNamespaceList(attr)));
    mProperties.insert(e);
    return prop;
}
Property *JSObject::defineVariable(Context * /*cx*/, const String &name, NamespaceList *names, JSType *type, JSValue v)
{
    Property *prop = new Property(new JSValue(v), type);
    const PropertyMap::value_type e(name, new NamespacedProperty(prop, names));
    mProperties.insert(e);
    return prop;
}



Reference *JSObject::genReference(const String& name, NamespaceList *names, Access acc, uint32 /*depth*/)
{
    PropertyIterator i;
    if (hasProperty(name, names, acc, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case ValuePointer:
            return new PropertyReference(name, acc, prop->mType);
        case FunctionPair:
            if (acc == Read)
                return new GetterFunctionReference(prop->mData.fPair.getterF);
            else {
                JSFunction *f = prop->mData.fPair.setterF;
                return new SetterFunctionReference(f, f->getArgType(0));
            }
        default:
            NOT_REACHED("bad storage kind");
            return NULL;
        }
    }
    NOT_REACHED("bad genRef call");
    return NULL;
}


void JSObject::getProperty(Context *cx, const String &name, NamespaceList *names)
{
    PropertyIterator i;
    if (hasProperty(name, names, Read, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case ValuePointer:
            cx->pushValue(*prop->mData.vp);
            break;
        case FunctionPair:
             cx->pushValue(cx->invokeFunction(prop->mData.fPair.getterF, JSValue(this), NULL, 0));
            break;
        case Constructor:
        case Method:
            cx->pushValue(JSValue(mType->mMethods[prop->mData.index]));
            break;
        default:
            ASSERT(false);  // XXX more to implement
            break;
        }
    }
    else
        cx->pushValue(kUndefinedValue);        
}



void JSInstance::getProperty(Context *cx, const String &name, NamespaceList *names)
{
    PropertyIterator i;
    if (hasProperty(name, names, Read, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case Slot:
            cx->pushValue(mInstanceValues[prop->mData.index]);
            break;
        case ValuePointer:
            cx->pushValue(*prop->mData.vp);
            break;
        case FunctionPair:
            cx->pushValue(cx->invokeFunction(prop->mData.fPair.getterF, JSValue(this), NULL, 0));
            break;
        case Constructor:
        case Method:
            cx->pushValue(JSValue(mType->mMethods[prop->mData.index]));
            break;
        case IndexPair:
            cx->pushValue(cx->invokeFunction(mType->mMethods[prop->mData.iPair.getterI], JSValue(this), NULL, 0));
            break;
        default:
            ASSERT(false);  // XXX more to implement
            break;
        }
    }
    else {
        if (mType->mStatics && mType->mStatics->hasProperty(name, names, Read, &i))
            mType->mStatics->getProperty(cx, name, names);
        else
            JSObject::getProperty(cx, name, names);
    }
}

void JSObject::setProperty(Context *cx, const String &name, NamespaceList *names, const JSValue &v)
{
    PropertyIterator i;
    if (hasProperty(name, names, Write, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case ValuePointer:
            *prop->mData.vp = v;
            break;
        case FunctionPair:
            {
                JSValue argv = v;
                cx->pushValue(cx->invokeFunction(prop->mData.fPair.setterF, JSValue(this), &argv, 1));
            }
            break;
        default:
            ASSERT(false);  // XXX more to implement ?
            break;
        }
    }
    else {
        defineVariable(cx, name, names, Object_Type, v);
    }
}

void JSInstance::setProperty(Context *cx, const String &name, NamespaceList *names, const JSValue &v)
{
    PropertyIterator i;
    if (hasProperty(name, names, Write, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case Slot:
            mInstanceValues[prop->mData.index] = v;
            break;
        case ValuePointer:
            *prop->mData.vp = v;
            break;
        case FunctionPair: 
            {
                JSValue argv = v;
                cx->pushValue(cx->invokeFunction(prop->mData.fPair.setterF, JSValue(this), &argv, 1));
            }
            break;
        case IndexPair: 
            {
                JSValue argv = v;
                cx->pushValue(cx->invokeFunction(mType->mMethods[prop->mData.iPair.setterI], JSValue(this), &argv, 1));
            }
            break;
        default:
            ASSERT(false);  // XXX more to implement ?
            break;
        }
    }
    else {
        if (mType->mStatics && mType->mStatics->hasProperty(name, names, Write, &i)) {
            mType->mStatics->setProperty(cx, name, names, v);
        }
        else {
            defineVariable(cx, name, names, Object_Type, v);
        }
    }
}

void JSArrayInstance::getProperty(Context *cx, const String &name, NamespaceList *names)
{
    if (name.compare(widenCString("length")) == 0) {
        cx->pushValue(JSValue((float64)mLength));
    }
    else
        JSInstance::getProperty(cx, name, names);
}

void JSArrayInstance::setProperty(Context *cx, const String &name, NamespaceList *names, const JSValue &v)
{
    if (name.compare(widenCString("length")) == 0) {
        uint32 newLength = (uint32)(v.toUInt32(cx).f64);
        if (newLength != v.toNumber(cx).f64)
            cx->reportError(Exception::rangeError, "out of range value for length"); 
        
        for (uint32 i = newLength; i < mLength; i++) {
            String *id = numberToString(i);
            if (findNamespacedProperty(*id, NULL) != mProperties.end())
                deleteProperty(*id, NULL);
            delete id;
        }

        mLength = newLength;
    }
    else {
        if (findNamespacedProperty(name, names) == mProperties.end())
            defineVariable(cx, name, names, Object_Type, v);
        else
            JSInstance::setProperty(cx, name, names, v);
        JSValue v = JSValue(&name);
        JSValue v_int = v.toUInt32(cx);
        if ((v_int.f64 != two32minus1) && (v_int.toString(cx).string->compare(name) == 0)) {
            if (v_int.f64 >= mLength)
                mLength = (uint32)(v_int.f64) + 1;
        }
    }
}

void JSStringInstance::getProperty(Context *cx, const String &name, NamespaceList *names)
{
    if (name.compare(widenCString("length")) == 0) {
        cx->pushValue(JSValue((float64)mLength));
    }
    else
        JSInstance::getProperty(cx, name, names);
}


void JSInstance::initInstance(Context *, JSType *type)
{
    if (type->mVariableCount)
        mInstanceValues = new JSValue[type->mVariableCount];

    // copy the instance variable names into the property map
    for (PropertyIterator pi = type->mProperties.begin(), 
                end = type->mProperties.end();
                (pi != end); pi++) {            
        const PropertyMap::value_type e(PROPERTY_NAME(pi), NAMESPACED_PROPERTY(pi));
        mProperties.insert(e);
    }

    // and then do the same for the super types
    JSType *t = type->mSuperType;
    while (t) {
        for (PropertyIterator i = t->mProperties.begin(), 
                    end = t->mProperties.end();
                    (i != end); i++) {            
            const PropertyMap::value_type e(PROPERTY_NAME(i), NAMESPACED_PROPERTY(i));
            mProperties.insert(e);            
        }
        t = t->mSuperType;
    }

    // copy instance values from the Ur-instance object
    if (type->mInitialInstance)
        for (uint32 i = 0; i < type->mVariableCount; i++)
            mInstanceValues[i] = type->mInitialInstance->mInstanceValues[i];
    mType = type;
}

// Create a new (empty) instance of this class. The prototype
// link for this new instance is established from the type's
// prototype object.
JSInstance *JSType::newInstance(Context *cx)
{
    JSInstance *result = new JSInstance(cx, this);
    result->mPrototype = mPrototypeObject;
    return result;
}

// allocate a new (empty) instance of the class and then
// run the instance initializer on that object. This becomes
// the initial instance that then gets copied into each new
// instance of this class.
void JSType::setInstanceInitializer(Context *cx, JSFunction *f)
{
    mInitialInstance = newInstance(cx);
    if (mVariableCount) {
        mInitialInstance->mInstanceValues = new JSValue[mVariableCount];
        if (mSuperType) {
            for (uint32 i = 0; i < mSuperType->mVariableCount; i++)
                mInitialInstance->mInstanceValues[i] = mSuperType->mInitialInstance->mInstanceValues[i];
        }
    }
    if (f) {
        JSValue thisValue(mInitialInstance);
        cx->interpret(f->getByteCode(), f->getScopeChain(), thisValue, NULL, 0);
    }
}

// Initialize the static instance and run the static initializer against it
void JSType::setStaticInitializer(Context *cx, JSFunction *f)
{
    ASSERT(mStatics);
    
    // build the static instance object
    if (mStatics->mVariableCount)
        mStatics->mInstanceValues = new JSValue[mStatics->mVariableCount];

/*
    XXX If any of the fields are marked as prototype, define a variable
    called 'prototype' and point it at the type's prototype object

    ??defineStaticVariable(widenCString("prototype"), NULL, Object_Type);
    ??PropertyIterator i;
    ??if (mStatics->hasProperty(widenCString("prototype"), NULL, Read, &i) {
    ??    ASSERT(PROPERTY_KIND(i) == Slot);
    ??    mStatics->mInstanceValues[PROPERTY_INDEX(i)] = JSValue(type->mSuperType->mPrototype);
    ??}
*/
    if (f) {
        JSValue thisValue(mStatics);
        cx->interpret(f->getByteCode(), f->getScopeChain(), thisValue, NULL, 0);
    }
}

void JSType::defineConstructor(Context *cx, const String& name, AttributeStmtNode *attr, JSFunction *f)
{
    ASSERT(mStatics);
    uint32 vTableIndex = mStatics->mMethods.size();
    mStatics->mMethods.push_back(f);

    const PropertyMap::value_type e(name, new NamespacedProperty(new Property(vTableIndex, Function_Type, Constructor), cx->buildNamespaceList(attr)));
    mStatics->mProperties.insert(e);
}

Property *JSType::defineVariable(Context *cx, const String& name, AttributeStmtNode *attr, JSType *type)
{
    Property *prop = new Property(mVariableCount++, type, Slot);
    const PropertyMap::value_type e(name, new NamespacedProperty(prop, cx->buildNamespaceList(attr)));
    mProperties.insert(e);
    return prop;
}

JSInstance *JSArrayType::newInstance(Context *cx)
{
    JSInstance *result = new JSArrayInstance(cx, this);
    result->mPrototype = mPrototypeObject;
    return result;
}

JSInstance *JSStringType::newInstance(Context *cx)
{
    JSInstance *result = new JSStringInstance(cx, this);
    result->mPrototype = mPrototypeObject;
    return result;
}

void ScopeChain::setNameValue(const String& name, AttributeStmtNode *attr, Context *cx)
{
    NamespaceList *names = cx->buildNamespaceList(attr);
    PropertyIterator i;
    JSObject *top = *mScopeStack.rbegin();
    JSValue v = cx->topValue();
    if (top->hasProperty(name, names, Write, &i)) {
        if (PROPERTY_KIND(i) == ValuePointer) {
            *PROPERTY_VALUEPOINTER(i) = v;
        }
        else
            ASSERT(false);      // what else needs to be implemented ?
    }
    else {
        cx->getGlobalObject()->defineVariable(cx, name, attr, Object_Type, v);
    }
}

void ScopeChain::getNameValue(const String& name, AttributeStmtNode *attr, Context *cx)
{
    NamespaceList *names = cx->buildNamespaceList(attr);
    uint32 depth = 0;
    for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
    {
        PropertyIterator i;
        if ((*s)->hasProperty(name, names, Read, &i)) {
            if (PROPERTY_KIND(i) == ValuePointer) {
                cx->pushValue(*PROPERTY_VALUEPOINTER(i));
            }
            else
                ASSERT(false);      // what else needs to be implemented ?
            return;
        }
    }
    m_cx->reportError(Exception::referenceError, "Not defined");
}





Reference *ParameterBarrel::genReference(const String& name, NamespaceList *names, Access acc, uint32 /*depth*/)
{
    PropertyIterator i;
    if (hasProperty(name, names, acc, &i)) {
        Property *prop = PROPERTY(i);
        ASSERT(prop->mFlag == Slot);
        return new ParameterReference(prop->mData.index, acc, prop->mType);
    }
    NOT_REACHED("bad genRef call");
    return NULL;
}













JSType *ScopeChain::findType(const StringAtom& typeName, size_t pos) 
{
    JSValue v = getCompileTimeValue(typeName, NULL);
    if (v.isType())
        return v.type;
    else {
        // Allow finding a function that has the same name as it's containing class
        // i.e. the default constructor.
        if (v.isFunction() && 
                (v.function->getClass() && (v.function->getClass()->mClassName.compare(v.function->getFunctionName()) == 0)))
            return v.function->getClass();
        m_cx->reportError(Exception::semanticError, "Unknown type", pos);
        return NULL;
    }
}

// Take the specified type in 't' and see if we have a compile-time
// type value for it. FindType will throw an error if a type by
// that name doesn't exist.
JSType *ScopeChain::extractType(ExprNode *t)
{
    JSType *type = Object_Type;
    if (t) {
        if (t->getKind() == ExprNode::identifier) {
            IdentifierExprNode* typeExpr = static_cast<IdentifierExprNode*>(t);
            type = findType(typeExpr->name, t->pos);
        }
        else
            NOT_REACHED("implement me - more complex types");
    }
    return type;
}

// return the type of the index'th parameter in function
JSType *Context::getParameterType(FunctionDefinition &function, int index)
{
    VariableBinding *v = function.parameters;
    while (v) {
        if (index-- == 0)
            return mScopeChain->extractType(v->type);
        else
            v = v->next;
    }
    return NULL;
}

// counts the number of pigs that can fit in small wicker basket
uint32 Context::getParameterCount(FunctionDefinition &function)
{
    uint32 count = 0;
    VariableBinding *v = function.parameters;
    while (v) {
        count++;
        v = v->next;
    }
    return count;
}


// Iterates over the linked list of statements, p.
// 1. Adds 'symbol table' entries for each class, var & function
// 2. Using information from pass 1, evaluate types (and XXX later XXX other
//      compile-time constants)
void Context::buildRuntime(StmtNode *p)
{
    ContextStackReplacement csr(this);

    mScopeChain->addScope(getGlobalObject());
    while (p) {
        mScopeChain->collectNames(p);         // adds declarations for each top-level entity in p
        buildRuntimeForStmt(p);               // adds definitions as they exist for ditto
        p = p->next;
    }
    mScopeChain->popScope();
}

// Generate bytecode for the linked list of statements in p
JS2Runtime::ByteCodeModule *Context::genCode(StmtNode *p, String /*sourceName*/)
{
    mScopeChain->addScope(getGlobalObject());
    JS2Runtime::ByteCodeGen bcg(this, mScopeChain);
    JS2Runtime::ByteCodeModule *result = bcg.genCodeForScript(p);
    mScopeChain->popScope();
    return result;
}


//  The first pass over the tree - it just installs the names of each declaration
void ScopeChain::collectNames(StmtNode *p)
{
    switch (p->getKind()) {
        // XXX - other statements, execute them (assuming they have constant control values) ?
        // or simply visit the contained blocks and process any references that need to be hoisted
    case StmtNode::Class:
        {
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            const StringAtom& name = classStmt->name;
            JSType *thisClass = new JSType(m_cx, name, NULL);

            m_cx->setAttributeValue(classStmt);            

            PropertyIterator it;
            if (hasProperty(name, NULL, Read, &it))
                m_cx->reportError(Exception::referenceError, "Duplicate class definition", p->pos);

            defineVariable(m_cx, name, classStmt, Type_Type, JSValue(thisClass));
            classStmt->mType = thisClass;
        }
        break;
    case StmtNode::block:
        {
            // should push a new Activation scope here?
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                collectNames(s);
                s = s->next;
            }            
        }
        break;
    case StmtNode::If:
    case StmtNode::With:
    case StmtNode::DoWhile:
    case StmtNode::While:
        {
            UnaryStmtNode *u = static_cast<UnaryStmtNode *>(p);
            collectNames(u->stmt);
        }
        break;
    case StmtNode::IfElse:
        {
            BinaryStmtNode *b = static_cast<BinaryStmtNode *>(p);
            collectNames(b->stmt);
            collectNames(b->stmt2);
        }
        break;
    case StmtNode::Try:
        {
            TryStmtNode *t = static_cast<TryStmtNode *>(p);
            if (t->catches) {
                CatchClause *c = t->catches;
                while (c) {
                    c->prop = defineVariable(m_cx, c->name, NULL, NULL);
                    c = c->next;
                }
            }
        }
        break;
    case StmtNode::For:
    case StmtNode::ForIn:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            if (f->initializer) collectNames(f->initializer);
        }
        break;
    case StmtNode::Const:
    case StmtNode::Var:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
            m_cx->setAttributeValue(vs);
            bool isStatic = (vs->attributeFlags & Property::Static) == Property::Static;

//            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                if (isStatic)
                    v->prop = defineStaticVariable(m_cx, *v->name, vs, NULL);
                else
                    v->prop = defineVariable(m_cx, *v->name, vs, NULL);
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            m_cx->setAttributeValue(f);

            bool isStatic = (f->attributeFlags & Property::Static) == Property::Static;
            bool isConstructor = (f->attributeFlags & Property::Constructor) == Property::Constructor;
            bool isOperator = (f->attributeFlags & Property::Operator) == Property::Operator;
            bool isPrototype = (f->attributeFlags & Property::Prototype) == Property::Prototype;
/*
            bool isStatic = hasAttribute(f->attributes, Token::Static);
            bool isConstructor = hasAttribute(f->attributes, m_cx->ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, m_cx->OperatorKeyWord);
            bool isPrototype = hasAttribute(f->attributes, m_cx->PrototypeKeyWord);
*/            
            JSFunction *fnc = new JSFunction(m_cx, NULL, m_cx->getParameterCount(f->function), this);
            fnc->setIsPrototype(isPrototype);
            f->mFunction = fnc;

            if (isOperator) {
                // no need to do anything yet, all operators are 'pre-declared'
            }
            else {
                const StringAtom& name = *f->function.name;
                fnc->setFunctionName(name);
                if (topClass())
                    fnc->setClass(topClass());
//                IdentifierExprNode *extendArg;
//                if (hasAttribute(f->attributes, m_cx->ExtendKeyWord, &extendArg)) 

                if ((f->attributeFlags & Property::Extend) == Property::Extend) {

                    JSType *extendedClass = m_cx->getExtendArg(f->attributeValue);
//                    JSType *extendedClass = extractType(extendArg);
                
                    // sort of want to fall into the code below, but use 'extendedClass' instead
                    // of whatever the topClass will turn out to be.
                    if (extendedClass->mClassName.compare(name) == 0)
                        isConstructor = true;       // can you add constructors?
                    if (isConstructor)
                        extendedClass->defineConstructor(m_cx, name, f, fnc);
                    else {
                        switch (f->function.prefix) {
                        case FunctionName::Get:
                            if (isStatic)
                                extendedClass->defineStaticGetterMethod(m_cx, name, f, fnc);
                            else
                                extendedClass->defineGetterMethod(m_cx, name, f, fnc);
                            break;
                        case FunctionName::Set:
                            if (isStatic)
                                extendedClass->defineStaticSetterMethod(m_cx, name, f, fnc);
                            else
                                extendedClass->defineSetterMethod(m_cx, name, f, fnc);
                            break;
                        case FunctionName::normal:
                            if (isStatic)
                                extendedClass->defineStaticMethod(m_cx, name, f, fnc);
                            else
                                extendedClass->defineMethod(m_cx, name, f, fnc);
                            break;
                        default:
                            NOT_REACHED("***** implement me -- throw an error because the user passed a quoted function name");
                            break;
                        }
                    }                    
                }
                else {
                    if (topClass() && (topClass()->mClassName.compare(name) == 0))
                        isConstructor = true;
                    if (isConstructor)
                        defineConstructor(m_cx, name, f, fnc);
                    else {
                        switch (f->function.prefix) {
                        case FunctionName::Get:
                            if (isStatic)
                                defineStaticGetterMethod(m_cx, name, f, fnc);
                            else
                                defineGetterMethod(m_cx, name, f, fnc);
                            break;
                        case FunctionName::Set:
                            if (isStatic)
                                defineStaticSetterMethod(m_cx, name, f, fnc);
                            else
                                defineSetterMethod(m_cx, name, f, fnc);
                            break;
                        case FunctionName::normal:
                            if (isStatic)
                                defineStaticMethod(m_cx, name, f, fnc);
                            else
                                defineMethod(m_cx, name, f, fnc);
                            break;
                        default:
                            NOT_REACHED("***** implement me -- throw an error because the user passed a quoted function name");
                            break;
                        }
                    }
                }
            }
        }
        break;
    case StmtNode::Namespace:
        {
            NamespaceStmtNode *n = static_cast<NamespaceStmtNode *>(p);
            JSInstance *i = Attribute_Type->newInstance(m_cx);
            i->setProperty(m_cx, widenCString("trueFlags"), (NamespaceList *)(NULL), kPositiveZero);
            i->setProperty(m_cx, widenCString("falseFlags"), (NamespaceList *)(NULL), kPositiveZero);
            JSInstance *names = Array_Type->newInstance(m_cx);
            names->setProperty(m_cx, widenCString("0"), (NamespaceList *)(NULL), JSValue(&n->name));
            i->setProperty(m_cx, widenCString("names"), (NamespaceList *)(NULL), JSValue(names));
            m_cx->getGlobalObject()->defineVariable(m_cx, n->name, (NamespaceList *)(NULL), Attribute_Type, JSValue(i));            
        }
        break;
    default:
        break;
    }
}

// CompleteClass - incorporates super class field & methods into
// this class.

// this needs to happen before any code is generated in this class
// since the code below assigns the slot indices for instance variables
void JSType::completeClass(Context *cx, ScopeChain *scopeChain, JSType *super)
{    
    // Note test of mStatics:
    // we want to complete the statics classes but not to the
    // extent of providing a default constructor.

    // if none exists, build a default constructor that calls 'super()'
    if (mStatics && getDefaultConstructor() == NULL) {
        JSFunction *fnc = new JSFunction(cx, Object_Type, 0, scopeChain);
        ByteCodeGen bcg(cx, scopeChain);

        if (mSuperType && mSuperType->getDefaultConstructor()) {
            bcg.addOp(LoadTypeOp);
            bcg.addPointer(this);
            bcg.addOp(NewThisOp);
            bcg.addOp(LoadThisOp);
            bcg.addOp(LoadFunctionOp);
            bcg.addPointer(mSuperType->getDefaultConstructor());
            bcg.addOpAdjustDepth(InvokeOp, -1);
            bcg.addLong(0);
            bcg.addByte(Explicit);
            bcg.addOp(PopOp);
        }
        bcg.addOp(LoadThisOp);
        ASSERT(bcg.mStackTop == 1);
        bcg.addOpSetDepth(ReturnOp, 0);
        fnc->setByteCode(new JS2Runtime::ByteCodeModule(&bcg));        

        scopeChain->defineConstructor(cx, mClassName, NULL, fnc);   // XXX attributes?
    }

    // add the super type instance variable count into the slot indices
    uint32 superInstanceVarCount = 0;
    uint32 super_vTableCount = 0;
    if (super) {
        superInstanceVarCount = super->mVariableCount;
        super_vTableCount = super->mMethods.size();
    }

    if (superInstanceVarCount) {
        mVariableCount += superInstanceVarCount;
        for (PropertyIterator i = mProperties.begin(), 
                    end = mProperties.end();
                    (i != end); i++) {            
            if (PROPERTY_KIND(i) == Slot)
                PROPERTY_INDEX(i) += superInstanceVarCount;
        }
    }
    
    // likewise for the vTable
    if (super_vTableCount) {
        for (PropertyIterator i = mProperties.begin(), 
                    end = mProperties.end();
                    (i != end); i++) {            
            if ((PROPERTY_KIND(i) == Method) 
                    || (PROPERTY_KIND(i) == Constructor))
                PROPERTY_INDEX(i) += super_vTableCount;
            else
                if (PROPERTY_KIND(i) == IndexPair) {
                    PROPERTY_GETTERI(i) += super_vTableCount;
                    PROPERTY_SETTERI(i) += super_vTableCount;
                }
        }
        mMethods.insert(mMethods.begin(), 
                            super->mMethods.begin(), 
                            super->mMethods.end());
    }
    // complete the static class (inherit static instances etc)
    if (mStatics && mSuperType)
        mStatics->completeClass(cx, scopeChain, mSuperType->mStatics);

}

void JSType::defineMethod(Context *cx, const String& name, AttributeStmtNode *attr, JSFunction *f)
{
    uint32 vTableIndex = mMethods.size();
    mMethods.push_back(f);

    const PropertyMap::value_type e(name, new NamespacedProperty(new Property(vTableIndex, Function_Type, Method), cx->buildNamespaceList(attr)));
    mProperties.insert(e);
}

void JSType::defineGetterMethod(Context *cx, const String &name, AttributeStmtNode *attr, JSFunction *f)
{
    PropertyIterator i;
    uint32 vTableIndex = mMethods.size();
    mMethods.push_back(f);

    NamespaceList *names = cx->buildNamespaceList(attr);

    if (hasProperty(name, names, Write, &i)) {
        ASSERT(PROPERTY_KIND(i) == IndexPair);
        ASSERT(PROPERTY_GETTERI(i) == 0);
        PROPERTY_GETTERI(i) = vTableIndex;
    }
    else {
        const PropertyMap::value_type e(name, new NamespacedProperty(new Property(vTableIndex, 0, Function_Type), names));
        mProperties.insert(e);
    }
}

void JSType::defineSetterMethod(Context *cx, const String &name, AttributeStmtNode *attr, JSFunction *f)
{
    PropertyIterator i;
    uint32 vTableIndex = mMethods.size();
    mMethods.push_back(f);

    NamespaceList *names = cx->buildNamespaceList(attr);

    if (hasProperty(name, names, Read, &i)) {
        ASSERT(PROPERTY_KIND(i) == IndexPair);
        ASSERT(PROPERTY_SETTERI(i) == 0);
        PROPERTY_SETTERI(i) = vTableIndex;
    }
    else {
        const PropertyMap::value_type e(name, new NamespacedProperty(new Property(0, vTableIndex, Function_Type), names));
        mProperties.insert(e);
    }
}

bool JSType::derivesFrom(JSType *other)
{
    if (mSuperType == other)
        return true;
    else
        if (mSuperType)
            return mSuperType->derivesFrom(other);
        else
            return false;
}

JSValue JSType::getPropertyValue(PropertyIterator &i)
{
    Property *prop = PROPERTY(i);
    switch (prop->mFlag) {
    case ValuePointer:
        return *prop->mData.vp;
    case Constructor:
        return JSValue(mMethods[prop->mData.index]);
    default:
        return kUndefinedValue;
    }
}

bool JSType::hasProperty(const String &name, NamespaceList *names, Access acc, PropertyIterator *p)
{
    if (hasOwnProperty(name, names, acc, p))
        return true;
    else
        if (mStatics && mSuperType)
            if (mSuperType->hasProperty(name, names, acc, p))
                return true;
/*
    if (mStatics)
        return mStatics->hasProperty(name, attr, acc, p);
    else
*/
        return false;
}

Reference *JSType::genReference(const String& name, NamespaceList *names, Access acc, uint32 depth)
{
    PropertyIterator i;
    /* look in the static instance first */
    if (mStatics) {
        Reference *result = mStatics->genReference(name, names, acc, depth);
        if (result)
            return result;
    }
    if (hasProperty(name, names, acc, &i)) {
        Property *prop = PROPERTY(i);
        switch (prop->mFlag) {
        case FunctionPair:
            if (acc == Read)
                return new GetterFunctionReference(prop->mData.fPair.getterF);
            else {
                JSFunction *f = prop->mData.fPair.setterF;
                return new SetterFunctionReference(f, f->getArgType(0));
            }
        case IndexPair:
            if (mStatics == NULL)   // i.e. this is a static method
                if (acc == Read)
                    return new StaticGetterMethodReference(prop->mData.iPair.getterI, prop->mType);
                else {
                    JSFunction *f = mMethods[prop->mData.iPair.setterI];
                    return new StaticSetterMethodReference(prop->mData.iPair.setterI, f->getArgType(0));
                }
            else
                if (acc == Read)
                    return new GetterMethodReference(prop->mData.iPair.getterI, this, prop->mType);
                else {
                    JSFunction *f = mMethods[prop->mData.iPair.setterI];
                    return new SetterMethodReference(prop->mData.iPair.setterI, this, f->getArgType(0));
                }
        case Slot:
            if (mStatics == NULL)   // i.e. this is a static method
                return new StaticFieldReference(prop->mData.index, acc, mSuperType, prop->mType);
            else
                return new FieldReference(prop->mData.index, acc, prop->mType);
        case Method:
            if (mStatics == NULL)   // i.e. this is a static method
                return new StaticFunctionReference(prop->mData.index, prop->mType);
            else
                return new MethodReference(prop->mData.index, this, prop->mType);
        case Constructor:
            // the mSuperType of the static component is the actual class
            ASSERT(mStatics == NULL);
            return new ConstructorReference(prop->mData.index, mSuperType);
        case ValuePointer:
            return new PropertyReference(name, acc, prop->mType);
        default:
            NOT_REACHED("bad storage kind");
            return NULL;
        }
    }
    // walk the supertype chain
    if (mStatics && mSuperType) // test mStatics because if it's NULL (i.e. in the static instance)
                                // then the superType is a pointer back to the class.
        return mSuperType->genReference(name, names, acc, depth);
    return NULL;
}

JSType::JSType(Context *cx, const String &name, JSType *super) 
            : JSInstance(cx, Type_Type),
                    mSuperType(super), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL),
                    mClassName(name),
                    mIsDynamic(false),
                    mUninitializedValue(kNullValue),
                    mPrototypeObject(NULL)
{
    for (uint32 i = 0; i < OperatorCount; i++)
        mUnaryOperators[i] = NULL;
    mStatics = new JSType(cx, this);

    // every class gets a prototype object
    mPrototypeObject = new JSObject();
    // and that object is prototype-linked to the super-type's prototype object
    if (mSuperType)
        mPrototypeObject->mPrototype = mSuperType->mPrototypeObject;
}

JSType::JSType(Context *cx, JSType *xClass)     // used for constructing the static component type
            : JSInstance(cx, Type_Type),
                    mSuperType(xClass), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL),
                    mIsDynamic(false),
                    mUninitializedValue(kNullValue),
                    mPrototypeObject(NULL)
{
    for (uint32 i = 0; i < OperatorCount; i++)
        mUnaryOperators[i] = NULL;
}

void JSType::setSuperType(JSType *super)
{
    mSuperType = super;
    if (mSuperType)
        mPrototypeObject->mPrototype = mSuperType->mPrototypeObject;
}

















void Activation::defineTempVariable(Reference *&readRef, Reference *&writeRef, JSType *type)
{
    readRef = new LocalVarReference(mVariableCount, Read, type);
    writeRef = new LocalVarReference(mVariableCount, Write, type);
    mVariableCount++;
}

Reference *Activation::genReference(const String& name, NamespaceList *names, Access acc, uint32 depth)
{
    PropertyIterator i;
    if (hasProperty(name, names, acc, &i)) {
        Property *prop = PROPERTY(i);
        ASSERT((prop->mFlag == Slot) || (prop->mFlag == FunctionPair)); 

        if (prop->mFlag == FunctionPair) 
            return (acc == Read) ? new AccessorReference(prop->mData.fPair.getterF)
                                 : new AccessorReference(prop->mData.fPair.setterF);

        if (depth)
            return new ClosureVarReference(depth, prop->mData.index, acc, prop->mType);

        return new LocalVarReference(prop->mData.index, acc, prop->mType);
    }
    NOT_REACHED("bad genRef call");
    return NULL;
}








void Context::buildRuntimeForFunction(FunctionDefinition &f, JSFunction *fnc)
{
    fnc->mParameterBarrel = new ParameterBarrel(this);
    mScopeChain->addScope(fnc->mParameterBarrel);
    VariableBinding *v = f.parameters;
    while (v) {
        if (v->name) {
            JSType *pType = mScopeChain->extractType(v->type);
            mScopeChain->defineVariable(this, *v->name, NULL, pType);       // XXX attributes?
        }
        v = v->next;
    }
    mScopeChain->addScope(&fnc->mActivation);
    mScopeChain->collectNames(f.body);
    buildRuntimeForStmt(f.body);
    mScopeChain->popScope();
    mScopeChain->popScope();
}

void Context::setAttributeValue(AttributeStmtNode *s)
{
    JSInstance *attributeValue = executeAttributes(s->attributes);
    
    attributeValue->getProperty(this, widenCString("trueFlags"), (NamespaceList *)(NULL));
    JSValue flags = popValue();
    ASSERT(flags.isNumber());
    s->attributeFlags = (uint32)(flags.f64);
    s->attributeValue = attributeValue;
}


// Second pass, collect type information and finish 
// off the definitions made in pass 1
void Context::buildRuntimeForStmt(StmtNode *p)
{
    switch (p->getKind()) {
    case StmtNode::block:
        {
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                buildRuntimeForStmt(s);
                s = s->next;
            }            
        }
        break;
    case StmtNode::Try:
        {
            TryStmtNode *t = static_cast<TryStmtNode *>(p);
            if (t->catches) {
                CatchClause *c = t->catches;
                while (c) {
                    if (c->type)
                        c->prop->mType = mScopeChain->extractType(c->type);
                    c = c->next;
                }
            }
        }
        break;
    case StmtNode::If:
    case StmtNode::With:
    case StmtNode::DoWhile:
    case StmtNode::While:
        {
            UnaryStmtNode *u = static_cast<UnaryStmtNode *>(p);
            buildRuntimeForStmt(u->stmt);
        }
        break;
    case StmtNode::IfElse:
        {
            BinaryStmtNode *b = static_cast<BinaryStmtNode *>(p);
            buildRuntimeForStmt(b->stmt);
            buildRuntimeForStmt(b->stmt2);
        }
        break;
    case StmtNode::For:
    case StmtNode::ForIn:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            if (f->initializer) buildRuntimeForStmt(f->initializer);
        }
        break;
    case StmtNode::Var:
    case StmtNode::Const:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
//            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                JSType *type = mScopeChain->extractType(v->type);
                v->prop->mType = type;
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
//            bool isStatic = hasAttribute(f->attributes, Token::Static);
//            bool isConstructor = hasAttribute(f->attributes, ConstructorKeyWord);
//            bool isOperator = hasAttribute(f->attributes, OperatorKeyWord);
            
            bool isOperator = (f->attributeFlags & Property::Operator) == Property::Operator;

            JSType *resultType = mScopeChain->extractType(f->function.resultType);
            JSFunction *fnc = f->mFunction;
            fnc->setResultType(resultType);
            
            fnc->setExpectedArgs(getParameterCount(f->function));
            VariableBinding *v = f->function.parameters;
            uint32 index = 0;
            while (v) {
                fnc->setArgType(index++, mScopeChain->extractType(v->type));
                v = v->next;
            }

            if (isOperator) {
                if (f->function.prefix != FunctionName::op) {
                    NOT_REACHED("***** Implement me -- signal an error here because the user entered an unquoted operator name");
                }
                ASSERT(f->function.name);
                const StringAtom& name = *f->function.name;
                Operator op = getOperator(getParameterCount(f->function), name);
                // if it's a unary operator, it just gets added 
                // as a method with a special name. Binary operators
                // get added to the Context's operator table.
                if (getParameterCount(f->function) == 1)
                    mScopeChain->defineUnaryOperator(op, fnc);
                else
                    defineOperator(op, getParameterType(f->function, 0), 
                                   getParameterType(f->function, 1), fnc);
            }

            // if it's an extending function, rediscover the extended class
            // and push the class scope onto the scope chain


/*
            bool isExtender = false;                
            if (hasAttribute(f->attributes, ExtendKeyWord)) {
                JSType *extendedClass = mScopeChain->extractType( <extend attribute argument> );
                mScopeChain->addScope(extendedClass->mStatics);
                mScopeChain->addScope(extendedClass);
            }
*/            

            buildRuntimeForFunction(f->function, fnc);
/*
            if (isExtender) {   // blow off the extended class's scope
                mScopeChain->popScope();
                mScopeChain->popScope();
            }
*/


        }
        break;
    case StmtNode::Class:
        {     
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            JSType *superClass = Object_Type;
            if (classStmt->superclass) {
                ASSERT(classStmt->superclass->getKind() == ExprNode::identifier);   // XXX
                IdentifierExprNode *superClassExpr = static_cast<IdentifierExprNode*>(classStmt->superclass);
                superClass = mScopeChain->findType(superClassExpr->name, superClassExpr->pos);
            }
            JSType *thisClass = classStmt->mType;
            thisClass->setSuperType(superClass);
            mScopeChain->addScope(thisClass->mStatics);
            mScopeChain->addScope(thisClass);
            if (classStmt->body) {
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    mScopeChain->collectNames(s);
                    s = s->next;
                }
                s = classStmt->body->statements;
                while (s) {
                    buildRuntimeForStmt(s);
                    s = s->next;
                }
            }
            thisClass->completeClass(this, mScopeChain, superClass);

            mScopeChain->popScope();
            mScopeChain->popScope();
        }        
        break;
    case StmtNode::Namespace:
        {
            // do anything ?
        }
        break;
    default:
        break;
    }

}


static JSValue Object_Constructor(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    JSValue v = thisValue;
    if (v.isNull())
        v = Object_Type->newInstance(cx);
    ASSERT(v.isObject());
    return v;
}

static JSValue Object_toString(Context *, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    if (thisValue.isObject())
        return JSValue(new String(widenCString("[object ") + widenCString("Object") + widenCString("]")));
    else
        if (thisValue.isType())
            return JSValue(new String(widenCString("[object ") + widenCString("Type") + widenCString("]")));
        else {
            NOT_REACHED("Object.prototype.toString on non-object");
            return kUndefinedValue;
        }
}

static JSValue Function_Constructor(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    JSValue v = thisValue;
    if (v.isNull())
        v = Function_Type->newInstance(cx);
    // XXX use the arguments to compile a string into a function
    ASSERT(v.isObject());
    return v;
}

static JSValue Function_toString(Context *, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isFunction());
    return JSValue(new String(widenCString("function () { }")));
}

static JSValue Function_hasInstance(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(argc == 1);
    JSValue v = argv[0];
    if (!v.isObject())
        return kFalseValue;

    ASSERT(thisValue.isFunction());
    thisValue.function->getProperty(cx, widenCString("prototype"), CURRENT_ATTR);
    JSValue p = cx->popValue();

    if (!p.isObject())
        cx->reportError(Exception::typeError, "HasInstance: Function has non-object prototype");

    JSObject *V = v.object->mPrototype;
    while (V) {
        if (V == p.object)
            return kTrueValue;
        V = V->mPrototype;
    }
    return kFalseValue;
}

static JSValue Number_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue v = thisValue;
    if (v.isNull())
        v = Number_Type->newInstance(cx);
    ASSERT(v.isObject());
    JSObject *thisObj = v.object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(new double(argv[0].toNumber(cx).f64));
    else
        thisObj->mPrivate = (void *)(new double(0.0));
    return v;
}

static JSValue Number_toString(Context *, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    return JSValue(numberToString(*((double *)(thisObj->mPrivate))));
}



static JSValue Integer_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue v = thisValue;
    if (v.isNull())
        v = Integer_Type->newInstance(cx);
    ASSERT(v.isObject());
    JSObject *thisObj = v.object;
    if (argc > 0) {
        float64 d = argv[0].toNumber(cx).f64;
        bool neg = (d < 0);
        d = fd::floor(neg ? -d : d);
        d = neg ? -d : d;
        thisObj->mPrivate = (void *)(new double(d));
    }
    else
        thisObj->mPrivate = (void *)(new double(0.0));
    return v;
}

static JSValue Integer_toString(Context *, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    return JSValue(numberToString(*((double *)(thisObj->mPrivate))));
}



static JSValue Boolean_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue v = thisValue;
    if (v.isNull())
        v = Boolean_Type->newInstance(cx);
    ASSERT(v.isObject());
    JSObject *thisObj = v.object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(argv[0].toBoolean(cx).boolean);
    else
        thisObj->mPrivate = (void *)(false);
    return v;
}

static JSValue Boolean_toString(Context *, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    if (thisObj->mPrivate != 0)
        return JSValue(new String(widenCString("true")));
    else
        return JSValue(new String(widenCString("false")));
}

static JSValue ExtendAttribute_Invoke(Context *cx, const JSValue& /*thisValue*/, JSValue *argv, uint32 argc)
{
    ASSERT(argc == 1);

    JSInstance *i = Attribute_Type->newInstance(cx);
    i->setProperty(cx, widenCString("trueFlags"), (NamespaceList *)(NULL), JSValue((float64)(Property::Extend)) );
    i->setProperty(cx, widenCString("falseFlags"), (NamespaceList *)(NULL), JSValue((float64)(Property::Extend | Property::Virtual)) );
    i->setProperty(cx, widenCString("names"), (NamespaceList *)(NULL), JSValue(Array_Type->newInstance(cx)));
    i->setProperty(cx, widenCString("extendArg"), (NamespaceList *)(NULL), argv[0]);

    return JSValue(i);
}
              




// Initialize a built-in class - setting the functions into the prototype object
void Context::initClass(JSType *type, JSType *super, ClassDef *cdef, PrototypeFunctions *pdef)
{
    mScopeChain->addScope(type);
    type->setDefaultConstructor(this, new JSFunction(this, cdef->defCon, Object_Type));

    // the prototype functions are defined in the prototype object...
    if (pdef) {
        for (uint32 i = 0; i < pdef->mCount; i++) {
            JSFunction *fun = new JSFunction(this, pdef->mDef[i].imp, pdef->mDef[i].result);
            fun->setExpectedArgs(pdef->mDef[i].length);
            type->mPrototypeObject->defineVariable(this, widenCString(pdef->mDef[i].name), 
                                               (NamespaceList *)(NULL), 
                                               pdef->mDef[i].result, 
                                               JSValue(fun));
        }
    }
    type->completeClass(this, mScopeChain, super);
    type->setStaticInitializer(this, NULL);
    type->mUninitializedValue = *cdef->uninit;
    getGlobalObject()->defineVariable(this, widenCString(cdef->name), (NamespaceList *)(NULL), Type_Type, JSValue(type));
    mScopeChain->popScope();
    if (pdef) delete pdef;
}



void Context::initBuiltins()
{
    ClassDef builtInClasses[] =
    {
        { "Object",     Object_Constructor,    &kNullValue    },
        { "Type",       NULL,                  &kNullValue    },
        { "Function",   Function_Constructor,  &kNullValue    },
        { "Number",     Number_Constructor,    &kPositiveZero },
        { "Integer",    Integer_Constructor,   &kPositiveZero },
        { "String",     String_Constructor,    &kNullValue    },
        { "Array",      Array_Constructor,     &kNullValue    },
        { "Boolean",    Boolean_Constructor,   &kFalseValue   },
        { "Void",       NULL,                  &kNullValue    },
        { "Unit",       NULL,                  &kNullValue    },
        { "Attribute",  NULL,                  &kNullValue    },
    };

    Object_Type  = new JSType(this, widenCString(builtInClasses[0].name), NULL);
    Object_Type->mIsDynamic = true;
    // XXX aren't all the built-ins thus?

    Type_Type      = new JSType(this, widenCString(builtInClasses[1].name), Object_Type);
    Function_Type  = new JSType(this, widenCString(builtInClasses[2].name), Object_Type);
    Number_Type    = new JSType(this, widenCString(builtInClasses[3].name), Object_Type);
    Integer_Type   = new JSType(this, widenCString(builtInClasses[4].name), Object_Type);
    String_Type    = new JSStringType(this, widenCString(builtInClasses[5].name), Object_Type);
    Array_Type     = new JSArrayType(this, widenCString(builtInClasses[6].name), Object_Type);
    Boolean_Type   = new JSType(this, widenCString(builtInClasses[7].name), Object_Type);
    Void_Type      = new JSType(this, widenCString(builtInClasses[8].name), Object_Type);
    Unit_Type      = new JSType(this, widenCString(builtInClasses[9].name), Object_Type);
    Attribute_Type = new JSType(this, widenCString(builtInClasses[10].name), Object_Type);


    String_Type->defineVariable(this, widenCString("fromCharCode"), NULL, String_Type, JSValue(new JSFunction(this, String_fromCharCode, String_Type)));


    ProtoFunDef objectProtos[] = 
    {
        { "toString", String_Type, 0, Object_toString },
        { "toSource", String_Type, 0, Object_toString },
        { NULL }
    };
    ProtoFunDef functionProtos[] = 
    {
        { "toString",    String_Type, 0, Function_toString },
        { "toSource",    String_Type, 0, Function_toString },
        { "hasInstance", Boolean_Type, 1, Function_hasInstance },
        { NULL }
    };
    ProtoFunDef numberProtos[] = 
    {
        { "toString", String_Type, 0, Number_toString },
        { "toSource", String_Type, 0, Number_toString },
        { NULL }
    };
    ProtoFunDef integerProtos[] = 
    {
        { "toString", String_Type, 0, Number_toString },
        { "toSource", String_Type, 0, Number_toString },
        { NULL }
    };
    ProtoFunDef booleanProtos[] = 
    {
        { "toString", String_Type, 0, Boolean_toString },
        { "toSource", String_Type, 0, Boolean_toString },
        { NULL }
    };

    ASSERT(mGlobal);
    *mGlobal = Object_Type->newInstance(this);
    initClass(Object_Type,  NULL,         &builtInClasses[0], new PrototypeFunctions(&objectProtos[0]) );
    
    // pull up them bootstraps 
    (*mGlobal)->mPrototype = Object_Type->mPrototype;

    initClass(Type_Type,        Object_Type,  &builtInClasses[1],  NULL );
    initClass(Function_Type,    Object_Type,  &builtInClasses[2],  new PrototypeFunctions(&functionProtos[0]) );
    initClass(Number_Type,      Object_Type,  &builtInClasses[3],  new PrototypeFunctions(&numberProtos[0]) );
    initClass(Integer_Type,     Object_Type,  &builtInClasses[4],  new PrototypeFunctions(&integerProtos[0]) );
    initClass(String_Type,      Object_Type,  &builtInClasses[5],  getStringProtos() );
    initClass(Array_Type,       Object_Type,  &builtInClasses[6],  getArrayProtos() );
    initClass(Boolean_Type,     Object_Type,  &builtInClasses[7],  new PrototypeFunctions(&booleanProtos[0]) );
    initClass(Void_Type,        Object_Type,  &builtInClasses[8],  NULL);
    initClass(Unit_Type,        Object_Type,  &builtInClasses[9],  NULL);
    initClass(Attribute_Type,   Object_Type,  &builtInClasses[10], NULL);
}

void Context::initAttributeValue(char *name, uint32 trueFlags, uint32 falseFlags)
{
    JSInstance *i = Attribute_Type->newInstance(this);
    i->setProperty(this, widenCString("trueFlags"), (NamespaceList *)(NULL), JSValue((float64)trueFlags));
    i->setProperty(this, widenCString("falseFlags"), (NamespaceList *)(NULL), JSValue((float64)falseFlags));
    i->setProperty(this, widenCString("names"), (NamespaceList *)(NULL), JSValue(Array_Type->newInstance(this)));
    getGlobalObject()->defineVariable(this, widenCString(name), (NamespaceList *)(NULL), Attribute_Type, JSValue(i));
}

Context::Context(JSObject **global, World &world, Arena &a, Pragma::Flags flags) 
    : VirtualKeyWord(world.identifiers["virtual"]),
      ConstructorKeyWord(world.identifiers["constructor"]),
      OperatorKeyWord(world.identifiers["operator"]),
      FixedKeyWord(world.identifiers["fixed"]),
      DynamicKeyWord(world.identifiers["dynamic"]),
      ExtendKeyWord(world.identifiers["extend"]),
      PrototypeKeyWord(world.identifiers["prototype"]),

      mWorld(world),
      mScopeChain(NULL),
      mArena(a),
      mFlags(flags),
      mDebugFlag(false),
      mCurModule(NULL),
      mPC(NULL),
      mThis(kNullValue),
      mStack(NULL),
      mStackTop(0),
      mStackMax(0),
      mLocals(NULL),
      mArgumentBase(NULL),
      mGlobal(global), 
      mReader(NULL)

{
    mScopeChain = new ScopeChain(this, mWorld);
    if (Object_Type == NULL) {                
        initBuiltins();
        JSObject *mathObj = Object_Type->newInstance(this);
        getGlobalObject()->defineVariable(this, widenCString("Math"), (NamespaceList *)(NULL), Object_Type, JSValue(mathObj));
        initMathObject(this, mathObj);    
        getGlobalObject()->defineVariable(this, widenCString("undefined"), (NamespaceList *)(NULL), Void_Type, kUndefinedValue);
        getGlobalObject()->defineVariable(this, widenCString("NaN"), (NamespaceList *)(NULL), Void_Type, kNaNValue);
        getGlobalObject()->defineVariable(this, widenCString("Infinity"), (NamespaceList *)(NULL), Void_Type, kPositiveInfinity);                
    }
    initOperators();
    
    struct Attribute_Init {
        char *name;
        uint32 trueFlags;
        uint32 falseFlags;
    } attribute_init[] = 
    {
        { "virtual",        Property::Virtual,          Property::Static | Property::Constructor },
        { "constructor",    Property::Constructor,      Property::Virtual },
        { "operator",       Property::Operator,         Property::Virtual | Property::Constructor },
        { "dynamic",        Property::Dynamic,          0 },
        { "fixed",          0,                          Property::Dynamic },
        { "prototype",      Property::Prototype,        0 },
        { "static",         Property::Static,           Property::Virtual },
    };
    
    for (uint32 i = 0; i < (sizeof(attribute_init) / sizeof(Attribute_Init)); i++)     
        initAttributeValue(attribute_init[i].name, attribute_init[i].trueFlags, attribute_init[i].falseFlags);

    JSFunction *x = new JSFunction(this, ExtendAttribute_Invoke, Attribute_Type);
    getGlobalObject()->defineVariable(this, widenCString("extend"), (NamespaceList *)(NULL), Attribute_Type, JSValue(x));

    NullAttribute = Attribute_Type->newInstance(this);
    NullAttribute->setProperty(this, widenCString("trueFlags"), (NamespaceList *)(NULL), kPositiveZero);
    NullAttribute->setProperty(this, widenCString("falseFlags"), (NamespaceList *)(NULL), kPositiveZero);
    NullAttribute->setProperty(this, widenCString("names"), (NamespaceList *)(NULL), JSValue(Array_Type->newInstance(this)));

}

void Context::reportError(Exception::Kind, char *message, size_t pos)
{
    if (mReader) {
        uint32 lineNum = mReader->posToLineNum(pos);
        const char16 *lineBegin;
        const char16 *lineEnd;
        size_t linePos = mReader->getLine(lineNum, lineBegin, lineEnd);
        ASSERT(lineBegin && lineEnd && linePos <= pos);

        throw Exception(Exception::semanticError, 
                            widenCString(message), 
                            mReader->sourceLocation, 
                            lineNum, pos - linePos, pos, lineBegin, lineEnd);
    }
    else {
        throw Exception(Exception::semanticError, message); 
    }
}

// assumes mPC has been set inside the interpreter loop prior 
// to dispatch to whatever routine invoked this error reporter
void Context::reportError(Exception::Kind kind, char *message)
{
    reportError(kind, message, mCurModule->getPositionForPC(toUInt32(mPC - mCurModule->mCodeBase)));
}


Formatter& operator<<(Formatter& f, const JSValue& value)
{
    switch (value.tag) {
    case JSValue::f64_tag:
        f << value.f64;
        break;
    case JSValue::object_tag:
        printFormat(f, "Object @ 0x%08X\n", value.object);
        f << *value.object;
        break;
    case JSValue::type_tag:
        printFormat(f, "Type @ 0x%08X\n", value.type);
        f << *value.type;
        break;
    case JSValue::boolean_tag:
        f << ((value.boolean) ? "true" : "false");
        break;
    case JSValue::string_tag:
        f << *value.string;
        break;
    case JSValue::undefined_tag:
        f << "undefined";
        break;
    case JSValue::null_tag:
        f << "null";
        break;
    case JSValue::function_tag:
        if (!value.function->isNative())
            f << "function\n" << *value.function->getByteCode();
        else
            f << "function\n";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    return f;
}

void JSType::printSlotsNStuff(Formatter& f) const
{
    f << "var. count = " << mVariableCount << "\n";
    f << "method count = " << (uint32)(mMethods.size()) << "\n";
    uint32 index = 0;
    for (MethodList::const_iterator i = mMethods.begin(), end = mMethods.end(); (i != end); i++) {
        f << "[#" << index++ << "]";
        if (*i == NULL)
            f << "NULL\n";
        else
            if (!(*i)->isNative()) {
                ByteCodeModule *m = (*i)->getByteCode();
                if (m)
                    f << *m;
            }
    }
    if (mStatics)
        f << "Statics :\n" << *mStatics;
}

Formatter& operator<<(Formatter& f, const JSObject& obj)
{
    obj.printProperties(f);
    return f;
}
Formatter& operator<<(Formatter& f, const JSType& obj)
{
    printFormat(f, "super @ 0x%08X\n", obj.mSuperType);
    f << "properties\n";
    obj.printProperties(f);
    f << "slotsnstuff\n";
    obj.printSlotsNStuff(f);
    f << "done with type\n";
    return f;
}
Formatter& operator<<(Formatter& f, const Access& slot)
{
    switch (slot) {
    case Read : f << "Read\n"; break;
    case Write : f << "Write\n"; break;
    }
    return f;
}
Formatter& operator<<(Formatter& f, const Property& prop)
{
    switch (prop.mFlag) {
    case ValuePointer : 
        {
            JSValue v = *prop.mData.vp;
            f << "ValuePointer --> "; 
            if (v.isObject())
                printFormat(f, "Object @ 0x%08X\n", v.object);
            else
            if (v.isType())
                printFormat(f, "Type @ 0x%08X\n", v.type);
            else
            if (v.isFunction())
                printFormat(f, "Function @ 0x%08X\n", v.function);
            else
                f << v << "\n";
        }
        break;
    case FunctionPair : f << "FunctionPair\n"; break;
    case IndexPair : f << "IndexPair\n"; break;
    case Slot : f << "Slot\n"; break;
    case Constructor : f << "Constructor\n"; break;
    case Method : f << "Method\n"; break;
    }
    return f;
}
Formatter& operator<<(Formatter& f, const JSInstance& obj)
{
    for (PropertyMap::const_iterator i = obj.mProperties.begin(), end = obj.mProperties.end(); (i != end); i++) {
        const Property *prop = PROPERTY(i);
        f << "[" << PROPERTY_NAME(i) << "] ";
        switch (prop->mFlag) {
        case ValuePointer : f << "ValuePointer --> " << *prop->mData.vp; break;
        case FunctionPair : f << "FunctionPair\n"; break;
        case IndexPair : f << "IndexPair\n"; break;
        case Slot : f << "Slot #" << prop->mData.index 
                         << " --> " << obj.mInstanceValues[prop->mData.index] << "\n"; break;
        case Method : f << "Method #" << prop->mData.index << "\n"; break;
        case Constructor : f << "Constructor #" << prop->mData.index << "\n"; break;
        }
    }
    return f;
}




}
}

