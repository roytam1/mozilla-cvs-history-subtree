#ifndef Token_h
#define Token_h

#include <string>
#include "Tokens.h"

namespace esc {
namespace v1 {
namespace lexer {


/*
 * Represents token instances: literals and identifiers.
 *
 * This file implements the class Token that is used to carry
 * information from the Scanner to the Parser.
 */

class Token {
	int    tokenClass;
    std::string* lexeme;
public:
	Token( int tokenClass, std::string* lexeme ) 
		: tokenClass(tokenClass), lexeme(lexeme) {
	}

	int getTokenClass() {
		return tokenClass;
	}

	/* 
	 * Return a copy of the token text string. Caller deletes it.
	 */
	
	std::string getTokenText() {
		if( tokenClass == stringliteral_token ) {
			std::string buf = std::string(lexeme->length()-2,0);
			// don't include quotes.
            lexeme->copy(buf.begin(),lexeme->length()-2,1);
			return buf;
		}

		return std::string(*lexeme);
	}

	std::string getTokenSource() {
		return std::string(*lexeme);
	}

	static const std::string getTokenClassName( int token_class ) {
		return tokenClassNames[-1*token_class];
	}

    /**
	 * main()
	 *
	 * Unit test driver. Execute the command 'java Token' at the
	 * shell to verify this class' basic functionality.
	 */

    static void main(int argc, char* args[]) {
	    printf("Token begin");
		for(int i = 0; i>=stringliteral_token;i--) {
		    printf("%s=%d\n",tokenClassNames[-1*i].c_str(),i);
	    }
		printf("Token end");
	}

};

}
}
}

#endif // Token_h

/*
 * The end.
 */
