/**
 * The base visitor object extended by semantic evaluators.
 *
 * This is a visitor that is used by the compiler for various forms for
 * evaluation of a parse tree (e.g. a type evaluator might compute the 
 * static type of an expression.)
 */

#ifndef NodePrinter_h
#define NodePrinter_h

#pragma warning ( disable : 4786 )
#include "Value.h"
#include "Context.h"

#include <string>
#include <sstream>

namespace esc {
namespace v1 {

struct Node;
struct IdentifierNode;
struct ThisExpressionNode; 
struct SuperExpressionNode;
struct QualifiedIdentifierNode; 
struct LiteralNullNode; 
struct LiteralBooleanNode; 
struct LiteralNumberNode; 
struct LiteralStringNode; 
struct LiteralUndefinedNode;
struct LiteralRegExpNode;
struct UnitExpressionNode;
struct FunctionExpressionNode;
struct ParenthesizedExpressionNode;
struct ParenthesizedListExpressionNode;
struct LiteralObjectNode;
struct LiteralFieldNode;
struct LiteralArrayNode;
struct PostfixExpressionNode; 
struct NewExpressionNode; 
struct IndexedMemberExpressionNode; 
struct ClassofExpressionNode;
struct MemberExpressionNode; 
struct CoersionExpressionNode; 
struct CallExpressionNode; 
struct GetExpressionNode; 
struct SetExpressionNode; 
struct UnaryExpressionNode; 
struct BinaryExpressionNode; 
struct ConditionalExpressionNode; 
struct AssignmentExpressionNode; 
struct ListNode; 
struct StatementListNode; 
struct EmptyStatementNode; 
struct ExpressionStatementNode; 
struct AnnotatedBlockNode; 
struct LabeledStatementNode; 
struct IfStatementNode; 
struct SwitchStatementNode; 
struct CaseLabelNode; 
struct DoStatementNode; 
struct WhileStatementNode; 
struct ForInStatementNode; 
struct ForStatementNode; 
struct WithStatementNode; 
struct ContinueStatementNode; 
struct BreakStatementNode; 
struct ReturnStatementNode; 
struct ThrowStatementNode; 
struct TryStatementNode; 
struct CatchClauseNode; 
struct FinallyClauseNode; 
struct UseStatementNode; 
struct IncludeStatementNode; 
struct ImportDefinitionNode; 
struct ImportBindingNode; 
struct AnnotatedDefinitionNode; 
struct AttributeListNode; 
struct ExportDefinitionNode; 
struct ExportBindingNode; 
struct VariableDefinitionNode; 
struct VariableBindingNode; 
struct TypedVariableNode; 
struct FunctionDefinitionNode; 
struct FunctionDeclarationNode; 
struct FunctionNameNode; 
struct FunctionSignatureNode; 
struct ParameterNode; 
struct OptionalParameterNode; 
struct RestParameterNode;
struct NamedParameterNode;
struct ClassDefinitionNode; 
struct ClassDeclarationNode; 
struct InheritanceNode; 
struct NamespaceDefinitionNode; 
struct PackageDefinitionNode; 
struct LanguageDeclarationNode;
struct ProgramNode; 

class NodePrinter : public Evaluator {

	std::ostringstream out;
	int level;
	int mode;

	void separate() {
		if( mode == machine_mode ) {
			out << "|";
		}
	}

	void push_in() {
		++level;
		if( mode == machine_mode ) {
			out << "[";
		} else {
			out << " ";
		}
	}

	void pop_out() {
		if( mode == machine_mode ) {
			out << "]";
		}
		--level;
	}

	void indent() {
		if( mode == man_mode ) {
			out << '\n';
			for( int i = level; i; --i ) {
				out << "   ";
			}
		}
	}

public:

	std::string str() {
		return out.str();
	}

	enum {
		man_mode,
		machine_mode,
	};

	NodePrinter(int mode = man_mode) : level(0), mode(mode) {
	}

    // Base node
    
	virtual Value* evaluate( Context& cx, Node* node ) { 
		indent();
		out << "error:undefined printer method";
        return 0;
    }

	// Expression evaluators

	virtual Value* evaluate( Context& cx, IdentifierNode* node ) {
		indent();
		out << "Identifier";
		push_in();
		out << node->name;
		pop_out();
		return 0;
    }

    // Expression evaluators

    virtual Value* evaluate( Context& cx, ThisExpressionNode* node ) { 
		indent();
		out << "ThisExpression";
		return 0;
    }
    virtual Value* evaluate( Context& cx, QualifiedIdentifierNode* node ) { 
		indent();
		out << "QualifiedIdentifier";
		push_in();
		out << node->name;
		separate();
		if( node->qualifier ) {
			node->qualifier->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralBooleanNode* node ) { 
		indent();
		out << "LiteralBoolean:";
		out << node->value;
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralNumberNode* node ) { 
		indent();
		out << "LiteralNumber:";
		out << node->value.c_str();
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralStringNode* node ) { 
		indent();
		out << "LiteralString:";
		out << node->value.c_str();
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralUndefinedNode* node ) {
		indent();
		out << "LiteralUndefined";
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralNullNode* node ) {
		indent();
		out << "LiteralNull";
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralRegExpNode* node ) {
		indent();
		out << "LiteralRegExp";
        return 0;
    }
    virtual Value* evaluate( Context& cx, UnitExpressionNode* node ) {
		indent();
		out << "UnitExpression";
		push_in();
		if( node->value ) {
			node->value->evaluate(cx,this);
		}
		separate();
		if( node->type ) {
			node->type->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, FunctionExpressionNode* node ) {
		indent();
		out << "FunctionExpression";
		push_in();
		if( node->name ) {
			node->name->evaluate(cx,this);
		}
		separate();
		if( node->signature ) {
			node->signature->evaluate(cx,this);
		}
		separate();
		if( node->body ) {
			node->body->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, ParenthesizedExpressionNode* node ) {
		indent();
		out << "ParenthesizedExpression";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ParenthesizedListExpressionNode* node ) {
		indent();
		out << "ParenthesizedListExpression";
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralObjectNode* node ) {
		indent();
		out << "LiteralObject";
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralFieldNode* node ) {
		indent();
		out << "LiteralField";
        return 0;
    }
    virtual Value* evaluate( Context& cx, LiteralArrayNode* node ) {
		indent();
		out << "LiteralArray";
		push_in();
		if( node->elementlist ) {
			node->elementlist->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, PostfixExpressionNode* node ) { 
		indent();
		out << "PostfixExpression";
		push_in();
		out << Token::getTokenClassName(node->op);
		pop_out();
		separate();		
		push_in();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, NewExpressionNode* node ) { 
		indent();
		out << "NewExpression";
		push_in();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, IndexedMemberExpressionNode* node ) { 
		indent();
		out << "IndexedMemberExpression";
		push_in();
		if( node->base ) {
			node->base->evaluate(cx,this);
		}
		separate();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, ClassofExpressionNode* node ) {
		indent();
		out << "ClassofExpression";
		push_in();
		if( node->base ) {
			node->base->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, MemberExpressionNode* node ) {
		indent();
		out << "MemberExpression";
		push_in();
		if( node->base ) {
			node->base->evaluate(cx,this);
		}
		separate();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();

        return 0;
    }
    virtual Value* evaluate( Context& cx, CoersionExpressionNode* node ) { 
		indent();
		out << "CoersionExpression";
		push_in();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		separate();
		if( node->type ) {
			node->type->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, CallExpressionNode* node ) { 
		indent();
		out << "CallExpression";
		push_in();
		if( node->member ) {
			node->member->evaluate(cx,this);
		}
		separate();
		if( node->args ) {
			node->args->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, GetExpressionNode* node ) {
		indent();
		out << "GetExpression";
		push_in();
		if( node->member ) {
			node->member->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, SetExpressionNode* node ) { 
		indent();
		out << "SetExpression";
        return 0;
    }
    virtual Value* evaluate( Context& cx, UnaryExpressionNode* node ) { 
		indent();
		out << "UnaryExpression";
		push_in();
		out << Token::getTokenClassName(node->op);
		pop_out();
		separate();		
		push_in();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, BinaryExpressionNode* node ) { 
		indent();
		out << "BinaryExpression";
		push_in();
		out << Token::getTokenClassName(node->op);
		pop_out();
		separate();		
		push_in();
		if( node->lhs ) {
			node->lhs->evaluate(cx,this);
		}
		separate();
		if( node->rhs ) {
			node->rhs->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, ConditionalExpressionNode* node ) { 
		indent();
		out << "ConditionalExpression";
		push_in();
		if( node->condition ) {
			node->condition->evaluate(cx,this);
		}
		pop_out();
		separate();		
		push_in();
		if( node->thenexpr ) {
			node->thenexpr->evaluate(cx,this);
		}
		separate();
		if( node->elseexpr ) {
			node->elseexpr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, AssignmentExpressionNode* node ) { 
		indent();
		out << "AssignmentExpression";
		push_in();
		out << Token::getTokenClassName(node->op);
		pop_out();
		separate();		
		push_in();
		if( node->lhs ) {
			node->lhs->evaluate(cx,this);
		}
		separate();
		if( node->rhs ) {
			node->rhs->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, ListNode* node ) { 
		indent();
		out << "List";
		push_in();
		if( node->list ) {
			node->list->evaluate(cx,this);
		}
		separate();
		if( node->item ) {
			node->item->evaluate(cx,this);
		}
		pop_out();
		return 0;
    }

    // Statements

    virtual Value* evaluate( Context& cx, StatementListNode* node ) {
		indent();
		out << "StatementList";
		push_in();
		if( node->list ) {
			node->list->evaluate(cx,this);
		}
		if( node->item ) {
			node->item->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, EmptyStatementNode* node ) { 
		indent();
		out << "EmptyStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ExpressionStatementNode* node ) {
		indent();
		out << "ExpressionStatement";
		push_in();
		if( node->expr ) {
			node->expr->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, AnnotatedBlockNode* node ) { 
		indent();
		out << "AnnotatedBlock";
        return 0;
    }
    virtual Value* evaluate( Context& cx, LabeledStatementNode* node ) { 
		indent();
		out << "LabeledStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, IfStatementNode* node ) { 
		indent();
		out << "IfStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, SwitchStatementNode* node ) { 
		indent();
		out << "SwitchStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, CaseLabelNode* node ) { 
		indent();
		out << "CaseLabel";
        return 0;
    }
    virtual Value* evaluate( Context& cx, DoStatementNode* node ) { 
		indent();
		out << "DoStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, WhileStatementNode* node ) { 
		indent();
		out << "WhileStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ForInStatementNode* node ) { 
		indent();
		out << "ForInStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ForStatementNode* node ) { 
		indent();
		out << "ForStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, WithStatementNode* node ) { 
		indent();
		out << "WithStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ContinueStatementNode* node ) { 
		indent();
		out << "ContinueStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, BreakStatementNode* node ) { 
		indent();
		out << "BreakStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ReturnStatementNode* node ) { 
		indent();
		out << "ReturnStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ThrowStatementNode* node ) { 
		indent();
		out << "ThrowStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, TryStatementNode* node ) { 
		indent();
		out << "TryStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, CatchClauseNode* node ) { 
		indent();
		out << "CatchClause";
        return 0;
    }
    virtual Value* evaluate( Context& cx, FinallyClauseNode* node ) { 
		indent();
		out << "FinallyClause";
        return 0;
    }
    virtual Value* evaluate( Context& cx, UseStatementNode* node ) { 
		indent();
		out << "UseStatement";
        return 0;
    }
    virtual Value* evaluate( Context& cx, IncludeStatementNode* node ) { 
		indent();
		out << "IncludeStatement";
        return 0;
    }

    // Definitions

    virtual Value* evaluate( Context& cx, ImportDefinitionNode* node ) { 
		indent();
		out << "ImportDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ImportBindingNode* node ) { 
		indent();
		out << "ImportBinding";
        return 0;
    }
    virtual Value* evaluate( Context& cx, AnnotatedDefinitionNode* node ) { 
		indent();
		out << "AnnotatedDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, AttributeListNode* node ) { 
		indent();
		out << "AttributeList";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ExportDefinitionNode* node ) { 
		indent();
		out << "ExportDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ExportBindingNode* node ) { 
		indent();
		out << "ExportBinding";
        return 0;
    }
    virtual Value* evaluate( Context& cx, VariableDefinitionNode* node ) { 
		indent();
		out << "VariableDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, VariableBindingNode* node ) { 
		indent();
		out << "VariableBinding";
        return 0;
    }
    virtual Value* evaluate( Context& cx, TypedVariableNode* node ) { 
		indent();
		out << "TypedVariable";
        return 0;
    }
    virtual Value* evaluate( Context& cx, FunctionDefinitionNode* node ) { 
		indent();
		out << "FunctionDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, FunctionDeclarationNode* node ) { 
		indent();
		out << "FunctionDeclaration";
        return 0;
    }
    virtual Value* evaluate( Context& cx, FunctionNameNode* node ) { 
		indent();
		out << "FunctionName";
        return 0;
    }
    virtual Value* evaluate( Context& cx, FunctionSignatureNode* node ) { 
		indent();
		out << "FunctionSignature";
		push_in();
		if( node->parameter ) {
			node->parameter->evaluate(cx,this);
		}
		separate();
		if( node->result ) {
			node->result->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, ParameterNode* node ) { 
		indent();
		out << "Parameter";
		push_in();
		if( node->identifier ) {
			node->identifier->evaluate(cx,this);
		}
		separate();
		if( node->type ) {
			node->type->evaluate(cx,this);
		}
		pop_out();
        return 0;
    }
    virtual Value* evaluate( Context& cx, OptionalParameterNode* node ) { 
		indent();
		out << "OptionalParameter";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ClassDefinitionNode* node ) { 
		indent();
		out << "ClassDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ClassDeclarationNode* node ) { 
		indent();
		out << "ClassDeclaration";
        return 0;
    }
    virtual Value* evaluate( Context& cx, InheritanceNode* node ) { 
		indent();
		out << "Inheritance";
        return 0;
    }
    virtual Value* evaluate( Context& cx, NamespaceDefinitionNode* node ) { 
		indent();
		out << "NamespaceDefinition";
        return 0;
    }
    virtual Value* evaluate( Context& cx, PackageDefinitionNode* node ) { 
		indent();
		out << "PackageDefinition: ";
        return 0;
    }
    virtual Value* evaluate( Context& cx, ProgramNode* node ) {
		indent(); 
		out << "Program";
		push_in();
		if( node->statements ) {
			node->statements->evaluate(cx,this);
		}
		pop_out();
		out << '\n';
        return 0;
    }
};

}
}

#endif // NodePrinter_h

/*
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
