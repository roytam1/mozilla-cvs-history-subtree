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
        LoadConstantNumberOp,   // <poolindex>

        LoadFunctionOp,         // <pointer>        XXX !!! XXX
        LoadTypeOp,             // <pointer>        XXX !!! XXX


        InvokeOp,               // <argCount>          <args> <invokor> --> <result>

        GetTypeOp,              //                     <object> --> <type of object>

        DoOperatorOp,           // <operation>         <op1> <op2> --> <result>

        PushNullOp,             //                     --> <Object(null)>
        PushIntOp,              // <int>               --> <Object(int)>
        PushNumOp,              // <num>               --> <Object(num)>
        PushStringOp,           // <poolindex>         --> <Object(index)>
        PushTypeOp,             // <poolindex>

        ReturnOp,               //                     <object> -->

        NewObjectOp,            //                     <args> <type> --> <object>


        JcondOp,                // <target>            <object> -->
        JumpOp,                 // <target>            


    
        // for instance members
        GetFieldOp,             // <slot>              <base> --> <slot> <invokor>
        SetFieldOp,             // <slot>              <base> --> <slot> <invokor>

        // for instance methods
        GetMethodOp,            // <slot>              <base> --> <invokor>

        // for argumentz
        GetArgOp,               // <index>             --> <object>
        SetArgOp,               // <index>             --> <object>

        // for local variables in the immediate scope
        GetLocalVarOp,          // <index>             --> <invokor>
        SetLocalVarOp,          // <index>             --> <invokor>

        // for local variables in the nth closure scope
        GetClosureVarOp,        // <depth>, <index>    --> <invokor>
        SetClosureVarOp,        // <depth>, <index>    --> <invokor>

        // for all other names
        GetNameOp,              // <poolindex>         --> <invokor>
        SetNameOp,              // <poolindex>         --> <invokor>


    } ByteCodeOp;


    class ByteCodeModule {
    public:
            
        ByteCodeModule(ByteCodeGen *bcg);

        uint32 getLong(int index) const      { return *((uint32 *)&mCodeBase[index]); }
        String getString(int index) const    { return String(&mStringPoolContents.at(index)); }
        float64 getNumber(int index) const   { return mNumberPoolContents.at(index); }


        uint32 mLocalsCount;        // number of local vars to allocate space for
        
        uint8 *mCodeBase;
        uint32 mLength;

        std::vector<char16> mStringPoolContents;
        std::vector<float64> mNumberPoolContents;

    };
    Formatter& operator<<(Formatter& f, const ByteCodeModule& bcm);

    #define BufferIncrement (32)

    class ByteCodeGen {
    public:

        ByteCodeGen() : mBuffer(new CodeBuffer) { }

        ByteCodeModule *genCodeForStatement(StmtNode *p);
        void genExpr(ExprNode *p);
        Reference *genReference(ExprNode *p, Access acc);

        typedef std::vector<uint8> CodeBuffer;

        std::stack<CodeBuffer *> grumpy;

        // this is the current code buffer
        CodeBuffer *mBuffer;

        void pushOnGrumpy()
        {
            grumpy.push(mBuffer);
            mBuffer = new CodeBuffer;
        }

        void popOffGrumpy()
        {
            mBuffer = grumpy.top();
            grumpy.pop();
        }


        void addByte(uint8 v)      { mBuffer->push_back(v); }
        void addPointer(void *v)   { addLong((uint32)v); }
        void addLong(uint32 v)     
            { mBuffer->insert(mBuffer->end(), (uint8 *)&v, (uint8 *)(&v) + sizeof(uint32)); }


        std::vector<char16> mStringPoolContents;
        typedef std::map<String, uint32, std::less<String> > StringPool;
        StringPool mStringPool;

        std::vector<float64> mNumberPoolContents;
        typedef std::map<float64, uint32, std::less<double> > NumberPool;
        NumberPool mNumberPool;


        void addNumber(float64 f)
        {
            NumberPool::iterator i = mNumberPool.find(f);
            if (i != mNumberPool.end())
                addLong(i->second);
            else {
                addLong(mNumberPool.size());
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
                mStringPoolContents.insert(mStringPoolContents.end(), str.begin(), str.end());
                mStringPoolContents.push_back(0);
            }
        }

        ScopeChain mScopeChain;

    };


}
}

#endif bytecodegen_h___