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


#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#endif

#include "parser.h"
#include "js2runtime.h"
#include "bytecodegen.h"

namespace JavaScript {    
namespace ByteCode {

    using namespace JS2Runtime;

    void ByteCodeGen::addByte(char v)
    {
        if (mBufferTop == mBufferMax) {
            uint32 bufSize = mBufferMax - mBuffer;
            ByteCodeOp *newBuffer = new ByteCodeOp[bufSize + BufferIncrement];
            if (mBuffer) {
                memcpy(newBuffer, mBuffer, bufSize);
                delete mBuffer;
            }
            mBuffer = newBuffer;
            mBufferTop = mBuffer + bufSize;
            mBufferMax = mBuffer + bufSize + BufferIncrement;
        }
        *mBufferTop++ = (ByteCodeOp)v;
    }
    
    ByteCodeModule *ByteCodeGen::genCodeForStatement(StmtNode *p)
    {
        switch (p->getKind()) {
        case StmtNode::expression:
            {
                ExprStmtNode *e = static_cast<ExprStmtNode *>(p);
                genExpr(e->expr);
            }
            break;
        }
        return NULL;
    }
   /*
    static bool isStaticName(JSClass *c, const StringAtom &name, Reference &ref)
    {
        do {
            bool isConstructor = false;
            if (c->hasStatic(name, ref.mType, isConstructor)) {
                ref.mClass = c;
                ref.mKind = (isConstructor) ? Constructor : Static;
                return true;
            }
            c = c->getSuperClass();
        } while (c);
        return false;
    }

    bool ICodeGenerator::getVariableByName(const StringAtom &name, Reference &ref)
    {
        TypedRegister v;
        v = variableList->findVariable(name);
        if (v.first == NotARegister)
            v = parameterList->findVariable(name);
        if (v.first != NotARegister) {
            ref.mKind = Var;
            ref.mBase = v;
            ref.mType = v.second;
            return true;
        }
        return false;
    }

    bool ICodeGenerator::scanForVariable(const StringAtom &name, Reference &ref)
    {
        if (getVariableByName(name, ref))
            return true;
    
        uint32 count = 0;
        ICodeGenerator *upper = mContainingFunction;
        while (upper) {
            if (upper->getVariableByName(name, ref)) {
                ref.mKind = ClosureVar;
                ref.mSlotIndex = ref.mBase.first;
                ref.mBase = getClosure(count);
                return true;
            }
            count++;
            upper = upper->mContainingFunction;
        }
        return false;
    }

    // find 'name' (unqualified) in the current context.
    // for local variable, returns v.first = register number
    // for slot/method, returns slotIndex and sets base appropriately
    // (note closure vars also get handled this way)
    // v.second is set to the type regardless
    bool ByteCodeGen::resolveIdentifier(const StringAtom &name, Reference &ref, Access access)
    {
        if (!mIsWithinWith) {
            if (scanForVariable(name, ref))
                return true;
            else {
                if (mClass) {   // we're compiling a method of a class
                    // look for static references first
                    if (isStaticName(mClass, name, ref, access)) {
                        return true;
                    }
                    // then instance methods (if we're in a instance member function)
                    if (!isStaticMethod()) {
                        if (isSlotName(mClass, name, ref, access)) {
                            return true;
                        }
                    }
                }
                // last chance - if it's a generic name in the global scope, try to get a type for it
                ref.mKind = Name;
                ref.mType = mContext->getGlobalObject()->getType(name);
                return true;
            }
        }
        // all bet's off, generic name & type
        ref.mKind = Name;
        ref.mType = &Object_Type;
        return true;
    }

    genReference(ExprNode *p)
    {
        switch (p->getKind()) {
        case ExprNode::identifer:
            {
        
    }
*/

    // a ByteCodeGen has a static scope chain (a JSScope is a JSType with a parent link)


    void ByteCodeGen::genExpr(ExprNode *p)
    {
        switch (p->getKind()) {
        case ExprNode::True:
            addByte(LoadConstantTrueOp);
            break;
        case ExprNode::False:
            addByte(LoadConstantFalseOp);
            break;
        case ExprNode::Null:
            addByte(LoadConstantNullOp);
            break;
        case ExprNode::add:
            {
                BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
                genExpr(b->op1);
                genExpr(b->op2);
                addByte(DoOperatorOp);
                addByte(Plus);
            }
            break;
        case ExprNode::identifier:
            {
                const StringAtom &name = static_cast<IdentifierExprNode *>(p)->name;
                Reference *ref = mScopeChain.getName(name, Read);
                ASSERT(ref);
                ref->emitCodeSequence();
            }
            break;
        case ExprNode::New:
            {
                InvokeExprNode *i = static_cast<InvokeExprNode *>(p);

                genExpr(i->op);
                addByte(GetTypeOp);

                ExprPairList *p = i->pairs;
                while (p) {
                    genExpr(p->value);
                    p = p->next;
                }
                addByte(NewObjectOp);


            }
            break;
        }
    }

}
}

