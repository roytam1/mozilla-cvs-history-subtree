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

#include "tracer.h"

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

#define JSDOUBLE_IS_POSZERO(d)  (JSDOUBLE_HI32(d) == 0 && JSDOUBLE_LO32(d) == 0)

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
    extern JSType *Function_Type;

    bool hasAttribute(AttributeList *attributes, Token::Kind tokenKind, IdentifierExprNode **attrArg = NULL);
    bool hasAttribute(AttributeList *attributes, const StringAtom &name, IdentifierExprNode **attrArg = NULL);

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
            null_tag
        } Tag;
        Tag tag;
        
#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSValue", s, t); return t; }
        void operator delete(void* t) { trace_release("JSValue", t); STD::free(t); }
#endif

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
        JSFunction*& operator=(JSFunction* function)    { return (tag = function_tag, this->function = function); }
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
        bool isPositiveZero() const                     { ASSERT(isNumber()); return JSDOUBLE_IS_POSZERO(f64); }

        bool isTrue() const                             { ASSERT(isBool()); return boolean; }
        bool isFalse() const                            { ASSERT(isBool()); return !boolean; }

        JSType *getType() const;

        JSValue toString(Context *cx) const             { return (isString() ? *this : valueToString(cx, *this)); }
        JSValue toNumber(Context *cx) const             { return (isNumber() ? *this : valueToNumber(cx, *this)); }
        JSValue toUInt32(Context *cx) const             { return valueToUInt32(cx, *this); }
        JSValue toUInt16(Context *cx) const             { return valueToUInt16(cx, *this); }
        JSValue toInt32(Context *cx) const              { return valueToInt32(cx, *this); }
        JSValue toObject(Context *cx) const             { return ((isObject() || isType() || isFunction()) ?
                                                                *this : valueToObject(cx, *this)); }
        JSValue toBoolean(Context *cx) const            { return (isBool() ? *this : valueToBoolean(cx, *this)); }

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

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("Reference", s, t); return t; }
        void operator delete(void* t)   { trace_release("Reference", t); STD::free(t); }
#endif

        // used by the invocation sequence to calculate
        // the stack depth and specify the 'this' flag
        virtual bool needsThis()                            { return false; }

        // issued as soon as the reference is generated to
        // establish any required base object.
        virtual void emitImplicitLoad(ByteCodeGen *)        { } 

        // acquires the invokable object
        virtual void emitInvokeSequence(ByteCodeGen *bcg)   { emitCodeSequence(bcg); }

        // issued before the rvalue is evaluated.
        // returns true if it pushes anything on the stack
        virtual bool emitPreAssignment(ByteCodeGen *)       { return false; }

        virtual void emitCodeSequence(ByteCodeGen *) 
                { throw Exception(Exception::internalError, "gen code for base ref"); }

        // returns the amount of stack used by the reference
        virtual int32 baseExpressionDepth()                 { return 0; }

        // generate code sequence for typeof operator
        virtual void emitTypeOf(ByteCodeGen *bcg);

        // generate code sequence for delete operator
        virtual void emitDelete(ByteCodeGen *bcg);
    };

    // a getter/setter function from an activation
    // the function is known directly
    class AccessorReference : public Reference {
    public:
        AccessorReference(JSFunction *f);
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
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
        int32 baseExpressionDepth() { return 1; }
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
        int32 baseExpressionDepth() { return 1; }
    };
    // a static function
    class StaticFunctionReference : public Reference {
    public:
        StaticFunctionReference(uint32 index, JSType *type)
            : Reference(type), mIndex(index) { }
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        void emitInvokeSequence(ByteCodeGen *bcg);
        int32 baseExpressionDepth() { return 1; }
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
        virtual int32 baseExpressionDepth() { return 1; }
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
        bool needsThis() { return false; }          // i.e. there is no 'this' specified by the
                                                    // the call sequence (it's a static function)
        int32 baseExpressionDepth() { return 1; }
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
        int32 baseExpressionDepth() { return 1; }
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
        void emitDelete(ByteCodeGen *bcg);
    };

    class ElementReference : public Reference {
    public:
        ElementReference(Access acc, int32 depth)
            : Reference(Object_Type), mAccess(acc), mDepth(depth) { }
        Access mAccess;
        int32 mDepth;
        void emitCodeSequence(ByteCodeGen *bcg);
        int32 baseExpressionDepth() { return mDepth; }
    };



    
    
    
    
    
    

    
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSObject(JSType *type = Object_Type) : mType(type), mPrivate(NULL), mPrototype(NULL) { }
        
        virtual ~JSObject() { } // keeping gcc happy
        
#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSObject", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSObject", t); STD::free(t); }
#endif

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


        // find a property by the given name, and then check to see if there's any
        // overlap between the supplied attribute list and the property's list.
        // ***** REWRITE ME -- matching attribute lists for inclusion is a bad idea.
        PropertyIterator findAttributedProperty(const String &name, AttributeList * /*attrs*/);

        void deleteProperty(const String &name, AttributeList *attr)
        {
            PropertyIterator i = findAttributedProperty(name, attr);
            mProperties.erase(i);
        }

        // see if the property exists by a specific kind of access
        bool hasOwnProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p);
        
        virtual bool hasProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p);

        virtual JSValue getPropertyValue(PropertyIterator &i);

        virtual void getProperty(Context *cx, const String &name, AttributeList *attr);

        virtual void setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);



        // add a property
        virtual Property *defineVariable(const String &name, AttributeList *attr, JSType *type)
        {
            Property *prop = new Property(new JSValue(), type);
            const PropertyMap::value_type e(name, new AttributedProperty(prop, attr));
            mProperties.insert(e);
            return prop;
        }
        virtual Property *defineStaticVariable(const String &name, AttributeList *attr, JSType *type)
        {
            return defineVariable(name, attr, type);    // XXX or error?
        }
        // add a method property
        virtual void defineMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            defineVariable(name, attr, Function_Type, JSValue(f));
        }
        virtual void defineStaticMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            defineVariable(name, attr, Function_Type, JSValue(f));    // XXX or error?
        }
        virtual void defineConstructor(const String& name, AttributeList *attr, JSFunction *f)
        {
            defineVariable(name, attr, Function_Type, JSValue(f));    // XXX or error?
        }
        virtual void defineStaticGetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            defineGetterMethod(name, attr, f);    // XXX or error?
        }
        virtual void defineStaticSetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            defineSetterMethod(name, attr, f);    // XXX or error?
        }
        virtual void defineGetterMethod(const String &name, AttributeList *attr, JSFunction *f);
        virtual void defineSetterMethod(const String &name, AttributeList *attr, JSFunction *f);

        virtual Property *defineVariable(const String &name, AttributeList *attr, JSType *type, JSValue v);
        virtual Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 /*depth*/);

        virtual bool hasLocalVars()     { return false; }
        virtual uint32 localVarCount()  { return 0; }

        // debug only        
        void printProperties(Formatter &f) const
        {
            for (PropertyMap::const_iterator i = mProperties.begin(), end = mProperties.end(); (i != end); i++) 
            {
                f << "[" << PROPERTY_NAME(i) << "] " << *PROPERTY(i);
            }
        }


    };

    Formatter& operator<<(Formatter& f, const JSObject& obj);
    

    // had to be after JSObject defn.
    inline JSType *JSValue::getType() const {
        switch (tag) {
        case f64_tag: return Number_Type;
        case boolean_tag: return Boolean_Type;
        case string_tag: return (JSType *)String_Type;
        case object_tag: return object->getType();
        case undefined_tag: return Void_Type;
        case null_tag: return Object_Type;
        case function_tag: return Function_Type;
        default: NOT_REACHED("bad type"); return NULL;
        }
    }



    


        
    
    
    
    
    

    
 


    class JSInstance : public JSObject {
    public:
        
        JSInstance(Context *cx, JSType *type) 
            : JSObject(type), mInstanceValues(NULL) { if (type) initInstance(cx, type); }

        virtual ~JSInstance() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSInstance", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSInstance", t); STD::free(t); }
#endif

        void initInstance(Context *cx, JSType *type);

        void getProperty(Context *cx, const String &name, AttributeList *attr);
        void setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);

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
                    mIsDynamic(false),
                    mUninitializedValue(kNullValue)
        {
            for (uint32 i = 0; i < OperatorCount; i++)
                mUnaryOperators[i] = NULL;
            mStatics = new JSType(cx, this);
        }

        JSType(Context *cx, JSType *xClass)     // used for constructing the static component type
            : JSInstance(cx, Type_Type),
                    mSuperType(xClass), 
                    mStatics(NULL), 
                    mVariableCount(0),
                    mInitialInstance(NULL),
                    mIsDynamic(false),
                    mUninitializedValue(kNullValue)
        {
            for (uint32 i = 0; i < OperatorCount; i++)
                mUnaryOperators[i] = NULL;
        }

        virtual ~JSType() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSType", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSType", t); STD::free(t); }
#endif

        void setStaticInitializer(Context *cx, JSFunction *f);
        void setInstanceInitializer(Context *cx, JSFunction *f);



        // construct a new (empty) instance of this class
        virtual JSInstance *newInstance(Context *cx);
    
        
        // static helpers

        void defineConstructor(const String& name, AttributeList *attr, JSFunction *f)
        {
            ASSERT(mStatics);
            uint32 vTableIndex = mStatics->mMethods.size();
            mStatics->mMethods.push_back(f);

            const PropertyMap::value_type e(name, new AttributedProperty(new Property(vTableIndex, Function_Type, Constructor), attr));
            mStatics->mProperties.insert(e);
        }

        void defineStaticMethod(const String& name, AttributeList *attr, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineMethod(name, attr, f);
        }

        Property *defineStaticVariable(const String& name, AttributeList *attr, JSValue v)
        {
            ASSERT(mStatics);
            return mStatics->defineVariable(name, attr, Function_Type, v);
        }

        Property *defineStaticVariable(const String& name, AttributeList *attr, JSType *type)
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
        
        void defineStaticGetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineGetterMethod(name, attr, f);
        }
        
        void defineStaticSetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            ASSERT(mStatics);
            mStatics->defineSetterMethod(name, attr, f);
        }

        //


        Property *defineVariable(const String& name, AttributeList *attr, JSType *type)
        {
            Property *prop = new Property(mVariableCount++, type, Slot);
            const PropertyMap::value_type e(name, new AttributedProperty(prop, attr));
            mProperties.insert(e);
            return prop;
        }

        Property *defineVariable(const String& name, AttributeList *attr, JSType *type, JSValue v)
        {   // XXX why doesn't the virtual function in JSObject get found?
            return JSObject::defineVariable(name, attr, type, v);
        }

        void defineMethod(const String& name, AttributeList *attr, JSFunction *f);
        
        void defineGetterMethod(const String &name, AttributeList *attr, JSFunction *f);

        void defineSetterMethod(const String &name, AttributeList *attr, JSFunction *f);

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
            defineConstructor(mClassName, NULL, f);    // XXX attr?
        }

        void addMethod(const String &name, AttributeList *attr, JSFunction *f);
        void addStaticMethod(const String &name, AttributeList *attr, JSFunction *f);

        // return true if 'other' is on the chain of supertypes
        bool derivesFrom(JSType *other);

        virtual JSValue getPropertyValue(PropertyIterator &i);

        virtual bool hasProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p);

        virtual Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth);

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

        JSValue getUninitializedValue()    { return mUninitializedValue; }

        // assumes that the super types have been completed already
        void completeClass(Context *cx, ScopeChain *scopeChain, JSType *super);

        virtual bool isDynamic() { return mIsDynamic; }

        JSType          *mSuperType;        // NULL implies that this is the base Object
        JSType          *mStatics;          // NULL implies that this is the static component

        uint32          mVariableCount;
        JSInstance      *mInitialInstance;
    
        // the 'vtable'
        MethodList      mMethods;
        String          mClassName;

        JSFunction      *mUnaryOperators[OperatorCount];    // XXX too wasteful

        bool            mIsDynamic;
        JSValue         mUninitializedValue;            // the value for uninitialized vars

        void printSlotsNStuff(Formatter& f) const;

    };

    Formatter& operator<<(Formatter& f, const JSType& obj);

    class JSArrayInstance : public JSInstance {
    public:
        JSArrayInstance(Context *cx, JSType * /*type*/) : JSInstance(cx, NULL), mLength(0) { mType = (JSType *)Array_Type; }
        virtual ~JSArrayInstance() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSArrayInstance", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSArrayInstance", t); STD::free(t); }
#endif

        // XXX maybe could have implemented length as a getter/setter pair?
        void setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v);
        void getProperty(Context *cx, const String &name, AttributeList *attr);


        uint32 mLength;

    };

    class JSArrayType : public JSType {
    public:
        JSArrayType(Context *cx, const String &name, JSType *super) 
            : JSType(cx, name, super)
        {
        }
        virtual ~JSArrayType() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSArrayType", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSArrayType", t); STD::free(t); }
#endif

        JSInstance *newInstance(Context *cx);

    };

    class JSStringInstance : public JSInstance {
    public:
        JSStringInstance(Context *cx, JSType * /*type*/) : JSInstance(cx, NULL), mLength(0) { mType = (JSType *)String_Type; }
        virtual ~JSStringInstance() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSStringInstance", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSStringInstance", t); STD::free(t); }
#endif

        void getProperty(Context *cx, const String &name, AttributeList *attr);


        uint32 mLength;

    };

    class JSStringType : public JSType {
    public:
        JSStringType(Context *cx, const String &name, JSType *super) 
            : JSType(cx, name, super)
        {
        }
        virtual ~JSStringType() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSStringType", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSStringType", t); STD::free(t); }
#endif

        JSInstance *newInstance(Context *cx);

    };





    // captures the Parameter names scope
    class ParameterBarrel : public JSType {
    public:

        ParameterBarrel(Context *cx) : JSType(cx, NULL) 
        {
        }
        virtual ~ParameterBarrel() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("ParameterBarrel", s, t); return t; }
        void operator delete(void* t)   { trace_release("ParameterBarrel", t); STD::free(t); }
#endif

        Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 /*depth*/)
        {
            PropertyIterator i;
            if (hasProperty(name, attr, acc, &i)) {
                Property *prop = PROPERTY(i);
                ASSERT(prop->mFlag == Slot);
                return new ParameterReference(prop->mData.index, acc, prop->mType);
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
                        mModule(NULL)  { }

        Activation(Context *cx, JSValue *locals, 
                        JSValue *stack, uint32 stackTop,
                        ScopeChain *scopeChain,
                        JSValue *argBase, JSValue curThis,
                        uint8 *pc, 
                        ByteCodeModule *module )
                    : JSType(cx, NULL), 
                        mLocals(locals), 
                        mStack(stack), 
                        mStackTop(stackTop),
                        mScopeChain(scopeChain),
                        mArgumentBase(argBase), 
                        mThis(curThis), 
                        mPC(pc), 
                        mModule(module)  { }

        virtual ~Activation() { } // keeping gcc happy

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("Activation", s, t); return t; }
        void operator delete(void* t)   { trace_release("Activation", t); STD::free(t); }
#endif

        
        // saved values from a previous execution
        JSValue *mLocals;
        JSValue *mStack;
        uint32 mStackTop;           
        ScopeChain *mScopeChain;
        JSValue *mArgumentBase;
        JSValue mThis;
        uint8 *mPC;
        ByteCodeModule *mModule;


        bool hasLocalVars()             { return true; }
        virtual uint32 localVarCount()  { return mVariableCount; }

        void defineTempVariable(Reference *&readRef, Reference *&writeRef, JSType *type);

        Reference *genReference(const String& name, AttributeList *attr, Access acc, uint32 depth);

    };


    
    
    class ScopeChain {
    public:

        ScopeChain(Context *cx, World &) :
              m_cx(cx)
        {
        }

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("ScopeChain", s, t); return t; }
        void operator delete(void* t)   { trace_release("ScopeChain", t); STD::free(t); }
#endif

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
        Property *defineVariable(const String& name, AttributeList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type);
        }
        Property *defineVariable(const String& name, AttributeList *attr, JSType *type, JSValue v)
        {
            JSObject *top = mScopeStack.back();
            return top->defineVariable(name, attr, type, v);
        }
        Property *defineStaticVariable(const String& name, AttributeList *attr, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            return top->defineStaticVariable(name, attr, type);
        }
        void defineMethod(const String& name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineMethod(name, attr, f);
        }   
        void defineStaticMethod(const String& name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticMethod(name, attr, f);
        }   
        void defineConstructor(const String& name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineConstructor(name, attr, f);
        }   
        void defineGetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineGetterMethod(name, attr, f);
        }
        void defineSetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            top->defineSetterMethod(name, attr, f);
        }
        void defineStaticGetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticGetterMethod(name, attr, f);
        }
        void defineStaticSetterMethod(const String &name, AttributeList *attr, JSFunction *f)
        {
            JSObject *top = mScopeStack.back();
            ASSERT(dynamic_cast<JSType *>(top));
            top->defineStaticSetterMethod(name, attr, f);
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

        void getNameValue(const String& name, AttributeList *attr, Context *cx);

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



        void setNameValue(const String& name, AttributeList *attr, Context *cx);

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
        JSType *findType(const StringAtom& typeName, size_t pos);

        // Get a type from an ExprNode 
        JSType *extractType(ExprNode *t);


    };


    class JSFunction : public JSObject {
    protected:
        JSFunction() : JSObject(Function_Type), mActivation(NULL) { mPrototype = Function_Type->mPrototype; }        // for JSBoundFunction (XXX ask Patrick about this structure)
    public:
        typedef JSValue (NativeCode)(Context *cx, const JSValue &thisValue, JSValue argv[], uint32 argc);

        JSFunction(Context *cx, JSType *resultType, uint32 argCount, ScopeChain *scopeChain) 
                    : JSObject(Function_Type), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mByteCode(NULL), 
                        mCode(NULL), 
                        mResultType(resultType), 
                        mExpectedArgs(argCount),
                        mScopeChain(NULL), 
                        mIsPrototype(false),
                        mClass(NULL)
        {
            if (scopeChain) {
                mScopeChain = new ScopeChain(*scopeChain);
            }
            mPrototype = Function_Type->mPrototype;
        }
        
        JSFunction(Context *cx, NativeCode *code, JSType *resultType) 
                    : JSObject(Function_Type), 
                        mParameterBarrel(NULL),
                        mActivation(cx),
                        mByteCode(NULL), 
                        mCode(code), 
                        mResultType(resultType), 
                        mExpectedArgs(0),
                        mScopeChain(NULL),
                        mIsPrototype(false),
                        mClass(NULL)
        {
            mPrototype = Function_Type->mPrototype;
        }

        ~JSFunction() { }  // keeping gcc happy
        
#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("JSFunction", s, t); return t; }
        void operator delete(void* t)   { trace_release("JSFunction", t); STD::free(t); }
#endif

        void setByteCode(ByteCodeModule *b)     { ASSERT(!isNative()); mByteCode = b; }
        void setResultType(JSType *r)           { mResultType = r; }
        void setExpectedArgs(uint32 e)          { mExpectedArgs = e; }
        void setIsPrototype(bool i)             { mIsPrototype = i; }
        void setFunctionName(const String &n)   { mFunctionName = n; }
        void setClass(JSType *c)                { mClass = c; }

        virtual bool hasBoundThis()             { return false; }
        virtual bool isNative()                 { return (mCode != NULL); }
        virtual bool isPrototype()              { return mIsPrototype; }
        virtual ByteCodeModule *getByteCode()   { ASSERT(!isNative()); return mByteCode; }
        virtual NativeCode *getNativeCode()     { ASSERT(isNative()); return mCode; }

        virtual JSType *getResultType()         { return mResultType; }
        virtual uint32 getExpectedArgs()        { return mExpectedArgs; }
        virtual ScopeChain *getScopeChain()     { return mScopeChain; }
        virtual JSValue getThisValue()          { return kNullValue; }         
        virtual JSType *getClass()              { return mClass; }
        virtual String &getFunctionName()       { return mFunctionName; }

        ParameterBarrel *mParameterBarrel;
        Activation mActivation;                 // not used during execution  (XXX so maybe we should handle it differently, hmmm?)

    private:
        ByteCodeModule *mByteCode;
        NativeCode *mCode;
        JSType *mResultType;
        uint32 mExpectedArgs;
        ScopeChain *mScopeChain;
        bool mIsPrototype;                      // set for functions with prototype attribute
        JSType *mClass;                         // pointer to owning class if this function is a method
        String mFunctionName;
    };

    class JSBoundFunction : public JSFunction {
    private:
        JSFunction *mFunction;
        JSObject *mThis;
    public:
        JSBoundFunction(JSFunction *f, JSObject *thisObj)
            : mFunction(f), mThis(thisObj) { }

        ~JSBoundFunction() { }  // keeping gcc happy

        bool hasBoundThis()             { return true; }
        bool isNative()                 { return mFunction->isNative(); }
        bool isPrototype()              { return mFunction->isPrototype(); }
        ByteCodeModule *getByteCode()   { return mFunction->getByteCode(); }
        NativeCode *getNativeCode()     { return mFunction->getNativeCode(); }

        JSType *getResultType()         { return mFunction->getResultType(); }
        uint32 getExpectedArgs()        { return mFunction->getExpectedArgs(); }
        ScopeChain *getScopeChain()     { return mFunction->getScopeChain(); }
        JSValue getThisValue()          { return JSValue(mThis); }         
        JSType *getClass()              { return mFunction->getClass(); }
        String &getFunctionName()       { return mFunction->getFunctionName(); }

        void getProperty(Context *cx, const String &name, AttributeList *attr) 
                                        { mFunction->getProperty(cx, name, attr); }
        void setProperty(Context *cx, const String &name, AttributeList *attr, const JSValue &v)
                                        { mFunction->setProperty(cx, name, attr, v); }
        bool hasProperty(const String &name, AttributeList *attr, Access acc, PropertyIterator *p)
                                        { return mFunction->hasProperty(name, attr, acc, p); }

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
            const JSValue *uninit;
        };

        Context(JSObject **global, World &world, Arena &a);

#ifdef DEBUG
        void* operator new(size_t s)    { void *t = STD::malloc(s); trace_alloc("Context", s, t); return t; }
        void operator delete(void* t)   { trace_release("Context", t); STD::free(t); }
#endif

        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 
        StringAtom& FixedKeyWord;
        StringAtom& DynamicKeyWord;
        StringAtom& ExtendKeyWord;
        StringAtom& PrototypeKeyWord;

        void initBuiltins();
        void initClass(JSType *type, JSType *super, ClassDef *cdef, PrototypeFunctions *pdef);
        void initOperators();
        
        void defineOperator(Operator which, JSType *t1, JSType *t2, JSFunction *imp)
        {
            OperatorDefinition *op = new OperatorDefinition(t1, t2, imp);
            mOperatorTable[which].push_back(op);
        }

        JSFunction *getOperator(Operator which, JSType *t1, JSType *t2);

        bool executeOperator(Operator op, JSType *t1, JSType *t2);

        JSValue invokeFunction(JSFunction *target, const JSValue& thisValue, JSValue *argv, uint32 argc);

        void setReader(Reader *r)           { mReader = r; }

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

        // the locals for the current function (an array, constructed on function entry)
        JSValue *mLocals;

        // the base of the incoming arguments for this function
        JSValue *mArgumentBase;

        typedef std::vector<OperatorDefinition *> OperatorList;
            
        OperatorList mOperatorTable[OperatorCount];
        

        JSValue readEvalFile(const String& fileName);

        void buildRuntime(StmtNode *p);
        void buildRuntimeForFunction(FunctionDefinition &f, JSFunction *fnc);
        void buildRuntimeForStmt(StmtNode *p);
        ByteCodeModule *genCode(StmtNode *p, String sourceName);

        JSValue interpret(ByteCodeModule *bcm, ScopeChain *scopeChain, const JSValue& thisValue, JSValue *argv, uint32 argc);
        JSValue interpret(uint8 *pc, uint8 *endPC);
        
        void reportError(Exception::Kind kind, char *message, size_t pos);
        void reportError(Exception::Kind kind, char *message);

        
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
        Reader *mReader;


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
        defineStaticMethod(name, attr, f);
    }

    inline void JSType::addMethod(const String &name, AttributeList *attr, JSFunction *f)
    {
        defineMethod(name, attr, f);
    }


    inline bool JSInstance::isDynamic() { return mType->isDynamic(); }

}
}

#endif //js2runtime_h___
