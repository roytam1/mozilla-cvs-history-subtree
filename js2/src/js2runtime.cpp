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
JSType *Void_Type;
JSArrayType *Array_Type;

bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind)
{
    while (identifiers) {
        if (identifiers->name.tokenKind == tokenKind)
            return true;
        identifiers = identifiers->next;
    }
    return false;
}

bool hasAttribute(const IdentifierList* identifiers, const StringAtom &name)
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
    JSValue v = getCompileTimeValue(typeName, NULL);
    if (v.isType())
        return v.type;
    else
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
            return mScopeChain->extractType(v->type);
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
	    ASSERT(p.lexer.peek(true).hasKind(Token::end));
            if (mDebugFlag)
            {
                PrettyPrinter f(stdOut, 30);
                {
            	    PrettyPrinter::Block b(f, 2);
                    f << "Program =";
                    f.linearBreak(1);
                    StmtNode::printStatements(f, parsedStatements);
                }
                f.end();
    	        stdOut << '\n';
            }

            buildRuntime(parsedStatements);
            JS2Runtime::ByteCodeModule* bcm = genCode(parsedStatements, fileName);
            if (bcm) {
                result = interpret(bcm, NULL, NULL, 0);
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
    mScopeChain->addScope(mGlobal);
    while (p) {
        mScopeChain->collectNames(p);         // adds declarations for each top-level entity in p
        buildRuntimeForStmt(p);               // adds definitions as they exist for ditto
        p = p->next;
    }
    mScopeChain->popScope();
}

ByteCodeModule *Context::genCode(StmtNode *p, String sourceName)
{
    mScopeChain->addScope(mGlobal);
    ByteCodeGen bcg(this, mScopeChain);
    ByteCodeModule *result = bcg.genCodeForScript(p);
    mScopeChain->popScope();
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

    OperatorList::iterator candidate = applicableOperators.begin();
    for (OperatorList::iterator aoi = applicableOperators.begin() + 1,
                aend = applicableOperators.end();
                (aoi != aend); aoi++) 
    {
        if ((*aoi)->mType1->derivesFrom((*candidate)->mType1)
                || ((*aoi)->mType2->derivesFrom((*candidate)->mType2)))
            candidate = aoi;
    }

    JSFunction *target = (*candidate)->mImp;

    if (target->isNative()) {
        JSValue result = target->mCode(this, NULL, mStack.end() - 2, 2);
        mStack.pop_back();      // XXX
        mStack.pop_back();
        mStack.push_back(result);
        return false;
    }
    else {
        // have to lie about the argCount since the Return sequence expects to 
        // consume the arguments AND the target pointer from the stack.
        mActivationStack.push(new Activation(this, mLocals, mArgumentBase, mThis, mPC, mCurModule, 1));
        mCurModule = target->mByteCode;
        mArgumentBase = mStack.size() - 2;
        return true;
    }
}

JSValue Context::interpret(ByteCodeModule *bcm, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    uint8 *pc = bcm->mCodeBase;
    uint8 *endPC = bcm->mCodeBase + bcm->mLength;

    mCurModule = bcm;

    mScopeChain->addScope(mGlobal);

    mArgumentBase = mStack.size();
    mLocals = new JSValue[bcm->mLocalsCount];
    for (uint32 i = 0; i < argc; i++)
        mStack.push_back(argv[0]);
    if (thisValue)
        mThis = *thisValue;
    else
        mThis = kNullValue;

    JSValue result = interpret(pc, endPC);
    
    // clean off the arguments to keep the stack balanced
    mStack.resize(mArgumentBase);

    mScopeChain->popScope();

    return result;
}

JSValue Context::interpret(uint8 *pc, uint8 *endPC)
{
    JSValue result = kUndefinedValue;
    while (pc != endPC) {
        try {
            if (mDebugFlag) {
                printFormat(stdOut, "                                  %d        ", mStack.size());
                printInstruction(stdOut, (pc - mCurModule->mCodeBase), *mCurModule);
            }
            switch ((ByteCodeOp)(*pc++)) {
            case PopOp:
                {
                    result = mStack.back(); // XXX debug only?
                    mStack.pop_back();
                }
                break;
            case DupOp:
                {
                    JSValue v = mStack.back();
                    mStack.push_back(v);
                }
                break;
            case DupInsertOp:   // XXX something more efficient than pop/push?
                {
                    JSValue v1 = mStack.back();
                    mStack.pop_back();
                    JSValue v2 = mStack.back();
                    mStack.pop_back();
                    mStack.push_back(v1);
                    mStack.push_back(v2);
                    mStack.push_back(v1);
                }
                break;
            case SwapOp:   // XXX something more efficient than pop/push?
                {
                    JSValue v1 = mStack.back();
                    mStack.pop_back();
                    JSValue v2 = mStack.back();
                    mStack.pop_back();
                    mStack.push_back(v1);
                    mStack.push_back(v2);
                }
                break;
            case LogicalXorOp:
                {
                    JSValue v2 = mStack.back();
                    mStack.pop_back();
                    ASSERT(v2.isBool());
                    JSValue v1 = mStack.back();
                    mStack.pop_back();
                    ASSERT(v1.isBool());

                    if (v1.boolean) {
                        if (v2.boolean) {
                            mStack.pop_back();
                            mStack.pop_back();
                            mStack.push_back(kFalseValue);
                        }
                        else
                            mStack.pop_back();
                    }
                    else {
                        if (v1.boolean) {
                            mStack.pop_back();
                            mStack.pop_back();
                            mStack.push_back(kFalseValue);
                        }
                        else {
                            JSValue t = mStack.back();
                            mStack.pop_back();
                            mStack.pop_back();
                            mStack.push_back(t);
                        }
                    }
                }
                break;
            case LogicalNotOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isBool());
                    mStack.push_back(JSValue(!v.boolean));
                }
                break;
            case JumpOp:
                {
                    uint32 offset = *((uint32 *)pc);
                    pc += offset;
                }
                break;
            case ToBooleanOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    mStack.push_back(v.toBoolean(this));
                }
                break;
            case JumpFalseOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isBool());
                    if (!v.boolean) {
                        uint32 offset = *((uint32 *)pc);
                        pc += offset;
                    }
                    else
                        pc += sizeof(uint32);
                }
                break;
            case JumpTrueOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isBool());
                    if (v.boolean) {
                        uint32 offset = *((uint32 *)pc);
                        pc += offset;
                    }
                    else
                        pc += sizeof(uint32);
                }
                break;
            case InvokeOp:
                {
                    uint32 argCount = *((uint32 *)pc); 
                    pc += sizeof(uint32);
                    ThisFlag thisFlag = (ThisFlag)(*pc++);
                    
                    JSValue *targetValue = mStack.end() - (argCount + 1);
                    if (!targetValue->isFunction())
                        throw Exception(Exception::referenceError, "Not a function");
                    JSFunction *target = targetValue->function;
                    uint32 argBase = 0;
                    uint32 cleanUp = argCount;

                    if (mStack.size() > argCount)
                        argBase = mStack.size() - argCount;

                    // get this  (, man)
                    JSValue newThis;
                    switch (thisFlag) {
                    case NoThis:
                        newThis = kNullValue; 
                        break;
                    case Inherent:
                        newThis = target->getThisValue(); 
                        break;
                    case Explicit:
                        newThis = *(mStack.end() - (argCount + 2));
                        cleanUp++;
                        break;
                    }

                    if (target->mByteCode) {
                        for (uint32 i = argCount; i < target->mExpectedArgs; i++) {
                            mStack.push_back(kUndefinedValue);
                            cleanUp++;
                        }
                        mActivationStack.push(new Activation(this, mLocals, mArgumentBase, mThis,
                                                                    pc, mCurModule, cleanUp));
                        mCurModule = target->mByteCode;
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        mArgumentBase = argBase;
                        mThis = newThis;
                        delete[] mLocals;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                    }
                    else {
                        JSValue result = (target->mCode)(this, &newThis, &mStack[argBase], argCount);
                        mStack.resize(mStack.size() - (cleanUp + 1));
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
                    mThis = prev->mThis;
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
                    mThis = prev->mThis;
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
            case LoadConstantUndefinedOp:
                mStack.push_back(kUndefinedValue);
                break;
            case LoadConstantTrueOp:
                mStack.push_back(kTrueValue);
                break;
            case LoadConstantFalseOp:
                mStack.push_back(kFalseValue);
                break;
            case LoadConstantNullOp:
                mStack.push_back(kNullValue);
                break;
            case TypeOfOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    if (v.isUndefined())
                        mStack.push_back(JSValue(new String(widenCString("undefined"))));
                    else
                    if (v.isNull())
                        mStack.push_back(JSValue(new String(widenCString("object"))));
                    else
                    if (v.isBool())
                        mStack.push_back(JSValue(new String(widenCString("boolean"))));
                    else
                    if (v.isNumber())
                        mStack.push_back(JSValue(new String(widenCString("number"))));
                    else
                    if (v.isString())
                        mStack.push_back(JSValue(new String(widenCString("string"))));
                    else
                    if (v.isFunction())
                        mStack.push_back(JSValue(new String(widenCString("function"))));
                    else
                        mStack.push_back(JSValue(new String(widenCString("object"))));
                }
                break;
            case GetNameOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (mScopeChain->getNameValue(name, CURRENT_ATTR, this)) {
                        // need to invoke
                    }
                }
                break;
            case GetTypeOfNameOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (mScopeChain->hasNameValue(name, CURRENT_ATTR)) {
                        if (mScopeChain->getNameValue(name, CURRENT_ATTR, this)) {
                            // need to invoke
                        }
                    }
                    else
                        mStack.push_back(kUndefinedValue);
                }
                break;
            case SetNameOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    if (mScopeChain->setNameValue(name, CURRENT_ATTR, this)) {
                        // need to invoke
                    }
                }
                break;
            case GetElementOp:
                {
                    JSValue index = mStack.back();
                    mStack.pop_back();
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    const String *name = index.toString(this).string;
                    if (obj->getProperty(this, *name, CURRENT_ATTR) ) {
                        // need to invoke
                    }
                }
                break;
            case SetElementOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    JSValue index = mStack.back();
                    mStack.pop_back();
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    const String *name = index.toString(this).string;
                    if (obj->setProperty(this, *name, CURRENT_ATTR, v) ) {
                        // need to invoke
                    }
                    else
                        mStack.push_back(v);
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
                    if (obj->getProperty(this, name, CURRENT_ATTR) ) {
                        // need to invoke
                    }
                }
                break;
            case GetInvokePropertyOp:
                {
                    JSValue base = mStack.back();
                    mStack.pop_back();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    mStack.push_back(JSValue(obj)); // want the "toObject'd" version of base
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    
                    const String &name = *mCurModule->getString(index);

//                    const String &name = *mCurModule->getIdentifierString(index);
//                    IdentifierList *attr = mCurModule->getIdentifierAttr(index);
//                    attr->next = CURRENT_ATTR;

                    if (obj->getProperty(this, name, CURRENT_ATTR) ) {
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
                    if (obj->setProperty(this, name, CURRENT_ATTR, v) ) {
                        // need to invoke
                    }
                    else
                        mStack.push_back(v);// ok to have this here, because the semantics for
                                            // the routine to be invoked require it to leave
                                            // the set value on the top of the stack
                }
                break;
            case DoUnaryOp:
                {
                    Operator op = (Operator)(*pc++);
                    JSValue v = mStack.back();
                    JSFunction *target;
                    if (v.isObject() && (target = v.object->getType()->getUnaryOperator(op)) )
                    {                    
                        uint32 argBase = mStack.size() - 1;
                        if (target->mByteCode) {
                            // lie about argCount to the activation since it
                            // would normally expect to clean the function pointer
                            // off the stack as well.
                            mActivationStack.push(new Activation(this, mLocals, mArgumentBase, 
                                                                   mThis, pc, mCurModule, 0));
                            mCurModule = target->mByteCode;
                            pc = mCurModule->mCodeBase;
                            endPC = mCurModule->mCodeBase + mCurModule->mLength;
                            mArgumentBase = argBase;
                            delete mLocals;
                            mLocals = new JSValue[mCurModule->mLocalsCount];
                        }
                        else {
                            JSValue result = (target->mCode)(this, NULL, &mStack[argBase], 0);
                            mStack.erase(&mStack[argBase], mStack.end());
                            mStack.push_back(result);
                        }
                        break;
                    }                    

                    switch (op) {
                    default:
                        NOT_REACHED("bad unary op");
                    case Increment: // defined in terms of '+'
                        {
                            mStack.push_back(JSValue(1.0));
                            mPC = pc;
                            if (executeOperator(Plus, v.getType(), Number_Type)) {
                                // need to invoke
                                pc = mCurModule->mCodeBase;
                                endPC = mCurModule->mCodeBase + mCurModule->mLength;
                                delete mLocals;
                                mLocals = new JSValue[mCurModule->mLocalsCount];
                            }
                        }
                        break;
                    case Decrement: // defined in terms of '-'
                        {
                            mStack.push_back(JSValue(1.0));
                            mPC = pc;
                            if (executeOperator(Minus, v.getType(), Number_Type)) {
                                // need to invoke
                                pc = mCurModule->mCodeBase;
                                endPC = mCurModule->mCodeBase + mCurModule->mLength;
                                delete mLocals;
                                mLocals = new JSValue[mCurModule->mLocalsCount];
                            }
                        }
                        break;
                    case Negate:
                        {
                            mStack.pop_back();
                            JSValue n = v.toNumber(this);
                            if (n.isNaN())
                                mStack.push_back(n);
                            else
                                mStack.push_back(JSValue(-n.f64));
                        }
                        break;
                    case Posate:
                        {
                            mStack.pop_back();
                            JSValue n = v.toNumber(this);
                            mStack.push_back(n);
                        }
                        break;
                    case Complement:
                        {
                            mStack.pop_back();
                            JSValue n = v.toInt32(this);
                            mStack.push_back(JSValue((float64)(~(int32)(n.f64))));
                        }
                        break;
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
                }
                break;
            case NewInstanceOp:
                {
                    JSValue v = mStack.back();
                    mStack.pop_back();
                    ASSERT(v.isType());
                    JSType *type = v.type;
                    mStack.push_back(JSValue(type->newInstance(this)));
                    mStack.push_back(v);    // keep type on top of stack
                }
                break;
            case NewObjectOp:
                {
                    mStack.push_back(JSValue(Object_Type->newInstance(this)));
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
                }
                break;
            case LoadThisOp:
                {
                    mStack.push_back(mThis);
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
                }
                break;
            case GetMethodOp:
                {
                    JSValue base = mStack.back();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mStack.push_back(JSValue(base.object->mType->mMethods[index]));
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
                    mStack.push_back(v);
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
                    mStack.push_back(v);
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
            case LoadGlobalObjectOp:
                {
                    mStack.push_back(JSValue(mGlobal));
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
    return result;
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
            JSType *thisClass = new JSType(m_cx, name, NULL);
            defineVariable(name, classStmt->attributes, Type_Type, JSValue(thisClass));
            classStmt->mType = thisClass;
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
    case StmtNode::For:
    case StmtNode::ForIn:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            if (f->initializer) collectNames(f->initializer);
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
                        v->prop = defineStaticVariable(i->name, vs->attributes, NULL);
                    else
                        v->prop = defineVariable(i->name, vs->attributes, NULL);
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
            JSFunction *fnc = new JSFunction(m_cx, NULL, m_cx->getParameterCount(f->function));
            f->mFunction = fnc;
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
                    if (isConstructor)
                        defineConstructor(name, f->attributes, NULL, fnc);
                    else {
                        switch (f->function.prefix) {
                        case FunctionName::Get:
                            if (isStatic)
                                defineStaticGetterMethod(name, f->attributes, NULL, fnc);
                            else
                                defineGetterMethod(name, f->attributes, NULL, fnc);
                            break;
                        case FunctionName::Set:
                            if (isStatic)
                                defineStaticSetterMethod(name, f->attributes, NULL, fnc);
                            else
                                defineSetterMethod(name, f->attributes, NULL, fnc);
                            break;
                        case FunctionName::normal:
                            if (isStatic)
                                defineStaticMethod(name, f->attributes, NULL, fnc);
                            else
                                defineMethod(name, f->attributes, NULL, fnc);
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

// this needs to happen before any code is generated in this class
// since the code below assigns the slot indices for instance variables
void JSType::completeClass(Context *cx, ScopeChain *scopeChain, JSType *super)
{    
    // Note test of mStatics:
    // we want to complete the statics classes but not to the
    // extent of providing a default constructor.

    // if none exists, build a default constructor that calls 'super()'
    if (mStatics && getDefaultConstructor() == NULL) {
        JSFunction *fnc = new JSFunction(cx, Object_Type, 0);
        ByteCodeGen bcg(cx, scopeChain);

        if (mSuperType && mSuperType->getDefaultConstructor()) {
            bcg.addByte(LoadThisOp);
            bcg.addByte(LoadFunctionOp);
            bcg.addPointer(mSuperType->getDefaultConstructor());
            bcg.addByte(InvokeOp);
            bcg.addLong(0);
            bcg.addByte(Explicit);
            bcg.addByte(PopOp);
        }
        bcg.addByte(LoadThisOp);
        bcg.addByte(ReturnOp);
        fnc->mByteCode = new ByteCodeModule(&bcg);        

        scopeChain->defineConstructor(mClassName, NULL, NULL, fnc);   // XXX attributes?
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
            else
                if (PROPERTY_KIND(i) == IndexPair) {
                    PROPERTY_GETTERI(i) += super_vTableCount;
                    PROPERTY_SETTERI(i) += super_vTableCount;
                }
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
    case StmtNode::block:
        {
            BlockStmtNode *b = static_cast<BlockStmtNode *>(p);
            StmtNode *s = b->statements;
            while (s) {
                buildRuntimeForStmt(s);
                s = s->next;
            }            
        }
        break;
    case StmtNode::For:
    case StmtNode::ForIn:
        {
            ForStmtNode *f = static_cast<ForStmtNode *>(p);
            if (f->initializer) buildRuntimeForStmt(f->initializer);
        }
        break;
    case StmtNode::Var:
    case StmtNode::Const:
        {
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
//            bool isStatic = hasAttribute(vs->attributes, Token::Static);
            while (v)  {
                Property &prop = PROPERTY(v->prop);
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    JSType *type = mScopeChain->extractType(v->type);
                    prop.mType = type;
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Function:
        {
            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(p);
//            bool isStatic = hasAttribute(f->attributes, Token::Static);
//            bool isConstructor = hasAttribute(f->attributes, mScopeChain->ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, mScopeChain->OperatorKeyWord);
            JSType *resultType = mScopeChain->extractType(f->function.resultType);
            JSFunction *fnc = f->mFunction;
            fnc->mResultType = resultType;
 
            if (isOperator) {
                ASSERT(f->function.name->getKind() == ExprNode::string);
                const String& name = static_cast<StringExprNode *>(f->function.name)->str;
                Operator op = getOperator(getParameterCount(f->function), name);
                // if it's a unary operator, it just gets added 
                // as a method with a special name. Binary operators
                // get added to the Context's operator table.
                if (getParameterCount(f->function) == 1)
                    mScopeChain->defineUnaryOperator(op, fnc);
                else
                    defineOperator(op, getParameterType(f->function, 0), 
                                   getParameterType(f->function, 1), fnc);
            }

            fnc->mParameterBarrel = new ParameterBarrel(this);
            mScopeChain->addScope(fnc->mParameterBarrel);
            VariableBinding *v = f->function.parameters;
            while (v) {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    JSType *pType = mScopeChain->extractType(v->type);
                    IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                    mScopeChain->defineVariable(i->name, NULL, pType);       // XXX attributes?
                }
                v = v->next;
            }
            mScopeChain->addScope(&fnc->mActivation);
            mScopeChain->collectNames(f->function.body);
            buildRuntimeForStmt(f->function.body);
            mScopeChain->popScope();
            mScopeChain->popScope();

        }
        break;
    case StmtNode::Class:
        {     
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            JSType *superClass = Object_Type;
            if (classStmt->superclass) {
                ASSERT(classStmt->superclass->getKind() == ExprNode::identifier);   // XXX
                IdentifierExprNode *superClassExpr = static_cast<IdentifierExprNode*>(classStmt->superclass);
                superClass = mScopeChain->findType(superClassExpr->name);
            }
            JSType *thisClass = classStmt->mType;
            thisClass->mSuperType = superClass;
            thisClass->createStaticComponent(this);
            mScopeChain->addScope(thisClass->mStatics);
            mScopeChain->addScope(thisClass);
            if (classStmt->body) {
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    mScopeChain->collectNames(s);
                    s = s->next;
                }
                s = classStmt->body->statements;
                while (s) {
                    buildRuntimeForStmt(s);
                    s = s->next;
                }
            }
            thisClass->completeClass(this, mScopeChain, superClass);

            mScopeChain->popScope();
            mScopeChain->popScope();
        }        
        break;
    default:
        break;
    }

}

JSValue numberPlus(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 + argv[1].f64);
}

JSValue numberMinus(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 - argv[1].f64);
}

JSValue objectPlus(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
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

JSValue objectMinus(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 - r2.toNumber(cx).f64);
}

JSValue objectMultiply(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 * r2.toNumber(cx).f64);
}

JSValue objectDivide(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 / r2.toNumber(cx).f64);
}

JSValue objectRemainder(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(fmod(r1.toNumber(cx).f64, r2.toNumber(cx).f64));
}



JSValue objectShiftLeft(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) << ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectShiftRight(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (int32)(r1.toInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectUShiftRight(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (uint32)(r1.toUInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectBitAnd(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) & (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitXor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) ^ (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitOr(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) | (int32)(r2.toInt32(cx).f64) ));
}

//
// implements r1 < r2, returning true or false or undefined
//
JSValue objectCompare(Context *cx, JSValue &r1, JSValue &r2)
{
    JSValue r1p = r1.toPrimitive(cx, JSValue::NumberHint);
    JSValue r2p = r2.toPrimitive(cx, JSValue::NumberHint);

    if (r1p.isString() && r2p.isString())
        return JSValue(bool(r1p.string->compare(*r2p.string) < 0));
    else {
        JSValue r1n = r1p.toNumber(cx);
        JSValue r2n = r2p.toNumber(cx);
        if (r1n.isNaN() || r2n.isNaN())
            return kUndefinedValue;
        else
            return JSValue(r1n.f64 < r2n.f64);
    }

}

JSValue objectLess(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    JSValue result = objectCompare(cx, r1, r2);
    if (result.isUndefined())
        return kFalseValue;
    else
        return result;
}

JSValue objectLessEqual(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    JSValue result = objectCompare(cx, r2, r1);
    if (result.isTrue() || result.isUndefined())
        return kFalseValue;
    else
        return kTrueValue;
}

JSValue compareEqual(Context *cx, JSValue r1, JSValue r2)
{
    if (r1.getType() != r2.getType()) {
        if (r1.isNull() && r2.isUndefined())
            return kTrueValue;
        if (r1.isUndefined() && r2.isNull())
            return kTrueValue;
        if (r1.isNumber() && r2.isString())
            return compareEqual(cx, r1, r2.toNumber(cx));
        if (r1.isString() && r2.isNumber())
            return compareEqual(cx, r1.toNumber(cx), r2.toString(cx));
        if (r1.isBool())
            return compareEqual(cx, r1.toNumber(cx), r2);
        if (r2.isBool())
            return compareEqual(cx, r1, r2.toNumber(cx));
        if ( (r1.isString() || r1.isNumber()) && (r2.isObject()) )
            return compareEqual(cx, r1, r2.toPrimitive(cx));
        if ( (r1.isObject()) && (r2.isString() || r2.isNumber()) )
            return compareEqual(cx, r1.toPrimitive(cx), r2);
        return kFalseValue;
    }
    else {
        if (r1.isUndefined())
            return kTrueValue;
        if (r1.isNull())
            return kTrueValue;
        if (r1.isNumber()) {
            if (r1.isNaN())
                return kFalseValue;
            if (r2.isNaN())
                return kFalseValue;
            return JSValue(r1.f64 == r2.f64);
        }
        else {
            if (r1.isString())
                return JSValue(bool(r1.string->compare(*r2.string) == 0));
            if (r1.isBool())
                return JSValue(r1.boolean == r2.boolean);
            if (r1.isObject())
                return JSValue(r1.object == r2.object);
            if (r1.isType())
                return JSValue(r1.type == r2.type);
            if (r1.isFunction())
                return JSValue(r1.function == r2.function);
            NOT_REACHED("unhandled type");
            return kFalseValue;
        }
    }
}

JSValue objectEqual(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    JSValue r1 = argv[0];
    JSValue r2 = argv[1];
    
    return compareEqual(cx, r1, r2);
}


void Context::initOperators()
{
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

        { Less, Object_Type, Object_Type, objectLess, Boolean_Type },
        { LessEqual, Object_Type, Object_Type, objectLessEqual, Boolean_Type },

        { Equal, Object_Type, Object_Type, objectEqual, Boolean_Type },
    };

    for (int i = 0; i < sizeof(OpTable) / sizeof(OpTableEntry); i++) {
        JSFunction *f = new JSFunction(this, OpTable[i].imp, OpTable[i].resType);
        OperatorDefinition *op = new OperatorDefinition(OpTable[i].op1, OpTable[i].op2, f);
        mOperatorTable[OpTable[i].which].push_back(op);
    }
}

JSValue Object_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    return *thisValue;
}

JSValue Object_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    return JSValue(new String(widenCString("[object") + /* [[class]] */ widenCString("]")));
}


JSValue Number_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(new double(argv[0].toNumber(cx).f64));
    else
        thisObj->mPrivate = (void *)(new double(0.0));
    return *thisValue;
}

JSValue Number_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    return JSValue(numberToString(*((double *)(thisObj->mPrivate))));
}


JSValue String_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(new String(*argv[0].toString(cx).string));
    else
        thisObj->mPrivate = (void *)(new String(widenCString("")));
    return *thisValue;
}

JSValue String_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    return JSValue((String *)thisObj->mPrivate);
}

JSValue String_split(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSValue S = thisValue->toString(cx);

    JSInstance *arrInst = Array_Type->newInstance(cx);
    JSValue separatorV;
    JSValue limitV;
    uint32 limit;

    if (argc > 1)
        separatorV = argv[1];
    if (argc > 2)
        limitV = argv[2];
    
    if (limitV.isUndefined())
        limit = two32minus1;
    else
        limit = (uint32)(limitV.toUInt32(cx).f64);

    uint32 s = S.string->size();

    uint32 p = 0;

    // if separatorV.isRegExp() -->

    const String *R = separatorV.toString(cx).string;

    if (limit == 0) 
        return JSValue(arrInst);

    if (separatorV.isUndefined()) {
        //step 33
        arrInst->setProperty(cx, widenCString("0"), NULL, S);
        return JSValue(arrInst);
    }

    if (s == 0) {
        // step 31
    }
    
    
    return JSValue(arrInst);
    
}

JSValue String_length(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    uint32 length = ((String *)(thisObj->mPrivate))->size();
    return JSValue((float64)length);
}


JSValue Array_Constructor(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;
    if (argc > 0) {
        if (argc == 1) {
            arrInst->mLength = (uint32)(argv[0].toNumber(cx).f64);
           // arrInst->mInstanceValues = new JSValue[arrInst->mLength];
        }
        else {
            arrInst->mLength = argc;
            //arrInst->mInstanceValues = new JSValue[arrInst->mLength];
            for (uint32 i = 0; i < argc; i++) {
                String *id = numberToString(i);
                arrInst->defineVariable(*id, NULL, Object_Type, argv[i]);
                delete id;
            }
        }
    }
    return *thisValue;
}

JSValue Array_toString(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    return kUndefinedValue;
}


JSValue Array_push(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    for (uint32 i = 0; i < argc; i++) {
        String *id = numberToString(i + arrInst->mLength);
        arrInst->defineVariable(*id, NULL, Object_Type, argv[i]);
        delete id;
    }
    arrInst->mLength += argc;
    return JSValue((float64)arrInst->mLength);
}
              
JSValue Array_pop(Context *cx, JSValue *thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue->isObject());
    JSObject *thisObj = thisValue->object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    if (arrInst->mLength > 0) {
        String *id = numberToString(arrInst->mLength - 1);
        arrInst->getProperty(cx, *id, NULL);
        JSValue result = cx->mStack.back();
        cx->mStack.pop_back();
        arrInst->deleteProperty(*id, NULL);
        --arrInst->mLength;
        delete id;
        return result;
    }
    else
        return kUndefinedValue;
}
              
JSValue JSValue::valueToObject(Context *cx, JSValue& value)
{
    switch (value.tag) {
    case f64_tag:
        {
            JSObject *obj = Number_Type->newInstance(cx);
            JSFunction *defCon = Number_Type->getDefaultConstructor();
            JSValue argv[1];
            JSValue thisValue = JSValue(obj);
            argv[0] = value;
            if (defCon->mCode) {
                (defCon->mCode)(cx, &thisValue, &argv[0], 1); 
            }
            else {
                ASSERT(false);  // need to throw a hot potato back to
                                // ye interpreter loop
            }
            return thisValue;
        }
/*
    case boolean_tag:
        return JSValue((value.boolean) ? 1.0 : 0.0);
*/
    case string_tag: 
        {
            JSObject *obj = String_Type->newInstance(cx);
            JSFunction *defCon = String_Type->getDefaultConstructor();
            JSValue argv[1];
            JSValue thisValue = JSValue(obj);
            argv[0] = value;
            if (defCon->mCode) {
                (defCon->mCode)(cx, &thisValue, &argv[0], 1); 
            }
            else {
                ASSERT(false);  // need to throw a hot potato back to
                                // ye interpreter loop
            }
            return thisValue;
        }

    case object_tag:
    case function_tag:
        return value;
    case undefined_tag:
        throw Exception(Exception::typeError, "ToObject");
        return value;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
}

float64 stringToNumber(const String *string)
{
    const char16 *numEnd;
    return stringToDouble(string->begin(), string->end(), numEnd);
}

JSValue JSValue::valueToNumber(Context *cx, JSValue& value)
{
    switch (value.tag) {
    case f64_tag:
        return value;
    case string_tag: 
        return JSValue(stringToNumber(value.string));
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

String *numberToString(float64 number)
{
    char buf[dtosStandardBufferSize];
    const char *chrp = doubleToStr(buf, dtosStandardBufferSize, number, dtosStandard, 0);
    return new JavaScript::String(widenCString(chrp));
}
              
JSValue JSValue::valueToString(Context *cx, JSValue& value)
{
    const char* chrp = NULL;
    JSObject *obj = NULL;
    switch (value.tag) {
    case f64_tag:
        return JSValue(numberToString(value.f64));
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
        PropertyIterator i;
        if (obj->hasProperty(widenCString("toString"), CURRENT_ATTR, Read, &i)) {
            JSValue v = obj->getPropertyValue(i);
            if (v.isFunction())
                target = v.function;
        }
        if (target == NULL) {
            if (obj->hasProperty(widenCString("valueOf"), CURRENT_ATTR, Read, &i)) {
                JSValue v = obj->getPropertyValue(i);
                if (v.isFunction())
                    target = v.function;
            }
        }
        if (target) {
            if (target->mByteCode) {
                // here we need to get the interpreter to do the job
                ASSERT(false);
            }
            else
                return (target->mCode)(cx, &value, NULL, 0);
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

JSValue JSValue::valueToBoolean(Context *cx, JSValue& value)
{
    JSObject *obj = NULL;
    switch (value.tag) {
    case f64_tag:
        return JSValue(!(value.f64 == 0.0) || JSDOUBLE_IS_NaN(value.f64));
    case string_tag: 
        return JSValue(value.string->length() != 0);
    case boolean_tag:
        return value;
    case object_tag:
        obj = value.object;
        break;
    case function_tag:
        obj = value.function;
        break;
    case undefined_tag:
        return kFalseValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
    ASSERT(obj);
    JSFunction *target = NULL;
    PropertyIterator i;
    if (obj->hasProperty(widenCString("toBoolean"), CURRENT_ATTR, Read, &i)) {
        JSValue v = obj->getPropertyValue(i);
        if (v.isFunction())
            target = v.function;
    }
    if (target) {
        if (target->mByteCode) {
            // here we need to get the interpreter to do the job
            ASSERT(false);
        }
        else
            return (target->mCode)(cx, NULL, &value, 1);
    }
    throw new Exception(Exception::runtimeError, "toBoolean");    // XXX
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
    f << "properties\n";
    obj.printProperties(f);
    f << "slotsnstuff\n";
    obj.printSlotsNStuff(f);
    f << "done with type\n";
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

