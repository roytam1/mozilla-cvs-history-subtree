/*
 * Execution context.
 */

#ifndef Context_h
#define Context_h

#include <stack>
#include <vector>
#include <fstream>
#include <map>


namespace esc {
namespace v1 {

class Value;
class ObjectValue;

class Context {

	ObjectValue* global;
	std::stack<ObjectValue*> scopes;
	std::ofstream* err_root;

public:
	static std::map<std::string,std::string> e3cp_profile;
	static std::map<std::string,std::string> e4_profile;
	std::map<std::string,std::string> features;
	std::ofstream& err;

	static void init();

	Context(std::ofstream& err, const std::map<std::string,std::string>& features = e4_profile) : err(err), err_root(0), features(features) {
	}

	Context() : err(*(err_root=new std::ofstream("default.err"))) {
	}

	~Context() {
		if( err_root ) {
			delete err_root;
		}
	}

	void pushScope(ObjectValue* scope) {
		if(scopes.empty()) {
			global = scope;
		}

		scopes.push(scope);
	}

	void popScope() {
		scopes.pop();
	}

	std::stack<ObjectValue*> getScopes() {
		return scopes;
	}

	ObjectValue* scope() {
		return scopes.top();
	}

	ObjectValue* globalScope() {
		return global;
	}

	std::vector<Value*> used_namespaces;
};

}
}

#endif // Context_h

/*
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
