/*
 * ClassFileGenerator
 */

#ifndef ClassFileGenerator_h
#define ClassFileGenerator_h

#include <vector>

#include "Value.h"
#include "Context.h"
#include "Evaluator.h"

#include "ByteCodeFactory.h"
//#include "ClassFileConstants.h"

namespace esc {
namespace v1 {


class ClassFileGenerator : public Evaluator, private ByteCodeFactory {

	struct ClassFile : protected ClassFileConstants {
		int    magic; 
		short  minor_version; 
		short  major_version;
		std::vector<std::vector<byte>*>* constant_pool;
		short  constant_pool_count;
		short  access_flags;
		short  this_class;
		short  super_class;
		short  interfaces_count;
		std::vector<std::vector<byte>*>* interfaces;
		short  fields_count;
		std::vector<std::vector<byte>*>* fields;
		// start method
		std::vector<byte>* exception_table;
		std::vector<byte>* code_attributes;
		std::vector<std::vector<byte>*>* methods;
		std::vector<byte>* code;  // use this for the start method (the global script)
		short methods_count;
		short  attributes_count;
		std::vector<std::vector<byte>*>* attributes;

		ClassFile() :
			magic(0xCAFEBABE), 
			minor_version(0),
			major_version(46),
			constant_pool(new std::vector<std::vector<byte>*>()),
			constant_pool_count((short)0),
			access_flags(ACC_SUPER|ACC_PUBLIC),
			this_class((short)2),
			super_class((short)3),
			interfaces_count((short)0),
			interfaces(new std::vector<std::vector<byte>*>()),
			fields_count(0),
			fields(new std::vector<std::vector<byte>*>()),
			exception_table(new std::vector<byte>()),
			code_attributes(new std::vector<byte>()),
			methods(new std::vector<std::vector<byte>*>()),
			code(new std::vector<byte>()),
			methods_count(0),
			attributes_count(0),
			attributes(new std::vector<std::vector<byte>*>()) 
		{
		}

		~ClassFile() {
			delete constant_pool;
			delete interfaces;
			delete fields;
			delete exception_table;
			delete code_attributes;
			delete methods;
			delete code;
			delete attributes;
		}

		short addConstant(std::vector<byte>& bytes);

	};

	ClassFile& cf;
	char max_locals;   // use to allocate local variables

    void init(std::string scriptname);

public:

	/*
	 * Test driver
	 */

    static int main(int argc, char* argv[]);

	/*
	 */

	ClassFileGenerator(std::string scriptname) 
		: cf(*new ClassFile()) {
		init(scriptname);
	};

	~ClassFileGenerator() {
		delete &cf;
	}

    std::vector<byte>& emit(std::vector<byte>& bytes);

	// Base node
    
	Value* evaluate( Context& cx, Node* node );

	// 3rd Edition features
	
	Value* evaluate( Context& cx, IdentifierNode* node );
    Value* evaluate( Context& cx, ThisExpressionNode* node );
    Value* evaluate( Context& cx, LiteralBooleanNode* node );
    Value* evaluate( Context& cx, LiteralNumberNode* node );
    Value* evaluate( Context& cx, LiteralStringNode* node );
    Value* evaluate( Context& cx, LiteralUndefinedNode* node );
    Value* evaluate( Context& cx, LiteralRegExpNode* node );
    Value* evaluate( Context& cx, FunctionExpressionNode* node );
    Value* evaluate( Context& cx, ParenthesizedExpressionNode* node );
    Value* evaluate( Context& cx, ParenthesizedListExpressionNode* node );
    Value* evaluate( Context& cx, LiteralObjectNode* node );
    Value* evaluate( Context& cx, LiteralFieldNode* node );
    Value* evaluate( Context& cx, LiteralArrayNode* node );
    Value* evaluate( Context& cx, PostfixExpressionNode* node );
    Value* evaluate( Context& cx, NewExpressionNode* node );
    Value* evaluate( Context& cx, IndexedMemberExpressionNode* node );
    Value* evaluate( Context& cx, MemberExpressionNode* node );
    Value* evaluate( Context& cx, CallExpressionNode* node );
    Value* evaluate( Context& cx, GetExpressionNode* node );
    Value* evaluate( Context& cx, SetExpressionNode* node );
    Value* evaluate( Context& cx, UnaryExpressionNode* node );
    Value* evaluate( Context& cx, BinaryExpressionNode* node );
    Value* evaluate( Context& cx, ConditionalExpressionNode* node );
    Value* evaluate( Context& cx, AssignmentExpressionNode* node );
    Value* evaluate( Context& cx, ListNode* node );
    Value* evaluate( Context& cx, StatementListNode* node );
    Value* evaluate( Context& cx, EmptyStatementNode* node );
    Value* evaluate( Context& cx, ExpressionStatementNode* node );
    Value* evaluate( Context& cx, AnnotatedBlockNode* node );
    Value* evaluate( Context& cx, LabeledStatementNode* node );
    Value* evaluate( Context& cx, IfStatementNode* node );
    Value* evaluate( Context& cx, SwitchStatementNode* node );
    Value* evaluate( Context& cx, CaseLabelNode* node );
    Value* evaluate( Context& cx, DoStatementNode* node );
    Value* evaluate( Context& cx, WhileStatementNode* node );
    Value* evaluate( Context& cx, ForInStatementNode* node );
    Value* evaluate( Context& cx, ForStatementNode* node );
    Value* evaluate( Context& cx, WithStatementNode* node );
    Value* evaluate( Context& cx, ContinueStatementNode* node );
    Value* evaluate( Context& cx, BreakStatementNode* node );
    Value* evaluate( Context& cx, ReturnStatementNode* node );
    Value* evaluate( Context& cx, ThrowStatementNode* node );
    Value* evaluate( Context& cx, TryStatementNode* node );
    Value* evaluate( Context& cx, CatchClauseNode* node );
    Value* evaluate( Context& cx, FinallyClauseNode* node );
    Value* evaluate( Context& cx, AnnotatedDefinitionNode* node );
    Value* evaluate( Context& cx, VariableDefinitionNode* node );
    Value* evaluate( Context& cx, VariableBindingNode* node );
    Value* evaluate( Context& cx, FunctionDefinitionNode* node );
    Value* evaluate( Context& cx, FunctionDeclarationNode* node );
    Value* evaluate( Context& cx, FunctionNameNode* node );
    Value* evaluate( Context& cx, FunctionSignatureNode* node );
    Value* evaluate( Context& cx, ParameterNode* node );
    Value* evaluate( Context& cx, ProgramNode* node );

	// 4th Edition features

    Value* evaluate( Context& cx, QualifiedIdentifierNode* node );
    Value* evaluate( Context& cx, UnitExpressionNode* node );
    Value* evaluate( Context& cx, ClassofExpressionNode* node );
    Value* evaluate( Context& cx, CoersionExpressionNode* node );
    Value* evaluate( Context& cx, UseStatementNode* node );
    Value* evaluate( Context& cx, IncludeStatementNode* node );
    Value* evaluate( Context& cx, ImportDefinitionNode* node );
    Value* evaluate( Context& cx, ImportBindingNode* node );
    Value* evaluate( Context& cx, AttributeListNode* node );
    Value* evaluate( Context& cx, ExportDefinitionNode* node );
    Value* evaluate( Context& cx, ExportBindingNode* node );
    Value* evaluate( Context& cx, TypedVariableNode* node );
    Value* evaluate( Context& cx, OptionalParameterNode* node );
    Value* evaluate( Context& cx, ClassDefinitionNode* node );
    Value* evaluate( Context& cx, ClassDeclarationNode* node );
    Value* evaluate( Context& cx, InheritanceNode* node );
    Value* evaluate( Context& cx, NamespaceDefinitionNode* node );
    Value* evaluate( Context& cx, PackageDefinitionNode* node );
};

}
}

#endif // ClassFileGenerator_h

/*
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
