# Ldap library
LDAPVERS	= 50
LDAPVERS_SUFFIX = 5.0
LDAP_LIBNAME	= ldap$(LDAPVERS)

# PrLdap library
PRLDAPVERS      = 50
PRLDAPVERS_SUFFIX= 5.0
PRLDAP_LIBNAME	= prldap$(PRLDAPVERS)

# lber library
LBERVERS	= 50
LBERVERS_SUFFIX = 5.0
LBER_LIBNAME	= lber$(LBERVERS)

# ldif library
LDIFVERS	= 50
LDIFVERS_SUFFIX = 5.0
LDIF_LIBNAME	= ldif$(LDIFVERS)

# iutil library
IUTILVERS	= 50
IUTILVERS_SUFFIX = 5.0
IUTIL_LIBNAME	= iutil$(IUTILVERS)

# ssl library
SSLDAPVERS	= 50
SSLDAPVERS_SUFFIX = 5.0
SSLDAP_LIBNAME	= ssldap$(SSLDAPVERS)

# nss library
NSSVERS		= 3
NSS_LIBNAME	= nss$(NSSVERS)
SSL_LIBNAME	= ssl$(NSSVERS)
HYBRID_LIBNAME	= freebl_hybrid_$(NSSVERS)
PURE32_LIBNAME	= freebl_pure32_$(NSSVERS)

ifneq ($(USE_64), 1)
ifeq ($(OS_ARCH), SunOS)
COPYFREEBL      = 1
endif
ifeq ($(OS_ARCH), HPUX)
COPYFREEBL      = 1
endif
endif

# svrcore library
SVRCOREVERS	=
SVRCOREVERS_SUFFIX =
SVRCORE_LIBNAME	= svrcore$(SVRCOREVERS)

#
# NSPR library
#

NSPR_LIBVERSION=4
ifeq ($(OS_ARCH), WIN95)
PLC_BASENAME=plc$(NSPR_LIBVERSION)
PLDS_BASENAME=plds$(NSPR_LIBVERSION)
NSPR_BASENAME=nspr$(NSPR_LIBVERSION)
else
PLC_BASENAME=libplc$(NSPR_LIBVERSION)
PLDS_BASENAME=libplds$(NSPR_LIBVERSION)
NSPR_BASENAME=libnspr$(NSPR_LIBVERSION)
PLC_LIBNAME=plc$(NSPR_LIBVERSION)
PLDS_LIBNAME=plds$(NSPR_LIBVERSION)
NSPR_LIBNAME=nspr$(NSPR_LIBVERSION)
endif
PLCBASE=plc$(NSPR_LIBVERSION)
PLDSBASE=plds$(NSPR_LIBVERSION)
NSPRBASE=nspr$(NSPR_LIBVERSION)

NSPR_LIBNAMES =  $(PLC_BASENAME) $(PLDS_BASENAME) $(NSPR_BASENAME)
ifeq ($(INCLUDE_SSL),1)
ifeq ($(OS_ARCH), OSF1)
# override libplc library name for OSF/1 INCLUDE_SSL=1 (external) builds
# XXXmcs: for an explanation, scan down for:  XXXmcs: 27-Oct-1999
NSPR_LIBNAMES =  $(PLC_BASENAME)+ $(PLDS_BASENAME)+ $(NSPR_BASENAME)
endif
endif

DYNAMICNSPR = -l$(PLCBASE) -l$(PLDSBASE) -l$(NSPRBASE)
LIBNSPR_DLL = $(addsuffix .$(DLL_SUFFIX), $(addprefix $(NSCP_DISTDIR)/lib/, $(NSPR_LIBNAMES)))

LIBNSPR = $(addsuffix .$(LIB_SUFFIX), $(addprefix $(NSCP_DISTDIR)/lib/, $(NSPR_LIBNAMES)))
LIBNSPR_DEP = $(LIBNSPR)



RM              = rm -f
SED             = sed

#
# uncomment this line to enable support for LDAP referrals in libldap
#
LDAP_REFERRALS  = -DLDAP_REFERRALS


DEFNETSSL	= -DNET_SSL 
NOLIBLCACHE	= -DNO_LIBLCACHE

# uncomment if have access to libnls
#HAVELIBNLS     = -DHAVE_LIBNLS
#BUILDCLU	= 1

#
# DEFS are included in CFLAGS
#
DEFS            = $(PLATFORMCFLAGS) $(LDAP_DEBUG) $(HAVELIBNLS) \
                  $(CLDAP) $(DEFNETSSL) $(NOLIBLCACHE) \
                  $(LDAP_REFERRALS) $(LDAP_DNS) $(STR_TRANSLATION) \
                  $(LIBLDAP_CHARSETS) $(LIBLDAP_DEF_CHARSET)
ifneq ($(OS_ARCH), WINNT)
ifdef BUILD_OPT
EXTRACFLAGS=-O
else
EXTRACFLAGS=-g
endif
endif

ifeq ($(OS_ARCH), WINNT)
ifeq ($(DEBUG), full)
DSLDDEBUG=/debug
else
ifeq ($(DEBUG), purify)
DSLDDEBUG=/debug
endif
endif
PDBOPT=NONE
endif

# line to enable support for LDAP referrals in libldap
#
LDAP_REFERRALS  = -DLDAP_REFERRALS

#
# Web server dynamic library.
#
ifeq ($(OS_ARCH), WINNT)

DLLEXPORTS_PREFIX=/DEF:

else
ifeq ($(OS_ARCH), SunOS)

DLLEXPORTS_PREFIX=-Blocal -M

else
ifeq ($(OS_ARCH), IRIX)

DLLEXPORTS_PREFIX=-exports_file

else
ifeq ($(OS_ARCH),HPUX)

else
ifeq ($(OS_ARCH),AIX)

DLLEXPORTS_PREFIX=-bE:
DL=-ldl

else
ifeq ($(OS_ARCH),OSF1)

DL=

ifeq ($(OS_ARCH), Linux)

DL=-ldl

else
ifeq ($(OS_ARCH),ReliantUNIX)

DL=-ldl

else
ifeq ($(OS_ARCH),UnixWare)

DL=

else

#default


endif # UnixWare
endif # ReliantUNIX
endif # Linux
endif # OSF1
endif # AIX
endif # HPUX
endif # IRIX
endif # SOLARIS
endif # WINNT


RPATHFLAG = ..:../lib:../../lib:../../../lib:../../../../lib

ifeq ($(OS_ARCH), SunOS)
# flag to pass to cc when linking to set runtime shared library search path
# this is used like this, for example:   $(RPATHFLAG_PREFIX)../..
RPATHFLAG_PREFIX=-Wl,-R,

# flag to pass to ld when linking to set runtime shared library search path
# this is used like this, for example:   $(LDRPATHFLAG_PREFIX)../..
LDRPATHFLAG_PREFIX=-R
endif

ifeq ($(OS_ARCH), OSF1)
# flag to pass to cc when linking to set runtime shared library search path
# this is used like this, for example:   $(RPATHFLAG_PREFIX)../..
RPATHFLAG_PREFIX=-Wl,-rpath,

# flag to pass to ld when linking to set runtime shared library search path
# this is used like this, for example:   $(LDRPATHFLAG_PREFIX)../..
LDRPATHFLAG_PREFIX=-rpath
endif # OSF1

ifeq ($(OS_ARCH), AIX)
# Flags to set runtime shared library search path.  For example:
# $(CC) $(RPATHFLAG_PREFIX)../..$(RPATHFLAG_EXTRAS)
RPATHFLAG_PREFIX=-blibpath:
RPATHFLAG_EXTRAS=:/usr/lib:/lib

# flag to pass to ld when linking to set runtime shared library search path
# this is used like this, for example:   $(LDRPATHFLAG_PREFIX)../..
LDRPATHFLAG_PREFIX=-blibpath:/usr/lib:/lib:
DLL_LDFLAGS= -bM:SRE -bnoentry \
    -L.:/usr/lib/threads:/usr/lpp/xlC/lib:/usr/lib:/lib
DLL_EXTRA_LIBS= -bI:/usr/lib/lowsys.exp -lC_r -lC -lpthreads -lc_r -lm \
    /usr/lib/libc.a

EXE_EXTRA_LIBS= -bI:/usr/lib/syscalls.exp -lsvld -lpthreads
endif # AIX

ifeq ($(OS_ARCH), HPUX)
# flag to pass to cc when linking to set runtime shared library search path
# this is used like this, for example:   $(RPATHFLAG_PREFIX)../..
RPATHFLAG_PREFIX=-Wl,+s,+b,

# flag to pass to ld when linking to set runtime shared library search path
# this is used like this, for example:   $(LDRPATHFLAG_PREFIX)../..
LDRPATHFLAG_PREFIX=+s +b
# we need to link in the rt library to get sem_*()
PLATFORMLIBS += -lV3 -lrt

PLATFORMCFLAGS= -Dhpux -D$(PLATFORM) -D_HPUX_SOURCE -D_REENTRANT -Aa
endif # HPUX

ifeq ($(OS_ARCH), Linux)
# flag to pass to cc when linking to set runtime shared library search path
# this is used like this, for example:   $(RPATHFLAG_PREFIX)../..
RPATHFLAG_PREFIX=-Wl,-rpath,

# flag to pass to ld when linking to set runtime shared library search path
# this is used like this, for example:   $(LDRPATHFLAG_PREFIX)../..
# note, there is a trailing space
LDRPATHFLAG_PREFIX=-rpath
endif

#
# XXX: does anyone know of a better way to solve the "LINK_LIB2" problem? -mcs
#
# Link to produce a console/windows exe on Windows
#

ifeq ($(OS_ARCH), WINNT)
LINK_EXE        = link -OUT:"$@" /MAP $(ALDFLAGS) $(LDFLAGS) $(ML_DEBUG) \
    $(LCFLAGS) /NOLOGO /PDB:NONE /DEBUGTYPE:BOTH /INCREMENTAL:NO \
    /SUBSYSTEM:$(SUBSYSTEM) $(DEPLIBS) $(EXTRA_LIBS) $(OBJS)
LINK_EXE_NOLIBSOBJS     = link -OUT:"$@" /MAP $(ALDFLAGS) $(LDFLAGS) \
    $(ML_DEBUG) $(LCFLAGS) /NOLOGO /PDB:NONE /DEBUGTYPE:BOTH /INCREMENTAL:NO \
    /SUBSYSTEM:$(SUBSYSTEM)
LINK_LIB        = lib -OUT:"$@"  $(OBJS)
LINK_DLL        = link /nologo /MAP /DLL /PDB:NONE /DEBUGTYPE:BOTH \
        $(ML_DEBUG) /SUBSYSTEM:WINDOWS $(LLFLAGS) $(DLL_LDFLAGS) \
        $(EXTRA_LIBS) /out:"$@" $(OBJS)
else # WINNT
#
# UNIX link commands
#
LINK_LIB        = $(RM) $@; $(AR) $(OBJS); $(RANLIB) $@
LINK_LIB2       = $(RM) $@; $(AR) $@ $(OBJS2); $(RANLIB) $@
ifdef SONAMEFLAG_PREFIX
LINK_DLL        = $(LD) $(DSO_LDOPTS) $(ALDFLAGS) $(DLL_LDFLAGS) $(DLL_EXPORT_FLAGS) \
                        -o $@ $(SONAMEFLAG_PREFIX)$(notdir $@) $(OBJS)
else # SONAMEFLAG_PREFIX
LINK_DLL        = $(LD) $(ALDFLAGS) $(DLL_LDFLAGS) $(DLL_EXPORT_FLAGS) \
                        -o $@ $(OBJS)
endif # SONAMEFLAG_PREFIX

ifeq ($(OS_ARCH), OSF1)
# The linker on OSF/1 gets confused if it finds an so_locations file
# that doesn't meet its expectations, so we arrange to remove it before
# linking.
SO_FILES_TO_REMOVE=so_locations
endif

ifeq ($(OS_ARCH), HPUX)
# On HPUX, we need a couple of changes:
# 1) Use the C++ compiler for linking, which will pass the +eh flag on down to t
he
#    linker so the correct exception-handling-aware libC gets used (libnshttpd.s
l
#    needs this).
# 2) Add a "-Wl,-E" option so the linker gets a "-E" flag.  This makes symbols
#    in an executable visible to shared libraries loaded at runtime.
LINK_EXE        = $(CXX) -Wl,-E $(ALDFLAGS) $(LDFLAGS) $(RPATHFLAG_PREFIX)$(RPAT
HFLAG) \
        -o $@ $(OBJS) $(EXTRA_LIBS)
LINK_EXE_NOLIBSOBJS     = $(CXX) -Wl,-E $(ALDFLAGS) $(LDFLAGS) \
        $(RPATHFLAG_PREFIX)$(RPATHFLAG) -o $@
LINK_EXE_NOLIBSOBJS_NOCXX       = $(CC) -Wl,-E $(ALDFLAGS) $(LDFLAGS) \
        $(RPATHFLAG_PREFIX)$(RPATHFLAG) -o $@
LINK_EXE_NOCXX = $(CC) -Wl,-E $(ALDFLAGS) $(LDFLAGS) $(RPATHFLAG_PREFIX)$(RPATHF
LAG) \
        -o $@ $(OBJS) $(EXTRA_LIBS)

else # HPUX
# everything except HPUX
ifeq ($(OS_ARCH), ReliantUNIX)
# Use the C++ compiler for linking if at least ONE object is C++
export LD_RUN_PATH=$(RPATHFLAG)
LINK_EXE      = $(CXX) $(ALDFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(EXTRA_LIBS)
LINK_EXE_NOLIBSOBJS   = $(CXX) $(ALDFLAGS) $(LDFLAGS) -o $@
LINK_EXE_NOLIBSOBJS_NOCXX     = $(CC) $(ALDFLAGS) $(LDFLAGS) -o $@
LINK_EXE_NOCXX = $(CC) $(ALDFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(EXTRA_LIBS)

else # ReliantUNIX
ifdef USE_LD_RUN_PATH
#does RPATH differently.  instead we export RPATHFLAG as LD_RUN_PATH
#see ns/netsite/ldap/clients/tools/Makefile for an example
export LD_RUN_PATH=$(RPATHFLAG)
LINK_EXE        = $(CC) $(ALDFLAGS) $(LDFLAGS) \
                        -o $@ $(OBJS) $(EXTRA_LIBS)
LINK_EXE_NOLIBSOBJS     =  $(CC) $(ALDFLAGS) $(LDFLAGS) -o $@
else # USE_LD_RUN_PATH
LINK_EXE        = $(CC) $(ALDFLAGS) $(LDFLAGS) \
                        $(RPATHFLAG_PREFIX)$(RPATHFLAG)$(RPATHFLAG_EXTRAS) \
                        -o $@ $(OBJS) $(EXTRA_LIBS)
LINK_EXE_NOLIBSOBJS     = $(CC) $(ALDFLAGS) $(LDFLAGS) \
                        $(RPATHFLAG_PREFIX)$(RPATHFLAG)$(RPATHFLAG_EXTRAS) -o $@
endif # USE_LD_RUN_PATH
endif # ReliantUNIX
endif # HPUX
endif # WINNT


PERL = perl
#
# shared library symbol export definitions
#
ifeq ($(OS_ARCH), WINNT)
GENEXPORTS=cmd /c  $(PERL) $(LDAP_SRC)/build/genexports.pl
else
GENEXPORTS=$(PERL) $(LDAP_SRC)/build/genexports.pl
endif

