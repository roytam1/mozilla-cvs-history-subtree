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

#include "jsstring.h"
#include "jsarray.h"
#include "jsmath.h"

#include "fdlibm_ns.h"

// this is the AttributeList passed to the name lookup routines
#define CURRENT_ATTR    (NULL)

namespace JavaScript {    
namespace JS2Runtime {


//
// XXX don't these belong in the context? But don't
// they need to compare equal across contexts?
//
JSType *Object_Type = NULL;
JSType *Number_Type;
JSStringType *String_Type;
JSType *Boolean_Type;
JSType *Type_Type;
JSType *Void_Type;
JSType *Unit_Type;
JSArrayType *Array_Type;

bool hasAttribute(AttributeList* attrs, Token::Kind tokenKind, IdentifierExprNode **attrArg)
{
    while (attrs) {
        if (attrs->expr->getKind() == ExprNode::identifier) {
            const StringAtom& name = (static_cast<IdentifierExprNode *>(attrs->expr))->name;
            if (name.tokenKind == tokenKind)
                return true;
        }
        else
            if (attrs->expr->getKind() == ExprNode::call) {
                InvokeExprNode *i = static_cast<InvokeExprNode *>(attrs->expr);        
                ASSERT(i->op->getKind() == ExprNode::identifier);
                const StringAtom& name = (static_cast<IdentifierExprNode *>(i->op))->name;
                if (name.tokenKind == tokenKind) {
                    if (attrArg) {
                        ExprPairList *p = i->pairs;
                        ASSERT(p && p->value->getKind() == ExprNode::identifier);
                        *attrArg = static_cast<IdentifierExprNode *>(p->value);
                    }
                    return true;
                }
            }
            else
                ASSERT(false);
        attrs = attrs->next;
    }
    return false;
}

bool hasAttribute(AttributeList* attrs, const StringAtom &name, IdentifierExprNode **attrArg)
{
    while (attrs) {
        if (attrs->expr->getKind() == ExprNode::identifier) {
            const StringAtom& idname = (static_cast<IdentifierExprNode *>(attrs->expr))->name;
            if (idname == name)
                return true;
            //  else
                // look up the name in the scopechain to see if it's a const definition
                // whose value we can access.
                // 
        }
        else
            if (attrs->expr->getKind() == ExprNode::call) {
                InvokeExprNode *i = static_cast<InvokeExprNode *>(attrs->expr);        
                ASSERT(i->op->getKind() == ExprNode::identifier);
                const StringAtom& idname = (static_cast<IdentifierExprNode *>(i->op))->name;
                if (idname == name) {
                    if (attrArg) {
                        ExprPairList *p = i->pairs;
                        ASSERT(p && p->value->getKind() == ExprNode::identifier);
                        *attrArg = static_cast<IdentifierExprNode *>(p->value);
                    }
                    return true;
                }
            }
            else
                ASSERT(false);
        attrs = attrs->next;
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
    case Token::New:
        return JS2Runtime::New;

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
        
    case Token::openBracket:
        {
            operatorLexer.get(false);   // the closeBracket
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
                result = interpret(bcm, kNullValue, NULL, 0);
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
    mScopeChain->addScope(getGlobalObject());
    while (p) {
        mScopeChain->collectNames(p);         // adds declarations for each top-level entity in p
        buildRuntimeForStmt(p);               // adds definitions as they exist for ditto
        p = p->next;
    }
    mScopeChain->popScope();
}

JS2Runtime::ByteCodeModule *Context::genCode(StmtNode *p, String sourceName)
{
    mScopeChain->addScope(getGlobalObject());
    JS2Runtime::ByteCodeGen bcg(this, mScopeChain);
    JS2Runtime::ByteCodeModule *result = bcg.genCodeForScript(p);
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
        JSValue result = target->getNativeCode()(this, kNullValue, getBase(stackSize() - 2), 2);
        popValue();      // XXX
        popValue();
        pushValue(result);
        return false;
    }
    else {
        // have to lie about the argCount since the Return sequence expects to 
        // consume the arguments AND the target pointer from the stack.
        mActivationStack.push(new Activation(this, mLocals, mStack, mStackTop, mScopeChain,
                                                mArgumentBase, mThis, mPC, mCurModule, 1));
        mCurModule = target->getByteCode();
        mArgumentBase = stackSize() - 2;
        mScopeChain = target->getScopeChain();
        return true;
    }
}

JSValue Context::interpret(JS2Runtime::ByteCodeModule *bcm, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    uint8 *pc = bcm->mCodeBase;
    uint8 *endPC = bcm->mCodeBase + bcm->mLength;

    JSValue *oldLocals = mLocals;
    JSValue *oldStack = mStack;
    uint32 oldStackTop = mStackTop;
    uint32 oldStackMax = mStackMax;
    JS2Runtime::ByteCodeModule *oldCurModule = mCurModule;
    uint32 oldArgumentBase = mArgumentBase;


    mScopeChain->addScope(getGlobalObject());

    mCurModule = bcm;
    mLocals = new JSValue[bcm->mLocalsCount];
    mStack = new JSValue[bcm->mStackDepth];
    mStackMax = bcm->mStackDepth;
    mStackTop = 0;
    mArgumentBase = stackSize();

    for (uint32 i = 0; i < argc; i++)
        pushValue(argv[i]);
    mThis = thisValue;

    JSValue result = interpret(pc, endPC);
    
    mScopeChain->popScope();

    
    mLocals = oldLocals;
    mStack = oldStack;
    mStackTop = oldStackTop;
    mStackMax = oldStackMax;
    mCurModule = oldCurModule;
    mArgumentBase = oldArgumentBase;
    
    return result;
}

JSValue Context::interpret(uint8 *pc, uint8 *endPC)
{
    JSValue result = kUndefinedValue;
    while (pc != endPC) {
        try {
            if (mDebugFlag) {
                printFormat(stdOut, "                                  %d        ", stackSize());
                printInstruction(stdOut, (pc - mCurModule->mCodeBase), *mCurModule);
            }
            switch ((ByteCodeOp)(*pc++)) {
            case PopOp:
                {
                    result = popValue(); // XXX debug only? - just decrement top
                }
                break;
            case DupOp:
                {
                    JSValue v = topValue();
                    pushValue(v);
                }
                break;
            case DupInsertOp:   // XXX something more efficient than pop/push?
                {
                    JSValue v1 = popValue();
                    JSValue v2 = popValue();
                    pushValue(v1);
                    pushValue(v2);
                    pushValue(v1);
                }
                break;
            case SwapOp:   // XXX something more efficient than pop/push?
                {
                    JSValue v1 = popValue();
                    JSValue v2 = popValue();
                    pushValue(v1);
                    pushValue(v2);
                }
                break;
            case LogicalXorOp:
                {
                    JSValue v2 = popValue();
                    ASSERT(v2.isBool());
                    JSValue v1 = popValue();
                    ASSERT(v1.isBool());

                    if (v1.boolean) {
                        if (v2.boolean) {
                            popValue();
                            popValue();
                            pushValue(kFalseValue);
                        }
                        else
                            popValue();
                    }
                    else {
                        if (v1.boolean) {
                            popValue();
                            popValue();
                            pushValue(kFalseValue);
                        }
                        else {
                            JSValue t = topValue();
                            popValue();
                            popValue();
                            pushValue(t);
                        }
                    }
                }
                break;
            case LogicalNotOp:
                {
                    JSValue v = popValue();
                    ASSERT(v.isBool());
                    pushValue(JSValue(!v.boolean));
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
                    JSValue v = popValue();
                    pushValue(v.toBoolean(this));
                }
                break;
            case JumpFalseOp:
                {
                    JSValue v = popValue();
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
                    JSValue v = popValue();
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
                    uint32 cleanUp = argCount;
                    pc += sizeof(uint32);
                    ThisFlag thisFlag = (ThisFlag)(*pc++);
                    
                    JSValue *targetValue = getBase(stackSize() - (argCount + 1));
                    JSFunction *target;
                    JSValue oldThis = mThis;
                    switch (thisFlag) {
                    case NoThis:
                        mThis = kNullValue; 
                        break;
                    case Explicit:
                        mThis = getValue(stackSize() - (argCount + 2));
                        cleanUp++;
                        break;
                    default:
                        NOT_REACHED("bad bytecode");
                    }

                    if (!targetValue->isFunction()) {
                        if (targetValue->isType()) {
                            // "  Type()  "
                            // - it's a cast expression, we call the
                            // default constructor, overriding the supplied 'this'.
                            //
                            target = targetValue->type->getDefaultConstructor();
                            mThis = kNullValue;

                            // XXX this isn't right, it's only a call to the constructor
                            // for some built-ins. For user-types its a cast from the 
                            // operand type - a runtime error if the types aren't related

                            // For various built-ins it can be a call to the 
                            // constructor: Array(2), or a conversion: String(2)

                            ASSERT("More work needed");
                        }
                        else
                            throw Exception(Exception::referenceError, "Not a function");
                    }
                    else {
                        target = targetValue->function;
                        if (target->hasBoundThis())   // then we use it instead of the expressed version
                            mThis = target->getThisValue();
                    }
                    
                    uint32 argBase = 0;
                    if (stackSize() > argCount)
                        argBase = stackSize() - argCount;

                    if (!target->isNative()) {
                        uint32 expectedArgCount = target->getExpectedArgs();
                        for (uint32 i = argCount; i < expectedArgCount; i++) {
                            pushValue(kUndefinedValue);
                            cleanUp++;
                        }
                        mActivationStack.push(new Activation(this, mLocals, mStack, mStackTop,
                                                                    mScopeChain,
                                                                    mArgumentBase, oldThis,
                                                                    pc, mCurModule, cleanUp));
                        mScopeChain = target->getScopeChain();
                        mCurModule = target->getByteCode();
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        mArgumentBase = argBase;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                        mStack = new JSValue[mCurModule->mStackDepth];
                        mStackMax = mCurModule->mStackDepth;
                        mStackTop = 0;
                    }
                    else {
                        JSValue result = (target->getNativeCode())(this, mThis, getBase(argBase), argCount);
                        mThis = oldThis;
                        resizeStack(stackSize() - (cleanUp + 1));
                        pushValue(result);
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
                    mStack = prev->mStack;
                    mStackTop = prev->mStackTop;
                    mStackMax = mCurModule->mStackDepth;
                    mLocals = prev->mLocals;
                    mArgumentBase = prev->mArgumentBase;
                    mThis = prev->mThis;
                    mScopeChain = prev->mScopeChain;
                    resizeStack(stackSize() - (prev->mArgCount + 1));
                }
                break;
            case ReturnOp:
                {
                    JSValue result = popValue();

                    Activation *prev = mActivationStack.top();
                    mActivationStack.pop();

                    mCurModule = prev->mModule;
                    pc = prev->mPC;
                    endPC = mCurModule->mCodeBase + mCurModule->mLength;
                    mStack = prev->mStack;
                    mStackTop = prev->mStackTop;
                    mStackMax = mCurModule->mStackDepth;
                    mLocals = prev->mLocals;
                    mArgumentBase = prev->mArgumentBase;
                    mThis = prev->mThis;
                    mScopeChain = prev->mScopeChain;
                    resizeStack(stackSize() - (prev->mArgCount + 1));
                    pushValue(result);
                }
                break;
            case LoadTypeOp:
                {
                    JSType *t = *((JSType **)pc);
                    pc += sizeof(JSType *);
                    pushValue(JSValue(t));
                }
                break;
            case LoadFunctionOp:
                {
                    JSFunction *f = *((JSFunction **)pc);
                    pc += sizeof(JSFunction *);
                    pushValue(JSValue(f));
                }
                break;
            case LoadConstantStringOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(JSValue(mCurModule->getString(index)));
                }
                break;
            case LoadConstantNumberOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(JSValue(mCurModule->getNumber(index)));
                }
                break;
            case LoadConstantUndefinedOp:
                pushValue(kUndefinedValue);
                break;
            case LoadConstantTrueOp:
                pushValue(kTrueValue);
                break;
            case LoadConstantFalseOp:
                pushValue(kFalseValue);
                break;
            case LoadConstantNullOp:
                pushValue(kNullValue);
                break;
            case DeleteOp:
                {
                    JSValue base = popValue();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    const String &name = *mCurModule->getString(index);
                    PropertyIterator it;
                    if (!obj->hasOwnProperty(name, CURRENT_ATTR, Read, &it))
                        pushValue(kTrueValue);
                    else {
                        obj->deleteProperty(name, CURRENT_ATTR);
                        pushValue(kTrueValue);
                    }
                }
                break;
            case TypeOfOp:
                {
                    JSValue v = popValue();
                    if (v.isUndefined())
                        pushValue(JSValue(new String(widenCString("undefined"))));
                    else
                    if (v.isNull())
                        pushValue(JSValue(new String(widenCString("object"))));
                    else
                    if (v.isBool())
                        pushValue(JSValue(new String(widenCString("boolean"))));
                    else
                    if (v.isNumber())
                        pushValue(JSValue(new String(widenCString("number"))));
                    else
                    if (v.isString())
                        pushValue(JSValue(new String(widenCString("string"))));
                    else
                    if (v.isFunction())
                        pushValue(JSValue(new String(widenCString("function"))));
                    else
                        pushValue(JSValue(new String(widenCString("object"))));
                }
                break;
            case AsOp:
                {
                    JSValue t = popValue();
                    JSValue v = popValue();
                    if (t.isType()) {
                        if (v.isObject() 
                                && (v.object->getType() == t.type))
                            pushValue(v);
                        else
                            pushValue(kNullValue);   // XXX or throw an exception if 
                                                            // NULL is not a member of type t
                    }
                    else
                        throw Exception(Exception::typeError, "As needs type");
                }
                break;
            case InstanceOfOp:
                {
                    JSValue t = popValue();
                    JSValue v = popValue();
                    if (t.isType()) {
                        if (v.isNull())
                            if (t.type == Object_Type)
                                pushValue(kTrueValue);
                            else
                                pushValue(kFalseValue);
                        else
                            if (v.isObject() 
                                    && ((v.object->getType() == t.type)
                                        || (v.object->getType()->derivesFrom(t.type))))
                                pushValue(kTrueValue);
                            else
                                pushValue(kFalseValue);
                    }
                    else {
                        if (t.isObject() && t.isFunction()) {                            
                            // XXX prove that t->function["prototype"] is on t.object->mPrototype chain
                            pushValue(kTrueValue);
                        }
                        else
                            throw Exception(Exception::typeError, "InstanceOf needs object");
                    }
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
                        pushValue(kUndefinedValue);
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
                    JSValue index = popValue();
                    JSValue base = popValue();
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
                    JSValue v = popValue();
                    JSValue index = popValue();
                    JSValue base = popValue();
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
                        pushValue(v);
                }
                break;
            case GetPropertyOp:
                {
                    JSValue base = popValue();
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
                    // if the result is a function of some kind, bind
                    // the base object to it
                    JSValue result = topValue();
                    if (result.isFunction()) {
                        popValue();
                        pushValue(JSValue(new JSBoundFunction(result.function, obj)));
                    }
                }
                break;
            case GetInvokePropertyOp:
                {
                    JSValue base = popValue();
                    JSObject *obj = NULL;
                    if (!base.isObject() && !base.isType())
                        obj = base.toObject(this).object;
                    else
                        obj = base.object;
                    pushValue(JSValue(obj)); // want the "toObject'd" version of base
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    
                    const String &name = *mCurModule->getString(index);

//                    const String &name = *mCurModule->getIdentifierString(index);
//                    AttributeList *attr = mCurModule->getIdentifierAttr(index);
//                    attr->next = CURRENT_ATTR;

                    if (obj->getProperty(this, name, CURRENT_ATTR) ) {
                        // need to invoke
                    }
                }
                break;
            case SetPropertyOp:
                {
                    JSValue v = popValue();
                    JSValue base = popValue();
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
                        pushValue(v);// ok to have this here, because the semantics for
                                            // the routine to be invoked require it to leave
                                            // the set value on the top of the stack
                }
                break;
            case DoUnaryOp:
                {
                    Operator op = (Operator)(*pc++);
                    JSValue v = topValue();
                    JSFunction *target;
                    if (v.isObject() && (target = v.object->getType()->getUnaryOperator(op)) )
                    {                    
                        uint32 argBase = stackSize() - 1;
                        if (!target->isNative()) {
                            // lie about argCount to the activation since it
                            // would normally expect to clean the function pointer
                            // off the stack as well.
                            mActivationStack.push(new Activation(this, mLocals, mStack, mStackTop, 
                                                                    mScopeChain,
                                                                    mArgumentBase, mThis,
                                                                    pc, mCurModule, 0));
                            mCurModule = target->getByteCode();
                            pc = mCurModule->mCodeBase;
                            endPC = mCurModule->mCodeBase + mCurModule->mLength;
                            mArgumentBase = argBase;
                            mLocals = new JSValue[mCurModule->mLocalsCount];
                            mStack = new JSValue[mCurModule->mStackDepth];
                            mStackMax = mCurModule->mStackDepth;
                            mStackTop = 0;
                        }
                        else {
                            JSValue result = (target->getNativeCode())(this, kNullValue, getBase(argBase), 0);
                            resizeStack(stackSize() -  1);
                            pushValue(result);
                        }
                        break;
                    }                    

                    switch (op) {
                    default:
                        NOT_REACHED("bad unary op");
                    case Increment: // defined in terms of '+'
                        {
                            pushValue(JSValue(1.0));
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
                            pushValue(JSValue(1.0));
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
                            popValue();
                            JSValue n = v.toNumber(this);
                            if (n.isNaN())
                                pushValue(n);
                            else
                                pushValue(JSValue(-n.f64));
                        }
                        break;
                    case Posate:
                        {
                            popValue();
                            JSValue n = v.toNumber(this);
                            pushValue(n);
                        }
                        break;
                    case Complement:
                        {
                            popValue();
                            JSValue n = v.toInt32(this);
                            pushValue(JSValue((float64)(~(int32)(n.f64))));
                        }
                        break;
                    }
                }
                break;
            case DoOperatorOp:
                {
                    Operator op = (Operator)(*pc++);
                    JSValue v1 = getValue(stackSize() - 2);
                    JSValue v2 = getValue(stackSize() - 1);
                    mPC = pc;
                    if (executeOperator(op, v1.getType(), v2.getType())) {
                        // need to invoke
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                        mStack = new JSValue[mCurModule->mStackDepth];
                        mStackMax = mCurModule->mStackDepth;
                        mStackTop = 0;
                    }
                }
                break;
            case GetConstructorOp:
                {
                    JSValue v = popValue();
                    ASSERT(v.isType());
                    pushValue(JSValue(v.type->getDefaultConstructor()));
                }
                break;
            case NewInstanceOp:
                {
                    uint32 argCount = *((uint32 *)pc); 
                    pc += sizeof(uint32);
                    uint32 argBase = 0;
                    if (stackSize() > argCount)
                        argBase = stackSize() - argCount;
                    JSValue oldThis = mThis;
                    uint32 cleanUp = argCount;

                    
                    JSValue *typeValue = getBase(stackSize() - (argCount + 1));
                    if (!typeValue->isType())
                        throw Exception(Exception::referenceError, "Not a type");
                        // XXX yes, but what about 1.5 style constructor functions

                    // if the type has an operator "new" use that, 
                    // otherwise use the default constructor (and pass NULL
                    // for the this value)
                    JSFunction *target = typeValue->type->getUnaryOperator(New);
                    if (target)
                        mThis = JSValue(typeValue->type->newInstance(this));
                    else {
                        mThis = kNullValue;
                        target = typeValue->type->getDefaultConstructor();
                    }
                    
                    if (!target->isNative()) {
                        uint32 expectedArgCount = target->getExpectedArgs();
                        for (uint32 i = argCount; i < expectedArgCount; i++) {
                            pushValue(kUndefinedValue);
                            cleanUp++;
                        }
                        mActivationStack.push(new Activation(this, mLocals, mStack, mStackTop,
                                                                    mScopeChain,
                                                                    mArgumentBase, oldThis,
                                                                    pc, mCurModule, cleanUp));
                        mCurModule = target->getByteCode();
                        pc = mCurModule->mCodeBase;
                        endPC = mCurModule->mCodeBase + mCurModule->mLength;
                        mArgumentBase = argBase;
                        mLocals = new JSValue[mCurModule->mLocalsCount];
                        mStack = new JSValue[mCurModule->mStackDepth];
                        mStackMax = mCurModule->mStackDepth;
                        mStackTop = 0;
                    }
                    else {
                        JSValue result = (target->getNativeCode())(this, mThis, getBase(argBase), argCount);
                        mThis = oldThis;
                        resizeStack(stackSize() - (cleanUp + 1));
                        pushValue(result);
                    }
                 }
                break;
            case NewThisOp:
                {
                    JSValue v = popValue();
                    if (mThis.isNull()) {
                        ASSERT(v.isType());
                        mThis = JSValue(v.type->newInstance(this));
                    }
                }
                break;
            case NewObjectOp:
                {
                    pushValue(JSValue(Object_Type->newInstance(this)));
                }
                break;
            case GetLocalVarOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(mLocals[index]);
                }
                break;
            case SetLocalVarOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    mLocals[index] = topValue();
                }
                break;
            case GetClosureVarOp:
                {
                    uint32 depth = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
//                    pushValue(mScopeChain->getClosureVar(depth, index));                    
                }
                break;
            case SetClosureVarOp:
                {
                    uint32 depth = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
//                    mScopeChain->setClosureVar(depth, index, topValue()));
                }
                break;
            case NewClosureOp:
                {
                }
                break;
            case LoadThisOp:
                {
                    pushValue(mThis);
                }
                break;
            case GetArgOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    Activation *prev = mActivationStack.top();
                    pushValue(prev->mStack[mArgumentBase + index]);
                }
                break;
            case SetArgOp:
                {
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    Activation *prev = mActivationStack.top();
                    prev->mStack[mArgumentBase + index] = topValue();
                }
                break;
            case GetMethodOp:
                {
                    JSValue base = topValue();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(JSValue(base.object->mType->mMethods[index]));
                }
                break;
            case GetStaticMethodOp:
                {
                    JSValue base = popValue();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    pushValue(JSValue(classP->mStatics->mMethods[index]));
                }
                break;
            case GetFieldOp:
                {
                    JSValue base = popValue();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(((JSInstance *)(base.object))->mInstanceValues[index]);
                }
                break;
            case SetFieldOp:
                {
                    JSValue v = popValue();
                    JSValue base = popValue();
                    ASSERT(dynamic_cast<JSInstance *>(base.object));
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    ((JSInstance *)(base.object))->mInstanceValues[index] = v;
                    pushValue(v);
                }
                break;
            case GetStaticFieldOp:
                {
                    JSValue base = popValue();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    pushValue(classP->mStatics->mInstanceValues[index]);
                }
                break;
            case SetStaticFieldOp:
                {
                    JSValue v = popValue();
                    JSValue base = popValue();
                    ASSERT(dynamic_cast<JSType *>(base.object));
                    JSType *classP = (JSType *)(base.object);
                    ASSERT(classP->mStatics);
                    uint32 index = *((uint32 *)pc);
                    pc += sizeof(uint32);
                    classP->mStatics->mInstanceValues[index] = v;
                    pushValue(v);
                }
                break;
            case WithinOp:
                {
                    JSValue base = popValue();
                    mScopeChain->addScope(base.toObject(this).object);
                }
                break;
            case WithoutOp:
                {
                    mScopeChain->popScope();
                }
                break;
            case PushScopeOp:
                {
                    JSObject *obj = *((JSObject **)pc);
                    mScopeChain->addScope(obj);
                    pc += sizeof(JSObject *);
                }
                break;
            case PopScopeOp:
                {
                    mScopeChain->popScope();
                }
                break;
            case LoadGlobalObjectOp:
                {
                    pushValue(JSValue(getGlobalObject()));
                }
                break;
            case JsrOp:
                {
                    uint32 offset = *((uint32 *)pc);
                    mSubStack.push(pc + sizeof(uint32));
                    pc += offset;
                }
                break;
            case RtsOp:
                {
                    pc = mSubStack.top();
                    mSubStack.pop();
                }
                break;

            case TryOp:
                {
                    Activation *curAct = (mActivationStack.size() > 0) ? mActivationStack.top() : NULL;
                    uint32 handler = *((uint32 *)pc);
                    if (handler != -1)
                        mTryStack.push(new HandlerData(pc + handler, stackSize(), curAct));
                    pc += sizeof(uint32);
                    handler = *((uint32 *)pc);
                    if (handler != -1)
                        mTryStack.push(new HandlerData(pc + handler, stackSize(), curAct));
                    pc += sizeof(uint32);
                }
                break;
            case HandlerOp:
                {
                    HandlerData *hndlr = (HandlerData *)mTryStack.top();
                    mTryStack.pop();
                    delete hndlr;
                }
                break;
            case ThrowOp:
                {   
                    JSValue x = topValue();
                    if (mTryStack.size() > 0) {
                        HandlerData *hndlr = (HandlerData *)mTryStack.top();
                        Activation *curAct = (mActivationStack.size() > 0) ? mActivationStack.top() : NULL;
                        if (curAct != hndlr->mActivation) {
                            Activation *prev = mActivationStack.top();
                            do {
                                mActivationStack.pop();
                                curAct = mActivationStack.top();                            
                            } while (hndlr->mActivation != curAct);
                            mCurModule = prev->mModule;
                            endPC = mCurModule->mCodeBase + mCurModule->mLength;
                            mLocals = prev->mLocals;
                            mStack = prev->mStack;
                            mStackTop = 1;          // just the exception object remains
                            mStackMax = mCurModule->mStackDepth;
                            mArgumentBase = prev->mArgumentBase;
                            mThis = prev->mThis;
                        }

                        resizeStack(hndlr->mStackSize);
                        pc = hndlr->mPC;
                        pushValue(x);
                    }
                    else
                        throw Exception(Exception::uncaughtError, "No handler for throw");
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
            if (hasAttribute(classStmt->attributes, m_cx->FixedKeyWord))
                thisClass->mIsDynamic = false;
            if (hasAttribute(classStmt->attributes, m_cx->DynamicKeyWord))
                thisClass->mIsDynamic = true;

            PropertyIterator it;
            if (hasProperty(name, classStmt->attributes, Read, &it))
                throw Exception(Exception::referenceError, "Duplicate class definition");
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
    case StmtNode::If:
    case StmtNode::With:
    case StmtNode::DoWhile:
    case StmtNode::While:
        {
            UnaryStmtNode *u = static_cast<UnaryStmtNode *>(p);
            collectNames(u->stmt);
        }
        break;
    case StmtNode::IfElse:
        {
            BinaryStmtNode *b = static_cast<BinaryStmtNode *>(p);
            collectNames(b->stmt);
            collectNames(b->stmt2);
        }
        break;
    case StmtNode::Try:
        {
            TryStmtNode *t = static_cast<TryStmtNode *>(p);
            if (t->catches) {
                CatchClause *c = t->catches;
                while (c) {
                    c->prop = defineVariable(c->name, NULL, NULL);
                    c = c->next;
                }
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
            bool isStatic = hasAttribute(f->attributes, Token::Static);
            bool isConstructor = hasAttribute(f->attributes, m_cx->ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, m_cx->OperatorKeyWord);
            JSFunction *fnc = new JSFunction(m_cx, NULL, m_cx->getParameterCount(f->function), this);
            f->mFunction = fnc;

            if (isOperator) {
                // no need to do anything yet, all operators are 'pre-declared'
            }
            else {
                if (f->function.name->getKind() == ExprNode::identifier) {
                    const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                    IdentifierExprNode *extendArg;
                    if (hasAttribute(f->attributes, m_cx->ExtendKeyWord, &extendArg)) {
                        JSType *extendedClass = extractType(extendArg);
                    
                          // sort of want to fall into the code below, but use 'extendedClass' instead
                          // of whatever the topClass will turn out to be.
                        if (extendedClass->mClassName.compare(name) == 0)
                            isConstructor = true;       // can you add constructors?
                        if (isConstructor)
                            extendedClass->defineConstructor(name, f->attributes, NULL, fnc);
                        else {
                            switch (f->function.prefix) {
                            case FunctionName::Get:
                                if (isStatic)
                                    extendedClass->defineStaticGetterMethod(name, f->attributes, NULL, fnc);
                                else
                                    extendedClass->defineGetterMethod(name, f->attributes, NULL, fnc);
                                break;
                            case FunctionName::Set:
                                if (isStatic)
                                    extendedClass->defineStaticSetterMethod(name, f->attributes, NULL, fnc);
                                else
                                    extendedClass->defineSetterMethod(name, f->attributes, NULL, fnc);
                                break;
                            case FunctionName::normal:
                                if (isStatic)
                                    extendedClass->defineStaticMethod(name, f->attributes, NULL, fnc);
                                else
                                    extendedClass->defineMethod(name, f->attributes, NULL, fnc);
                                break;
                            default:
                                NOT_REACHED("unexpected prefix");
                                break;
                            }
                        }                    
                    }
                    else {
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
                else
                    NOT_REACHED("implement me - qualified function name");
            }
        }
        break;
    case StmtNode::Namespace:
        {
            // ok, so it's a namespace.
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
        JSFunction *fnc = new JSFunction(cx, Object_Type, 0, scopeChain);
        ByteCodeGen bcg(cx, scopeChain);

        if (mSuperType && mSuperType->getDefaultConstructor()) {
            bcg.addOp(LoadTypeOp);
            bcg.addPointer(this);
            bcg.addOp(NewThisOp);
            bcg.addOp(LoadThisOp);
            bcg.addOp(LoadFunctionOp);
            bcg.addPointer(mSuperType->getDefaultConstructor());
            bcg.addOpAdjustDepth(InvokeOp, -1);
            bcg.addLong(0);
            bcg.addByte(Explicit);
            bcg.addOp(PopOp);
        }
        bcg.addOp(LoadThisOp);
        ASSERT(bcg.mStackTop == 1);
        bcg.addOpSetDepth(ReturnOp, 0);
        fnc->setByteCode(new JS2Runtime::ByteCodeModule(&bcg));        

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

void Context::buildRuntimeForFunction(FunctionDefinition &f, JSFunction *fnc)
{
    fnc->mParameterBarrel = new ParameterBarrel(this);
    mScopeChain->addScope(fnc->mParameterBarrel);
    VariableBinding *v = f.parameters;
    while (v) {
        if (v->name && (v->name->getKind() == ExprNode::identifier)) {
            JSType *pType = mScopeChain->extractType(v->type);
            IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
            mScopeChain->defineVariable(i->name, NULL, pType);       // XXX attributes?
        }
        v = v->next;
    }
    mScopeChain->addScope(&fnc->mActivation);
    mScopeChain->collectNames(f.body);
    buildRuntimeForStmt(f.body);
    mScopeChain->popScope();
    mScopeChain->popScope();
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
    case StmtNode::Try:
        {
            TryStmtNode *t = static_cast<TryStmtNode *>(p);
            if (t->catches) {
                CatchClause *c = t->catches;
                while (c) {
                    if (c->type) {
                        Property &prop = PROPERTY(c->prop);
                        prop.mType = mScopeChain->extractType(c->type);
                    }
                    c = c->next;
                }
            }
        }
        break;
    case StmtNode::If:
    case StmtNode::With:
    case StmtNode::DoWhile:
    case StmtNode::While:
        {
            UnaryStmtNode *u = static_cast<UnaryStmtNode *>(p);
            buildRuntimeForStmt(u->stmt);
        }
        break;
    case StmtNode::IfElse:
        {
            BinaryStmtNode *b = static_cast<BinaryStmtNode *>(p);
            buildRuntimeForStmt(b->stmt);
            buildRuntimeForStmt(b->stmt2);
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
//            bool isConstructor = hasAttribute(f->attributes, ConstructorKeyWord);
            bool isOperator = hasAttribute(f->attributes, OperatorKeyWord);
            JSType *resultType = mScopeChain->extractType(f->function.resultType);
            JSFunction *fnc = f->mFunction;
            fnc->setResultType(resultType);
 
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

            // if it's an extending function, rediscover the extended class
            // and push the class scope onto the scope chain
/*
            bool isExtender = false;                
            if (hasAttribute(f->attributes, ExtendKeyWord)) {
                JSType *extendedClass = mScopeChain->extractType( <extend attribute argument> );
                mScopeChain->addScope(extendedClass->mStatics);
                mScopeChain->addScope(extendedClass);
            }
*/            

            buildRuntimeForFunction(f->function, fnc);
/*
            if (isExtender) {   // blow off the extended class's scope
                mScopeChain->popScope();
                mScopeChain->popScope();
            }
*/


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
    case StmtNode::Namespace:
        {
            // do anything ?
        }
        break;
    default:
        break;
    }

}

JSValue numberPlus(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 + argv[1].f64);
}

JSValue numberMinus(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    return JSValue(argv[0].f64 - argv[1].f64);
}

JSValue objectPlus(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
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

JSValue objectMinus(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 - r2.toNumber(cx).f64);
}

JSValue objectMultiply(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 * r2.toNumber(cx).f64);
}

JSValue objectDivide(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(r1.toNumber(cx).f64 / r2.toNumber(cx).f64);
}

JSValue objectRemainder(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue(fd::fmod(r1.toNumber(cx).f64, r2.toNumber(cx).f64));
}



JSValue objectShiftLeft(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) << ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectShiftRight(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (int32)(r1.toInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectUShiftRight(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64) ( (uint32)(r1.toUInt32(cx).f64) >> ( (uint32)(r2.toUInt32(cx).f64) & 0x1F)) );
}

JSValue objectBitAnd(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) & (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitXor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    return JSValue((float64)( (int32)(r1.toInt32(cx).f64) ^ (int32)(r2.toInt32(cx).f64) ));
}

JSValue objectBitOr(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
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

JSValue objectLess(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue &r1 = argv[0];
    JSValue &r2 = argv[1];
    JSValue result = objectCompare(cx, r1, r2);
    if (result.isUndefined())
        return kFalseValue;
    else
        return result;
}

JSValue objectLessEqual(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
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

JSValue objectEqual(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
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

JSValue Object_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue thatValue = thisValue;
    if (thatValue.isNull())
        thatValue = Object_Type->newInstance(cx);
    ASSERT(thatValue.isObject());
    return thatValue;
}

JSValue Object_toString(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue.isObject());
    return JSValue(new String(widenCString("[object") + /* [[class]] */ widenCString("]")));
}


JSValue Number_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue thatValue = thisValue;
    if (thatValue.isNull())
        thatValue = Number_Type->newInstance(cx);
    ASSERT(thatValue.isObject());
    JSObject *thisObj = thatValue.object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(new double(argv[0].toNumber(cx).f64));
    else
        thisObj->mPrivate = (void *)(new double(0.0));
    return thatValue;
}

JSValue Number_toString(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    return JSValue(numberToString(*((double *)(thisObj->mPrivate))));
}



JSValue Boolean_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue thatValue = thisValue;
    if (thatValue.isNull())
        thatValue = Boolean_Type->newInstance(cx);
    ASSERT(thatValue.isObject());
    JSObject *thisObj = thatValue.object;
    if (argc > 0)
        thisObj->mPrivate = (void *)(argv[0].toBoolean(cx).boolean);
    else
        thisObj->mPrivate = (void *)(false);
    return thatValue;
}

JSValue Boolean_toString(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    if (thisObj->mPrivate != 0)
        return JSValue(new String(widenCString("true")));
    else
        return JSValue(new String(widenCString("false")));
}


              
JSValue JSValue::valueToObject(Context *cx, const JSValue& value)
{
    switch (value.tag) {
    case f64_tag:
        {
            JSObject *obj = Number_Type->newInstance(cx);
            JSFunction *defCon = Number_Type->getDefaultConstructor();
            JSValue argv[1];
            JSValue thisValue = JSValue(obj);
            argv[0] = value;
            if (defCon->isNative()) {
                (defCon->getNativeCode())(cx, thisValue, &argv[0], 1); 
            }
            else {
                ASSERT(false);  // need to throw a hot potato back to
                                // ye interpreter loop
            }
            return thisValue;
        }
    case boolean_tag:
        {
            JSObject *obj = Boolean_Type->newInstance(cx);
            JSFunction *defCon = Boolean_Type->getDefaultConstructor();
            JSValue argv[1];
            JSValue thisValue = JSValue(obj);
            argv[0] = value;
            if (defCon->isNative()) {
                (defCon->getNativeCode())(cx, thisValue, &argv[0], 1); 
            }
            else {
                ASSERT(false);
            }
            return thisValue;
        }
    case string_tag: 
        {
            JSObject *obj = String_Type->newInstance(cx);
            JSFunction *defCon = String_Type->getDefaultConstructor();
            JSValue argv[1];
            JSValue thisValue = JSValue(obj);
            argv[0] = value;
            if (defCon->isNative()) {
                (defCon->getNativeCode())(cx, thisValue, &argv[0], 1); 
            }
            else {
                ASSERT(false);
            }
            return thisValue;
        }
    case object_tag:
    case function_tag:
        return value;
    case null_tag:
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

JSValue JSValue::valueToNumber(Context *cx, const JSValue& value)
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
              
JSValue JSValue::valueToString(Context *cx, const JSValue& value)
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
            if (!target->isNative()) {
                // here we need to get the interpreter to do the job
                ASSERT(false);
            }
            else
                return (target->getNativeCode())(cx, value, NULL, 0);
        }
        throw new Exception(Exception::runtimeError, "toString");    // XXX
    }
    else
        return JSValue(new JavaScript::String(widenCString(chrp)));

}

JSValue JSValue::toPrimitive(Context *cx, Hint hint) const
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


JSValue JSValue::valueToInt32(Context *cx, const JSValue& value)
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
    d = fd::fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    if (d >= two31)
	return JSValue((float64)(d - two32));
    else
	return JSValue((float64)d);    
}

JSValue JSValue::valueToUInt32(Context *cx, const JSValue& value)
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
    d = fd::floor(neg ? -d : d);
    d = neg ? -d : d;
    d = fd::fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    return JSValue((float64)d);
}

JSValue JSValue::valueToUInt16(Context *cx, const JSValue& value)
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
    d = fd::floor(neg ? -d : d);
    d = neg ? -d : d;
    d = fd::fmod(d, two16);
    d = (d >= 0) ? d : d + two16;
    return JSValue((float64)d);
}

JSValue JSValue::valueToBoolean(Context *cx, const JSValue& value)
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
        if (!target->isNative()) {
            // here we need to get the interpreter to do the job
            ASSERT(false);
        }
        else {
            JSValue args = value;
            return (target->getNativeCode())(cx, kNullValue, &args, 1);
        }
    }
    throw new Exception(Exception::runtimeError, "toBoolean");    // XXX
}

    


void Context::initClass(JSType *type, JSType *super, ClassDef *cdef, PrototypeFunctions *pdef)
{
    mScopeChain->addScope(type);
    type->createStaticComponent(this);
    type->setDefaultConstructor(new JSFunction(this, cdef->defCon, Object_Type));
    if (pdef) {
        for (uint32 i = 0; i < pdef->mCount; i++) {
            JSFunction *fun = new JSFunction(this, pdef->mDef[i].imp, pdef->mDef[i].result);
            fun->setExpectedArgs(pdef->mDef[i].length);
            type->mPrototype->defineVariable(widenCString(pdef->mDef[i].name), 
                                               NULL, 
                                               pdef->mDef[i].result, 
                                               JSValue(fun));
        }
    }
    type->completeClass(this, mScopeChain, super);
    type->setStaticInitializer(this, NULL);
    getGlobalObject()->defineVariable(widenCString(cdef->name), NULL, Type_Type, JSValue(type));
    mScopeChain->popScope();
    if (pdef) delete pdef;
}



void Context::initBuiltins()
{
    ClassDef builtInClasses[] =
    {
        { "Object",     Object_Constructor  },
        { "Number",     Number_Constructor  },
        { "String",     String_Constructor  },
        { "Array",      Array_Constructor   },
        { "Boolean",    Boolean_Constructor },
        { "Void",       NULL                },
        { "Unit",       NULL                },
    };

    Object_Type  = new JSType(this, widenCString(builtInClasses[0].name), NULL);
    Object_Type->mIsDynamic = true;
    // XXX aren't all the built-ins thus?

    Number_Type  = new JSType(this, widenCString(builtInClasses[1].name), Object_Type);
    String_Type  = new JSStringType(this, widenCString(builtInClasses[2].name), Object_Type);
    Array_Type   = new JSArrayType(this, widenCString(builtInClasses[3].name), Object_Type);
    Boolean_Type = new JSType(this, widenCString(builtInClasses[4].name), Object_Type);
    Void_Type    = new JSType(this, widenCString(builtInClasses[5].name), Object_Type);
    Unit_Type    = new JSType(this, widenCString(builtInClasses[6].name), Object_Type);


    String_Type->defineVariable(widenCString("fromCharCode"), NULL, String_Type, JSValue(new JSFunction(this, String_fromCharCode, String_Type)));


    ProtoFunDef objectProtos[] = 
    {
        { "toString", String_Type, 0, Object_toString },
        { "toSource", String_Type, 0, Object_toString },
        { NULL }
    };
    ProtoFunDef numberProtos[] = 
    {
        { "toString", String_Type, 0, Number_toString },
        { "toSource", String_Type, 0, Number_toString },
        { NULL }
    };

    ProtoFunDef booleanProtos[] = 
    {
        { "toString", String_Type, 0, Boolean_toString },
        { "toSource", String_Type, 0, Boolean_toString },
        { NULL }
    };

    ASSERT(mGlobal);
    *mGlobal = Object_Type->newInstance(this);
    initClass(Object_Type,  NULL,         &builtInClasses[0], new PrototypeFunctions(&objectProtos[0]) );
    
    // pull up them bootstraps 
    (*mGlobal)->mPrototype = Object_Type->mPrototype;

    initClass(Number_Type,  Object_Type,  &builtInClasses[1], new PrototypeFunctions(&numberProtos[0]) );
    initClass(String_Type,  Object_Type,  &builtInClasses[2], getStringProtos() );
    initClass(Array_Type,   Object_Type,  &builtInClasses[3], getArrayProtos() );
    initClass(Boolean_Type, Object_Type,  &builtInClasses[4], new PrototypeFunctions(&booleanProtos[0]) );
    initClass(Void_Type,    Object_Type,  &builtInClasses[5], NULL);
}

Context::Context(JSObject **global, World &world, Arena &a) 
    : mGlobal(global), 
      mWorld(world),
      mScopeChain(NULL),
      mArena(a),
      mDebugFlag(false),

      VirtualKeyWord(world.identifiers["virtual"]),
      ConstructorKeyWord(world.identifiers["constructor"]),
      OperatorKeyWord(world.identifiers["operator"]),
      FixedKeyWord(world.identifiers["fixed"]),
      DynamicKeyWord(world.identifiers["dynamic"]),
      ExtendKeyWord(world.identifiers["extend"])

{
    mScopeChain = new ScopeChain(this, mWorld);
    if (Object_Type == NULL) {                
        initBuiltins();
        JSObject *mathObj = Object_Type->newInstance(this);
        getGlobalObject()->defineVariable(widenCString("Math"), NULL, Object_Type, JSValue(mathObj));
        initMathObject(this, mathObj);    
        getGlobalObject()->defineVariable(widenCString("undefined"), NULL, Void_Type, kUndefinedValue);
        getGlobalObject()->defineVariable(widenCString("NaN"), NULL, Void_Type, kNaNValue);
        getGlobalObject()->defineVariable(widenCString("Infinity"), NULL, Void_Type, kPositiveInfinity);                
    }
    initOperators();
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
        if (!value.function->isNative())
            f << "function\n" << *value.function->getByteCode();
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
            if (!(*i)->isNative())
                f << *((*i)->getByteCode());
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

