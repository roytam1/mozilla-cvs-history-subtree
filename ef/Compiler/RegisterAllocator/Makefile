#! gmake

DEPTH			= ../..

MODULE_NAME		= RegisterAllocator

include $(DEPTH)/config/config.mk

INCLUDES		+=										\
	-I$(DEPTH)/Utilities/General						\
	-I$(DEPTH)/Utilities/zlib							\
	-I$(DEPTH)/Runtime/ClassReader						\
	-I$(DEPTH)/Runtime/NativeMethods					\
	-I$(DEPTH)/Runtime/System							\
	-I$(DEPTH)/Runtime/ClassInfo						\
	-I$(DEPTH)/Runtime/FileReader						\
	-I$(DEPTH)/Compiler/PrimitiveGraph					\
	-I$(DEPTH)/Compiler/FrontEnd						\
	-I$(DEPTH)/Compiler/Optimizer						\
	-I$(DEPTH)/Compiler/CodeGenerator					\
	-I$(DEPTH)/Compiler/CodeGenerator/md				\
	-I$(DEPTH)/Compiler/CodeGenerator/md/$(CPU_ARCH)	\
	-I$(DEPTH)/Compiler/RegisterAllocator				\
	-I$(DEPTH)/Driver/StandAloneJava					\
	-I$(DEPTH)/Debugger									\
	$(NULL)

CXXSRCS 		=										\
	RegisterAllocator.cpp								\
	RegisterAllocatorTools.cpp							\
	DominatorGraph.cpp									\
	VirtualRegister.cpp									\
	BitSet.cpp											\
	SparseSet.cpp										\
	$(NULL)


include $(DEPTH)/config/rules.mk

libs:: $(MODULE)
