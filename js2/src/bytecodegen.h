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

        InvokeOp,               //                     <args> <invokor> --> <result>

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
        ByteCodeOp *mCodeBase;
        uint32 mLength;

    };

    #define BufferIncrement (32)

    class ByteCodeGen {
    public:
        ByteCodeModule *genCodeForStatement(StmtNode *p);
        void genExpr(ExprNode *p);


        void addByte(char v);
        void addPointer(void *v) { }
        void addLong(uint32 i)   { }

        String mStringPoolContents;
        typedef std::map<String, uint32, std::less<String> > StringPool;
        StringPool mStringPool;

        void addStringRef(const String &str)
        {
            StringPool::iterator i = mStringPool.find(str);
            if (i != mStringPool.end())
                addLong(i->second);
            else {
                addLong(mStringPoolContents.size());
                mStringPoolContents += str;
            }
        }

        ScopeChain mScopeChain;

        ByteCodeOp *mBuffer;
        ByteCodeOp *mBufferTop;
        ByteCodeOp *mBufferMax;
    };


}
}

#endif bytecodegen_h___