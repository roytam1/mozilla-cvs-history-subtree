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
namespace ByteCode {


    using namespace JS2Runtime;

    typedef enum {

        LoadConstantTrueOp,     //                     --> <true value object>
        LoadConstantFalseOp,    //                     --> <false value object>
        LoadConstantNullOp,     //                     --> <null value object>
        LoadConstantNumberOp,   // <poolindex>         --> <Number value object>
        LoadConstantStringOp,   // <poolindex>         --> <String value object>

        LoadThisOp,             //                     --> <this object>
        
        LoadFunctionOp,         // <pointer>        XXX !!! XXX
        LoadTypeOp,             // <pointer>        XXX !!! XXX


        InvokeOp,               // <argCount>          <function> <args>  --> [<result>]

        GetTypeOp,              //                     <object> --> <type of object>

        DoUnaryOp,              // <operation>         <object> --> <result>
        DoOperatorOp,           // <operation>         <object> <object> --> <result>

        PushNullOp,             //                     --> <Object(null)>
        PushIntOp,              // <int>               --> <Object(int)>
        PushNumOp,              // <num>               --> <Object(num)>
        PushStringOp,           // <poolindex>         --> <Object(index)>
        PushTypeOp,             // <poolindex>

        ReturnOp,               //                     <function> <args> <result> --> <result>
        ReturnVoidOp,           //                     <function> <args> -->

        GetConstructorOp,       //                     <type> --> <function> <type>
        NewObjectOp,            //                     <type> --> <object>


        JumpFalseOp,            // <target>            <object> -->
        JumpTrueOp,             // <target>            <object> -->
        JumpOp,                 // <target>            

        LogicalNotOp,           //                     <object> --> <object>
        SwapOp,                 //                     <object1> <object2> --> <object2> <object1>
        DupOp,                  //                     <object> --> <object> <object>
        DupInsertOp,            //                     <object1> <object2> --> <object2> <object1> <object2>
        PopOp,                  //                     <object> -->
    
        // for instance members
        GetFieldOp,             // <slot>              <base> --> <object>
        SetFieldOp,             // <slot>              <object> <base>  --> 
        // for static members
        GetStaticFieldOp,       // <slot>              <base> --> <object>
        SetStaticFieldOp,       // <slot>              <object> <base>  --> 

        // for instance methods
        GetMethodOp,            // <slot>              <base> --> <function> <base> 
        // for static methods
        GetStaticMethodOp,      // <slot>              <base> --> <function>

        // for argumentz
        GetArgOp,               // <index>             --> <object>
        SetArgOp,               // <index>             --> <object>

        // for local variables in the immediate scope
        GetLocalVarOp,          // <index>             --> <object>
        SetLocalVarOp,          // <index>             --> <object>

        // for local variables in the nth closure scope
        GetClosureVarOp,        // <depth>, <index>    --> <object>
        SetClosureVarOp,        // <depth>, <index>    --> <object>

        // for properties
        GetPropertyOp,          // <poolindex>         <base> --> <object>
        SetPropertyOp,          // <poolindex>         <object> <base> -->

        // for all generic names 
        GetNameOp,              // <poolindex>         --> <object>
        SetNameOp,              // <poolindex>         --> <object>

        
        PushScopeOp,            // <pointer>        XXX !!! XXX
        PopScopeOp,             // <pointer>        XXX !!! XXX


    } ByteCodeOp;


    class ByteCodeModule {
    public:
            
        ByteCodeModule(ByteCodeGen *bcg);

        uint32 getLong(int index) const             { return *((uint32 *)&mCodeBase[index]); }
        int32 getOffset(int index) const            { return *((int32 *)&mCodeBase[index]); }
        const String *getString(int index) const    { return &mStringPoolContents[index]; }
        float64 getNumber(int index) const          { return mNumberPoolContents[index]; }


        uint32 mLocalsCount;        // number of local vars to allocate space for
        
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
            : mBuffer(new CodeBuffer), mScopeChain(scopeChain), m_cx(cx) { }

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
            uint32 result;
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