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
    class Slot;


    extern JSType *Object_Type;         // the base type for all types
    extern JSType *Number_Type;


    class JSValue {
    public:
        union {
            float64 f64;
            JSObject *object;
            JSFunction *function;
            JSType *type;
            bool boolean;
            Slot *slot;
        };
        
        typedef enum {
            undefined_tag, 
            f64_tag,
            object_tag,
            function_tag,
            type_tag,
            slot_tag,
            boolean_tag,
            null_tag,
        } Tag;
        Tag tag;
        
        JSValue() : f64(0.0), tag(undefined_tag) {}
        explicit JSValue(float64 f64) : f64(f64), tag(f64_tag) {}
        explicit JSValue(JSObject *object) : object(object), tag(object_tag) {}
        explicit JSValue(JSFunction *function) : function(function), tag(function_tag) {}
        explicit JSValue(JSType *type) : type(type), tag(type_tag) {}
        explicit JSValue(Slot *slot) : slot(slot), tag(slot_tag) {}
        explicit JSValue(bool boolean) : boolean(boolean), tag(boolean_tag) {}
        explicit JSValue(Tag tag) : tag(tag) {}

        float64& operator=(float64 f64)                 { return (tag = f64_tag, this->f64 = f64); }
        JSObject*& operator=(JSObject* object)          { return (tag = object_tag, this->object = object); }
        JSType*& operator=(JSType* type)                { return (tag = type_tag, this->type = type); }
        Slot*& operator=(Slot* slot)                    { return (tag = slot_tag, this->slot = slot); }
        JSFunction*& operator=(JSFunction* slot)        { return (tag = function_tag, this->function = function); }
        bool& operator=(bool boolean)                   { return (tag = boolean_tag, this->boolean = boolean); }
        
        bool isObject() const                           { return (tag == object_tag); }
        bool isNumber() const                           { return (tag == f64_tag); }
        bool isBool() const                             { return (tag == boolean_tag); }
        bool isType() const                             { return (tag == type_tag); }
        bool isSlot() const                             { return (tag == slot_tag); }
        bool isFunction() const                         { return (tag == function_tag); }

        bool isUndefined() const                        { return (tag == undefined_tag); }
        bool isNull() const                             { return (tag == null_tag); }
        bool isNaN() const;
        bool isNegativeInfinity() const;
        bool isPositiveInfinity() const;
        bool isNegativeZero() const;
        bool isPositiveZero() const;

        JSType *getType();

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
    
    
    
    class JSFunction {
    typedef JSValue (NativeCode)(JSValue *argv, uint32 argc);
    public:
        JSFunction(ByteCodeModule *bcm) : mByteCode(bcm), mCode(NULL) { }
        JSFunction(NativeCode *code) : mByteCode(NULL), mCode(code) { }

        bool isNative() { return (mCode != NULL); }

        ByteCodeModule *mByteCode;
        NativeCode *mCode;
    };
    





    class Context;

    
    
    
    class Reference {
    public:
        virtual void emitCodeSequence(ByteCodeGen *bcg) 
                { throw Exception(Exception::runtimeError, "gen code for base ref"); }
        virtual JSValue getValue()
                { throw Exception(Exception::runtimeError, "get value for base ref"); }
        virtual bool getValue(Context *cx)
                { throw Exception(Exception::runtimeError, "get value(cx) for base ref"); }
        virtual bool setValue(Context *cx)
                { throw Exception(Exception::runtimeError, "set value for base ref"); }
    };

    class AccessorReference : public Reference {
    public:
        AccessorReference(JSFunction *f) : mFunction(f) { }
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx);
        bool setValue(Context *cx);
    };
    class LocalVarReference : public Reference {
    public:
        LocalVarReference(uint32 index, Access acc) : mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
        bool getValue(Context *cx);
        bool setValue(Context *cx);
    };
    class ClosureVarReference : public LocalVarReference {
    public:
        ClosureVarReference(uint32 depth, uint32 index, Access acc) 
                        : LocalVarReference(index, acc), mDepth(depth) { }
        uint32 mDepth;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class FieldReference : public Reference {
    public:
        FieldReference(uint32 index, Access acc) : mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class MethodReference : public Reference {
    public:
        MethodReference(uint32 index) : mIndex(index) { }
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class FunctionReference : public Reference {
    public:
        FunctionReference(JSFunction *f) : mFunction(f) { }
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class ValueReference : public Reference {
    public:
        ValueReference(JSObject *base, const String& name, Access acc) 
            : mBase(base), mName(name), mAccess(acc) { }
        JSObject *mBase;
        const String& mName;
        Access mAccess;
        void emitCodeSequence(ByteCodeGen *bcg);
        virtual bool getValue(Context *cx);
        virtual bool setValue(Context *cx);
        virtual JSValue getValue();
    };
    class ParameterReference : public Reference {
    public:
        ParameterReference(uint32 index, Access acc) : mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
    };

    // the generic "we don't know anybody by that name"
    class NameReference : public Reference {
    public:
        NameReference(const String& name, Access acc) : mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);
        virtual bool getValue(Context *cx)
                { throw Exception(Exception::runtimeError, "get value for name ref"); }
        virtual bool setValue(Context *cx)
                { throw Exception(Exception::runtimeError, "set value for name ref"); }
    };




    
    
    
    
    
    
    typedef enum { ValuePointer, FunctionPair, IndexPair, Slot, Method } PropertyFlag;    
    
    class Property {
    public:
        Property() { }

        Property(uint32 g, uint32 s, JSType *type)
            : mType(type), mFlag(IndexPair) { mData.iPair.getIndex = g;  mData.iPair.setIndex = s; }

        Property(uint32 i, JSType *type, PropertyFlag flag) 
            : mType(type), mFlag(flag) { mData.index = i; }

        Property(JSValue *p, JSType *type) 
            : mType(type), mFlag(ValuePointer) { mData.vp = p; }
        
        
        union {
            JSValue *vp;
            struct {
                JSFunction *getF;
                JSFunction *setF;
            } fPair;
            struct {
                uint32 getIndex;
                uint32 setIndex;
            } iPair;
            uint32 index;
        } mData;
        JSType *mType;
        PropertyFlag mFlag;
    };
    Formatter& operator<<(Formatter& f, const Property& prop);
   

    typedef std::map<String, Property, std::less<const String> > PropertyMap;

    
#define PROPERTY_KIND(it)           (it->second.mFlag)
#define PROPERTY(it)                (it->second)
#define PROPERTY_VALUEPOINTER(it)   (it->second.mData.vp)
#define PROPERTY_NAME(it)           (it->first)
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSObject(JSType *type = Object_Type) : mType(type) { }

        
        // every object has a type
        JSType        *mType;

        // the property data is kept (or referenced from) here
        PropertyMap   mProperties;


        JSType *getType() { return mType; }

        // see if the property exists in any form
        bool hasProperty(const String &name)
        {
            PropertyMap::iterator i = mProperties.find(name);
            return (i != mProperties.end());
        }

        // see if the property exists by a specific kind of access
        bool hasProperty(const String &name, Access acc)
        {
            PropertyMap::iterator i = mProperties.find(name);
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

        // add a property
        virtual void defineVariable(const String &name, JSType *type)
        {
            mProperties[name] = Property(new JSValue(), type);
        }

        // add a property (with a value)
        virtual void defineVariable(const String &name, JSType *type, JSValue v)
        {
            mProperties[name] = Property(new JSValue(v), type);
        }

        // get all the goods on a specific property
        Property &getPropertyData(const String &name)
        {
            PropertyMap::iterator i = mProperties.find(name);
            return PROPERTY(i);
        }

        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            switch (prop.mFlag) {
            case ValuePointer:
                return new ValueReference(this, name, acc);
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
                f << "[" << PROPERTY_NAME(i) << "] " << PROPERTY(i) << "\n";
            }
        }


    };

    Formatter& operator<<(Formatter& f, const JSObject& obj);
    

    // had to be after JSObject defn.
    inline JSType *JSValue::getType() {
        switch (tag) {
        case f64_tag: return Number_Type;
        case object_tag: return object->getType();
        default: NOT_REACHED("bad type"); return NULL;
        }
    }




    
        
    
    
    
    
    

    
 


    class JSInstance : public JSObject {
    public:
        JSInstance(JSType *type);

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

    
    typedef std::vector<JSFunction *> MethodList;

    class JSType : public JSObject {
    public:
        
        JSType(JSObject *super) : mSuperType(super), 
                                    mStatics(NULL), 
                                    mVariableCount(0)
        {
        }

        void createStaticComponent()
        {
            mStatics = new JSType(NULL);
        }

        JSInstance *newInstance();
    
        
        // static helpers

        void defineStaticMethod(const String& name, JSFunction *f, JSType *type)
        {
            mStatics->defineMethod(name, f, type);
        }

        void defineStaticVariable(const String& name, JSType *type)
        {
            mStatics->defineVariable(name, type);
        }

        bool hasStatic(const String& name, Access acc)
        {
            return mStatics->hasProperty(name, acc);
        }

        //


        void defineVariable(const String& name, JSType *type)
        {
            mProperties[name] = Property(mVariableCount++, type, Slot);
        }

        void defineMethod(const String& name, JSFunction *f, JSType *type)
        {
            uint32 vTableIndex = mMethods.size();
            mMethods.push_back(f);
            mProperties[name] = Property(vTableIndex, type, Method);
        }

        void defineConstructor(const String& name, JSFunction *f, JSType *type)
        {
            defineMethod(name, f, type);
        }





        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            switch (prop.mFlag) {
            case FunctionPair:
                if (acc == Read)
                    return new FunctionReference(prop.mData.fPair.getF);
                else
                    return new FunctionReference(prop.mData.fPair.setF);
            case IndexPair:
                if (acc == Read)
                    return new MethodReference(prop.mData.iPair.getIndex);
                else
                    return new MethodReference(prop.mData.iPair.setIndex);
            case Slot:
                return new FieldReference(prop.mData.index, acc);
            case Method:
                return new MethodReference(prop.mData.index);
            default:
                NOT_REACHED("bad storage kind");
                return NULL;
            }
        }


        JSObject        *mSuperType;

        uint32          mVariableCount;
        JSType          *mStatics;          // or null if this is the static component

        // the 'vtable'
        MethodList      mMethods;
        
        void printSlotsNStuff(Formatter& f) const
        {
            f << "var. count = " << mVariableCount << "\n";
            f << "method count = " << (uint32)(mMethods.size()) << "\n";

            JSObject::printProperties(f);
        }

    };

    Formatter& operator<<(Formatter& f, const JSType& obj);

    
    









    inline JSInstance::JSInstance(JSType *type)
                : JSObject(type), mInstanceValues(NULL)
    {
        if (mType->mVariableCount)
            mInstanceValues = new JSValue[mType->mVariableCount];
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
                return new ClosureVarReference(depth, prop.mData.index, acc);

            return new LocalVarReference(prop.mData.index, acc);
        }

    };


    class ParameterBarrel : public JSType {
    public:

        ParameterBarrel() : JSType(NULL) { }

        Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Property &prop = getPropertyData(name);
            ASSERT(prop.mFlag == Slot);
            return new ParameterReference(prop.mData.index, acc);
        }

    };


    class ScopeChain {
    public:

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
        void defineVariable(const String& name, JSType *type)
        {
            JSObject *top = mScopeStack.back();
            top->defineVariable(name, type);
        }   
        
        // see if the current scope contains a name already
        bool hasProperty(const String& name)
        {
            JSObject *top = mScopeStack.back();
            return top->hasProperty(name);
        }

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
        JSValue getValue(const String& name)
        {
            Reference *ref = getName(name, Read);
            return ref->getValue();
        }


        // a runtime request to get the value for a name
        // it'll either return a value or cause the
        // interpreter to switch execution to a getter function
        bool getNameValue(const String& name, Context *cx)
        {
            Reference *ref = getName(name, Read);
            return ref->getValue(cx);
        }

        bool setNameValue(const String& name, Context *cx)
        {
            JSObject *top = *mScopeStack.rbegin();
            Reference *ref = NULL;
            if (top->hasProperty(name, Write))
                ref = top->genReference(name, Write, 0);
            else {
                // then define the property in the top scope
                ref = new ValueReference(top, name, Write); // XXX Optimize this by defining the variable
                                                            // and setting it's value without building a reference
                                                            // (and re-visit the whole issue of constructing
                                                            // references at runtime anyway [whose deleting these?])
                top->defineVariable(name, Object_Type);
            }
            return ref->setValue(cx);
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


    class OperatorDefinition {
    public:
        
        OperatorDefinition(JSType *type1, JSType *type2, JSFunction *imp)
            : mType1(type1), mType2(type2), mImp(imp) { }


        JSType *mType1;
        JSType *mType2;
        JSFunction *mImp;

        bool isApplicable(JSType *tx, JSType *ty)
        {
            return (tx == mType1) && (ty == mType2);
        }


    };

    class Context {
    public:

        Context(JSObject *global, World &world) 
            : mGlobal(global), 
              mWorld(world), 
              VirtualKeyWord(mWorld.identifiers["virtual"]),
              ConstructorKeyWord(mWorld.identifiers["constructor"]),
              OperatorKeyWord(mWorld.identifiers["operator"])
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


        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 

        

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


}
}

#endif //js2runtime_h___