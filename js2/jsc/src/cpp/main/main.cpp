// main.cpp : Defines the entry point for the console application.
//
#pragma warning( disable : 4786 )
#include <stdio.h>
#include <fstream>
#include "ByteCodeFactory.h"
#include "ObjectValue.h"
#include "ConstantEvaluator.h"
#include "NodeFactory.h"
#include "ClassFileGenerator.h"
#include "TypeValue.h"
#include "InputBuffer.h"
#include "Token.h"
#include "Scanner.h"
#include "Parser.h"
#include "Node.h"
#include "NodePrinter.h"

int main(int argc, char* argv[])
{
	using namespace esc::v1;
	using namespace esc::v1::parser;

	printf("esc\n");

	bool show_parsetrees = false;
	bool do_test = false;
	bool do_help = false;
	std::string source;

	if( argc == 1 ) {
		++do_test;
	} else {
		for( int i = argc-1; i > 0; i-- ) {
			char* flag = argv[i];
			if(flag[0]=='-') {
				switch(flag[1]) {
				case 'p':
					++show_parsetrees;
					break;
				case 'h':
				default:
					++do_help;
					break;
				}
			} else {
				source = argv[i];
			}
		}
	}
	if( do_help ) {
		return 0;
	}

	TypeValue::init();
	ObjectValue::init();

	if( do_test ) {

		std::ofstream dbg("test.dbg");
		Debugger::init(&dbg);

		InputBuffer::main(argc,argv);
		InputBuffer::test_getLineText();
		InputBuffer::test_markandcopy();

		using namespace esc::v1::lexer;

		Token::main(argc,argv);
		Scanner::main(argc,argv);
		
		NodeFactory::main(argc,argv);

		using namespace esc::v1::parser;
		Parser::main(argc,argv);

		ConstantEvaluator::main(argc,argv);
		ObjectValue::main(argc,argv);
		ByteCodeFactory::main(argc,argv);
		ClassFileGenerator::main(argc,argv);

	} else {

		// Init the subsystems

		NodeFactory::init();
		std::ifstream in = std::ifstream((source+".js").c_str());
		std::ofstream err((source+".err").c_str());
		std::ofstream dbg((source+".dbg").c_str());

		Debugger::init(&dbg);
        Context cx(err);

		// Parse
		
		Parser* parser = new Parser(in,err,source);
        ProgramNode* node = parser->parseProgram();

		if( show_parsetrees ) {
			NodePrinter* printer = new NodePrinter();
			node->evaluate(cx,printer);
			cout << printer->str().c_str();
			return 0;
		}		

		// Analyze

		ConstantEvaluator* analyzer = new ConstantEvaluator();
		node->evaluate(cx,analyzer);

		// Generate
		
		ClassFileGenerator* generator = new ClassFileGenerator(source);
		node->evaluate(cx,generator);

		// Write the class file
		
		std::vector<byte>& bytes=*new std::vector<byte>();
		generator->emit(bytes);			// Emit it
		std::ofstream* of = new std::ofstream((source+".class").c_str(),std::ios::binary);
		of->write(bytes.begin(),bytes.size());

		// Clean up

		delete generator;  // Get rid of the generator after one use.
		delete analyzer;

		NodeFactory::clear();
	}

	ObjectValue::fini();
	TypeValue::fini();

	return 0;
}

/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company.
 * All Rights Reserved.
 */
