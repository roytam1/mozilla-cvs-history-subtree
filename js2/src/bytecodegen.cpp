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

// this is the IdentifierList passed to the name lookup routines
#define CURRENT_ATTR    mNamespaceList

namespace JavaScript {    
namespace JS2Runtime {


void Reference::emitTypeOf(ByteCodeGen *bcg)
{
    emitCodeSequence(bcg); 
    bcg->addOp(TypeOfOp);
}

void Reference::emitDelete(ByteCodeGen *bcg)
{
    bcg->addOp(LoadConstantFalseOp);
}

void AccessorReference::emitCodeSequence(ByteCodeGen *bcg) 
{ 
    ASSERT(false);      // NYI
//    bcg->addOp(InvokeOp); 
//    bcg->addPointer(mFunction);
}

void LocalVarReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetLocalVarOp);
    else
        bcg->addOp(SetLocalVarOp);
    bcg->addLong(mIndex); 
}

void ParameterReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetArgOp);
    else
        bcg->addOp(SetArgOp);
    bcg->addLong(mIndex); 
}

void ClosureVarReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetClosureVarOp);
    else
        bcg->addOp(SetClosureVarOp);
    bcg->addLong(mDepth); 
    bcg->addLong(mIndex); 
}

void StaticFieldReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadTypeOp);
    bcg->addPointer(mClass);
}

void FieldReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadThisOp);
}

void FieldReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetFieldOp);
    else
        bcg->addOp(SetFieldOp);
    bcg->addLong(mIndex); 
}

void StaticFieldReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetStaticFieldOp);
    else
        bcg->addOp(SetStaticFieldOp);
    bcg->addLong(mIndex); 
}

void MethodReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadThisOp);
}

void MethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetMethodRefOp);
    bcg->addLong(mIndex); 
}

void MethodReference::emitInvokeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetMethodOp);
    bcg->addLong(mIndex); 
}

void GetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetMethodOp);
    bcg->addLong(mIndex); 
    bcg->addOpAdjustDepth(InvokeOp, -1);    // function, 'this' --> result
    bcg->addLong(0);
    bcg->addByte(Explicit);
}

void SetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOpAdjustDepth(InvokeOp, -2);    // leaves value on stack
    bcg->addLong(1);
    bcg->addByte(Explicit);
}

bool SetterMethodReference::emitPreAssignment(ByteCodeGen *bcg) 
{
    bcg->addOp(GetMethodOp);
    bcg->addLong(mIndex);
    return true;
}



void StaticFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetStaticMethodRefOp);
    bcg->addLong(mIndex); 
}

void StaticFunctionReference::emitInvokeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetStaticMethodOp);
    bcg->addLong(mIndex); 
}

void StaticGetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetStaticMethodOp);
    bcg->addLong(mIndex); 
    bcg->addOpAdjustDepth(InvokeOp, 0);
    bcg->addLong(0);
    bcg->addByte(NoThis);
}

void StaticSetterMethodReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOpAdjustDepth(InvokeOp, 0);
    bcg->addLong(1);
    bcg->addByte(NoThis);
}

bool StaticSetterMethodReference::emitPreAssignment(ByteCodeGen *bcg) 
{
    bcg->addOp(GetStaticMethodOp);
    bcg->addLong(mIndex);
    return true;
}



void FunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadFunctionOp);
    bcg->addPointer(mFunction);
}

void ConstructorReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetStaticMethodOp);
    bcg->addLong(mIndex); 
}

void GetterFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadFunctionOp);
    bcg->addPointer(mFunction);
    bcg->addOpAdjustDepth(InvokeOp, -1);
    bcg->addLong(0);
    bcg->addByte(Explicit);
}

void SetterFunctionReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadFunctionOp);
    bcg->addPointer(mFunction);
}

void SetterFunctionReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    bcg->addOpAdjustDepth(InvokeOp, -1);
    bcg->addLong(1);
    bcg->addByte(Explicit);
}

void NameReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetNameOp);
    else
        bcg->addOp(SetNameOp);
    bcg->addStringRef(mName); 
}

void NameReference::emitTypeOf(ByteCodeGen *bcg)
{
    bcg->addOp(GetTypeOfNameOp);
    bcg->addStringRef(mName); 
    bcg->addOp(TypeOfOp);
}

void NameReference::emitDelete(ByteCodeGen *bcg)
{
    bcg->addOp(DeleteOp);
    bcg->addStringRef(mName); 
}


void PropertyReference::emitImplicitLoad(ByteCodeGen *bcg) 
{
    bcg->addOp(LoadGlobalObjectOp);
}

void PropertyReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetPropertyOp);
    else
        bcg->addOp(SetPropertyOp);
    bcg->addStringRef(mName); 
}

void PropertyReference::emitInvokeSequence(ByteCodeGen *bcg) 
{
    bcg->addOp(GetInvokePropertyOp);
    bcg->addStringRef(mName); 
}

void PropertyReference::emitDelete(ByteCodeGen *bcg)
{
    bcg->addOp(DeleteOp);
    bcg->addStringRef(mName); 
}

void ElementReference::emitCodeSequence(ByteCodeGen *bcg) 
{
    if (mAccess == Read)
        bcg->addOp(GetElementOp);
    else
        bcg->addOp(SetElementOp);
}

ByteCodeData gByteCodeData[OpCodeCount] = {
{ 1,        "LoadConstantUndefined", },
{ 1,        "LoadConstantTrue", },
{ 1,        "LoadConstantFalse", },
{ 1,        "LoadConstantNull", },
{ 1,        "LoadConstantNumber", },
{ 1,        "LoadConstantString", },
{ 1,        "LoadThis", },
{ 1,        "LoadFunction", },
{ 1,        "LoadType", },
{ -128,     "Invoke", },
{ 0,        "GetType", },
{ -1,       "Cast", },
{ 0,        "DoUnary", },
{ -1,       "DoOperator", },
{ 1,        "PushNull", },
{ 1,        "PushInt", },
{ 1,        "PushNum", },
{ 1,        "PushString", },
{ 1,        "PushType", },
{ -128,     "Return", },
{ -128,     "ReturnVoid", },
{ 0,        "GetConstructor", },
{ 1,        "NewObject", },
{ -1,       "NewThis", },
{ -1,       "NewInstance", },
{ 0,        "Delete", },
{ 0,        "TypeOf", },
{ -1,       "InstanceOf", },
{ -1,       "At", },
{ 0,        "ToBoolean", },
{ -1,       "JumpFalse", },
{ -1,       "JumpTrue", },
{ 0,        "Jump", },
{ 0,        "Try", },
{ 0,        "Jsr", },
{ 0,        "Rts", },
{ -1,       "Within", },
{ 0,        "Without", },
{ -128,     "Throw", },
{ 0,        "Handler", },
{ -3,       "LogicalXor", },
{ 0,        "LogicalNot", },
{ 0,        "Swap", },
{ 1,        "Dup", },
{ 1,        "DupInsert", },
{ -128,     "DupN", },
{ -128,     "DupInsertN", },
{ -1,       "Pop", },
{ 0,        "GetField", },
{ -1,       "SetField", },
{ 0,        "GetStaticField", },
{ -1,       "SetStaticField", },
{ 1,        "GetMethod", },
{ 0,        "GetMethodRef", },
{ 0,        "GetStaticMethod", },
{ 0,        "GetStaticMethodRef", },
{ 1,        "GetArg", },
{ 0,        "SetArg", },
{ 1,        "GetLocalVar", },
{ 0,        "SetLocalVar", },
{ 1,        "GetClosureVar", },
{ 0,        "SetClosureVar", },
{ -1,       "GetElement", },
{ -2,       "SetElement", },
{ 0,        "GetProperty", },
{ 1,        "GetInvokeProperty", },
{ -1,       "SetProperty", },
{ 1,        "GetName", },
{ 1,        "GetTypeOfName", },
{ 0,        "SetName", },
{ 1,        "LoadGlobalObject", },
{ 0,        "PushScope", },
{ 0,        "PopScope", },
{ 0,        "NewClosure" },

};

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
    mStackDepth = bcg->mStackMax;
}

void ByteCodeGen::genCodeForFunction(FunctionDefinition &f, JSFunction *fnc, bool isConstructor, JSType *topClass)
{
    mScopeChain->addScope(fnc->mParameterBarrel);
    mScopeChain->addScope(&fnc->mActivation);
    // OPT - no need to push the parameter and function
    // scopes if the function doesn't contain any 'eval'
    // calls, all other references to the variables mapped
    // inside these scopes will have been turned into
    // localVar references.
/*
    addByte(PushScopeOp);
    addPointer(fnc->mParameterBarrel);
    addByte(PushScopeOp);   
    addPointer(&fnc->mActivation);
*/

#ifdef DEBUG
    if (f.name && (f.name->getKind() == ExprNode::identifier)) {
        const StringAtom& name = (static_cast<IdentifierExprNode *>(f.name))->name;
//      stdOut << "gencode for " << name << "\n";
    }
#endif

    if (isConstructor) {
        //
        // add a code sequence to create a new empty instance if the
        // incoming 'this' is null
        //
        addOp(LoadTypeOp);
        addPointer(topClass);
        addOp(NewThisOp);
        //
        //  Invoke the super class constructor if there isn't an explicit
        // statement to do so.
        //  
        bool foundSuperCall = false;
        BlockStmtNode *b = f.body;
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
                            if ((b->op1->getKind() == ExprNode::superExpr) && (b->op2->getKind() == ExprNode::identifier)) {
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
                            if (i->op->getKind() == ExprNode::superExpr) {
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
                    addOp(LoadThisOp);
                    addOp(LoadFunctionOp);
                    addPointer(superConstructor);
                    addOpAdjustDepth(InvokeOp, -1);
                    addLong(0);
                    addByte(Explicit);
                    addOp(PopOp);
                }
            }
        }
    }

    genCodeForStatement(f.body, NULL);
    
/*
    // OPT - see above
    addByte(PopScopeOp);
    addByte(PopScopeOp);
*/    
    if (isConstructor) {        // the codegen model depends on all constructors returning the 'this'
        addOp(LoadThisOp);
        ASSERT(mStackTop == 1);
        addOpSetDepth(ReturnOp, 0);
    }
    else {
        // if there is no return statement, add one
        // XXX obviously there are better things that can be done    
        addOp(LoadConstantUndefinedOp);
        ASSERT(mStackTop == 1);
        addOpSetDepth(ReturnOp, 0);
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
//    ASSERT(mStackTop == 0);
    switch (p->getKind()) {
    case StmtNode::Class:
        {
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            JSType *thisClass = classStmt->mType;

            mScopeChain->addScope(thisClass->mStatics);
            mScopeChain->addScope(thisClass);
            if (classStmt->body) {
                ByteCodeGen static_cg(m_cx, mScopeChain);       // this will capture the static initializations
                ByteCodeGen bcg(m_cx, mScopeChain);             // this will capture the instance initializations
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    bcg.genCodeForStatement(s, &static_cg);
                    s = s->next;
                }
                JSFunction *f = NULL;
                if (static_cg.hasContent()) {
                    // build a function to be invoked 
                    // when the class is loaded
                    f = new JSFunction(m_cx, Void_Type, 0, mScopeChain);
                    f->mByteCode = new ByteCodeModule(&static_cg);
                }
                thisClass->setStaticInitializer(m_cx, f);
                f = NULL;
                if (bcg.hasContent()) {
                    // execute this function now to form the initial instance
                    f = new JSFunction(m_cx, Void_Type, 0, mScopeChain);
                    f->mByteCode = new ByteCodeModule(&bcg);
                }
                thisClass->setInstanceInitializer(m_cx, f);
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
                            ref->emitImplicitLoad(static_cg);
                            static_cg->genExpr(v->initializer);
                            ref->emitCodeSequence(static_cg);
                            static_cg->addOp(PopOp);
                        }
                        else {
                            ref->emitImplicitLoad(this);
                            genExpr(v->initializer);
                            ref->emitCodeSequence(this);
                            addOp(PopOp);
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
            bool isConstructor = hasAttribute(f->attributes, m_cx->ConstructorKeyWord);
            JSFunction *fnc = f->mFunction;    

            if (f->function.name->getKind() == ExprNode::identifier) {
                const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                if (mScopeChain->topClass() 
                            && (mScopeChain->topClass()->mClassName.compare(name) == 0))
                    isConstructor = true;
            }
            ByteCodeGen bcg(m_cx, mScopeChain);
            bcg.genCodeForFunction(f->function, fnc, isConstructor, mScopeChain->topClass());
        }
        break;
    case StmtNode::While:
        {
            UnaryStmtNode *w = static_cast<UnaryStmtNode *>(p);
            addOp(JumpOp);
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
            addOp(ToBooleanOp);
            addOp(JumpTrueOp);
            addFixup(labelAtTopOfBlock);
            setLabel(breakLabel);
        }
        break;
    case StmtNode::DoWhile:
        {
            UnaryStmtNode *d = static_cast<UnaryStmtNode *>(p);
            uint32 breakLabel = getLabel(Label::BreakLabel);
            uint32 labelAtTopOfBlock = getLabel();
            uint32 labelAtTestCondition = getLabel(Label::ContinueLabel); 
            setLabel(labelAtTopOfBlock);

            mLabelStack.push_back(breakLabel);
            mLabelStack.push_back(labelAtTestCondition);
            genCodeForStatement(d->stmt, static_cg);
            mLabelStack.pop_back();
            mLabelStack.pop_back();

            setLabel(labelAtTestCondition);
            genExpr(d->expr);
            addOp(ToBooleanOp);
            addOp(JumpTrueOp);
            addFixup(labelAtTopOfBlock);
            setLabel(breakLabel);
        }
        break;
    case StmtNode::ForIn:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            if (f->initializer->getKind() == StmtNode::Var) {
                VariableStmtNode *vs = static_cast<VariableStmtNode *>(f->initializer);
                VariableBinding *v = vs->bindings;
                IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                Reference *value = mScopeChain->getName(i->name, CURRENT_ATTR, Write);
                
                uint32 breakLabel = getLabel(Label::BreakLabel);
                uint32 labelAtTopOfBlock = getLabel();
                uint32 labelAtIncrement = getLabel(Label::ContinueLabel); 
                uint32 labelAtTestCondition = getLabel(); 
                uint32 labelAtEnd = getLabel(); 
/*
                iterator = object.forin()
                goto test
                top:
                    v = iterator.value
                    <statement body>
                continue:
                    iterator = object.next(iterator)
                test:
                    if (iterator == null)
                        goto end
                    goto top                    
                break:
                    object.done(iterator)
                end:
*/

                // acquire a local from the scopechain, and copy the target object
                // into it.
                Reference *objectReadRef, *objectWriteRef;
                Reference *iteratorReadRef, *iteratorWriteRef;
                mScopeChain->defineTempVariable(objectReadRef, objectWriteRef, Object_Type);
                mScopeChain->defineTempVariable(iteratorReadRef, iteratorWriteRef, Object_Type);


                    genExpr(f->expr2);
                    objectWriteRef->emitCodeSequence(this);
                    addOp(DupOp);
                    addOp(GetInvokePropertyOp);
//                    addIdentifierRef(widenCString("Iterator"), widenCString("forin"));
                    addStringRef(widenCString("forin"));
                    addOpAdjustDepth(InvokeOp, -1);
                    addLong(0);
                    addByte(Explicit);
                    iteratorWriteRef->emitCodeSequence(this);
                
                    addOp(JumpOp);
                    addFixup(labelAtTestCondition);

                setLabel(labelAtTopOfBlock);
                    iteratorReadRef->emitCodeSequence(this);
                    addOp(GetPropertyOp);
                    addStringRef(widenCString("value"));
                    value->emitCodeSequence(this);

                    mLabelStack.push_back(breakLabel);
                    mLabelStack.push_back(labelAtIncrement);
                    genCodeForStatement(f->stmt, static_cg);
                    mLabelStack.pop_back();
                    mLabelStack.pop_back();

                setLabel(labelAtIncrement);
                    objectReadRef->emitCodeSequence(this);
                    addOp(DupOp);
                    addOp(GetInvokePropertyOp);
                    addStringRef(widenCString("next"));
                    iteratorReadRef->emitCodeSequence(this);
                    addOpAdjustDepth(InvokeOp, -1);
                    addLong(1);
                    addByte(Explicit);
                    iteratorWriteRef->emitCodeSequence(this);
                    addOp(PopOp);

                setLabel(labelAtTestCondition);
                    iteratorReadRef->emitCodeSequence(this);
                    addOp(LoadConstantNullOp);
                    addOp(DoOperatorOp);
                    addByte(Equal);
                    addOp(JumpTrueOp);
                    addFixup(labelAtEnd);
                    addOp(JumpOp);
                    addFixup(labelAtTopOfBlock);

                setLabel(breakLabel);
                    objectReadRef->emitCodeSequence(this);
                    addOp(DupOp);
                    addOp(GetInvokePropertyOp);
                    addStringRef(widenCString("done"));
                    iteratorReadRef->emitCodeSequence(this);
                    addOpAdjustDepth(InvokeOp, -2);
                    addLong(1);
                    addByte(Explicit);
                    addOp(PopOp);

                setLabel(labelAtEnd);


            }
            else
                NOT_REACHED("implement me");
        }
        break;
    case StmtNode::For:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            uint32 breakLabel = getLabel(Label::BreakLabel);
            uint32 labelAtTopOfBlock = getLabel();
            uint32 labelAtIncrement = getLabel(Label::ContinueLabel); 
            uint32 labelAtTestCondition = getLabel(); 

            if (f->initializer) 
                genCodeForStatement(f->initializer, static_cg);
            addOp(JumpOp);
            addFixup(labelAtTestCondition);

            setLabel(labelAtTopOfBlock);

            mLabelStack.push_back(breakLabel);
            mLabelStack.push_back(labelAtIncrement);
            genCodeForStatement(f->stmt, static_cg);
            mLabelStack.pop_back();
            mLabelStack.pop_back();

            setLabel(labelAtIncrement);
            if (f->expr3) {
                genExpr(f->expr3);
                addOp(PopOp);
            }

            setLabel(labelAtTestCondition);
            if (f->expr2) {
                genExpr(f->expr2);
                addOp(ToBooleanOp);
                addOp(JumpTrueOp);
                addFixup(labelAtTopOfBlock);
            }

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
            addOp(JumpOp);
            if (g->name)
                addFixup(getTopLabel(Label::BreakLabel, g->name));
            else
                addFixup(getTopLabel(Label::BreakLabel));
        }
        break;
    case StmtNode::Continue:
        {
            GoStmtNode *g = static_cast<GoStmtNode *>(p);
            addOp(JumpOp);
            if (g->name)
                addFixup(getTopLabel(Label::ContinueLabel, g->name));
            else
                addFixup(getTopLabel(Label::ContinueLabel));
        }
        break;
    case StmtNode::Switch:
        {
/*
            <swexpr>        
            SetVarOp    <switchTemp>
            Pop

        // test sequence in source order except 
        // the default is moved to end.

            GetVarOp    <switchTemp>
            <case1expr>
            Equal
            JumpTrue --> case1StmtLabel
            GetVarOp    <switchTemp>
            <case2expr>
            Equal
            JumpTrue --> case2StmtLabel
            Jump --> default, if there is one, or break label

    case1StmtLabel:
            <stmt>
    case2StmtLabel:
            <stmt>
    defaultLabel:
            <stmt>
    case3StmtLabel:
            <stmt>
            ..etc..     // all in source order
    
    breakLabel:
*/
            uint32 breakLabel = getLabel(Label::BreakLabel);
            uint32 defaultLabel = (uint32)(-1);

            Reference *switchTempReadRef, *switchTempWriteRef;
            mScopeChain->defineTempVariable(switchTempReadRef, switchTempWriteRef, Object_Type);

            SwitchStmtNode *sw = static_cast<SwitchStmtNode *>(p);
            genExpr(sw->expr);
            switchTempWriteRef->emitCodeSequence(this);
            addOp(PopOp);

            StmtNode *s = sw->statements;
            while (s) {
                if (s->getKind() == StmtNode::Case) {
                    ExprStmtNode *c = static_cast<ExprStmtNode *>(s);
                    c->label = getLabel();
                    if (c->expr) {
                        switchTempReadRef->emitCodeSequence(this);
                        genExpr(c->expr);
                        addOp(DoOperatorOp);
                        addByte(Equal);
                        addOp(JumpTrueOp);
                        addFixup(c->label);
                    }
                    else
                        defaultLabel = c->label;
                }
                s = s->next;
            }
            addOp(JumpOp);
            if (defaultLabel != -1)
                addFixup(defaultLabel);
            else
                addFixup(breakLabel);          
            
            s = sw->statements;
            mLabelStack.push_back(breakLabel);
            while (s) {
                if (s->getKind() == StmtNode::Case) {
                    ExprStmtNode *c = static_cast<ExprStmtNode *>(s);
                    setLabel(c->label);
                }
                else
                    genCodeForStatement(s, static_cg);
                s = s->next;
            }
            mLabelStack.pop_back();
            setLabel(breakLabel);
        }
        break;
    case StmtNode::If:
        {
            UnaryStmtNode *i = static_cast<UnaryStmtNode *>(p);
            genExpr(i->expr);
            addOp(ToBooleanOp);
            addOp(JumpFalseOp);
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
            addOp(ToBooleanOp);
            addOp(JumpFalseOp);
            uint32 elseStatementLabel = getLabel(); 
            addFixup(elseStatementLabel);
            genCodeForStatement(i->stmt, static_cg);
            addOp(JumpOp);
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
            if (e->expr) {
                genExpr(e->expr);
                ASSERT(mStackTop == 1);
                addOpSetDepth(ReturnOp, 0);
            }
            else {
                ASSERT(mStackTop == 0);
                addOpSetDepth(ReturnVoidOp, 0);
            }
        }
        break;
    case StmtNode::expression:
        {
            ExprStmtNode *e = static_cast<ExprStmtNode *>(p);
            genExpr(e->expr);
            addOp(PopOp);
        }
        break;
    case StmtNode::empty:
        /* nada */
        break;
    case StmtNode::Throw:
        {
            ExprStmtNode *e = static_cast<ExprStmtNode *>(p);
            genExpr(e->expr);
            addOpSetDepth(ThrowOp, 0);
        }
        break;
    case StmtNode::With:
        {
            UnaryStmtNode *w = static_cast<UnaryStmtNode *>(p);
            JSType *objType = genExpr(w->expr);
            addOp(WithinOp);
            mScopeChain->addScope(objType);
            genCodeForStatement(w->stmt, static_cg);
            mScopeChain->popScope();
            addOp(WithoutOp);
        }
        break;
    case StmtNode::Try:
        {
/*

            try {   //  [catch,finally] handler labels are pushed on try stack
                    <tryblock>
                }   //  catch handler label is popped off try stack
                jsr finally
                jump-->finished                 

            finally:        // finally handler label popped off
                {           // a throw from in here goes to the 'next' handler
                }
                rts

            finallyInvoker:              <---
                push exception              |
                jsr finally                 |--- the handler labels 
                throw exception             | 
                                            |
            catchLabel:                  <---
                catch (exception) { // catch handler label popped off
                        // any throw from in here must jump to the finallyInvoker
                        // (i.e. not the catch handler!)
                    the incoming exception
                    is on the top of the stack. it
                    get stored into the variable
                    we've associated with the catch clause

                }
                // 'normal' fall thru from catch
                jsr finally

            finished:
*/
            TryStmtNode *t = static_cast<TryStmtNode *>(p);

            uint32 catchClauseLabel = (t->catches) ? getLabel() : (uint32)(-1);
            uint32 finallyInvokerLabel = (t->finally) ? getLabel() : (uint32)(-1);
            uint32 finishedLabel = getLabel();

            addOp(TryOp);
            addFixup(finallyInvokerLabel);            
            addFixup(catchClauseLabel);

            genCodeForStatement(t->stmt, static_cg);

            uint32 finallyLabel;
            if (t->finally) {
                finallyLabel = getLabel(); 
                addOp(JsrOp);
                addFixup(finallyLabel);
                addOp(JumpOp);
                addFixup(finishedLabel);
                setLabel(finallyLabel);
                addOp(HandlerOp);
                genCodeForStatement(t->finally, static_cg);
                addOp(RtsOp);

                setLabel(finallyInvokerLabel);
                // the exception object is on the top of the stack already
                addOp(JsrOp);
                addFixup(finallyLabel);
                addOp(ThrowOp);

            }
            else {
                addOp(JumpOp);
                addFixup(finishedLabel);
            }

            if (t->catches) {
                setLabel(catchClauseLabel);
                addOp(HandlerOp);
                CatchClause *c = t->catches;
                ASSERT(mStackTop == 0);
                mStackTop = 1;
                if (mStackMax < 1) mStackMax = 1;
                while (c) {
                    Reference *ref = mScopeChain->getName(c->name, CURRENT_ATTR, Write);
                    ref->emitImplicitLoad(this);
                    ref->emitCodeSequence(this);
                    delete ref;
                    genCodeForStatement(c->stmt, static_cg);
                    c = c->next;
                    if (c) {
                        mStackTop = 1;
                        Reference *ref = mScopeChain->getName(c->name, CURRENT_ATTR, Read);
                        ref->emitCodeSequence(this);
                        delete ref;
                    }
                }
                addOp(PopOp);       // the exception object has persisted 
                                    // on the top of the stack until here
                if (t->finally) {
                    addOp(JsrOp);
                    addFixup(finallyLabel);
                }
            }
            setLabel(finishedLabel);
        }
        break;
    case StmtNode::Use:
        {
            UseStmtNode *u = static_cast<UseStmtNode *>(p);
            ExprList *eList = u->exprs;
            while (eList) {
                ExprNode *e = eList->expr;
                if (e->getKind() == ExprNode::identifier) {
                    AttributeList *id = new(m_cx->mArena) AttributeList(e);
                    id->next = CURRENT_ATTR;
                }
                else
                    NOT_REACHED("implement me");
                eList = eList->next;
            }

        }
        break;
    case StmtNode::Namespace:
        {
            // do anything at bytecodegen?
        }
        break;
    default:
        NOT_REACHED("Not Implemented Yet");
    }
}

Reference *ByteCodeGen::genReference(ExprNode *p, Access acc)
{
    switch (p->getKind()) {
    case ExprNode::index:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            genExpr(i->op);
            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }
            Reference *ref = new ElementReference(acc, argCount);
            return ref;
        }
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
            // a newInstanceOp). 
            if (b->op1->getKind() == ExprNode::identifier) {
                const StringAtom &name = static_cast<IdentifierExprNode *>(b->op1)->name;
                JSValue v = mScopeChain->getCompileTimeValue(name, NULL);
                if (v.isType() && v.type->mStatics) {
                    lType = v.type->mStatics;
                    genExpr(b->op1);
                }
            }

            if (lType == NULL)
                lType = genExpr(b->op1);    // generate code for leftside of dot
            if (b->op2->getKind() == ExprNode::qualify) {
                BinaryExprNode *q = static_cast<BinaryExprNode *>(b->op2);
                ASSERT(q->op1->getKind() == ExprNode::identifier);
                ASSERT(q->op2->getKind() == ExprNode::identifier);
                const StringAtom &fieldName = static_cast<IdentifierExprNode *>(q->op2)->name;
//                const StringAtom &qualifierName = static_cast<IdentifierExprNode *>(q->op1)->name;
                AttributeList id(q->op1);
                id.next = CURRENT_ATTR;
                Reference *ref = lType->genReference(fieldName, &id, acc, 0);
                if (ref == NULL)
                    ref = new PropertyReference(fieldName, acc, Object_Type);
                return ref;
            }
            else {
                ASSERT(b->op2->getKind() == ExprNode::identifier);
                const StringAtom &fieldName = static_cast<IdentifierExprNode *>(b->op2)->name;
                Reference *ref = lType->genReference(fieldName, CURRENT_ATTR, acc, 0);
                if (ref == NULL)
                    ref = new PropertyReference(fieldName, acc, Object_Type);
                return ref;
            }

        }
/*
    default:
        NOT_REACHED("Invalid l-value");      // XXX should be a semantic error
        return NULL;
*/
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
    case ExprNode::index:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            genExpr(i->op);
            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }
            readRef = new ElementReference(Read, argCount);
            writeRef = new ElementReference(Write, argCount);
        }
        break;
    case ExprNode::dot:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            
            JSType *lType = NULL;

            if (b->op1->getKind() == ExprNode::identifier) {
                const StringAtom &name = static_cast<IdentifierExprNode *>(b->op1)->name;
                JSValue v = mScopeChain->getCompileTimeValue(name, NULL);
                if (v.isType() && v.type->mStatics) {
                    lType = v.type->mStatics;
                    genExpr(b->op1);
                }
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
                    readRef = new PropertyReference(fieldName, Read, Object_Type);
                writeRef = lType->genReference(fieldName, CURRENT_ATTR, Write, 0);
                if (writeRef == NULL)
                    writeRef = new PropertyReference(fieldName, Write, Object_Type);
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
        addOp(LoadConstantTrueOp);
        return Boolean_Type;
    case ExprNode::False:
        addOp(LoadConstantFalseOp);
        return Boolean_Type;
    case ExprNode::Null:
        addOp(LoadConstantNullOp);
        return Object_Type;
    case ExprNode::number :
        addOp(LoadConstantNumberOp);
        addNumberRef((static_cast<NumberExprNode *>(p))->value);
        return Number_Type;
    case ExprNode::string :
        addOp(LoadConstantStringOp);
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
            addOp(DoOperatorOp);
            addByte(op);
            return Object_Type;
        }

    case ExprNode::notEqual:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(DoOperatorOp);
            addByte(Equal);
            addOp(LogicalNotOp);
            return Object_Type;
        }
    case ExprNode::greaterThan:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(SwapOp);
            addOp(DoOperatorOp);
            addByte(Less);
            return Object_Type;
        }
    case ExprNode::greaterThanOrEqual:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(SwapOp);
            addOp(DoOperatorOp);
            addByte(LessEqual);
            return Object_Type;
        }
    case ExprNode::notIdentical:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(DoOperatorOp);
            addByte(SpittingImage);
            addOp(LogicalNotOp);
            return Object_Type;
        }

    case ExprNode::minus:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addOp(DoUnaryOp);
            addByte(Negate);
            return Object_Type;
        }

    case ExprNode::plus:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addOp(DoUnaryOp);
            addByte(Posate);
            return Object_Type;
        }

    case ExprNode::complement:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addOp(DoUnaryOp);
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
            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupNOp, -baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
                readRef->emitCodeSequence(this);
                addOpStretchStack(DoUnaryOp, 1);
                addByte(op);
            }
            else {
                readRef->emitCodeSequence(this);
                addOpStretchStack(DoUnaryOp, 1);
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
            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupNOp, baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
                readRef->emitCodeSequence(this);
                   // duplicate the value and bury it
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupInsertNOp, baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupInsertOp);
            }
            else {
                readRef->emitCodeSequence(this);
                addOp(DupOp);
            }
            addOpStretchStack(DoUnaryOp, 1);
            addByte(op);
            writeRef->emitCodeSequence(this);
            addOp(PopOp);     // because the SetXXX will propogate the new value
            return Object_Type;
        }

    case ExprNode::addEquals:
        op = Plus;
        goto BinaryOpEquals;
    case ExprNode::subtractEquals:
        op = Minus;
        goto BinaryOpEquals;
    case ExprNode::multiplyEquals:
        op = Multiply;
        goto BinaryOpEquals;
    case ExprNode::divideEquals:
        op = Divide;
        goto BinaryOpEquals;
    case ExprNode::moduloEquals:
        op = Remainder;
        goto BinaryOpEquals;
    case ExprNode::leftShiftEquals:
        op = ShiftLeft;
        goto BinaryOpEquals;
    case ExprNode::rightShiftEquals:
        op = ShiftRight;
        goto BinaryOpEquals;
    case ExprNode::logicalRightShiftEquals:
        op = UShiftRight;
        goto BinaryOpEquals;
    case ExprNode::bitwiseAndEquals:
        op = BitAnd;
        goto BinaryOpEquals;
    case ExprNode::bitwiseXorEquals:
        op = BitXor;
        goto BinaryOpEquals;
    case ExprNode::bitwiseOrEquals:
        op = BitOr;
        goto BinaryOpEquals;

BinaryOpEquals:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;
            genReferencePair(b->op1, readRef, writeRef);

            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOp(DupNOp);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
            }
            if (writeRef->emitPreAssignment(this))
                addOp(SwapOp);
            readRef->emitCodeSequence(this);
            genExpr(b->op2);
            addOp(DoOperatorOp);
            addByte(op);
            writeRef->emitCodeSequence(this);
            return Object_Type;
        }


    case ExprNode::logicalAndEquals:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;
            genReferencePair(b->op1, readRef, writeRef);

            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupNOp, -baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
            }

            uint32 labelAfterSecondExpr = getLabel();
            if (writeRef->emitPreAssignment(this))
                addOp(SwapOp);
            readRef->emitCodeSequence(this);
            addOp(DupOp);
            addOp(ToBooleanOp);
            addOp(JumpFalseOp);
            addFixup(labelAfterSecondExpr);
            addOp(PopOp);
            genExpr(b->op2);
            setLabel(labelAfterSecondExpr);    
            writeRef->emitCodeSequence(this);
            return Object_Type;
        }

    case ExprNode::logicalOrEquals:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;
            genReferencePair(b->op1, readRef, writeRef);

            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupNOp, -baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
            }

            uint32 labelAfterSecondExpr = getLabel();
            if (writeRef->emitPreAssignment(this))
                addOp(SwapOp);
            readRef->emitCodeSequence(this);
            addOp(DupOp);
            addOp(ToBooleanOp);
            addOp(JumpTrueOp);
            addFixup(labelAfterSecondExpr);
            addOp(PopOp);
            genExpr(b->op2);
            setLabel(labelAfterSecondExpr);    
            writeRef->emitCodeSequence(this);
            return Object_Type;
        }

    case ExprNode::logicalXorEquals:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *readRef;
            Reference *writeRef;
            genReferencePair(b->op1, readRef, writeRef);

            uint32 baseDepth = readRef->baseExpressionDepth();
            if (baseDepth) {         // duplicate the base expression
                if (baseDepth > 1) {
                    addOpAdjustDepth(DupNOp, -baseDepth);
                    addByte(baseDepth);
                }
                else
                    addOp(DupOp);
            }

            if (writeRef->emitPreAssignment(this))
                addOp(SwapOp);
            readRef->emitCodeSequence(this);
            genExpr(b->op2);
            addOp(LogicalXorOp);
            writeRef->emitCodeSequence(this);
            return Object_Type;
        }

    case ExprNode::logicalAnd:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            uint32 labelAfterSecondExpr = getLabel();
            genExpr(b->op1);
            addOp(DupOp);
            addOp(ToBooleanOp);
            addOp(JumpFalseOp);
            addFixup(labelAfterSecondExpr);
            addOp(PopOp);
            genExpr(b->op2);
            setLabel(labelAfterSecondExpr);            
            return Object_Type;
        }
    case ExprNode::logicalXor:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            addOp(DupOp);
            addOp(ToBooleanOp);
            genExpr(b->op2);
            addOp(DupInsertOp);
            addOp(ToBooleanOp);            
            addOp(LogicalXorOp);
            return Object_Type;
        }
    case ExprNode::logicalOr:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            uint32 labelAfterSecondExpr = getLabel();
            genExpr(b->op1);
            addOp(DupOp);
            addOp(ToBooleanOp);
            addOp(JumpTrueOp);
            addFixup(labelAfterSecondExpr);
            addOp(PopOp);
            genExpr(b->op2);
            setLabel(labelAfterSecondExpr);            
            return Object_Type;
        }
        
    case ExprNode::logicalNot:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            genExpr(u->op);
            addOp(ToBooleanOp);
            addOp(LogicalNotOp);
            return Boolean_Type;
        }

    case ExprNode::assignment:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            Reference *ref = genReference(b->op1, Write);
            ref->emitPreAssignment(this);
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
    case ExprNode::This:
        {
            addOp(LoadThisOp);
            return Object_Type; // XXX find class we're in for methods
        }
    case ExprNode::dot:
        {
            Reference *ref = genReference(p, Read);
            ref->emitCodeSequence(this);
            JSType *type = ref->mType;
            delete ref;
            return type;
        }
    case ExprNode::Delete:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            Reference *ref = genReference(u->op, Read);
            if (ref == NULL)
                addOp(LoadConstantTrueOp);
            else {
                ref->emitDelete(this);
                delete ref;
            }
            return Boolean_Type;
        }
    case ExprNode::Typeof:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            Reference *ref = genReference(u->op, Read);
            if (ref == NULL) {
                genExpr(u->op);
                addOp(TypeOfOp);
            }
            else {
                ref->emitTypeOf(this);
                delete ref;
            }
            return String_Type;
        }
    case ExprNode::New:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            JSType *type = genExpr(i->op);

            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }
            addOpAdjustDepth(NewInstanceOp, -argCount);
            addLong(argCount);
            return type;
        }
    case ExprNode::index:
        {
            Reference *ref = genReference(p, Read);
            ref->emitCodeSequence(this);
            return Object_Type;
        }
    case ExprNode::call:
        {
            InvokeExprNode *i = static_cast<InvokeExprNode *>(p);
            Reference *ref = genReference(i->op, Read);

            // if the reference is the name of a type, then this
            // is a cast of the argument to that type.
            //if (ref->isTypeName()) {
            //    addByte(LoadTypeOp);
            //    addPointer(ref->getType());
            //    ASSERT(i->pairs);
            //    genExpr(i->pairs);
            //    ASSERT(i->pairs->next == NULL);
            //    addByte(CastOp);
            //}
            ref->emitInvokeSequence(this);

            ExprPairList *p = i->pairs;
            uint32 argCount = 0;
            while (p) {
                genExpr(p->value);
                argCount++;
                p = p->next;
            }

            if (ref->needsThis()) {
                addOpAdjustDepth(InvokeOp, -(argCount + 1));
                addLong(argCount);
                addByte(Explicit);
            }
            else {
                addOpAdjustDepth(InvokeOp, -argCount);
                addLong(argCount);
                addByte(NoThis);
            }
            JSType *type = ref->mType;
            delete ref;
            return type;
        }
    case ExprNode::parentheses:
        {
            UnaryExprNode *u = static_cast<UnaryExprNode *>(p);
            return genExpr(u->op);
        }
    case ExprNode::conditional:
        {
            uint32 falseConditionExpression = getLabel();
            uint32 labelAtBottom = getLabel();

            TernaryExprNode *c = static_cast<TernaryExprNode *>(p);
            genExpr(c->op1);
            addOp(ToBooleanOp);
            addOp(JumpFalseOp);
            addFixup(falseConditionExpression);
            genExpr(c->op2);
            addOp(JumpOp);
            addFixup(labelAtBottom);
            setLabel(falseConditionExpression);
            adjustStack(-1);        // the true case will leave a stack entry pending
                                    // but we can discard it since only path will be taken.
            genExpr(c->op3);
            setLabel(labelAtBottom);
            return Object_Type;
        }
    case ExprNode::objectLiteral:
        {
            addOp(NewObjectOp);
            PairListExprNode *plen = static_cast<PairListExprNode *>(p);
            ExprPairList *e = plen->pairs;
            while (e) {
                if (e->field && e->value && (e->field->getKind() == ExprNode::identifier)) {
                    addOp(DupOp);
                    genExpr(e->value);
                    addOp(SetPropertyOp);
                    addStringRef((static_cast<IdentifierExprNode *>(e->field))->name);
                    addOp(PopOp);
                }
                e = e->next;
            }
        }
        break;
    case ExprNode::arrayLiteral:
        {
            addOp(LoadTypeOp);
            addPointer(Array_Type);
            addOpAdjustDepth(NewInstanceOp, 0);
            addLong(0);
            PairListExprNode *plen = static_cast<PairListExprNode *>(p);
            ExprPairList *e = plen->pairs;
            int index = 0;
            while (e) {
                if (e->value) {
                    addOp(DupOp);
                    addOp(LoadConstantNumberOp);
                    addNumberRef(index);
                    genExpr(e->value);
                    addOp(SetElementOp);
                    addOp(PopOp);
                }
                index++;
                e = e->next;
            }
        }
        break;
    case ExprNode::Instanceof:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(InstanceOfOp);
        }
        break;
    case ExprNode::As:
        {
            BinaryExprNode *b = static_cast<BinaryExprNode *>(p);
            genExpr(b->op1);
            genExpr(b->op2);
            addOp(AsOp);
        }
        break;
    case ExprNode::numUnit:
        {
            // turn the unit string into a function call into the
            // Unit package, passing the number literal arguments
            // Requires winding up a new lexer/parser chunk. For
            // now we'll handle single units only
            NumUnitExprNode *n = static_cast<NumUnitExprNode *>(p);

            addOp(LoadTypeOp);
            addPointer(Unit_Type);
            addOp(GetInvokePropertyOp);
            addStringRef(n->str);
            addOp(LoadConstantNumberOp);
            addNumberRef(n->num);
            addOp(LoadConstantStringOp);
            addStringRef(n->numStr);
            addOp(InvokeOp);
            addLong(2);
            addOp(NoThis);
        }
        break;
    case ExprNode::functionLiteral:
        {
            FunctionExprNode *f = static_cast<FunctionExprNode *>(p);
            JSFunction *fnc = new JSFunction(m_cx, NULL, m_cx->getParameterCount(f->function), mScopeChain);
            m_cx->buildRuntimeForFunction(f->function, fnc);
            ByteCodeGen bcg(m_cx, mScopeChain);
            bcg.genCodeForFunction(f->function, fnc, false, NULL);
            addOp(LoadFunctionOp);
            addPointer(fnc);
            addOp(NewClosureOp);
        }
        break;
    default:
        NOT_REACHED("Not Implemented Yet");
    }
    return NULL;
}

int printInstruction(Formatter &f, int i, const ByteCodeModule& bcm)
{
    int32 offset;
    uint8 op = bcm.mCodeBase[i];
    f << gByteCodeData[op].opName << " ";
    i++;
    switch (op) {

    case LoadConstantUndefinedOp:
    case LoadConstantTrueOp:
    case LoadConstantFalseOp:
    case LoadConstantNullOp:
    case LoadThisOp:
    case GetTypeOp:
    case CastOp:
    case ReturnOp:
    case ReturnVoidOp:
    case GetConstructorOp:
    case NewObjectOp:
    case NewThisOp:
    case TypeOfOp:
    case InstanceOfOp:
    case AsOp:
    case ToBooleanOp:
    case RtsOp:
    case WithinOp:
    case WithoutOp:
    case ThrowOp:
    case HandlerOp:
    case LogicalXorOp:
    case LogicalNotOp:
    case SwapOp:
    case DupOp:
    case DupInsertOp:
    case PopOp:
    case GetElementOp:
    case SetElementOp:
    case LoadGlobalObjectOp:
    case NewClosureOp:
        break;

    case DoUnaryOp:
    case DupNOp:
    case DupInsertNOp:
    case DoOperatorOp:
        f << bcm.mCodeBase[i];
        i++;
        break;
    
    case JumpOp:
    case JumpTrueOp:
    case JumpFalseOp:
        offset = bcm.getOffset(i);
        f << offset << " --> " << (i) + offset;
        i += 4;
        break;

    case InvokeOp:
        f << bcm.getLong(i) << " " << bcm.mCodeBase[i + 4];
        i += 5;
        break;

    case GetLocalVarOp:
    case SetLocalVarOp:
    case GetArgOp:
    case SetArgOp:
    case GetMethodOp:
    case GetMethodRefOp:
    case GetStaticMethodOp:
    case GetFieldOp:
    case SetFieldOp:
    case GetStaticFieldOp:
    case SetStaticFieldOp:
    case NewInstanceOp:
        f << bcm.getLong(i);
        i += 4;
        break;

    case GetClosureVarOp:
    case SetClosureVarOp:
        f << bcm.getLong(i);
        i += 4;
        f << " " << bcm.getLong(i);
        i += 4;
        break;

    case GetNameOp:
    case GetTypeOfNameOp:
    case SetNameOp:
    case GetPropertyOp:
    case GetInvokePropertyOp:
    case SetPropertyOp:
    case LoadConstantStringOp:
    case DeleteOp:
        f << *bcm.getString(bcm.getLong(i));
        i += 4;
        break;

    case LoadConstantNumberOp:
        f << bcm.getNumber(bcm.getLong(i));
        i += 4;
        break;

    case LoadTypeOp:
    case LoadFunctionOp:
    case PushScopeOp:
        printFormat(f, "0x%X", bcm.getLong(i));
        i += 4;
        break;

    case JsrOp:
        offset = bcm.getOffset(i);
        f << offset << " --> " << i + offset;
        i += 4;
        break;

    case TryOp:
        offset = bcm.getOffset(i);
        f << offset << " --> " << i + offset;
        i += 4;
        offset = bcm.getOffset(i);
        f << " " << offset << " --> " << i + offset;
        i += 4;
        break;
    default:
        printFormat(f, "Unknown Opcode 0x%X", bcm.mCodeBase[i]);
        i++;
        break;
    }
    f << "\n";
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

