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

        JSValue toString(Context *cx) const             { return (isString() ? *this : valueToString(cx, *this)); }
        JSValue toNumber() const                        { return (isNumber() ? *this : valueToNumber(*this)); }

        /* These are the ECMA types, for use in 'toPrimitive' calls */
        enum ECMA_type {
            Undefined, Null, Boolean, Number, Object, String, 
            NoHint
        };
        JSValue toPrimitive(Context *cx, ECMA_type hint = NoHint) const;
        
        static JSValue valueToNumber(const JSValue& value);
        static JSValue valueToString(Context *cx, const JSValue& value);
        
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

        virtual void emitImplicitLoad(ByteCodeGen *bcg) { } 

        virtual void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true) 
                { throw Exception(Exception::runtimeError, "gen code for base ref"); }
        virtual JSValue getValue()
                { throw Exception(Exception::runtimeError, "get value for base ref"); }
        virtual bool getValue(Context *cx)
                { throw Exception(Exception::runtimeError, "get value(cx) for base ref"); }

        virtual bool setValue(Context *cx)
                { throw Exception(Exception::runtimeError, "set value(cx) for base ref"); }
    };

    // a getter/setter function from an activation
    // the function is known directly
    class AccessorReference : public Reference {
    public:
        AccessorReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
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
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
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
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
    };
    // a member field in an instance/Class
    class FieldReference : public Reference {
    public:
        FieldReference(uint32 index, Access acc, JSType *baseClass, JSType *type, bool isStatic) 
            : Reference(type), mAccess(acc), mIndex(index), mIsStatic(isStatic) { }
        Access mAccess;
        uint32 mIndex;
        JSType *mClass;
        bool mIsStatic;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
        void emitImplicitLoad(ByteCodeGen *bcg);
    };
    // a member function in a vtable
    class MethodReference : public Reference {
    public:
        MethodReference(uint32 index, JSType *baseClass, JSType *type) 
            : Reference(type), mIndex(index), mClass(baseClass) { }
        uint32 mIndex;
        JSType *mClass;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
        virtual bool needsThis() { return true; }
        virtual JSValue getValue();
    };
    // a function
    class FunctionReference : public Reference {
    public:
        FunctionReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
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
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
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
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
    };
    // a parameter slot (they can't have getter/setter, right?)
    class ParameterReference : public Reference {
    public:
        ParameterReference(uint32 index, Access acc, JSType *type) 
            : Reference(type), mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
    };

    // the generic "we don't know anybody by that name" - not bound to a specific object
    // so it's a scope chain lookup
    class NameReference : public Reference {
    public:
        NameReference(const String& name, Access acc)
            : Reference(Object_Type), mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg, bool hasBase = true);
        bool getValue(Context *cx)
                { throw Exception(Exception::runtimeError, "get value for name ref"); }
        bool setValue(Context *cx)
                { throw Exception(Exception::runtimeError, "set value for name ref"); }
    };




    
    
    
    
    
    

    
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSObject(JSType *type = Object_Type) : mType(type) { }

        
        // every object has a type
        JSType        *mType;

        // the property data is kept (or referenced from) here
        PropertyMap   mProperties;


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
                    return (acc == Read) ? (prop.mData.fPair.getF != NULL)
                                         : (prop.mData.fPair.setF != NULL);
                else
                    if (prop.mFlag == IndexPair)
                        return (acc == Read) ? (prop.mData.iPair.getIndex != -1)
                                             : (prop.mData.iPair.setIndex != -1);
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
        virtual void setStaticValue(Property& prop, JSValue v)
        {
            setValue(prop, v);
        }
        virtual JSValue getValue(Property& prop)
        {
            ASSERT(prop.mFlag == ValuePointer);
            return *prop.mData.vp;
        }
        virtual JSValue getStaticValue(Property& prop)
        {
            return getValue(prop);
        }

        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            switch (prop.mFlag) {
            case ValuePointer:
                return new ValueReference(this, name, acc, prop.mType);
            case FunctionPair:
                if (acc == Read)
                    return new FunctionReference(prop.mData.fPair.getF);
                else
                    return new FunctionReference(prop.mData.fPair.setF);
            default:
                NOT_REACHED("bad storage kind");
                return NULL;
            }
        }

        virtual bool hasLocalVars()     { return false; }
        virtual uint32 localVarCount()  { return 0; }

        
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



    class ParameterBarrel;

    class JSFunction : public JSObject {
    typedef JSValue (NativeCode)(Context *cx, JSValue *argv, uint32 argc);
    public:
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
    };
    


        
    
    
    
    
    

    
 


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


    // A Type (a JS2 Class) is also an instance, that's where the static values
    // get stored.
    class JSType : public JSInstance {
    public:
        
        JSType(JSType *super) : JSInstance(NULL),
                                    mSuperType(super), 
                                    mStatics(NULL), 
                                    mVariableCount(0)
        {
        }

        void createStaticComponent()
        {
            mStatics = new JSType(NULL);
        }

        void createStaticInstance()
        {
            initInstance(mStatics);
        }

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

        void defineConstructor(const String& name, JSType *type)
        {
            defineMethod(name, type);
        }

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
            case Method:
                ASSERT(v.isFunction());
                mMethods[prop.mData.index] = v.function;
                break;
            default:
                JSObject::setValue(prop, v);
                break;
            }
        }

        JSValue getValue(Property& prop)
        {
            switch (prop.mFlag) {
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
                        return new FunctionReference(prop.mData.fPair.getF);
                    else
                        return new FunctionReference(prop.mData.fPair.setF);
                case IndexPair:
                    if (acc == Read)
                        return new MethodReference(prop.mData.iPair.getIndex, this, prop.mType);
                    else
                        return new MethodReference(prop.mData.iPair.setIndex, this, prop.mType);
                case Slot:
                    return new FieldReference(prop.mData.index, acc, this, prop.mType, (mStatics == NULL));
                case Method:
                    return new MethodReference(prop.mData.index, this, prop.mType);
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

        // assumes that the super types have been completed already
        void completeClass(JSType *super)
        {
            // add the super type instance variable count into the slot indices
            uint32 superInstanceVarCount = 0;
            uint32 super_vTableCount = 0;
            if (super) {
                superInstanceVarCount = super->mVariableCount;
                super_vTableCount = super->mMethods.size();
            }

            mVariableCount += superInstanceVarCount;
            if (superInstanceVarCount) {
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
                    if (PROPERTY_KIND(i) == Method)
                        PROPERTY_INDEX(i) += super_vTableCount;
                }
                mMethods.insert(mMethods.begin(), 
                                    super->mMethods.begin(), 
                                    super->mMethods.end());
            }
            if (mStatics && mSuperType)
                mStatics->completeClass(mSuperType->mStatics);
        }


        JSType          *mSuperType;

        uint32          mVariableCount;
        JSType          *mStatics;

        // the 'vtable'
        MethodList      mMethods;
        
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

        Activation() : JSType(NULL), mLocals(NULL), mPC(0), mModule(NULL) { }

        Activation(JSValue *locals, JSValue *argBase, uint8 *pc, ByteCodeModule *module);

        
        // saved values from a previous execution
        JSValue *mLocals;
        JSValue *mArgumentBase;
        uint8 *mPC;
        ByteCodeModule *mModule;


        bool hasLocalVars()             { return true; }
        virtual uint32 localVarCount()  { return mVariableCount; }

        Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            ASSERT((prop.mFlag == Slot) || (prop.mFlag == FunctionPair)); 

            if (prop.mFlag == FunctionPair) 
                return (acc == Read) ? new AccessorReference(prop.mData.fPair.getF)
                                     : new AccessorReference(prop.mData.fPair.setF);

            if (depth)
                return new ClosureVarReference(depth, prop.mData.index, acc, prop.mType);

            return new LocalVarReference(prop.mData.index, acc, prop.mType);
        }

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

        void setStaticValue(Property& prop, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            top->setStaticValue(prop, v);
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


        void processDeclarations(StmtNode *p);

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

    class Context {
    public:

        Context(JSObject *global, World &world) 
            : mGlobal(global), 
              mWorld(world),
              mScopeChain(mWorld)
        {
            initOperators();
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
        // (it's a pointer into the execution stack)
        JSValue *mArgumentBase;

        typedef std::vector<OperatorDefinition *> OperatorList;
            
        OperatorList mOperatorTable[OperatorCount];
        

        JSValue readEvalFile(const String& fileName);

        void buildRuntime(StmtNode *p);
        void buildRuntimeForStmt(StmtNode *p);
        void processDeclarations(StmtNode *p);

        
        ByteCodeModule *genCode(StmtNode *p, String sourceName);
        JSValue interpret(ByteCodeModule *bcm, JSValueList args);
        JSValue interpret(uint8 *pc, uint8 *endPC);
        
        
        /* utility routines */

        // Extract the operator from the string literal function name
        // - requires the paramter count in order to distinguish
        // between unary and binary operators.
        Operator getOperator(uint32 parameterCount, String &name);

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
                cx->switchToFunction(prop.mData.fPair.getF);
                return true;
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
                cx->switchToFunction(prop.mData.fPair.getF);
                return true;
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
                cx->switchToFunction(prop.mData.fPair.setF);
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
                cx->switchToFunction(prop.mData.fPair.setF);
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


    bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind);

}
}

#endif //js2runtime_h___