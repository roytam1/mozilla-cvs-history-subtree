
include $(top_srcdir)/common.mk

INCLUDES = -I$(BOEHM_DIR)

noinst_LIBRARIES = libjs2.a

# can't add this to the library, clients will need to link to gc.a
# libjs2_a_LIBADD = $(LIBBOEHM)

libjs2_a_DEPENDENCIES = $(LIBBOEHM)
libjs2_a_SOURCES = \
	bytecodegen.cpp \
	exception.cpp \
	formatter.cpp \
	hash.cpp \
	js2runtime.cpp \
	lexer.cpp \
	mem.cpp \
	numerics.cpp \
	parser.cpp \
	reader.cpp \
	strings.cpp \
	token.cpp \
	utilities.cpp \
	world.cpp
