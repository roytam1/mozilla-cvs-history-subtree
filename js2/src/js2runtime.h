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

#ifndef js2runtime_h___
#define js2runtime_h___

#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#endif


#include <vector>
#include <stack>
#include <map>

#include "systemtypes.h"
#include "strings.h"
#include "formatter.h"
#include "property.h"

namespace JavaScript {
namespace JS2Runtime {

    class ByteCodeGen;
    class ByteCodeModule;


#ifdef IS_LITTLE_ENDIAN
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[1])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[0])
#else
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[0])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[1])
#endif

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff

#define JSDOUBLE_IS_NaN(x)                                                    \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) == JSDOUBLE_HI32_EXPMASK &&   \
     (JSDOUBLE_LO32(x) || (JSDOUBLE_HI32(x) & JSDOUBLE_HI32_MANTMASK)))

#define JSDOUBLE_IS_INFINITE(x)                                               \
    ((JSDOUBLE_HI32(x) & ~JSDOUBLE_HI32_SIGNBIT) == JSDOUBLE_HI32_EXPMASK &&   \
     !JSDOUBLE_LO32(x))

#define JSDOUBLE_IS_FINITE(x)                                                 \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) != JSDOUBLE_HI32_EXPMASK)

#define JSDOUBLE_IS_NEGZERO(d)  (JSDOUBLE_HI32(d) == JSDOUBLE_HI32_SIGNBIT && \
				 JSDOUBLE_LO32(d) == 0)


static const double two32minus1 = 4294967295.0;
static const double two32 = 4294967296.0;
static const double two16 = 65536.0;
static const double two31 = 2147483648.0;


    class JSObject;
    class JSFunction;
    class JSType;
    class JSArrayType;
    class JSStringType;
    class Context;


    extern JSType *Object_Type;         // the base type for all types
    extern JSType *Number_Type;
    extern JSStringType *String_Type;
    extern JSType *Type_Type;           // the type for variables that are types 
                                        // (e.g. the property 'C' from class C...
                                        // has this type).
    extern JSType *Boolean_Type;
    extern JSType *Void_Type;
    extern JSArrayType *Array_Type;
    extern JSType *Unit_Type;

    bool hasAttribute(AttributeList* identifiers, Token::Kind tokenKind, IdentifierExprNode **attrArg = NULL);
    bool hasAttribute(AttributeList* identifiers, const StringAtom &name, IdentifierExprNode **attrArg = NULL);

    String *numberToString(float64 number);
    float64 stringToNumber(const String *string);


    class JSValue {
    public:
        union {
            float64 f64;
            JSObject *object;
            JSFunction *function;
            const String *string;
            JSType *type;
            bool boolean;
        };
        
        typedef enum {
            undefined_tag, 
            f64_tag,
            object_tag,
            function_tag,
            type_tag,
            boolean_tag,
            string_tag,
            null_tag,
        } Tag;
        Tag tag;
        
        JSValue() : f64(0.0), tag(undefined_tag) {}
        explicit JSValue(float64 f64) : f64(f64), tag(f64_tag) {}
        explicit JSValue(JSObject *object) : object(object), tag(object_tag) {}
        explicit JSValue(JSFunction *function) : function(function), tag(function_tag) {}
        explicit JSValue(JSType *type) : type(type), tag(type_tag) {}
        explicit JSValue(const String *string) : string(string), tag(string_tag) {}
        explicit JSValue(bool boolean) : boolean(boolean), tag(boolean_tag) {}
        explicit JSValue(Tag tag) : tag(tag) {}

        float64& operator=(float64 f64)                 { return (tag = f64_tag, this->f64 = f64); }
        JSObject*& operator=(JSObject* object)          { return (tag = object_tag, this->object = object); }
        JSType*& operator=(JSType* type)                { return (tag = type_tag, this->type = type); }
        JSFunction*& operator=(JSFunction* slot)        { return (tag = function_tag, this->function = function); }
        bool& operator=(bool boolean)                   { return (tag = boolean_tag, this->boolean = boolean); }
        
        bool isObject() const                           { return (tag == object_tag); }
        bool isNumber() const                           { return (tag == f64_tag); }
        bool isBool() const                             { return (tag == boolean_tag); }
        bool isType() const                             { return (tag == type_tag); }
        bool isFunction() const                         { return (tag == function_tag); }
        bool isString() const                           { return (tag == string_tag); }

        bool isUndefined() const                        { return (tag == undefined_tag); }
        bool isNull() const                             { return (tag == null_tag); }
        bool isNaN() const                              { ASSERT(isNumber()); return JSDOUBLE_IS_NaN(f64); }
        bool isNegativeInfinity() const                 { ASSERT(isNumber()); return (f64 < 0) && JSDOUBLE_IS_INFINITE(f64); }
        bool isPositiveInfinity() const                 { ASSERT(isNumber()); return (f64 > 0) && JSDOUBLE_IS_INFINITE(f64); }
        bool isNegativeZero() const                     { ASSERT(isNumber()); return JSDOUBLE_IS_NEGZERO(f64); }
        bool isPositiveZero() const                     { ASSERT(isNumber()); return (f64 == 0.0) && !JSDOUBLE_IS_NEGZERO(f64); }

        bool isTrue() const                             { ASSERT(isBool()); return boolean; }
        bool isFalse() const                            { ASSERT(isBool()); return !boolean; }

        JSType *getType();

        JSValue toString(Context *cx) const            { return (isString() ? *this : valueToString(cx, *this)); }
        JSValue toNumber(Context *cx) const            { return (isNumber() ? *this : valueToNumber(cx, *this)); }
        JSValue toUInt32(Context *cx) const            { return valueToUInt32(cx, *this); }
        JSValue toUInt16(Context *cx) const            { return valueToUInt16(cx, *this); }
        JSValue toInt32(Context *cx) const             { return valueToInt32(cx, *this); }
        JSValue toObject(Context *cx) const            { return ((isObject() || isType() || isFunction()) ?
                                                                *this : valueToObject(cx, *this)); }
        JSValue toBoolean(Context *cx) const           { return (isBool() ? *this : valueToBoolean(cx, *this)); }

        /* These are for use in 'toPrimitive' calls */
        enum Hint {
            NumberHint, StringHint, NoHint
        };
        JSValue toPrimitive(Context *cx, Hint hint = NoHint) const;
        
        static JSValue valueToNumber(Context *cx, const JSValue& value);
        static JSValue valueToString(Context *cx, const JSValue& value);
        static JSValue valueToObject(Context *cx, const JSValue& value);
        static JSValue valueToUInt32(Context *cx, const JSValue& value);
        static JSValue valueToUInt16(Context *cx, const JSValue& value);
        static JSValue valueToInt32(Context *cx, const JSValue& value);
        static JSValue valueToBoolean(Context *cx, const JSValue& value);
        
        int operator==(const JSValue& value) const;

    };
    Formatter& operator<<(Formatter& f, const JSValue& value);

    extern const JSValue kUndefinedValue;
    extern const JSValue kNaNValue;
    extern const JSValue kTrueValue;
    extern const JSValue kFalseValue;
    extern const JSValue kNullValue;
    extern const JSValue kNegativeZero;
    extern const JSValue kPositiveZero;
    extern const JSValue kNegativeInfinity;
    extern const JSValue kPositiveInfinity;
    

    
    typedef enum { Read, Write } Access;
    Formatter& operator<<(Formatter& f, const Access& acc);
    
    
    typedef enum {
        None,
        Posate,
        Negate,
        Complement,
        Increment,
        Decrement,
        Const,
        Call,
        New,
        Index,
        IndexEqual,
        DeleteIndex,
        Plus,
        Minus,
        Multiply,
        Divide,
        Remainder,
        ShiftLeft,
        ShiftRight,
        UShiftRight,
        Less,
        LessEqual,
        In,
        Equal,
        SpittingImage,
        BitAnd,
        BitXor,
        BitOr,
        OperatorCount
    } Operator;
    
    

   

    
    class Reference {
    public:
        Reference(JSType *type) : mType(type) { }
        JSType *mType;

        virtual bool needsThis() { return false; }

        virtual bool isConstructor()    { return false; }   // XXX cheaper than RTTI ? (or is that on anyway)

        virtual void emitImplicitLoad(ByteCodeGen *bcg) { } 

        virtual void emitInvokeSequence(ByteCodeGen *bcg)   { emitCodeSequence(bcg); }

        virtual bool emitPreAssignment(ByteCodeGen *bcg)    { return false; }

        virtual void emitCodeSequence(ByteCodeGen *bcg) 
                { throw Exception(Exception::internalError, "gen code for base ref"); }

        virtual bool getValue(Context *cx)
                { throw Exception(Exception::internalError, "get value(cx) for base ref"); }
        
        virtual bool setValue(Context *cx)
                { throw Exception(Exception::internalError, "set value(cx) for base ref"); }

        virtual uint32 baseExpressionDepth()
                { return 0; }

        virtual void emitTypeOf(ByteCodeGen *bcg);

        virtual void emitDelete(ByteCodeGen *bcg);
    };

    // a getter/setter function from an activation
    // the function is known directly
    class AccessorReference : public Reference {
    public:
        AccessorReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx);
        bool setValue(Context *cx);
    };
    // a simple local variable reference - it's a slot
    // in the current activation
    class LocalVarReference : public Reference {
    public:
        LocalVarReference(uint32 index, Access acc, JSType *type)
            : Reference(type), mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx);
        bool setValue(Context *cx);
    };
    // a local variable 'n' activations up the
    // execution stack
    class ClosureVarReference : public LocalVarReference {
    public:
        ClosureVarReference(uint32 depth, uint32 index, Access acc, JSType *type) 
                        : LocalVarReference(index, acc, type), mDepth(depth) { }
        uint32 mDepth;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    // a member field in an instance
    class FieldReference : public Reference {
    public:
        FieldReference(uint32 index, Access acc, JSType *type) 
            : Reference(type), mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitImplicitLoad(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
    };
    // a static field
    class StaticFieldReference : public Reference {
    public:
        StaticFieldReference(uint32 index, Access acc, JSType *baseClass, JSType *type) 
            : Reference(type), mAccess(acc), mIndex(index), mClass(baseClass) { }
        Access mAccess;
        uint32 mIndex;
        JSType *mClass;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitImplicitLoad(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
    };
    // a static function
    class StaticFunctionReference : public Reference {
    public:
        StaticFunctionReference(uint32 index, JSType *type)
            : Reference(type), mIndex(index) { }
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitInvokeSequence(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
    };
    class StaticGetterMethodReference : public StaticFunctionReference {
    public:
        StaticGetterMethodReference(uint32 index, JSType *type)
            : StaticFunctionReference(index, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class StaticSetterMethodReference : public StaticFunctionReference {
    public:
        StaticSetterMethodReference(uint32 index, JSType *type)
            : StaticFunctionReference(index, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        bool emitPreAssignment(ByteCodeGen *bcg);
    };
    // a member function in a vtable
    class MethodReference : public Reference {
    public:
        MethodReference(uint32 index, JSType *baseClass, JSType *type) 
            : Reference(type), mIndex(index), mClass(baseClass) { }
        uint32 mIndex;
        JSType *mClass;
        void emitCodeSequence(ByteCodeGen *bcg);
        virtual bool needsThis() { return true; }
        virtual void emitImplicitLoad(ByteCodeGen *bcg);
        virtual uint32 baseExpressionDepth() { return 1; }
        void emitInvokeSequence(ByteCodeGen *bcg);
    };
    class GetterMethodReference : public MethodReference {
    public:
        GetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class SetterMethodReference : public MethodReference {
    public:
        SetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        bool emitPreAssignment(ByteCodeGen *bcg);
    };

    // a function
    class FunctionReference : public Reference {
    public:
        FunctionReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    // a constructor
    class ConstructorReference : public Reference {
    public:
        ConstructorReference(uint32 index, JSType *baseClass)
            : Reference(baseClass), mIndex(index) { }
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool isConstructor()    { return true; }
        bool needsThis() { return false; }          // i.e. there is no 'this' specified by the
                                                    // the call sequence (it's a static function)
        uint32 baseExpressionDepth() { return 1; }
    };
    // a getter function
    class GetterFunctionReference : public Reference {
    public:
        GetterFunctionReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    // a setter function
    class SetterFunctionReference : public Reference {
    public:
        SetterFunctionReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitImplicitLoad(ByteCodeGen *bcg);
    };
    // Either an existing value property (dynamic) or
    // the "we don't know any field by that name".
    class PropertyReference : public Reference {
    public:
        PropertyReference(const String& name, Access acc, JSType *type)
            : Reference(type), mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitInvokeSequence(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
        bool needsThis() { return true; }
        void emitImplicitLoad(ByteCodeGen *bcg);
        void emitDelete(ByteCodeGen *bcg);
    };
    // a parameter slot (they can't have getter/setter, right?)
    class ParameterReference : public Reference {
    public:
        ParameterReference(uint32 index, Access acc, JSType *type) 
            : Reference(type), mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
    };

    // the generic "we don't know anybody by that name" - not bound to a specific object
    // so it's a scope chain lookup at runtime
    class NameReference : public Reference {
    public:
        NameReference(const String& name, Access acc)
            : Reference(Object_Type), mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitTypeOf(ByteCodeGen *bcg);
        bool getValue(Context *cx)
                { throw Exception(Exception::internalError, "get value for name ref"); }
        bool setValue(Context *cx)
                { throw Exception(Exception::internalError, "set value for name ref"); }
        void emitDelete(ByteCodeGen *bcg);
    };

    class ElementReference : public Reference {
    public:
        ElementReference(Access acc, uint32 depth)
            : mAccess(acc), mDepth(depth), Reference(Object_Type) { }
        Access mAccess;
        uint32 mDepth;
        void emitCodeSequence(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return mDepth; }
    };



    
    
    
    
    
    

    
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSObject(JSType *type = Object_Type) : mType(type), mPrivate(NULL), mPrototype(NULL) { }

        
        // every object has a type
        JSType        *mType;

        // the property data is kept (or referenced from) here
        PropertyMap   mProperties;

        // Every JSObject has a private part
        void          *mPrivate;

        // Every JSObject (except the Ur-object) has a prototype
        JSObject      *mPrototype;

        JSType *getType() const { return mType; }

        virtual bool isDynamic() { return true; }

/*
        // see if the property exists in any form
        bool hasProperty(const String &name)
        {
            PropertyIterator i = mProperties.find(name);
            return (i != mProperties.end());
        }
*/
        PropertyIterator findAttributedProperty(const String &name, AttributeList *attr)
        {
            for (PropertyIterator i = mProperties.lower_bound(name), 
                            end = mProperties.upper_bound(name); (i != end); i++) {
                if (attr) {
                    AttributeList *propAttr = PROPERTY_ATTRLIST(i);
                    if (propAttr == NULL)
                        return i;
                    while (attr) {
                        if (attr->expr->getKind() == ExprNode::identifier) {
                            const StringAtom& name = (static_cast<IdentifierExprNode *>(attr->expr))->name;
                            if (hasAttribute(propAttr, name))
                                return i;
                        }
                        else
                            ASSERT(false);  // XXX NYI
                        attr = attr->next;
                    }
                }
                else
                    return i;
            }
            return mProperties.end();
        }

        void deleteProperty(const String &name, AttributeList *attr)
        {
            PropertyIterator i = findAttributedProperty(name, attr);
            mProperties.erase(i);
        }

        // see if the property exists by a specific kind of access
        bool hasOwnProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p)
        {
            *p = findAttributedProperty(name, attr);
            if (*p != mProperties.end()) {
                Property& prop = PROPERTY(*p);
                if (prop.mFlag == FunctionPair)
                    return (acc == Read) ? (prop.mData.fPair.getterF != NULL)
                                         : (prop.mData.fPair.setterF != NULL);
                else
                    if (prop.mFlag == IndexPair)
                        return (acc == Read) ? (prop.mData.iPair.getterI != -1)
                                             : (prop.mData.iPair.setterI != -1);
                    else
                        return true;
            }
            else
                return false;
        }

        virtual bool hasProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p)
        {
            if (hasOwnProperty(name, attr, acc, p))
                return true;
            else
                if (mPrototype)
                    return mPrototype->hasProperty(name, attr, acc, p);
                else
                    return false;
        }


        // get a property value
        virtual JSValue getPropertyValue(PropertyIterator &i)
        {
            Property &prop = PROPERTY(i);
            ASSERT(prop.mFlag == ValuePointer);
            return *prop.mData.vp;
        }

        virtual bool getProperty(Context *cx, const String &name, AttributeList *attr);

        virtual bool setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);



        // add a property
        virtual PropertyIterator defineVariable(const String &name, AttributeList *attr, JSType *type)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(new JSValue(), type), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }
        virtual PropertyIterator defineStaticVariable(const String &name, AttributeList *attr, JSType *type)
        {
            return defineVariable(name, attr, type);    // XXX or error?
        }
        // add a method property
        virtual void defineMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));
        }
        virtual void defineStaticMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));    // XXX or error?
        }
        virtual void defineConstructor(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));    // XXX or error?
        }
        virtual void defineStaticGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            defineGetterMethod(name, attr, type, f);    // XXX or error?
        }
        virtual void defineStaticSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            defineSetterMethod(name, attr, type, f);    // XXX or error?
        }
        virtual void defineGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, Write, &i)) {
                ASSERT(PROPERTY_KIND(i) == FunctionPair);
                ASSERT(PROPERTY_GETTERF(i) == NULL);
                PROPERTY_GETTERF(i) = f;
            }
            else {
                const PropertyMap::value_type e(name, AttributedProperty(Property(type, f, NULL), attr));
                mProperties.insert(e);
            }
        }
        virtual void defineSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, Read, &i)) {
                ASSERT(PROPERTY_KIND(i) == FunctionPair);
                ASSERT(PROPERTY_SETTERF(i) == NULL);
                PROPERTY_SETTERF(i) = f;
            }
            else {
                const PropertyMap::value_type e(name, AttributedProperty(Property(type, NULL, f), attr));
                mProperties.insert(e);
            }
        }

        // add a property (with a value)
        virtual PropertyIterator defineVariable(const String &name, AttributeList *attr, JSType *type, JSValue v)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(new JSValue(v), type), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }

        virtual Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                switch (prop.mFlag) {
                case ValuePointer:
                    return new PropertyReference(name, acc, prop.mType);
                case FunctionPair:
                    if (acc == Read)
                        return new GetterFunctionReference(prop.mData.fPair.getterF);
                    else
                        return new SetterFunctionReference(prop.mData.fPair.setterF);
                default:
                    NOT_REACHED("bad storage kind");
                    return NULL;
                }
            }
            NOT_REACHED("bad genRef call");
            return NULL;
        }

        virtual bool hasLocalVars()     { return false; }
        virtual uint32 localVarCount()  { return 0; }

        // debug only        
        void printProperties(Formatter &f) const
        {
            for (PropertyMap::const_iterator i = mProperties.begin(), end = mProperties.end(); (i != end); i++) 
            {
                f << "[" << PROPERTY_NAME(i) << "] " << PROPERTY(i);
            }
        }


    };

    Formatter& operator<<(Formatter& f, const JSObject& obj);
    

    // had to be after JSObject defn.
    inline JSType *JSValue::getType() {
        switch (tag) {
        case f64_tag: return Number_Type;
        case string_tag: return (JSType *)String_Type;
        case object_tag: return object->getType();
        case undefined_tag: return Void_Type;
        case null_tag: return Object_Type;
        default: NOT_REACHED("bad type"); return NULL;
        }
    }



    


        
    
    
    
    
    

    
 


    class JSInstance : public JSObject {
    public:
        
        JSInstance(Context *cx, JSType *type) 
            : JSObject(type), mInstanceValues(NULL) { if (type) initInstance(cx, type); }

        void initInstance(Context *cx, JSType *type);

        bool getProperty(Context *cx, const String &name, AttributeList *attr);
        bool setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);

        JSValue getField(uint32 index)
        {
            return mInstanceValues[index];
        }

        void setField(uint32 index, JSValue v)
        {
            mInstanceValues[index] = v;
        }

        virtual bool isDynamic();

        JSValue         *mInstanceValues;
    };
    Formatter& operator<<(Formatter& f, const JSInstance& obj);

    

    typedef std::vector<JSFunction *> MethodList;

    class ScopeChain;

    class JSType : public JSInstance {
    public:
        
        JSType(Context *cx, const String &name, JSType *super) 
            : JSInstance(cx, Type_Type),
                    mSuperType(super), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL),
                    mClassName(name),
                    mIsDynamic(false)
        {
            for (uint32 i = 0; i < OperatorCount; i++)
                mUnaryOperators[i] = NULL;
        }

        JSType(Context *cx, JSType *xClass)     // used for constructing the static component type
            : JSInstance(cx, Type_Type),
                    mSuperType(xClass), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL),
                    mIsDynamic(false)
        {
            for (uint32 i = 0; i < OperatorCount; i++)
                mUnaryOperators[i] = NULL;
        }

        void setStaticInitializer(Context *cx, JSFunction *f);
        void setInstanceInitializer(Context *cx, JSFunction *f);

        void createStaticComponent(Context *cx)
                                        // builds the JSType that will hold the static
                                        // info (map etc) for this class
        {
            mStatics = new JSType(cx, this);
            // set the prototype object
            mPrototype = new JSObject();
            if (mSuperType) mPrototype->mPrototype = mSuperType->mPrototype;
            defineStaticVariable(widenCString("prototype"), NULL, Object_Type);
        }

        // construct a new (empty) instance of this class
        virtual JSInstance *newInstance(Context *cx);
    
        
        // static helpers

        void defineConstructor(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            uint32 vTableIndex = mStatics->mMethods.size();
            mStatics->mMethods.push_back(f);

            const PropertyMap::value_type e(name, AttributedProperty(Property(vTableIndex, type, Constructor), attr));
            mStatics->mProperties.insert(e);
        }

        void defineStaticMethod(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineMethod(name, attr, type, f);
        }

        PropertyIterator defineStaticVariable(const String& name, AttributeList *attr, JSType *type, JSValue v)
        {
            ASSERT(mStatics);
            return mStatics->defineVariable(name, attr, type, v);
        }

        PropertyIterator defineStaticVariable(const String& name, AttributeList *attr, JSType *type)
        {
            ASSERT(mStatics);
            return mStatics->defineVariable(name, attr, type);
        }

        bool hasStatic(const String& name, AttributeList *attr, Access acc)
        {
            ASSERT(mStatics);
            PropertyIterator i;
            return mStatics->hasProperty(name, attr, acc, &i);
        }
        
        void defineStaticGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineGetterMethod(name, attr, type, f);
        }
        
        void defineStaticSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineSetterMethod(name, attr, type, f);
        }

        //


        PropertyIterator defineVariable(const String& name, AttributeList *attr, JSType *type)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(mVariableCount++, type, Slot), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }

        PropertyIterator defineVariable(const String& name, AttributeList *attr, JSType *type, JSValue v)
        {   // XXX why doesn't the virtual function in JSObject get found?
            return JSObject::defineVariable(name, attr, type, v);
        }

        void defineMethod(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(f);

            const PropertyMap::value_type e(name, AttributedProperty(Property(vTableIndex, type, Method), attr));
            mProperties.insert(e);
        }
        
        void defineGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            PropertyIterator i;
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(f);

            if (hasProperty(name, attr, Write, &i)) {
                ASSERT(PROPERTY_KIND(i) == IndexPair);
                ASSERT(PROPERTY_GETTERI(i) == 0);
                PROPERTY_GETTERI(i) = vTableIndex;
            }
            else {
                const PropertyMap::value_type e(name, AttributedProperty(Property(vTableIndex, 0, type), attr));
                mProperties.insert(e);
            }
        }

        void defineSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            PropertyIterator i;
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(f);

            if (hasProperty(name, attr, Read, &i)) {
                ASSERT(PROPERTY_KIND(i) == IndexPair);
                ASSERT(PROPERTY_SETTERI(i) == 0);
                PROPERTY_SETTERI(i) = vTableIndex;
            }
            else {
                const PropertyMap::value_type e(name, AttributedProperty(Property(0, vTableIndex, type), attr));
                mProperties.insert(e);
            }
        }

        void defineUnaryOperator(Operator which, JSFunction *f)
        {
            mUnaryOperators[which] = f;
        }

        JSFunction *getUnaryOperator(Operator which)
        {
            return mUnaryOperators[which];
        }

        void setDefaultConstructor(JSFunction *f)
        {
            defineConstructor(mClassName, NULL, Object_Type, f);    // XXX attr?
        }

        void addMethod(const String &name, AttributeList *attr, JSFunction *f);
        void addStaticMethod(const String &name, AttributeList *attr, JSFunction *f);

        // return true if 'other' is on the chain of supertypes
        bool derivesFrom(JSType *other)
        {
            if (mSuperType == other)
                return true;
            else
                if (mSuperType)
                    return mSuperType->derivesFrom(other);
                else
                    return false;
        }

        virtual JSValue getPropertyValue(PropertyIterator &i)
        {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case ValuePointer:
                return *prop.mData.vp;
            case Constructor:
                return JSValue(mMethods[prop.mData.index]);
            default:
                return kUndefinedValue;
            }
        }

        virtual bool hasProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p)
        {
            if (hasOwnProperty(name, attr, acc, p))
                return true;
            else
                if (mStatics && mSuperType)
                    return mSuperType->hasProperty(name, attr, acc, p);
                else
                    return false;
        }

        virtual Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            /* look in the static instance first 
            if (mStatics) {
                Reference *result = mStatics->genReference(name, attr, acc, depth);
                if (result)
                    return result;
            }*/

            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                switch (prop.mFlag) {
                case FunctionPair:
                    if (acc == Read)
                        return new GetterFunctionReference(prop.mData.fPair.getterF);
                    else
                        return new SetterFunctionReference(prop.mData.fPair.setterF);
                case IndexPair:
                    if (mStatics == NULL)   // i.e. this is a static method
                        if (acc == Read)
                            return new StaticGetterMethodReference(prop.mData.iPair.getterI, prop.mType);
                        else
                            return new StaticSetterMethodReference(prop.mData.iPair.setterI, prop.mType);
                    else
                        if (acc == Read)
                            return new GetterMethodReference(prop.mData.iPair.getterI, this, prop.mType);
                        else
                            return new SetterMethodReference(prop.mData.iPair.setterI, this, prop.mType);
                case Slot:
                    if (mStatics == NULL)   // i.e. this is a static method
                        return new StaticFieldReference(prop.mData.index, acc, mSuperType, prop.mType);
                    else
                        return new FieldReference(prop.mData.index, acc, prop.mType);
                case Method:
                    if (mStatics == NULL)   // i.e. this is a static method
                        return new StaticFunctionReference(prop.mData.index, prop.mType);
                    else
                        return new MethodReference(prop.mData.index, this, prop.mType);
                case Constructor:
                    // the mSuperType of the static component is the actual class
                    ASSERT(mStatics == NULL);
                    return new ConstructorReference(prop.mData.index, mSuperType);
                case ValuePointer:
                    return new PropertyReference(name, acc, prop.mType);
                default:
                    NOT_REACHED("bad storage kind");
                    return NULL;
                }
            }
            // walk the supertype chain
            if (mStatics && mSuperType) // test mStatics because if it's NULL (i.e. in the static instance)
                                        // then the superType is a pointer back to the class.
                return mSuperType->genReference(name, attr, acc, depth);
            return NULL;
        }

        // constructor functions are added as static methods
        // XXX is it worth just having a default constructor 
        // pointer in the class?
        JSFunction *getDefaultConstructor()
        {
            ASSERT(mStatics);
            for (PropertyIterator i = mStatics->mProperties.lower_bound(mClassName), 
                            end = mStatics->mProperties.upper_bound(mClassName); (i != end); i++) {
                if (PROPERTY_KIND(i) == Constructor)
                    return mStatics->mMethods[PROPERTY_INDEX(i)];
            }
            return NULL;
        }

        // assumes that the super types have been completed already
        void completeClass(Context *cx, ScopeChain *scopeChain, JSType *super);

        virtual bool isDynamic() { return mIsDynamic; }

        JSType          *mSuperType;        // NULL implies that this is the base Object

        uint32          mVariableCount;
        JSInstance      *mInitialInstance;
        JSType          *mStatics;          // NULL implies that this is the static component
    
        // the 'vtable'
        MethodList      mMethods;
        String          mClassName;

        JSFunction      *mUnaryOperators[OperatorCount];    // XXX too wasteful

        bool            mIsDynamic;

        void printSlotsNStuff(Formatter& f) const;

    };

    Formatter& operator<<(Formatter& f, const JSType& obj);

    class JSArrayInstance : public JSInstance {
    public:
        JSArrayInstance(Context *cx, JSType *type) : JSInstance(cx, NULL), mLength(0) { mType = (JSType *)Array_Type; }

        // XXX maybe could have implemented length as a getter/setter pair?
        bool setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);
        bool getProperty(Context *cx, const String &name, AttributeList *attr);


        uint32 mLength;

    };

    class JSArrayType : public JSType {
    public:
        JSArrayType(Context *cx, const String &name, JSType *super) 
            : JSType(cx, name, super)
        {
        }

        JSInstance *newInstance(Context *cx);

    };

    class JSStringInstance : public JSInstance {
    public:
        JSStringInstance(Context *cx, JSType *type) : JSInstance(cx, NULL), mLength(0) { mType = (JSType *)String_Type; }

        bool getProperty(Context *cx, const String &name, AttributeList *attr);


        uint32 mLength;

    };

    class JSStringType : public JSType {
    public:
        JSStringType(Context *cx, const String &name, JSType *super) 
            : JSType(cx, name, super)
        {
        }

        JSInstance *newInstance(Context *cx);

    };





    // captures the Parameter names scope
    class ParameterBarrel : public JSType {
    public:

        ParameterBarrel(Context *cx) : JSType(cx, NULL) 
        {
        }

        Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                ASSERT(prop.mFlag == Slot);
                return new ParameterReference(prop.mData.index, acc, prop.mType);
            }
            NOT_REACHED("bad genRef call");
            return NULL;
        }

    };








    // an Activation has two jobs:
    // 1. At compile time it represents the function/method being compiled and collects
    //      the local vars/consts being defined in that function. 
    // 2. At runtime it is the container for the values of those local vars
    //      (although it's only constructed as such when the function 
    //          either calls another function - so the activation represents
    //          the saved state, or when a closure object is constructed)

    class Activation : public JSType {
    public:

        Activation(Context *cx) 
                    : JSType(cx, NULL), 
                        mLocals(NULL), 
                        mStack(NULL),
                        mStackTop(0),
                        mPC(0), 
                        mModule(NULL), 
                        mArgCount(0) { }

        Activation(Context *cx, JSValue *locals, 
                        JSValue *stack, uint32 stackTop,
                        ScopeChain *scopeChain,
                        uint32 argBase, JSValue curThis,
                        uint8 *pc, 
                        ByteCodeModule *module, 
                        uint32 argCount)
                    : JSType(cx, NULL), 
                        mLocals(locals), 
                        mStack(stack), 
                        mStackTop(stackTop),
                        mScopeChain(scopeChain),
                        mArgumentBase(argBase), 
                        mThis(curThis), 
                        mPC(pc), 
                        mModule(module), 
                        mArgCount(argCount) { }


        
        // saved values from a previous execution
        JSValue *mLocals;
        JSValue *mStack;
        uint32 mStackTop;
        ScopeChain *mScopeChain;
        uint32 mArgumentBase;
        JSValue mThis;
        uint8 *mPC;
        ByteCodeModule *mModule;
        uint32 mArgCount;


        bool hasLocalVars()             { return true; }
        virtual uint32 localVarCount()  { return mVariableCount; }

        void defineTempVariable(Reference *&readRef, Reference *&writeRef, JSType *type)
        {
            readRef = new LocalVarReference(mVariableCount, Read, type);
            writeRef = new LocalVarReference(mVariableCount, Write, type);
            mVariableCount++;
        }

        Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                ASSERT((prop.mFlag == Slot) || (prop.mFlag == FunctionPair)); 

                if (prop.mFlag == FunctionPair) 
                    return (acc == Read) ? new AccessorReference(prop.mData.fPair.getterF)
                                         : new AccessorReference(prop.mData.fPair.setterF);

                if (depth)
                    return new ClosureVarReference(depth, prop.mData.index, acc, prop.mType);

                return new LocalVarReference(prop.mData.index, acc, prop.mType);
            }
            NOT_REACHED("bad genRef call");
            return NULL;
        }

    };


    
    
    class ScopeChain {
    public:

        ScopeChain(Context *cx, World &world) :
              m_cx(cx)
        {
        }

        Context *m_cx;

        std::vector<JSObject *> mScopeStack;
        typedef std::vector<JSObject *>::reverse_iterator ScopeScanner;


        void addScope(JSObject *s)
        {
            mScopeStack.push_back(s);
        }

        void popScope()
        {
            ASSERT(mScopeStack.size());
            mScopeStack.pop_back();
        }

        // add a new name to the current scope
        PropertyIterator defineVariable(const String& name, AttributeList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type);
        }
        PropertyIterator defineVariable(const String& name, AttributeList *attr, JSType *type, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type, v);
        }
        PropertyIterator defineStaticVariable(const String& name, AttributeList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            return top->defineStaticVariable(name, attr, type);
        }
        void defineMethod(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineMethod(name, attr, type, f);
        }   
        void defineStaticMethod(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticMethod(name, attr, type, f);
        }   
        void defineConstructor(const String& name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineConstructor(name, attr, type, f);
        }   
        void defineGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineGetterMethod(name, attr, type, f);
        }
        void defineSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineSetterMethod(name, attr, type, f);
        }
        void defineStaticGetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticGetterMethod(name, attr, type, f);
        }
        void defineStaticSetterMethod(const String &name, AttributeList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticSetterMethod(name, attr, type, f);
        }
        void defineUnaryOperator(Operator which, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            ((JSType *)top)->defineUnaryOperator(which, f);
        }

        // see if the current scope contains a name already
        bool hasProperty(const String& name, AttributeList *attr, Access acc, PropertyIterator *p)
        {
            JSObject *top = mScopeStack.back();
            return top->hasProperty(name, attr, acc, p);
        }

        // generate a reference to the given name
        Reference *getName(const String& name, AttributeList *attr, Access acc)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                PropertyIterator i;
                if ((*s)->hasProperty(name, attr, acc, &i))
                    return (*s)->genReference(name, attr, acc, depth);
                else
                    if ((*s)->isDynamic())
                        return NULL;

            }
            return NULL;
        }

        bool hasNameValue(const String& name, AttributeList *attr)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                PropertyIterator i;
                if ((*s)->hasProperty(name, attr, Read, &i))
                    return true;
            }
            return false;
        }

        bool getNameValue(const String& name, AttributeList *attr, Context *cx);

        // return the class on the top of the stack (or NULL if there
        // isn't one there).
        // XXX would it be better to have addScopeClass() and track when the
        // top scope is a class rather than require RTTI just for this.
        JSType *topClass()
        {
            return dynamic_cast<JSType *>(mScopeStack.back());
        }

        void defineTempVariable(Reference *&readRef, Reference *&writeRef, JSType *type)
        {
            ASSERT(dynamic_cast<Activation *>(mScopeStack.back()));
            ((Activation *)(mScopeStack.back()))->defineTempVariable(readRef, writeRef, type);
        }

        // a compile time request to get the value for a name
        // (i.e. we're accessing a constant value)
        JSValue getCompileTimeValue(const String& name, AttributeList *attr)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                PropertyIterator i;
                if ((*s)->hasProperty(name, attr, Read, &i)) 
                    return (*s)->getPropertyValue(i);
            }
            return kUndefinedValue;
        }



        bool setNameValue(const String& name, AttributeList *attr, Context *cx);

        // return the number of local vars used by all the 
        // Activations on the top of the chain
        uint32 countVars()
        {
            uint32 result = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++)
            {
                if ((*s)->hasLocalVars())
                    result += (*s)->localVarCount();
                else
                    break;
            }
            return result;
        }

        uint32 countActivations()
        {
            uint32 result = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++)
            {
                if ((*s)->hasLocalVars())
                    result++;
            }
            return result;
        }

        void collectNames(StmtNode *p);

        // Lookup a name as a type in the chain
        JSType *findType(const StringAtom& typeName);

        // Get a type from an ExprNode 
        JSType *extractType(ExprNode *t);


    };


    class JSFunction : public JSObject {
    protected:
        JSFunction() : mActivation(NULL) { }        // for JSBoundFunction (XXX ask Patrick about this structure)
    public:
        typedef JSValue (NativeCode)(Context *cx, const JSValue &thisValue, JSValue argv[], uint32 argc);

        JSFunction(Context *cx, JSType *resultType,
                         uint32 argCount, ScopeChain *scopeChain,
                         bool isConstructor = false) 

                    : mByteCode(NULL), 
                        mCode(NULL), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mExpectedArgs(argCount),
                        mScopeChain(NULL)
        {
            if (scopeChain) {
                mScopeChain = new ScopeChain(*scopeChain);
            }
            mPrototype = Object_Type->mPrototype;
        }
        
        JSFunction(Context *cx, NativeCode *code, JSType *resultType) 
                    : mByteCode(NULL), 
                        mCode(code), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mExpectedArgs(0),
                        mScopeChain(NULL)
        {
            mPrototype = Object_Type->mPrototype;
        }

        void setByteCode(ByteCodeModule *b)     { ASSERT(!isNative()); mByteCode = b; }
        void setResultType(JSType *r)           { mResultType = r; }
        void setExpectedArgs(uint32 e)          { mExpectedArgs = e; }

        virtual bool hasBoundThis()             { return false; }
        virtual bool isNative()                 { return (mCode != NULL); }
        virtual ByteCodeModule *getByteCode()   { ASSERT(!isNative()); return mByteCode; }
        virtual NativeCode *getNativeCode()     { ASSERT(isNative()); return mCode; }

        virtual JSType *getResultType()         { return mResultType; }
        virtual uint32 getExpectedArgs()        { return mExpectedArgs; }
        virtual ScopeChain *getScopeChain()     { return mScopeChain; }
        virtual JSValue getThisValue()          { return kNullValue; }         

        ParameterBarrel *mParameterBarrel;
        Activation mActivation;                 // not used during execution   
    private:
        ByteCodeModule *mByteCode;
        NativeCode *mCode;
        JSType *mResultType;
        uint32 mExpectedArgs;
        ScopeChain *mScopeChain;
    };

    class JSBoundFunction : public JSFunction {
    private:
        JSFunction *mFunction;
        JSObject *mThis;
    public:
        JSBoundFunction(JSFunction *f, JSObject *thisObj)
            : mFunction(f), mThis(thisObj) { }

        bool hasBoundThis()             { return true; }
        bool isNative()                 { return mFunction->isNative(); }
        ByteCodeModule *getByteCode()   { return mFunction->getByteCode(); }
        NativeCode *getNativeCode()     { return mFunction->getNativeCode(); }

        JSType *getResultType()         { return mFunction->getResultType(); }
        uint32 getExpectedArgs()        { return mFunction->getExpectedArgs(); }
        ScopeChain *getScopeChain()     { return mFunction->getScopeChain(); }
        JSValue getThisValue()          { return JSValue(mThis); }         
    };

    // This is for binary operators, it collects together the operand
    // types and the function pointer for the given operand. See also
    // Context::initOperators where the default operators are set up.
    class OperatorDefinition {
    public:
        
        OperatorDefinition(JSType *type1, JSType *type2, JSFunction *imp)
            : mType1(type1), mType2(type2), mImp(imp) { ASSERT(mType1); ASSERT(mType2); }


        JSType *mType1;
        JSType *mType2;
        JSFunction *mImp;

        // see if this operator is applicable when 
        // being invoked by the given types
        bool isApplicable(JSType *tx, JSType *ty)
        {
            return ( ((tx == mType1) || tx->derivesFrom(mType1))
                        && 
                      ((ty == mType2) || ty->derivesFrom(mType2)) );
        }


    };


    
    class Context {
    public:
        struct ProtoFunDef {
            char *name;
            JSType *result;
            uint32 length;
            JSFunction::NativeCode *imp;
        };
        class PrototypeFunctions {
        public:
            PrototypeFunctions(ProtoFunDef *p)
            {
                uint32 count = 0;
                mDef = NULL;
                if (p) {
                    while (p[count].name) count++;
                    mDef = new ProtoFunDef[count];
                    for (uint32 i = 0; i < count; i++)
                        mDef[i] = *p++;
                }
                mCount = count;
            }
            ~PrototypeFunctions()
            {
                if (mDef) delete mDef;
            }
            ProtoFunDef *mDef;
            uint32 mCount;
        };

        struct ClassDef {
            char *name;
            JSFunction::NativeCode *defCon;
        };

        Context(JSObject **global, World &world, Arena &a);

        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 
        StringAtom& FixedKeyWord;
        StringAtom& DynamicKeyWord;
        StringAtom& ExtendKeyWord;

        void initBuiltins();
        void initClass(JSType *type, JSType *super, ClassDef *cdef, PrototypeFunctions *pdef);
        void initOperators();
        
        void defineOperator(Operator which, JSType *t1, JSType *t2, JSFunction *imp)
        {
            OperatorDefinition *op = new OperatorDefinition(t1, t2, imp);
            mOperatorTable[which].push_back(op);
        }

        JSFunction *getOperator(Operator which, JSType *t1, JSType *t2)
        {
            for (OperatorList::iterator oi = mOperatorTable[which].begin(),
                        end = mOperatorTable[which].end();
                            (oi != end); oi++) 
                if ( ((*oi)->mType1 == t1) && ((*oi)->mType2 == t2) )
                    return (*oi)->mImp;

            NOT_REACHED("operator gone missing");
            return NULL;
        }

        bool executeOperator(Operator op, JSType *t1, JSType *t2);

        void switchToFunction(JSFunction *f)
        {
            ASSERT(false);  // somebody should write some code here one day
        }

        JSObject *getGlobalObject()         { ASSERT(mGlobal); return *mGlobal; }

        World &mWorld;
        ScopeChain *mScopeChain;
        Arena &mArena;
        bool mDebugFlag;

        // the currently executing 'function'
        ByteCodeModule *mCurModule;

        uint8 *mPC;

        JSValue mThis;

        // this is the execution stack (for the current function)
        JSValue *mStack;
        uint32 mStackTop;
        uint32 mStackMax;

        void pushValue(JSValue v)
        {
            ASSERT(mStackTop < mStackMax);
            mStack[mStackTop++] = v;
        }

        JSValue popValue()
        {
            ASSERT(mStackTop > 0);
            return mStack[--mStackTop];
        }

        JSValue topValue()
        {
            return mStack[mStackTop - 1];
        }

        void resizeStack(uint32 n)
        {
            ASSERT(n < mStackMax);
            mStackTop = n;
        }

        uint32 stackSize()
        {
            return mStackTop;
        }

        JSValue getValue(uint32 n)
        {
            ASSERT(n < mStackTop);
            return mStack[n];
        }
/*
        void setValue(uint32 n, JSValue v)
        {
            ASSERT(n < mStackTop);
            mStack[n] = v;
        }
*/
        JSValue *getBase(uint32 n)
        {
            ASSERT(n <= mStackTop);     // s'ok to point beyond the end
            return &mStack[n];
        }

        // the activation stack
        std::stack<Activation *> mActivationStack;

        struct HandlerData {
            HandlerData(uint8 *pc, uint32 stackSize, Activation *curAct) 
                : mPC(pc), mStackSize(stackSize), mActivation(curAct) { }

            uint8 *mPC;
            uint32 mStackSize;
            Activation *mActivation;
        };

        std::stack<HandlerData *> mTryStack;
        std::stack<uint8 *> mSubStack;

        // the locals for the current function (an array, constructed on fudnction entry)
        JSValue *mLocals;

        // the base of the incoming arguments for this function
        // (it's an index into the execution stack)
        uint32 mArgumentBase;

        typedef std::vector<OperatorDefinition *> OperatorList;
            
        OperatorList mOperatorTable[OperatorCount];
        

        JSValue readEvalFile(const String& fileName);

        void buildRuntime(StmtNode *p);
        void buildRuntimeForFunction(FunctionDefinition &f, JSFunction *fnc);
        void buildRuntimeForStmt(StmtNode *p);
        ByteCodeModule *genCode(StmtNode *p, String sourceName);

        JSValue interpret(ByteCodeModule *bcm, const JSValue& thisValue, JSValue *argv, uint32 argc);
        JSValue interpret(uint8 *pc, uint8 *endPC);
        
        
        /* utility routines */

        // Extract the operator from the string literal function name
        // - requires the paramter count in order to distinguish
        // between unary and binary operators.
        Operator getOperator(uint32 parameterCount, const String &name);

        // Get the type of the nth parameter.
        JSType *getParameterType(FunctionDefinition &function, int index);

        // Get the number of parameters.
        uint32 getParameterCount(FunctionDefinition &function);

    private:
        JSObject **mGlobal;


    };

    class ContextStackReplacement {
    public:
        enum { ReplacementStackSize = 4 };
        ContextStackReplacement(Context *cx) 
        { 
            m_cx = cx;
            mOldStack = cx->mStack; 
            mOldStackTop = cx->mStackTop; 
            mOldStackMax = cx->mStackMax; 
            cx->mStack = &mStack[0];
            cx->mStackTop = 0;
            cx->mStackMax = ReplacementStackSize;
        }

        ~ContextStackReplacement()
        {
            m_cx->mStack = mOldStack;
            m_cx->mStackTop = mOldStackTop;
            m_cx->mStackMax = mOldStackMax;
        }

        JSValue mStack[ReplacementStackSize];

        Context *m_cx;

        JSValue *mOldStack;
        uint32 mOldStackTop;
        uint32 mOldStackMax;

    };

    inline AccessorReference::AccessorReference(JSFunction *f)
        : Reference(f->getResultType()), mFunction(f) 
    {
    }

    inline bool AccessorReference::getValue(Context *cx) 
    { 
        NOT_REACHED("need this?");
        cx->switchToFunction(mFunction);
        return true; 
    }
    
    inline bool AccessorReference::setValue(Context *cx) 
    { 
        NOT_REACHED("need this?");
        cx->switchToFunction(mFunction);
        return true; 
    }
    
    inline bool LocalVarReference::getValue(Context *cx)
    {
        NOT_REACHED("need this?");
        cx->pushValue(cx->mLocals[mIndex]);
        return false;
    }

    inline bool LocalVarReference::setValue(Context *cx)
    {
        NOT_REACHED("need this?");
        cx->mLocals[mIndex] = cx->topValue();
        return false;
    }
  
    inline bool JSObject::getProperty(Context *cx, const String &name, AttributeList *attr)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Read, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case ValuePointer:
                cx->pushValue(*prop.mData.vp);
                return false;
            case FunctionPair:
                cx->switchToFunction(prop.mData.fPair.getterF);
                return true;
            case Constructor:
            case Method:
                cx->pushValue(JSValue(mType->mMethods[prop.mData.index]));
                return false;
            default:
                ASSERT(false);  // XXX more to implement
                return false;
            }
        }
        else
            NOT_REACHED("mismatch between hasProp and getProp");        
        return false;
    }

    inline bool JSInstance::getProperty(Context *cx, const String &name, AttributeList *attr)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Read, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case Slot:
                cx->pushValue(mInstanceValues[prop.mData.index]);
                return false;
            case ValuePointer:
                cx->pushValue(*prop.mData.vp);
                return false;
            case FunctionPair:
                cx->switchToFunction(prop.mData.fPair.getterF);
                return true;
            case Constructor:
            case Method:
                cx->pushValue(JSValue(mType->mMethods[prop.mData.index]));
                return false;
            default:
                ASSERT(false);  // XXX more to implement
                return false;
            }
        }
        else {
            cx->pushValue(kUndefinedValue);
            // XXX prototype chain walking?
        }
        return false;
    }

    inline bool JSObject::setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Write, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case ValuePointer:
                *prop.mData.vp = v;
                return false;
            case FunctionPair:
                cx->pushValue(v);
                cx->switchToFunction(prop.mData.fPair.setterF);
                return true;
            default:
                ASSERT(false);  // XXX more to implement ?
                return false;
            }
        }
        else {
            defineVariable(name, attr, Object_Type, v);
            return false;
        }
    }

    inline bool JSInstance::setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Write, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case Slot:
                mInstanceValues[prop.mData.index] = v;
                return false;
            case ValuePointer:
                *prop.mData.vp = v;
                return false;
            case FunctionPair:
                cx->pushValue(v);
                cx->switchToFunction(prop.mData.fPair.setterF);
                return true;
            default:
                ASSERT(false);  // XXX more to implement ?
                return false;
            }
        }
        else {
            defineVariable(name, attr, Object_Type, v);
            return false;
        }
    }

    inline bool JSArrayInstance::getProperty(Context *cx, const String &name, AttributeList *attr)
    {
        if (name.compare(widenCString("length")) == 0) {
            cx->pushValue(JSValue((float64)mLength));
            return true;
        }
        else
            return JSInstance::getProperty(cx, name, attr);
    }

    inline bool JSArrayInstance::setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v)
    {
        if (name.compare(widenCString("length")) == 0) {
            uint32 newLength = (uint32)(v.toUInt32(cx).f64);
            if (newLength != v.toNumber(cx).f64)
                throw Exception(Exception::rangeError, "out of range value for length"); 
            
            for (uint32 i = newLength; i < mLength; i++) {
                String *id = numberToString(i);
                if (findAttributedProperty(*id, NULL) != mProperties.end())
                    deleteProperty(*id, NULL);
                delete id;
            }

            mLength = newLength;
        }
        else {
            if (findAttributedProperty(name, attr) == mProperties.end())
                defineVariable(name, attr, Object_Type, v);
            else
                JSInstance::setProperty(cx, name, attr, v);
            JSValue v = JSValue(&name);
            JSValue v_int = v.toUInt32(cx);
            if ((v_int.f64 != two32minus1) && (v_int.toString(cx).string->compare(name) == 0)) {
                if (v_int.f64 >= mLength)
                    mLength = (uint32)v_int.f64 + 1;
            }
        }
        return false;
    }

    inline bool JSStringInstance::getProperty(Context *cx, const String &name, AttributeList *attr)
    {
        if (name.compare(widenCString("length")) == 0) {
            cx->pushValue(JSValue((float64)mLength));
            return true;
        }
        else
            return JSInstance::getProperty(cx, name, attr);
    }


    inline FunctionReference::FunctionReference(JSFunction *f) 
            : Reference(f->getResultType()), mFunction(f)
    {
    }

    inline GetterFunctionReference::GetterFunctionReference(JSFunction *f) 
            : Reference(f->getResultType()), mFunction(f)
    {
    }

    inline SetterFunctionReference::SetterFunctionReference(JSFunction *f) 
            : Reference(f->getResultType()), mFunction(f)
    {
    }

    inline void JSType::addStaticMethod(const String &name, AttributeList *attr, JSFunction *f)
    {
        defineStaticMethod(name, attr, f->getResultType(), f);
    }

    inline void JSType::addMethod(const String &name, AttributeList *attr, JSFunction *f)
    {
        defineMethod(name, attr, f->getResultType(), f);
    }

    inline void JSInstance::initInstance(Context *cx, JSType *type)
    {
        if (type->mVariableCount)
            mInstanceValues = new JSValue[type->mVariableCount];

        // Don't do this when 'this' and 'type' are the same, that's
        // the static instance being initialized to itself
        if (this != type) {
            // copy the instance variable names into the property map
            for (PropertyIterator pi = type->mProperties.begin(), 
                        end = type->mProperties.end();
                        (pi != end); pi++) {            
                const PropertyMap::value_type e(PROPERTY_NAME(pi), ATTR_PROPERTY(pi));
                mProperties.insert(e);
            }

            // and then do the same for the super types
            JSType *t = type->mSuperType;
            while (t) {
                for (PropertyIterator i = t->mProperties.begin(), 
                            end = t->mProperties.end();
                            (i != end); i++) {            
                    const PropertyMap::value_type e(PROPERTY_NAME(i), ATTR_PROPERTY(i));
                    mProperties.insert(e);            
                }
                t = t->mSuperType;
            }

            // copy instance values from the Ur-instance object
            if (type->mInitialInstance)
                for (int i = 0; i < type->mVariableCount; i++)
                    mInstanceValues[i] = type->mInitialInstance->mInstanceValues[i];
        }
        else {  // for the static instance...
            // hook up the prototype object
            PropertyIterator i;
            type->hasProperty(widenCString("prototype"), NULL, Read, &i);
            ASSERT(PROPERTY_KIND(i) == Slot);
            mInstanceValues[PROPERTY_INDEX(i)] = JSValue(type->mSuperType->mPrototype);
        }
        mType = type;
    }

    inline JSInstance *JSType::newInstance(Context *cx)
    {
        JSInstance *result = new JSInstance(cx, this);
        result->mPrototype = mPrototype;
        return result;
    }

    inline void JSType::setInstanceInitializer(Context *cx, JSFunction *f)
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
            cx->interpret(f->getByteCode(), thisValue, NULL, 0);
        }
    }

    inline void JSType::setStaticInitializer(Context *cx, JSFunction *f)
    {
        ASSERT(mStatics);
        mStatics->initInstance(cx, mStatics);       // build the static instance object
        if (f) {
            JSValue thisValue(mStatics);
            cx->interpret(f->getByteCode(), thisValue, NULL, 0);
        }
    }

    inline JSInstance *JSArrayType::newInstance(Context *cx)
    {
        JSInstance *result = new JSArrayInstance(cx, this);
        result->mPrototype = mPrototype;
        return result;
    }

    inline JSInstance *JSStringType::newInstance(Context *cx)
    {
        JSInstance *result = new JSStringInstance(cx, this);
        result->mPrototype = mPrototype;
        return result;
    }

    inline bool ScopeChain::setNameValue(const String& name, AttributeList *attr, Context *cx)
    {
        PropertyIterator i;
        JSObject *top = *mScopeStack.rbegin();
        Reference *ref = NULL;
        if (top->hasProperty(name, attr, Write, &i)) {
            ref = top->genReference(name, attr, Write, 0);
            bool result = ref->setValue(cx);
            delete ref;
            return result;
        }
        else {
            JSValue v = cx->topValue();
            top->defineVariable(name, attr, Object_Type, v);
            return false;
        }
    }

    inline bool ScopeChain::getNameValue(const String& name, AttributeList *attr, Context *cx)
    {
        uint32 depth = 0;
        for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
        {
            PropertyIterator i;
            if ((*s)->hasProperty(name, attr, Read, &i)) {
                
                if (PROPERTY_KIND(i) == ValuePointer) {
                    cx->pushValue(*PROPERTY_VALUEPOINTER(i));
                    return false;
                }
                else
                    ASSERT(false);      // what else needs to be implemented ?
            }
        }
        throw Exception(Exception::referenceError, "Not defined");
        return false;
    }

    inline bool JSInstance::isDynamic() { return mType->isDynamic(); }

}
}

#endif //js2runtime_h___
