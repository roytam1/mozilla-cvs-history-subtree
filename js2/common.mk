
BOEHM_DIR = $(top_srcdir)/../gc/boehm/
LIBBOEHM = $(BOEHM_DIR)/gc.a

JS2_DIR = $(top_srcdir)/src/
LIBJS2 = $(JS2_DIR)/libjs2.a

FDLIBM_DIR = $(top_srcdir)/../js/src/fdlibm/Linux_All_DBG.OBJ
LIBFDLIBM = $(FDLIBM_DIR)/libfdm.a

WFLAGS = -Wmissing-prototypes -Wstrict-prototypes -Wunused \
         -Wswitch


if DEBUG
CXXFLAGS = -DXP_UNIX -g -DDEBUG $(WFLAGS)
else
CXXFLAGS = -DXP_UNIX -O2 -Wuninitialized $(WFLAGS)
endif
