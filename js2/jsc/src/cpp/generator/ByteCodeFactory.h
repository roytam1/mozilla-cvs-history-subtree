/*
 * ByteCodeFactory.h
 *
 * Emits byte code for a particular component of the classfile.
 */

#ifndef ByteCodeFactory_h
#define ByteCodeFactory_h

#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>

#include "ClassFileConstants.h"

#define byte char

namespace esc {
namespace v1 {

class ByteCodeFactory: protected ClassFileConstants {

public:

private: 
	enum { debug = false, };

	std::string name;
    std::vector<char> bytes;


public:

    /*
     * Tags
     */

	enum {
		CONSTANT_Utf8        = 0x01,
		CONSTANT_Class       = 0x07,
		CONSTANT_String      = 0x08,
		CONSTANT_Methodref   = 0x0a,
		CONSTANT_NameAndType = 0x0c,
    };

    /*
     * Class access and property modifiers
     */

    enum {
		ACC_PUBLIC    = 0x0001,
		ACC_STATIC    = 0x0008,
		ACC_FINAL     = 0x0010,
		ACC_SUPER     = 0x0020,
		ACC_INTERFACE = 0x0200,
		ACC_ABSTRACT  = 0x0400,
	};

    static std::vector<byte>& ConstantFieldrefInfo(
		std::vector<byte>& bytes, 
		short class_index, 
        short name_and_type_index);

    static std::vector<byte>& ConstantFieldrefInfo(
		short class_index, 
        short name_and_type_index);

    static std::vector<byte>& ConstantMethodrefInfo(
		std::vector<byte>& bytes, 
		short class_index, 
        short name_and_type_index);

    static std::vector<byte>& ConstantMethodrefInfo(
		short class_index, 
        short name_and_type_index);

	static std::vector<byte>& ConstantPool(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& constants);
		
	static std::vector<byte>& Interfaces(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& interfaces);
		
	static std::vector<byte>& Fields(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& fields);
		
	static std::vector<byte>& Methods(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& methods);
		
	static std::vector<byte>& Attributes(
		std::vector<byte>& bytes,
        std::vector<std::vector<byte>*>& attributes);
		
    static std::vector<byte>& ConstantUtf8Info(
		std::vector<byte>& bytes,
		const char* text);

    static std::vector<byte>& ConstantUtf8Info(
		const char* text);

    static std::vector<byte>& ConstantStringInfo(
		std::vector<byte>& bytes,
		short index);
    
    static std::vector<byte>& ConstantStringInfo(
		short index);
    
	static std::vector<byte>& ConstantClassInfo(
		std::vector<byte>& bytes,
		short name_index);
	
	static std::vector<byte>& ConstantClassInfo(
		short name_index);
	
    static std::vector<byte>& ConstantNameAndTypeInfo(
		std::vector<byte>& bytes,
		short name_index,
		short descriptor_index);
	
    static std::vector<byte>& ConstantNameAndTypeInfo(
		short name_index,
		short descriptor_index);
	
    static std::vector<byte>& MethodInfo(
		std::vector<byte>& bytes,
		short access_flags, 
		short name_index,
        short descriptor_index, 
		short attributes_count, 
		std::vector<byte>& attributes);	
	
    static std::vector<byte>& CodeAttribute(
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
        std::vector<byte>& attributes);
		
    /**
     * Make a class file.
     */

    static std::vector<byte>& ClassFile(
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
        std::vector<std::vector<byte>*>& attributes );

	static std::vector<byte>& ClassFile(std::vector<byte>& bytes, char* classname);

	static std::vector<byte>& Byte(std::vector<byte>& bytes, byte v) {
		bytes.push_back((byte)v);
		return bytes;
	}

	static std::vector<byte>& Short(std::vector<byte>& bytes, unsigned short v) {
		bytes.push_back((byte)(v>>8));
		bytes.push_back((byte)v);
		return bytes;
	}

    static std::vector<byte>& Int(std::vector<byte>& bytes, unsigned int v) {
		bytes.push_back((byte)(v>>24));
		bytes.push_back((byte)(v>>16));
		bytes.push_back((byte)(v>>8));
		bytes.push_back((byte)v);
		return bytes;
	}

    static int main(int argc, char* argv[]);
};

}
}


#endif // ByteCodeFactory_h

/*
 * Copyright (c) 1998-2001 by Mountain View Compiler Company
 * All rights reserved.
 */
