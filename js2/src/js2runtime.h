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
    namespace ByteCode {
        class ByteCodeGen;
        class ByteCodeModule;
    }
namespace JS2Runtime {

    using namespace ByteCode;


#define CURRENT_ATTR    (NULL)

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
static const double two31 = 2147483648.0;


    class JSObject;
    class JSFunction;
    class JSType;
    class JSArrayType;
    class Context;


    extern JSType *Object_Type;         // the base type for all types
    extern JSType *Number_Type;
    extern JSType *String_Type;
    extern JSType *Type_Type;           // the type for variables that are types 
                                        // (e.g. the property 'C' from class C...
                                        // has this type).
    extern JSType *Boolean_Type;
    extern JSType *Void_Type;
    extern JSArrayType *Array_Type;

    bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind);
    bool hasAttribute(const IdentifierList* identifiers, const StringAtom &name);

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

        JSValue toString(Context *cx)             { return (isString() ? *this : valueToString(cx, *this)); }
        JSValue toNumber(Context *cx)             { return (isNumber() ? *this : valueToNumber(cx, *this)); }
        JSValue toUInt32(Context *cx)             { return valueToUInt32(cx, *this); }
        JSValue toInt32(Context *cx)              { return valueToInt32(cx, *this); }
        JSValue toObject(Context *cx)             { return ((isObject() || isType() || isFunction()) ?
                                                                *this : valueToObject(cx, *this)); }
        JSValue toBoolean(Context *cx)            { return (isBool() ? *this : valueToBoolean(cx, *this)); }

        /* These are for use in 'toPrimitive' calls */
        enum Hint {
            NumberHint, StringHint, NoHint
        };
        JSValue toPrimitive(Context *cx, Hint hint = NoHint);
        
        static JSValue valueToNumber(Context *cx, JSValue& value);
        static JSValue valueToString(Context *cx, JSValue& value);
        static JSValue valueToObject(Context *cx, JSValue& value);
        static JSValue valueToUInt32(Context *cx, JSValue& value);
        static JSValue valueToInt32(Context *cx, JSValue& value);
        static JSValue valueToBoolean(Context *cx, JSValue& value);
        
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
        NewArgs,
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
    
    

    class Context;

    
    
    
    class Reference {
    public:
        Reference(JSType *type) : mType(type) { }
        JSType *mType;

        virtual bool needsThis() { return false; }

        virtual bool isConstructor()    { return false; }   // XXX cheaper than RTTI ? (or is that on anyway)

        virtual void emitImplicitLoad(ByteCodeGen *bcg) { } 

        virtual void emitInvokeSequence(ByteCodeGen *bcg)   { emitCodeSequence(bcg); }

        virtual void emitCodeSequence(ByteCodeGen *bcg) 
                { throw Exception(Exception::internalError, "gen code for base ref"); }

        virtual bool getValue(Context *cx)
                { throw Exception(Exception::internalError, "get value(cx) for base ref"); }
        virtual bool setValue(Context *cx)
                { throw Exception(Exception::internalError, "set value(cx) for base ref"); }

        virtual uint32 baseExpressionDepth()
                { return 0; }

        virtual void emitTypeOf(ByteCodeGen *bcg);
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
        uint32 baseExpressionDepth() { return 1; }
    };
    // a member function in a vtable
    class MethodReference : public Reference {
    public:
        MethodReference(uint32 index, JSType *baseClass, JSType *type) 
            : Reference(type), mIndex(index), mClass(baseClass) { }
        uint32 mIndex;
        JSType *mClass;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool needsThis() { return true; }
//        JSValue getValue();
        uint32 baseExpressionDepth() { return 1; }
        void emitInvokeSequence(ByteCodeGen *bcg);
    };
    class GetterMethodReference : public MethodReference {
    public:
        GetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        bool needsThis() { return true; }
        uint32 baseExpressionDepth() { return 1; }
    };
    class SetterMethodReference : public MethodReference {
    public:
        SetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        bool needsThis() { return true; }
        void emitImplicitLoad(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
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
        ConstructorReference(JSFunction *f, JSType *baseClass)
            : Reference(baseClass), mFunction(f) { }
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool isConstructor()    { return true; }
        bool needsThis() { return true; }
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
    // the "we don't know any field by that name", either it'll be a dynamic property
    // or we just didn't have enough type info at compile time.
    class PropertyReference : public Reference {
    public:
        PropertyReference(const String& name, Access acc)
            : Reference(Object_Type), mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitInvokeSequence(ByteCodeGen *bcg);
        uint32 baseExpressionDepth() { return 1; }
        bool needsThis() { return true; }
        void emitImplicitLoad(ByteCodeGen *bcg);
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
/*
        // see if the property exists in any form
        bool hasProperty(const String &name)
        {
            PropertyIterator i = mProperties.find(name);
            return (i != mProperties.end());
        }
*/
        PropertyIterator findAttributedProperty(const String &name, IdentifierList *attr)
        {
            for (PropertyIterator i = mProperties.lower_bound(name), 
                            end = mProperties.upper_bound(name); (i != end); i++) {
                if (attr) {
                    IdentifierList *propAttr = PROPERTY_ATTRLIST(i);
                    if (propAttr == NULL)
                        return i;
                    while (attr) {
                        if (hasAttribute(propAttr, attr->name))
                            return i;
                        attr = attr->next;
                    }
                }
                else
                    return i;
            }
            return mProperties.end();
        }

        void deleteProperty(const String &name, IdentifierList *attr)
        {
            PropertyIterator i = findAttributedProperty(name, attr);
            mProperties.erase(i);
        }

        // see if the property exists by a specific kind of access
        bool hasOwnProperty(const String &name, IdentifierList *attr, Access acc, PropertyIterator *p)
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

        bool hasProperty(const String &name, IdentifierList *attr, Access acc, PropertyIterator *p)
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

        virtual bool getProperty(Context *cx, const String &name, IdentifierList *attr);

        virtual bool setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v);



        // add a property
        virtual PropertyIterator defineVariable(const String &name, IdentifierList *attr, JSType *type)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(new JSValue(), type), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }
        virtual PropertyIterator defineStaticVariable(const String &name, IdentifierList *attr, JSType *type)
        {
            return defineVariable(name, attr, type);    // XXX or error?
        }
        // add a method property
        virtual void defineMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));
        }
        virtual void defineStaticMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));    // XXX or error?
        }
        virtual void defineConstructor(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            defineVariable(name, attr, type, JSValue(f));    // XXX or error?
        }
        virtual void defineGetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
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
        virtual void defineSetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
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
        virtual PropertyIterator defineVariable(const String &name, IdentifierList *attr, JSType *type, JSValue v)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(new JSValue(v), type), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }

        virtual Reference *genReference(const String& name, IdentifierList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                switch (prop.mFlag) {
                case ValuePointer:
                    return new PropertyReference(name, acc);
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
        case string_tag: return String_Type;
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

        bool getProperty(Context *cx, const String &name, IdentifierList *attr);
        bool setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v);

        JSValue getField(uint32 index)
        {
            return mInstanceValues[index];
        }

        void setField(uint32 index, JSValue v)
        {
            mInstanceValues[index] = v;
        }

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
                    mClassName(name)
        {
        }

        JSType(Context *cx, JSType *super) 
            : JSInstance(cx, Type_Type),
                    mSuperType(super), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL)
        {
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
            defineVariable(widenCString("prototype"), NULL, Object_Type, JSValue(mPrototype));
        }

        // construct a new (empty) instance of this class
        virtual JSInstance *newInstance(Context *cx);
    
        
        // static helpers

        void defineConstructor(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            uint32 vTableIndex = mStatics->mMethods.size();
            mStatics->mMethods.push_back(f);

            const PropertyMap::value_type e(name, AttributedProperty(Property(vTableIndex, type, Constructor), attr));
            mStatics->mProperties.insert(e);
        }

        void defineStaticMethod(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineMethod(name, attr, type, f);
        }

        PropertyIterator defineStaticVariable(const String& name, IdentifierList *attr, JSType *type)
        {
            ASSERT(mStatics);
            return mStatics->defineVariable(name, attr, type);
        }

        bool hasStatic(const String& name, IdentifierList *attr, Access acc)
        {
            ASSERT(mStatics);
            PropertyIterator i;
            return mStatics->hasProperty(name, attr, acc, &i);
        }

        //


        PropertyIterator defineVariable(const String& name, IdentifierList *attr, JSType *type)
        {
            const PropertyMap::value_type e(name, AttributedProperty(Property(mVariableCount++, type, Slot), attr));
            PropertyIterator r = mProperties.insert(e);
            return r;
        }

        PropertyIterator defineVariable(const String& name, IdentifierList *attr, JSType *type, JSValue v)
        {   // XXX why doesn't the virtual function in JSObject get found?
            return JSObject::defineVariable(name, attr, type, v);
        }

        void defineMethod(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(f);

            const PropertyMap::value_type e(name, AttributedProperty(Property(vTableIndex, type, Method), attr));
            mProperties.insert(e);
        }
        
        void defineGetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
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

        void defineSetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
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

        void addMethod(const String &name, IdentifierList *attr, JSFunction *f);
        void addStaticMethod(const String &name, IdentifierList *attr, JSFunction *f);

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

        virtual Reference *genReference(const String& name, IdentifierList *attr, Access acc, uint32 depth)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property &prop = PROPERTY(i);
                switch (prop.mFlag) {
                case FunctionPair:
                    if (acc == Read)
                        return new GetterFunctionReference(prop.mData.fPair.getterF);
                    else
                        return new SetterFunctionReference(prop.mData.fPair.setterF);
                case IndexPair:
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
                    return new ConstructorReference(mMethods[prop.mData.index], mSuperType);
                case ValuePointer:
                    return new PropertyReference(name, acc);
                default:
                    NOT_REACHED("bad storage kind");
                    return NULL;
                }
            }
            // walk the supertype chain
            if (mSuperType)
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


        JSType          *mSuperType;        // NULL implies that this is the base Object

        uint32          mVariableCount;
        JSInstance      *mInitialInstance;
        JSType          *mStatics;          // NULL implies that this is the static component
    
        // the 'vtable'
        MethodList      mMethods;
        String          mClassName;

        JSFunction      *mUnaryOperators[OperatorCount];    // XXX too wasteful

        void printSlotsNStuff(Formatter& f) const;

    };

    Formatter& operator<<(Formatter& f, const JSType& obj);

    class JSArrayInstance : public JSInstance {
    public:
        JSArrayInstance(Context *cx, JSType *type) : JSInstance(cx, NULL), mLength(0) { }

        // XXX maybe could have implemented length as a getter/setter pair?
        bool setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v);
        bool getProperty(Context *cx, const String &name, IdentifierList *attr);


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





    // captures the Parameter names scope
    class ParameterBarrel : public JSType {
    public:

        ParameterBarrel(Context *cx) : JSType(cx, NULL) 
        {
        }

        Reference *genReference(const String& name, IdentifierList *attr, Access acc, uint32 depth)
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
                        mPC(0), 
                        mModule(NULL), 
                        mArgCount(0) { }

        Activation(Context *cx, JSValue *locals, 
                        uint32 argBase, JSValue curThis,
                        uint8 *pc, 
                        ByteCodeModule *module, 
                        uint32 argCount);

        
        // saved values from a previous execution
        JSValue *mLocals;
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

        Reference *genReference(const String& name, IdentifierList *attr, Access acc, uint32 depth)
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


//    class ParameterBarrel;

    class JSFunction : public JSObject {
    public:
        typedef JSValue (NativeCode)(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);

        JSFunction(Context *cx, JSType *resultType, uint32 argCount) 
                    : mByteCode(NULL), 
                        mCode(NULL), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mExpectedArgs(argCount)

        {
        }
        
        JSFunction(Context *cx, NativeCode *code, JSType *resultType) 
                    : mByteCode(NULL), 
                        mCode(code), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mExpectedArgs(0)
        {
        }

        bool isNative() { return (mCode != NULL); }

        ByteCodeModule *mByteCode;
        NativeCode *mCode;
        JSType *mResultType;
        ParameterBarrel *mParameterBarrel;
        Activation mActivation;
        uint32 mExpectedArgs;

        JSValue getThisValue() { return kNullValue; } 
        
    
    };

    
    
    class ScopeChain {
    public:

        ScopeChain(Context *cx, World &world) :
              VirtualKeyWord(world.identifiers["virtual"]),
              ConstructorKeyWord(world.identifiers["constructor"]),
              OperatorKeyWord(world.identifiers["operator"]),
              m_cx(cx)
        {
        }

        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 
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
        PropertyIterator defineVariable(const String& name, IdentifierList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type);
        }
        PropertyIterator defineVariable(const String& name, IdentifierList *attr, JSType *type, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type, v);
        }
        PropertyIterator defineStaticVariable(const String& name, IdentifierList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            return top->defineStaticVariable(name, attr, type);
        }
        void defineMethod(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineMethod(name, attr, type, f);
        }   
        void defineStaticMethod(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticMethod(name, attr, type, f);
        }   
        void defineConstructor(const String& name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineConstructor(name, attr, type, f);
        }   
        void defineGetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineGetterMethod(name, attr, type, f);
        }
        void defineSetterMethod(const String &name, IdentifierList *attr, JSType *type, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineSetterMethod(name, attr, type, f);
        }
        void defineUnaryOperator(Operator which, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            ((JSType *)top)->defineUnaryOperator(which, f);
        }

        // see if the current scope contains a name already
        bool hasProperty(const String& name, IdentifierList *attr, Access acc, PropertyIterator *p)
        {
            JSObject *top = mScopeStack.back();
            return top->hasProperty(name, attr, acc, p);
        }

        // generate a reference to the given name
        Reference *getName(const String& name, IdentifierList *attr, Access acc)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                PropertyIterator i;
                if ((*s)->hasProperty(name, attr, acc, &i))
                    return (*s)->genReference(name, attr, acc, depth);
            }
            return NULL;
        }

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
        JSValue getCompileTimeValue(const String& name, IdentifierList *attr)
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


        // a runtime request to get the value for a name
        // it'll either return a value or cause the
        // interpreter to switch execution to a getter function
        bool getNameValue(const String& name, IdentifierList *attr, Context *cx)
        {
            Reference *ref = getName(name, attr, Read);
            if (ref == NULL)
                throw Exception(Exception::referenceError, name);
            bool result = ref->getValue(cx);
            delete ref;
            return result;
        }

        bool hasNameValue(const String& name, IdentifierList *attr)
        {
            Reference *ref = getName(name, attr, Read);
            bool result = (ref != NULL);
            delete ref;
            return result;
        }

        bool setNameValue(const String& name, IdentifierList *attr, Context *cx);

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


        void collectNames(StmtNode *p);

        // Lookup a name as a type in the chain
        JSType *findType(const StringAtom& typeName);

        // Get a type from an ExprNode 
        JSType *extractType(ExprNode *t);


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

    JSValue Object_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue Object_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);

    JSValue Number_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue Number_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);

    JSValue String_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue String_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue String_split(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue String_length(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);

    JSValue Array_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue Array_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue Array_push(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);
    JSValue Array_pop(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc);

    
    class Context {
    public:

        Context(JSObject *global, World &world) 
            : mGlobal(global), 
              mWorld(world),
              mScopeChain(NULL),
              mDebugFlag(false)
        {
            mScopeChain = new ScopeChain(this, mWorld);
            if (Object_Type == NULL) {
                Object_Type = new JSType(this, widenCString("Object"), NULL);
                mScopeChain->addScope(Object_Type);
                Object_Type->createStaticComponent(this);
                Object_Type->setDefaultConstructor(new JSFunction(this, &Object_Constructor, Object_Type));
                Object_Type->addMethod(widenCString("toString"), NULL, new JSFunction(this, &Object_toString, String_Type));
                Object_Type->completeClass(this, mScopeChain, Object_Type);
                Object_Type->setStaticInitializer(this, NULL);
                global->defineVariable(widenCString("Object"), NULL, Type_Type, JSValue(Object_Type));
                mScopeChain->popScope();

                global->defineVariable(widenCString("undefined"), NULL, Void_Type, kUndefinedValue);
                global->defineVariable(widenCString("NaN"), NULL, Void_Type, kNaNValue);
                global->defineVariable(widenCString("Infinity"), NULL, Void_Type, kPositiveInfinity);

                
                Number_Type = new JSType(this, widenCString("Number"), Object_Type);
                mScopeChain->addScope(Number_Type);
                Number_Type->createStaticComponent(this);
                Number_Type->setDefaultConstructor(new JSFunction(this, &Number_Constructor, Object_Type));
                Number_Type->addMethod(widenCString("toString"), NULL, new JSFunction(this, &Number_toString, String_Type));
                Number_Type->completeClass(this, mScopeChain, Object_Type);
                Number_Type->setStaticInitializer(this, NULL);
                global->defineVariable(widenCString("Number"), NULL, Type_Type, JSValue(Number_Type));
                mScopeChain->popScope();

                Void_Type = new JSType(this, NULL);
                global->defineVariable(widenCString("Void"), NULL, Type_Type, JSValue(Void_Type));


                String_Type = new JSType(this, widenCString("String"), Object_Type);
                mScopeChain->addScope(Number_Type);
                String_Type->createStaticComponent(this);
                String_Type->setDefaultConstructor(new JSFunction(this, &String_Constructor, Object_Type));
                String_Type->addMethod(widenCString("toString"), NULL, new JSFunction(this, &String_toString, String_Type));
                String_Type->mPrototype->defineVariable(widenCString("split"), NULL, Array_Type, JSValue(new JSFunction(this, &String_split, Array_Type)));
                String_Type->addMethod(widenCString("length"), NULL, new JSFunction(this, &String_length, Number_Type));
                String_Type->completeClass(this, mScopeChain, Object_Type);
                String_Type->setStaticInitializer(this, NULL);
                global->defineVariable(widenCString("String"), NULL, Type_Type, JSValue(Number_Type));
                mScopeChain->popScope();

                Array_Type = new JSArrayType(this, widenCString("Array"), Object_Type);
                mScopeChain->addScope(Number_Type);
                Array_Type->createStaticComponent(this);
                Array_Type->setDefaultConstructor(new JSFunction(this, &Array_Constructor, Object_Type));
                Array_Type->addMethod(widenCString("toString"), NULL, new JSFunction(this, &Array_toString, String_Type));
                Array_Type->mPrototype->defineVariable(widenCString("push"), NULL, Number_Type, JSValue(new JSFunction(this, &Array_push, Number_Type)));
                Array_Type->mPrototype->defineVariable(widenCString("pop"), NULL, Object_Type, JSValue(new JSFunction(this, &Array_pop, Object_Type)));
                Array_Type->completeClass(this, mScopeChain, Object_Type);
                Array_Type->setStaticInitializer(this, NULL);
                global->defineVariable(widenCString("Array"), NULL, Type_Type, JSValue(Array_Type));
                mScopeChain->popScope();

                Boolean_Type = new JSType(this, Object_Type);
                Type_Type = new JSType(this, Object_Type);        
            }
            initOperators();
        }

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

        JSObject *mGlobal;
        World &mWorld;
        ScopeChain *mScopeChain;
        bool mDebugFlag;

        // the currently executing 'function'
        ByteCodeModule *mCurModule;

        uint8 *mPC;

        JSValue mThis;

        // this is the execution stack (for the current function at least)
        std::vector<JSValue> mStack;

        // the activation stack
        std::stack<Activation *> mActivationStack;

        // the locals for the current function (an array, constructed on function entry)
        JSValue *mLocals;

        // the base of the incoming arguments for this function
        // (it's an index into the execution stack)
        uint32 mArgumentBase;

        typedef std::vector<OperatorDefinition *> OperatorList;
            
        OperatorList mOperatorTable[OperatorCount];
        

        JSValue readEvalFile(const String& fileName);

        void buildRuntime(StmtNode *p);
        void buildRuntimeForStmt(StmtNode *p);
        ByteCodeModule *genCode(StmtNode *p, String sourceName);

        JSValue interpret(ByteCodeModule *bcm, JSValue *thisValue, JSValue *argv, uint32 argc);
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


    };

    inline AccessorReference::AccessorReference(JSFunction *f)
        : Reference(f->mResultType), mFunction(f) 
    {
    }

    inline bool AccessorReference::getValue(Context *cx) 
    { 
        cx->switchToFunction(mFunction);
        return true; 
    }
    
    inline bool AccessorReference::setValue(Context *cx) 
    { 
        cx->switchToFunction(mFunction);
        return true; 
    }
    
    inline bool LocalVarReference::getValue(Context *cx)
    {
        cx->mStack.push_back(cx->mLocals[mIndex]);
        return false;
    }

    inline bool LocalVarReference::setValue(Context *cx)
    {
        cx->mLocals[mIndex] = cx->mStack.back();
        return false;
    }
  
    inline bool JSObject::getProperty(Context *cx, const String &name, IdentifierList *attr)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Read, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case ValuePointer:
                cx->mStack.push_back(*prop.mData.vp);
                return false;
            case FunctionPair:
                cx->switchToFunction(prop.mData.fPair.getterF);
                return true;
            case Constructor:
            case Method:
                cx->mStack.push_back(JSValue(mType->mMethods[prop.mData.index]));
                return false;
            default:
                ASSERT(false);  // XXX more to implement
                return false;
            }
        }
        else {
            ASSERT(false);
            // prototype chain walking?
        }
        return false;
    }

    inline bool JSInstance::getProperty(Context *cx, const String &name, IdentifierList *attr)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Read, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case Slot:
                cx->mStack.push_back(mInstanceValues[prop.mData.index]);
                return false;
            case ValuePointer:
                cx->mStack.push_back(*prop.mData.vp);
                return false;
            case FunctionPair:
                cx->switchToFunction(prop.mData.fPair.getterF);
                return true;
            case Constructor:
            case Method:
                cx->mStack.push_back(JSValue(mType->mMethods[prop.mData.index]));
                return false;
            default:
                ASSERT(false);  // XXX more to implement
                return false;
            }
        }
        else {
            cx->mStack.push_back(kUndefinedValue);
            // XXX prototype chain walking?
        }
        return false;
    }

    inline bool JSObject::setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v)
    {
        PropertyIterator i;
        if (hasProperty(name, attr, Write, &i)) {
            Property &prop = PROPERTY(i);
            switch (prop.mFlag) {
            case ValuePointer:
                *prop.mData.vp = v;
                return false;
            case FunctionPair:
                cx->mStack.push_back(v);
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

    inline bool JSInstance::setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v)
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
                cx->mStack.push_back(v);
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

    inline bool JSArrayInstance::getProperty(Context *cx, const String &name, IdentifierList *attr)
    {
        if (name.compare(widenCString("length")) == 0) {
            cx->mStack.push_back(JSValue((float64)mLength));
            return true;
        }
        else
            return JSInstance::getProperty(cx, name, attr);
    }

    inline bool JSArrayInstance::setProperty(Context *cx, const String &name, IdentifierList *attr, JSValue &v)
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
                    mLength = v_int.f64 + 1;
            }
        }
        return false;
    }

    inline FunctionReference::FunctionReference(JSFunction *f) 
            : Reference(f->mResultType), mFunction(f)
    {
    }

    inline GetterFunctionReference::GetterFunctionReference(JSFunction *f) 
            : Reference(f->mResultType), mFunction(f)
    {
    }

    inline SetterFunctionReference::SetterFunctionReference(JSFunction *f) 
            : Reference(f->mResultType), mFunction(f)
    {
    }

    inline void JSType::addStaticMethod(const String &name, IdentifierList *attr, JSFunction *f)
    {
        defineStaticMethod(name, attr, f->mResultType, f);
    }

    inline void JSType::addMethod(const String &name, IdentifierList *attr, JSFunction *f)
    {
        defineMethod(name, attr, f->mResultType, f);
    }

    inline void JSInstance::initInstance(Context *cx, JSType *type)
    {
        if (type->mVariableCount)
            mInstanceValues = new JSValue[type->mVariableCount];

        // copy the instance variable names into the property map
        // only don't do it when 'this' and 'type' are the same, that's
        // the static instance being initialized to itself
        if (this != type) {
            
            // copy instance values from the Ur-instance object
            for (int i = 0; i < type->mVariableCount; i++)
                mInstanceValues[i] = type->mInitialInstance->mInstanceValues[i];

            for (PropertyIterator pi = type->mProperties.begin(), 
                        end = type->mProperties.end();
                        (pi != end); pi++) {            
                const PropertyMap::value_type e(PROPERTY_NAME(pi), ATTR_PROPERTY(pi));
                mProperties.insert(e);
            }
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
        mInitialInstance = new JSInstance(cx, NULL);
        if (mVariableCount)
            mInitialInstance->mInstanceValues = new JSValue[mVariableCount];
        if (f) {
            JSValue thisValue(mInitialInstance);
            cx->interpret(f->mByteCode, &thisValue, NULL, 0);
        }
    }

    inline void JSType::setStaticInitializer(Context *cx, JSFunction *f)
    {
        ASSERT(mStatics);
        mStatics->initInstance(cx, mStatics);       // build the static instance object
        if (f) {
            JSValue thisValue(mStatics);
            cx->interpret(f->mByteCode, &thisValue, NULL, 0);
        }
    }

    inline JSInstance *JSArrayType::newInstance(Context *cx)
    {
        JSInstance *result = new JSArrayInstance(cx, this);
        result->mPrototype = mPrototype;
        return result;
    }

    inline bool ScopeChain::setNameValue(const String& name, IdentifierList *attr, Context *cx)
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
            JSValue v = cx->mStack.back();
            top->defineVariable(name, attr, Object_Type, v);
            return false;
        }
    }


}
}

#endif //js2runtime_h___
