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

#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "parser.h"
#include "numerics.h"
#include "js2runtime.h"
#include "bytecodegen.h"

namespace JavaScript {    
namespace JS2Runtime {

using namespace ByteCode;

//
// XXX don't these belong in the context? But don't
// they need to compare equal across contexts?
//
JSType *Object_Type = NULL;
JSType *Number_Type;
JSType *String_Type;
JSType *Boolean_Type;
JSType *Type_Type;

bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind)
{
    while (identifiers) {
        if (identifiers->name.tokenKind == tokenKind)
            return true;
        identifiers = identifiers->next;
    }
    return false;
}

bool hasAttribute(const IdentifierList* identifiers, StringAtom &name)
{
    while (identifiers) {
        if (identifiers->name == name)
            return true;
        identifiers = identifiers->next;
    }
    return false;
}

JSType *ScopeChain::findType(const StringAtom& typeName) 
{
    Reference *ref = getName(typeName, Read);
    if (ref) {
        const JSValue type = ref->getValue();
        delete ref;
        if (type.isType())
            return type.type;
    }
    return Object_Type;
}

JSType *ScopeChain::extractType(ExprNode *t)
{
    JSType *type = Object_Type;
    if (t && (t->getKind() == ExprNode::identifier)) {
        IdentifierExprNode* typeExpr = static_cast<IdentifierExprNode*>(t);
        type = findType(typeExpr->name);
    }
    return type;
}


JS2Runtime::Operator simpleLookup[Token::kindsEnd] = {
   JS2Runtime::None,                    // none,
   JS2Runtime::None,                    // identifier,
   JS2Runtime::None,                    // number,
   JS2Runtime::None,                    // string,
   JS2Runtime::None,                    // regExp
   JS2Runtime::None,                    // Null,
   JS2Runtime::None,                    // True,
   JS2Runtime::None,                    // False,
   JS2Runtime::None,                    // This,
   JS2Runtime::None,                    // Super,
   JS2Runtime::None,                    // parentheses,
   JS2Runtime::None,                    // numUnit,
   JS2Runtime::None,                    // exprUnit,
   JS2Runtime::None,                    // qualify,
   JS2Runtime::None,                    // objectLiteral,
   JS2Runtime::None,                    // arrayLiteral,
   JS2Runtime::None,                    // functionLiteral,
   JS2Runtime::None,                    // call,
   JS2Runtime::None,                    // New,
   JS2Runtime::None,                    // index,
   JS2Runtime::None,                    // dot,
   JS2Runtime::None,                    // dotClass,
   JS2Runtime::None,                    // dotParen,
   JS2Runtime::None,                    // at,
   JS2Runtime::None,                    // Delete,
   JS2Runtime::None,                    // Typeof,
   JS2Runtime::None,                    // Eval,
   JS2Runtime::None,                    // preIncrement,
   JS2Runtime::None,                    // preDecrement,
   JS2Runtime::None,                    // postIncrement,
   JS2Runtime::None,                    // postDecrement,
   JS2Runtime::None,                    // plus,
   JS2Runtime::None,                    // minus,
   JS2Runtime::Complement,              // complement,
   JS2Runtime::None,                    // logicalNot,
   JS2Runtime::None,                    // add,
   JS2Runtime::None,                    // subtract,
   JS2Runtime::Multiply,                // multiply,
   JS2Runtime::Divide,                  // divide,
   JS2Runtime::Remainder,               // modulo,
   JS2Runtime::ShiftLeft,               // leftShift,
   JS2Runtime::ShiftRight,              // rightShift,
   JS2Runtime::UShiftRight,             // logicalRightShift,
   JS2Runtime::BitAnd,                  // bitwiseAnd,
   JS2Runtime::BitXor,                  // bitwiseXor,
   JS2Runtime::BitOr,                   // bitwiseOr,
   JS2Runtime::None,                    // logicalAnd,
   JS2Runtime::None,                    // logicalXor,
   JS2Runtime::None,                    // logicalOr,
   JS2Runtime::Equal,                   // equal,
   JS2Runtime::None,                    // notEqual,
   JS2Runtime::Less,                    // lessThan,
   JS2Runtime::LessEqual,               // lessThanOrEqual,
   JS2Runtime::None,                    // greaterThan,
   JS2Runtime::None,                    // greaterThanOrEqual,
   JS2Runtime::SpittingImage,           // identical,
   JS2Runtime::None,                    // notIdentical,
   JS2Runtime::In,                      // In,
   JS2Runtime::None,                    // Instanceof,
   JS2Runtime::None,                    // assignment,
   JS2Runtime::None,                    // addEquals,
   JS2Runtime::None,                    // subtractEquals,
   JS2Runtime::None,                    // multiplyEquals,
   JS2Runtime::None,                    // divideEquals,
   JS2Runtime::None,                    // moduloEquals,
   JS2Runtime::None,                    // leftShiftEquals,
   JS2Runtime::None,                    // rightShiftEquals,
   JS2Runtime::None,                    // logicalRightShiftEquals,
   JS2Runtime::None,                    // bitwiseAndEquals,
   JS2Runtime::None,                    // bitwiseXorEquals,
   JS2Runtime::None,                    // bitwiseOrEquals,
   JS2Runtime::None,                    // logicalAndEquals,
   JS2Runtime::None,                    // logicalXorEquals,
   JS2Runtime::None,                    // logicalOrEquals,
   JS2Runtime::None,                    // conditional,
   JS2Runtime::None,                    // comma,
};


JS2Runtime::Operator Context::getOperator(uint32 parameterCount, const String &name)
{
    Lexer operatorLexer(mWorld, name, widenCString("Operator name"), 0); // XXX get source and line number from function ???   
    const Token &t = operatorLexer.get(false);  // XXX what's correct for preferRegExp parameter ???

    switch (t.getKind()) {
    case Token::complement:
        return JS2Runtime::Complement;
    case Token::increment:
        return JS2Runtime::Increment;
    case Token::decrement:
        return JS2Runtime::Decrement;
    case Token::Const:
        return JS2Runtime::Const;
    case Token::times:
        return JS2Runtime::Multiply;
    case Token::divide:
        return JS2Runtime::Divide;
    case Token::modulo:
        return JS2Runtime::Remainder;
    case Token::leftShift:
        return JS2Runtime::ShiftLeft;
    case Token::rightShift:
        return JS2Runtime::ShiftRight;
    case Token::logicalRightShift:
        return JS2Runtime::UShiftRight;
    case Token::lessThan:
        return JS2Runtime::Less;
    case Token::lessThanOrEqual:
        return JS2Runtime::LessEqual;
    case Token::In:
        return JS2Runtime::In;
    case Token::equal:
        return JS2Runtime::Equal;
    case Token::identical:
        return JS2Runtime::SpittingImage;
    case Token::bitwiseAnd:
        return JS2Runtime::BitAnd;
    case Token::bitwiseXor:
        return JS2Runtime::BitXor;
    case Token::bitwiseOr:
        return JS2Runtime::BitOr;

    default:
        NOT_REACHED("Illegal operator name");

    case Token::plus:
        if (parameterCount == 1)
            return JS2Runtime::Posate;
        else
            return JS2Runtime::Plus;
    case Token::minus:
        if (parameterCount == 1)
            return JS2Runtime::Negate;
        else
            return JS2Runtime::Minus;

    case Token::openParenthesis:
        return JS2Runtime::Call;
    
    case Token::New:
        if (parameterCount > 1)
            return JS2Runtime::NewArgs;
        else
            return JS2Runtime::New;
    
    case Token::openBracket:
        {
            const Token &t2 = operatorLexer.get(false);
            ASSERT(t2.getKind() == Token::closeBracket);
            const Token &t3 = operatorLexer.get(false);
            if (t3.getKind() == Token::equal)
                return JS2Runtime::IndexEqual;
            else
                return JS2Runtime::Index;
        }
        
    case Token::Delete:
        return JS2Runtime::DeleteIndex;
    }
    return JS2Runtime::None;    
}

// return the type of the index'th parameter in function
JSType *Context::getParameterType(FunctionDefinition &function, int index)
{
    VariableBinding *v = function.parameters;
    while (v) {
        if (index-- == 0)
            return mScopeChain.extractType(v->type);
        else
            v = v->next;
    }
    return NULL;
}

uint32 Context::getParameterCount(FunctionDefinition &function)
{
    uint32 count = 0;
    VariableBinding *v = function.parameters;
    while (v) {
        count++;
        v = v->next;
    }
    return count;
}

inline char narrow(char16 ch) { return char(ch); }

JSValue Context::readEvalFile(const String& fileName)
{
    String buffer;
    int ch;

    JSValue result = kUndefinedValue;

    std::string str(fileName.length(), char());
    std::transform(fileName.begin(), fileName.end(), str.begin(), narrow);
    FILE* f = fopen(str.c_str(), "r");
    if (f) {
        while ((ch = getc(f)) != EOF)
	        buffer += static_cast<char>(ch);
        fclose(f);
    
        
        try {
            Arena a;
            Parser p(mWorld, a, buffer, fileName);
            StmtNode *parsedStatements = p.parseProgram();

    /*******/
	    ASSERT(p.lexer.peek(true).hasKind(Token::end));
            {
                PrettyPrinter f(stdOut, 30);
                {
            	    PrettyPrinter::Block b(f, 2);
                    f << "Program =";
                    f.linearBreak(1);
                    StmtNode::printStatements(f, parsedStatements);
                }
                f.end();
            }
    	    stdOut << '\n';
    /*******/
/*
            Context cx(mGlobal, mWorld);        // the file is compiled into
                                                // the current global object, not
                                                // the current scope.

            cx.buildRuntime(parsedStatements);
            JS2Runtime::ByteCodeModule* bcm = cx.genCode(parsedStatements, fileName);
            if (bcm) {
                result = cx.interpret(bcm, JSValueList());
                delete bcm;
            }
*/        

            buildRuntime(parsedStatements);
            JS2Runtime::ByteCodeModule* bcm = genCode(parsedStatements, fileName);
            if (bcm) {
                result = interpret(bcm, JSValueList());
                delete bcm;
            }
        
        
        } catch (Exception &e) {
            throw e;
        }
    }
    return result;
}


void Context::buildRuntime(StmtNode *p)
{
    mScopeChain.addScope(mGlobal);
    while (p) {
        mScopeChain.collectNames(p);          // adds declarations for each top-level entity in p
        buildRuntimeForStmt(p);               // adds definitions as they exist for ditto
        p = p->next;
    }
    mScopeChain.popScope();
}

ByteCodeModule *Context::genCode(StmtNode *p, String sourceName)
{
    mScopeChain.addScope(mGlobal);
    ByteCodeGen bcg(this, &mScopeChain);
    ByteCodeModule *result = bcg.genCodeForScript(p);
    mScopeChain.popScope();
    return result;
}

bool Context::executeOperator(Operator op, JSType *t1, JSType *t2)
{
    // look in the operator table for applicable operators
    OperatorList applicableOperators;

    for (OperatorList::iterator oi = mOperatorTable[op].begin(),
                end = mOperatorTable[op].end();
                    (oi != end); oi++) 
    {
        if ((*oi)->isApplicable(t1, t2)) {
            applicableOperators.push_back(*oi);
        }
    }
    if (applicableOperators.size() == 0)
        throw Exception(Exception::runtimeError, "No applicable operators found");

    JSFunction *candidate = applicableOperators[0]->mImp;
    // XXX more code needed here, obviously

    if (candidate->isNative()) {
        // if the candidate can't 
        JSValue result = candidate->mCode(this, mStack.end() - 2, 2);
        mStack.pop_back();      // XXX
        mStack.pop_back();
        mStack.push_back(result);
        return false;
    }
    else {
        // have to lie about the argCount since the Return sequence expects to 
        // consume the arguments AND the target pointer from the stack.
        mActivationStack.push(new Activation(mLocals, mArgumentBase, mPC, mCurModule, 1));
        mCurModule = candidate->mByteCode;
        mArgumentBase = mStack.size() - 2;
        return true;
    }
}

JSValue Context::interpret(ByteCodeModule *bcm, JSValueList args)
{
    uint8 *pc = bcm->mCodeBase;
    uint8 *endPC = bcm->mCodeBase + bcm->mLength;

    mCurModule = bcm;

    mScopeChain.addScope(mGlobal);

    mLocals = new JSValue[bcm->mLocalsCount];

    JSValue result = interpret(pc, endPC);

    mScopeChain.popScope();

    return result;
}

JSValue Context::interpret(uint8 *pc, uint8 *endPC)
{
    while (pc != endPC) {
        try {
            switch ((ByteCodeOp)(*pc++)) {
            case InvokeOp:
                {
                    uint32 argCount = *((uint32 *)pc); 
                    pc += sizeof(uint32);
                    
                    JSValue *targetValue = mStack.end() - (argCount + 1);
                    if (!targetValue->isFunction())
                        throw Exception(Exception::referenceError, "Not a function");
                    JSFunction *target = targetValue->function;
                    uint32 argBase = 0;

                    if (mStack.size() > argCount)
                        argBase = mStack.size() - argCount;

                    if (target->mByteCode) {
                        mActivationStack.push(new Activation(mLocals, mArgumentBase, 
                                                                    pc, mCurModule, argCount));
                        mCurModule = target->mByteCode;
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        mArgumentBase = argBase;
                        delete mLocals;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                    }
                    else {
                        JSValue result = (target->mCode)(this, &mStack[argBase], argCount);
                        mStack.erase(&mStack[argBase], mStack.end());
                        mStack.push_back(result);
                    }

                }
                break;
            case ReturnVoidOp:
                {
                    Activation *prev = mActivationStack.top();
                    mActivationStack.pop();

                    mCurModule = prev->mModule;
                    pc = prev->mPC;
                    endPC = mCurModule->mCodeBase + mCurModule->mLength;
                    mLocals = new JSValue[mCurModule->mLocalsCount];
                    memcpy(mLocals, prev->mLocals, sizeof(JSValue) * mCurModule->mLocalsCount);
                    mArgumentBase = prev->mArgumentBase;

                    mStack.resize(mStack.size() - (prev->mArgCount + 1));
                }
                break;
            case ReturnOp:
                {
                    JSValue result = mStack.back();
                    mStack.pop_back();

                    Activation *prev = mActivationStack.top();
                    mActivationStack.pop();

                    mCurModule = prev->mModule;
                    pc = prev->mPC;
                    endPC = mCurModule->mCodeBase + mCurModule->mLength;
                    mLocals = new JSValue[mCurModule->mLocalsCount];
                    memcpy(mLocals, prev->mLocals, sizeof(JSValue) * mCurModule->mLocalsCount);
                    mArgumentBase = prev->mArgumentBase;

                    mStack.resize(mStack.size() - (prev->mArgCount + 1));
                    mStack.push_back(result);
                }
                break;
            case LoadTypeOp:
                {
                    JSType *t = *((JSType **)pc);
                    pc += sizeof(JSType *);
                    mStack.push_back(JSValue(t));
                }
                break;
            case LoadFunctionOp:
                {
                    JSFunction *f = *((JSFunction **)pc);
                    pc += sizeof(JSFunction *);
                    mStack.push_back(JSValue(f));
                }
                break;
            case LoadConstantStringOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(JSValue(mCurModule->getString(index)));
                }
                break;
            case LoadConstantNumberOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(JSValue(mCurModule->getNumber(index)));
                }
                break;
            case GetNameOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (mScopeChain.getNameValue(name, this)) {
                        // need to invoke
                    }
                }
                break;
            case SetNameOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (mScopeChain.setNameValue(name, this)) {
                        // need to invoke
                    }
                }
                break;
            case GetPropertyOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (obj->getProperty(this, name) ) {
                        // need to invoke
                    }
                }
                break;
            case SetPropertyOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (obj->setProperty(this, name, v) ) {
                        // need to invoke
                    }
                }
                break;
            case DoOperatorOp:
                {
                    Operator op = (Operator)(*pc++);
                    JSValue v1 = *(mStack.end() - 2);
                    JSValue v2 = *(mStack.end() - 1);
                    mPC = pc;
                    if (executeOperator(op, v1.getType(), v2.getType())) {
                        // need to invoke
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        delete mLocals;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                    }
                }
                break;
            case GetConstructorOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isType());
                    mStack.push_back(JSValue(v.type->getDefaultConstructor()));
                    mStack.push_back(v);    // keep type on top of stack
                }
                break;
            case NewObjectOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isType());
                    JSType *type = v.type;
                    mStack.push_back(JSValue(type->newInstance()));
                }
                break;
            case GetLocalVarOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(mLocals[index]);
                }
                break;
            case SetLocalVarOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mLocals[index] = mStack.back();
                    mStack.pop_back();
                }
                break;
            case LoadThisOp:
                {
                    mStack.push_back(mStack[mArgumentBase + 0]);
                }
                break;
            case GetArgOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(mStack[mArgumentBase + index]);
                }
                break;
            case SetArgOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack[mArgumentBase + index] = mStack.back();
                    mStack.pop_back();
                }
                break;
            case GetMethodOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(JSValue(base.object->mType->mMethods[index]));
                    mStack.push_back(base);
                }
                break;
            case GetStaticMethodOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    mStack.push_back(JSValue(classP->mStatics->mMethods[index]));
                }
                break;
            case GetFieldOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(((JSInstance *)(base.object))->mInstanceValues[index]);
                }
                break;
            case SetFieldOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    ((JSInstance *)(base.object))->mInstanceValues[index] = v;
                }
                break;
            case GetStaticFieldOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(classP->mStatics->mInstanceValues[index]);
                }
                break;
            case SetStaticFieldOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    classP->mStatics->mInstanceValues[index] = v;
                }
                break;
            case PushScopeOp:
                {
//                    JSObject *s = *((JSFunction **)pc);
                    pc += sizeof(JSFunction *);
                }
                break;
            case PopScopeOp:
                {
                }
                break;
            default:
                throw Exception(Exception::internalError, "Bad Opcode");
            }
        }
        catch (Exception x) {
            throw x;
            break;
        }
    }
    return kUndefinedValue;
}

//  The first pass over the tree - it just installs the names of each declaration
void ScopeChain::collectNames(StmtNode *p)
{
    switch (p->getKind()) {
        // XXX - other statements, execute them (assuming they have constant control values) ?
        // or simply visit the contained blocks and process any references that need to be hoisted
    case StmtNode::Class:
        {
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            ASSERT(classStmt->name->getKind() == ExprNode::identifier);     // XXX need to handle qualified names!!!
            IdentifierExprNode *className = static_cast<IdentifierExprNode*>(classStmt->name);
            const StringAtom& name = className->name;
            JSType *thisClass = new JSType(name, NULL);
            p->prop = defineVariable(name, Type_Type, JSValue(thisClass));
        }
        break;
    case StmtNode::block:
        {
            // should push a new Activation scope here?
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                collectNames(s);
                s = s->next;
            }            
        }
        break;
    case StmtNode::Const:
    case StmtNode::Var:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                    if (isStatic)
                        v->prop = defineStaticVariable(i->name, NULL);
                    else
                        v->prop = defineVariable(i->name, NULL);
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            bool isStatic = hasAttribute(f->attributes, Token::Static);
            bool isConstructor = hasAttribute(f->attributes, ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, OperatorKeyWord);
            if (isOperator) {
                // no need to do anything yet, all operators are 'pre-declared'
            }
            else {
                if (f->function.name->getKind() == ExprNode::identifier) {
                    const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                    if (topClass() && (topClass()->mClassName.compare(name) == 0))
                        isConstructor = true;
                    if (isConstructor) {
                        p->prop = defineStaticMethod(name, NULL);
                        PROPERTY_KIND(p->prop) = Constructor;
                    }
                    else {
                        switch (f->function.prefix) {
                        case FunctionName::Get:
                            p->prop = defineGetterMethod(name, NULL);
                            break;
                        case FunctionName::Set:
                            p->prop = defineSetterMethod(name, NULL);
                            break;
                        case FunctionName::normal:
                            if (isStatic)
                                p->prop = defineStaticMethod(name, NULL);
                            else
                                p->prop = defineMethod(name, NULL);
                            break;
                        default:
                            NOT_REACHED("unexpected prefix");
                            break;
                        }
                    }
                }
            }
        }
        break;
    default:
        break;
    }
}

void JSType::completeClass(Context *cx, ScopeChain *scopeChain, JSType *super)
{    
    // Note test of mStatics:
    // we want to complete the statics classes but not to the
    // extent of providing a default constructor.

    // if none exists, build a default constructor that calls 'super()'
    if (mStatics && getDefaultConstructor() == NULL) {
        JSFunction *fnc = new JSFunction(NULL);
        ByteCodeGen bcg(cx, scopeChain);

        if (mSuperType && mSuperType->getDefaultConstructor()) {
            bcg.addByte(LoadFunctionOp);
            bcg.addPointer(mSuperType->getDefaultConstructor());
            bcg.addByte(LoadThisOp);
            bcg.addByte(InvokeOp);
            bcg.addLong(1);
        }
        bcg.addByte(LoadThisOp);
        bcg.addByte(ReturnOp);
        fnc->mByteCode = new ByteCodeModule(&bcg);        

        PropertyIterator propIt = scopeChain->defineStaticMethod(mClassName, NULL);
        PROPERTY_KIND(propIt) = Constructor;

        scopeChain->setStaticValue(PROPERTY(propIt), JSValue(fnc));                
    }

    // add the super type instance variable count into the slot indices
    uint32 superInstanceVarCount = 0;
    uint32 super_vTableCount = 0;
    if (super) {
        superInstanceVarCount = super->mVariableCount;
        super_vTableCount = super->mMethods.size();
    }

    mVariableCount += superInstanceVarCount;
    if (superInstanceVarCount) {
        for (PropertyIterator i = mProperties.begin(), 
                    end = mProperties.end();
                    (i != end); i++) {            
            if (PROPERTY_KIND(i) == Slot)
                PROPERTY_INDEX(i) += superInstanceVarCount;
        }
    }
    
    // likewise for the vTable
    if (super_vTableCount) {
        for (PropertyIterator i = mProperties.begin(), 
                    end = mProperties.end();
                    (i != end); i++) {            
            if ((PROPERTY_KIND(i) == Method) 
                    || (PROPERTY_KIND(i) == Constructor))
                PROPERTY_INDEX(i) += super_vTableCount;
        }
        mMethods.insert(mMethods.begin(), 
                            super->mMethods.begin(), 
                            super->mMethods.end());
    }
    // complete the static class (inherit static instances etc)
    if (mStatics && mSuperType)
        mStatics->completeClass(cx, scopeChain, mSuperType->mStatics);
}


// Second pass, collect type information
void Context::buildRuntimeForStmt(StmtNode *p)
{
    switch (p->getKind()) {
    case StmtNode::Var:
    case StmtNode::Const:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
//            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                Property &prop = PROPERTY(v->prop);
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    JSType *type = mScopeChain.extractType(v->type);
                    prop.mType = type;
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            bool isStatic = hasAttribute(f->attributes, Token::Static);
            bool isConstructor = hasAttribute(f->attributes, mScopeChain.ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, mScopeChain.OperatorKeyWord);
            JSType *resultType = mScopeChain.extractType(f->function.resultType);
            JSFunction *fnc = new JSFunction(resultType);
 
            if (isOperator) {
                ASSERT(f->function.name->getKind() == ExprNode::string);
                const String& name = static_cast<StringExprNode *>(f->function.name)->str;
                Operator op = getOperator(getParameterCount(f->function), name);
                // if it's a unary operator, it just gets added 
                // as a method with a special name. Binary operators
                // get added to the Context's operator table.
                defineOperator(op, getParameterType(f->function, 0), 
                                   getParameterType(f->function, 1), fnc);
            }
            else {
                Property &prop = PROPERTY(p->prop);
                prop.mType = resultType;
                if (f->function.name->getKind() == ExprNode::identifier) {
                    const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                    if (mScopeChain.topClass() 
                                && (mScopeChain.topClass()->mClassName.compare(name) == 0))
                        isConstructor = true;
                    if (isStatic || isConstructor)
                        mScopeChain.setStaticValue(prop, JSValue(fnc));
                    else {
                        switch (f->function.prefix) {
                        case FunctionName::Get:
                            mScopeChain.setGetterValue(prop, fnc);
                            break;
                        case FunctionName::Set:
                            mScopeChain.setSetterValue(prop, fnc);
                            break;
                        case FunctionName::normal:
                            mScopeChain.setValue(prop, JSValue(fnc));
                            break;
                        }
                    }
                }
                else
                    ASSERT(false);
            }

            fnc->mParameterBarrel = new ParameterBarrel();
            mScopeChain.addScope(fnc->mParameterBarrel);
            VariableBinding *v = f->function.parameters;
            while (v) {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    JSType *pType = mScopeChain.extractType(v->type);
                    IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                    mScopeChain.defineVariable(i->name, pType);
                }
                v = v->next;
            }
            mScopeChain.addScope(&fnc->mActivation);
            mScopeChain.collectNames(f->function.body);
            buildRuntimeForStmt(f->function.body);
            mScopeChain.popScope();
            mScopeChain.popScope();

        }
        break;
    case StmtNode::Class:
        {     
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            JSType *superClass = Object_Type;
            if (classStmt->superclass) {
                ASSERT(classStmt->superclass->getKind() == ExprNode::identifier);   // XXX
                IdentifierExprNode *superClassExpr = static_cast<IdentifierExprNode*>(classStmt->superclass);
                superClass = mScopeChain.findType(superClassExpr->name);
            }
            ASSERT(PROPERTY_KIND(p->prop) == ValuePointer);
            ASSERT(PROPERTY_VALUEPOINTER(p->prop)->isType());
            JSType *thisClass = (JSType *)(PROPERTY_VALUEPOINTER(p->prop)->type);
            thisClass->mSuperType = superClass;
            thisClass->createStaticComponent();
            mScopeChain.addScope(thisClass->mStatics);
            mScopeChain.addScope(thisClass);
            if (classStmt->body) {
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    mScopeChain.collectNames(s);
                    s = s->next;
                }
                s = classStmt->body->statements;
                while (s) {
                    buildRuntimeForStmt(s);
                    s = s->next;
                }
            }
            thisClass->completeClass(this, &mScopeChain, superClass);
            thisClass->createStaticInstance();      // XXX should be a part of the class initialization code

            mScopeChain.popScope();
            mScopeChain.popScope();
        }        
        break;
    default:
        break;
    }

}

JSValue numberPlus(Context *cx, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 + argv[1].f64);
}

JSValue numberMinus(Context *cx, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 - argv[1].f64);
}

JSValue objectPlus(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    if (r1.isNumber() && r2.isNumber()) {
//        try {
            return JSValue(r1.toNumber(cx).f64 + r2.toNumber(cx).f64);
//        }
//        catch (HotPotato) {
            // push invocation of toNumber, toNumber, Plus onto stack
//        }
    }

    if (r1.isString()) {
        if (r2.isString())
            return JSValue(new String(*r1.string + *r2.string));
        else
            return JSValue(new String(*r1.string + *r2.toString(cx).string));
    }
    else {
        if (r2.isString()) 
            return JSValue(new String(*r1.toString(cx).string + *r2.string));
        else {
            JSValue r1p = r1.toPrimitive(cx);
            JSValue r2p = r2.toPrimitive(cx);
            if (r1p.isString() || r2p.isString()) {
                if (r1p.isString())
                    if (r2p.isString())
                        return JSValue(new String(*r1p.string + *r2p.string));
                    else
                        return JSValue(new String(*r1p.string + *r2p.toString(cx).string));
                else
                    return JSValue(new String(*r1p.toString(cx).string + *r2p.string));
            }
            else {
                JSValue num1(r1.toNumber(cx));
                JSValue num2(r2.toNumber(cx));
                return JSValue(num1.f64 + num2.f64);
            }
        }
    }
}

JSValue objectMinus(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 + r2.toNumber(cx).f64);
}

JSValue objectMultiply(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 * r2.toNumber(cx).f64);
}

JSValue objectDivide(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 / r2.toNumber(cx).f64);
}

JSValue objectRemainder(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(fmod(r1.toNumber(cx).f64, r2.toNumber(cx).f64));
}



JSValue objectShiftLeft(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) << ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectShiftRight(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (int32)(r1.toInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectUShiftRight(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (uint32)(r1.toUInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectBitAnd(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) & (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitXor(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) ^ (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitOr(Context *cx, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) | (int32)(r2.toInt32(cx).f64) ));
}

struct OpTableEntry {
    Operator which;
    JSType *op1;
    JSType *op2;
    JSFunction::NativeCode *imp;
    JSType *resType;
} OpTable[] = {
    { Plus,  Object_Type, Object_Type, objectPlus,  Object_Type },
    { Plus,  Number_Type, Number_Type, numberPlus,  Number_Type },

    { Minus, Object_Type, Object_Type, objectMinus, Number_Type },
    { Minus, Number_Type, Number_Type, numberMinus, Number_Type },

    { ShiftLeft, Object_Type, Object_Type, objectShiftLeft, Number_Type },
    { ShiftRight, Object_Type, Object_Type, objectShiftRight, Number_Type },
    { UShiftRight, Object_Type, Object_Type, objectUShiftRight, Number_Type },
    { BitAnd, Object_Type, Object_Type, objectBitAnd, Number_Type },
    { BitXor, Object_Type, Object_Type, objectBitXor, Number_Type },
    { BitOr, Object_Type, Object_Type, objectBitOr, Number_Type },

    { Multiply, Object_Type, Object_Type, objectMultiply, Number_Type },
    { Divide, Object_Type, Object_Type, objectDivide, Number_Type },
    { Remainder, Object_Type, Object_Type, objectRemainder, Number_Type },

};

void Context::initOperators()
{
    for (int i = 0; i < sizeof(OpTable) / sizeof(OpTableEntry); i++) {
        JSFunction *f = new JSFunction(OpTable[i].imp, OpTable[i].resType);
        OperatorDefinition *op = new OperatorDefinition(OpTable[i].op1, OpTable[i].op2, f);
        mOperatorTable[OpTable[i].which].push_back(op);
    }
}

JSValue Number_Constructor(Context *cx, JSValue *argv, uint32 argc)
{
    ASSERT(argc >= 1);
    ASSERT(argv[0].isObject());
    JSValue that = argv[0];
    JSObject *thatObj = that.object;
    if (argc > 1)
        thatObj->mPrivate = argv[1];
    else
        thatObj->mPrivate = kPositiveZero;
    return that;
}

JSValue Number_ToString(Context *cx, JSValue *argv, uint32 argc)
{
    return argv[0].toString(cx);
}

bool JSValue::isNaN() const
{
    ASSERT(isNumber());
    switch (tag) {
    case f64_tag:
        return JSDOUBLE_IS_NaN(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}
              
bool JSValue::isNegativeInfinity() const
{
    ASSERT(isNumber());
    switch (tag) {
    case f64_tag:
        return (f64 < 0) && JSDOUBLE_IS_INFINITE(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}
              
bool JSValue::isPositiveInfinity() const
{
    ASSERT(isNumber());
    switch (tag) {
    case f64_tag:
        return (f64 > 0) && JSDOUBLE_IS_INFINITE(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}
              
bool JSValue::isNegativeZero() const
{
    ASSERT(isNumber());
    switch (tag) {
    case f64_tag:
        return JSDOUBLE_IS_NEGZERO(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}
              
bool JSValue::isPositiveZero() const
{
    ASSERT(isNumber());
    switch (tag) {
    case f64_tag:
        return (f64 == 0.0) && !JSDOUBLE_IS_NEGZERO(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}

JSValue JSValue::valueToObject(Context *cx, JSValue& value)
{
    switch (value.tag) {
    case f64_tag:
        {
            JSObject *obj = Number_Type->newInstance();
            JSFunction *defCon = Number_Type->getDefaultConstructor();
            JSValue argv[2];
            argv[0] = JSValue(obj);
            argv[1] = value;
            if (defCon->mCode) {
                (defCon->mCode)(cx, &argv[0], 2); 
            }
            else {
                ASSERT(false);  // need to throw a hot potato back to
                                // ye interpreter loop
            }
            return argv[0];
        }
/*
    case boolean_tag:
        return JSValue((value.boolean) ? 1.0 : 0.0);
    case string_tag: 
        {
            const char16 *numEnd;
	    double d = stringToDouble(value.string->begin(), value.string->end(), numEnd);
            return JSValue(d);
        }
*/
    case object_tag:
    case function_tag:
        return value;
    case undefined_tag:
        return value;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
}

JSValue JSValue::valueToNumber(Context *cx, JSValue& value)
{
    switch (value.tag) {
    case f64_tag:
        return value;
    case string_tag: 
        {
            const char16 *numEnd;
	    double d = stringToDouble(value.string->begin(), value.string->end(), numEnd);
            return JSValue(d);
        }
    case object_tag:
    case function_tag:
        return value.toPrimitive(cx, NumberHint).toNumber(cx);
    case boolean_tag:
        return JSValue((value.boolean) ? 1.0 : 0.0);
    case undefined_tag:
        return kNaNValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
}
              
JSValue JSValue::valueToString(Context *cx, JSValue& value)
{
    const char* chrp = NULL;
    JSObject *obj = NULL;
    char buf[dtosStandardBufferSize];
    switch (value.tag) {
    case f64_tag:
        chrp = doubleToStr(buf, dtosStandardBufferSize, value.f64, dtosStandard, 0);
        break;
    case object_tag:
        obj = value.object;
        break;
    case function_tag:
        obj = value.function;
        break;
    case string_tag:
        return value;
    case boolean_tag:
        chrp = (value.boolean) ? "true" : "false";
        break;
    case undefined_tag:
        chrp = "undefined";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    if (obj) {
        JSFunction *target = NULL;
        if (obj->hasProperty(widenCString("toString"), Read)) {
            JSValue v = obj->getProperty(widenCString("toString"));
            if (v.isFunction())
                target = v.function;
        }
        if (target == NULL) {
            if (obj->hasProperty(widenCString("valueOf"), Read)) {
                JSValue v = obj->getProperty(widenCString("valueOf"));
                if (v.isFunction())
                    target = v.function;
            }
        }
        if (target) {
            if (target->mByteCode) {
                // here we need to get the interpreter to
            }
            else
                return (target->mCode)(cx, &value, 1);
        }
        throw new Exception(Exception::runtimeError, "toString");    // XXX
    }
    else
        return JSValue(new JavaScript::String(widenCString(chrp)));

}

JSValue JSValue::toPrimitive(Context *cx, Hint hint)
{
    JSObject *obj;
    switch (tag) {
    case f64_tag:
    case string_tag:
    case boolean_tag:
    case undefined_tag:
        return *this;

    case object_tag:
        obj = object;
        break;
    case function_tag:
        obj = function;
        break;

    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
/*
    JSFunction *target = NULL;
    JSValue result;
    JSValues argv(1);
    argv[0] = *this;

    // The following is [[DefaultValue]]
    //
    if ((hint == NumberHint) || (hint == NoHint)) {
        const JSValue &valueOf = obj->getProperty(widenCString("valueOf"));
        if (valueOf.isFunction()) {
            target = valueOf.function;
            if (target->isNative()) {
                result = static_cast<JSNativeFunction*>(target)->mCode(cx, argv);
            }
            else {
                Context new_cx(cx);
                result = new_cx.interpret(target->getICode(), argv);
            }
            if (result.isPrimitive())
                return result;
        }
        const JSValue &toString = obj->getProperty(widenCString("toString"));
        if (toString.isFunction()) {
            target = toString.function;
            if (target->isNative()) {
                result = static_cast<JSNativeFunction*>(target)->mCode(cx, argv);
            }
            else {
                Context new_cx(cx);
                result = new_cx.interpret(target->getICode(), argv);
            }
            if (result.isPrimitive())
                return result;
        }
    }
    else {
        const JSValue &toString = obj->getProperty(widenCString("toString"));
        if (toString.isFunction()) {
            target = toString.function;
            if (target->isNative()) {
                result = static_cast<JSNativeFunction*>(target)->mCode(cx, argv);
            }
            else {
                Context new_cx(cx);
                result = new_cx.interpret(target->getICode(), argv);
            }
            if (result.isPrimitive())
                return result;
        }
        const JSValue &valueOf = obj->getProperty(widenCString("valueOf"));
        if (valueOf.isFunction()) {
            target = valueOf.function;
            if (target->isNative()) {
                result = static_cast<JSNativeFunction*>(target)->mCode(cx, argv);
            }
            else {
                Context new_cx(cx);
                result = new_cx.interpret(target->getICode(), argv);
            }
            if (result.isPrimitive())
                return result;
        }
    }
    throw Exception(Exception::runtimeError, "toPrimitive");    // XXX
*/
    return kUndefinedValue;
    
}

int JSValue::operator==(const JSValue& value) const
{
    if (this->tag == value.tag) {
#       define CASE(T) case T##_tag: return (this->T == value.T)
        switch (tag) {
        CASE(f64);
        CASE(object);
        CASE(boolean);
        #undef CASE
        // question:  are all undefined values equal to one another?
        case undefined_tag: return 1;
        default:
            NOT_REACHED("Broken compiler?");            
        }
    }
    return 0;
}

static const double two32 = 4294967296.0;
static const double two31 = 2147483648.0;

JSValue JSValue::valueToInt32(Context *cx, JSValue& value)
{
    float64 d;
    switch (value.tag) {
    case f64_tag:
        d = value.f64;
        break;
    case string_tag: 
        {
            const char16 *numEnd;
	    d = stringToDouble(value.string->begin(), value.string->end(), numEnd);
        }
        break;
    case boolean_tag:
        return JSValue((float64)((value.boolean) ? 1 : 0));
    case object_tag:
    case undefined_tag:
        // toNumber(toPrimitive(hint Number))
        return kUndefinedValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
    if ((d == 0.0) || !JSDOUBLE_IS_FINITE(d) )
        return JSValue((float64)0);
    d = fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    if (d >= two31)
	return JSValue((float64)(d - two32));
    else
	return JSValue((float64)d);    
}

JSValue JSValue::valueToUInt32(Context *cx, JSValue& value)
{
    float64 d;
    switch (value.tag) {
    case f64_tag:
        d = value.f64;
        break;
    case string_tag: 
        {
            const char16 *numEnd;
	    d = stringToDouble(value.string->begin(), value.string->end(), numEnd);
        }
        break;
    case boolean_tag:
        return JSValue((float64)((value.boolean) ? 1 : 0));
    case object_tag:
    case undefined_tag:
        // toNumber(toPrimitive(hint Number))
        return kUndefinedValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
    if ((d == 0.0) || !JSDOUBLE_IS_FINITE(d))
        return JSValue((float64)0);
    bool neg = (d < 0);
    d = floor(neg ? -d : d);
    d = neg ? -d : d;
    d = fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    return JSValue((float64)d);
}



Formatter& operator<<(Formatter& f, const JSValue& value)
{
    switch (value.tag) {
    case JSValue::f64_tag:
        f << value.f64;
        break;
    case JSValue::object_tag:
        printFormat(f, "Object @ 0x%08X\n", value.object);
        f << *value.object;
        break;
    case JSValue::type_tag:
        printFormat(f, "Type @ 0x%08X\n", value.type);
        f << *value.type;
        break;
    case JSValue::boolean_tag:
        f << ((value.boolean) ? "true" : "false");
        break;
    case JSValue::string_tag:
        f << *value.string;
        break;
    case JSValue::undefined_tag:
        f << "undefined";
        break;
    case JSValue::null_tag:
        f << "null";
        break;
    case JSValue::function_tag:
        if (value.function->mByteCode != NULL)
            f << "function\n" << *value.function->mByteCode;
        else
            f << "function\n";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    return f;
}

void JSType::printSlotsNStuff(Formatter& f) const
{
    f << "var. count = " << mVariableCount << "\n";
    f << "method count = " << (uint32)(mMethods.size()) << "\n";
    uint32 index = 0;
    for (MethodList::const_iterator i = mMethods.begin(), end = mMethods.end(); (i != end); i++) {
        f << "[#" << index++ << "]";
        if (*i == NULL)
            f << "NULL\n";
        else
            if ((*i)->mByteCode)
                f << *((*i)->mByteCode);
    }
    if (mStatics)
        f << "Statics :\n" << *mStatics;
}

Formatter& operator<<(Formatter& f, const JSObject& obj)
{
    obj.printProperties(f);
    return f;
}
Formatter& operator<<(Formatter& f, const JSType& obj)
{
    printFormat(f, "super @ 0x%08X\n", obj.mSuperType);
    obj.printProperties(f);
    obj.printSlotsNStuff(f);
    return f;
}
Formatter& operator<<(Formatter& f, const Access& slot)
{
    switch (slot) {
    case Read : f << "Read\n"; break;
    case Write : f << "Write\n"; break;
    }
    return f;
}
Formatter& operator<<(Formatter& f, const Property& prop)
{
    switch (prop.mFlag) {
    case ValuePointer : 
        {
            JSValue v = *prop.mData.vp;
            f << "ValuePointer --> "; 
            if (v.isObject())
                printFormat(f, "Object @ 0x%08X\n", v.object);
            else
            if (v.isType())
                printFormat(f, "Type @ 0x%08X\n", v.type);
            else
            if (v.isFunction())
                printFormat(f, "Function @ 0x%08X\n", v.function);
            else
                f << v << "\n";
        }
        break;
    case FunctionPair : f << "FunctionPair\n"; break;
    case IndexPair : f << "IndexPair\n"; break;
    case Slot : f << "Slot\n"; break;
    case Constructor : f << "Constructor\n"; break;
    case Method : f << "Method\n"; break;
    }
    return f;
}
Formatter& operator<<(Formatter& f, const JSInstance& obj)
{
    for (PropertyMap::const_iterator i = obj.mProperties.begin(), end = obj.mProperties.end(); (i != end); i++) {
        const Property& prop = PROPERTY(i);
        f << "[" << PROPERTY_NAME(i) << "] ";
        switch (prop.mFlag) {
        case ValuePointer : f << "ValuePointer --> " << *prop.mData.vp; break;
        case FunctionPair : f << "FunctionPair\n"; break;
        case IndexPair : f << "IndexPair\n"; break;
        case Slot : f << "Slot #" << prop.mData.index 
                         << " --> " << obj.mInstanceValues[prop.mData.index] << "\n"; break;
        case Method : f << "Method #" << prop.mData.index << "\n"; break;
        case Constructor : f << "Constructor #" << prop.mData.index << "\n"; break;
        }
    }
    return f;
}




}
}

