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

#ifndef bytecodegen_h___
#define bytecodegen_h___

#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#endif


#include <vector>
#include <map>

#include "systemtypes.h"
#include "strings.h"

namespace JavaScript {
namespace JS2Runtime {

    typedef enum {
        NoThis,
        Inherent,
        Explicit
    } ThisFlag;     // maybe have three different invoke ops instead?

typedef enum {

LoadConstantUndefinedOp,//                          --> <undefined value object>
LoadConstantTrueOp,     //                          --> <true value object>
LoadConstantFalseOp,    //                          --> <false value object>
LoadConstantNullOp,     //                          --> <null value object>
LoadConstantNumberOp,   // <poolindex>              --> <Number value object>
LoadConstantStringOp,   // <poolindex>              --> <String value object>
LoadThisOp,             //                          --> <this object>      
LoadFunctionOp,         // <pointer>        XXX !!! XXX
LoadTypeOp,             // <pointer>        XXX !!! XXX
InvokeOp,               // <argc> <thisflag>        <function> <args>  --> [<result>]
GetTypeOp,              //                          <object> --> <type of object>
CastOp,                 //                          <type> <object> --> <object>
DoUnaryOp,              // <operation>              <object> --> <result>
DoOperatorOp,           // <operation>              <object> <object> --> <result>
PushNullOp,             //                          --> <Object(null)>
PushIntOp,              // <int>                    --> <Object(int)>
PushNumOp,              // <num>                    --> <Object(num)>
PushStringOp,           // <poolindex>              --> <Object(index)>
PushTypeOp,             // <poolindex>
ReturnOp,               //                          <function> <args> <result> --> <result>
ReturnVoidOp,           //                          <function> <args> -->
GetConstructorOp,       //                          <type> --> <function> 
NewObjectOp,            //                          <type> --> <object>
NewThisOp,              //                          <type> -->
NewInstanceOp,          //  <argc>                  <type> <args> --> <object>
TypeOfOp,               //                          <object> --> <string>
InstanceOfOp,           //                          <object> <object> --> <boolean>
AtOp,                   //                          <object> <type> --> <object>
ToBooleanOp,            //                          <object> --> <boolean>
JumpFalseOp,            // <target>                 <object> -->
JumpTrueOp,             // <target>                 <object> -->
JumpOp,                 // <target>            
TryOp,                  // <handler> <handler>
JsrOp,                  // <target>
RtsOp,
WithinOp,               //                          <object> -->
WithoutOp,              //
ThrowOp,                //                          <whatever> <object> --> <object>
HandlerOp,
LogicalXorOp,           //                          <object> <object> <boolean> <boolean> --> <object> 
LogicalNotOp,           //                          <object> --> <object>
SwapOp,                 //                          <object1> <object2> --> <object2> <object1>
DupOp,                  //                          <object> --> <object> <object>
DupInsertOp,            //                          <object1> <object2> --> <object2> <object1> <object2>
DupNOp,                 // <N>                      <object> --> <object> { N times }
DupInsertNOp,           // <N>                      <object> {xN} <object2> --> <object2> <object> {xN} <object2>
PopOp,                  //                          <object> -->   
// for instance members
GetFieldOp,             // <slot>                   <base> --> <object>
SetFieldOp,             // <slot>                   <base> <object> --> <object>
// for static members
GetStaticFieldOp,       // <slot>                   <base> --> <object>
SetStaticFieldOp,       // <slot>                   <base> <object> --> <object>
// for instance methods
GetMethodOp,            // <slot>                   <base> --> <base> <function>
GetMethodRefOp,         // <slot>                   <base> --> <bound function> 
// for static methods
GetStaticMethodOp,      // <slot>                   <base> --> <function>
GetStaticMethodRefOp,   // <slot>                   <base> --> <bound function> 
// for argumentz
GetArgOp,               // <index>                  --> <object>
SetArgOp,               // <index>                  <object> --> <object>
// for local variables in the immediate scope
GetLocalVarOp,          // <index>                  --> <object>
SetLocalVarOp,          // <index>                  <object> --> <object>
// for local variables in the nth closure scope
GetClosureVarOp,        // <depth>, <index>         --> <object>
SetClosureVarOp,        // <depth>, <index>         <object> --> <object>
// for array elements
GetElementOp,           //                          <base> <index> --> <object>
SetElementOp,           //                          <base> <index> <object> --> <object>
// for properties
GetPropertyOp,          // <poolindex>              <base> --> <object>
GetInvokePropertyOp,    // <poolindex>              <base> --> <base> <object> 
SetPropertyOp,          // <poolindex>              <base> <object> --> <object>
// for all generic names 
GetNameOp,              // <poolindex>              --> <object>
GetTypeOfNameOp,        // <poolindex>              --> <object>
SetNameOp,              // <poolindex>              <object> --> <object>
LoadGlobalObjectOp,     //                          --> <object>
PushScopeOp,            // <pointer>        XXX !!! XXX
PopScopeOp,             // <pointer>        XXX !!! XXX

OpCodeCount

} ByteCodeOp;

struct ByteCodeData {
    int8 stackImpact;
    char *opName;
};
extern ByteCodeData gByteCodeData[OpCodeCount];


    class ByteCodeModule {
    public:
            
        ByteCodeModule(ByteCodeGen *bcg);

        uint32 getLong(int index) const             { return *((uint32 *)&mCodeBase[index]); }
        int32 getOffset(int index) const            { return *((int32 *)&mCodeBase[index]); }
        const String *getString(int index) const    { return &mStringPoolContents[index]; }
        float64 getNumber(int index) const          { return mNumberPoolContents[index]; }


        uint32 mLocalsCount;        // number of local vars to allocate space for
        uint32 mStackDepth;         // max. depth of execution stack
        
        uint8 *mCodeBase;
        uint32 mLength;

        String *mStringPoolContents;
        float64 *mNumberPoolContents;

    };
    Formatter& operator<<(Formatter& f, const ByteCodeModule& bcm);

    #define BufferIncrement (32)

    class Label {
    public:
        
        typedef enum { InternalLabel, NamedLabel, BreakLabel, ContinueLabel } LabelKind;

        Label() : mKind(InternalLabel), mHasLocation(false) { }
        Label(LabelStmtNode *lbl) : mKind(NamedLabel), mHasLocation(false), mLabelStmt(lbl) { }
        Label(LabelKind kind) : mKind(kind), mHasLocation(false) { }


        bool matches(const StringAtom *name)
        {
            return ((mKind == NamedLabel) && (mLabelStmt->name.compare(*name) == 0));
        }

        bool matches(LabelKind kind)
        {
            return (mKind == kind);
        }

        void addFixup(ByteCodeGen *bcg, uint32 branchLocation);        
        void setLocation(ByteCodeGen *bcg, uint32 location);

        std::vector<uint32> mFixupList;

        LabelKind mKind;
        bool mHasLocation;
        LabelStmtNode *mLabelStmt;

        uint32 mLocation;
    };

    class ByteCodeGen {
    public:

        ByteCodeGen(Context *cx, ScopeChain *scopeChain) 
            :   mBuffer(new CodeBuffer), 
                mScopeChain(scopeChain), 
                m_cx(cx), 
                mNamespaceList(NULL) ,
                mStackTop(0),
                mStackMax(0)
        { }

        ByteCodeModule *genCodeForScript(StmtNode *p);
        void genCodeForStatement(StmtNode *p, ByteCodeGen *static_cg);
        void genCodeForFunction(FunctionStmtNode *f, 
                                    JSFunction *fnc, 
                                    bool isConstructor, 
                                    JSType *topClass);
        JSType *genExpr(ExprNode *p);
        Reference *genReference(ExprNode *p, Access acc);
        void genReferencePair(ExprNode *p, Reference *&readRef, Reference *&writeRef);

        typedef std::vector<uint8> CodeBuffer;

        // this is the current code buffer
        CodeBuffer *mBuffer;
        ScopeChain *mScopeChain;

        Context *m_cx;
        
        std::vector<Label> mLabelList;
        std::vector<uint32> mLabelStack;

        AttributeList *mNamespaceList;

        uint32 mStackTop;
        uint32 mStackMax;

        bool hasContent()
        {
            return (mBuffer->size() > 0);
        }
       
        void addOp(uint8 op)        
        { 
            addByte(op);
            ASSERT(gByteCodeData[op].stackImpact != -128);
            mStackTop += gByteCodeData[op].stackImpact;
            if (mStackTop > mStackMax)
                mStackMax = mStackTop; 
        }

        void addOpStretchStack(uint8 op, uint32 n)        
        {
            addByte(op);
            mStackTop += gByteCodeData[op].stackImpact;
            if ((mStackTop + n) > mStackMax)
                mStackMax = (mStackTop + n); 
        }

        // these routines assume the depth is being reduced
        // i.e. they don't reset mStackMax
        void addOpAdjustDepth(uint8 op, uint32 depth)        
                                    { addByte(op); mStackTop += depth; }
        void addOpSetDepth(uint8 op, uint32 depth)        
                                    { addByte(op); mStackTop = depth; }

        void addByte(uint8 v)       { mBuffer->push_back(v); }
        void addPointer(void *v)    { addLong((uint32)v); }
        void addLong(uint32 v)     
            { mBuffer->insert(mBuffer->end(), (uint8 *)&v, (uint8 *)(&v) + sizeof(uint32)); }
        void addOffset(int32 v)     
            { mBuffer->insert(mBuffer->end(), (uint8 *)&v, (uint8 *)(&v) + sizeof(int32)); }
        void setOffset(uint32 index, int32 v)
            {   *((int32 *)(mBuffer->begin() + index)) = v; }   // XXX

        void addFixup(uint32 label) 
        { 
            mLabelList[label].addFixup(this, mBuffer->size()); 
        }

        uint32 getLabel()
        {
            uint32 result = mLabelList.size();
            mLabelList.push_back(Label());
            return result;
        }

        uint32 getLabel(Label::LabelKind kind)
        {
            uint32 result = mLabelList.size();
            mLabelList.push_back(Label(kind));
            return result;
        }

        uint32 getLabel(LabelStmtNode *lbl)
        {
            uint32 result = mLabelList.size();
            mLabelList.push_back(Label(lbl));
            return result;
        }

        uint32 getTopLabel(Label::LabelKind kind, const StringAtom *name)
        {
            uint32 result = -1;
            for (std::vector<uint32>::reverse_iterator i = mLabelStack.rbegin(),
                                end = mLabelStack.rend();
                                (i != end); i++)
            {
                // find the closest kind of label
                if (mLabelList[*i].matches(kind))
                    result = *i;
                else // and return it when we get the name
                    if (mLabelList[*i].matches(name))
                        return result;
            }
            NOT_REACHED("label not found");
            return false;
        }

        uint32 getTopLabel(Label::LabelKind kind)
        {
            for (std::vector<uint32>::reverse_iterator i = mLabelStack.rbegin(),
                                end = mLabelStack.rend();
                                (i != end); i++)
            {
                if (mLabelList[*i].matches(kind))
                    return *i;
            }
            NOT_REACHED("label not found");
            return false;
        }

        void setLabel(uint32 label)
        {
            mLabelList[label].setLocation(this, mBuffer->size()); 
        }

        std::vector<String> mStringPoolContents;
        typedef std::map<String, uint32, std::less<String> > StringPool;
        StringPool mStringPool;

        std::vector<float64> mNumberPoolContents;
        typedef std::map<float64, uint32, std::less<double> > NumberPool;
        NumberPool mNumberPool;


        void addNumberRef(float64 f)
        {
            NumberPool::iterator i = mNumberPool.find(f);
            if (i != mNumberPool.end())
                addLong(i->second);
            else {
                addLong(mNumberPoolContents.size());
                mNumberPool[f] = mNumberPoolContents.size();
                mNumberPoolContents.push_back(f);
            }
        }
        
        
        void addStringRef(const String &str)
        {
            StringPool::iterator i = mStringPool.find(str);
            if (i != mStringPool.end())
                addLong(i->second);
            else {
                addLong(mStringPoolContents.size());
                mStringPool[str] = mStringPoolContents.size();
                mStringPoolContents.push_back(str);
            }
        }


    };

    inline void Label::setLocation(ByteCodeGen *bcg, uint32 location)
    {
        mHasLocation = true;
        mLocation = location;
        for (std::vector<uint32>::iterator i = mFixupList.begin(), end = mFixupList.end(); 
                        (i != end); i++)
        {
            uint32 branchLocation = *i;
            bcg->setOffset(branchLocation, mLocation - branchLocation); 
        }
    }

    inline void Label::addFixup(ByteCodeGen *bcg, uint32 branchLocation) 
    { 
        if (mHasLocation)
            bcg->addOffset(mLocation - branchLocation);
        else {
            mFixupList.push_back(branchLocation); 
            bcg->addLong(0);
        }
    }


    int printInstruction(Formatter &f, int i, const ByteCodeModule& bcm);
}
}

#endif bytecodegen_h___