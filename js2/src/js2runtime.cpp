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
#include "numerics.h"
#include "js2runtime.h"
#include "bytecodegen.h"

namespace JavaScript {    
namespace JS2Runtime {

using namespace ByteCode;

JSType *Object_Type = new JSType(NULL);
JSType *Number_Type = new JSType(Object_Type);

static bool hasAttribute(const IdentifierList* identifiers, Token::Kind tokenKind)
{
    while (identifiers) {
        if (identifiers->name.tokenKind == tokenKind)
            return true;
        identifiers = identifiers->next;
    }
    return false;
}

static bool hasAttribute(const IdentifierList* identifiers, StringAtom &name)
{
    while (identifiers) {
        if (identifiers->name == name)
            return true;
        identifiers = identifiers->next;
    }
    return false;
}

JSType *Context::findType(const StringAtom& typeName) 
{
    const JSValue type = mGlobal->getProperty(typeName);
    if (type.isType())
        return type.type;
    return Object_Type;
}

JSType *Context::extractType(ExprNode *t)
{
    JSType *type = Object_Type;
    if (t && (t->getKind() == ExprNode::identifier)) {
        IdentifierExprNode* typeExpr = static_cast<IdentifierExprNode*>(t);
        type = findType(typeExpr->name);
    }
    return type;
}


JS2Runtime::Operator simpleLookup[ExprNode::kindsEnd] = {
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


JS2Runtime::Operator Context::getOperator(uint32 parameterCount, String &name)
{
    Lexer operatorLexer(mWorld, name, widenCString("Operator name"), 0); // XXX get source and line number from function ???   
    const Token &t = operatorLexer.get(false);  // XXX what's correct for preferRegExp parameter ???

    JS2Runtime::Operator op = simpleLookup[t.getKind()];
    if (op != JS2Runtime::None)
        return op;
    else {
        switch (t.getKind()) {
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
    }
    return JS2Runtime::None;    
}

JSType *Context::getParameterType(FunctionDefinition &function, int index)
{
    VariableBinding *v = function.parameters;
    while (v) {
        if (index-- == 0)
            return extractType(v->type);
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


ByteCodeModule *Context::genCode(StmtNode *p, String sourceName)
{
    ByteCodeGen bcg;
    return bcg.genCodeForStatement(p);
}

bool Context::executeOperator(Operator op, JSType *t1, JSType *t2)
{
    return false;
}

JSValue Context::interpret(ByteCodeModule *bcm, JSValueList args)
{
    ByteCodeOp *pc = bcm->mCodeBase;
    ByteCodeOp *endPC = bcm->mCodeBase + bcm->mLength;
    
    std::stack<JSValue> stack;

// XXX !!! BOGUS !!! XXX
#define MaxLocals (32)
    JSValue *locals = new JSValue[MaxLocals];

    while (pc != endPC) {
        switch (*pc) {
        case DoOperatorOp:
            {
                Operator op = (Operator)(*pc++);
                JSValue v1 = stack.top();
                stack.pop();
                JSValue v2 = stack.top();
                stack.pop();
                if (executeOperator(op, v1.getType(), v2.getType())) {
                    // need to invoke
                }
            }
            break;
        case NewObjectOp:
            {
                JSValue v = stack.top();
                stack.pop();
                ASSERT(v.isType());
                JSType *type = v.type;
                stack.push(JSValue(type->newInstance()));
            }
            break;
        case GetLocalVarOp:
            {
                uint32 i = *((uint32 *)pc);
                pc += sizeof(uint32);
                stack.push(locals[i]);
            }
            break;
        case SetLocalVarOp:
            {
                uint32 i = *((uint32 *)pc);
                pc += sizeof(uint32);
                locals[i] = stack.top();
                stack.pop();
            }
            break;
        }
    }
    return kUndefinedValue;
}

void Context::buildRuntime(StmtNode *p)
{
    mScopeChain.addScope(mGlobal);
    buildRuntimeForStmt(p);
}

void Context::buildRuntimeForStmt(StmtNode *p)
{
    switch (p->getKind()) {
    case StmtNode::Var:
        {
            // enter the variable into the current scope object
            VariableStmtNode *vs = static_cast<VariableStmtNode *>(p);
            VariableBinding *v = vs->bindings;
            while (v)  {
                if (v->name && (v->name->getKind() == ExprNode::identifier)) {
                    IdentifierExprNode *i = static_cast<IdentifierExprNode *>(v->name);
                    JSType *type = extractType(v->type);
                    mScopeChain.defineVariable(i->name, type);
                    if (v->initializer) {
                        // assign value from v->initializer to the variable just defined
                    }
                }
                v = v->next;
            }
        }
        break;
    case StmtNode::Class:
        {
            // build a new type object and it's static component
            // enter the class name into the global object
        
            // construct the vtable, instance & static slotmaps
        
            ClassStmtNode *classStmt = static_cast<ClassStmtNode *>(p);
            ASSERT(classStmt->name->getKind() == ExprNode::identifier);     // XXX need to handle qualified names!!!

            IdentifierExprNode* nameExpr = static_cast<IdentifierExprNode*>(classStmt->name);
            JSType *superclass = 0;
            if (classStmt->superclass) {
                ASSERT(classStmt->superclass->getKind() == ExprNode::identifier);   // XXX
                IdentifierExprNode *superclassExpr = static_cast<IdentifierExprNode*>(classStmt->superclass);

                JSValue superclassValue = mGlobal->getProperty(superclassExpr->name);
            
            
                ASSERT(superclassValue.isType() && !superclassValue.isNull());
                superclass = static_cast<JSType*>(superclassValue.type);
            }
            JSType* thisClass = new JSType(superclass);

            // is it ok for a partially defined class to appear in global scope? this is needed
            // to handle recursive types, such as linked list nodes.
            mGlobal->setProperty(nameExpr->name, JSValue(thisClass));

/*
    Declare all the methods & fields
*/
            bool needsInstanceInitializer = false;
            if (classStmt->body) {
                StmtNode* s = classStmt->body->statements;
                while (s) {
                    switch (s->getKind()) {
                    case StmtNode::Const:
                    case StmtNode::Var:
                        {
                            VariableStmtNode *vs = static_cast<VariableStmtNode *>(s);
                            bool isStatic = hasAttribute(vs->attributes, Token::Static);
                            VariableBinding *v = vs->bindings;
                            while (v)  {
                                if (v->name) {
                                    ASSERT(v->name->getKind() == ExprNode::identifier);        // XXX
                                    IdentifierExprNode* idExpr = static_cast<IdentifierExprNode*>(v->name);
                                    JSType *type = extractType(v->type);
                                    if (isStatic)
                                        thisClass->defineStaticVariable(idExpr->name, type);
                                    else {
                                        if (hasAttribute(vs->attributes, VirtualKeyWord))
                                            thisClass->defineVariable(idExpr->name, type);
                                        else
                                            thisClass->defineVariable(idExpr->name, type);
                                        if (v->initializer)
                                            needsInstanceInitializer = true;
                                    }
                                }
                                v = v->next;
                            }
                        }
                        break;
                    case StmtNode::Function:
                        {
                            FunctionStmtNode *f = static_cast<FunctionStmtNode *>(s);
                            bool isStatic = hasAttribute(f->attributes, Token::Static);
                            bool isConstructor = hasAttribute(f->attributes, ConstructorKeyWord);
                            bool isOperator = hasAttribute(f->attributes, OperatorKeyWord);
                            if (isOperator) {
                                ASSERT(f->function.name->getKind() == ExprNode::string);
                                Operator op = getOperator(getParameterCount(f->function),
                                                                (static_cast<StringExprNode *>(f->function.name))->str);
                                defineOperator(op, getParameterType(f->function, 0), 
                                                   getParameterType(f->function, 1), NULL);
                            }
                            else
                                if (f->function.name->getKind() == ExprNode::identifier) {
                                    const StringAtom& name = (static_cast<IdentifierExprNode *>(f->function.name))->name;
                                    if (isConstructor)
                                        thisClass->defineConstructor(name, NULL, NULL);
                                    else
                                        if (isStatic)
                                            thisClass->defineStaticMethod(name, NULL, NULL);
                                        else {
                                            switch (f->function.prefix) {
/*
                                            case FunctionName::Get:
                                                thisClass->setGetter(name, NULL, mContext->extractType(f->function.resultType));
                                                break;
                                            case FunctionName::Set:
                                                thisClass->setSetter(name, NULL, mContext->extractType(f->function.resultType));
                                                break;
*/
                                            case FunctionName::normal:
                                                thisClass->defineMethod(name, NULL, NULL);
                                                break;
                                            default:
                                                NOT_REACHED("unexpected prefix");
                                                break;
                                            }
                                        }
                                }
                        }                        
                        break;
                    default:
                        NOT_REACHED("unimplemented class member statement");
                        break;
                    }
                    s = s->next;
                }
            }
//            if (needsInstanceInitializer)
//                thisClass->defineStatic(mInitName, &Function_Type);
    
        }        
        break;
    case StmtNode::Function:
        {
        }
        break;
    }

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
    case JSValue::undefined_tag:
        f << "undefined";
        break;
    case JSValue::null_tag:
        f << "null";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    return f;
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

    void NameReference::emitCodeSequence(ByteCodeGen *bcg) 
    {
        if (mAccess == Read)
            bcg->addByte(GetNameOp);
        else
            bcg->addByte(SetNameOp);
        bcg->addStringRef(mName); 
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
Formatter& operator<<(Formatter& f, const PropertyFlag& flg)
{
    switch (flg) {
    case ValuePointer : f << "ValuePointer\n"; break;
    case FunctionPair : f << "FunctionPair\n"; break;
    case IndexPair : f << "IndexPair\n"; break;
    case Slot : f << "Slot\n"; break;
    case Method : f << "Method\n"; break;
    }
    return f;
}




}
}

