/*
 * Parse tree nodes.
 */

#ifndef Nodes_h
#define Nodes_h

#include <string>
#include <typeinfo>

#include "Node.h"
#include "Evaluator.h"
#include "Tokens.h"
#include "Type.h"

namespace esc {
namespace v1 {

using namespace lexer;

class ReferenceValue;

/*
 * IdentifierNode
 */

struct IdentifierNode : public Node {

	std::string name;
	Value* ref;

    IdentifierNode( std::string name, int pos )  : Node(pos) {
        this->name = name;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "IdentifierNode";
    }
};

/**
 * AnnotatedBlockNode
 */

struct AnnotatedBlockNode : public Node {

    Node* attributes;
	Node* statements;

    AnnotatedBlockNode( Node* attributes, Node* statements ) {
        this->attributes = attributes;
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    Node* last() {
        return statements->last();
    }

    const char* toString() {
        return "AnnotatedBlockNode";
    }
};

/**
 * AnnotatedDefinitionNode
 */

struct AnnotatedDefinitionNode : public Node {

    Node* attributes;
	Node* definition;

    AnnotatedDefinitionNode( Node* attributes, Node* definition ) {
        this->attributes = attributes;
        this->definition = definition;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "AnnotatedDefinitionNode";
    }
};

/**
 * AssignmentExpressionNode
 */

struct AssignmentExpressionNode : public Node {

    Node* lhs;
	Node* rhs;
    int  op;
    Value ref;

    AssignmentExpressionNode( Node* lhs, int op, Node* rhs ) {
        this->lhs = lhs;
        this->op = op;
        this->rhs = rhs;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    static bool isFieldName(Node* node) {
        return typeid(node) == typeid(LiteralStringNode) ||
               typeid(node) == typeid(LiteralNumberNode) ||
               typeid(node) == typeid(IdentifierNode) ||
               typeid(node) == typeid(ParenthesizedExpressionNode);
    }

    const char* toString() {
        return "AssignmentExpressionNode";
    }
};

/**
 * AttributeListNode
 */

struct AttributeListNode : public Node {

    Node* item;
	Node* list;

    AttributeListNode( Node* item, Node* list ) {
        this->item = item;
        this->list = list;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "AttributeListNode";
    }
};

/**
 * BinaryExpressionNode
 */

struct BinaryExpressionNode : public Node {

    Node* lhs;
	Node* rhs;
    int  op;
	//Slot lhs_slot,rhs_slot;

    BinaryExpressionNode( int op, Node* lhs, Node* rhs ) {
        this->op  = op;
        this->lhs = lhs;
        this->rhs = rhs;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "BinaryExpressionNode";
    }
};

/**
 * BreakStatementNode
 */

struct BreakStatementNode : public Node {

    Node* expr;

    BreakStatementNode(Node* expr) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "BreakStatementNode";
    }
};

/*
 * CallExpressionNode
 */

struct CallExpressionNode : public Node {

    Node* member;
	ListNode* args;

	ReferenceValue* ref;

    CallExpressionNode( Node* member, ListNode* args ) {
        this->member = member;
        this->args   = args;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

	bool isCallExpression() {
		return true;
	}

    const char* toString() {
        return "CallExpressionNode";
    }
};

/*
 * GetExpressionNode
 */

struct GetExpressionNode : public Node {

    MemberExpressionNode* member;

	ReferenceValue* ref;

    GetExpressionNode( MemberExpressionNode* member ) {
        this->member = member;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "GetExpressionNode";
    }

};

/*
 * SetExpressionNode
 */

struct SetExpressionNode : public Node {

    Node* member;
	Node* args;

	ReferenceValue* ref;

    SetExpressionNode( Node* member, Node* args ) {
        this->member = member;
        this->args   = args;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "SetExpressionNode";
    }
};

/**
 * CaseLabelNode
 */

struct CaseLabelNode : public Node {

    Node* label;
    
    CaseLabelNode( Node* label ) {
        this->label = label;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "CaseLabelNode";
    }
};

/**
 * CatchClauseNode
 */

struct CatchClauseNode : public Node {

    Node* parameter;
	Node* statements;

    CatchClauseNode(Node* parameter, Node* statements) {
        this->parameter = parameter;
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "CatchClauseNode";
    }
};

/**
 * ClassDeclarationNode
 */

struct ClassDeclarationNode : public Node {

    Node* name;

    ClassDeclarationNode( Node* name ) {
        this->name = name;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ClassDeclarationNode";
    }
};

/**
 * ClassDefinitionNode
 */

struct ClassDefinitionNode : public Node {

    Node* name;
	Node* statements;
    InheritanceNode* inheritance;
    //Slot slot;

    static ClassDefinitionNode* ClassDefinition( Node* name, Node* inheritance, Node* statements ) {
        return new ClassDefinitionNode(name,inheritance,statements);
    }

    ClassDefinitionNode( Node* name, Node* inheritance, Node* statements ) {
        this->name        = name;
        this->inheritance = (InheritanceNode*)inheritance;
        this->statements  = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    Node* first() {
        return statements->first();
    }

    Node* last() {
        return statements->last();
    }

    const char* toString() {
        return "ClassDefinitionNode";
    }
};

/**
 * ClassofExpressionNode
 */

struct ClassofExpressionNode : public Node {

    Node* base;

    ClassofExpressionNode( Node* base ) {
        this->base = base;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ClassofExpressionNode";
    }
};

/**
 * CoersionExpressionNode
 */

struct CoersionExpressionNode : public Node {

    Node* expr;
	Node* type;
    std::string type_name;

    CoersionExpressionNode( Node* expr, Node* type, int pos )  : Node(pos) {
        this->expr = expr;
        this->type = type;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "CoersionExpressionNode";
    }
};

/**
 * ConditionalExpressionNode
 */

struct ConditionalExpressionNode : public Node {

	Node* condition;
	Node* thenexpr;
	Node* elseexpr;

	ConditionalExpressionNode( Node* condition, Node* thenexpr, Node* elseexpr ) {
		this->condition = condition;
		this->thenexpr  = thenexpr;
		this->elseexpr  = elseexpr;
	}

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

	const char* toString() {
		return "ConditionalExpressionNode";
	}
};

/**
 * ContinueStatementNode
 */

struct ContinueStatementNode : public Node {

    Node* expr;

    ContinueStatementNode(Node* expr) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "ContinueStatementNode";
    }
};

/**
 * DoStatementNode
 */

struct DoStatementNode : public Node {

    Node* statements;
	Node* expr;

    DoStatementNode(Node* statements, Node* expr) {
        this->statements = statements;
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "DoStatementNode";
    }
};

/**
 * ElementListNode
 */

struct ElementListNode : public Node {

    Node* list;
	Node* item;

    ElementListNode( Node* list, Node* item ) {
        this->list = list;
        this->item = item;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ElementListNode";
    }
};

/**
 * EmptyStatementNode
 */

struct EmptyStatementNode : public Node {

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "EmptyStatementNode";
    }
};

/**
 * ExpressionStatementNode
 */

struct ExpressionStatementNode : public Node {

    Node* expr;

    ExpressionStatementNode( Node* expr ) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ExpressionStatementNode";
    }
};

/**
 * ExportBindingNode
 */

struct ExportBindingNode : public Node {
  
    Node* name;
    Node* value;

    ExportBindingNode( Node* name, Node* value ) {
        this->name  = name;
	    this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ExportBindingNode";
    }
};

/**
 * ExportDefinitionNode
 */

struct ExportDefinitionNode : public Node {
  
    Node* list;

    ExportDefinitionNode( Node* list ) {
	    this->list = list;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ExportDefinitionNode";
    }
};

/**
 * FieldListNode
 */

struct FieldListNode : public Node {

    Node* list;
	Node* field;

    FieldListNode( Node* list, Node* field ) {
        this->list = list;
        this->field     = field;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "FieldListNode";
    }
};

/**
 * FinallyClauseNode
 */

struct FinallyClauseNode : public Node {

    Node* statements;
    
    FinallyClauseNode(Node* statements) {
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "FinallyClauseNode";
    }
};

/**
 * ForStatementNode
 */

struct ForStatementNode : public Node {

    Node* initialize;
	Node* test;
	Node* increment;
	Node* statement;

    ForStatementNode( Node* initialize, Node* test, Node* increment, Node* statement ) {

        if( initialize == NULL ) {
//            this->initialize = NodeFactory::LiteralUndefined();
        } else {
            this->initialize = initialize;
        }

        if( test == NULL ) {
//            this->test = NodeFactory::LiteralBoolean(true);
        } else {
            this->test = test;
        }

        if( increment == NULL ) {
//            this->increment = NodeFactory::LiteralUndefined();
        } else {
            this->increment = increment;
        }

        this->statement = statement;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "ForStatementNode";
    }
};

/**
 * ForInStatementNode
 */

struct ForInStatementNode : public Node {

    Node* property;
	Node* object;
	Node* statement;

    ForInStatementNode( Node* property, Node* object, Node* statement ) {
        this->property  = property;
        this->object    = object;
        this->statement = statement;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "forinstatement";
    }
};

/**
 * FunctionExpressionNode
 */

struct FunctionExpressionNode : public Node {

    Node* name;
	Node* signature;
	Node* body;

    FunctionExpressionNode( Node* name, Node* signature, Node* body ) {
        this->name      = name;
        this->signature = signature;
        this->body      = body;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "FunctionExpressionNode";
    }
};

/**
 * ReturnStatementNode
 */

struct ReturnStatementNode : public Node {

    Node* expr;

    ReturnStatementNode( Node* expr ) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "ReturnStatementNode";
    }
};

/**
 * FunctionDeclarationNode
 */

struct FunctionDeclarationNode : public Node {

    Node* name;
	Node* signature;
    //Slot slot;
	Value ref;

    FunctionDeclarationNode( Node* name, Node* signature ) {
        this->name      = name;
        this->signature = signature;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "FunctionDeclarationNode";
    }
};

/**
 * FunctionDefinitionNode
 */

struct FunctionDefinitionNode : public Node {

    Node* decl;
	Node* body;
	int fixedCount;

    FunctionDefinitionNode( Node* decl, Node* body ) {
        this->decl = decl;
        this->body = body;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    Node* first() {
        return body->first();
    }

    Node* last() {
        return body->last();
    }

    const char* toString() {
        return "FunctionDefinitionNode";
    }
};

/**
 * FunctionNameNode
 */

struct FunctionNameNode : public Node {

    Node* name;
    int  kind;

    FunctionNameNode( int kind, Node* name ) {
	    this->kind = kind;
        this->name = (IdentifierNode*)name;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "functionname";
    }
};

/**
 * FunctionSignatureNode
 */

struct FunctionSignatureNode : public Node {

    Node* parameter;
	Node* result;
    //Slot slot;

    FunctionSignatureNode( Node* parameter, Node* result ) {
        this->parameter = parameter;
        this->result    = result;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "FunctionNameNode";
    }
};

/**
 * IfStatementNode
 */

struct IfStatementNode : public Node {

    Node* condition;
	Node* thenactions;
	Node* elseactions;

    IfStatementNode( Node* condition, Node* thenactions, Node* elseactions ) {
        this->condition   = condition;
        this->thenactions = thenactions;
        this->elseactions = elseactions;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "IfStatementNode";
    }

    bool isBranch() {
        return true;
    }
};

/**
 * ImportBindingNode
 */

struct ImportBindingNode : public Node {

    Node* identifier;
    Node* item;

    ImportBindingNode(Node* identifier, Node* item) {
        this->identifier = identifier;
        this->item = item;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ImportBindingNode";
    }
};

/**
 * ImportDefinitionNode
 */

struct ImportDefinitionNode : public Node {

    Node* item;
	Node* list;

    ImportDefinitionNode(Node* item, Node* list) {
        this->item = item;
        this->list = list;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "importdefinition";
    }
};

/**
 * IncludeStatementNode
 */

struct IncludeStatementNode : public Node {

    Node* item;

    IncludeStatementNode(Node* item) {
        this->item = item;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ImportDefinitionNode";
    }
};

/**
 * InheritanceNode
 */

struct InheritanceNode : public Node {

    Node* baseclass;
	Node* interfaces;

    InheritanceNode( Node* baseclass, Node* interfaces ) {
        this->baseclass  = baseclass;
        this->interfaces = interfaces;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "inheritance";
    }
};

/**
 * InterfaceDeclarationNode
 */

struct InterfaceDeclarationNode : public Node {

    Node* name;

    InterfaceDeclarationNode( Node* name ) {
        this->name = name;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "InheritanceNode";
    }
};

/**
 * InterfaceDefinitionNode
 */

struct InterfaceDefinitionNode : public Node {

    Node* name;
	Node* interfaces;
	Node* statements;
    //Slot slot;

    InterfaceDefinitionNode( Node* name, Node* interfaces, Node* statements ) {
        this->name       = name;
        this->interfaces = interfaces;
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "InterfaceDefinitionNode";
    }
};

/**
 * LabeledStatementNode
 */

struct LabeledStatementNode : public Node {

    Node* label;
    Node* statement;

    LabeledStatementNode(Node* label, Node* statement) {
        this->label     = label;
        this->statement = statement;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "LabeledStatementNode";
    }
};

/**
 * LanguageDeclarationNode
 */

struct LanguageDeclarationNode : public Node {

    Node* list;

    LanguageDeclarationNode(Node* list) {
        this->list = list;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LanguageDeclarationNode";
    }
};

/**
 * ListNode
 */

struct ListNode : public Node {

    ListNode* list;
	Node* item;

    ListNode( ListNode* list, Node* item, int pos )  : Node(pos) {
        this->list = list;
        this->item = item;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    short size() {
		ListNode* node = list;
		short count = 1;
		while( node ) {
			++count; 
			node = node->list; 
		}
		return count;
	}
    
	int pos() {
        return item->pos();
    }

    const char* toString() {
        return "ListNode";
    }
};

/**
 * LiteralArrayNode
 */

struct LiteralArrayNode : public Node {

    Node* elementlist;
	Value value;

    LiteralArrayNode( Node* elementlist ) {
        this->elementlist = elementlist;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralArrayNode";
    }
};

/**
 * LiteralBooleanNode
 */

struct LiteralBooleanNode : public Node {

    bool value;

    LiteralBooleanNode( bool value ) {
        this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralBooleanNode";
    }
};

/**
 * LiteralFieldNode
 */

struct LiteralFieldNode : public Node {

    Node* name;
    Node* value;
	Value ref;

    LiteralFieldNode( Node* name, Node* value ) {
        this->name  = name;
        this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralFieldNode";
    }
};

/**
 * LiteralNullNode
 */

struct LiteralNullNode : public Node {

  LiteralNullNode() {
  }

  Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
  }

  const char* toString() {
    return "LiteralNullNode";
  }
};

/**
 * LiteralNumberNode
 */

struct LiteralNumberNode : public Node {

    std::string value;

    LiteralNumberNode( std::string value ) {
        this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralNumberNode";
    }
};

/**
 * LiteralObjectNode
 */

struct LiteralObjectNode : public Node {

    Node* fieldlist;
	Value value;

    LiteralObjectNode( Node* fieldlist ) {
        this->fieldlist = fieldlist;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralObjectNode";
    }
};

/**
 * LiteralRegExpNode
 */

struct LiteralRegExpNode : public Node {

    std::string value;

    LiteralRegExpNode( std::string value ) {
        this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralRegExpNode";
    }
};

/**
 * LiteralStringNode
 */

struct LiteralStringNode : public Node {

    std::string value;

    LiteralStringNode( std::string value ) {
        this->value = value;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralStringNode";
    }
};

/**
 * LiteralTypeNode
 */

struct LiteralTypeNode : public Node {

    Type* type;

    LiteralTypeNode( Type* type ) {
        this->type = type;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "LiteralTypeNode";
    }
};

/**
 * LiteralUndefinedNode
 */

struct LiteralUndefinedNode : public Node {

    LiteralUndefinedNode() {
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
      return "LiteralUndefinedNode";
    }
};

/**
 * MemberExpressionNode
 */

struct MemberExpressionNode : public Node {

    Node* base;
	Node* expr;
    Value ref;

    MemberExpressionNode( Node* base, Node* expr, int pos )  : Node(pos) {
        this->base = base;
	    this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

	bool isMemberExpression() {
		return true;
	}

    const char* toString() {
        return "MemberExpressionNode";
    }
};

/*
 * IndexedMemberExpressionNode
 */
struct IndexedMemberExpressionNode : public MemberExpressionNode {

    Node* base;
	Node* expr;

    IndexedMemberExpressionNode( Node* base, Node* expr, int pos )  
		: MemberExpressionNode(base,expr,pos) {
        this->base = base;
	    this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "IndexedMemberExpressionNode";
    }
};


/**
 * NamedParameterNode
 */

struct NamedParameterNode : public Node {
  
    Node* name;
    Node* parameter;

    NamedParameterNode( Node* name, Node* parameter ) {
        this->name      = name;
	    this->parameter = parameter;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "NamedParameterNode";
    }
};

/**
 * NamespaceDefinitionNode
 */

struct NamespaceDefinitionNode : public Node {

    Node* name;
	Node* publiclist;

    NamespaceDefinitionNode( Node* name, Node* publiclist ) {
        this->name       = name;
        this->publiclist = publiclist;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "NamespaceDefinitionNode";
    }
};

/**
 * NewExpressionNode
 */

struct NewExpressionNode : public Node {

    Node* expr;

    NewExpressionNode( Node* expr ) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

	bool isFullNewExpression() {
		return this->expr->isCallExpression();
	}

    const char* toString() {
        return "NewExpressionNode";
    }
};

/**
 * PackageDefinitionNode
 */

struct PackageDefinitionNode : public Node {

    Node* name;
	Node* statements;

    PackageDefinitionNode( Node* name, Node* statements ) {
        this->name  = name;
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "PackageDefinitionNode";
    }
};

/**
 * OptionalParameterNode
 */

struct OptionalParameterNode : public Node {
  
    Node* parameter;
    Node* initializer;

    OptionalParameterNode( Node* parameter, Node* initializer ) {
        this->parameter  = parameter;
	    this->initializer = initializer;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "OptionalParameterNode";
    }
};

/**
 * ParameterNode
 */

struct ParameterNode : public Node {
  
    Node* identifier;
    Node* type;
    //Slot slot;

    ParameterNode( Node* identifier, Node* type ) {
        this->identifier  = identifier;
	    this->type        = type;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ParameterNode";
    }
};

/**
 * ParenthesizedExpressionNode
 */

struct ParenthesizedExpressionNode : public Node {

    Node* expr;

    ParenthesizedExpressionNode( Node* expr, int pos ) : Node(pos) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ParenthesizedExpressionNode";
    }
};

/**
 * ParenthesizedListExpressionNode
 */

struct ParenthesizedListExpressionNode : public Node {

    Node* expr;

    ParenthesizedListExpressionNode( Node* expr ) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

	bool isListExpression() {
        return dynamic_cast<ListNode*>(expr) != 0;
	}

    const char* toString() {
        return "ParenthesizedListExpressionNode";
    }
};

/**
 * PostfixExpressionNode
 */

struct PostfixExpressionNode : public Node {

    int  op;
    Node* expr;

    PostfixExpressionNode( int op, Node* expr ) {
        this->op   = op;
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "PostfixExpressionNode";
    }
};

/**
 * ProgramNode
 */

struct ProgramNode : public Node {

    Node* statements;

    ProgramNode(Node* statements) {
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ProgramNode";
    }
};

/**
 * QualifiedIdentifierNode
 *
 * QualifiedIdentifier : QualifiedIdentifier Identifier
 */

struct QualifiedIdentifierNode : IdentifierNode {

    Node* qualifier; 

    QualifiedIdentifierNode( Node* qualifier, IdentifierNode* identifier, int pos ) 
			: IdentifierNode(identifier->name,pos) {
        this->qualifier = qualifier;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "QualifiedIdentifierNode";
    }
};

/**
 * RestParameterNode
 */

struct RestParameterNode : public Node {
  
    Node* parameter;

    RestParameterNode( Node* parameter ) {
        this->parameter = parameter;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "RestParameterNode";
    }
};

/**
 * StatementListNode
 */

struct StatementListNode : public Node {

    StatementListNode* list;
    Node* item;

    StatementListNode( StatementListNode* list, Node* item ) {
        this->list = list;
	    this->item = item;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    Node* first() {
        StatementListNode* node = this;
        while(node->list!=NULL) {
            node = node->list;
        }
        return node->item;
    }

    Node* last() {
        return this->item;
    }

    const char* toString() {
        return "StatementListNode";
    }
};

/**
 * SwitchStatementNode
 */

struct SwitchStatementNode : public Node {

    Node* expr;
    StatementListNode* statements;
    
    SwitchStatementNode( Node* expr, StatementListNode* statements ) {
        this->expr = expr;
        this->statements = statements;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "SwitchStatementNode";
    }
};

/**
 * SuperExpressionNode
 */


struct SuperExpressionNode : public Node {

	Node* expr;

	SuperExpressionNode(Node* expr) : expr(expr) {
	}

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "SuperExpressionNode";
    }

	bool isFullSuperExpression() {
		return expr != 0;
	}

};

/**
 * ThisExpressionNode
 */


struct ThisExpressionNode : public Node {

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "ThisExpressionNode";
    }
};

/**
 * ThrowStatementNode
 */

struct ThrowStatementNode : public Node {

    Node* expr;
    
    ThrowStatementNode(Node* expr) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "ThrowStatementNode";
    }
};

/**
 * TryStatementNode
 */

struct TryStatementNode : public Node {

    Node* tryblock;
    StatementListNode* catchlist;
    Node* finallyblock;

    TryStatementNode(Node* tryblock, StatementListNode* catchlist, Node* finallyblock) {
        this->tryblock     = tryblock;
        this->catchlist    = catchlist;
        this->finallyblock = finallyblock;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "TryStatementNode";
    }
};

/**
 * TypedVariableNode
 */

struct TypedVariableNode : public Node {
  
    IdentifierNode* identifier;
    IdentifierNode* type;
    //Slot slot;

    TypedVariableNode( Node* identifier, Node* type, int pos ) : Node(pos) {
        this->identifier  = (IdentifierNode*)identifier;
	    this->type        = (IdentifierNode*)type;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "TypedVariableNode";
    }
};

/**
 * UnaryExpressionNode
 */

struct UnaryExpressionNode : public Node {

    Node* expr;
    int  op;

    UnaryExpressionNode( int op, Node* expr ) {
	    this->op   = op;
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "UnaryExpressionNode";
    }
};

/**
 * UnitExpressionNode
 */

struct UnitExpressionNode : public Node {

    Node* value;
	Node* type;

    UnitExpressionNode( Node* value, Node* type ) {
        this->value = value;
        this->type  = type;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "UnitExpressionNode";
    }
};

/**
 * UseStatementNode
 */

struct UseStatementNode : public Node {

    Node* expr;

    UseStatementNode( Node* expr ) {
        this->expr = expr;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "UseStatementNode";
    }
};

/**
 * VariableBindingNode
 */

struct VariableBindingNode : public Node {
  
    TypedVariableNode* variable;
    Node* initializer;

    VariableBindingNode( Node* variable, Node* initializer ) {
        this->variable  = (TypedVariableNode*)variable;
	    this->initializer = initializer;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    int pos() {
        return variable->pos();
    }

    const char* toString() {
        return "VariableBindingNode";
    }
};

/**
 * VariableDefinitionNode
 */

struct VariableDefinitionNode : public Node {
  
    int  kind;
    Node* list;

    VariableDefinitionNode( int kind, Node* list, int pos ) : Node(pos) {
        this->kind = kind;
	    this->list = list;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "VariableDefinitionNode";
    }
};

/**
 * WhileStatementNode
 */

struct WhileStatementNode : public Node {

    Node* expr;
	Node* statement;

    WhileStatementNode( Node* expr, Node* statement ) {
	    this->expr      = expr;
	    this->statement = statement;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    bool isBranch() {
        return true;
    }

    const char* toString() {
        return "WhileStatementNode";
    }
};

/**
 * WithStatementNode
 */

struct WithStatementNode : public Node {

    Node* expr;
    Node* statement;

    WithStatementNode( Node* expr, Node* statement ) {
	    this->expr      = expr;
	    this->statement = statement;
    }

    Value* evaluate( Context& cx, Evaluator* evaluator ) {
		if( evaluator->checkFeature(cx,this) ) {
			return evaluator->evaluate( cx, this );
		} else {
			return 0;
		}
    }

    const char* toString() {
        return "WithStatementNode";
    }
};

}
}

#endif // Nodes_h

/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
