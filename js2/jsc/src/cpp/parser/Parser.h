#ifndef Parser_h
#define Parser_h

/*
 * Parse ECMAScript programs.
 */

#include <string>
#include <vector>
#include <sstream>
#include <stdio.h>

#include "Tokens.h"
#include "Token.h"
#include "States.h"
#include "InputBuffer.h"
#include "CharacterClasses.h"
#include "Debugger.h"
#include "Nodes.h"
#include "NodeFactory.h"
#include "NodePrinter.h"

using namespace esc::v1::lexer;

namespace esc {
namespace v1 {
namespace parser {

static const debug = true;

const int abbrevIfElse_mode = else_token;   // lookahead uses this value. don't change.
const int abbrevDoWhile_mode = while_token; // ditto.
const int abbrev_mode  = last_token-1;
const int full_mode    = abbrev_mode-1;

const int allowIn_mode = full_mode-1;
const int noIn_mode    = allowIn_mode-1;

const int syntax_error = 0;

class Parser {

    Scanner* scanner;
    int      lasttoken;
    int      nexttoken;
    int      thistoken;
    bool     isNewLine;
	std::ostream& err;
	int      error_count;


    /*
     * Log a syntax error and recover
     */

    void error(std::string msg) {
		error(syntax_error,msg,error_token);
	}

    void error(int kind, std::string msg, int tokenid) {
	    error_count++;
		std::ostringstream loc;
        switch(kind) {
            case syntax_error:
            default:
                cout << "Ln " << (scanner->input->markLn+1) << ", Col " << (scanner->input->markCol) << ": ";
				//msg << msg?error_messages[kind]:msg;
                break;
        }
		cout << msg.c_str();
        cout << "\n";
        cout << scanner->input->getLineText(scanner->input->positionOfMark()).c_str();
        cout << "\n";
		cout << scanner->getLinePointer(scanner->input->markCol-1).c_str();
        cout << "\n";
        skiperror(kind);
    }

    void error(int kind, std::string msg) {
        error(kind,msg,error_token);    
    }

    void error(int kind) {
        error(kind,0);    
    }

    /**
     * skip ahead after an error is detected. this simply goes until the next
     * whitespace or end of input->
     */

    void skiperror() {
        skiperror(syntax_error);
    }

    void skiperror(int kind) {
		if( debug ) {
	    //    Debugger::trace("skipping syntax error\n");
		}
        switch(kind) {
            case syntax_error:
            default:
                while ( true ) {
                    char nc = scanner->nextchar();
                    if( nc == ';' || nc == '\n' || nc == 0 ) {
                        return;
                    }
                }
        } 
    }


public:
    /*
     * 
     */

    Parser (std::istream& in, std::ostream& err, std::string& name) 
		: err(err), lasttoken(empty_token), nexttoken(empty_token), thistoken(empty_token),
		  isNewLine(false), error_count(0) {
        
		this->scanner = new Scanner(in,err,name);
        //this.statements = new Vector();
        //this.functions  = new Vector();
        //NodeFactory::init(scanner->input);
        //Context.init(scanner->input);
    }

    /**
     *
     */

    bool newline() {
        if( nexttoken == empty_token ) {
            nexttoken = scanner->nexttoken();
        }
        return scanner->followsLineTerminator();
    }

    /**
     * Match the current input with an expected token. lookahead is managed by 
     * setting the state of this.nexttoken to empty_token after an match is
     * attempted. the next lookahead will initialize it again.
     */

    int match( int expectedTokenClass ) {

	    //if( debug ) {
	    //    Debugger::trace( "matching " + Token.getTokenClassName(expectedTokenClass) + " with input " +
	    //                  Token.getTokenClassName(scanner->getTokenClass(nexttoken)) );
        //}

        int result = error_token;

        if( !lookahead( expectedTokenClass ) ) {
            error(syntax_error,"Expecting " + Token::getTokenClassName(expectedTokenClass) +
	                      " before " + scanner->getTokenText(nexttoken),error_token);
										// leak!
			nexttoken = empty_token;
        } else
        if( expectedTokenClass != scanner->getTokenClass( nexttoken ) ) {
	        result    = thistoken;
	    } else {
	        result    = nexttoken;
            lasttoken = nexttoken;
            nexttoken = empty_token;
	    }

	    //if( (debug || debug_match) ) {
	    //    Debugger::trace( "match " + Token.getTokenClassName(expectedTokenClass) );
        //}

        return result;
    }

    /**
     * Match the current input with an expected token. lookahead is managed by 
     * setting the state of this.nexttoken to empty_token after an match is
     * attempted. the next lookahead will initialize it again.
     */

    bool lookaheadSemicolon(int mode) {

	    //if( debug ) {
	    //    Debugger::trace( "looking for virtual semicolon with input " +
	    //                  Token.getTokenClassName(scanner->getTokenClass(nexttoken)) );
        //}

        bool result = false;

        if( lookahead(semicolon_token) ||
            lookahead(eos_token) ||
            lookahead(rightbrace_token) ||
            lookahead(mode) ||
            scanner->followsLineTerminator() ) {
	        result = true;
	    } 

        return result;
    }

    bool lookaheadNoninsertableSemicolon(int mode) {

	    //if( debug ) {
	    //    Debugger::trace( "looking for virtual semicolon with input " +
	    //                  Token.getTokenClassName(scanner->getTokenClass(nexttoken)) );
        //}

        bool result = false;

        if( lookahead(eos_token) ||
            lookahead(rightbrace_token) ||
            lookahead(mode) ) {
	        result = true;
	    } 

        return result;
    }

    int matchSemicolon(int mode) {

	    //if( debug ) {
	    //    Debugger::trace( "matching semicolon with input " +
	    //                  Token.getTokenClassName(scanner->getTokenClass(nexttoken)) );
        //}

        int result = error_token;

        if( lookahead( semicolon_token ) ) {
            result = match(semicolon_token);
        } else if( scanner->followsLineTerminator() || 
            lookahead(eos_token) ||
            lookahead(rightbrace_token) ) {
	        result = semicolon_token;
        } else if( (mode == abbrevIfElse_mode || mode == abbrevDoWhile_mode) 
                   && lookahead(mode) ) {
	        result = semicolon_token;
	    } else {
            error(syntax_error,"Expecting semicolon before '" + scanner->getTokenText(nexttoken));
	    }

        return result;
    }

    /**
     * Match a non-insertable semi-colon. This function looks for
     * a semicolon_token or other grammatical markers that indicate
     * that the empty_token is equivalent to a semicolon.
     */

    int matchNoninsertableSemicolon(int mode) {

	    //if( debug ) {
	    //    Debugger::trace( "matching semicolon with input " +
	    //                  Token.getTokenClassName(scanner->getTokenClass(nexttoken)) );
        //}

        int result = error_token;

        if( lookahead( semicolon_token ) ) {
            result = match(semicolon_token);
        } else if( lookahead(eos_token) ||
                   lookahead(rightbrace_token) ) {
	        result = semicolon_token;
        } else if( (mode == abbrevIfElse_mode || mode == abbrevDoWhile_mode) 
                   && lookahead(mode) ) {
	        result = semicolon_token;
	    } else {
            error(syntax_error,"Expecting semicolon before '" + scanner->getTokenText(nexttoken));
	    }

        return result;
    }

    /**
     * Lookahead to the next token.
     */

    bool lookahead( int expectedTokenClass ) {

        bool result = false;

	    // If the nexttoken is empty_token, then fetch another. This is the first
	    // lookahead since the last match.
	    
        if( nexttoken == empty_token ) {
            nexttoken = scanner->nexttoken();
			if( debug ) {
				Debugger::trace( "\tnexttoken() returning " + Token::getTokenClassName(scanner->getTokenClass(nexttoken)) );
            }
        }

	    if( debug ) {
            Debugger::trace( "\t" + Token::getTokenClassName(scanner->getTokenClass(nexttoken)) + " lookahead(" + Token::getTokenClassName(expectedTokenClass) + ")");
        }

	    // Check for invalid token.
    
	    if( nexttoken == error_token ) {
			error(syntax_error,"Invalid word");
        }

	    // Compare the expected token class against the token class of 
        // the nexttoken.

        if( expectedTokenClass != scanner->getTokenClass(nexttoken) ) {
            return false;
        } else {
            thistoken = expectedTokenClass;
            return true;
	    }
    }

	int errorCount() {
	    return error_count;
	}

    /**
     * Start grammar.
     */

    /**
     * Identifier	
     *     Identifier
     *     get
     *     set
	 *     exclude
	 *     include
     */

    IdentifierNode* parseIdentifier() {

        if( debug ) {
            Debugger::trace( "begin parseIdentifier" );
        }

        IdentifierNode* result = 0;

        if( lookahead( get_token ) ) {
            match( get_token );
            result = NodeFactory::Identifier("get");
        } else if( lookahead( set_token ) ) {
            match( set_token );
            result = NodeFactory::Identifier("set");
        } else if( lookahead( exclude_token ) ) {
            match( exclude_token );
            result = NodeFactory::Identifier("exclude");
        } else if( lookahead( include_token ) ) {
            match( include_token );
            result = NodeFactory::Identifier("include");
        } else if( lookahead( identifier_token ) ) {
            result = NodeFactory::Identifier(scanner->getTokenText(match(identifier_token)));
        } else {
            error(syntax_error,"Expecting an Identifier.");
        }

        if( debug ) {
            Debugger::trace("finish parseIdentifier");
        }
        return result;
    }

    static void testIdentifier() {

        Debugger::trace( "begin testIdentifier" );

		std::string input[] = { 
            "xx",
            "get",
            "set", 
            "exclude", 
            "(include)", 
        };

		std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[xx]]]]]]", 
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[get]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[set]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[exclude]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|List[|MemberExpression[|Identifier[include]]]]]]]",
        };

        testParser("Identifier",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * Qualifier	
	 *     Identifier
	 *     public
	 *     private
     */

    Node* parseQualifier() {

        if( debug ) {
            Debugger::trace( "begin parseQualifier" );
        }

        Node* result;

        if( lookahead( public_token ) ) {
            match( public_token );
            result = NodeFactory::Identifier("public");
        } else if( lookahead( private_token ) ) {
            match( private_token );
            result = NodeFactory::Identifier("private");
        } else {
            result = parseIdentifier();
        }

        if( debug ) {
            Debugger::trace( "finish parseQualifier" );
        }

        return result;
    }

    /*
     * SimpleQualifiedIdentifier	
     *     Identifier
     *     Qualifier :: Identifier
     */
	
    Node* parseSimpleQualifiedIdentifier() {

        if( debug ) {
            Debugger::trace( "begin parseSimpleQualifiedIdentifier" );
        }

        Node* result;
		Node* first;

        if( lookahead( public_token ) ) {
            match(public_token);
            first = NodeFactory::Identifier("public");
            match(doublecolon_token);
            result = NodeFactory::QualifiedIdentifier(first,parseIdentifier());
        } else if( lookahead(private_token) ) {
            match(private_token);
            first = NodeFactory::Identifier("private");
            match(doublecolon_token);
            result = NodeFactory::QualifiedIdentifier(first,parseIdentifier());
        } else {
            first = parseIdentifier();
            if( lookahead(doublecolon_token) ) {
                match(doublecolon_token);
                result = NodeFactory::QualifiedIdentifier(first,parseIdentifier());
            } else {
                result = first;
            }
        }
                 
        if( debug ) {
            Debugger::trace( "finish parseSimpleQualifiedIdentifier" );
        }

        return result;
    }

    static void testSimpleQualifiedIdentifier() {

        Debugger::trace( "begin testSimpleQualifiedIdentifier" );

        std::string input[] = { 
            "x::y",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|QualifiedIdentifier[y|Identifier[x]]]]]]]",
        };

        testParser("SimpleQualifiedIdentifier",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * ExpressionQualifiedIdentifier	
     *     ParenthesizedExpression :: Identifier
     */

    Node* parseExpressionQualifiedIdentifier() {

        if( debug ) {
            Debugger::trace( "begin parseExpressionQualifiedIdentifier" );
        }

        Node* result;
		Node* first;

        first = parseParenthesizedExpression();
        match(doublecolon_token);
        result = NodeFactory::QualifiedIdentifier(first,parseIdentifier());

        if( debug ) {
            Debugger::trace( "finish parseExpressionQualifiedIdentifier" );
        }

        return result;
    }

    static void testExpressionQualifiedIdentifier() {

        Debugger::trace( "begin testExpressionQualifiedIdentifier" );

        std::string input[] = { 
            "(x)::y",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|QualifiedIdentifier[y|MemberExpression[|Identifier[x]]]]]]]", 
        };

        testParser("ExpressionQualifiedIdentifier",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * QualifiedIdentifier	
     *     SimpleQualifiedIdentifier
     *     ExpressionQualifiedIdentifier
     */
	
    Node* parseQualifiedIdentifier() {

        if( debug ) {
            Debugger::trace( "begin parseQualifiedIdentifier" );
        }

        Node* result;

        if( lookahead( leftparen_token ) ) {
            result = parseExpressionQualifiedIdentifier();
        } else {
            result = parseSimpleQualifiedIdentifier();
        }
         
        if( debug ) {
            Debugger::trace( "finish parseQualifiedIdentifier" );
        }

        return result;
    }

    static void testQualifiedIdentifier() {

        Debugger::trace( "begin testQualifiedIdentifier" );

        std::string input[] = { 
	        "(y)::x",       // ParenthesizedExpression :: QualifiedIdentifier
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|QualifiedIdentifier[x|MemberExpression[|Identifier[y]]]]]]]",
        };

        testParser("QualifiedIdentifier",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * UnitExpression	
     * 	   ParenthesizedListExpression UnitExpressionPrime
     * 	   Number UnitExpressionPrime
     * 	
     * UnitExpressionPrime	
     * 	   [no line break] String UnitExpressionPrime
     * 	   empty
     */
	
    Node* parseUnitExpression() {

        if( debug ) {
            Debugger::trace( "begin parseUnitExpression" );
        }

        Node* result;
		Node* first;

        if( lookahead( leftparen_token ) ) {
            first = parseParenthesizedListExpression();
        } else if( lookahead(numberliteral_token) ) {
			first = NodeFactory::LiteralNumber(scanner->getTokenText(match(numberliteral_token)));
        }
         
		result = parseUnitExpressionPrime(first);

        if( debug ) {
            Debugger::trace( "finish parseUnitExpression" );
        }

        return result;
    }

    Node* parseUnitExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseUnitExpressionPrime" );
        }

        Node* result;

		// Error if linebreak here.

        if( lookahead( stringliteral_token ) ) {
            Node* second = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
			result = parseUnitExpressionPrime(NodeFactory::UnitExpression(first,second));
        } else {
			result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseUnitExpressionPrime" );
        }

        return result;
    }

    static void testUnitExpression() {

        Debugger::trace( "begin testUnitExpression" );

        std::string input[] = { 
	        "(y) 'pesos'",
			"5 'dollars'",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|UnitExpression[List[|MemberExpression[|Identifier[y]]]|LiteralString:pesos]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnitExpression[LiteralNumber:5|LiteralString:dollars]]]]]",
        };

        testParser("UnitExpression",input,expected,sizeof(input)/sizeof(std::string));
    }


			
	/*
     * PrimaryExpression	
     *     null
     *     true
     *     false
     *     public
     *     Number
     *     String
     *     this
     *     RegularExpression
     *     UnitExpression
     *     ArrayLiteral
     *     ObjectLiteral
     *     FunctionExpression
     */

    Node* parsePrimaryExpression() {

        if( debug ) {
            Debugger::trace( "begin parsePrimaryExpression" );
        }

        Node* result;

        if( lookahead( null_token ) ) {
            match( null_token );
            result = NodeFactory::LiteralNull();
        } else if( lookahead(true_token) ) {
            match( true_token );
            result = NodeFactory::LiteralBoolean(true);
        } else if( lookahead(false_token) ) {
            match( false_token );
            result = NodeFactory::LiteralBoolean(false);
        } else if( lookahead(public_token) ) {
            match( public_token );
            result = NodeFactory::Identifier("public");
        } else if( lookahead(numberliteral_token) || lookahead(leftparen_token) ) {
            result = parseUnitExpression(); //NodeFactory::LiteralNumber(scanner->getTokenText(match(numberliteral_token)));
        } else if( lookahead(stringliteral_token) ) {
            result = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
        } else if( lookahead(this_token) ) {
            match( this_token );
            result = NodeFactory::ThisExpression();
        } else if( lookahead(regexpliteral_token) ) {
            result = NodeFactory::LiteralRegExp(scanner->getTokenText(match(regexpliteral_token)));
        } else if( lookahead(leftbracket_token) ) {
            result = parseArrayLiteral();
        } else if( lookahead(leftbrace_token) ) {
            result = parseObjectLiteral();
        } else if( lookahead(function_token) ) {
            result = parseFunctionExpression();
        } else {
            error(syntax_error,"Expecting <primary expression> before '" + 
                          scanner->getTokenText(nexttoken),error_token);
        }

        if( debug ) {
            Debugger::trace( "finish parsePrimaryExpression" );
        }

        return result;
    }

    static void testPrimaryExpression() {

        Debugger::trace( "begin testPrimaryExpression" );

        std::string input[] = { 
            "null",               //null
            "true",               //true
            "false",              //false
            //"(public)",           //public
            "123",                //Number
            "'abc'",              //String
            "this",               //this
	        "/xyz/gim",           //RegularExpression
	        "(y)",                //ParenthesizedExpression
	        "[a,b,c]",            //ArrayLiteral
	        "({a:1,b:2,c:3})",      //LiteralObject
	        "(function f(x,y) {})", //FunctionExpression
        };

        std::string expected[] = {
            "Program[StatementList[ExpressionStatement[List[|LiteralNull]]]]", 
            "Program[StatementList[ExpressionStatement[List[|LiteralBoolean:1]]]]", 
            "Program[StatementList[ExpressionStatement[List[|LiteralBoolean:0]]]]", 
            //"Program[StatementList[ExpressionStatement[List[|ParenthesizedListExpressionList[|Identifier:public]]]]]", 
            "Program[StatementList[ExpressionStatement[List[|LiteralNumber:123]]]]", 
            "Program[StatementList[ExpressionStatement[List[|LiteralString:abc]]]]", 
            "Program[StatementList[ExpressionStatement[List[|ThisExpression]]]]", 
            "Program[]", 
            "Program[StatementList[ExpressionStatement[List[|ParenthesizedListExpressionList[|GetExpression[MemberExpression[Identifier:y]]]]]]]", 
            "Program[StatementList[ExpressionStatement[List[|LiteralArray[List[List[List[|MemberExpression[|Identifier[a]]]|MemberExpression[|Identifier[b]]]|MemberExpression[|Identifier[c]]]]]]]]", 
            "Program[]", 
            "Program[StatementList[ExpressionStatement[List[|List[|FunctionExpression[Identifier[f]|FunctionSignature[List[List[|Parameter[Identifier[x]|]]|Parameter[Identifier[y]|]]|]|]]]]]]", 
        };

        testParser("PrimaryExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * ParenthesizedExpression	
     *     ( AssignmentExpressionallowIn )
     */

    Node* parseParenthesizedExpression() {

        if( debug ) {
            Debugger::trace( "begin parseParenthesizedExpression" );
        }

        Node* result;
    
        match(leftparen_token); 
		int mark = scanner->input->positionOfMark();
        result = parseAssignmentExpression(allowIn_mode);
        result->position = mark;
        match(rightparen_token);
         
        if( debug ) {
            Debugger::trace( "finish parseParenthesizedExpression" );
        }

        return result;
    }

    static void testParenthesizedExpression() {

        std::string input[] = { 
	        "(null)",
        };

        std::string expected[] = { 
            "",
        };

        testParser("ParenthesizedExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * ParenthesizedListExpression	
     *     ParenthesizedExpression
     *     ( ListExpressionallowIn , AssignmentExpressionallowIn )
     */

    //ParenthesizedListExpressionNode* parseParenthesizedListExpression() {
    ListNode* parseParenthesizedListExpression() {

        if( debug ) {
            Debugger::trace( "begin parseParenthesizedListExpression" );
        }

        //ParenthesizedListExpressionNode* result;
        ListNode* result;
    
        match( leftparen_token ); 

        //result = NodeFactory::ParenthesizedListExpression(parseListExpression(allowIn_mode));
        result = parseListExpression(allowIn_mode);

        match( rightparen_token );
         
        if( debug ) {
            Debugger::trace( "finish parseParenthesizedListExpression" );
        }

        return result;
    }

    static void testParenthesizedListExpression() {

        std::string input[] = { 
	        "(a,b,c)",
	        "(x)" 
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|ParenthesizedListExpressionList[List[List[|GetExpression[MemberExpression[Identifier[a]]]]|GetExpression[MemberExpression[Identifier[b]]]]|GetExpression[MemberExpression[Identifier[c]]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|ParenthesizedListExpressionList[|ParenthesizedListExpressionList[|ParenthesizedListExpressionList[|GetExpression[MemberExpression[Identifier[x]]]]]]]]]]" 
        };

        testParser("ParenthesizedListExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * PrimaryExpressionOrExpressionQualifiedIdentifier	
     *     ParenthesizedListExpression {if first->isParenExpr() && lookahead(doublecolon_token)} ...
     *     ParentehsizedListExpression UnitOperator
     *     PrimaryExpression
     */

    Node* parsePrimaryExpressionOrExpressionQualifiedIdentifier() {

        if( debug ) {
            Debugger::trace( "begin parsePrimaryExpressionOrExpressionQualifiedIdentifier" );
        }

        Node* result;
    
        if( lookahead(leftparen_token) ) {
            ListNode* first = parseParenthesizedListExpression();
            if( lookahead(doublecolon_token) ) {
                match(doublecolon_token);
				if( first->list != 0 || first->item == 0 ) {
					error(syntax_error,"cannot qualify identifier with a list.");
				} else {
	                result = NodeFactory::QualifiedIdentifier(first->item,parseIdentifier());
				}
            } else {
                result = parseUnitExpressionPrime(first);
            }
        } else {
            result = parsePrimaryExpression();
        }

        if( debug ) {
            Debugger::trace( "finish parsePrimaryExpressionOrExpressionQualifiedIdentifier" );
        }

        return result;
    }

    /*
     * FunctionExpression	
     *     function FunctionSignature Block
     *     function Identifier FunctionSignature Block
     */

    Node* parseFunctionExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFunctionExpression" );
        }

        Node* result;
		Node* first;
		Node* second;
		Node* third;
    
        match(function_token); 

        if( lookahead(leftparen_token) ) {
            first = 0;
        } else {
            first = parseIdentifier();
        }

        second = parseFunctionSignature();
        third = parseBlock();
         
        result = NodeFactory::FunctionExpression(first,second,third);

        if( debug ) {
            Debugger::trace( "finish parseFunctionExpression" );
        }

        return result;
    }

    static void testFunctionExpression() {

        std::string input[] = { 
	        "(function () {})",
	        "(function f() {})" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("FunctionExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * ObjectLiteral	
     *     { }
     *     { FieldList }
     */

    Node* parseObjectLiteral() {

        if( debug ) {
            Debugger::trace( "begin parseObjectLiteral" );
        }

        Node* result;
		Node* first;
    
        // {

        match( leftbrace_token ); 

        // }

        if( lookahead( rightbrace_token ) ) {

            match( rightbrace_token );
            first = 0;

        // FieldList }

        } else {

            // Inlining parseFieldList:
            //     FieldList: LiteralField FieldListPrime

            first = parseLiteralField();
            first = parseFieldListPrime(NodeFactory::List(0,first));

            match( rightbrace_token );

        }

        result = NodeFactory::LiteralObject(first);

        if( debug ) {
            Debugger::trace( "leaving parseObjectLiteral with result = " );
        }

        return result;
    }

    static void testObjectLiteral() {

        std::string input[] = { 
	        "({a:0,'b':1,2:2,(d=e):3})",
        };

        std::string expected[] = { 
            "literalobject( list( list( list( list( 0, literalfield( identifier( a ), literalnumber( 0 ) ) ), literalfield( literalstring( b ), literalnumber( 1 ) ) ), literalfield( literalnumber( 2 ), literalnumber( 2 ) ) ), literalfield( assignmentexpression( assign_token, qualifiedidentifier( 0, d ), coersionexpression( qualifiedidentifier( 0, e ), qualifiedidentifier( 0, d ) )), literalnumber( 3 ) ) ) )",
        };

        testParser("ObjectLiteral",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * FieldListPrime	
     *     , LiteralField FieldListPrime
     *     empty
     */

    Node* parseFieldListPrime( Node* first ) {

        if( debug ) {
            Debugger::trace( "begin parseFieldListPrime" );
        }

        Node* result;

        // , LiteralField FieldListPrime

        if( lookahead( comma_token ) ) {

            Node* second;
            match( comma_token );
            second = parseLiteralField();
            result = parseFieldListPrime(NodeFactory::List(NodeFactory::List(0,first),second));

        // empty

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "leaving parseFieldListPrime with result = " );
        }
    
        return result;
    }

    /**
     * LiteralField	
     *     FieldName : AssignmentExpressionallowIn
     */

    Node* parseLiteralField() {

        if( debug ) {
            Debugger::trace( "begin parseLiteralField" );
        }

        Node* result;
		Node* first;
		Node* second;

        first = parseFieldName();
        match(colon_token);
        second = parseAssignmentExpression(allowIn_mode);

        result = NodeFactory::LiteralField(first,second);

        if( debug ) {
            Debugger::trace( "finish parseLiteralField" );
        }

        return result;
    }

    static void testLiteralField() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("LiteralField",input,expected);
    }

    /**
     * FieldName	
     *     Identifier
     *     String
     *     Number
     *     ParenthesizedExpression
     */

    Node* parseFieldName() {

        if( debug ) {
            Debugger::trace( "begin parseFieldName" );
        }

        Node* result;

        if( lookahead( stringliteral_token ) ) {
            result = NodeFactory::LiteralString( scanner->getTokenText(match( stringliteral_token )) );
        } else if( lookahead( numberliteral_token ) ) {
            result = NodeFactory::LiteralNumber(scanner->getTokenText(match(numberliteral_token)));
        } else if( lookahead( leftparen_token ) ) {
            result = parseParenthesizedExpression();
        } else {
            result = parseIdentifier();
        }

        if( debug ) {
            Debugger::trace( "finish parseFieldName" );
        }

        return result;
    }

    static void testFieldName() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("FieldName",input,expected);
    }

    /**
     * ArrayLiteral	
     *     [ ElementList ]
     */

    Node* parseArrayLiteral() {

        if( debug ) {
            Debugger::trace( "begin parseArrayLiteral" );
        }

        Node* result;
		Node* first;
    
        match( leftbracket_token ); 

        // ElementList : LiteralElement ElementListPrime (inlined)
        
        first = parseLiteralElement();

        result = NodeFactory::LiteralArray(parseElementListPrime(NodeFactory::List(0,first)));

        match( rightbracket_token ); 

        if( debug ) {
            Debugger::trace( "finish parseArrayLiteral" );
        }

        return result;
    }

    static void testArrayLiteral() {

        std::string input[] = { 
	        "[]",
	        "[1,2,3]" 
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|LiteralArray[List[|]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|LiteralArray[List[List[List[|LiteralNumber:1]|LiteralNumber:2]|LiteralNumber:3]]]]]]" 
        };

        testParser("ArrayLiteral",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * ElementListPrime	
     *     , LiteralElement ElementListPrime
     *     empty
     */

    ListNode* parseElementListPrime( ListNode* first ) {

        if( debug ) {
            Debugger::trace( "begin parseElementListPrime" );
        }

        ListNode* result;

        if( lookahead( comma_token ) ) {
            Node* second;
            match( comma_token );
            second = parseLiteralElement(); // may be empty
            result = parseElementListPrime(NodeFactory::List(first,second));
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseElementListPrime" );
        }
    
        return result;
    }

    /**
     * LiteralElement
     *     AssignmentExpression
     *     empty
     */

    Node* parseLiteralElement() {

        if( debug ) {
            Debugger::trace( "begin parseLiteralElement" );
        }

        Node* result;

        // empty

        if( lookahead( comma_token ) ||
            lookahead( rightbracket_token ) ) {

            result = 0;

        // AssignmentExpression

        } else {

            result = parseAssignmentExpression(allowIn_mode);

        }

        if( debug ) {
            Debugger::trace( "finish parseLiteralElement" );
        }
    
        return result;
    }

    static void testLiteralElement() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("LiteralElement",input,expected);
    }

    /*
	 * PostfixExpression	
	 *     [lookahead { public, private, get, set, include, exclude, Identifier }] AttributeExpression
	 *     [lookahead { null, true, false, public, Number, String, this, RegularExpression, (, [, {, function, private, Identifier, (, new, super }] FullPostfixExpression
	 *     [lookahead { new }] ShortNewExpression
     */

    Node* parsePostfixExpression() {

        if( debug ) {
            Debugger::trace( "begin parsePostfixExpression" );
        }

        Node* result;

        if( lookahead(public_token) ||
			lookahead(private_token) ||
			lookahead(get_token) ||
			lookahead(set_token) ||
			lookahead(include_token) ||
			lookahead(exclude_token) ||
			lookahead(identifier_token) ) {
			result = parseAttributeOrFullPostfixExpressionAttributeExpression();
        } else 
		if( lookahead(null_token) ||
			lookahead(true_token) ||
			lookahead(false_token) ||
			lookahead(public_token) ||
			lookahead(numberliteral_token) ||
			lookahead(stringliteral_token) ||
			lookahead(this_token) ||
			lookahead(regexpliteral_token) ||
			lookahead(leftparen_token) ||
			lookahead(leftbracket_token) ||
			lookahead(leftbrace_token) ||
			lookahead(function_token) ||
			lookahead(private_token) ||
			lookahead(identifier_token) ||
			lookahead(leftparen_token) ||
			lookahead(new_token) ||
			lookahead(super_token) ) {
			result = parseFullPostfixExpression();
		} else
		if( lookahead(new_token) ) {
			result = parseShortNewExpression();
		} else {
			error(syntax_error,"Expecting PostfixExpression");
		}

        if( debug ) {
            Debugger::trace( "finish parsePostfixExpression" );
        }
    
        return result;
    }

    static void testPostfixExpression() {

        std::string input[] = {
			"a.b",
			"a[b]",
	        "b::c.d[e](f,g)[h].i",
            "(x.class)",
            "(x++)",
			"new C(a,b,c)",
			"new C",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[Identifier[a]|Identifier[b]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|IndexedMemberExpression[Identifier[a]|List[|Identifier[b]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[IndexedMemberExpression[CallExpression[IndexedMemberExpression[MemberExpression[QualifiedIdentifier[c|Identifier[b]]|Identifier[d]]|List[|Identifier[e]]]List[List[|Identifier[f]]|Identifier[g]]]|List[|Identifier[h]]]|Identifier[i]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|List[|ClassofExpression[Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|List[|PostfixExpression[plusplus_token]|[Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|NewExpression[CallExpression[Identifier[C]List[List[List[|Identifier[a]]|Identifier[b]]|Identifier[c]]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|NewExpression[Identifier[C]]]]]]",
        };

        testParser("PostfixExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * SuperExpression	
     *     super
     *     FullSuperExpression
     */

    Node* parseSuperExpression() {

        if( debug ) {
            Debugger::trace( "begin parseSuperExpression" );
        }

        Node* result;

        if( lookahead(super_token) ) {
			match(super_token);
			if( lookahead(leftparen_token) ) {
				result = NodeFactory::SuperExpression(parseParenthesizedExpression());
			} else {
				result = NodeFactory::SuperExpression(0);
			}

        }

        if( debug ) {
            Debugger::trace( "finish parseSuperExpression" );
        }
    
        return result;
    }

    /*
     * FullSuperExpression	
     *     super ParenthesizedExpression
     */

    Node* parseFullSuperExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullSuperExpression" );
        }

        Node* result;

		match(super_token);
		if( lookahead(leftparen_token) ) {
			result = parseParenthesizedExpression();
			// Make a node for a call to a super constructor
		}

        if( debug ) {
            Debugger::trace( "finish parseFullSuperExpression" );
        }
    
        return result;
    }

    /*
     * PostfixExpressionOrSuper	
     *     PostfixExpression
     *     SuperExpression
     */

    Node* parsePostfixExpressionOrSuper() {

        if( debug ) {
            Debugger::trace( "begin parsePostfixExpressionOrSuper" );
        }

        Node* result;

		// This has an ambiguity with the lookahead is super_token.
		// The prefix can be either a SuperExpression or a SuperExpression
		// followed by { ., ++, --, ()()... }
        
		if( lookahead(super_token) ) {
			result = parseSuperExpression();
		} else {
			result = parsePostfixExpression();
		}

        if( debug ) {
            Debugger::trace( "finish parsePostfixExpressionOrSuper" );
        }
    
        return result;
    }

    /*
     * AttributeExpression	
     *     [lookahead { public, private, Identifier }] SimpleQualifiedIdentifier AttributeExpressionPrime
     * 
     * AttributeExpressionPrime	
     *     «empty»
     *     [lookahead { ., [ }] MemberOperator AttributeExpressionPrime
     *     [lookahead { ( }] Arguments AttributeExpressionPrime
     */

    Node* parseAttributeExpression() {

        if( debug ) {
            Debugger::trace( "begin parseAttributeExpression" );
        }

        Node* result;
		Node* first;

        first  = parseSimpleQualifiedIdentifier();
        result = parseAttributeExpressionPrime(first);

        if( debug ) {
            Debugger::trace( "finish parseAttributeExpression" );
        }
    
        return result;
    }

    Node* parseAttributeExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseAttributeExpressionPrime" );
        }

        Node* result;

        if( lookahead(dot_token) || lookahead(leftbracket_token) ) {
            result = parseAttributeExpressionPrime(parseMemberOperator(first));
        } else if ( lookahead(leftparen_token) ) {
            result = parseAttributeExpressionPrime(parseArguments(first));
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseAttributeExpressionPrime" );
        }
    
        return result;
    }

    static void testAttributeExpression() {

        std::string input[] = { 
	        "c.d[e](f,g)[h].i",
        };

        std::string expected[] = { 
            "",
        };

        testParser("AttributeExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /*
     * FullPostfixExpression	
     *     [lookahead { null, true, false, public, Number, String, this, RegularExpression, (, [, {, function }] PrimaryExpression FullPostfixExpressionPrime
     *     [lookahead { public, private, Identifier }] FullPostfixExpressionAttributeExpression
     *     [lookahead { ( }] ExpressionQualifiedIdentifier FullPostfixExpressionPrime
     *     [lookahead { new }] FullPostfixExpressionNewExpression
     *     [lookahead { super }] FullPostfixExpressionSuperExpression
     */

    Node* parseFullPostfixExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpression" );
        }

        Node* result;

		// There are two conficts in this function, public_token with prefix
		// PrimaryExpression and AttributeExpressionSuffix, and left_paren
		// with PrimaryExpression and ExpressionQualifiedIdentifier. The
		// latter is resolved by parsing the two expresssions together.

        if( lookahead(null_token) ||
            lookahead(true_token) ||
            lookahead(false_token) ||
            lookahead(public_token) ||
            lookahead(numberliteral_token) ||
            lookahead(stringliteral_token) ||
            lookahead(this_token) ||
            lookahead(regexpliteral_token) ||
            //lookahead(leftparen_token) ||
            lookahead(leftbracket_token) ||
            lookahead(leftbrace_token) ||
            lookahead(function_token) ) {
			Node* first;
            first  = parsePrimaryExpression();
			result = parseFullPostfixExpressionPrime(first);
        //} else if( lookahead(public_token) ||
        //    lookahead(identifier_token) ||
        //    lookahead(private_token) ) {
		//	result = parseFullPostfixExpressionAttributeExpression();
        } else if( lookahead(leftparen_token) ) {
			Node* first;
			first  = parsePrimaryExpressionOrExpressionQualifiedIdentifier();
			result = parseFullPostfixExpressionPrime(first);
        } else if( lookahead(new_token) ) {
			result = parseFullPostfixExpressionNewExpression();
        } else if( lookahead(super_token) ) {
			result = parseFullPostfixExpressionSuperExpression();
		} else {
            error("expecting FullPostfixExpression");
        }

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpression" );
        }
    
        return result;
    }

    /*
     * FullPostfixExpressionAttributeExpression	
     *     AttributeExpression FullPostfixExpressionIncrementExpressionSuffix
     *     AttributeExpression FullPostfixExpressionCoersionExpressionSuffix
     */

    Node* parseAttributeOrFullPostfixExpressionAttributeExpression() {

        if( debug ) {
            Debugger::trace( "begin parseAttributeOrFullPostfixExpressionAttributeExpression" );
        }

        Node* result;
		Node* first;

		first = parseAttributeExpression();

        if( lookahead(plusplus_token) || lookahead(minusminus_token) ) {
			result = parseFullPostfixExpressionIncrementExpressionSuffix(first);
		} else if( lookahead(ampersand_token) ) {
			result = parseFullPostfixExpressionCoersionExpressionSuffix(first);
        } else {
			result = first;
		}

        if( debug ) {
            Debugger::trace( "finish parseAttributeOrFullPostfixExpressionAttributeExpression" );
        }
    
        return result;
    }

    /*
     * FullPostfixExpressionNewExpression	
     *     FullNewExpression
     *     ShortNewExpression FullPostfixExpressionIncrementExpressionSuffix
     *     ShortNewExpression FullPostfixExpressionCoersionExpressionSuffix
     */

    Node* parseFullPostfixExpressionNewExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpressionNewExpression" );
        }

        Node* result;
		NewExpressionNode* first;

		first = parseFullOrShortNewExpression();

        if( first->isFullNewExpression() ) {
			result = first;
        } else {
			if( lookahead(plusplus_token) || lookahead(minusminus_token) ) {
				result = parseFullPostfixExpressionIncrementExpressionSuffix(first);
			} else if( lookahead(ampersand_token) ) {
				result = parseFullPostfixExpressionCoersionExpressionSuffix(first);
			} else {
				result = first;
			}
		}

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpressionNewExpression" );
        }
    
        return result;
    }

    /*
     * FullPostfixExpressionSuperExpression	
     *     SuperExpression FullPostfixExpressionIncrementExpressionSuffix
     *     SuperExpression DotOperator FullPostfixExpressionPrime
     *     FullSuperExpression Arguments FullPostfixExpressionPrime
     */

    Node* parseFullPostfixExpressionSuperExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpressionSuperExpression" );
        }

        Node* result;
		Node* first;

		first = parseSuperExpression();

		if( lookahead(plusplus_token) || lookahead(minusminus_token) ) {
			result = parseFullPostfixExpressionIncrementExpressionSuffix(first);
		} else if( lookahead(dot_token) ) {
			first  = parseDotOperator(first);
			result = parseFullPostfixExpressionPrime(first);
		} else if( first->isFullSuperExpression() ) {
			first  = parseArguments(first);
			result = parseFullPostfixExpressionPrime(first);
        } else {
			error("Expecting FullPostfixExpressionSuperExpression");
		}

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpressionSuperExpression" );
        }
    
        return result;
    }

	/*
     * FullPostfixExpressionIncrementExpressionSuffix	
     *     [no line break] ++ FullPostfixExpressionPrime
     *     [no line break] -- FullPostfixExpressionPrime
     */

    Node* parseFullPostfixExpressionIncrementExpressionSuffix(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpressionIncrementExpressionSuffix" );
        }

        Node* result;

        if( lookahead(plusplus_token) ) {
            if( newline() ) {
                error("Newline not allowed before postfix increment operator.");
            }
            first  = NodeFactory::PostfixExpression(match(plusplus_token),first);
			result = parseFullPostfixExpressionPrime(first);
        } else if( lookahead(minusminus_token) ) {
            if( newline() ) {
                error("Newline not allowed before postfix increment operator.");
            }
            first  = NodeFactory::PostfixExpression(match(minusminus_token),first);
			result = parseFullPostfixExpressionPrime(first);
        } else {
			error("Expecting increment or decrement operator.");
        }

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpressionIncrementExpressionSuffix" );
        }
    
        return result;
    }

	/*
     * FullPostfixExpressionCoersionExpressionSuffix	
     *     @ QualifiedIdentifier FullPostfixExpressionPrime
     *     @ ParenthesizedExpression FullPostfixExpressionPrime
     */

    Node* parseFullPostfixExpressionCoersionExpressionSuffix(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpressionCoersionExpressionSuffix" );
        }

        Node* result;

		match(ampersand_token);

        if( lookahead(leftparen_token) ) {
            first  = NodeFactory::CoersionExpression(first,parseParenthesizedExpression());
			result = parseFullPostfixExpressionPrime(first);
        } else {
            first  = NodeFactory::CoersionExpression(first,parseQualifiedIdentifier());
			result = parseFullPostfixExpressionPrime(first);
		}

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpressionCoersionExpressionSuffix" );
        }
    
        return result;
    }

    /**
     * FullPostfixExpressionPrime	
     *     MemberOperator FullPostfixExpressionPrime
     *     Arguments FullPostfixExpressionPrime
     *     FullPostfixExpressionIncrementExpressionSuffix
     *     FullPostfixExpressionCoersionExpressionSuffix
     *     «empty»
     */

    Node* parseFullPostfixExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseFullPostfixExpressionPrime" );
        }

        Node* result;

        if( lookahead(dot_token) || lookahead(leftbracket_token) ) {
            first  = parseMemberOperator(first);
			result = parseFullPostfixExpressionPrime(first);
        } else if( lookahead(leftparen_token) ) {
            first  = parseArguments(first);
			result = parseFullPostfixExpressionPrime(first);
        } else if( lookahead(plusplus_token) || lookahead(minusminus_token) ) {
			result = parseFullPostfixExpressionIncrementExpressionSuffix(first);
        } else if( lookahead(ampersand_token) ){
			result = parseFullPostfixExpressionCoersionExpressionSuffix(first);
        } else {
			result = first;
		}

        if( debug ) {
            Debugger::trace( "finish parseFullPostfixExpressionPrime" );
        }
    
        return result;
    }

    /*
     * FullOrShortNewExpression	
     *     new FullSuperExpression Arguments
     *     new SuperExpression
     *     new FullNewSubexpression Arguments
     *     new FullNewSubexpression
     *     new ShortNewExpression
     */

    NewExpressionNode* parseFullOrShortNewExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullOrShortNewExpression" );
        }

        NewExpressionNode* result;
		Node* first;

        match(new_token);

        if( lookahead(super_token) ) {
			first  = parseFullSuperExpression();
			// disambiguate SuperExpression and FullSuperExpression();
        } else if( lookahead(new_token) ) {
			first = parseShortNewExpression();
		} else {
            first = parseFullNewSubexpression();
			if( lookahead(leftparen_token) ) {
				first = parseArguments(first);
			} else {
				first = first;
			}
        }

        result = NodeFactory::NewExpression(first);

        if( debug ) {
            Debugger::trace( "finish parseFullOrShortNewExpression" );
        }
    
        return result;
    }

    /*
     * FullNewExpression	
     *     new FullSuperExpression Arguments
     *     new FullNewSubexpression Arguments
     */

    NewExpressionNode* parseFullNewExpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullNewExpression" );
        }

        NewExpressionNode* result;
		Node* first;

        match(new_token);

        if( lookahead(super_token) ) {
			first  = parseFullSuperExpression();
        } else {
            first  = parseFullNewSubexpression();
        }

        result = NodeFactory::NewExpression(parseArguments(first));

        if( debug ) {
            Debugger::trace( "finish parseFullNewExpression" );
        }
    
        return result;
    }

    /*
     * FullNewSubexpression	
     *     PrimaryExpression FullNewSubexpressionPrime
     *     QualifiedIdentifier FullNewSubexpressionPrime
     *     FullNewExpression FullNewSubexpressionPrime
     *     SuperExpression DotOperator FullNewSubexpressionPrime
     */

    Node* parseFullNewSubexpression() {

        if( debug ) {
            Debugger::trace( "begin parseFullNewSubexpression" );
        }

		Node* result;

		if( lookahead(new_token) ) {
			Node*  first;
			first  = parseFullNewExpression();
			result = parseFullNewSubexpressionPrime(first);
		} else if( lookahead(super_token) ) {
			Node*  first;
			first  = parseSuperExpression();
			first  = parseDotOperator(first);
			result = parseFullNewSubexpressionPrime(first);
        } else if( lookahead(leftparen_token) ) {
			Node*  first;
			first  = parsePrimaryExpressionOrExpressionQualifiedIdentifier();
			result = parseFullNewSubexpressionPrime(first);
		} else if( lookahead(public_token) || lookahead(private_token) ||
			lookahead(get_token) || lookahead(set_token) || 
			lookahead(include_token) || lookahead(exclude_token) || 
			lookahead(identifier_token) ) {
			Node*  first;
			first  = parseQualifiedIdentifier();
			result = parseFullNewSubexpressionPrime(first);
		} else {
			Node*  first;
			first  = parsePrimaryExpression();
			result = parseFullNewSubexpressionPrime(first);
		}

        if( debug ) {
            Debugger::trace( "finish parseFullNewSubexpression" );
        }
    
        return result;
    }

    /*
     * FullNewSubexpressionPrime	
     *     MemberOperator FullNewSubexpressionPrime
     *     «empty»
     */

    Node* parseFullNewSubexpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseFullNewSubexpressionPrime" );
        }

		Node* result;

		if( lookahead(dot_token) || lookahead(leftbracket_token) ) {
			first  = parseMemberOperator(first);
			result = parseFullNewSubexpressionPrime(first);
		} else {
			result = first;
		}

        if( debug ) {
            Debugger::trace( "finish parseFullNewSubexpressionPrime" );
        }
    
        return result;
    }

    /*
     * ShortNewExpression	
     *     new ShortNewSubexpression
     *     new SuperExpression
     */

    Node* parseShortNewExpression() {

        if( debug ) {
            Debugger::trace( "begin parseShortNewExpression" );
        }

        Node* result;
		Node* first;

		match(new_token);

        if( lookahead(super_token) ) {
			first = parseSuperExpression();
        } else {
			first = parseShortNewSubexpression();
		}

		result = NodeFactory::NewExpression(first);

        if( debug ) {
            Debugger::trace( "finish parseShortNewExpression" );
        }
    
        return result;
    }

    /*
     * ShortNewSubexpression	
     *     FullNewSubexpression
     *     ShortNewExpression
     */

    Node* parseShortNewSubexpression() {

        if( debug ) {
            Debugger::trace( "begin parseShortNewSubexpression" );
        }

        Node* result;

        result = parseFullNewSubexpression();
		// Implement branch into ShortNewExpression

        if( debug ) {
            Debugger::trace( "finish parseShortNewSubexpression" );
        }
    
        return result;
    }

    /* 
     * MemberOperator	
     *     . class
     *     . ParenthesizedExpression
     *     . QualifiedIdentifier
     *     Brackets
     */

    Node* parseMemberOperator(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseMemberOperator" );
        }

		Node* result;

		if( lookahead(dot_token) ) {
			match(dot_token);
			if( lookahead(class_token) ) {
				match(class_token);
				result = NodeFactory::ClassofExpression(first);
			} else if( lookahead(leftparen_token) ) {
				match(leftparen_token);
				match(rightparen_token);
				result = 0; // Not implemented.
			} else {
				result = NodeFactory::MemberExpression(first,parseQualifiedIdentifier());
			}
		} else {
			result = parseBrackets(first);
		}

        if( debug ) {
            Debugger::trace( "finish parseMemberOperator" );
        }
    
        return result;
    }

    /* 
     * DotOperator	
     *     . QualifiedIdentifier
     *     Brackets
     */

    Node* parseDotOperator(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseDotOperator" );
        }

		Node* result;

		if( lookahead(dot_token) ) {
			result = NodeFactory::MemberExpression(first,parseQualifiedIdentifier());
		} else {
			result = parseBrackets(first);
		}

        if( debug ) {
            Debugger::trace( "finish parseDotOperator" );
        }
    
        return result;
    }

    /* 
     * Brackets	
     *     [  ]
     *     [ ListExpressionallowIn ]
     *     [ NamedArgumentList ]
     */

    Node* parseBrackets(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseBrackets" );
        }

		Node* result;
		ListNode* second;

		match(leftbracket_token);

		if( lookahead(rightbracket_token) ) {
			second = 0;
		} else {
			second = parseListExpression(allowIn_mode);
			if( lookahead(colon_token) ) {
				second = parseNamedArgumentListPrimePrime(second);
			}
		}

        if( debug ) {
            Debugger::trace( "finish parseBrackets" );
        }

		match(rightbracket_token);

		result = NodeFactory::IndexedMemberExpression(first,second);
    
        return result;
    }

    static void testBrackets() {

        std::string input[] = { 
	        "",
        };

        std::string expected[] = { 
            "",
        };

        testParser("Brackets",input,expected);
    }

    /*
     * Arguments	
     *     ( )
     *     ( ListExpressionallowIn )
     *     ( NamedArgumentList )
     */

    Node* parseArguments(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseArguments" );
        }

		Node* result;
		ListNode* second;

		match(leftparen_token);

		if( lookahead(rightparen_token) ){
			second = 0;
		} else {
			second = parseListExpression(allowIn_mode);
			if( lookahead(colon_token) ) {
				second = parseNamedArgumentListPrimePrime(second);
			}
		}

		match(rightparen_token);

		result = NodeFactory::CallExpression(first,second);

        if( debug ) {
            Debugger::trace( "finish parseArguments" );
        }
    
        return result;
    }

    static void testArguments() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Arguments",input,expected);
    }

    /*
     * NamedArgumentListPrime	
     *     «empty»
     *     , FieldName NamedArgumentListPrimePrime
     */

    ListNode* parseNamedArgumentListPrime(ListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseNamedArgumentListPrime" );
        }

        ListNode* result;

        if( lookahead(comma_token) ) {
            match(comma_token);
            result = parseNamedArgumentListPrimePrime(NodeFactory::List(first,parseFieldName()));
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseNamedArgumentListPrime" );
        }

        return result;
    }

    /*
     * NamedArgumentListPrimePrime	
     *     : AssignmentExpressionallowIn NamedArgumentListPrime
     */

    ListNode* parseNamedArgumentListPrimePrime(ListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseNamedArgumentListPrimePrime" );
        }

        ListNode* result;

		match(colon_token);

		// Tranform the last item in the list from a fieldname to a literal field.

		first->item = NodeFactory::LiteralField(first->item,parseAssignmentExpression(allowIn_mode));
		result = parseNamedArgumentListPrime(first);

        if( debug ) {
            Debugger::trace( "finish parseNamedArgumentListPrimePrime" );
        }

        return result;
    }
    /**
     * UnaryExpression	
     *     PostfixExpression
     *     delete PostfixExpression
     *     void UnaryExpression
     *     typeof UnaryExpression
     *     ++ PostfixExpression
     *     -- PostfixExpression
     *     + UnaryExpression
     *     - UnaryExpression
     *     ~ UnaryExpression
     *     ! UnaryExpression
     */

    Node* parseUnaryExpression() {

        if( debug ) {
            Debugger::trace( "begin parseUnaryExpression" );
        }

        Node* result;

        if( lookahead(delete_token) ) {
            match(delete_token);
            result = NodeFactory::UnaryExpression(delete_token,parsePostfixExpression());
        } else if( lookahead(void_token) ) {
            match(void_token);
            result = NodeFactory::UnaryExpression(void_token,parseUnaryExpression());
        } else if( lookahead(typeof_token) ) {
            match(typeof_token);
            result = NodeFactory::UnaryExpression(typeof_token,parseUnaryExpression());
        } else if( lookahead(plusplus_token) ) {
            match(plusplus_token);
            result = NodeFactory::UnaryExpression(plusplus_token,parseUnaryExpression());
        } else if( lookahead(minusminus_token) ) {
            match(minusminus_token);
            result = NodeFactory::UnaryExpression(minusminus_token,parseUnaryExpression());
        } else if( lookahead(plus_token) ) {
            match(plus_token);
            result = NodeFactory::UnaryExpression(plus_token,parseUnaryExpression());
        } else if( lookahead(minus_token) ) {
            match(minus_token);
            result = NodeFactory::UnaryExpression(minus_token,parseUnaryExpression());
        } else if( lookahead(bitwisenot_token) ) {
            match(bitwisenot_token);
            result = NodeFactory::UnaryExpression(bitwisenot_token,parseUnaryExpression());
        } else if( lookahead(not_token) ) {
            match(not_token);
            result = NodeFactory::UnaryExpression(not_token,parseUnaryExpression());
        } else {
            result = parsePostfixExpression();
        }
        
        if( debug ) {
            Debugger::trace( "finish parseUnaryExpression" );
        }

        return result;
    }

    static void testUnaryExpression() {

        std::string input[] = { 
	        "delete x",
	        "typeof x",
            "++x",
            "--x",
            "+x",
            "-x",
            "~x",
            "~~x",
            "!x",
            "typeof !x",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[delete_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[typeof_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[plusplus_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[minusminus_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[plus_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[minus_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[bitwisenot_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[bitwisenot_token]|[UnaryExpression[bitwisenot_token]|[MemberExpression[|Identifier[x]]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[not_token]|[MemberExpression[|Identifier[x]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|UnaryExpression[typeof_token]|[UnaryExpression[not_token]|[MemberExpression[|Identifier[x]]]]]]]]",
        };

        testParser("UnaryExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * MultiplicativeExpression	
     *     UnaryExpression
     *     MultiplicativeExpression * UnaryExpression
     *     MultiplicativeExpression / UnaryExpression
     *     MultiplicativeExpression % UnaryExpression
     */

    Node* parseMultiplicativeExpression() {

        if( debug ) {
            Debugger::trace( "begin parseMultiplicativeExpression" );
        }

        Node* result;
		Node* first;

        scanner->enterSlashDivContext();

        try {
            first = parseUnaryExpression();
            result = parseMultiplicativeExpressionPrime(first);
            scanner->exitSlashDivContext();
        } catch(...) {
			Debugger::trace("exception caught in parseMultiplicativeExpression");
            scanner->exitSlashDivContext();
			throw;
        }

        
        if( debug ) {
            Debugger::trace( "finish parseMultiplicativeExpression" );
        }

        return result;
    }

    Node* parseMultiplicativeExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseMultiplicativeExpressionPrime" );
        }

        Node* result;

        if( lookahead(mult_token) ) {
            match(mult_token);
            first = NodeFactory::BinaryExpression(mult_token,first,parseUnaryExpression());
            result = parseMultiplicativeExpressionPrime(first);
        } else if( lookahead(div_token) ) {
            match(div_token);
            first = NodeFactory::BinaryExpression(div_token,first,parseUnaryExpression());
            result = parseMultiplicativeExpressionPrime(first);
        } else if( lookahead(modulus_token) ) {
            match(modulus_token);
            first = NodeFactory::BinaryExpression(modulus_token,first,parseUnaryExpression());
            result = parseMultiplicativeExpressionPrime(first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseMultiplicativeExpressionPrime" );
        }

        return result;
    }

    static void testMultiplicativeExpression() {

        std::string input[] = { 
	        "1*2;",
	        "x/y;",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|BinaryExpression[mult_token]|[LiteralNumber:1|LiteralNumber:2]]]]]",
            "Program[StatementList[ExpressionStatement[List[|BinaryExpression[div_token]|[MemberExpression[|Identifier[x]]|MemberExpression[|Identifier[y]]]]]]]",
        };

        testParser("MultiplicativeExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * AdditiveExpression	
     *     MultiplicativeExpression
     *     AdditiveExpression + MultiplicativeExpression
     *     AdditiveExpression - MultiplicativeExpression
     */

    Node* parseAdditiveExpression() {

        if( debug ) {
            Debugger::trace( "begin parseAdditiveExpression" );
        }

        Node* result;
		Node* first;

        first = parseMultiplicativeExpression();
        result = parseAdditiveExpressionPrime(first);
        
        if( debug ) {
            Debugger::trace( "finish parseAdditiveExpression" );
        }

        return result;
    }

    Node* parseAdditiveExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseAdditiveExpressionPrime" );
        }

        Node* result;

        if( lookahead(plus_token) ) {
            match(plus_token);
            first = NodeFactory::BinaryExpression(plus_token,first,parseMultiplicativeExpression());
            result = parseAdditiveExpressionPrime(first);
        } else if( lookahead(minus_token) ) {
            match(minus_token);
            first = NodeFactory::BinaryExpression(minus_token,first,parseMultiplicativeExpression());
            result = parseAdditiveExpressionPrime(first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseAdditiveExpressionPrime" );
        }

        return result;
    }

    static void testAdditiveExpression() {

        std::string input[] = { 
	        "1+2;",
	        "x-y;",
        };

        std::string expected[] = { 
            "",
            "",
        };

        testParser("AdditiveExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * ShiftExpression	
     *     AdditiveExpression
     *     ShiftExpression << AdditiveExpression
     *     ShiftExpression >> AdditiveExpression
     *     ShiftExpression >>> AdditiveExpression
     */

    Node* parseShiftExpression() {

        if( debug ) {
            Debugger::trace( "begin parseShiftExpression" );
        }

        Node* result;
		Node* first;

        first = parseAdditiveExpression();
        result = parseShiftExpressionPrime(first);
        
        if( debug ) {
            Debugger::trace( "finish parseShiftExpression" );
        }

        return result;
    }

    Node* parseShiftExpressionPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseShiftExpressionPrime" );
        }

        Node* result;

        if( lookahead(leftshift_token) ) {
            match(leftshift_token);
            first = NodeFactory::BinaryExpression(leftshift_token,first,parseAdditiveExpression());
            result = parseShiftExpressionPrime(first);
        } else if( lookahead(rightshift_token) ) {
            match(rightshift_token);
            first = NodeFactory::BinaryExpression(rightshift_token,first,parseAdditiveExpression());
            result = parseShiftExpressionPrime(first);
        } else if( lookahead(unsignedrightshift_token) ) {
            match(unsignedrightshift_token);
            first = NodeFactory::BinaryExpression(unsignedrightshift_token,first,parseAdditiveExpression());
            result = parseShiftExpressionPrime(first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseShiftExpressionPrime" );
        }

        return result;
    }

    /**
     * RelationalExpression[allowIn|noIn]
     *     ShiftExpression
     *     RelationalExpression[allowIn] < ShiftExpression
     *     RelationalExpression[allowIn] > ShiftExpression
     *     RelationalExpression[allowIn] <= ShiftExpression
     *     RelationalExpression[allowIn] >= ShiftExpression
     *     RelationalExpression[allowIn] instanceof ShiftExpression
     *     RelationalExpression[allowIn] in ShiftExpression
     */

    Node* parseRelationalExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseRelationalExpression" );
        }

        Node* result;
		Node* first;

        first = parseShiftExpression();
        result = parseRelationalExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseRelationalExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseRelationalExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseRelationalExpressionPrime" );
        }

        Node* result;

        if( lookahead(lessthan_token) ) {
            match(lessthan_token);
            first = NodeFactory::BinaryExpression(lessthan_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else if( lookahead(greaterthan_token) ) {
            match(greaterthan_token);
            first = NodeFactory::BinaryExpression(greaterthan_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else if( lookahead(lessthanorequals_token) ) {
            match(lessthanorequals_token);
            first = NodeFactory::BinaryExpression(lessthanorequals_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else if( lookahead(greaterthanorequals_token) ) {
            match(greaterthanorequals_token);
            first = NodeFactory::BinaryExpression(greaterthanorequals_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else if( lookahead(instanceof_token) ) {
            match(instanceof_token);
            first = NodeFactory::BinaryExpression(instanceof_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else if( mode == allowIn_mode && lookahead(in_token) ) {
            match(in_token);
            first = NodeFactory::BinaryExpression(in_token,first,parseShiftExpression());
            result = parseRelationalExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseRelationalExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * EqualityExpression	
     *     RelationalExpression
     *     EqualityExpression == RelationalExpression
     *     EqualityExpression != RelationalExpression
     *     EqualityExpression === RelationalExpression
     *     EqualityExpression !== RelationalExpression
     */

    Node* parseEqualityExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseEqualityExpression" );
        }

        Node* result;
		Node* first;

        first = parseRelationalExpression(mode);
        result = parseEqualityExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseEqualityExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseEqualityExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseEqualityExpressionPrime" );
        }

        Node* result;

        if( lookahead(equals_token) ) {
            match(equals_token);
            first = NodeFactory::BinaryExpression(equals_token,first,parseRelationalExpression(mode));
            result = parseEqualityExpressionPrime(mode,first);
        } else if( lookahead(notequals_token) ) {
            match(notequals_token);
            first = NodeFactory::BinaryExpression(notequals_token,first,parseRelationalExpression(mode));
            result = parseEqualityExpressionPrime(mode,first);
        } else if( lookahead(strictequals_token) ) {
            match(strictequals_token);
            first = NodeFactory::BinaryExpression(strictequals_token,first,parseRelationalExpression(mode));
            result = parseEqualityExpressionPrime(mode,first);
        } else if( lookahead(strictnotequals_token) ) {
            match(strictnotequals_token);
            first = NodeFactory::BinaryExpression(strictnotequals_token,first,parseRelationalExpression(mode));
            result = parseEqualityExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseEqualityExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * BitwiseAndExpression	
     *     EqualityExpression
     *     BitwiseAndExpression & EqualityExpression
     */

    Node* parseBitwiseAndExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseAndExpression" );
        }

        Node* result;
		Node* first;

        first = parseEqualityExpression(mode);
        result = parseBitwiseAndExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseBitwiseAndExpression" );
        }

        return result;
    }

    Node* parseBitwiseAndExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseAndExpressionPrime" );
        }

        Node* result;

        if( lookahead(bitwiseand_token) ) {
            match(bitwiseand_token);
            first = NodeFactory::BinaryExpression(bitwiseand_token,first,parseEqualityExpression(mode));
            result = parseBitwiseAndExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseBitwiseAndExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * BitwiseXorExpression	
     *     BitwiseAndExpression
     *     BitwiseXorExpression ^ BitwiseAndExpression
     */

    Node* parseBitwiseXorExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseXorExpression" );
        }

        Node* result;
		Node* first;

        first = parseBitwiseAndExpression(mode);
        result = parseBitwiseXorExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseBitwiseXorExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseBitwiseXorExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseXorExpressionPrime" );
        }

        Node* result;

        if( lookahead(bitwisexor_token) ) {
            match(bitwisexor_token);
            first = NodeFactory::BinaryExpression(bitwisexor_token,first,parseBitwiseAndExpression(mode));
            result = parseBitwiseXorExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseBitwiseXorExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * BitwiseOrExpression	
     *     BitwiseXorExpression
     *     BitwiseOrExpression | BitwiseXorExpression
     */

    Node* parseBitwiseOrExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseOrExpression" );
        }

        Node* result;
		Node* first;

        first = parseBitwiseXorExpression(mode);
        result = parseBitwiseOrExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseBitwiseOrExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseBitwiseOrExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseBitwiseOrExpressionPrime" );
        }

        Node* result;

        if( lookahead(bitwiseor_token) ) {
            match(bitwiseor_token);
            first = NodeFactory::BinaryExpression(bitwiseor_token,first,parseBitwiseXorExpression(mode));
            result = parseBitwiseOrExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseBitwiseOrExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * LogicalAndExpression	
     *     BitwiseOrExpression
     *     LogicalAndExpression && BitwiseOrExpression
     */

    Node* parseLogicalAndExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalAndExpression" );
        }

        Node* result;
		Node* first;

        first = parseBitwiseOrExpression(mode);
        result = parseLogicalAndExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseLogicalAndExpression" );
        }

        return result;
    }

    Node* parseLogicalAndExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalAndExpressionPrime" );
        }

        Node* result;

        if( lookahead(logicaland_token) ) {
            match(logicaland_token);
            first = NodeFactory::BinaryExpression(logicaland_token,first,parseBitwiseOrExpression(mode));
            result = parseLogicalAndExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseLogicalAndExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * LogicalXorExpression	
     *     LogicalAndExpression
     *     LogicalXorExpression ^^ LogicalAndExpression
     */

    Node* parseLogicalXorExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalXorExpression" );
        }

        Node* result;
		Node* first;

        first = parseLogicalAndExpression(mode);
        result = parseLogicalXorExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseLogicalXorExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseLogicalXorExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalXorExpressionPrime" );
        }

        Node* result;

        if( lookahead(logicalxor_token) ) {
            match(logicalxor_token);
            first = NodeFactory::BinaryExpression(logicalxor_token,first,parseLogicalAndExpression(mode));
            result = parseLogicalXorExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseLogicalXorExpressionPrime with ", result->toString() );
        }

        return result;
    }

    /**
     * LogicalOrExpression	
     *     LogicalXorExpression
     *     LogicalOrExpression || LogicalXorExpression
     */

    Node* parseLogicalOrExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalOrExpression" );
        }

        Node* result;
		Node* first;

        first = parseLogicalXorExpression(mode);
        result = parseLogicalOrExpressionPrime(mode,first);
        
        if( debug ) {
            Debugger::trace( "finish parseLogicalOrExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseLogicalOrExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseLogicalOrExpressionPrime" );
        }

        Node* result;

        if( lookahead(logicalor_token) ) {
            match(logicalor_token);
            first = NodeFactory::BinaryExpression(logicalor_token,first,parseLogicalXorExpression(mode));
            result = parseLogicalOrExpressionPrime(mode,first);
        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseLogicalOrExpressionPrime" );
        }

        return result;
    }

    /**
     * ConditionalExpression	
     *     LogicalOrExpression
     *     LogicalOrExpression ? AssignmentExpression : AssignmentExpression
     */

    Node* parseConditionalExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseConditionalExpression" );
        }

        Node* result;
		Node* first;
        
        first = parseLogicalOrExpression(mode);

        if( lookahead(questionmark_token) ) {
            Node* second;
			Node* third;
            second = parseAssignmentExpression(mode);
            match(colon_token);
            third = parseAssignmentExpression(mode);
            result = NodeFactory::ConditionalExpression(first,second,third);
        } else {
            result = first;
        }
    
        if( debug ) {
            Debugger::trace( "finish parseConditionalExpression with ", result->toString() );
        }

        return result;
    }

    /**
     * NonAssignmentExpression	
     *     LogicalOrExpression
     *     LogicalOrExpression ? NonAssignmentExpression : NonAssignmentExpression
     */

    Node* parseNonAssignmentExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseNonAssignmentExpression" );
        }

        Node* result;
		Node* first;
        
        first = parseLogicalOrExpression(mode);

        if( lookahead(questionmark_token) ) {
            Node* second;
			Node* third;
            second = parseNonAssignmentExpression(mode);
            match(colon_token);
            third = parseNonAssignmentExpression(mode);
            result = NodeFactory::ConditionalExpression(first,second,third);
        } else {
            result = first;
        }


    
        if( debug ) {
            Debugger::trace( "finish parseNonAssignmentExpression with ", result->toString() );
        }

        return result;
    }

    /*
     * AssignmentExpression	
     *     ConditionalExpression
     *     PostfixExpression AssignmentExpressionPrime
     * 	
     * AssignmentExpressionPrime	
     *     = AssignmentExpression AssignmentExpressionPrime
     *     CompoundAssignment AssignmentExpression
     */

    Node* parseAssignmentExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseAssignmentExpression" );
        }

        Node* result;
		Node* first;

        first = parseConditionalExpression(mode);
        result = parseAssignmentExpressionPrime(mode,first);

        if( debug ) {
            Debugger::trace( "finish parseAssignmentExpression with ", result->toString() );
        }

        return result;
    }

    Node* parseAssignmentExpressionPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseAssignmentExpressionPrime" );
        }

        Node* result;
        int  second = error_token;

        if( lookahead( assign_token ) ? ((second = match( assign_token )) != error_token) :
            lookahead( multassign_token ) ? ((second = match( multassign_token )) != error_token) :
            lookahead( divassign_token ) ? ((second = match( divassign_token )) != error_token) :
            lookahead( modulusassign_token ) ? ((second = match( modulusassign_token )) != error_token) :
            lookahead( plusassign_token ) ? ((second = match( plusassign_token )) != error_token) :
            lookahead( minusassign_token ) ? ((second = match( minusassign_token )) != error_token) :
            lookahead( leftshiftassign_token ) ? ((second = match( leftshiftassign_token )) != error_token) :
            lookahead( rightshiftassign_token ) ? ((second = match( rightshiftassign_token )) != error_token) :
            lookahead( unsignedrightshiftassign_token ) ? ((second = match( unsignedrightshiftassign_token )) != error_token) :
            lookahead( bitwiseandassign_token ) ? ((second = match( bitwiseandassign_token )) != error_token) :
            lookahead( bitwisexorassign_token )  ? ((second = match( bitwisexorassign_token )) != error_token) :
            lookahead( bitwiseorassign_token )  ? ((second = match( bitwiseorassign_token )) != error_token) : 
            lookahead( logicalandassign_token ) ? ((second = match( bitwiseandassign_token )) != error_token) :
            lookahead( logicalxorassign_token )  ? ((second = match( bitwisexorassign_token )) != error_token) :
            lookahead( logicalorassign_token )  ? ((second = match( bitwiseorassign_token )) != error_token) : false ) {

            // ACTION: verify first->isPostfixExpression();

            result = NodeFactory::AssignmentExpression(first,second,
                     NodeFactory::CoersionExpression(parseAssignmentExpression(mode),
                            NodeFactory::ClassofExpression(first)));

        } else {
			result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseAssignmentExpression with ", result->toString() );
        }

        return result;
    }
    
    static void testAssignmentExpression() {

        std::string input[] = { 
	        "x=10",
	        "x=y*=20", 
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|AssignmentExpression[assign_token]|[Identifier[x]|CoersionExpression[LiteralNumber:10|ClassofExpression[Identifier[x]]]]]]]]",
            "Program[StatementList[ExpressionStatement[List[|AssignmentExpression[assign_token]|[Identifier[x]|CoersionExpression[AssignmentExpression[multassign_token]|[Identifier[y]|CoersionExpression[LiteralNumber:20|ClassofExpression[Identifier[y]]]]|ClassofExpression[Identifier[x]]]]]]]]",
        };

        testParser("AssignmentExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * ListExpression	
     *     AssignmentExpression
     *     ListExpression , AssignmentExpression
     */

    ListNode* parseListExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseListExpression" );
        }

        ListNode* result;
		Node* first;
        
        first  = parseAssignmentExpression(mode);
        result = parseListExpressionPrime(mode,NodeFactory::List(0,first));

        if( debug ) {
            Debugger::trace( "finish parseListExpression with ", result->toString() );
        }

        return result;
    }

    ListNode* parseListExpressionPrime(int mode, ListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseListExpressionPrime" );
        }

        ListNode* result;

        // , AssignmentExpression ListExpressionPrime

        if( lookahead( comma_token ) ) {

            match( comma_token );

            Node* second;
        
            second = parseAssignmentExpression(mode);
            result = parseListExpressionPrime(mode,NodeFactory::List(first,second));

        // <empty>

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseListExpression with ", result->toString() );
        }

        return result;
    }

    static void testListExpression() {

        std::string input[] = { 
	        "x,1,'a'",
	        "z" 
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[List[List[|MemberExpression[|Identifier[x]]]|LiteralNumber:1]|LiteralString:a]]]]",
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[z]]]]]]" 
        };

        testParser("ListExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * TypeExpression
     *     NonassignmentExpression
     */

    Node* parseTypeExpression(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseTypeExpression" );
        }

        Node* result = parseNonAssignmentExpression(mode);
    
        if( debug ) {
            Debugger::trace( "finish parseTypeExpression" );
        }

        return result;
    }

    static void testTypeExpression() {

        std::string input[] = { 
	        "Object",
        };

        std::string expected[] = { 
            "Program[StatementList[ExpressionStatement[List[|MemberExpression[|Identifier[Object]]]]]]",
        };

        testParser("TypeExpression",input,expected,sizeof(input)/sizeof(std::string));
    }

    /**
     * Directive	
     *     Statement
     *     LanguageDeclaration NoninsertableSemicolon
     *     PackageDefinition
     */

    Node* parseDirective(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseDirective" );
        }

        Node* result = 0;
    
        try {
		
		if( lookahead(use_token) ) {
            result = parseLanguageDeclaration();
            matchNoninsertableSemicolon(mode);
        } else if( lookahead(package_token) ) {
            result = parsePackageDefinition();
        } else {
            result = parseStatement(mode);
        }

        if( debug ) {
            Debugger::trace( "finish parseDirective with ", result->toString() );
        }

		} catch (...) {
		    // Do nothing. We are simply recovering from an error in the
			// current statement.
		}

        return result;
    }

    static void testDirective() {

        std::string input[] = { 
	        "var x;",
	        "var y;" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Directive",input,expected);
    }

    /**
     * Statement	
     *     EmptyStatement
     *     IfStatement
     *     SwitchStatement
     *     DoStatement Semicolon
     *     WhileStatement
     *     ForStatement
     *     WithStatement
     *     ContinueStatement Semicolon
     *     BreakStatement Semicolon
     *     ReturnStatement Semicolon
     *     ThrowStatement Semicolon
     *     TryStatement
     *     UseStatement Semicolon
     *     IncludeStatement Semicolon
     *     AnnotatedDefinition
     *     AnnotatedBlock
     *     ExpressionStatement Semicolon
     *     LabeledStatement
     */

    Node* parseStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseStatement" );
        }

        Node* result;
    
        if( lookahead(semicolon_token) ) {
            match(semicolon_token);
            result = NodeFactory::EmptyStatement();
        } else if( lookahead(if_token) ) {
            result = parseIfStatement(mode);
        } else if( lookahead(switch_token) ) {
            result = parseSwitchStatement();
        } else if( lookahead(do_token) ) {
            result = parseDoStatement();
            matchSemicolon(mode);
        } else if( lookahead(while_token) ) {
            result = parseWhileStatement(mode);
        } else if( lookahead(for_token) ) {
            result = parseForStatement(mode);
        } else if( lookahead(with_token) ) {
            result = parseWithStatement(mode);
        } else if( lookahead(continue_token) ) {
            result = parseContinueStatement();
            matchSemicolon(mode);
        } else if( lookahead(break_token) ) {
            result = parseBreakStatement();
            matchSemicolon(mode);
        } else if( lookahead(return_token) ) {
            result = parseReturnStatement();
            matchSemicolon(mode);
        } else if( lookahead(throw_token) ) {
            result = parseThrowStatement();
            matchSemicolon(mode);
        } else if( lookahead(try_token) ) {
            result = parseTryStatement();
        } else if( lookahead(use_token) ) {
            result = parseUseStatement();
            matchSemicolon(mode);
        } else if( lookahead(include_token) ) {
            result = parseIncludeStatement();
            matchSemicolon(mode);
        } else {
            result = parseAnnotatedOrLabeledOrExpressionStatement(mode);
        }

        if( debug ) {
            Debugger::trace( "finish parseStatement" );
        }

        return result;
    }

    static void testStatement() {

        std::string input[] = { 
	        "var x;",
	        "var y;" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Statement",input,expected);
    }

    /**
     * AnnotatedOrLabeledOrExpressionStatement	
     *     Attributes Block
     *     Attributes Definition
     *     Identifier : Statement
     *     [lookahead?{function, {, const }] ListExpressionallowIn ;
     */

    Node* parseAnnotatedOrLabeledOrExpressionStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseAnnotatedOrLabeledOrExpressionStatement" );
        }

        Node* result;

        if( lookahead(leftbrace_token) ) {
            result = NodeFactory::AnnotatedBlock(0,parseBlock());
        } else if( lookahead(import_token) || lookahead(export_token) ||
            lookahead(var_token) || lookahead(const_token) ||
            lookahead(function_token) || lookahead(class_token) ||
            lookahead(namespace_token) || lookahead(interface_token) ) {
            result = NodeFactory::AnnotatedDefinition(0,parseDefinition(mode));
        } else if( lookahead(abstract_token) || lookahead(final_token) ||
            lookahead(private_token) || lookahead(public_token) ||
            lookahead(static_token) ) {
            Node* first;
            first = parseAttributes();
            if( lookahead(leftbrace_token) ) {
                result = NodeFactory::AnnotatedBlock(first,parseBlock());
            } else {
                result = NodeFactory::AnnotatedDefinition(first,parseDefinition(mode));
            }
        } else {
            Node* first;
            first = parseListExpression(allowIn_mode);
            if( lookahead(colon_token) ) {
                // ACTION: verify that first is an IdentifierNode.
                match(colon_token);
                result = NodeFactory::LabeledStatement(first,parseStatement(mode));
            } else if( lookahead(leftbrace_token) ) {
                // ACTION: verify that first is an AttributeNode.
                result = NodeFactory::AnnotatedBlock(first,parseBlock());
            } else if( lookahead(semicolon_token) ) {
                match(semicolon_token);
                result = NodeFactory::ExpressionStatement(first);
            } else if( lookaheadSemicolon(mode) ) {
                result = NodeFactory::ExpressionStatement(first);
                matchSemicolon(mode);
            } else {
                // ACTION: verify that first is an AttributeNode.
                first = NodeFactory::AttributeList(first,parseAttributes());
                result = NodeFactory::AnnotatedDefinition(first,parseDefinition(mode));
            }
        }


        if( debug ) {
            Debugger::trace( "finish parseAnnotatedOrLabeledOrExpressionStatement with ", result->toString() );
        }

        return result;
    }

    /**
     * ExpressionStatement	
     *     [lookahead != { function,{,const }] ListExpression[allowIn]
     */

    Node* parseExpressionStatement() {

        if( debug ) {
            Debugger::trace( "begin parseExpressionStatement" );
        }

        Node* result;
    
        if( lookahead(function_token) || lookahead(leftbrace_token) ||
            lookahead(const_token) ) {
            result = 0;  // Should not get here.
            if(debug) {
				Debugger::trace("invalid lookahead for expression statement.");
			}
        } else {
            result = NodeFactory::ExpressionStatement(parseListExpression(allowIn_mode));
        }

        if( debug ) {
            Debugger::trace( "finish parseExpressionStatement" );
        }

        return result;
    }

    static void testExpressionStatement() {

        std::string input[] = { 
	        "x=1",
	        "x=y" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("ExpressionStatement",input,expected);
    }

    /**
     * AnnotatedBlock	
     *     Attributes Block
     */

    Node* parseAnnotatedBlock() {

        if( debug ) {
            Debugger::trace( "begin parseAnnotatedBlock" );
        }

        Node* result;

        result = NodeFactory::AnnotatedBlock(parseAttributes(),parseBlock());
    
        if( debug ) {
            Debugger::trace( "finish parseAnnotatedBlock" );
        }

        return result;
    }

    static void testAnnotatedBlock() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("AnnotatedBlock",input,expected);
    }

    /**
     * Block	
     *     { Directives }
     */

    Node* parseBlock() {

        if( debug ) {
            Debugger::trace( "begin parseBlock" );
        }

        Node* result;
    
        match( leftbrace_token );
        result = parseDirectives();
        match( rightbrace_token );

        if( debug ) {
            Debugger::trace( "finish parseBlock" );
        }

        return result;
    }

    static void testBlock() {

        std::string input[] = { 
	        "{}",
	        "{}" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Block",input,expected);
    }

    /**
     * Directives	
     *     «empty»
     *     DirectivesPrefix Directive[abbrev]
     * 
     * TypeStatementsPrefix	
     *     Directive[full] DirectivePrefixPrime
     * 	
     * DirectivePrefixPrime	
     *     Directive[full] DirectivePrefixPrime
     *     empty
     */

    StatementListNode* parseDirectives() {

        if( debug ) {
            Debugger::trace( "begin parseDirectives" );
        }

        StatementListNode* result;

        if( lookahead(rightbrace_token) || lookahead(eos_token) ) {

            result = 0; 

        } else {
            
            StatementListNode* first;
            first = parseDirectivesPrefix();
            if( !(lookahead(rightbrace_token) || lookahead(eos_token)) ) {
                Node* second;
                second = parseDirective(abbrev_mode);
                result = NodeFactory::StatementList(first,second); 
            } else {
                result = first;
            }


        }

        if( debug ) {
            Debugger::trace( "finish parseDirectives" );
        }

        return result;
    }

    StatementListNode* parseDirectivesPrefix() {

        if( debug ) {
            Debugger::trace( "begin parseDirectivesPrefix" );
        }

        StatementListNode* result;
        Node* first;

        first = parseDirective(full_mode);
        result = parseDirectivesPrefixPrime(NodeFactory::StatementList(0,first));

        if( debug ) {
            Debugger::trace( "finish parseDirectivesPrefix" );
        }

        return result;
    }

    StatementListNode* parseDirectivesPrefixPrime(StatementListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseDirectivesPrefixPrime" );
        }

        StatementListNode* result;

        if( lookahead(rightbrace_token) || lookahead(eos_token) ) {

            result = first; 

        } else {
            
            Node* second;
            second = parseDirective(full_mode);
            result = parseDirectivesPrefixPrime(NodeFactory::StatementList(first,second));

        }

        if( debug ) {
            Debugger::trace( "finish parseDirectivesPrefixPrime" );
        }

        return result;
    }

    static void testDirectives() {

        std::string input[] = { 
	        "class C;",
	        "var x; var y;",
            "x = 1;",
            "v1 var x;",
             
        };

        std::string expected[] = { 
            "classdeclaration( qualifiedidentifier( 0, C ) )",
            "list( variabledefinition( var_token, variablebinding( typedvariable( qualifiedidentifier( 0, x ), 0 ), 0 ) ), variabledefinition( var_token, variablebinding( typedvariable( qualifiedidentifier( 0, y ), 0 ), 0 ) ) )",
            "expressionstatement( assign( assign_token, qualifiedidentifier( 0, x ), coersionexpression( literalnumber( 1 ), qualifiedidentifier( 0, x ) )) )",
            ""
        };

        testParser("Directives",input,expected);
    }

    /**
     * LabeledStatement	
     *     Identifier : Statement
     */

    Node* parseLabeledStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseLabeledStatement" );
        }

        Node* result;
		Node* first;
    
        first = parseIdentifier();
        match( colon_token );
        result = NodeFactory::LabeledStatement(first,parseStatement(mode));

        if( debug ) {
            Debugger::trace( "finish parseLabeledStatement" );
        }

        return result;
    }

    /**
     * IfStatement	
     *     if ParenthesizedExpression Statement
     *     if ParenthesizedExpression Statement else Statement
     */

    Node* parseIfStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseIfStatement" );
        }

        Node* result;
		Node* first;
		Node* second;
		Node* third=0;

        match(if_token);
        first = parseParenthesizedListExpression();
        second = parseStatement(abbrevIfElse_mode);
        if( lookahead(else_token) ) {
            match(else_token);
            third = parseStatement(mode);
        }

        result = NodeFactory::IfStatement(first,second,third);

        if( debug ) {
            Debugger::trace( "finish parseIfStatement" );
        }

        return result;
    }

    static void testIfStatement() {

        std::string input[] = { 
	        "if(x) t;",
	        "if(x) t; else e;",
	        "if(x) {a;b;c;} else {1;2;3;}",
        };

        std::string expected[] = { 
            "",
            "",
            "",
        };

        testParser("IfStatement",input,expected);
    }

    /**
     * SwitchStatement	
     *     switch ParenthesizedListExpression { CaseStatements }
     */

    Node* parseSwitchStatement() {

        if( debug ) {
            Debugger::trace( "begin parseSwitchStatement" );
        }

        Node* result;
		Node* first;
        StatementListNode* second;

        match(switch_token);
        first = parseParenthesizedListExpression();
        match(leftbrace_token);
        second = parseCaseStatements();
        match(rightbrace_token);
        
        result = NodeFactory::SwitchStatement(first,second);

        if( debug ) {
            Debugger::trace( "finish parseSwitchStatement" );
        }

        return result;
    }

    /**
     * CaseStatement	
     *     Statement
     *     CaseLabel
     */

    Node* parseCaseStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseCaseStatement" );
        }

        Node* result;

        if( lookahead(case_token) || lookahead(default_token) ) {
            result = parseCaseLabel();
        } else {
            result = parseStatement(mode);
        }

        if( debug ) {
            Debugger::trace( "finish parseCaseStatement" );
        }

        return result;
    }

    /**
     * CaseLabel	
     *     case ListExpressionallowIn :
     *     default :
     */

    Node* parseCaseLabel() {

        if( debug ) {
            Debugger::trace( "begin parseCaseLabel" );
        }

        Node* result;

        if( lookahead(case_token) ) {
            match(case_token);
            result = NodeFactory::CaseLabel(parseListExpression(allowIn_mode));
        } else if( lookahead(default_token) ) {
            match(default_token);
            result = NodeFactory::CaseLabel(0); // 0 argument means default case.
        } else {
            error(syntax_error,"expecting CaseLabel");
        }
        match(colon_token);

        if( debug ) {
            Debugger::trace( "finish parseCaseLabel" );
        }

        return result;
    }

    /**
     * CaseStatements
     *     «empty»
     *     CaseLabel
     *     CaseLabel CaseStatementsPrefix CaseStatementabbrev
     */

    StatementListNode* parseCaseStatements() {

        if( debug ) {
            Debugger::trace( "begin parseCaseStatements" );
        }

        StatementListNode* result;

        if( !lookahead(rightbrace_token) ) {
            Node* first;
            first = parseCaseLabel();
            if( !lookahead(rightbrace_token) ) {
                result = parseCaseStatementsPrefix(NodeFactory::StatementList(0,first));
            } else {
                result = NodeFactory::StatementList(0,first);
            }

        } else {
            result = 0;
        }

        if( debug ) {
            Debugger::trace( "finish parseCaseStatements" );
        }

        return result;
    }

    /**
     * CaseStatementsPrefix	
     *     «empty»
     *     CaseStatement[full] CaseStatementsPrefix
     */

    StatementListNode* parseCaseStatementsPrefix(StatementListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseCaseStatementsPrefix" );
        }

        StatementListNode* result=0;

        if( !lookahead(rightbrace_token) ) {
            first = NodeFactory::StatementList(first,parseCaseStatement(full_mode));
            while( !lookahead(rightbrace_token) ) {
                first = NodeFactory::StatementList(first,parseCaseStatement(full_mode));
            }
            result = first;
        }


        if( debug ) {
            Debugger::trace( "finish parseCaseStatementsPrefix" );
        }

        return result;
    }

    /**
     * DoStatement	
     *     do Statement[abbrev] while ParenthesizedListExpression
     */

    Node* parseDoStatement() {

        if( debug ) {
            Debugger::trace( "begin parseWhileStatement" );
        }

        Node* result;
		Node* first;

        match(do_token);    
        first = parseStatement(abbrevDoWhile_mode);
        match(while_token);    
        result = NodeFactory::DoStatement(first,parseParenthesizedListExpression());

        if( debug ) {
            Debugger::trace( "finish parseDoStatement" );
        }

        return result;
    }

    /**
     * WhileStatement	
     *     while ParenthesizedListExpression Statement
     */

    Node* parseWhileStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseWhileStatement" );
        }

        Node* result;

        match(while_token);    
        result = NodeFactory::WhileStatement(parseParenthesizedListExpression(),parseStatement(mode));

        if( debug ) {
            Debugger::trace( "finish parseWhileStatement" );
        }

        return result;
    }

    /**
     * ForStatement	
     *     for (  ; OptionalExpression ; OptionalExpression ) Statement
     *     for ( Attributes VariableDefinitionKind VariableBinding[noIn] in ListExpression[allowIn] ) Statement
     *     for ( Attributes VariableDefinitionKind VariableBindingList[noIn] ; OptionalExpression ; OptionalExpression ) Statement
     *     for ( AssignmentExpression[noIn] ListExpressionPrime[noIn] ; OptionalExpression ; OptionalExpression ) Statement
     *     for ( PostfixExpression in ListExpression[allowIn] ) Statement
     */

    Node* parseForStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseForStatement" );
        }

        Node* result;

        match(for_token);
        match(leftparen_token);

        if( lookahead(semicolon_token) ) {
            match(semicolon_token);
            Node* first;
			Node* second;
			Node* third;
            first = 0;
            if( lookahead(semicolon_token) ) {
                second = 0;
            } else {
                second = parseListExpression(allowIn_mode);
            }
            match(semicolon_token);
            if( lookahead(rightparen_token) ) {
                third = 0;
            } else {
                third = parseListExpression(mode);
            }
            match(rightparen_token);
            result = NodeFactory::ForStatement(first,second,third,parseStatement(mode));
        } else if( lookahead(abstract_token) || lookahead(final_token) ||
            lookahead(private_token) || lookahead(public_token) ||
            lookahead(static_token) ||
            lookahead(const_token) || lookahead(var_token) ) {
            Node* first;
            first = parseAttributes();

            // [Copied below]

            if( lookahead(const_token) || lookahead(var_token) ) {
                int second;
                Node* third;
                second = lookahead(const_token) ? match(const_token) :
                     lookahead(var_token) ? match(var_token) : var_token;
                third = parseVariableBinding(noIn_mode);
                if( lookahead(in_token) ) {
                    Node* fourth;
                    match(in_token);
                    third = NodeFactory::VariableDefinition(second,third);
                    third = NodeFactory::AnnotatedDefinition(first,third);
                    fourth = parseListExpression(allowIn_mode);
                    match(rightparen_token);
                    result = NodeFactory::ForInStatement(third,fourth,parseStatement(mode));
                } else {
                    Node* fourth;
					Node* fifth;
                    third = parseVariableBindingListPrime(noIn_mode,third);
                    third = NodeFactory::VariableDefinition(second,third);
                    third = NodeFactory::AnnotatedDefinition(first,third);
                    match(semicolon_token);
                    if( lookahead(semicolon_token) ) {
                        match(semicolon_token);
                        fourth = 0;
                    } else {
                        fourth = parseListExpression(allowIn_mode);
                    }
                    match(semicolon_token);
                    if( lookahead(rightparen_token) ) {
                        fifth = 0;
                    } else {
                        fifth = parseListExpression(allowIn_mode);
                    }
                    match(rightparen_token);
                    result = NodeFactory::ForStatement(third,fourth,fifth,parseStatement(mode));
                }
            } else {
				error(syntax_error,"expecting const or var token");
            }
        } else {
            Node* first;
            first = parseAssignmentExpression(noIn_mode); // noIn

            // for ( AssignmentExpression[noIn] ListExpressionPrime[noIn] ; OptionalExpression ; OptionalExpression ) Statement
            if( lookahead(semicolon_token) ||
                lookahead(comma_token) ) {

                Node* second;
				Node* third;
                
                // Either way we should continue parsing as a ListExpression.
                first = parseListExpressionPrime(noIn_mode,NodeFactory::List(0,first));

                match(semicolon_token);
                if( lookahead(semicolon_token) ) {
                    second = 0;
                } else {
                    second = parseListExpression(allowIn_mode);
                }
                match(semicolon_token);
                if( lookahead(rightparen_token) ) {
                    third = 0;
                } else {
                    third = parseListExpression(allowIn_mode);
                }
                match(rightparen_token);
                result = NodeFactory::ForStatement(first,second,third,parseStatement(mode));

            // for ( PostfixExpression in ListExpression[allowIn] ) Statement
            } else if( lookahead(in_token) ) {
                match(in_token);
                Node* second;
                second = parseListExpression(allowIn_mode);
                match(rightparen_token);
                result = NodeFactory::ForInStatement(first,second,parseStatement(mode));
            } else {

                // Otherwise assume that it is an AttributeExpression and continue
                // parsing as above (after seening an attribute keyword.

                // for ( Attributes VariableDefinitionKind VariableBinding[noIn] in ListExpression[allowIn] ) Statement
                // for ( Attributes VariableDefinitionKind VariableBindingList[noIn] ; OptionalExpression ; OptionalExpression ) Statement

                if( !true /* verify that first is an AttributeExpression */ ) {
					error(syntax_error,"expecting an attribute expression.");
                }

                first = NodeFactory::AttributeList(first,parseAttributes());

                // [Copied from above.]

                if( lookahead(const_token) || lookahead(var_token) ) {
                    int second;
                    Node* third;
                    second = lookahead(const_token) ? match(const_token) :
                         lookahead(var_token) ? match(var_token) : var_token;
                    third = parseVariableBinding(noIn_mode);
                    if( lookahead(in_token) ) {
                        Node* fourth;
                        match(in_token);
                        third = NodeFactory::VariableDefinition(second,third);
                        third = NodeFactory::AnnotatedDefinition(first,third);
                        fourth = parseListExpression(allowIn_mode);
                        match(rightparen_token);
                        result = NodeFactory::ForInStatement(third,fourth,parseStatement(mode));
                    } else {
                        Node* fourth;
						Node* fifth;
                        third = parseVariableBindingListPrime(noIn_mode,third);
                        third = NodeFactory::VariableDefinition(second,third);
                        third = NodeFactory::AnnotatedDefinition(first,third);
                        match(semicolon_token);
                        if( lookahead(semicolon_token) ) {
                            fourth = 0;
                        } else {
                            fourth = parseListExpression(allowIn_mode);
                        }
                        match(semicolon_token);
                        if( lookahead(rightparen_token) ) {
                            fifth = 0;
                        } else {
                            fifth = parseListExpression(allowIn_mode);
                        }
                        match(rightparen_token);
                        result = NodeFactory::ForStatement(third,fourth,fifth,parseStatement(mode));
                    }
                } else {
                    error(syntax_error,"expecting const or var token.");
                }
            }
        }

        return result;
    }

    /**
     * WithStatement	
     *     with ParenthesizedListExpression Statement
     */

    Node* parseWithStatement(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseWithStatement" );
        }

        Node* result;

        match(with_token);    
        result = NodeFactory::WithStatement(parseParenthesizedListExpression(),parseStatement(mode));

        if( debug ) {
            Debugger::trace( "finish parseWithStatement" );
        }

        return result;
    }

    /**
     * ContinueStatement	
     *     continue
     *     continue [no line break] Identifier
     */

    Node* parseContinueStatement() {

        if( debug ) {
            Debugger::trace( "begin parseContinueStatement" );
        }

        Node* result;
		Node* first=0;

        match(continue_token);    
        if( !lookaheadSemicolon(full_mode) ) {
            first = parseIdentifier();
        }

        result = NodeFactory::ContinueStatement(first);

        if( debug ) {
            Debugger::trace( "finish parseContinueStatement" );
        }

        return result;
    }

    /**
     * BreakStatement	
     *     break
     *     break [no line break] Identifier
     */

    Node* parseBreakStatement() {

        if( debug ) {
            Debugger::trace( "begin parseBreakStatement" );
        }

        Node* result;
		Node* first=0;

        match(break_token);    
        if( !lookaheadSemicolon(full_mode) ) {
            first = parseIdentifier();
        }

        result = NodeFactory::BreakStatement(first);

        if( debug ) {
            Debugger::trace( "finish parseBreakStatement" );
        }

        return result;
    }

    /**
     * ReturnStatement	
     *     return
     *     return [no line break] ExpressionallowIn
     */

    Node* parseReturnStatement() {

        if( debug ) {
            Debugger::trace( "begin parseReturnStatement" );
        }

        Node* result;
		Node* first=0;

        match(return_token);
        
        // ACTION: check for VirtualSemicolon
            
        if( !lookaheadSemicolon(full_mode) ) {
            first = parseListExpression(allowIn_mode);
        }

        result = NodeFactory::ReturnStatement(first);

        if( debug ) {
            Debugger::trace( "finish parseReturnStatement" );
        }

        return result;
    }

    static void testReturnStatement() {

        std::string input[] = { 
	        "return;",
	        "return true;" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("ReturnStatement",input,expected);
    }

    /**
     * ThrowStatement 	
     *     throw [no line break] ListExpression[allowIn]
     */

    Node* parseThrowStatement() {

        if( debug ) {
            Debugger::trace( "begin parseThrowStatement" );
        }

        Node* result;

        match(throw_token);
        lookahead(empty_token);  // ACTION: fix newline() so that we don't 
                                 // have to force a lookahead to the next 
                                 // token before it works properly.
        if( newline() ) {
            error(syntax_error,"No newline allowed in this position.");
        }
        result = NodeFactory::ThrowStatement(parseListExpression(allowIn_mode));

        if( debug ) {
            Debugger::trace( "finish parseThrowStatement" );
        }

        return result;
    }

    /**
     * TryStatement	
     *     try AnnotatedBlock CatchClauses
     *     try AnnotatedBlock FinallyClause
     *     try AnnotatedBlock CatchClauses FinallyClause
     */

    Node* parseTryStatement() {

        if( debug ) {
            Debugger::trace( "begin parseTryStatement" );
        }

        Node* result;
		Node* first;

        match(try_token);
        first = parseAnnotatedBlock();
        if( lookahead(catch_token) ) {
            StatementListNode* second = parseCatchClauses();
            if( lookahead(finally_token) ) {
                result = NodeFactory::TryStatement(first,second,parseFinallyClause());
            } else {
                result = NodeFactory::TryStatement(first,second,0);
            }
        } else if( lookahead(finally_token) ) {
            result = NodeFactory::TryStatement(first,0,parseFinallyClause());
        } else {
            error(syntax_error,"expecting catch or finally clause.");
        }

        if( debug ) {
            Debugger::trace( "finish parseTryStatement" );
        }

        return result;
    }

    /**
     * CatchClauses	
     *     CatchClause
     *     CatchClauses CatchClause
     */

    StatementListNode* parseCatchClauses() {

        if( debug ) {
            Debugger::trace( "begin parseCatchClauses" );
        }

        StatementListNode* result;

        result = NodeFactory::StatementList(0,parseCatchClause());
        while( lookahead(catch_token) ) {
            result = NodeFactory::StatementList(result,parseCatchClause());
        }

        if( debug ) {
            Debugger::trace( "finish parseCatchClauses" );
        }

        return result;
    }

    /**
     * CatchClause	
     *     catch ( Parameter ) AnnotatedBlock
     */

    Node* parseCatchClause() {

        if( debug ) {
            Debugger::trace( "begin parseCatchClause" );
        }

        Node* result;
		Node* first;

        match(catch_token);
        match(leftparen_token);
        first = parseParameter();
        match(rightparen_token);
               
        result = NodeFactory::CatchClause(first,parseAnnotatedBlock());

        if( debug ) {
            Debugger::trace( "finish parseCatchClause" );
        }

        return result;
    }

    /**
     * FinallyClause	
     *     finally AnnotatedBlock
     */

    Node* parseFinallyClause() {

        if( debug ) {
            Debugger::trace( "begin parseFinallyClause" );
        }

        Node* result;

        match(finally_token);
        
        // No line break.

        result = NodeFactory::FinallyClause(parseAnnotatedBlock());

        if( debug ) {
            Debugger::trace( "finish parseFinallyClause" );
        }

        return result;
    }

    /**
     * IncludeStatement	
     *     include [no line break] String
     */

    Node* parseIncludeStatement() {

        if( debug ) {
            Debugger::trace( "begin parseIncludeStatement" );
        }

        Node* result;
		Node* first;

        match(include_token);
        
        // No line break.

        first = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
        result = NodeFactory::IncludeStatement(first);

        if( debug ) {
            Debugger::trace( "finish parseIncludeStatement" );
        }

        return result;
    }

    static void testIncludeStatement() {

        std::string input[] = { 
	        "",
        };

        std::string expected[] = { 
            "",
        };

        testParser("IncludeStatement",input,expected);
    }

    /**
     * UseStatement	
     *     use [no line break] namespace NonAssignmentExpressionList
     */

    Node* parseUseStatement() {

        if( debug ) {
            Debugger::trace( "begin parseUseStatement" );
        }

        Node* result;

        match(use_token);
        match(namespace_token);
        result = NodeFactory::UseStatement(parseNonAssignmentExpression(allowIn_mode));

        if( debug ) {
            Debugger::trace( "finish parseUseStatement" );
        }

        return result;
    }

    static void testUseStatement() {

        std::string input[] = { 
	        "use namespace P1",
	        "use namespace P1.P2.P3", 
	        "use namespace P1.P2.P3,P1,P4", 
        };

        std::string expected[] = { 
            "usestatement( qualifiedidentifier( 0, P1 ) )",
            "usestatement( memberexpression( memberexpression( qualifiedidentifier( 0, P1 ), qualifiedidentifier( 0, P2 ) ), qualifiedidentifier( 0, P3 ) ) )",
            "usestatement( list( list( memberexpression( memberexpression( qualifiedidentifier( 0, P1 ), qualifiedidentifier( 0, P2 ) ), qualifiedidentifier( 0, P3 ) ), qualifiedidentifier( 0, P1 ) ), qualifiedidentifier( 0, P4 ) ) )", 
        };

        testParser("UseStatement",input,expected);
    }

    /**
     * NonAssignmentExpressionList	
     *     NonAssignmentExpression[allowIn] NonAssignmentExpressionListPrime
     *     
     * NonAssignmentExpressionListPrime	
     *     , NonAssignmentExpression[allowIn] NonAssignmentExpressionListPrime
     *     empty
     */

    Node* parseNonAssignmentExpressionList() {

        if( debug ) {
            Debugger::trace( "begin parseNonAssignmentExpressionList" );
        }

        Node* result;
		Node* first;
        
        first = parseNonAssignmentExpression(allowIn_mode);
        result = parseNonAssignmentExpressionListPrime(first);

        if( debug ) {
            Debugger::trace( "finish parseNonAssignmentExpressionList" );
        }

        return result;
    }

    Node* parseNonAssignmentExpressionListPrime( Node* first ) {

        if( debug ) {
            Debugger::trace( "begin parseNonAssignmentExpressionListPrime" );
        }

        Node* result;

        if( lookahead( comma_token ) ) {

            match( comma_token );

            Node* second;
        
            second = parseNonAssignmentExpression(allowIn_mode);
            result = parseNonAssignmentExpressionListPrime(NodeFactory::List(NodeFactory::List(0,first),second));

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseNonAssignmentExpressionListPrime'" );
        }

        return result;
    }

    static void testNonAssignmentExpressionList() {

        std::string input[] = { 
	        "x,1,2",
	        "z" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("NonAssignmentExpressionList",input,expected);
    }

    /**
     * AnnotatedDefinition	
     *     Attributes Definition
     */

    AnnotatedDefinitionNode* parseAnnotatedDefinition(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseAnnotatedDefinition" );
        }

        AnnotatedDefinitionNode* result;
        Node* first;

        first = parseAttributes();
        result = NodeFactory::AnnotatedDefinition(first,parseDefinition(mode));
            
        if( debug ) {
            Debugger::trace( "finish parseAnnotatedDefinition" );
        }

        return result;
    }

    static void testAnnotatedDefinition() {

        std::string input[] = { 
	        "private var x",
	        "v2 const y",
	        "constructor function c() {}", 
        };

        std::string expected[] = { 
            "",
            "",
            "",
        };

        testParser("AnnotatedDefinition",input,expected);
    }

    /**
     * Attributes	
     *     «empty»
     *     Attribute [no line break] Attributes
     */

    Node* parseAttributes() {

        if( debug ) {
            Debugger::trace( "begin parseAttributes" );
        }

        Node* result;

        if( lookahead(leftbrace_token) || 
            lookahead(export_token) || 
            lookahead(import_token) || 
            lookahead(const_token) ||
            lookahead(var_token) ||
            lookahead(function_token) || 
            lookahead(class_token) ||
            lookahead(interface_token) || 
            lookahead(namespace_token) ) {

            result = 0;

        } else {

            Node* first;

            first = parseAttribute();

            if( newline() ) {
                error(syntax_error,"No line break in attributes list.");
            }

			if( lookahead(rightbrace_token) || lookahead(eos_token) ) {
				result = first;
			} else {
				Node* second;
				second = parseAttributes();
				result = NodeFactory::AttributeList(first,second);
			}

        }

        if( debug ) {
            Debugger::trace( "finish parseAttributes" );
        }

        return result;
    }

    static void testAttributes() {

        std::string input[] = { 
	        "private export",
	        "private public export" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Attributes",input,expected);
    }

    /**
     * Attribute	
     *     AttributeExpression
     *     abstract
     *     final
     *     private
     *     public
     *     static
     */

    Node* parseAttribute() {

        if( debug ) {
            Debugger::trace( "begin parseAttribute" );
        }

        Node* result;
    
        if( lookahead(abstract_token) ) {
            match(abstract_token);
            result = NodeFactory::Identifier("abstract");
        } else if( lookahead(final_token) ) {
            match(final_token);
            result = NodeFactory::Identifier("final");
        } else if ( lookahead(private_token) ) {
            match(private_token);
            result = NodeFactory::Identifier("private");
            if( lookahead(doublecolon_token) ) {
                match(doublecolon_token);
                result = NodeFactory::QualifiedIdentifier(result,parseIdentifier());
			}
        } else if ( lookahead(public_token) ) {
            match(public_token);
            result = NodeFactory::Identifier("public");
            if( lookahead(doublecolon_token) ) {
                match(doublecolon_token);
                result = NodeFactory::QualifiedIdentifier(result,parseIdentifier());
			}
        } else if ( lookahead(static_token) ) {
            match(static_token);
            result = NodeFactory::Identifier("static");
        } else if ( lookahead(true_token) ) {
            match(true_token);
            result = NodeFactory::LiteralBoolean(true);
        } else if ( lookahead(false_token) ) {
            match(false_token);
            result = NodeFactory::LiteralBoolean(false);
        } else {
            result = parseAttributeExpression();
        }

        if( debug ) {
            Debugger::trace( "finish parseAttribute" );
        }

        return result;
    }

    static void testAttribute() {

        std::string input[] = { 
	        "final",
	        "package",
	        "private",
	        "public",
	        "static",
	        "volatile",
	        "pascal" 
        };

        std::string expected[] = { 
            "",
            "",
            "",
            "",
            "",
            "",
            "" 
        };

        testParser("Attribute",input,expected);
    }

    /**
     * Definition	
     *     ImportDefinition Semicolon
     *     ExportDefinition Semicolon
     *     VariableDefinition Semicolon
     *     FunctionDefinition
     *     ClassDefinition
     *     NamespaceDefinition Semicolon
     *     InterfaceDefinition
     */

    Node* parseDefinition(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseDefinition" );
        }

        Node* result = 0;
    
        if( lookahead(import_token) ) {
            result = parseImportDefinition();
            matchSemicolon(mode);
        } else if( lookahead(export_token) ) {
            result = parseExportDefinition();
            matchSemicolon(mode);
        } else if( lookahead(const_token) || lookahead(var_token) ) {
            result = parseVariableDefinition();
            matchSemicolon(mode);
        } else if( lookahead(function_token) ) {
            result = parseFunctionDefinition(mode);
        } else if( lookahead(class_token) ) {
            result = parseClassDefinition(mode);
        //} else if( lookahead(interface_token) ) {
        //    result = parseInterfaceDefinition(mode);
        } else if( lookahead(namespace_token) ) {
            result = parseNamespaceDefinition();
            matchSemicolon(mode);
        }

        if( debug ) {
            Debugger::trace( "finish parseDefinition" );
        }

        return result;
    }

    static void testDefinition() {

        std::string input[] = { 
	        "var x;",
	        "class C;" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("Directive",input,expected);
    }

    /**
     * ImportDefinition	
     *     import ImportBinding
     *     import ImportBinding : NonassignmentExpressionList
     */

    Node* parseImportDefinition() {

        if( debug ) {
            Debugger::trace( "begin parseImportDefinition" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        match(import_token);
        first = parseImportBinding();
        if( lookahead(colon_token) ) {
            second = parseNonAssignmentExpressionList();
        }

        result = NodeFactory::ImportDefinition(first,second);
    
        if( debug ) {
            Debugger::trace( "finish parseImportDefinition" );
        }

        return result;
    }

    static void testImportDefinition() {

        std::string input[] = { 
	        "import x",
	        "import f=g",
            "import f=g,h" 
        };

        std::string expected[] = { 
            "",
            "",
            "",
        };

        testParser("ImportDefinition",input,expected);
    }

    /**
     * ImportBinding	
     *     Identifier PackageNamePrime
     *     Identifier = ImportItem
     *     String
     */

    Node* parseImportBinding() {

        if( debug ) {
            Debugger::trace( "begin parseImportBinding" );
        }

        Node* result;
		Node* first;
        
        if( lookahead(stringliteral_token) ) {
            result = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
        } else {
            first = parseIdentifier();
            if( lookahead(dot_token) ) {
                result = parsePackageNamePrime(first);
            } else if( lookahead(assign_token) ) {
                match(assign_token);
                result = NodeFactory::ImportBinding(first,parseImportItem());
            } else {
                error(syntax_error,"expecting dot or assignment after identifier.");
            }
        }

        if( debug ) {
            Debugger::trace( "finish parseImportBinding" );
        }

        return result;
    }

    /**
     * ImportItem	
     *     String
     *     PackageName
     */

    Node* parseImportItem() {

        if( debug ) {
            Debugger::trace( "begin parseImportItem" );
        }

        Node* result;
        
        if( lookahead(stringliteral_token) ) {
            result = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
        } else {
            result = parsePackageName();
        }

        if( debug ) {
            Debugger::trace( "finish parseImportItem" );
        }

        return result;
    }

    /**
     * ExportDefinition	
     *     export ExportBindingList
     */

    Node* parseExportDefinition() {

        if( debug ) {
            Debugger::trace( "begin parseExportDefinition" );
        }

        Node* result;
		Node* first;
        
        match(export_token);
        first = parseExportBindingList();
        result = NodeFactory::ExportDefinition(first);

    
        if( debug ) {
            Debugger::trace( "finish parseExportDefinition" );
        }

        return result;
    }

    static void testExportDefinition() {

        std::string input[] = { 
	        "export x",
	        "export f=g",
            "export f=g,h" 
        };

        std::string expected[] = { 
            "",
            "",
            "",
        };

        testParser("ExportDefinition",input,expected);
    }

    /**
     * ExportBindingList	
     *     ExportBinding ExportBindingListPrime
     * 	
     * ExportBindingListPrime	
     *     , ExportBinding ExportBindingListPrime
     *     empty
     */

    Node* parseExportBindingList() {

        if( debug ) {
            Debugger::trace( "begin parseExportBindingList" );
        }

        Node* result;
		Node* first;
        
        first = parseExportBinding();
        result = parseExportBindingListPrime(NodeFactory::List(0,first));

        if( debug ) {
            Debugger::trace( "finish parseExportBindingList" );
        }

        return result;
    }

    Node* parseExportBindingListPrime( Node* first ) {

        if( debug ) {
            Debugger::trace( "begin parseExportBindingListPrime" );
        }

        Node* result;

        if( lookahead( comma_token ) ) {

            match( comma_token );

            Node* second;
        
            second = parseExportBinding();
            result = parseExportBindingListPrime(NodeFactory::List(NodeFactory::List(0,first),second));

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseExportBindingListPrime'" );
        }

        return result;
    }

    static void testExportBindingList() {

        std::string input[] = { 
	        "x,y,z",
	        "z" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("ExportBindingList",input,expected);
    }

    /**
     * ExportBinding	
     *     FunctionName
     *     FunctionName = FunctionName
     */

    Node* parseExportBinding() {

        if( debug ) {
            Debugger::trace( "begin parseExportBinding" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        first = parseFunctionName();
        if( lookahead(assign_token) ) {
            match(assign_token);
            second = parseFunctionName();
        } else {
            second = 0;
        }

        result = NodeFactory::ExportBinding(first,second);

    
        if( debug ) {
            Debugger::trace( "finish parseExportBinding" );
        }

        return result;
    }

    static void testExportBinding() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("ExportBinding",input,expected);
    }

    /**
     * VariableDefinition	
     *     VariableDefinitionKind VariableBindingList[allowIn]
     */

    Node* parseVariableDefinition() {

        if( debug ) {
            Debugger::trace( "begin parseVariableDefinition" );
        }

        int  first;
        Node* result;
		Node* second;
        
        // The following logic goes something like this: If it is a
        // const_token, then first is a const_token. If it is a var_token
        // then first is var_token. If it is anything else then first is
        // the default (var_token).

        first = lookahead(const_token) ? match(const_token) :
             lookahead(var_token) ? match(var_token) : var_token;

        second = parseVariableBindingList(allowIn_mode);
        result = NodeFactory::VariableDefinition(first,second);
    
        if( debug ) {
            Debugger::trace( "finish parseVariableDefinition" );
        }

        return result;
    }

    static void testVariableDefinition() {

        std::string input[] = { 
	        "var x=10",
	        "const pi:Number=3.1415",
            "var n:Number,o:Object,b:bool=false" 
        };

        std::string expected[] = { 
            "variabledefinition( var_token, variablebinding( typedvariable( qualifiedidentifier( 0, x ), 0 ), literalnumber( 10 ) ) )",
            "variabledefinition( const_token, variablebinding( typedvariable( qualifiedidentifier( 0, pi ), qualifiedidentifier( 0, Number ) ), literalnumber( 3.1415 ) ) )",
            "variabledefinition( var_token, list( list( variablebinding( typedvariable( qualifiedidentifier( 0, n ), qualifiedidentifier( 0, Number ) ), 0 ), variablebinding( typedvariable( qualifiedidentifier( 0, o ), qualifiedidentifier( 0, Object ) ), 0 ) ), variablebinding( typedvariable( qualifiedidentifier( 0, b ), qualifiedidentifier( 0, bool ) ), literalbool( false ) ) ) )"
        };

        testParser("VariableDefinition",input,expected);
    }

    /**
     * VariableBindingList	
     *     VariableBinding VariableBindingListPrime
     * 	
     * VariableBindingListPrime	
     *     , VariableBinding VariableBindingListPrime
     *     empty
     */

    Node* parseVariableBindingList(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseVariableBindingList" );
        }

        Node* result;
		Node* first;
        
        first = parseVariableBinding(mode);
        result = parseVariableBindingListPrime(mode,NodeFactory::List(0,first));

        if( debug ) {
            Debugger::trace( "finish parseVariableBindingList" );
        }

        return result;
    }

    Node* parseVariableBindingListPrime(int mode, Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseVariableBindingListPrime" );
        }

        Node* result;

        if( lookahead( comma_token ) ) {

            match( comma_token );

            Node* second;
        
            second = parseVariableBinding(mode);
            result = parseVariableBindingListPrime(mode,NodeFactory::List(NodeFactory::List(0,first),second));

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseVariableBindingListPrime'" );
        }

        return result;
    }

    static void testVariableBindingList() {

        std::string input[] = { 
	        "x,y,z",
	        "z" 
        };

        std::string expected[] = { 
            "list( list( variablebinding( typedvariable( identifier( x ), 0 ), 0 ), variablebinding( typedvariable( identifier( y ), 0 ), 0 ) ), variablebinding( typedvariable( identifier( z ), 0 ), 0 ) )",
            "variablebinding( typedvariable( identifier( z ), 0 ), 0 )" 
        };

        testParser("VariableBindingList",input,expected);
    }

    /**
     * VariableBinding	
     *     TypedVariable
     *     TypedVariable = AssignmentExpression
     *     TypedVariable = MultipleAttributes
     */

    Node* parseVariableBinding(int mode, bool hasInitializer[]) {

        if( debug ) {
            Debugger::trace( "begin parseVariableBinding" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        first = parseTypedVariable(mode);
        if( lookahead(assign_token) ) {
            match(assign_token);
            if( lookahead(abstract_token) || lookahead(final_token) ||
                lookahead(private_token) || lookahead(public_token) ||
                lookahead(static_token) ) {
                second = parseMultipleAttributes();
            } else {
                second = parseAssignmentExpression(mode);
                if( lookahead(semicolon_token) ) {
                    // do nothing.
                } else {
                    second = parseMultipleAttributesPrime(NodeFactory::List(0,second));
                }
            }
        } else {
            second = 0;
        }

        result = NodeFactory::VariableBinding(first,second);
    
        if( debug ) {
            Debugger::trace( "finish parseVariableBinding" );
        }

        return result;
    }

    Node* parseVariableBinding(int mode) {
        bool hasValue[] = {false};
        return parseVariableBinding(mode,hasValue);
    }

    static void testVariableBinding() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("VariableBinding",input,expected);
    }

    /**
     * MultipleAttributes	
     *     	Attribute [no line break] Attribute
     *     	MulitpleAttributes [no line break] Attribute
     */

    Node* parseMultipleAttributes() {

        if( debug ) {
            Debugger::trace( "begin parseMultipleAttributes" );
        }

        Node* result;
		Node* first;
        
        first = parseAttribute();
        result = parseMultipleAttributesPrime(NodeFactory::List(0,first));

        if( debug ) {
            Debugger::trace( "finish parseMultipleAttributes" );
        }

        return result;
    }

    ListNode* parseMultipleAttributesPrime(ListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseMultipleAttributes" );
        }

        ListNode* result;
        
        if( newline() ) {
            error(syntax_error,"No line break in multiple attributes definition.");
        }
            
        first = NodeFactory::List(first,parseAttribute());
        
        while( !lookahead(semicolon_token) ) {

            if( newline() ) {
                error(syntax_error,"No line break in multiple attributes definition.");
            }
            
            first = NodeFactory::List(first,parseAttribute());
        
        }

        result = first;

        if( debug ) {
            Debugger::trace( "finish parseMultipleAttributes" );
        }

        return result;
    }

    /**
     * TypedVariable	
     *     	Identifier
     *     	Identifier : TypeExpression
     */

    Node* parseTypedVariable(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseTypedVariable" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        first = parseIdentifier();
        if( lookahead(colon_token) ) {
            match(colon_token);
            second = parseTypeExpression(mode);
        } else {
            second = 0;
        }

        result = NodeFactory::TypedVariable(first,second);
    
        if( debug ) {
            Debugger::trace( "finish parseTypedVariable" );
        }

        return result;
    }

    static void testTypedVariable() {

        std::string input[] = { 
	        "a::b::c::x=10",
	        "y" 
        };

        std::string expected[] = { 
            "typedvariable( qualifiedidentifier( list( list( list( 0, identifier( a ) ), identifier( b ) ), identifier( c ) ), x ), 0 )",
            "typedvariable( qualifiedidentifier( 0, y ), 0 )" 
        };

        testParser("TypedVariable",input,expected);
    }

    /**
     * FunctionDefinition	
     *     FunctionDeclaration Block
     *     FunctionDeclaration Semicolon
     */

    Node* parseFunctionDefinition(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseFunctionDefinition" );
        }

        Node* result;
		Node* first;

        first = parseFunctionDeclaration();
        if( lookahead(leftbrace_token) ) {
            Node* second;
            second = parseBlock();
            result = NodeFactory::FunctionDefinition(first,second);
        } else {
            matchSemicolon(mode);
            result = first;
        }
         
        if( debug ) {
            Debugger::trace( "finish parseFunctionDefinition" );
        }

        return result;
    }

    static void testFunctionDefinition() {

        std::string input[] = { 
	        "function f();",
	        "function f(x,y) {var z;}", 
        };

        std::string expected[] = { 
            "",
            "",
            "",
        };

        testParser("FunctionDefinition",input,expected);
    }

    /**
     * FunctionDeclaration	
     *     function FunctionName FunctionSignature
     */

    Node* parseFunctionDeclaration() {

        if( debug ) {
            Debugger::trace( "begin parseFunctionDeclaration" );
        }

        Node* result;
		Node* first;
		Node* second;
    
        match(function_token);
        first = parseFunctionName();
        second = parseFunctionSignature();
        result = NodeFactory::FunctionDeclaration(first,second);

        if( debug ) {
            Debugger::trace( "finish parseFunctionDeclaration" );
        }

        return result;
    }

    static void testFunctionDeclaration() {

        std::string input[] = { 
	        "",
	        "" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("FunctionDeclaration",input,expected);
    }

    /**
     * FunctionName	
     *     Identifier
     *     get [no line break] Identifier
     *     set [no line break] Identifier
     */

    Node* parseFunctionName() {

        if( debug ) {
            Debugger::trace( "begin parseFunctionName" );
        }

        Node* result;

        if( lookahead( get_token ) ) {
            match( get_token );
            result = NodeFactory::FunctionName(get_token,parseIdentifier());
        } else if( lookahead( set_token ) ) {
            match( set_token );
            result = NodeFactory::FunctionName(set_token,parseIdentifier());
        } else {
            result = NodeFactory::FunctionName(empty_token,parseIdentifier());
        }

        if( debug ) {
            Debugger::trace( "finish parseFunctionName" );
        }
        return result;
    }

    static void testFunctionName() {

        Debugger::trace( "begin testFunctionName" );

        std::string input[] = { 
            "x", 
            "get x", 
            "set x" 
        };

        std::string expected[] = { 
            "", 
            "", 
            "" 
        };

        testParser("FunctionName",input,expected);
    }

    /**
     * FunctionSignature	
     *     ParameterSignature ResultSignature
     * 
     * ParameterSignature	
     *     ( Parameters )
     */

    Node* parseFunctionSignature() {

        if( debug ) {
            Debugger::trace( "begin parseFunctionSignature" );
        }

        Node* result;
		Node* first;
		Node* second;
    
        // inlined: parseParameterSignature

        match( leftparen_token ); 
        first = parseParameters();
        match( rightparen_token );
        second = parseResultSignature();

        result = NodeFactory::FunctionSignature(first,second);
         
        if( debug ) {
            Debugger::trace( "finish parseFunctionSignature" );
        }

        return result;
    }

    static void testFunctionSignature() {

        std::string input[] = { 
	        "(u,v:String='default', | 'a' x:String='default', 'b' y:Number,...z:Object) : bool",
	        "(u,v:String='default', 'a' x:String='default', | 'b' y:Number,...z:Object) : bool",
        };

        std::string expected[] = { 
            "", 
            "", 
        };

        testParser("FunctionSignature",input,expected);
    }

    /**
     * Parameters	
     *     	«empty»
     *     	AllParameters
     */

    Node* parseParameters() {

        if( debug ) {
            Debugger::trace( "begin parseParameters" );
        }

        Node* result;
        
        if( lookahead(rightparen_token) ) {
            result = 0;
        } else {
            result = parseAllParameters(0);
        }

        if( debug ) {
            Debugger::trace( "finish parseParameters" );
        }

        return result;
    }

    static void testParameters() {

        std::string input[] = { 
	        "",
            "",
        };

        std::string expected[] = { 
            "",
            "",
        };

        testParser("Parameters",input,expected);
    }

    /**
     * AllParameters	
     *     Parameter
     *     Parameter , AllParameters
     *     Parameter OptionalParameterPrime
     *     Parameter OptionalParameterPrime , OptionalNamedRestParameters
     *     | NamedRestParameters
     *     RestParameter
     *     RestParameter , | NamedParameters
     */

    Node* parseAllParameters(ListNode* first) {

        if( debug ) {
            Debugger::trace( "begin parseAllParameters" );
        }

        Node* result;
        
        if( lookahead(tripledot_token) ) {
            first = NodeFactory::List(first,parseRestParameter());
            if( lookahead(comma_token) ) {
                match(comma_token);
                match(bitwiseor_token);
                result = NodeFactory::List(first,parseNamedParameters());
            } else {
                result = first;
            }
        } else if( lookahead(bitwiseor_token) ) {
            match(bitwiseor_token);
            result = parseNamedRestParameters();
        } else {
            first = NodeFactory::List(first,parseParameter());
            if( lookahead(comma_token) ) {
                match(comma_token);
                result = parseAllParameters(first);
            } else if( lookahead(assign_token) ) {
                first = NodeFactory::List(0,parseOptionalParameterPrime(first));
                if( lookahead(comma_token) ) {
                    match(comma_token);
                    result = NodeFactory::List(first,parseOptionalNamedRestParameters());
                } else {
                    result = first;
                }
            } else {
                result = first;
            }
        }
    
        if( debug ) {
            Debugger::trace( "finish parseAllParameters" );
        }

        return result;
    }

    /**
     * OptionalNamedRestParameters	
     *     OptionalParameter
     *     OptionalParameter , OptionalNamedRestParameters
     *     | NamedRestParameters
     *     RestParameter
     *     RestParameter , | NamedParameters
     */

    ListNode* parseOptionalNamedRestParameters() {

        if( debug ) {
            Debugger::trace( "begin parseOptionalNamedRestParameters" );
        }

        ListNode* result;
        
        if( lookahead(tripledot_token) ) {
            ListNode* first = NodeFactory::List(0,parseRestParameter());
            if( lookahead(comma_token) ) {
                match(comma_token);
                match(bitwiseor_token);
                result = NodeFactory::List(first,parseNamedParameters());
            } else {
                result = first;
            }
        } else if( lookahead(bitwiseor_token) ) {
            match(bitwiseor_token);
            result = parseNamedRestParameters();
        } else {
            ListNode* first = NodeFactory::List(0,parseOptionalParameter());
            if( lookahead(comma_token) ) {
                match(comma_token);
                result = NodeFactory::List(first,parseOptionalNamedRestParameters());
            } else {
                result = first;
            }
        }
    
        if( debug ) {
            Debugger::trace( "finish parseOptionalNamedRestParameters" );
        }

        return result;
    }

    /**
     * NamedRestParameters	
     *     NamedParameter
     *     NamedParameter , NamedRestParameters
     *     RestParameter
     */

    ListNode* parseNamedRestParameters() {

        if( debug ) {
            Debugger::trace( "begin parseNamedRestParameters" );
        }

        ListNode* result;
        
        if( lookahead(tripledot_token) ) {
            result = NodeFactory::List(0,parseRestParameter());
        } else {
            ListNode* first = NodeFactory::List(0,parseNamedParameter());
            if( lookahead(comma_token) ) {
                match(comma_token);
                result = NodeFactory::List(first,parseNamedRestParameters());
            } else {
                result = first;
            }
        }
    
        if( debug ) {
            Debugger::trace( "finish parseNamedRestParameters" );
        }

        return result;
    }

    /**
     * NamedParameters	
     *     NamedParameter
     *     NamedParameter , NamedParameters
     */

    ListNode* parseNamedParameters() {

        if( debug ) {
            Debugger::trace( "begin parseNamedParameters" );
        }

        ListNode* result;
		ListNode* first;
        
        first = NodeFactory::List(0,parseNamedParameter());
        if( lookahead(comma_token) ) {
            match(comma_token);
            result = NodeFactory::List(first,parseNamedParameters());
        } else {
            result = first;
        }
    
        if( debug ) {
            Debugger::trace( "finish parseNamedParameters" );
        }

        return result;
    }

    static void testNamedParameters() {

        std::string input[] = { 
	        "'a' x:String='default'",
	        "'a' x:String='default', 'b' y:Number,'c' z:Object" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("NamedParameters",input,expected);
    }

    /**
     * RestParameter	
     *     ...
     *     ... Parameter
     */

    Node* parseRestParameter() {

        if( debug ) {
            Debugger::trace( "begin parseRestParameter" );
        }

        Node* result;
		Node* first;
        
        match(tripledot_token);
        if( lookahead(identifier_token) ||
            lookahead(get_token) ||
            lookahead(set_token) ) {
            first = parseParameter();
        } else {
            first = 0;
        }

        result = NodeFactory::RestParameter(first);

    
        if( debug ) {
            Debugger::trace( "finish parseRestParameter" );
        }

        return result;
    }

    static void testRestParameter() {

        std::string input[] = { 
	        "...rest",
	        "...args" 
        };

        std::string expected[] = { 
            "restparameter( typedidentifier( identifier( rest ), 0 ), 0 )",
            "restparameter( typedidentifier( identifier( args ), 0 ), 0 )" 
        };

        testParser("RestParameter",input,expected);
    }

    /**
     * Parameter	
     *     Identifier
     *     Identifier : TypeExpression[allowIn]
     */

    Node* parseParameter() {

        if( debug ) {
            Debugger::trace( "begin parseParameter" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        first = parseIdentifier();
        if( lookahead(colon_token) ) {
            match(colon_token);
            second = parseTypeExpression(allowIn_mode);
        } else {
            second = 0;
        }

        result = NodeFactory::Parameter(first,second);
    
        if( debug ) {
            Debugger::trace( "finish parseParameter" );
        }

        return result;
    }

    static void testParameter() {

        std::string input[] = { 
	        "x",
	        "x:Object" 
        };

        std::string expected[] = { 
            "typedidentifier( identifier( x ), 0 )",
            "typedidentifier( identifier( x ), qualifiedidentifier( 0, Object ) )" 
        };

        testParser("Parameter",input,expected);
    }

    /**
     * OptionalParameter	
     *     Parameter = AssignmentExpressionallowIn
     */

    Node* parseOptionalParameter() {

        if( debug ) {
            Debugger::trace( "begin parseOptionalParameter" );
        }

        Node* result;
		Node* first;
        
        first = parseParameter();
        result = parseOptionalParameterPrime(first);

        if( debug ) {
            Debugger::trace( "finish parseOptionalParameter" );
        }

        return result;
    }

    Node* parseOptionalParameterPrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parseOptionalParameterPrime" );
        }

        Node* result;
        
        match(assign_token);
        result = NodeFactory::OptionalParameter(first,parseAssignmentExpression(allowIn_mode));
    
        if( debug ) {
            Debugger::trace( "finish parseOptionalParameterPrime" );
        }

        return result;
    }

    static void testOptionalParameter() {

        std::string input[] = { 
	        "x",
	        "x:Object" 
        };

        std::string expected[] = { 
            "typedidentifier( identifier( x ), 0 )",
            "typedidentifier( identifier( x ), qualifiedidentifier( 0, Object ) )" 
        };

        testParser("OptionalParameter",input,expected);
    }

    /**
     * NamedParameter	
     *     Parameter
     *     OptionalParameter
     *     String NamedParameter
     */

    Node* parseNamedParameter() {

        if( debug ) {
            Debugger::trace( "begin parseNamedParameter" );
        }

        Node* result;
		Node* first;
        
        if( lookahead(stringliteral_token) ) {
            first = NodeFactory::LiteralString(scanner->getTokenText(match(stringliteral_token)));
            result = NodeFactory::NamedParameter(first,parseNamedParameter());
        } else {
            first = parseParameter();
            if( lookahead(assign_token) ) {
                result = parseOptionalParameterPrime(first);
            } else {
                result = first;
            }
        }

        if( debug ) {
            Debugger::trace( "finish parseNamedParameter" );
        }

        return result;
    }

    static void testNamedParameter() {

        std::string input[] = { 
	        "x",
	        "x:Object" 
        };

        std::string expected[] = { 
            "typedidentifier( identifier( x ), 0 )",
            "typedidentifier( identifier( x ), qualifiedidentifier( 0, Object ) )" 
        };

        testParser("NamedParameter",input,expected);
    }

    /**
     * ResultSignature	
     *     «empty»
     *     : TypeExpression[allowIn]
     */

    Node* parseResultSignature() {

        if( debug ) {
            Debugger::trace( "begin parseResultSignature" );
        }

        Node* result;
        
        if( lookahead(colon_token) ) {
            match(colon_token);
            result = parseTypeExpression(allowIn_mode);
        } else {
            result = 0;
        }

        if( debug ) {
            Debugger::trace( "finish parseResultSignature" );
        }

        return result;
    }

    static void testResultSignature() {

        std::string input[] = { 
	        ": String",
	        ": Object" 
        };

        std::string expected[] = { 
            "qualifiedidentifier( 0, String )",
            "qualifiedidentifier( 0, Object )" 
        };

        testParser("ResultSignature",input,expected);
    }

    /**
     * ClassDefinition	
     *     class Identifier Inheritance Block
     *     class Identifier Semicolon
     */

    Node* parseClassDefinition(int mode) {

        if( debug ) {
            Debugger::trace( "begin parseClassDefinition" );
        }

        Node* result;
		Node* first;

        match(class_token);
        first = parseIdentifier();

        if( lookaheadSemicolon(mode) ) {
            matchSemicolon(mode);
            result = NodeFactory::ClassDeclaration(first);
        } else {
        
            Node* second;
			Node* third;

            second = parseInheritance();
            third = parseBlock();
            result = NodeFactory::ClassDefinition(first,second,third);

        }
        
            
        if( debug ) {
            Debugger::trace( "finish parseClassDefinition" );
        }

        return result;
    }

    static void testClassDefinition() {

        std::string input[] = { 
	        "class C;",
	        "class C {}",
            "class C extends B {}",
            "class C implements I,J {}",
            "class C extends B implements I {}"
        };

        std::string expected[] = { 
            "classdeclaration( identifier( C ) )",
            "classdefinition( identifier( C ), inheritance( 0, 0 ), 0 )",
            "classdefinition( identifier( C ), inheritance( superclass( qualifiedidentifier( 0, B ) ), 0 ), 0 )",
            "classdefinition( identifier( C ), inheritance( 0, list( list( 0, qualifiedidentifier( 0, I ) ), qualifiedidentifier( 0, J ) ) ), 0 )",
            "classdefinition( identifier( C ), inheritance( superclass( qualifiedidentifier( 0, B ) ), list( 0, qualifiedidentifier( 0, I ) ) ), 0 )"
        };

        testParser("ClassDefinition",input,expected);
    }

    /**
     * Inheritance	
     *     «empty»
     *     extends TypeExpressionallowIn
     *     implements TypeExpressionList
     *     extends TypeExpressionallowIn implements TypeExpressionList
     */

    Node* parseInheritance() {

        if( debug ) {
            Debugger::trace( "begin parseInheritance" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        if( lookahead(extends_token) ) {
            match(extends_token);
            first = parseTypeExpression(allowIn_mode);
        } else {
            first = 0;
        }
    
        if( lookahead(implements_token) ) {
            match(implements_token);
            second = parseTypeExpressionList();
        } else {
            second = 0;
        }

        result = NodeFactory::Inheritance(first,second);

        if( debug ) {
            Debugger::trace( "finish parseInheritance" );
        }

        return result;
    }

    static void testSuperclass() {

        std::string input[] = { 
	        "",
            "" 
        };

        std::string expected[] = { 
	        "",
            "" 
        };

        testParser("Superclass",input,expected);
    }

    /**
     * ImplementsList	
     *     «empty»
     *     implements TypeExpressionList
     */

    Node* parseImplementsList() {

        if( debug ) {
            Debugger::trace( "begin parseImplementsList" );
        }

        Node* result;
        
        if( lookahead(implements_token) ) {
            match(implements_token);
            result = parseTypeExpressionList();
        } else {
            result = 0;
        }
    
        if( debug ) {
            Debugger::trace( "finish parseImplementsList" );
        }

        return result;
    }

    static void testImplementsList() {

        std::string input[] = { 
	        "",
            "" 
        };

        std::string expected[] = { 
	        "",
            "" 
        };

        testParser("ImplementsList",input,expected);
    }

    /**
     * TypeExpressionList	
     *     TypeExpression[allowin] TypeExpressionListPrime
     *
     * TypeExpressionListPrime:	
     *     , TypeExpression[allowin] TypeExpressionListPrime
     *     empty
     */

    Node* parseTypeExpressionList() {

        if( debug ) {
            Debugger::trace( "begin parseTypeExpressionList" );
        }

        Node* result;
		Node* first;
        
        first = parseTypeExpression(allowIn_mode);
        result = parseTypeExpressionListPrime(NodeFactory::List(0,first));

        if( debug ) {
            Debugger::trace( "finish parseTypeExpressionList" );
        }

        return result;
    }

    Node* parseTypeExpressionListPrime( Node* first ) {

        if( debug ) {
            Debugger::trace( "begin parseTypeExpressionListPrime" );
        }

        Node* result;

        if( lookahead( comma_token ) ) {

            match( comma_token );

            Node* second;
        
            second = parseTypeExpression(allowIn_mode);
            result = parseTypeExpressionListPrime(NodeFactory::List(NodeFactory::List(0,first),second));

        } else {
            result = first;
        }

        if( debug ) {
            Debugger::trace( "finish parseTypeExpressionListPrime'" );
        }

        return result;
    }

    static void testTypeExpressionList() {

        std::string input[] = { 
	        "I1",
	        "I1,I2" 
        };

        std::string expected[] = { 
            "",
            "" 
        };

        testParser("TypeExpressionList",input,expected);
    }

    /**
     * ExtendsList	
     *     «empty»
     *     extends TypeExpressionList
     */

    Node* parseExtendsList() {

        if( debug ) {
            Debugger::trace( "begin parseExtendsList" );
        }

        Node* result;
        
        if( lookahead(extends_token) ) {
            match(extends_token);
            result = parseTypeExpressionList();
        } else {
            result = 0;
        }
    
        if( debug ) {
            Debugger::trace( "finish parseExtendsList" );
        }

        return result;
    }

    static void testExtendsList() {

        std::string input[] = { 
	        "",
            "" 
        };

        std::string expected[] = { 
	        "",
            "" 
        };

        testParser("ExtendsList",input,expected);
    }

    /**
     * NamespaceDefinition	
     *     namespace Identifier ExtendsList
     */

    Node* parseNamespaceDefinition() {

        if( debug ) {
            Debugger::trace( "begin parseNamespaceDefinition" );
        }

        Node* result;
		Node* first;
		Node* second;
        
        match(namespace_token);
        first = parseIdentifier();
        second = parseExtendsList();
        result = NodeFactory::NamespaceDefinition(first,second);
            
        if( debug ) {
            Debugger::trace( "finish parseNamespaceDefinition" );
        }

        return result;
    }

    static void testNamespaceDefinition() {

        std::string input[] = { 
	        "namespace N;",
        };

        std::string expected[] = { 
            "",
        };

        testParser("NamespaceDefinition",input,expected);
    }

    /**
     * LanguageDeclaration	
     *     use LanguageAlternatives
     */

    Node* parseLanguageDeclaration() {

        if( debug ) {
            Debugger::trace( "begin parseLanguageDeclaration" );
        }

        Node* result;

        match(use_token);
        result = NodeFactory::LanguageDeclaration(parseLanguageAlternatives());

        if( debug ) {
            Debugger::trace( "finish parseLanguageDeclaration" );
        }

        return result;
    }

    /**
     * LanguageAlternatives	
     *     LanguageIds
     *     LanguageAlternatives | LanguageIds
     */

    Node* parseLanguageAlternatives() {

        if( debug ) {
            Debugger::trace( "begin parseLanguageAlternatives" );
        }

        ListNode* result;

        result = parseLanguageIds();

        while( lookahead(bitwiseor_token) ) {

            match(bitwiseor_token);
            result = NodeFactory::List(result,parseLanguageIds());

        }

        if( debug ) {
            Debugger::trace( "finish parseLanguageAlternatives" );
        }

        return result;
    }

    /**
     * LanguageIds	
     *     «empty»
     *     LanguageId LanguageIds
     */

    ListNode* parseLanguageIds() {

        if( debug ) {
            Debugger::trace( "begin parseLanguageIds" );
        }

        ListNode* result;

        if( lookahead(bitwiseor_token) || lookahead(semicolon_token) ) {
            result = 0;
        } else {
            result = NodeFactory::List(NodeFactory::List(0,parseLanguageId()),parseLanguageIds());
        }

        if( debug ) {
            Debugger::trace( "finish parseLanguageIds" );
        }

        return result;
    }

    /**
     * LanguageId	
     *     Identifier
     *     Number
     */

    Node* parseLanguageId() {

        if( debug ) {
            Debugger::trace( "begin parseLanguageId" );
        }

        Node* result;
        
        if( lookahead(numberliteral_token) ) {
            result = NodeFactory::LiteralNumber(scanner->getTokenText(match(numberliteral_token)));
        } else {
            result = parseIdentifier();
        }

        if( debug ) {
            Debugger::trace( "finish parseLanguageId" );
        }

        return result;
    }

    /**
     * PackageDefinition	
     *     package Block
     *     package PackageName Block
     */

    Node* parsePackageDefinition() {

        if( debug ) {
            Debugger::trace( "begin parsePackageDefinition" );
        }

        Node* result;
        
        match(package_token);
        if( lookahead(leftbrace_token) ) {
            result = NodeFactory::PackageDefinition(0,parseBlock());
        } else {
            result = NodeFactory::PackageDefinition(parsePackageName(),parseBlock());
        }
            
        if( debug ) {
            Debugger::trace( "finish parsePackageDefinition" );
        }

        return result;
    }

    static void testPackageDefinition() {

        std::string input[] = { 
	        "package {}",
	        "package P {}",
        };

        std::string expected[] = { 
            "",
            "",
        };

        testParser("PackageDefinition",input,expected);
    }

    /**
     * PackageName	
     *     Identifier
     *     PackageName . Identifier
     */

    Node* parsePackageName() {

        if( debug ) {
            Debugger::trace( "begin parsePackageName" );
        }

        Node* result;
		Node* first;

        first = parseIdentifier();
        result = parsePackageNamePrime(first);

        if( debug ) {
            Debugger::trace( "finish parsePackageName" );
        }

        return result;
    }

    Node* parsePackageNamePrime(Node* first) {

        if( debug ) {
            Debugger::trace( "begin parsePackageNamePrime" );
        }

        Node* result;

        while( lookahead(dot_token) ) {
            match(dot_token);
            first = NodeFactory::List(NodeFactory::List(0,first),parseIdentifier());
        }

        result = first;

        if( debug ) {
            Debugger::trace( "finish parsePackageNamePrime" );
        }

        return result;
    }

    /*
     * Program	
     *     Directives
     */

    ProgramNode* parseProgram() {

        if( debug ) {
            Debugger::trace( "begin parseProgram" );
        }

        ProgramNode* result;
        
		StatementListNode* first = parseDirectives();
            
        if( debug ) {
            Debugger::trace( "finish parseProgram" );
        }

		if( error_count == 0 ) {
		    result = NodeFactory::Program(first);
		} else {
            result = NodeFactory::Program(0);
        }
        return result;
    }

    static void testProgram() {

        std::string input[] = { 
	        "function f() {} interface I { function m(); } class C extends B implements I,J,K { static function g() : I { return new C; } function m() {} } C.g().f();",
            "package P { include 'file1'; import 'lib1'; namespace V1; const v1 = V1; v1 var x; }",
        };

        std::string expected[] = { 
            "program( statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( 0, annotateddefinition( 0, functiondefinition( functiondeclaration( functionname( <empty>, identifier( f ) ), functionsignature( 0, 0 ) ), 0 ) ) ), annotateddefinition( 0, interfacedefinition( identifier( I ), 0, statementlist( statementlist( statementlist( 0, annotateddefinition( 0, functiondeclaration( functionname( <empty>, identifier( m ) ), functionsignature( 0, 0 ) ) ) ), 0 ), 0 ) ) ) ), annotateddefinition( 0, classdefinition( identifier( C ), inheritance( superclass( qualifiedidentifier( 0, B ) ), list( list( list( 0, qualifiedidentifier( 0, I ) ), qualifiedidentifier( 0, J ) ), qualifiedidentifier( 0, K ) ) ), statementlist( statementlist( statementlist( statementlist( 0, annotateddefinition( attributelist( identifier( static ), 0 ), functiondefinition( functiondeclaration( functionname( <empty>, identifier( g ) ), functionsignature( 0, qualifiedidentifier( 0, I ) ) ), statementlist( statementlist( statementlist( statementlist( 0, 0:returnstatement( newexpression( qualifiedidentifier( 0, C ) ) ) ), emptystatement ), 0 ), 0 ) ) ) ), annotateddefinition( 0, functiondefinition( functiondeclaration( functionname( <empty>, identifier( m ) ), functionsignature( 0, 0 ) ), 0 ) ) ), 0 ), 0 ) ) ) ), callexpression( memberexpression( callexpression( memberexpression( qualifiedidentifier( 0, C ), qualifiedidentifier( 0, g ) ), 0 ), qualifiedidentifier( 0, f ) ), 0 ) ), emptystatement ), 0 ), 0 ) )",
            "program( statementlist( statementlist( statementlist( 0, packagedefinition( identifier( P ), statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( statementlist( 0, includestatement( literalstring( file1 ) ) ), emptystatement ), annotateddefinition( 0, importdefinition( literalstring( lib1 ), 0 ) ) ), emptystatement ), annotateddefinition( 0, namespacedefinition( identifier( V1 ), 0 ) ) ), annotateddefinition( 0, variabledefinition( const_token, list( 0, variablebinding( typedvariable( identifier( v1 ), 0 ), qualifiedidentifier( 0, V1 ) ) ) ) ) ), annotateddefinition( attributelist( qualifiedidentifier( 0, v1 ), 0 ), variabledefinition( var_token, list( 0, variablebinding( typedvariable( identifier( x ), 0 ), 0 ) ) ) ) ), 0 ), 0 ) ) ), 0 ), 0 ) )",
        };

        testParser("Program",input,expected);
    }

    /**
     * self tests.
     */

    static void main(int argc, char* args[] ) {
	    int failureCount = 0;
        //try {

        //    Debugger::setDbgFile("debug.dbg");
        //    Debugger::setOutFile("debug.out");
        //    Debugger::setErrFile("debug.err");

            Debugger::trace( "Parser test begin" );
            
            // Expressions

            testIdentifier();
            testSimpleQualifiedIdentifier();
			testExpressionQualifiedIdentifier();
            testQualifiedIdentifier();
			testUnitExpression();
            testPrimaryExpression();
			testParenthesizedExpression();
			testParenthesizedListExpression();
			testFunctionExpression();
            testObjectLiteral();
            testArrayLiteral();
            testAttributeExpression();
            testPostfixExpression();
			testUnaryExpression();
            testMultiplicativeExpression();
            testAdditiveExpression();
            testAssignmentExpression();
            testListExpression();
            testTypeExpression();
#if 0

            // Statements

            testDirective();
            testStatement();
            testIfStatement();
            testExpressionStatement();
            testBlock();
            testDirectives();
            testUseStatement();
            testReturnStatement();

            // Definitions

            testAnnotatedDefinition();
            testAttributes();
            testAttribute();
            testDefinition();
            testVariableDefinition();
            testVariableBindingList();
            testTypedVariable();
            testFunctionDefinition();
            testFunctionSignature();
            testParameters();
            testParameter();
            testNamedParameters();
            testRestParameter();
            testResultSignature();
            testClassDefinition();
            testProgram();
#endif
            if( failureCount != 0 ) {
                Debugger::trace( "Parser test completed: tests failed" );
            } else {
                Debugger::trace( "Parser test completed successfully" );
            }

        //} catch( Exception e ) {
        //    Debugger::trace( "compile error: " + e );
        //    e.printStackTrace();
        //} finally {
        //    System.setOut( 0 );
        //
    
	
	}

//    static void testParser(std::string name, std::string input[], std::string expected[], int count=0) {
//		testParser(input,expected,count);
//	}

    static void testParser(std::string name, std::string input[], std::string expected[], int count=0) {

        Debugger::trace( "begin testParser: " + name );

		std::string result;

        Parser* parser;
        long t=0;

		std::ofstream log((name+".log").c_str());
		Context::init();
		Debugger::init(&log);
		Context cx(log,Context::e4_profile);
        for( int i = 0; i < count; i++ ) {
			try {
			std::istringstream in = std::istringstream(input[i]);
            parser = new Parser(in,log,name);
			NodePrinter printer(NodePrinter::machine_mode);
            Node* node = parser->parseProgram();
			node->evaluate(cx,&printer);
			std::string result = printer.str();
			int l1 = strlen(result.c_str());
			int l2 = strlen(expected[i].c_str());
			std::string s1 = result.c_str();
			std::string s2 = expected[i].c_str();
			if( !strncmp(result.c_str(),expected[i].c_str(),strlen(expected[i].c_str()))) {
				log << "passed : " << input[i] << " -> " << result;
				NodePrinter printer(NodePrinter::man_mode);
				node->evaluate(cx,&printer);
				log << printer.str();
			} else {
				log << "failed : " << input[i] << " -> " << result;
			}
			} catch(...) {
				log << "internal error : " << input[i];
			}


        }
		Debugger::reset();
        Debugger::trace( "finish testParser" );
    }

};

}
}
}

#endif // Parser_h

/*
 * The end.
 */
