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


    class JSObject;
    class JSFunction;
    class JSType;
    class Context;


    extern JSType *Object_Type;         // the base type for all types
    extern JSType *Number_Type;
    extern JSType *String_Type;
    extern JSType *Type_Type;           // the type for variables that are types 
                                        // (e.g. the property 'C' from class C...
                                        // has this type).
    extern JSType *Boolean_Type;

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
        bool isNaN() const;
        bool isNegativeInfinity() const;
        bool isPositiveInfinity() const;
        bool isNegativeZero() const;
        bool isPositiveZero() const;

        JSType *getType();

        JSValue toString(Context *cx)             { return (isString() ? *this : valueToString(cx, *this)); }
        JSValue toNumber(Context *cx)             { return (isNumber() ? *this : valueToNumber(cx, *this)); }
        JSValue toObject(Context *cx)             { return ((isObject() || isType() || isFunction()) ?
                                                                *this : valueToObject(cx, *this)); }

        /* These are for use in 'toPrimitive' calls */
        enum Hint {
            NumberHint, StringHint, NoHint
        };
        JSValue toPrimitive(Context *cx, Hint hint = NoHint);
        
        static JSValue valueToNumber(Context *cx, JSValue& value);
        static JSValue valueToString(Context *cx, JSValue& value);
        static JSValue valueToObject(Context *cx, JSValue& value);
        
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

    typedef std::vector<JSValue> JSValueList;
    

    
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

        virtual void emitCodeSequence(ByteCodeGen *bcg) 
                { throw Exception(Exception::internalError, "gen code for base ref"); }
        virtual JSValue getValue()
                { throw Exception(Exception::internalError, "get value for base ref"); }
        virtual bool getValue(Context *cx)
                { throw Exception(Exception::internalError, "get value(cx) for base ref"); }

        virtual bool setValue(Context *cx)
                { throw Exception(Exception::internalError, "set value(cx) for base ref"); }
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
    };
    // a static function
    class StaticFunctionReference : public Reference {
    public:
        StaticFunctionReference(uint32 index, JSType *type)
            : Reference(type), mIndex(index) { }
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
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
        virtual JSValue getValue();
    };
    class GetterMethodReference : public MethodReference {
    public:
        GetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        virtual bool needsThis() { return true; }
    };
    class SetterMethodReference : public MethodReference {
    public:
        SetterMethodReference(uint32 index, JSType *baseClass, JSType *type)
            : MethodReference(index, baseClass, type) { }
        void emitCodeSequence(ByteCodeGen *bcg);
        virtual bool needsThis() { return true; }
        void emitImplicitLoad(ByteCodeGen *bcg);
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
        virtual bool needsThis() { return true; }
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
    // a known-to-exist property in an object
    // (this is where the global objects like classes are getting accumulated
    // during compilation for example)
    class ValueReference : public Reference {
    public:
        ValueReference(JSObject *base, const String& name, Access acc, JSType *type) 
            : Reference(type), mBase(base), mName(name), mAccess(acc) { }
        JSObject *mBase;
        const String& mName;
        Access mAccess;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx);
        bool setValue(Context *cx);
        JSValue getValue();
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
    // so it's a scope chain lookup
    class NameReference : public Reference {
    public:
        NameReference(const String& name, Access acc)
            : Reference(Object_Type), mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx)
                { throw Exception(Exception::internalError, "get value for name ref"); }
        bool setValue(Context *cx)
                { throw Exception(Exception::internalError, "set value for name ref"); }
    };




    
    
    
    
    
    

    
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSObject(JSType *type = Object_Type) : mType(type) { }

        
        // every object has a type
        JSType        *mType;

        // the property data is kept (or referenced from) here
        PropertyMap   mProperties;

        // Every JSObject has a private part
        JSValue       mPrivate;


        JSType *getType() const { return mType; }
/*
        // see if the property exists in any form
        bool hasProperty(const String &name)
        {
            PropertyIterator i = mProperties.find(name);
            return (i != mProperties.end());
        }
*/
        // see if the property exists by a specific kind of access
        bool hasProperty(const String &name, Access acc)
        {
            PropertyIterator i = mProperties.find(name);
            if (i != mProperties.end()) {
                Property& prop = PROPERTY(i);
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

        // get a property value
        JSValue getProperty(const String &name)
        {
            Property &prop = getPropertyData(name);
            ASSERT(prop.mFlag == ValuePointer);
            return *prop.mData.vp;
        }

        // set a property value
        void setProperty(const String &name, JSValue &v)
        {
            Property &prop = getPropertyData(name);
            ASSERT(prop.mFlag == ValuePointer);
            *prop.mData.vp = v;
        }

        virtual bool getProperty(Context *cx, const String &name);

        virtual bool setProperty(Context *cx, const String &name, JSValue &v);



        // add a property
        virtual PropertyIterator defineVariable(const String &name, JSType *type)
        {
            const PropertyMap::value_type e(name, Property(new JSValue(), type));
            std::pair<PropertyIterator, bool> r = mProperties.insert(e);
            return r.first;
        }
        virtual PropertyIterator defineStaticVariable(const String &name, JSType *type)
        {
            return defineVariable(name, type);
        }
        // add a method property
        virtual PropertyIterator defineMethod(const String &name, JSType *type)
        {
            return defineVariable(name, type);
        }
        virtual PropertyIterator defineStaticMethod(const String &name, JSType *type)
        {
            return defineVariable(name, type);
        }
        virtual PropertyIterator defineGetterMethod(const String &name, JSType *type)
        {
            if (hasProperty(name, Write)) {
                PropertyIterator i = mProperties.find(name);
                ASSERT(PROPERTY_KIND(i) == FunctionPair);
                ASSERT(PROPERTY_GETTERF(i) == NULL);
                return i;
            }
            else {
                const PropertyMap::value_type e(name, Property(type, NULL, NULL));
                std::pair<PropertyIterator, bool> r = mProperties.insert(e);
                return r.first;
            }
        }
        virtual PropertyIterator defineSetterMethod(const String &name, JSType *type)
        {
            if (hasProperty(name, Read)) {
                PropertyIterator i = mProperties.find(name);
                ASSERT(PROPERTY_KIND(i) == FunctionPair);
                ASSERT(PROPERTY_SETTERF(i) == NULL);
                return i;
            }
            else {
                const PropertyMap::value_type e(name, Property(type, NULL, NULL));
                std::pair<PropertyIterator, bool> r = mProperties.insert(e);
                return r.first;
            }
        }

        // add a property (with a value)
        virtual PropertyIterator defineVariable(const String &name, JSType *type, JSValue v)
        {
            const PropertyMap::value_type e(name, Property(new JSValue(v), type));
            std::pair<PropertyIterator, bool> r = mProperties.insert(e);
            return r.first;
        }

        // get all the goods on a specific property
        Property &getPropertyData(const String &name)
        {
            PropertyIterator i = mProperties.find(name);
            ASSERT(i != mProperties.end());
            return PROPERTY(i);
        }


        virtual void setValue(Property& prop, JSValue v)
        {
            ASSERT(prop.mFlag == ValuePointer);
            *prop.mData.vp = v;
        }
        virtual void setGetterValue(Property& prop, JSFunction *f)
        {
            ASSERT(prop.mFlag == FunctionPair);
            prop.mData.fPair.getterF = f;
        }
        virtual void setSetterValue(Property& prop, JSFunction *f)
        {
            ASSERT(prop.mFlag == FunctionPair);
            prop.mData.fPair.setterF = f;
        }
        virtual JSFunction *getGetterValue(Property& prop)
        {
            ASSERT(prop.mFlag == FunctionPair);
            return prop.mData.fPair.getterF;
        }
        virtual JSFunction *getSetterValue(Property& prop)
        {
            ASSERT(prop.mFlag == FunctionPair);
            return prop.mData.fPair.setterF;
        }
        virtual void setStaticValue(Property& prop, JSValue v)
        {
            setValue(prop, v);      // XXX or should this be an error ? 
        }
        virtual JSValue getValue(Property& prop)
        {
            ASSERT(prop.mFlag == ValuePointer);
            return *prop.mData.vp;
        }
        virtual JSValue getStaticValue(Property& prop)
        {
            return getValue(prop);  // XXX or should this be an error ?
        }

        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            switch (prop.mFlag) {
            case ValuePointer:
                return new ValueReference(this, name, acc, prop.mType);
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
        default: NOT_REACHED("bad type"); return NULL;
        }
    }



    


        
    
    
    
    
    

    
 


    class JSInstance : public JSObject {
    public:
        
        JSInstance(JSType *type) 
            : JSObject(type), mInstanceValues(NULL) { if (type) initInstance(type); }

        void initInstance(JSType *type);

        bool getProperty(Context *cx, const String &name);
        bool setProperty(Context *cx, const String &name, JSValue &v);

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
        
        JSType(const String &name, JSType *super) : JSInstance(Type_Type),
                                            mSuperType(super), 
                                            mStatics(NULL), 
                                            mVariableCount(0),
                                            mClassName(name)
        {
        }

        JSType(JSType *super) : JSInstance(Type_Type),
                                            mSuperType(super), 
                                            mStatics(NULL), 
                                            mVariableCount(0)
        {
        }

        void createStaticComponent()    // builds the JSType that will hold the static
                                        // info (map etc) for this class
        {
            mStatics = new JSType(this);
        }

        void createStaticInstance()     // when the class is 'complete' the static instance
                                        // is constructed ready to hold the static instance values.
        {
            mStatics->initInstance(mStatics);
        }

        // construct a new (empty) instance of this class
        JSInstance *newInstance();
    
        
        // static helpers

        PropertyIterator defineStaticMethod(const String& name, JSType *type)
        {
            ASSERT(mStatics);
            return mStatics->defineMethod(name, type);
        }

        PropertyIterator defineStaticVariable(const String& name, JSType *type)
        {
            ASSERT(mStatics);
            return mStatics->defineVariable(name, type);
        }

        bool hasStatic(const String& name, Access acc)
        {
            ASSERT(mStatics);
            return mStatics->hasProperty(name, acc);
        }

        void setStaticValue(Property& prop, JSValue v)
        {
            ASSERT(mStatics);
            mStatics->setValue(prop, v);
        }

        JSValue getStaticValue(Property& prop)
        {
            ASSERT(mStatics);
            return mStatics->getValue(prop);
        }

        //


        PropertyIterator defineVariable(const String& name, JSType *type)
        {
            const PropertyMap::value_type e(name, Property(mVariableCount++, type, Slot));
            std::pair<PropertyIterator, bool> r = mProperties.insert(e);
            return r.first;        
        }

        PropertyIterator defineMethod(const String& name, JSType *type)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(NULL);

            const PropertyMap::value_type e(name, Property(vTableIndex, type, Method));
            std::pair<PropertyIterator, bool> r = mProperties.insert(e);
            return r.first;
        }
        
        PropertyIterator defineGetterMethod(const String &name, JSType *type)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(NULL);

            if (hasProperty(name, Write)) {
                PropertyIterator i = mProperties.find(name);
                ASSERT(PROPERTY_KIND(i) == IndexPair);
                ASSERT(PROPERTY_GETTERI(i) == 0);
                PROPERTY_GETTERI(i) = vTableIndex;
                return i;
            }
            else {
                const PropertyMap::value_type e(name, Property(vTableIndex, 0, type));
                std::pair<PropertyIterator, bool> r = mProperties.insert(e);
                return r.first;
            }
        }

        PropertyIterator defineSetterMethod(const String &name, JSType *type)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(NULL);

            if (hasProperty(name, Read)) {
                PropertyIterator i = mProperties.find(name);
                ASSERT(PROPERTY_KIND(i) == IndexPair);
                ASSERT(PROPERTY_SETTERI(i) == 0);
                PROPERTY_SETTERI(i) = vTableIndex;
                return i;
            }
            else {
                const PropertyMap::value_type e(name, Property(0, vTableIndex, type));
                std::pair<PropertyIterator, bool> r = mProperties.insert(e);
                return r.first;
            }
        }

        void setDefaultConstructor(JSFunction *f)
        {
            PropertyIterator it = defineStaticMethod(mClassName, Object_Type);
            PROPERTY_KIND(it) = Constructor;
            setStaticValue(PROPERTY(it), JSValue(f));
        }

        void addStaticMethod(const String &name, JSFunction *f);

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

        void setValue(Property& prop, JSValue v)
        {
            switch (prop.mFlag) {
            case Constructor:
            case Method:
                ASSERT(v.isFunction());
                mMethods[prop.mData.index] = v.function;
                break;
            default:
                JSObject::setValue(prop, v);
                break;
            }
        }
        void setGetterValue(Property& prop, JSFunction *f)
        {
            ASSERT(prop.mFlag == IndexPair);
            mMethods[prop.mData.iPair.getterI] = f;
        }
        void setSetterValue(Property& prop, JSFunction *f)
        {
            ASSERT(prop.mFlag == IndexPair);
            mMethods[prop.mData.iPair.setterI] = f;
        }
        JSFunction *getGetterValue(Property& prop)
        {
            ASSERT(prop.mFlag == IndexPair);
            return mMethods[prop.mData.iPair.getterI];
        }
        JSFunction *getSetterValue(Property& prop)
        {
            ASSERT(prop.mFlag == IndexPair);
            return mMethods[prop.mData.iPair.setterI];
        }

        JSValue getValue(Property& prop)
        {
            switch (prop.mFlag) {
            case Constructor:
            case Method:
                return JSValue(mMethods[prop.mData.index]);
            default:
                return JSObject::getValue(prop);
            }
        }

        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            if (hasProperty(name, acc)) {
                Property &prop = getPropertyData(name);
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
                default:
                    NOT_REACHED("bad storage kind");
                    return NULL;
                }
            }
            // walk the supertype chain
            if (mSuperType)
                return mSuperType->genReference(name, acc, depth);
            return NULL;
        }

        // constructor functions are added as static methods
        // XXX is it worth just having a default constructor 
        // pointer in the class?
        JSFunction *getDefaultConstructor()
        {
            if (!mSuperType) return NULL;   // ground out at Object
            ASSERT(mStatics);
            for (PropertyIterator i = mStatics->mProperties.begin(), 
                            end = mStatics->mProperties.end(); (i != end); i++) {
                if ((PROPERTY_KIND(i) == Constructor)
                        && (PROPERTY_NAME(i).compare(mClassName) == 0)) {
                    return mStatics->mMethods[PROPERTY_INDEX(i)];
                }
            }
            return NULL;
        }

        // assumes that the super types have been completed already
        void completeClass(ScopeChain *scopeChain, JSType *super);


        JSType          *mSuperType;        // NULL implies that this is the base Object

        uint32          mVariableCount;
        JSType          *mStatics;          // NULL implies that this is the static component

        // the 'vtable'
        MethodList      mMethods;
        String          mClassName;


        void printSlotsNStuff(Formatter& f) const;

    };

    Formatter& operator<<(Formatter& f, const JSType& obj);

    
    





    // captures the Parameter names scope
    class ParameterBarrel : public JSType {
    public:

        ParameterBarrel() : JSType(NULL) { }

        Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            ASSERT(prop.mFlag == Slot);
            return new ParameterReference(prop.mData.index, acc, prop.mType);
        }

    };






    inline void JSInstance::initInstance(JSType *type)
    {
        if (type->mVariableCount)
            mInstanceValues = new JSValue[type->mVariableCount];
        // copy the instance variable names into the property map
        for (PropertyIterator i = type->mProperties.begin(), 
                    end = type->mProperties.end();
                    (i != end); i++) {            
            mProperties[PROPERTY_NAME(i)] = PROPERTY(i);
        }
        // and then do the same for the super types
        JSType *t = type->mSuperType;
        while (t) {
            for (PropertyIterator i = t->mProperties.begin(), 
                        end = t->mProperties.end();
                        (i != end); i++) {            
                mProperties[PROPERTY_NAME(i)] = PROPERTY(i);            
            }
            t = t->mSuperType;
        }
        mType = type;
    }

    inline JSInstance *JSType::newInstance()
    {
        return new JSInstance(this);
    }







    // an Activation has two jobs:
    // 1. At compile time it represents the function/method being compiled and collects
    //      the local vars/consts being defined in that function. 
    // 2. At runtime it is the container for the values of those local vars
    //      (although it's only constructed as such when the function 
    //          either calls another function - so the activation represents
    //          the saved state, or when a closure object is constructed)

    class Activation : public JSType {
    public:

        Activation() : JSType(NULL), 
                        mLocals(NULL), 
                        mPC(0), 
                        mModule(NULL), 
                        mArgCount(0) { }

        Activation(JSValue *locals, 
                        uint32 argBase, 
                        uint8 *pc, 
                        ByteCodeModule *module, 
                        uint32 argCount);

        
        // saved values from a previous execution
        JSValue *mLocals;
        uint32 mArgumentBase;
        uint8 *mPC;
        ByteCodeModule *mModule;
        uint32 mArgCount;


        bool hasLocalVars()             { return true; }
        virtual uint32 localVarCount()  { return mVariableCount; }

        Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            ASSERT((prop.mFlag == Slot) || (prop.mFlag == FunctionPair)); 

            if (prop.mFlag == FunctionPair) 
                return (acc == Read) ? new AccessorReference(prop.mData.fPair.getterF)
                                     : new AccessorReference(prop.mData.fPair.setterF);

            if (depth)
                return new ClosureVarReference(depth, prop.mData.index, acc, prop.mType);

            return new LocalVarReference(prop.mData.index, acc, prop.mType);
        }

    };


//    class ParameterBarrel;

    class JSFunction : public JSObject {
    public:
        typedef JSValue (NativeCode)(Context *cx, JSValue *argv, uint32 argc);
        JSFunction(JSType *resultType) 
                    : mByteCode(NULL), 
                        mCode(NULL), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL) { }
        JSFunction(NativeCode *code, JSType *resultType) 
                    : mByteCode(NULL), 
                        mCode(code), 
                        mResultType(resultType), 
                        mParameterBarrel(NULL) { }

        bool isNative() { return (mCode != NULL); }

        ByteCodeModule *mByteCode;
        NativeCode *mCode;
        JSType *mResultType;
        ParameterBarrel *mParameterBarrel;
        Activation mActivation;
        
    
    };

    
    
    class ScopeChain {
    public:

        ScopeChain(World &world) :
              VirtualKeyWord(world.identifiers["virtual"]),
              ConstructorKeyWord(world.identifiers["constructor"]),
              OperatorKeyWord(world.identifiers["operator"])           { }

        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 

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
        PropertyIterator defineVariable(const String& name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, type);
        }
        PropertyIterator defineVariable(const String& name, JSType *type, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, type, v);
        }
        PropertyIterator defineStaticVariable(const String& name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            return top->defineStaticVariable(name, type);
        }
        PropertyIterator defineMethod(const String& name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineMethod(name, type);
        }   
        PropertyIterator defineStaticMethod(const String& name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            return top->defineStaticMethod(name, type);
        }   
        PropertyIterator defineGetterMethod(const String &name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineGetterMethod(name, type);
        }
        PropertyIterator defineSetterMethod(const String &name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineSetterMethod(name, type);
        }

        // see if the current scope contains a name already
        bool hasProperty(const String& name, Access acc)
        {
            JSObject *top = mScopeStack.back();
            return top->hasProperty(name, acc);
        }

        // generate a reference to the given name
        Reference *getName(const String& name, Access acc)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                if ((*s)->hasProperty(name, acc))
                    return (*s)->genReference(name, acc, depth);
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

        // a compile time request to get the value for a name
        // (i.e. we're accessing a constant value)
        JSValue getValue(const String& name)
        {
            Reference *ref = getName(name, Read);
            ASSERT(ref);
            JSValue result = ref->getValue();
            delete ref;
            return result;
        }

        // Compile time requests to set a value - these follow
        // the creation of the value in the topmost scope, and 
        // so are guaranteed to succeed.
        void setValue(Property& prop, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            top->setValue(prop, v);
        }
        void setGetterValue(Property& prop, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->setGetterValue(prop, f);
        }
        void setSetterValue(Property& prop, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->setSetterValue(prop, f);
        }
        void setStaticValue(Property& prop, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            top->setStaticValue(prop, v);
        }

        JSFunction *getGetterValue(Property& prop)
        {
            JSObject *top = mScopeStack.back();
            return top->getGetterValue(prop);
        }
        JSFunction *getSetterValue(Property& prop)
        {
            JSObject *top = mScopeStack.back();
            return top->getSetterValue(prop);
        }
        JSValue getValue(Property& prop)
        {
            JSObject *top = mScopeStack.back();
            return top->getValue(prop);
        }
        JSValue getStaticValue(Property& prop)
        {
            JSObject *top = mScopeStack.back();
            return top->getStaticValue(prop);
        }

        // a runtime request to get the value for a name
        // it'll either return a value or cause the
        // interpreter to switch execution to a getter function
        bool getNameValue(const String& name, Context *cx)
        {
            Reference *ref = getName(name, Read);
            if (ref == NULL)
                throw Exception(Exception::referenceError, name);
            bool result = ref->getValue(cx);
            delete ref;
            return result;
        }

        bool setNameValue(const String& name, Context *cx)
        {
            JSObject *top = *mScopeStack.rbegin();
            Reference *ref = NULL;
            if (top->hasProperty(name, Write))
                ref = top->genReference(name, Write, 0);
            else {
                // then define the property in the top scope
                ref = new ValueReference(top, name, Write, Object_Type);
                                                            // XXX Optimize this by defining the variable
                                                            // and setting it's value without building a reference
                                                            // (and re-visit the whole issue of constructing
                                                            // references at runtime anyway [who's deleting these?])
                top->defineVariable(name, Object_Type);
            }
            bool result = ref->setValue(cx);
            delete ref;
            return result;
        }

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
            : mType1(type1), mType2(type2), mImp(imp) { }


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

    JSValue Number_Constructor(Context *cx, JSValue *argv, uint32 argc);
    JSValue Number_ToString(Context *cx, JSValue *argv, uint32 argc);

    class Context {
    public:

        Context(JSObject *global, World &world) 
            : mGlobal(global), 
              mWorld(world),
              mScopeChain(mWorld)
        {
            initOperators();
            if (Object_Type == NULL) {
                Object_Type = new JSType(NULL);
                Number_Type = new JSType(widenCString("Number"), Object_Type);
                Number_Type->createStaticComponent();
                Number_Type->setDefaultConstructor(new JSFunction(&Number_Constructor, Object_Type));
                Number_Type->addStaticMethod(widenCString("toString"), new JSFunction(&Number_ToString, String_Type));
                Number_Type->completeClass(&mScopeChain, Object_Type);
                Number_Type->createStaticInstance();
                
                String_Type = new JSType(Object_Type);
                Boolean_Type = new JSType(Object_Type);
                Type_Type = new JSType(Object_Type);        
            }
        }

        void initOperators();
        
        void defineOperator(Operator op, JSType *t1, JSType *t2, JSFunction *imp)
        {
        
        }

        bool executeOperator(Operator op, JSType *t1, JSType *t2);

        void switchToFunction(JSFunction *f)
        {
            ASSERT(false);  // somebody should write some code here one day
        }

        JSObject *mGlobal;
        World &mWorld;
        ScopeChain mScopeChain;

        // the currently executing 'function'
        ByteCodeModule *mCurModule;

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
        JSValue interpret(ByteCodeModule *bcm, JSValueList args);
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
        cx->mStack.pop_back();
        return false;
    }

    inline JSValue ValueReference::getValue()
    {
        return mBase->getProperty(mName);
    }

    inline bool ValueReference::getValue(Context *cx)
    {
        cx->mStack.push_back(mBase->getProperty(mName));
        return false;
    }

    inline bool ValueReference::setValue(Context *cx)
    {
        mBase->setProperty(mName, cx->mStack.back());
        cx->mStack.pop_back();
        return false;
    }

    inline JSValue MethodReference::getValue()
    {
        return JSValue(mClass->mMethods[mIndex]);
    }

    
    inline bool JSObject::getProperty(Context *cx, const String &name)
    {
        if (hasProperty(name, Read)) {
            Property &prop = getPropertyData(name);
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

    inline bool JSInstance::getProperty(Context *cx, const String &name)
    {
        if (hasProperty(name, Read)) {
            Property &prop = getPropertyData(name);
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

    inline bool JSObject::setProperty(Context *cx, const String &name, JSValue &v)
    {
        if (hasProperty(name, Write)) {
            Property &prop = getPropertyData(name);
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
            defineVariable(name, Object_Type, v);
            return false;
        }
    }

    inline bool JSInstance::setProperty(Context *cx, const String &name, JSValue &v)
    {
        if (hasProperty(name, Write)) {
            Property &prop = getPropertyData(name);
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
            defineVariable(name, Object_Type, v);
            return false;
        }
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

    inline void JSType::addStaticMethod(const String &name, JSFunction *f)
    {
        PropertyIterator it = defineStaticMethod(name, f->mResultType);
        setStaticValue(PROPERTY(it), JSValue(f));
    }

    bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind);
    bool hasAttribute(const IdentifierList* identifiers, StringAtom &name);

}
}

#endif //js2runtime_h___
