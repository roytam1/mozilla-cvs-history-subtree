
include $(top_srcdir)/common.mk

INCLUDES = -I$(JS2_DIR)
LIBS = -L$(JS2_DIR) -ljs2

bin_PROGRAMS = parser_test js2_shell

parser_test_DEPENDENCIES = $(LIBJS2)
parser_test_SOURCES = parser_test.cpp

js2_shell_DEPENDENCIES = $(LIBJS2)
js2_shell_SOURCES = DikDik_Shell.cpp
