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
            f64_tag,
            object_tag,
            function_tag,
            type_tag,
            slot_tag,
            boolean_tag,
            undefined_tag, 
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
    typedef std::map<String, JSValue, std::less<String> > ValueMap;
    
    
    
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
    typedef JSValue (NativeCode)(JSValueList);
    public:
        JSFunction(NativeCode *code) : mCode(code) { }
        NativeCode *mCode;
    };
    
    


    
    
#define PROPERTY_VALUE(it) (it->second)
#define PROPERTY_NAME(it) (it->first)
        
    class JSObject {
    public:
    // The generic Javascript object. Every JS2 object is one of these
        JSType        *mType;
        ValueMap      mProperties;    // simple map from <name> to <JSValue>

        JSObject(JSType *type = Object_Type) : mType(type) { }


        JSType *getType() { return mType; }
        
        void setProperty(const String &name, JSValue v)       
        {
            mProperties[name] = v;
        }
        
        bool hasProperty(const String &name)
        {
            return (mProperties.find(name) != mProperties.end());
        }

        JSValue getProperty(const String &name)
        { 
            ValueMap::iterator i = mProperties.find(name);
            if (i == mProperties.end())
                return kUndefinedValue;
            else
                return PROPERTY_VALUE(i);
        }
        
        void printProperties(Formatter &f) const
        {
            for (ValueMap::const_iterator i = mProperties.begin(), end = mProperties.end(); (i != end); i++) 
            {
                f << "[" << PROPERTY_NAME(i) << "] " << PROPERTY_VALUE(i) << "\n";
            }
        }


    };

    Formatter& operator<<(Formatter& f, const JSObject& obj);
    



    inline JSType *JSValue::getType() {
        switch (tag) {
        case f64_tag: return Number_Type;
        case object_tag: return object->getType();
        default: NOT_REACHED("bad type"); return NULL;
        }
    }




    
    
    
    class Slot {
    public:
        Slot() : mIndex(-1), mAccessor(NULL) { }
        Slot(JSFunction *f, uint32 i) : mIndex(i), mAccessor(f) { }
        uint32          mIndex;              // default getters & setters use this to address the mInstanceValues array
        JSFunction      *mAccessor;
    };
    Formatter& operator<<(Formatter& f, const Slot& slot);
    
    typedef enum { Read, Write } Access;
    Formatter& operator<<(Formatter& f, const Access& acc);

    typedef std::pair<String, Access> NameAccessPair;
    typedef std::map<NameAccessPair, Slot *, std::less<const NameAccessPair> > SlotMap;

#define SLOT_NAME(it) (it->first.first)
#define SLOT_ACCESS(it) (it->first.second)
#define SLOT(it) (it->second)
    
    
    
    
    
    
    
    
    

    
 
    class Reference {
    public:
        virtual void emitCodeSequence() { }
    };

    class AccessorReference : public Reference {
    public:
        AccessorReference(JSFunction *f) : mFunction(f) { }
        JSFunction *mFunction;
        void emitCodeSequence(ByteCodeGen *bcg);
    };
    class LocalVarReference : public Reference {
    public:
        LocalVarReference(uint32 index, Access acc) : mAccess(acc), mIndex(index) { }
        Access mAccess;
        uint32 mIndex;
        void emitCodeSequence(ByteCodeGen *bcg);
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
    class NameReference : public Reference {
    public:
        NameReference(const String& name, Access acc) : mAccess(acc), mName(name) { }
        Access mAccess;
        const String& mName;
        void emitCodeSequence(ByteCodeGen *bcg);

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

        JSObject *newInstance();
    
        
        // static helpers

        void defineStaticMethod(const String& name, JSFunction *f, JSFunction *methodGetter)
        {
            mStatics->defineMethod(name, f, methodGetter);
        }

        void defineStaticVariable(const String& name, 
                                    JSType *type, 
                                    JSFunction *defaultGetter, 
                                    JSFunction *defaultSetter)
        {
            mStatics->defineVariable(name, type, defaultGetter, defaultSetter);
        }

        bool hasStatic(const String& name, Access acc)
        {
            return mStatics->hasName(name, acc);
        }

        Slot *getStatic(const String& name, Access acc)
        {
            return mStatics->getName(name, acc);
        }

        //

        void defineConstructor(const String& name, JSFunction *f, JSFunction *methodGetter)
        {
            defineMethod(name, f, methodGetter);
        }

        void defineMethod(const String& name, JSFunction *f, JSFunction *methodGetter)
        {
            NameAccessPair nap(name, Read);
            uint32 index = mMethods.size();
            mMethods.push_back(f);
            mSlotMap[nap] = new Slot(methodGetter, index);
        }


        void defineVariable(const String& name, 
                                    JSType *type, 
                                    JSFunction *defaultGetter, 
                                    JSFunction *defaultSetter)
        {
            NameAccessPair read_nap(name, Read); 
            mSlotMap[read_nap] = new Slot(defaultGetter, mVariableCount);

            NameAccessPair write_nap(name, Write);
            mSlotMap[write_nap] = new Slot(defaultSetter, mVariableCount);
            ++mVariableCount;
        }

        
        Slot *getName(const String& name, Access acc)
        {
            NameAccessPair nap(name, acc);
            ASSERT(mSlotMap.find(nap) != mSlotMap.end());
            return mSlotMap[nap];
        }

        bool hasName(const String& name, Access acc)
        {
            NameAccessPair nap(name, acc);
            return (mSlotMap.find(nap) != mSlotMap.end());
        }

        virtual Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Slot *slot = getName(name, acc);
            ASSERT(slot);
            if (slot->mAccessor) 
                return new MethodReference(slot->mIndex);

            return new FieldReference(slot->mIndex, acc);
        }


        JSObject        *mSuperType;

        uint32          mVariableCount;
        JSType          *mStatics;          // or null if this is the static component

        SlotMap         mSlotMap;           // maps <name>&<access> to slot
        MethodList      mMethods;
        
        void printSlotsNStuff(Formatter& f) const
        {
            f << "var. count = " << mVariableCount << "\n";
            f << "method count = " << (uint32)(mMethods.size()) << "\n";

            for (SlotMap::const_iterator i = mSlotMap.begin(), end = mSlotMap.end(); (i != end); i++)
            {
                f << SLOT_NAME(i) << " " 
                    << SLOT_ACCESS(i) << " = " 
                    << *SLOT(i);
            }
        }

    };
    Formatter& operator<<(Formatter& f, const JSType& obj);

    
    









    class JSInstance : public JSObject {
    public:
        JSInstance(JSType *type) : JSObject(type), mInstanceValues(NULL)
        {
            if (mType->mVariableCount)
                mInstanceValues = new JSValue[mType->mVariableCount];
        }

        JSValue         *mInstanceValues;
    };

    inline JSObject *JSType::newInstance()
    {
        return new JSInstance(this);
    }








    class Activation : public JSType {
    public:
        Reference *genReference(const String& name, Access acc, uint32 depth)
        {
            Slot *slot = getName(name, acc);
            ASSERT(slot);
            if (slot->mAccessor) 
                return new AccessorReference(slot->mAccessor);

            if (depth)
                return new ClosureVarReference(depth, slot->mIndex, acc);

            return new LocalVarReference(slot->mIndex, acc);
        }
    };








    class ScopeChain {
    public:

        std::vector<JSType *> mScopeStack;
        typedef std::vector<JSType *>::reverse_iterator ScopeScanner;


        void addScope(JSType *s)
        {
            mScopeStack.push_back(s);
        }

        void popScope()
        {
            mScopeStack.pop_back();
        }

        void defineVariable(const String& name, 
                                    JSType *type, 
                                    JSFunction *defaultGetter, 
                                    JSFunction *defaultSetter)
        {
            JSType *top = mScopeStack.back();
            top->defineVariable(name, type, defaultGetter, defaultSetter);
        }

        Reference *getName(const String& name, Access acc)
        {
            uint32 depth = 0;
            for (ScopeScanner s = mScopeStack.rbegin(), end = mScopeStack.rend(); (s != end); s++, depth++)
            {
                if ((*s)->hasName(name, acc)) {
                    return (*s)->genReference(name, acc, depth);
                }
            }
            return new NameReference(name, acc);
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
        }
        
        void defineOperator(Operator op, JSType *t1, JSType *t2, JSFunction *imp)
        {
        }
        bool executeOperator(Operator op, JSType *t1, JSType *t2);

        JSObject *mGlobal;
        World &mWorld;

        StringAtom& VirtualKeyWord; 
        StringAtom& ConstructorKeyWord; 
        StringAtom& OperatorKeyWord; 

        
        JSObject *getGlobalObject() { return mGlobal; }
        World &getWorld()           { return mWorld; }


        void buildRuntime(StmtNode *p);
        void buildRuntimeForStmt(StmtNode *p);

        
        ByteCodeModule *genCode(StmtNode *p, String sourceName);
        JSValue interpret(ByteCodeModule *bcm, JSValueList args);


        // the default accessors used to access instance (& static) variables & methods
        static JSFunction *defaultGetter;
        static JSFunction *defaultSetter;
        static JSFunction *methodGetter;
        
        
        /* utility routines */

        // Extract the operator from the string literal function name
        // - requires the paramter count in order to distinguish
        // between unary and binary operators.
        Operator getOperator(uint32 parameterCount, String &name);

        // Get the type of the nth parameter.
        JSType *getParameterType(FunctionDefinition &function, int index);

        // Get the number of parameters.
        uint32 getParameterCount(FunctionDefinition &function);

        // Lookup a name as a type in the global object
        JSType *findType(const StringAtom& typeName);

        // Get a type from an ExprNode 
        JSType *extractType(ExprNode *t);

        
        
        

    };

}
}

#endif //js2runtime_h___