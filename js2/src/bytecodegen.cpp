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
#include "formatter.h"

#include <string.h>

namespace JavaScript {    
namespace JS2Runtime {

using namespace ByteCode;

Activation::Activation(JSValue *locals, uint32 argBase, uint8 *pc, ByteCodeModule *module, uint32 argCount)
    : JSType(NULL), mArgumentBase(argBase), mPC(pc), mModule(module), mArgCount(argCount)
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

void StaticFieldReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadTypeOp);
    bcg->addPointer(mClass);
}

void FieldReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadThisOp);
}

void FieldReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetFieldOp);
    else
        bcg->addByte(SetFieldOp);
    bcg->addLong(mIndex); 
}

void StaticFieldReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetStaticFieldOp);
    else
        bcg->addByte(SetStaticFieldOp);
    bcg->addLong(mIndex); 
}

void MethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(GetMethodOp);
    bcg->addLong(mIndex); 
}

void StaticFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(GetStaticMethodOp);
    bcg->addLong(mIndex); 
}

void GetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(GetMethodOp);
    bcg->addLong(mIndex); 
    bcg->addByte(InvokeOp);
    bcg->addLong(1);
}

void SetterMethodReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addByte(GetMethodOp);
    bcg->addLong(mIndex); 
}

void SetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(InvokeOp);
    bcg->addLong(1);
}

void FunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadFunctionOp);
    bcg->addPointer(mFunction);
}

void ConstructorReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadFunctionOp);
    bcg->addPointer(mFunction);
}

void GetterFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadFunctionOp);
    bcg->addPointer(mFunction);
    bcg->addByte(InvokeOp);
    bcg->addLong(0);
}

void SetterFunctionReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addByte(LoadFunctionOp);
    bcg->addPointer(mFunction);
}

void SetterFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addByte(InvokeOp);
    bcg->addLong(1);
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

void PropertyReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addByte(GetPropertyOp);
    else
        bcg->addByte(SetPropertyOp);
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
    mStringPoolContents = new String[bcg->mStringPoolContents.size()];

    int index = 0;
    for (std::vector<String>::iterator s_i = bcg->mStringPoolContents.begin(),
            s_end = bcg->mStringPoolContents.end(); (s_i != s_end); s_i++, index++)
        mStringPoolContents[index] = *s_i;

    mNumberPoolContents = new float64[bcg->mNumberPoolContents.size()];
    index = 0;
    for (std::vector<float64>::iterator f_i = bcg->mNumberPoolContents.begin(),
            f_end = bcg->mNumberPoolContents.end(); (f_i != f_end); f_i++, index++)
        mNumberPoolContents[index] = *f_i;

    mLocalsCount = bcg->mScopeChain->countVars();
}

void ByteCodeGen::genCodeForFunction(FunctionStmtNode *f, JSFunction *fnc, bool isConstructor, JSType *topClass)
{
    mScopeChain->addScope(fnc->mParameterBarrel);
    mScopeChain->addScope(&fnc->mActivation);
    // OPT - no need to push the parameter and function
    // scopes if the function doesn't contain any 'eval'
    // calls, all other references to the variables mapped
    // inside these scopes will have been turned into
    // localVar references.
    addByte(PushScopeOp);
    addPointer(fnc->mParameterBarrel);
    addByte(PushScopeOp);   
    addPointer(&fnc->mActivation);

    if (isConstructor) {
        //
        //  Invoke the super class constructor if there isn't an explicit
        // statement to do so.
        //  
        bool foundSuperCall = false;
        BlockStmtNode *b = f->function.body;
        if (b && b->statements) {
            if (b->statements->getKind() == StmtNode::expression) {
                ExprStmtNode *e = static_cast<ExprStmtNode *>(b->statements);
                if (e->expr->getKind() == ExprNode::call) {
                    InvokeExprNode *i = static_cast<InvokeExprNode *>(e->expr);
                    if (i->op->getKind() == ExprNode::dot) {
                        // here, under '.' we check for 'super.m()' or 'this.m()'
                        // 
                        BinaryExprNode *b = static_cast<BinaryExprNode *>(i->op);
                        if ((b->op1->getKind() == ExprNode::This) && (b->op2->getKind() == ExprNode::identifier)) {
//                            IdentifierExprNode *i = static_cast<IdentifierExprNode *>(b->op2);
                            // XXX verify that i->name is a constructor in the superclass
                            foundSuperCall = true;
                        }
                        else {
                            if ((b->op1->getKind() == ExprNode::Super) && (b->op2->getKind() == ExprNode::identifier)) {
//                                IdentifierExprNode *i = static_cast<IdentifierExprNode *>(b->op2);
                                // XXX verify that i->name is a constructor in this class
                                foundSuperCall = true;
                            }
                        }
                    }
                    else {
                        // look for calls to 'this()' or 'super'
                        if (i->op->getKind() == ExprNode::This) {
                            foundSuperCall = true;
                        }
                        else {
                            if (i->op->getKind() == ExprNode::Super) {
                                foundSuperCall = true;
                            }
                        }
                    }
                }
            }
            else {
                // is there (going to be) such a thing as a SuperStatement ?
            }
        }

        if (!foundSuperCall) { // invoke the default superclass constructor
            JSType *superClass = topClass->mSuperType;
            if (superClass) {
                JSFunction *superConstructor = superClass->getDefaultConstructor();
                if (superConstructor) {
                    addByte(LoadFunctionOp);
                    addPointer(superConstructor);
                    addByte(LoadThisOp);
                    addByte(InvokeOp);
                    addLong(1);
                }
            }
        }
    }

    genCodeForStatement(f->function.body, NULL);

    // OPT - see above
    addByte(PopScopeOp);
    addByte(PopScopeOp);
    if (isConstructor) {
        addByte(LoadThisOp);
        addByte(ReturnOp);
    }
    
    fnc->mByteCode = new ByteCodeModule(this);        

    mScopeChain->popScope();
    mScopeChain->popScope();
}


ByteCodeModule *ByteCodeGen::genCodeForScript(StmtNode *p)
{
    while (p) {
        genCodeForStatement(p, NULL);
        p = p->next;
    }
    return new ByteCodeModule(this);
}


void ByteCodeGen::genCodeForStatement(StmtNode *p, ByteCodeGen *static_cg)
{
    switch (p->getKind()) {
    case StmtNode::Class:
        {
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);

            ASSERT(PROPERTY_KIND(p->prop) == ValuePointer);
            ASSERT(PROPERTY_VALUEPOINTER(p->prop)->isType());
            JSType *thisClass = (JSType *)(PROPERTY_VALUEPOINTER(p->prop)->type);

            mScopeChain->addScope(thisClass->mStatics);
            mScopeChain->addScope(thisClass);
            ByteCodeGen bcg(m_cx, mScopeChain);       // this will capture the initializations
            if (classStmt->body) {
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    bcg.genCodeForStatement(s, static_cg);
                    s = s->next;
                }
            }
            mScopeChain->popScope();
            mScopeChain->popScope();
        }
        break;
    case StmtNode::Var:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    if (v->initializer) {
                        IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                        Reference *ref = mScopeChain->getName(i->name, CURRENT_ATTR, Write);
                        ASSERT(ref);    // must have been added previously by collectNames
                        if (isStatic && (static_cg != NULL)) {
                            static_cg->genExpr(v->initializer);
                            ref->emitCodeSequence(static_cg);
                        }
                        else {
                            genExpr(v->initializer);
                            ref->emitCodeSequence(this);
                        }
                        delete ref;
                    }
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            bool isConstructor = hasAttribute(f->attributes, mScopeChain->ConstructorKeyWord);
            JSFunction *fnc = f->mFunction;    

            if (f->function.name->getKind() == ExprNode::identifier) {
                const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                if (mScopeChain->topClass() 
                            && (mScopeChain->topClass()->mClassName.compare(name) == 0))
                    isConstructor = true;
            }
/*            
            bool isStatic = hasAttribute(f->attributes, Token::Static);
            bool isOperator = hasAttribute(f->attributes, mScopeChain->OperatorKeyWord);
            if (isOperator) {   // XXX
                ASSERT(f->function.name->getKind() == ExprNode::string);
                const String& name = static_cast<StringExprNode *>(f->function.name)->str;
                Operator op = m_cx->getOperator(m_cx->getParameterCount(f->function), name);

                fnc = m_cx->getOperator(op, m_cx->getParameterType(f->function, 0), 
                                   m_cx->getParameterType(f->function, 1));
            }
            else {
                ASSERT(f->function.name->getKind() == ExprNode::identifier);
                const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                if (mScopeChain->topClass() 
                            && (mScopeChain->topClass()->mClassName.compare(name) == 0))
                    isConstructor = true;

                if (isStatic || isConstructor) {
                    JSValue v = mScopeChain->getStaticValue(PROPERTY(p->prop));
                    ASSERT(v.isFunction());
                    fnc = v.function;
                }
                else {
                    switch (f->function.prefix) {
                    case FunctionName::Get:
                        fnc = mScopeChain->getGetterValue(PROPERTY(p->prop));
                        break;
                    case FunctionName::Set:
                        fnc = mScopeChain->getSetterValue(PROPERTY(p->prop));
                        break;
                    case FunctionName::normal:
                        {
                            JSValue v = mScopeChain->getValue(PROPERTY(p->prop));
                            ASSERT(v.isFunction());
                            fnc = v.function;
                        }
                        break;
                    }
                }
            }
*/
            ByteCodeGen bcg(m_cx, mScopeChain);
            bcg.genCodeForFunction(f, fnc, isConstructor, mScopeChain->topClass());
        }
        break;
    case StmtNode::While:
        {
            UnaryStmtNode *w = static_cast<UnaryStmtNode *>(p);
            addByte(JumpOp);
            uint32 labelAtTestCondition = getLabel(Label::ContinueLabel); 
            addFixup(labelAtTestCondition);
            uint32 labelAtTopOfBlock = getLabel();
            setLabel(labelAtTopOfBlock);
            uint32 breakLabel = getLabel(Label::BreakLabel);

            mLabelStack.push_back(breakLabel);
            mLabelStack.push_back(labelAtTestCondition);
            genCodeForStatement(w->stmt, static_cg);
            mLabelStack.pop_back();
            mLabelStack.pop_back();
            
            setLabel(labelAtTestCondition);
            genExpr(w->expr);
            addByte(JumpTrueOp);
            addFixup(labelAtTopOfBlock);
            setLabel(breakLabel);
        }
        break;
    case StmtNode::label:
        {
            LabelStmtNode *l = static_cast<LabelStmtNode *>(p);
            mLabelStack.push_back(getLabel(l));
            genCodeForStatement(l->stmt, static_cg);
            mLabelStack.pop_back();
        }
        break;
    case StmtNode::Break:
        {
            GoStmtNode *g = static_cast<GoStmtNode *>(p);
            addByte(JumpOp);
            if (g->name)
                addFixup(getTopLabel(Label::BreakLabel, g->name));
            else
                addFixup(getTopLabel(Label::BreakLabel));
        }
        break;
    case StmtNode::Continue:
        {
            GoStmtNode *g = static_cast<GoStmtNode *>(p);
            addByte(JumpOp);
            if (g->name)
                addFixup(getTopLabel(Label::ContinueLabel, g->name));
            else
                addFixup(getTopLabel(Label::ContinueLabel));
        }
        break;
    case StmtNode::If:
        {
            UnaryStmtNode *i = static_cast<UnaryStmtNode *>(p);
            genExpr(i->expr);
            addByte(JumpFalseOp);
            uint32 label = getLabel(); 
            addFixup(label);
            genCodeForStatement(i->stmt, static_cg);
            setLabel(label);
        }
        break;
    case StmtNode::IfElse:
        {
            BinaryStmtNode *i = static_cast<BinaryStmtNode *>(p);
            genExpr(i->expr);
            addByte(JumpFalseOp);
            uint32 elseStatementLabel = getLabel(); 
            addFixup(elseStatementLabel);
            genCodeForStatement(i->stmt, static_cg);
            addByte(JumpOp);
            uint32 branchAroundElselabel = getLabel(); 
            addFixup(branchAroundElselabel);
            setLabel(elseStatementLabel);
            genCodeForStatement(i->stmt2, static_cg);
            setLabel(branchAroundElselabel);
        }
        break;
    case StmtNode::block:
        {
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                genCodeForStatement(s, static_cg);
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
            addByte(PopOp);
        }
        break;
    default:
        NOT_REACHED("Not Implemented Yet");
    }
}

Reference *ByteCodeGen::genReference(ExprNode *p, Access acc)
{
    switch (p->getKind()) {
    case ExprNode::identifier:
        {
            const StringAtom &name = static_cast<IdentifierExprNode *>(p)->name;
            Reference *ref = mScopeChain->getName(name, CURRENT_ATTR, acc);            
            if (ref == NULL)
                ref = new NameReference(name, acc);
            ref->emitImplicitLoad(this);
            return ref;
        }
    case ExprNode::dot:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            
            JSType *lType = NULL;

            // Optimize for ClassName.identifier. If we don't
            // do this we simply generate a getProperty op
            // against the Type_Type object the leftside has found.
            //
            // If we find it, emit the code to 'load' the class
            // (which loads the static instance) and the name 
            // lookup can then proceed against the static type.
            //
            // Note that we're depending on this to discover when
            // a class constructor is being invoked (and hence needs
            // a newObjectOp). 
            if (b->op1->getKind() == ExprNode::identifier) {
                const StringAtom &name = static_cast<IdentifierExprNode *>(b->op1)->name;
                Reference *ref = mScopeChain->getName(name, CURRENT_ATTR, Read);
                if (ref && (ref->mType == Type_Type)) {
                    JSValue v = mScopeChain->getValue(name, CURRENT_ATTR);
                    ASSERT(v.isType());
                    if (v.type->mStatics) {
                        lType = v.type->mStatics;
                        genExpr(b->op1);
                    }
                }
                if (ref) delete ref;
            }

            if (lType == NULL)
                lType = genExpr(b->op1);    // generate code for leftside of dot
            if (b->op2->getKind() == ExprNode::qualify) {
                BinaryExprNode *q = static_cast<BinaryExprNode *>(b->op2);
                ASSERT(q->op1->getKind() == ExprNode::identifier);
                ASSERT(q->op2->getKind() == ExprNode::identifier);
                const StringAtom &fieldName = static_cast<IdentifierExprNode *>(q->op2)->name;
                const StringAtom &qualifierName = static_cast<IdentifierExprNode *>(q->op1)->name;
                IdentifierList id(qualifierName);
                id.next = CURRENT_ATTR;
                Reference *ref = lType->genReference(fieldName, &id, acc, 0);
                if (ref == NULL)
                    ref = new PropertyReference(fieldName, acc);
                return ref;
            }
            else {
                ASSERT(b->op2->getKind() == ExprNode::identifier);
                const StringAtom &fieldName = static_cast<IdentifierExprNode *>(b->op2)->name;
                Reference *ref = lType->genReference(fieldName, CURRENT_ATTR, acc, 0);
                if (ref == NULL)
                    ref = new PropertyReference(fieldName, acc);
                return ref;
            }

        }
    default:
        NOT_REACHED("Bad genReference op");
    }
    return NULL;
}

void ByteCodeGen::genReferencePair(ExprNode *p, Reference *&readRef, Reference *&writeRef)
{
    switch (p->getKind()) {
    case ExprNode::identifier:
        {
            const StringAtom &name = static_cast<IdentifierExprNode *>(p)->name;
            readRef = mScopeChain->getName(name, CURRENT_ATTR, Read);            
            if (readRef == NULL)
                readRef = new NameReference(name, Read);
            writeRef = mScopeChain->getName(name, CURRENT_ATTR, Write);            
            if (writeRef == NULL)
                writeRef = new NameReference(name, Write);
            readRef->emitImplicitLoad(this);
        }
        break;
    case ExprNode::dot:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            
            JSType *lType = NULL;

            if (b->op1->getKind() == ExprNode::identifier) {
                const StringAtom &name = static_cast<IdentifierExprNode *>(b->op1)->name;
                Reference *ref = mScopeChain->getName(name, CURRENT_ATTR, Read);
                if (ref && (ref->mType == Type_Type)) {
                    JSValue v = mScopeChain->getValue(name, CURRENT_ATTR);
                    ASSERT(v.isType());
                    if (v.type->mStatics) {
                        lType = v.type->mStatics;
                        genExpr(b->op1);
                    }
                }
                if (ref) delete ref;
            }

            if (lType == NULL)
                lType = genExpr(b->op1);    // generate code for leftside of dot
            if (b->op2->getKind() != ExprNode::identifier) {
                // this is where we handle n.q::id
            }
            else {
                const StringAtom &fieldName = static_cast<IdentifierExprNode *>(b->op2)->name;
                readRef = lType->genReference(fieldName, CURRENT_ATTR, Read, 0);
                if (readRef == NULL)
                    readRef = new PropertyReference(fieldName, Read);
                writeRef = lType->genReference(fieldName, CURRENT_ATTR, Write, 0);
                if (writeRef == NULL)
                    writeRef = new PropertyReference(fieldName, Write);
            }
        }
        break;
    default:
        NOT_REACHED("Bad genReferencePair op");
    }
}


JSType *ByteCodeGen::genExpr(ExprNode *p)
{
    Operator op;

    switch (p->getKind()) {
    case ExprNode::True:
        addByte(LoadConstantTrueOp);
        return Boolean_Type;
    case ExprNode::False:
        addByte(LoadConstantFalseOp);
        return Boolean_Type;
    case ExprNode::Null:
        addByte(LoadConstantNullOp);
        return Object_Type;
    case ExprNode::number :
        addByte(LoadConstantNumberOp);
        addNumberRef((static_cast<NumberExprNode *>(p))->value);
        return Number_Type;
    case ExprNode::string :
        addByte(LoadConstantStringOp);
        addStringRef((static_cast<StringExprNode *>(p))->str);
        return String_Type;

    case ExprNode::add:
        op = Plus;
        goto BinaryOperator;
    case ExprNode::subtract:
        op = Minus;
        goto BinaryOperator;
    case ExprNode::multiply:
        op = Multiply;
        goto BinaryOperator;
    case ExprNode::divide:
        op = Divide;
        goto BinaryOperator;
    case ExprNode::modulo:
        op = Remainder;
        goto BinaryOperator;
    case ExprNode::leftShift:
        op = ShiftLeft;
        goto BinaryOperator;
    case ExprNode::rightShift:
        op = ShiftRight;
        goto BinaryOperator;
    case ExprNode::logicalRightShift:
        op = UShiftRight;
        goto BinaryOperator;
    case ExprNode::bitwiseAnd:
        op = BitAnd;
        goto BinaryOperator;
    case ExprNode::bitwiseXor:
        op = BitXor;
        goto BinaryOperator;
    case ExprNode::bitwiseOr:
        op = BitOr;
        goto BinaryOperator;
    case ExprNode::lessThan:
        op = Less;
        goto BinaryOperator;
    case ExprNode::lessThanOrEqual:
        op = LessEqual;
        goto BinaryOperator;
    case ExprNode::In:
        op = In;
        goto BinaryOperator;
    case ExprNode::equal:
        op = Equal;
        goto BinaryOperator;
    case ExprNode::identical:
        op = SpittingImage;
        goto BinaryOperator;

BinaryOperator:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addByte(DoOperatorOp);
            addByte(op);
            return Object_Type;
        }

    case ExprNode::notEqual:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addByte(DoOperatorOp);
            addByte(Equal);
            addByte(LogicalNotOp);
            return Object_Type;
        }
    case ExprNode::greaterThan:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addByte(SwapOp);
            addByte(DoOperatorOp);
            addByte(Less);
            return Object_Type;
        }
    case ExprNode::greaterThanOrEqual:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addByte(SwapOp);
            addByte(DoOperatorOp);
            addByte(LessEqual);
            return Object_Type;
        }
    case ExprNode::notIdentical:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addByte(DoOperatorOp);
            addByte(SpittingImage);
            addByte(LogicalNotOp);
            return Object_Type;
        }

    case ExprNode::minus:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addByte(DoUnaryOp);
            addByte(Negate);
            return Object_Type;
        }

    case ExprNode::plus:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addByte(DoUnaryOp);
            addByte(Posate);
            return Object_Type;
        }

    case ExprNode::complement:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addByte(DoUnaryOp);
            addByte(Complement);
            return Object_Type;
        }

    case ExprNode::preIncrement:
        op = Increment;
        goto PreXcrement;
    case ExprNode::preDecrement:
        op = Decrement;
        goto PreXcrement;

PreXcrement:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;            
            genReferencePair(u->op, readRef, writeRef);
            if (readRef->hasBaseExpression()) {
                addByte(DupOp);         // duplicate the base expression
                readRef->emitCodeSequence(this);
                addByte(DoUnaryOp);
                addByte(op);
            }
            else {
                readRef->emitCodeSequence(this);
                addByte(DoUnaryOp);
                addByte(op);
            }
            writeRef->emitCodeSequence(this);
            return Object_Type;
        }

    case ExprNode::postIncrement:
        op = Increment;
        goto PostXcrement;
    case ExprNode::postDecrement:
        op = Decrement;
        goto PostXcrement;

PostXcrement:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;
            genReferencePair(u->op, readRef, writeRef);
            if (readRef->hasBaseExpression()) {
                addByte(DupOp);         // duplicate the base expression
                readRef->emitCodeSequence(this);
                addByte(DupInsertOp);   // duplicate the value and bury it
            }
            else {
                readRef->emitCodeSequence(this);
                addByte(DupOp);
            }
            addByte(DoUnaryOp);
            addByte(op);
            writeRef->emitCodeSequence(this);
            addByte(PopOp);     // because the SetXXX will propogate the new value
            return Object_Type;
        }

    case ExprNode::logicalNot:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addByte(LogicalNotOp);
            return Boolean_Type;
        }

    case ExprNode::assignment:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *ref = genReference(b->op1, Write);
            genExpr(b->op2);
            ref->emitCodeSequence(this);
            delete ref;
            return Object_Type;
        }
    case ExprNode::identifier:
        {
            Reference *ref = genReference(p, Read);
            ref->emitCodeSequence(this);
            JSType *type = ref->mType;
            delete ref;
            return type;
        }
    case ExprNode::dot:
        {
            Reference *ref = genReference(p, Read);
            ref->emitCodeSequence(this);
            JSType *type = ref->mType;
            delete ref;
            return type;
        }
    case ExprNode::New:
        {
            // if we can get the type, this is an invocation of
            // whichever default constructor matches
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            JSType *type = genExpr(i->op);
            addByte(GetConstructorOp);
            addByte(NewObjectOp);

            ExprPairList *p = i->pairs;
            uint32 argCount = 1;        // 'this' object on stack already
            while (p) {
                genExpr(p->value);
                p = p->next;
            }
            addByte(InvokeOp);
            addLong(argCount);
            return type;
        }
    case ExprNode::call:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            Reference *ref = genReference(i->op, Read);
            ref->emitCodeSequence(this);

            // we want to be able to detect calls to a class constructor
            // (other than as super.m() or this.m()) and precede them
            // with a call to newObject

            if (ref->isConstructor()) {
                addByte(LoadTypeOp);
                addPointer(ref->mType);
                addByte(NewObjectOp);
            }


            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }

            addByte(InvokeOp);
            if (ref->needsThis())
                argCount++;
            addLong(argCount);


            JSType *type = ref->mType;
            delete ref;
            return type;
        }
    case ExprNode::parentheses:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            return genExpr(u->op);
        }

    default:
        NOT_REACHED("Not Implemented Yet");
    }
    return NULL;
}

int printInstruction(Formatter &f, int i, const ByteCodeModule& bcm)
{
    switch (bcm.mCodeBase[i]) {
    case SwapOp:
        f << "Swap\n";
        i++;
        break;
    case DupOp:
        f << "Dup\n";
        i++;
        break;
    case PopOp:
        f << "PopOp\n";
        i++;
        break;
    case DupInsertOp:
        f << "DupInsert\n";
        i++;
        break;
    case LogicalNotOp:
        f << "LogicalNot\n";
        i++;
        break;
    case JumpFalseOp:
        f << "JumpFalse " << bcm.getOffset(i + 1) << "\n";
        i += 5;
        break;
    case JumpTrueOp:
        f << "JumpTrue " << bcm.getOffset(i + 1) << "\n";
        i += 5;
        break;
    case JumpOp:
        f << "Jump " << bcm.getOffset(i + 1) << "\n";
        i += 5;
        break;
    case ReturnOp:
        f << "Return\n";
        i++;
        break;
    case ReturnVoidOp:
        f << "ReturnVoid\n";
        i++;
        break;
    case InvokeOp:
        f << "Invoke " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;
    case DoUnaryOp:
        f << "DoUnary " << bcm.mCodeBase[i + 1] << "\n";
        i += 2;
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
    case GetMethodOp:
        f << "GetMethod " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;            
    case GetStaticMethodOp:
        f << "GetStaticMethod " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;            
    case GetFieldOp:
        f << "GetField " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;
    case SetFieldOp:
        f << "SetField " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;
    case GetStaticFieldOp:
        f << "GetStaticField " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;
    case SetStaticFieldOp:
        f << "SetStaticField " << bcm.getLong(i + 1) << "\n";
        i += 5;
        break;
    case GetNameOp:
        f << "GetName " << *bcm.getString(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case SetNameOp:
        f << "SetName " << *bcm.getString(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case GetPropertyOp:
        f << "GetProperty " << *bcm.getString(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case SetPropertyOp:
        f << "SetProperty " << *bcm.getString(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case LoadConstantTrueOp:
        f << "LoadConstantTrue\n";
        i++;
        break;
    case LoadConstantFalseOp:
        f << "LoadConstantFalse\n";
        i++;
        break;
    case LoadConstantNullOp:
        f << "LoadConstantNull\n";
        i++;
        break;
    case LoadConstantNumberOp:
        f << "LoadConstantNumber " << bcm.getNumber(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case LoadConstantStringOp:
        f << "LoadConstantString " << *bcm.getString(bcm.getLong(i + 1)) << "\n";
        i += 5;
        break;
    case LoadTypeOp:
        printFormat(f, "LoadType 0x%X\n", bcm.getLong(i + 1));
        i += 5;
        break;
    case LoadFunctionOp:
        printFormat(f, "LoadFunction 0x%X\n", bcm.getLong(i + 1));
        i += 5;
        break;
    case PushScopeOp:
        printFormat(f, "PushScope 0x%X\n", bcm.getLong(i + 1));
        i += 5;
        break;
    case PopScopeOp:
        f << "PopScope\n";
        i++;
        break;
    case GetConstructorOp:
        f << "GetConstructor\n";
        i++;
        break;
    case NewObjectOp:
        f << "NewObject\n";
        i++;
        break;
    case LoadThisOp:
        f << "LoadThis\n";
        i++;
        break;
    default:
        printFormat(f, "Unknown Opcode 0x%X\n", bcm.mCodeBase[i]);
        i++;
        break;
    }
    return i;
}

Formatter& operator<<(Formatter& f, const ByteCodeModule& bcm)
{
    uint32 i = 0;
    while (i < bcm.mLength) {
        printFormat(f, "%.4d ", i);
        i = printInstruction(f, i, bcm);
    }
    return f;
}

}
}

