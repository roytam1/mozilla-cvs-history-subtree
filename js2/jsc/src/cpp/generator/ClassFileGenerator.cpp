#pragma warning ( disable : 4786 )

#include "Nodes.h"
#include "ClassFileGenerator.h"
#include "NodeFactory.h"
#include "ReferenceValue.h"
#include "ConstantEvaluator.h"
#include "Builder.h"
#include "GlobalObjectBuilder.h"

namespace esc {
namespace v1 {

    void ClassFileGenerator::init(std::string scriptname) {

		short class_index,name_index,descriptor_index,name_and_type_index;
		short methodref_index,attribute_name_index;
		
		// This class
        
        name_index     = cf.addConstant(ConstantUtf8Info((char*)scriptname.c_str()));
        class_index	   = cf.addConstant(ConstantClassInfo((short)name_index));
        cf.this_class  = class_index;

		// Super class
        
        name_index     = cf.addConstant(ConstantUtf8Info("java/lang/Object"));
        class_index	   = cf.addConstant(ConstantClassInfo((short)name_index));
        cf.super_class = class_index;

        // Methods

        std::vector<byte>& exception_table = *new std::vector<byte>();
        std::vector<byte>& code_attributes = *new std::vector<byte>();

        // <init>
		
		std::vector<byte>* code            = new std::vector<byte>();
        std::vector<byte>* code_attribute  = new std::vector<byte>();

		// Methodref for Object.<init>

        name_index          = cf.addConstant(ConstantUtf8Info("<init>"));
        descriptor_index    = cf.addConstant(ConstantUtf8Info("()V"));
        name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
		methodref_index     = cf.addConstant(ConstantMethodrefInfo(cf.super_class,name_and_type_index));

        Byte(*code,(byte)OP_aload_0);
        Byte(*code,(byte)OP_invokespecial);
		Short(*code,methodref_index);
        Byte(*code,(byte)OP_return);

        attribute_name_index = cf.addConstant(ConstantUtf8Info("Code"));
        CodeAttribute(*code_attribute,attribute_name_index,17,(short)1,(short)1,5,*code,(short)0,exception_table,(short)0,code_attributes);

        name_index          = cf.addConstant(ConstantUtf8Info("<init>"));
        descriptor_index    = cf.addConstant(ConstantUtf8Info("()V"));

		cf.methods->push_back(new std::vector<byte>());
        MethodInfo(*cf.methods->back(),(short)ACC_PUBLIC,name_index,descriptor_index,(short)1,*code_attribute);
        
		delete code_attribute;
		delete code;

		// main
		
		code = new std::vector<byte>();
        Byte(*code,(byte)OP_return);
        code_attribute = new std::vector<byte>();
        attribute_name_index = cf.addConstant(ConstantUtf8Info("Code"));
        CodeAttribute(*code_attribute,attribute_name_index,code->size()+12,(short)0,(short)1,code->size(),*code,(short)0,exception_table,(short)0,code_attributes);

        name_index          = cf.addConstant(ConstantUtf8Info("main"));
        descriptor_index    = cf.addConstant(ConstantUtf8Info("([Ljava/lang/String;)V"));
		cf.methods->push_back(new std::vector<byte>());
        MethodInfo(*cf.methods->back(),(short)(ACC_PUBLIC|ACC_STATIC),name_index,descriptor_index,(short)1,*code_attribute);

		delete code_attribute;
		delete code;

		// init

		code = new std::vector<byte>();
        code_attribute = new std::vector<byte>();

        Byte(*code,(byte)OP_return);
        attribute_name_index = cf.addConstant(ConstantUtf8Info("Code"));
        CodeAttribute(*code_attribute,attribute_name_index,code->size()+12,(short)0,(short)2,code->size(),*code,(short)0,exception_table,(short)0,code_attributes);
        name_index          = cf.addConstant(ConstantUtf8Info("init"));
        descriptor_index    = cf.addConstant(ConstantUtf8Info("(Lcom/compilercompany/esrt/v1/ObjectValue;[Ljava/lang/Object;)V"));
		cf.methods->push_back(new std::vector<byte>());
        MethodInfo(*cf.methods->back(),(short)(ACC_PUBLIC|ACC_STATIC),name_index,descriptor_index,(short)1,*code_attribute);

		delete code_attribute;
		delete code;
    }

    std::vector<byte>& ClassFileGenerator::emit(std::vector<byte>& bytes) {

        // Finish generating code for the start method
		
        short name_index           = cf.addConstant(ConstantUtf8Info("run"));
        short descriptor_index     = cf.addConstant(ConstantUtf8Info("(Ljava/util/Stack;Lcom/compilercompany/esrt/v1/ObjectValue;)Lcom/compilercompany/esrt/v1/ObjectValue;"));
        short attribute_name_index = cf.addConstant(ConstantUtf8Info("Code"));

        std::vector<byte>& code_attribute = *new std::vector<byte>();
        CodeAttribute(code_attribute,attribute_name_index,cf.code->size()+12,(short)4,(short)max_locals,cf.code->size(),*cf.code,(short)0,*cf.exception_table,(short)0,*cf.code_attributes);

		cf.methods->push_back(new std::vector<byte>());
        MethodInfo(*cf.methods->back(),(short)(ACC_PUBLIC|ACC_STATIC),(short)name_index,(short)descriptor_index,(short)1,code_attribute);

		delete &code_attribute;

        return ByteCodeFactory::ClassFile(
			bytes,
			cf.magic,
			cf.minor_version,
			cf.major_version,
            (short)(cf.constant_pool->size()==0?0:cf.constant_pool->size()+1),
            *cf.constant_pool,
			cf.access_flags,
            cf.this_class,
            cf.super_class,
            (short)(cf.interfaces->size()==0?0:cf.interfaces->size()),
            *cf.interfaces,
            (short)(cf.fields->size()==0?0:cf.fields->size()),
            *cf.fields,
            (short)(cf.methods->size()==0?0:cf.methods->size()),
            *cf.methods,
            (short)(cf.attributes->size()==0?0:cf.attributes->size()),
            *cf.attributes);
    }

	short ClassFileGenerator::ClassFile::addConstant(std::vector<byte>& bytes) {

		std::vector<std::vector<byte>*>::iterator next = constant_pool->begin();
		std::vector<std::vector<byte>*>::iterator end = constant_pool->end();

		// See if this constant is already in the pool.
		
		while ( next != end ) {
			if( (*next)->size() == bytes.size() ) {
				std::vector<byte>::iterator it1 = (*next)->begin();
			    std::vector<byte>::iterator it2 = bytes.begin();
			    std::vector<byte>::iterator it1_end = (*next)->end();
				while( it1 != it1_end ) {
					if( *it1 != *it2 ) {
						break; // no match
					}
					++it1;
					++it2;
				}
				if( it1 == it1_end ) {
					short index = next - constant_pool->begin() + 1;
					return index;
				}
			}
			++next;
		}

		// If not, then add it.

		constant_pool->push_back(&bytes);
		short index = (short) constant_pool->size();
		return index;
	}

	// Evaluators

    // Base node
    
	Value* ClassFileGenerator::evaluate( Context& cx, Node* node ) { 
        throw;
    }

	// Expression evaluators

    Value* ClassFileGenerator::evaluate( Context& cx, ThisExpressionNode* node ) { 
		throw;
    }
    
	/*
	 * Unqualified identifiers evaluate to a ReferenceValue during semantic analysis,
	 * and so this method is never called.
	 */
	
	Value* ClassFileGenerator::evaluate( Context& cx, IdentifierNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, QualifiedIdentifierNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralBooleanNode* node ) { 
        throw;
    }

    Value* ClassFileGenerator::evaluate( Context& cx, LiteralNumberNode* node ) { 
        throw;
    }

	/*
	 * Literal string
	 */
	
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralStringNode* node ) { 

        short index;

        index = cf.addConstant(ConstantUtf8Info(node->value.c_str()));
        index = cf.addConstant(ConstantStringInfo(index));

        Byte(*cf.code,OP_ldc);

        if(index <= 0xff) {
            Byte(*cf.code, (byte)index);
        } else {
            Short(*cf.code,index);
        }

        return 0;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralUndefinedNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralRegExpNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, UnitExpressionNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FunctionExpressionNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ParenthesizedExpressionNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ParenthesizedListExpressionNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralObjectNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralFieldNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LiteralArrayNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, PostfixExpressionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, NewExpressionNode* node ) { 
        throw;
    }
    
	/*
	 * Indexed member expressions evaluate to a ReferenceValue during semantic analysis,
	 * and so this method is never called.
	 */

	Value* ClassFileGenerator::evaluate( Context& cx, IndexedMemberExpressionNode* node ) {
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ClassofExpressionNode* node ) {
        throw;
    }
    
	/*
	 * Member expressions evaluate to a ReferenceValue during semantic analysis,
	 * and so this method is never called.
	 */

	Value* ClassFileGenerator::evaluate( Context& cx, MemberExpressionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, CoersionExpressionNode* node ) { 
        throw;
    }

	/*
	 * CallExpressionNode
	 *
	 * Call expressions can be generated as invocations of the function
	 * object's call method, or as a direct call to a native function.
	 * If constant evaluation was able to resolve the function reference
	 * to a built-in native function, then call a direct call is generated.
	 *
	 * NOTE: this code is being generated into the start function with
	 *       parameters (Stack scope, ObjectValue this). These are in
	 *       local registers (0 and 1).
	 */

	Value* ClassFileGenerator::evaluate( Context& cx, CallExpressionNode* node ) { 

       
		// The local var index of the function object.

		node->local_var_index = max_locals++;

		Slot* slot = node->ref->getSlot(cx);
		if( false /*slot*/ ) {

			/* We have a slot so generate direct access to its method.
			 */

			MemberExpressionNode* member = (MemberExpressionNode*)node->member;

			if( member && member->base ) {

				/*
				 * For references with a base object, we use the base object
				 * as the target activation.
				 */

				// Put the base on the operand stack by
				// generating code with:

				member->base->evaluate(cx,this);

			} else {

				/*
				 * For references without a base object, we use the slot_index
				 * to know what object on the scope stack is the target activation.
				 *
				 * All generated code can assume that the 0th parameter is the
				 * scope stack, the 1st parameter is the this object, and the
				 * nth parameters are the fixed parameters.
				 *
				 *	  Method void start(java.util.Stack, com.compilercompany.esrt.v1.ObjectValue)
				 *
				 *	     0 iconst_0			// scope_index = 0
				 *	     1 istore_2			// temp for scope_index
				 *	     2 aload_0			// scope stack
				 *	     3 iload_2			// scope_index
				 *	     4 invokevirtual #2 <Method java.lang.Object elementAt(int)>
				 *	     7 checkcast #3 <Class com.compilercompany.esrt.v1.ObjectValue>
   				 */

				short index, class_index, name_index, descriptor_index, name_and_type_index, methodref_index;

				index				= cf.addConstant(ConstantUtf8Info("java/util/Vector"));
				class_index			= cf.addConstant(ConstantClassInfo(index));
				name_index          = cf.addConstant(ConstantUtf8Info("elementAt"));
				descriptor_index    = cf.addConstant(ConstantUtf8Info("(I)Ljava/lang/Object;"));
				name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
				methodref_index		= cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));

				// Generate code to get the nth activation frame on
				// the scope stack.

				int scope_index = node->ref->getScopeIndex();

				Byte(*cf.code,OP_aload_0);		  // Load the scope stack
				Byte(*cf.code,OP_bipush);		  // Load the scope index (0 means global)
				Byte(*cf.code,(byte)scope_index);
				Byte(*cf.code,OP_invokevirtual);  // Call Stack.elementAt and leave the 
				Short(*cf.code,methodref_index);  // result on the stack.

			}

			/*
			 * Checkcast the base object
			 */

			{
				short index, class_index;

				index		= cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
				class_index	= cf.addConstant(ConstantClassInfo(index));
				Byte(*cf.code,OP_checkcast);      // Call Stack.elementAt and leave the 
				Short(*cf.code,class_index);  // result on the stack.
			}


		} else {

			/* If not bound to a slot, then generate a dynamic lookup of the
			 * function.
			 */
			
			short index;

			index = cf.addConstant(ConstantUtf8Info(node->ref->name.begin()));
			index = cf.addConstant(ConstantStringInfo(index));

			// Load the receiver object

			Byte(*cf.code,(byte)OP_aload_1);   // This is wrong. In identifier must be looked up in scope.

			// Load the name of the identifier

			Byte(*cf.code,(byte)OP_ldc);

			if(index <= 0xff) {
				Byte(*cf.code,(byte)index);
			} else {
				Short(*cf.code,index);
			}

			index					  = cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
			short class_index		  = cf.addConstant(ConstantClassInfo(index));
			short name_index          = cf.addConstant(ConstantUtf8Info("get"));
			short descriptor_index    = cf.addConstant(ConstantUtf8Info("(Ljava/lang/String;)Lcom/compilercompany/esrt/v1/ObjectValue;"));
			short name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
			short methodref_index     = cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));
        
			//node->local_var_index = max_locals++;

			Byte(*cf.code,OP_invokevirtual);
			Short(*cf.code,methodref_index);
			Byte(*cf.code,OP_astore);
			Byte(*cf.code,node->local_var_index);

		}

		// Create args array

		{

			short index,class_index;
			index       = cf.addConstant(ConstantUtf8Info("java/lang/Object"));
			class_index = cf.addConstant(ConstantClassInfo(index));

			/* Allocate local register to hold the array and get the
			 * size of the array.
			 */
			byte args_var_index = this->max_locals++;
			short args_size     = node->args->size();
        
			Byte(*cf.code,OP_sipush);
			Short(*cf.code,args_size);
			Byte(*cf.code,OP_anewarray);
			Short(*cf.code,class_index);
			Byte(*cf.code,OP_astore);
			Byte(*cf.code,args_var_index);

			// Do user args

			if( node->args ) {
				node->args->local_var_index = args_var_index;
				node->args->evaluate(cx,this); 
			}

		}
		
		/* Generate the function call
		 */

        if( true /* is a function object */ ) {
			short index, class_index;

			index       = cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
			class_index = cf.addConstant(ConstantClassInfo(index));

			short name_index, descriptor_index, name_and_type_index, methodref_index;

			name_index          = cf.addConstant(ConstantUtf8Info("call"));
			descriptor_index    = cf.addConstant(ConstantUtf8Info("(Lcom/compilercompany/esrt/v1/ObjectValue;[Ljava/lang/Object;)Lcom/compilercompany/esrt/v1/ObjectValue;"));
			name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
			methodref_index     = cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));
    
			Byte(*cf.code,OP_aload);
			Byte(*cf.code,node->local_var_index);
			Byte(*cf.code,OP_aload_1);  // Load the current object (this)
			Byte(*cf.code,OP_aload);
			Byte(*cf.code,node->args->local_var_index);
			Byte(*cf.code,OP_invokevirtual);
			Short(*cf.code,methodref_index);
		
		} else {
		
		    /* Is a native method
			 */


		}
		
		return 0;
    }
    
	/*
	 * GetExpressionNode
	 *
	 * Get expressions are psuedo syntactic constructs, created when
	 * a member expression is used in a context where a value is
	 * expected. In the general case, a get expression is the same as
	 * a call expression with no arguments. In specfic cases, a get
	 * expression can be optimized as a direct access of a native 
	 * field.
	 */

	/* 
	 * What do we need to compile a variable reference to a field id?
	 * the name and the class that defines it. Instance variables would
	 * be instance fields of the Global prototype object. The runtime
	 * version of this object would have the native field that implements
	 * that variable.
	 *
	 * get x ();
	 *
	 * 1 aload_1							// get the target object value
	 * 2 getfield #3 <Field int _values_[]>	// get the property values array
	 * 5 iconst_0							// get the index of value
	 * 6 iaload								// load the value from values
	 */
		
	Value* ClassFileGenerator::evaluate( Context& cx, GetExpressionNode* node ) { 

		Slot* slot = node->ref->getSlot(cx);
		if( slot ) {

			/*
			 * We have a slot so generate code that directly accesses the value.
			 */

			if( node->member->base ) {

				/*
				 * For references with a base object, we use the base object
				 * as the target activation.
				 */

				// Put the base on the operand stack by
				// generating code with:

				node->member->base->evaluate(cx,this);

			} else {

				/*
				 * For references without a base object, we use the slot_index
				 * to know what object on the scope stack is the target activation.
				 *
				 * All generated code can assume that the 0th parameter is the
				 * scope stack, the 1st parameter is the this object, and the
				 * nth parameters are the fixed parameters.
				 *
				 *	  Method com.compilercompany.esrt.v1.ObjectValue 
				 *            run(java.util.Stack, com.compilercompany.esrt.v1.ObjectValue)
				 *
				 *	     0 iconst_0			// scope_index = 0
				 *	     1 istore_2			// temp for scope_index
				 *	     2 aload_0			// scope stack
				 *	     3 iload_2			// scope_index
				 *	     4 invokevirtual #2 <Method java.lang.Object elementAt(int)>
				 *	     7 checkcast #3 <Class com.compilercompany.esrt.v1.ObjectValue>
   				 */

				short index, class_index, name_index, descriptor_index, name_and_type_index, methodref_index;

				index				= cf.addConstant(ConstantUtf8Info("java/util/Vector"));
				class_index			= cf.addConstant(ConstantClassInfo(index));
				name_index          = cf.addConstant(ConstantUtf8Info("elementAt"));
				descriptor_index    = cf.addConstant(ConstantUtf8Info("(I)Ljava/lang/Object;"));
				name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
				methodref_index		= cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));

				// Generate code to get the nth activation frame on
				// the scope stack.

				int scope_index = node->ref->getScopeIndex();

				Byte(*cf.code,OP_aload_0);		  // Load the scope stack
				Byte(*cf.code,OP_bipush);		  // Load the scope index (0 means global)
				Byte(*cf.code,(byte)scope_index);
				Byte(*cf.code,OP_invokevirtual);  // Call Stack.elementAt and leave the 
				Short(*cf.code,methodref_index);  // result on the stack.

			}

			/*
			 * Checkcast the base object
			 */

			{
				short index, class_index;

				index		= cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
				class_index	= cf.addConstant(ConstantClassInfo(index));
				Byte(*cf.code,OP_checkcast);      // Call Stack.elementAt and leave the 
				Short(*cf.code,class_index);  // result on the stack.
			}

			/*
			 * If the slot value is an index into the values array, then 
			 * get the indexed value.
			 *
			 * If it is a get function, then generate a call to it. [Or,
			 * in the future, possibly inline the accessor code.]
			 */

			if( true /*slot->attrs & ATTR_SlotValueIsIndex*/ ) {

				short index, class_index, name_index, descriptor_index, name_and_type_index, fieldref_index;

				index				= cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
				class_index			= cf.addConstant(ConstantClassInfo(index));
				name_index          = cf.addConstant(ConstantUtf8Info("_values_"));
				descriptor_index    = cf.addConstant(ConstantUtf8Info("[Lcom/compilercompany/esrt/v1/ObjectValue;"));
				name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
				fieldref_index		= cf.addConstant(ConstantFieldrefInfo(class_index,name_and_type_index));
    
				Byte(*cf.code,OP_getfield);		// Get the _values_ array of the current object.
				Short(*cf.code,fieldref_index);	 
				Byte(*cf.code,OP_sipush);		// Push the index of the value to get
				Short(*cf.code,slot->intValue); // Value index
				Byte(*cf.code,OP_aaload);		// Push the value from _values_
			}

		} else {

			/* If not bound to a slot, then generate a dynamic lookup of the
			 * property.
			 */
			
			if( node->member->base ) {
			} else {

				/*
				 * For references without a base object, we use the slot_index
				 * to know what object on the scope stack is the target activation.
				 *
				 * All generated code can assume that the 0th parameter is the
				 * scope stack, the 1st parameter is the this object, and the
				 * nth parameters are the fixed parameters.
				 *
				 *	  Method void start(java.util.Stack, com.compilercompany.esrt.v1.ObjectValue)
				 *
				 *	     0 iconst_0			// scope_index = 0
				 *	     1 istore_2			// temp for scope_index
				 *	     2 aload_0			// scope stack
				 *	     3 iload_2			// scope_index
				 *	     4 invokevirtual #2 <Method java.lang.Object elementAt(int)>
				 *	     7 checkcast #3 <Class com.compilercompany.esrt.v1.ObjectValue>
   				 */

				short index, class_index, name_index, descriptor_index, name_and_type_index, methodref_index;

				index				= cf.addConstant(ConstantUtf8Info("java/util/Vector"));
				class_index			= cf.addConstant(ConstantClassInfo(index));
				name_index          = cf.addConstant(ConstantUtf8Info("elementAt"));
				descriptor_index    = cf.addConstant(ConstantUtf8Info("(I)Ljava/lang/Object;"));
				name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
				methodref_index		= cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));

				// Generate code to get the nth activation frame on
				// the scope stack.

				int scope_index = node->ref->getScopeIndex();

				Byte(*cf.code,OP_aload_0);		  // Load the scope stack
				Byte(*cf.code,OP_bipush);		  // Load the scope index (0 means global)
				Byte(*cf.code,(byte)scope_index);
				Byte(*cf.code,OP_invokevirtual);  // Call Stack.elementAt and leave the 
				Short(*cf.code,methodref_index);  // result on the stack.

			}

			/*
			 * Checkcast the base object
			 */

			{
				short index, class_index;

				index		= cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
				class_index	= cf.addConstant(ConstantClassInfo(index));
				Byte(*cf.code,OP_checkcast);      // Call Stack.elementAt and leave the 
				Short(*cf.code,class_index);  // result on the stack.
			}

			{

				// Load the receiver object

				short index;

				index = cf.addConstant(ConstantUtf8Info(node->ref->name.begin()));
				index = cf.addConstant(ConstantStringInfo(index));

				// Load the name of the identifier

				Byte(*cf.code,(byte)OP_ldc);

				if(index <= 0xff) {
					Byte(*cf.code,(byte)index);
				} else {
					Short(*cf.code,index);
				}

				index					  = cf.addConstant(ConstantUtf8Info("com/compilercompany/esrt/v1/ObjectValue"));
				short class_index		  = cf.addConstant(ConstantClassInfo(index));
				short name_index          = cf.addConstant(ConstantUtf8Info("get"));
				short descriptor_index    = cf.addConstant(ConstantUtf8Info("(Ljava/lang/String;)Lcom/compilercompany/esrt/v1/ObjectValue;"));
				short name_and_type_index = cf.addConstant(ConstantNameAndTypeInfo(name_index,descriptor_index));
				short methodref_index     = cf.addConstant(ConstantMethodrefInfo(class_index,name_and_type_index));
        
				node->local_var_index = max_locals++;

				Byte(*cf.code,OP_invokevirtual);
				Short(*cf.code,methodref_index);
				Byte(*cf.code,OP_astore);
				Byte(*cf.code,node->local_var_index);
			}
		}


		return 0;
    }
    
	/*
	 * SetExpressionNode
	 *
	 * Set expressions are psuedo syntactic constructs, created when
	 * a member expression is used in a context where a storage location
	 * is expected. In the general case, a set expression is the same as
	 * a call expression with one argument (the value to be stored.) In 
	 * specfic cases, a get expression can be optimized as a direct access 
	 * of a native field.
	 *
	 * set x (value);
     *
	 * 1 aload_1							// get the target object value
	 * 2 getfield #3 <Field int values[]>	// get the property values array
	 * 5 iconst_0							// get the index of the value
	 * 6 iconst_5							// get the value
	 * 7 iastore							// store the value in values
	 */

	Value* ClassFileGenerator::evaluate( Context& cx, SetExpressionNode* node ) { 

		if( node->member ) {

			// Only do this if you want to generate code for runtime lookup.
			// If this member has been bound, then generate a direct call to
			// the implementing function.
		
			node->member->evaluate(cx,this); 
		}

		// Code to call getter goes here.

		return 0;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, UnaryExpressionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, BinaryExpressionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ConditionalExpressionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, AssignmentExpressionNode* node ) { 
        throw;
    }
    
	/*
	 * Generate the code for a list (e.g. argument list). The owner of this node
	 * has already allocated a fixed size array. This function stuffs the list
	 * values into that array.
	 */
	
	int list_index;
	int list_array_register;

	Value* ClassFileGenerator::evaluate( Context& cx, ListNode* node ) { 
        
		/* Traverse to the bottom of the list and zero the list index.
		 */
		if( node->list ) {
			node->list->local_var_index = node->local_var_index; // inherited attribute
			node->list->evaluate(cx,this); 
		} else {
			list_index = 0;
		}

        /* If we are accumulating values in a list.
		 */
		
		if( node->local_var_index ) {
			
			/* Push the array that will hold the list value onto the stack.
			 */
			Byte(*cf.code,OP_aload);
			Byte(*cf.code,(byte)node->local_var_index);	// Use fast instructions

			/* Push the list index.
			 */
			Byte(*cf.code,OP_bipush);					// Use fast instructions
			Byte(*cf.code,(byte)list_index);
		}

		/* Evaluate the item expression, leaving it on
		 * the stack.
		 */
		if( node->item ) {
			node->item->evaluate(cx,this); // push the arg onto the stack
		}

		/* If there is a local var index for this node,
		 * then assume it is the value array and use it.
		 */

		if( node->local_var_index ) {
			/* Store the expression value in the array.
			 */
			Byte(*cf.code,OP_aastore);  // store the arg into the args array
			++list_index;
		}
		return 0;
    }

    // Statements

    Value* ClassFileGenerator::evaluate( Context& cx, StatementListNode* node ) { 
		if( node->list ) {
			node->list->evaluate(cx,this); 
		} 
		if( node->item ) {
			node->item->evaluate(cx,this);
		}
		return ObjectValue::undefinedValue;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, EmptyStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ExpressionStatementNode* node ) { 
		if( node->expr ) {
			node->expr->local_var_index = 0;  // this is a list so give it an array for the list value.
	        node->expr->evaluate(cx,this);
		}
		return 0;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, AnnotatedBlockNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, LabeledStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, IfStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, SwitchStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, CaseLabelNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, DoStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, WhileStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ForInStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ForStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, WithStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ContinueStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, BreakStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ReturnStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ThrowStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, TryStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, CatchClauseNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FinallyClauseNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, UseStatementNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, IncludeStatementNode* node ) { 
        throw;
    }

    // Definitions

    Value* ClassFileGenerator::evaluate( Context& cx, ImportDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ImportBindingNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, AnnotatedDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, AttributeListNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ExportDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ExportBindingNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, VariableDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, VariableBindingNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, TypedVariableNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FunctionDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FunctionDeclarationNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FunctionNameNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, FunctionSignatureNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ParameterNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, OptionalParameterNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ClassDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, ClassDeclarationNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, InheritanceNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, NamespaceDefinitionNode* node ) { 
        throw;
    }
    
	Value* ClassFileGenerator::evaluate( Context& cx, PackageDefinitionNode* node ) { 
        throw;
    }

    Value* ClassFileGenerator::evaluate( Context& cx, ProgramNode* node ) {

		/* The body of this script is put in a function called run, which
		 * takes two parameters: scope, and thisobj. Set max_locals
		 * to indicate how many local variables are used.
		 */
		max_locals = 2;
        
		if( node->statements != (ProgramNode*)0 ) {
			node->statements->evaluate(cx,this); 
		} else {
            Byte(*cf.code,(byte)OP_aconst_null);
		}
        Byte(*cf.code,(byte)OP_areturn);
        return (Value*)0;
    }

    /*
     * Test driver
     */

	static unsigned int col_counter = 0;
    void print_byte(byte b) {
		if(col_counter%16==0) {
			printf("\n\t");
		}
		printf("%3.2X",(unsigned char)b);
		++col_counter;
	}

    int ClassFileGenerator::main(int argc, char* argv[]) {

		NodeFactory::init();

		// Test #1
		
		{
			std::ofstream err("TestEmpty.err");
			Context cx(err);

			Builder*     globalObjectBuilder = new GlobalObjectBuilder();
			ObjectValue* globalPrototype     = new ObjectValue(*globalObjectBuilder);
			ObjectValue  global              = ObjectValue(globalPrototype);

			cx.pushScope(&global); // first scope is always considered the global scope.
			cx.used_namespaces.push_back(ObjectValue::publicNamespace);

			// print('hello, world')

			Node* node = NodeFactory::Program(0);



			Evaluator* analyzer = new ConstantEvaluator();
			ClassFileGenerator* generator = new ClassFileGenerator("TestEmpty");

			node->evaluate(cx,analyzer); 	// Analyze it
			node->evaluate(cx,generator); 	// Generate it

			std::vector<byte>& bytes=*new std::vector<byte>();
			generator->emit(bytes);			// Emit it
			delete generator;  // Get rid of the generator after one use.
			delete analyzer;

			// Write the bytes to a disk file.
			
			std::ofstream* of = new std::ofstream("TestEmpty.class",std::ios::binary);
			of->write(bytes.begin(),bytes.size());
			printf("\n\nTestEmpty.class:\n");
			col_counter = 0;
		    std::for_each(bytes.begin(),bytes.end(),print_byte);
			delete &bytes;
			delete of;

		}

		// Test #2
		{
			std::ofstream err("TestHelloWorld.err");
			Context cx(err);

			Builder*     globalObjectBuilder = new GlobalObjectBuilder();
			ObjectValue* globalPrototype     = new ObjectValue(*globalObjectBuilder);
			ObjectValue  global              = ObjectValue(globalPrototype);

			cx.pushScope(&global); // first scope is always considered the global scope.
			cx.used_namespaces.push_back(ObjectValue::publicNamespace);

			// print('hello, world')

			Node* node = NodeFactory::Program(
				NodeFactory::CallExpression(
					NodeFactory::MemberExpression(0,NodeFactory::Identifier("print")),
					NodeFactory::List(0,NodeFactory::LiteralString("hello, world"))));

			Evaluator* analyzer = new ConstantEvaluator();
			ClassFileGenerator* generator = new ClassFileGenerator("TestHelloWorld");

			node->evaluate(cx,analyzer); 	// Analyze it
			node->evaluate(cx,generator); 	// Generate it

			std::vector<byte>& bytes=*new std::vector<byte>();
			generator->emit(bytes);			// Emit it
			delete generator;  // Get rid of the generator after one use.
			delete analyzer;

			// Write the bytes to a disk file.
			
			std::ofstream* of = new std::ofstream("TestHelloWorld.class",std::ios::binary);
			of->write(bytes.begin(),bytes.size());
			printf("\n\nTestHelloWorld.class:\n");
			col_counter = 0;
		    std::for_each(bytes.begin(),bytes.end(),print_byte);
			delete &bytes;
			delete of;

		}

		// Test #3

		{
			std::ofstream err("TestGetVersion.err");
			Context cx(err);

			Builder*     globalObjectBuilder = new GlobalObjectBuilder();
			ObjectValue* globalPrototype     = new ObjectValue(*globalObjectBuilder);
			ObjectValue  global              = ObjectValue(globalPrototype);

			cx.pushScope(&global); // first scope is always considered the global scope.
			cx.used_namespaces.push_back(ObjectValue::publicNamespace);

			// version

			Node* node = NodeFactory::Program(
				NodeFactory::GetExpression(
					NodeFactory::MemberExpression(0,
						NodeFactory::Identifier("version"))));

			Evaluator* analyzer = new ConstantEvaluator();
			ClassFileGenerator* generator = new ClassFileGenerator("TestGetVersion");

			node->evaluate(cx,analyzer); 	// Analyze it
			node->evaluate(cx,generator); 	// Generate it

			std::vector<byte>& bytes=*new std::vector<byte>();
			generator->emit(bytes);			// Emit it
			delete generator;  // Get rid of the generator after one use.
			delete analyzer;

			// Write the bytes to a disk file.
			
			std::ofstream* of = new std::ofstream("TestGetVersion.class",std::ios::binary);
			of->write(bytes.begin(),bytes.size());
			printf("\n\nTestGetVersion.class:\n");
			col_counter = 0;
		    std::for_each(bytes.begin(),bytes.end(),print_byte);
			delete &bytes;
			delete of;

		}

		// Test #4

		{
			std::ofstream err("TestHelloGetVersion.err");
			Context cx(err);

			Builder*     globalObjectBuilder = new GlobalObjectBuilder();
			ObjectValue* globalPrototype     = new ObjectValue(*globalObjectBuilder);
			ObjectValue  global              = ObjectValue(globalPrototype);

			cx.pushScope(&global); // first scope is always considered the global scope.
			cx.used_namespaces.push_back(ObjectValue::publicNamespace);

			// version

			Node* node = NodeFactory::Program(
				NodeFactory::StatementList(NodeFactory::StatementList(0,
					NodeFactory::CallExpression(
						NodeFactory::MemberExpression(0,NodeFactory::Identifier("print")),
						NodeFactory::List(0,
							NodeFactory::LiteralString("hello, world")))),
					NodeFactory::GetExpression(
						NodeFactory::MemberExpression(0,
							NodeFactory::Identifier("version")))));

			Evaluator* analyzer = new ConstantEvaluator();
			ClassFileGenerator* generator = new ClassFileGenerator("TestHelloGetVersion");

			node->evaluate(cx,analyzer); 	// Analyze it
			node->evaluate(cx,generator); 	// Generate it

			std::vector<byte>& bytes=*new std::vector<byte>();
			generator->emit(bytes);			// Emit it
			delete generator;  // Get rid of the generator after one use.
			delete analyzer;

			// Write the bytes to a disk file.
			
			std::ofstream* of = new std::ofstream("TestHelloGetVersion.class",std::ios::binary);
			of->write(bytes.begin(),bytes.size());
			printf("\n\nTestHelloGetVersion.class:\n");
			col_counter = 0;
		    std::for_each(bytes.begin(),bytes.end(),print_byte);
			delete &bytes;
			delete of;

		}
		NodeFactory::clear();

		return 0;
    }
}
}

/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
