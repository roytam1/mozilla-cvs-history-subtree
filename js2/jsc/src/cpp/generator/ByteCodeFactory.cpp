/*
 * ByteCodeFactory.cpp
 *
 * Emits byte code for a particular component of the classfile.
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <algorithm>

#include "ClassFileConstants.h"
#include "ByteCodeFactory.h"

namespace esc {
namespace v1 {

    std::ofstream* of;
    void put_byte(unsigned byte b) {
		printf("%3.2x",(unsigned char)b);
		of->put((unsigned char)b);
	}

    /**
     * Make a CONSTANT_Methodref_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantFieldrefInfo(
		std::vector<byte>& bytes,
		short class_index, 
        short name_and_type_index) {

        bytes = Byte(bytes,CONSTANT_Fieldref);
        bytes = Short(bytes,class_index);
        bytes = Short(bytes,name_and_type_index);
        
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantFieldrefInfo(
		short class_index, 
        short name_and_type_index) {
		return ConstantFieldrefInfo(*new std::vector<byte>(),class_index,name_and_type_index);
    }

    /**
     * Make a CONSTANT_Methodref_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantMethodrefInfo(
		std::vector<byte>& bytes,
		short class_index, 
        short name_and_type_index) {

        bytes = Byte(bytes,CONSTANT_Methodref);
        bytes = Short(bytes,class_index);
        bytes = Short(bytes,name_and_type_index);
        
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantMethodrefInfo(
		short class_index, 
        short name_and_type_index) {
		return ConstantMethodrefInfo(*new std::vector<byte>(),class_index,name_and_type_index);
    }

    /**
     * Make a Constant Pool
     */

	std::vector<byte>& ByteCodeFactory::ConstantPool(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& constants) {
        
		for(int i=0;i<constants.size();i++) {
			bytes.insert(bytes.end(),constants[i]->begin(),constants[i]->end());
        }

        return bytes;
    }

    /**
     * Make an Interfaces table
     */

	std::vector<byte>& ByteCodeFactory::Interfaces(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& interfaces) {
        
		for(int i=0;i<interfaces.size();i++) {
			bytes.insert(bytes.end(),interfaces[i]->begin(),interfaces[i]->end());
        }
        return bytes;
    }

    /**
     * Make a Fields table
     */

	std::vector<byte>& ByteCodeFactory::Fields(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& fields) {
        
		for(int i=0;i<fields.size();i++) {
			bytes.insert(bytes.end(),fields[i]->begin(),fields[i]->end());
        }
        return bytes;
    }

    /**
     * Make a Methods table
     */

	std::vector<byte>& ByteCodeFactory::Methods(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& methods) {
        
		for(int i=0;i<methods.size();i++) {
			bytes.insert(bytes.end(),methods[i]->begin(),methods[i]->end());
        }
        return bytes;
    }

    /**
     * Make an Attributes table
     */

	std::vector<byte>& ByteCodeFactory::Attributes(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& attributes) {
        
		for(int i=0;i<attributes.size();i++) {
			bytes.insert(bytes.end(),attributes[i]->begin(),attributes[i]->end());
        }
        return bytes;
    }

    /**
     * Make a CONSTANT_Utf8_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantUtf8Info(
		std::vector<byte>& bytes,
		const char* text) {

		int text_length = strlen(text);
        bytes = Byte(bytes,CONSTANT_Utf8);
        bytes = Short(bytes,text_length);
		bytes.insert(bytes.end(),text,text+text_length);
        
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantUtf8Info(
		const char* text) {
		return ConstantUtf8Info(*new std::vector<byte>(),text);
    }

    /**
     * Make a CONSTANT_String_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantStringInfo(
		std::vector<byte>& bytes,
		short index) {
        bytes = Byte(bytes,CONSTANT_String);
        bytes = Short(bytes,index);
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantStringInfo(
		short index) {
		return ConstantStringInfo(*new std::vector<byte>(),index);
    }

    /**
     * Make a CONSTANT_Class_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantClassInfo(
		std::vector<byte>& bytes,
		short name_index) {

        bytes = Byte(bytes,CONSTANT_Class);
        bytes = Short(bytes,name_index);
        
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantClassInfo(
		short name_index) {
		return ConstantClassInfo(*new std::vector<byte>(),name_index);
    }

    /**
     * Make a CONSTANT_NameAndType_info
     */

    std::vector<byte>& ByteCodeFactory::ConstantNameAndTypeInfo(
		std::vector<byte>& bytes,
		short name_index,
		short descriptor_index) {

        bytes = Byte(bytes,CONSTANT_NameAndType);
        bytes = Short(bytes,name_index);
        bytes = Short(bytes,descriptor_index);
        
		return bytes;
    }

    std::vector<byte>& ByteCodeFactory::ConstantNameAndTypeInfo(
		short name_index,
		short descriptor_index) {
		return ConstantNameAndTypeInfo(*new std::vector<byte>(),name_index,descriptor_index);
    }

    /**
     * Make a method_info
     */

    std::vector<byte>& ByteCodeFactory::MethodInfo(
		std::vector<byte>& bytes,
		short access_flags, 
		short name_index,
        short descriptor_index, 
		short attributes_count, 
		std::vector<byte>& attributes) {

        bytes = Short(bytes,access_flags);
        bytes = Short(bytes,name_index);
        bytes = Short(bytes,descriptor_index);
        bytes = Short(bytes,attributes_count);
        if( attributes_count != 0 ) {
			bytes.insert(bytes.end(),attributes.begin(),attributes.end());
        }
        
		return bytes;
    }

    /**
     * Make a Code_attribute
     */

    std::vector<byte>& ByteCodeFactory::CodeAttribute(
		std::vector<byte>& bytes,
		short attribute_name_index, 
		int attribute_length,
        short max_stack, 
		short max_locals, 
		int code_length, 
		std::vector<byte>& code,
        short exception_table_length, 
		std::vector<byte>& exception_table, 
		short attributes_count,
        std::vector<byte>& attributes) {

        Short(bytes,attribute_name_index);
        Int(bytes,attribute_length);
        Short(bytes,max_stack);
        Short(bytes,max_locals);
        Int(bytes,code_length);
        if( code_length != 0 ) {
			bytes.insert(bytes.end(),code.begin(),code.end());
        }
        Short(bytes,exception_table_length);
        if( exception_table_length != 0 ) {
			bytes.insert(bytes.end(),exception_table.begin(),exception_table.end());
        }
        Short(bytes,attributes_count);
        if( code_length != 0 ) {
			bytes.insert(bytes.end(),attributes.begin(),attributes.end());
        }
		return bytes;
    }

    /**
     * Make a class file.
     */

    std::vector<byte>& ByteCodeFactory::ClassFile(std::vector<byte>& bytes, char* classname) {

        int    magic = 0xCAFEBABE; 
        short  minor_version = 0; 
        short  major_version = 46;

        // Constants - a vector of byte vectors.
		// 

        std::vector<std::vector<byte>*>& constants = *new std::vector<std::vector<byte>*>();
		
		constants.push_back(new std::vector<byte>());
		ConstantMethodrefInfo(*constants.back(),(short)3,(short)7);

		constants.push_back(new std::vector<byte>());
        ConstantClassInfo(*constants.back(),(short)8);

		constants.push_back(new std::vector<byte>());
        ConstantClassInfo(*constants.back(),(short)9);

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"<init>");

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"()V");

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"Code");

		constants.push_back(new std::vector<byte>());
        ConstantNameAndTypeInfo(*constants.back(),(short)4,(short)5);

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),classname);

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"java/lang/Object");

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"main");

		constants.push_back(new std::vector<byte>());
        ConstantUtf8Info(*constants.back(),"([Ljava/lang/String;)V");

		constants.push_back(new std::vector<byte>());
        ConstantNameAndTypeInfo(*constants.back(),(short)10,(short)11);

        short  constant_pool_count = (short)(constants.size()+1);

        // Class structure

        short  access_flags = ACC_SUPER|ACC_PUBLIC;
        short  this_class = 2;
        short  super_class = 3;
        short  interfaces_count = 0;
        std::vector<std::vector<byte>*>& interfaces = *new std::vector<std::vector<byte>*>();
        short  fields_count = 0;
        std::vector<std::vector<byte>*>& fields = *new std::vector<std::vector<byte>*>();

        // Methods

        std::vector<byte>& exception_table_bytes = *new std::vector<byte>();
        std::vector<byte>& code_attributes_bytes = *new std::vector<byte>();

        
        std::vector<std::vector<byte>*>& methods = *new std::vector<std::vector<byte>*>();
        std::vector<byte>& code_bytes2 = *new std::vector<byte>();

        Byte(code_bytes2,(byte)0x2a); // 
        Byte(code_bytes2,(byte)0xb7);
        Byte(code_bytes2,(byte)0x00);
        Byte(code_bytes2,(byte)0x01);
        Byte(code_bytes2,(byte)0xb1);
        std::vector<byte>& code_attribute2 = *new std::vector<byte>();
        code_attribute2 = CodeAttribute(code_attribute2,(short)6,17,(short)1,(short)1,5,code_bytes2,(short)0,exception_table_bytes,(short)0,code_attributes_bytes);

		methods.push_back(new std::vector<byte>());
        MethodInfo(*methods.back(),(short)0,(short)4,(short)5,(short)1,code_attribute2);
        
		delete &code_attribute2;
		delete &code_bytes2;

		std::vector<byte>& code_bytes1 = *new std::vector<byte>();
        Byte(code_bytes1,(byte)0xb1); // 
        std::vector<byte>& code_attribute1 = *new std::vector<byte>();
        code_attribute1 = CodeAttribute(code_attribute1,(short)6,13,(short)0,(short)1,1,code_bytes1,(short)0,exception_table_bytes,(short)0,code_attributes_bytes);

		methods.push_back(new std::vector<byte>());
        MethodInfo(*methods.back(),(short)(ACC_PUBLIC|ACC_STATIC),(short)10,(short)11,(short)1,code_attribute1);

        short methods_count = (short)methods.size();

		delete &code_attribute1;
		delete &code_bytes1;

        // Attributes

        short  attributes_count = 0;
        std::vector<std::vector<byte>*>& attributes = *new std::vector<std::vector<byte>*>();

        ClassFile(bytes,magic,minor_version,major_version,constant_pool_count,
            constants,access_flags,this_class,super_class,interfaces_count,
            interfaces,fields_count,fields,methods_count,methods,
            attributes_count,attributes);

		return bytes;
	}

    std::vector<byte>& ByteCodeFactory::ClassFile(
        std::vector<byte>& bytes,
		int    magic, 
        short  minor_version, 
        short  major_version,
        short  constant_pool_count,
        std::vector<std::vector<byte>*>& constant_pool,
        short  access_flags,
        short  this_class,
        short  super_class,
        short  interfaces_count,
        std::vector<std::vector<byte>*>& interfaces,
        short  fields_count,
        std::vector<std::vector<byte>*>& fields,
        short  methods_count,
        std::vector<std::vector<byte>*>& methods,
        short  attributes_count,
        std::vector<std::vector<byte>*>& attributes ) {


		Int(bytes,magic);
		Short(bytes,minor_version);
		Short(bytes,major_version);
        Short(bytes,constant_pool_count);
		if( constant_pool_count != 0 ) {
			bytes = ConstantPool(bytes,constant_pool);
        }
        Short(bytes,access_flags);
        Short(bytes,this_class);
        Short(bytes,super_class);

        Short(bytes,interfaces_count);
        if( interfaces_count != 0 ) {
			bytes = Interfaces(bytes,interfaces);
        }
        Short(bytes,fields_count);
        if( fields_count != 0 ) {
			bytes = Fields(bytes,fields);
        }
        Short(bytes,methods_count);
        if( methods_count != 0 ) {
			bytes = Methods(bytes,methods);
        }
        Short(bytes,attributes_count);
        if( attributes_count != 0 ) {
			bytes = Attributes(bytes,attributes);
        }

        return bytes;
    }

    /*
     * Test driver
     */

    int ByteCodeFactory::main(int argc, char* argv[]) {
		of = new std::ofstream("TestByteCodeGenerator.class",std::ios::binary);
		std::vector<byte> &bytes=*new std::vector<byte>();
		ClassFile(bytes,"TestByteCodeGenerator");
		printf("\n\nTestByteCodeGenerator.class: ");
		std::for_each(bytes.begin(),bytes.end(),put_byte);
		delete &bytes;
		return 0;
    };

};
};



/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
