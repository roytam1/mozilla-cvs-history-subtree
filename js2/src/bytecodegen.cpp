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
#include "numerics.h"

namespace JavaScript {    
namespace JS2Runtime {

using namespace ByteCode;

Activation::Activation(JSValue *locals, JSValue *argBase, uint8 *pc, ByteCodeModule *module)
    : JSType(NULL), mArgumentBase(argBase), mPC(pc), mModule(module) 
{
    // need a private copy in case this activation gets persisted
    // within a closure ???
    mLocals = new JSValue[mModule->mLocalsCount];
    memcpy(mLocals, locals, sizeof(JSValue) * mModule->mLocalsCount);
}


void AccessorReference::emitCodeSequence(ByteCodeGen *bcg) 
{ 
    bcg->addByte(InvokeOp); 
    bcg->addPointer(mFunction);
}

void LocalVarReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetLocalVarOp);
    else
        bcg->addByte(SetLocalVarOp);
    bcg->addLong(mIndex); 
}

void ParameterReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetArgOp);
    else
        bcg->addByte(SetArgOp);
    bcg->addLong(mIndex); 
}

void ClosureVarReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetClosureVarOp);
    else
        bcg->addByte(SetClosureVarOp);
    bcg->addLong(mDepth); 
    bcg->addLong(mIndex); 
}

void FieldReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetFieldOp);
    else
        bcg->addByte(SetFieldOp);
    bcg->addLong(mIndex); 
}

void MethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(GetMethodOp);
    bcg->addLong(mIndex); 
}

void FunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
}

void NameReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetNameOp);
    else
        bcg->addByte(SetNameOp);
    bcg->addStringRef(mName); 
}

void ValueReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetNameOp);
    else
        bcg->addByte(SetNameOp);
    bcg->addStringRef(mName); 
}

}   // namespace JS2Runtime



namespace ByteCode {

using namespace JS2Runtime;

ByteCodeModule::ByteCodeModule(ByteCodeGen *bcg)
{
    mLength = bcg->mBuffer->size();
    mCodeBase = new uint8[mLength];
    memcpy(mCodeBase, bcg->mBuffer->begin(), mLength);
    mStringPoolContents = bcg->mStringPoolContents;
    mNumberPoolContents = bcg->mNumberPoolContents;
    mLocalsCount = bcg->mScopeChain.countVars();
}

ByteCodeModule *ByteCodeGen::genCodeForStatement(StmtNode *p)
{
    switch (p->getKind()) {
    case StmtNode::Var:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
            while (v)  {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    if (v->initializer) {
                        IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                        Reference *ref = mScopeChain.getName(i->name, Write);
                        ASSERT(ref);    // must have been added previously by processDeclarations
                        genExpr(v->initializer);
                        ref->emitCodeSequence(this);
                    }
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            if (f->function.name->getKind() == ExprNode::identifier) {
                const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                Reference *ref = mScopeChain.getName(name, Write);
                ASSERT(ref);

                mScopeChain.addScope(new ParameterBarrel());                

                VariableBinding *v = f->function.parameters;
                while (v) {
                    if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                        JSType *pType = mScopeChain.extractType(v->type);
                        IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                        mScopeChain.defineVariable(i->name, pType);
                    }
                    v = v->next;
                }
                
                mScopeChain.addScope(new Activation());                
                pushOnGrumpy();
                mScopeChain.processDeclarations(f->function.body);
                ByteCodeModule *bcm = genCodeForStatement(f->function.body);
                popOffGrumpy();
                mScopeChain.popScope();
                mScopeChain.popScope();

                addByte(LoadFunctionOp);
                addPointer(new JSFunction(bcm));
                ref->emitCodeSequence(this);
            }
        }
        break;
    case StmtNode::block:
        {
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                genCodeForStatement(s);
                s = s->next;
            }            
        }
        break;
    case StmtNode::Return:
        {
            ExprStmtNode *e = static_cast<ExprStmtNode *>(p);
            if (e->expr)
                genExpr(e->expr);
            addByte(ReturnOp);
        }
        break;
    case StmtNode::expression:
        {
            ExprStmtNode *e = static_cast<ExprStmtNode *>(p);
            genExpr(e->expr);
        }
        break;
    }
    return new ByteCodeModule(this);
}

Reference *ByteCodeGen::genReference(ExprNode *p, Access acc)
{
    switch (p->getKind()) {
    case ExprNode::identifier:
        {
            const StringAtom &name = static_cast<IdentifierExprNode *>(p)->name;
            Reference *ref = mScopeChain.getName(name, acc);
            if (ref == NULL)
                ref = new NameReference(name, acc);
            return ref;
        }
    default:
        NOT_REACHED("Bad genReference op");
    }
    return NULL;
}


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
    case ExprNode::number :
        addByte(LoadConstantNumberOp);
        addNumber((static_cast<NumberExprNode *>(p))->value);
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
    case ExprNode::assignment:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *ref = genReference(b->op1, Write);
            genExpr(b->op2);
            ref->emitCodeSequence(this);
        }
        break;
    case ExprNode::identifier:
        {
            Reference *ref = genReference(p, Read);
            ref->emitCodeSequence(this);
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
    case ExprNode::call:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);

            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }

            Reference *ref = genReference(i->op, Read);
            ref->emitCodeSequence(this);

            addByte(InvokeOp);
            addLong(argCount);

        }
        break;
    }
}

Formatter& operator<<(Formatter& f, const ByteCodeModule& bcm)
{
    uint32 i = 0;
    while (i < bcm.mLength) {
        switch (bcm.mCodeBase[i]) {
        case ReturnOp:
            f << "Return\n";
            i++;
            break;
        case InvokeOp:
            f << "Invoke " << bcm.getLong(i + 1) << "\n";
            i += 5;
            break;
        case DoOperatorOp:
            f << "DoOperator " << bcm.mCodeBase[i + 1] << "\n";
            i += 2;
            break;
        case GetLocalVarOp:
            f << "GetLocalVar " << bcm.getLong(i + 1) << "\n";
            i += 5;
            break;
        case SetLocalVarOp:
            f << "SetLocalVar " << bcm.getLong(i + 1) << "\n";
            i += 5;
            break;
        case GetArgOp:
            f << "GetArg " << bcm.getLong(i + 1) << "\n";
            i += 5;
            break;
        case SetArgOp:
            f << "SetArg " << bcm.getLong(i + 1) << "\n";
            i += 5;
            break;
        case GetNameOp:
            f << "GetName " << bcm.getString(bcm.getLong(i + 1)) << "\n";
            i += 5;
            break;
        case SetNameOp:
            f << "SetName " << bcm.getString(bcm.getLong(i + 1)) << "\n";
            i += 5;
            break;
        case LoadConstantNumberOp:
            f << "LoadConstantNumber " << bcm.getNumber(bcm.getLong(i + 1)) << "\n";
            i += 5;
            break;
        case LoadFunctionOp:
            printFormat(f, "LoadFunction 0x%X\n", bcm.getLong(i + 1));
            i += 5;
            break;
        default:
            printFormat(f, "Unknown Opcode 0x%X\n", bcm.mCodeBase[i]);
            i++;
            break;
        }
    }
    return f;
}

}
}

