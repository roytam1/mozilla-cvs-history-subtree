ifdef USE_MSVC
LIB_OBJS  = $(addprefix $(OBJDIR)/, $(LIB_CFILES:.c=.obj))
PROG_OBJS = $(addprefix $(OBJDIR)/, $(PROG_CFILES:.c=.obj))
else
LIB_OBJS  = $(addprefix $(OBJDIR)/, $(LIB_CFILES:.c=.o))
LIB_OBJS  += $(addprefix $(OBJDIR)/, $(LIB_ASFILES:.s=.o))
PROG_OBJS = $(addprefix $(OBJDIR)/, $(PROG_CFILES:.c=.o))
endif

CFILES = $(LIB_CFILES) $(PROG_CFILES)
OBJS   = $(LIB_OBJS) $(PROG_OBJS)

ifdef USE_MSVC
# TARGETS = $(LIBRARY)   # $(PROGRAM) not supported for MSVC yet
TARGETS = $(OBJDIR)/$(FDLIBM_LIBRARY) $(SHARED_LIBRARY) $(PROGRAM)  # it is now
else
TARGETS = $(OBJDIR)/$(FDLIBM_LIBRARY) $(LIBRARY) $(SHARED_LIBRARY) $(PROGRAM) 
endif

all:	$(TARGETS)
	+$(LOOP_OVER_DIRS)

$(OBJDIR)/%: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ $(CFLAGS) $*.c $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $*.c

$(OBJDIR)/%.o: %.s
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) $*.s

# windows only
$(OBJDIR)/%.obj: %.c
	@$(MAKE_OBJDIR)
	$(CC) -Fo$(OBJDIR)/ -c $(CFLAGS) $(JSDLL_CFLAGS) $*.c

$(OBJDIR)/js.obj: js.c
	@$(MAKE_OBJDIR)
	$(CC) -Fo$(OBJDIR)/ -c $(CFLAGS) $<

ifeq ($(OS_ARCH),OS2)
$(LIBRARY): $(LIB_OBJS)
	$(AR) $@ $? $(AR_OS2_SUFFIX)
	$(RANLIB) $@
else
ifdef USE_MSVC
$(SHARED_LIBRARY): $(LIB_OBJS)
	link.exe fdlibm/$(OBJDIR)/fdlibm.lib $(LIB_LINK_FLAGS) /base:0x61000000 $(OTHER_LIBS) \
	    /out:"$@" /pdb:"$(OBJDIR)/$(@F:.dll=.pdb)" \
	    /implib:"$(OBJDIR)/$(@F:.dll=.lib)" $?
else
$(LIBRARY): $(LIB_OBJS)
	$(AR) rv $@ $?
	$(RANLIB) $@

$(SHARED_LIBRARY): $(LIB_OBJS)
	$(MKSHLIB) -o $@ $(LIB_OBJS) $(LDFLAGS) $(OTHER_LIBS)
endif
endif

$(OBJDIR)/$(FDLIBM_LIBRARY): fdlibm/$(OBJDIR)/$(FDLIBM_LIBRARY)
	@$(MAKE_OBJDIR)
	cp fdlibm/$(OBJDIR)/$(FDLIBM_LIBRARY) $(OBJDIR)/$(FDLIBM_LIBRARY)

fdlibm/$(OBJDIR)/$(FDLIBM_LIBRARY):
	cd fdlibm; gmake -f Makefile.ref

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir $(@D); fi
endef

ifdef DIRS
LOOP_OVER_DIRS		=					\
	@for d in $(DIRS); do					\
		if test -d $$d; then				\
			set -e;			\
			echo "cd $$d; $(MAKE) -f Makefile.ref $@"; 		\
			cd $$d; $(MAKE) -f Makefile.ref $@; cd ..;	\
			set +e;					\
		else						\
			echo "Skipping non-directory $$d...";	\
		fi;						\
	done
endif

export:	
	$(INSTALL) -m 644 $(HFILES) $(DIST)/include
	$(INSTALL) -m 755 $(LIBRARY) $(DIST)/lib
	$(INSTALL) -m 755 $(SHARED_LIBRARY) $(DIST)/lib
	$(INSTALL) -m 755 $(PROGRAM) $(DIST)/bin
	+$(LOOP_OVER_DIRS)

clean:
	rm -rf $(OBJS)
	cd fdlibm; gmake -f Makefile.ref clean


clobber:
	rm -rf $(OBJS) $(TARGETS) $(DEPENDENCIES)
	cd fdlibm; gmake -f Makefile.ref clobber

depend:
	gcc -MM $(CFLAGS) $(LIB_CFILES)

tar:
	tar cvf $(TARNAME) $(TARFILES)
	gzip $(TARNAME)

