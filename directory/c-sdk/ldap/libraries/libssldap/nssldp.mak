#
#  Windows Makefile for LDAP-over-SSL piece of LDAP Client SDK.
#
#  This makefile produces:
#	16-bit		32-bit
#	------		------
#	nssldap.dll	nssldap32.dll	LDAP-over-SSL DLL
#	nssldap.lib	nssldap32.lib	LDAP-over-SSL import library
#
#  This makefile works the same way as MOZILLA.MAK
#
#  Setting in your environment or the command line will determine what you 
#   build.  
#
#   MOZ_SRC      place holding the ns tree.  Default = 'y:'
#   MOZ_OUT      place you would like output to go.  Default = '.\'
#   MOZ_DEBUG    if defined, you are building debug
#   MOZ_BITS     set to 16 to build Win16, defaults to Win32.
#   MOZ_SEC      set to DOMESTIC for 128 US, defaults to EXPORT.
# also
#   LDAP_SRC     place holding the ldap tree.  Default = '$(MOZ_SRC)\ns\netsite
#   MSVC4	 if defined, you are using Visual C++ 4.x tools, 
#		 else you are using Visual C++ 2.* tools
#   WINVER	 set to 3.51 or 4.0 for NT. Default 3.51.
#
#  In order to build, you first have to build dependencies. Build dependencies
#   by:
#
#       nmake -f nssldp.mak DEPEND=1 MOZ_DEBUG=1
#   
#
#  Once dependencies are built, you can build by:
#
#       nmake -f nssldp.mak MOZ_DEBUG=1
#

.SUFFIXES: .cpp .c .rc

!if !defined(MOZ_SRC)
MOZ_SRC=y:
!endif

!if !defined(LDAP_SRC)
LDAP_SRC=$(MOZ_SRC)\ns\netsite
!endif

!if !defined(MOZ_OUT)
MOZ_OUT=. 
!endif

!if !defined(MOZ_INT)
MOZ_INT=$(MOZ_OUT)
!endif

!if !defined(MOZ_BITS)
MOZ_BITS=32
!else
!endif

!if "$(MOZ_BITS)"=="32"
LIB_BITS=32
!endif

!if !defined(MOZ_SEC)
MOZ_SEC=EXPORT
!endif

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!if !defined(WINVER)
!if "$(NTVERSION)"==""
WINVER=3.51
!else
WINVER=$(NTVERSION)
!endif
!endif

!if defined(MOZ_DEBUG)
BUILD_FLAVOR=DBG
!else
BUILD_FLAVOR=OPT
!endif

CPP=cl.exe /nologo
MTL=mktyplib.exe /nologo
LIB_MAN= \
!if "$(MOZ_BITS)"=="32"
    link.exe -lib /nologo
!else
    lib.exe /nologo
!endif

#
# Add different product values here, like dec alpha, mips etc, win16...
#
!if "$(MOZ_BITS)"=="32"
PROD=x86
!else
PROD=16x86
!endif

#
# Get compiler version info right
#
!if "$(PROD)"=="x86"
# Intel 32 bit can use version 2 or 4
!if defined(MSVC4)
# using Visual C++ 4.*
!if defined(MOZ_DEBUG)
C_RUNTIME=msvcrtd.lib
# To produce a PDB, add /Zi /Gm /Gi to CFLAGS_DEBUG, remove /Z7
CFLAGS_DEBUG=/Z7 /DDEBUG /D_DEBUG /MDd /Od /Gy
!else
C_RUNTIME=msvcrt.lib
!endif
!else
# using Visual C++ 2.*
C_RUNTIME=msvcrt.lib
!if defined(MOZ_DEBUG)
CFLAGS_DEBUG=/Z7 /DDEBUG /D_DEBUG /MD /Od /Gy
!endif
!endif
!endif

#
#       Should reflect non debug settings always,
#               regardless if CFLAGS_DEBUG is doing
#               so also.
#       This is so 16 bits can compile only portions desired
#               as debug (and still link).
#
CFLAGS_RELEASE=/DNDEBUG /MD /Ox

LIBFLAGS_DEBUG= \
!if "$(MOZ_BITS)"=="32"
    /nologo
!endif

!if defined(MOZ_DEBUG)
VERSTR=Dbg
DIST=$(MOZ_SRC)\ns\dist\WIN$(MOZ_BITS)_D.OBJ
RCFLAGS_DEBUG=/DDEBUG /D_DEBUG
!else
VERSTR=Rel
DIST=$(MOZ_SRC)\ns\dist\WIN$(MOZ_BITS)_O.OBJ
CFLAGS_DEBUG=$(CFLAGS_RELEASE)
RCFLAGS_DEBUG=/DNODEBUG
!endif


#
#       Edit these in order to control 16 bit
#               debug targets.
#
CFLAGS_LIBSSLDAP_C=\
!if "$(MOZ_BITS)"=="32"
    $(CFLAGS_DEBUG)
!else
    $(CFLAGS_RELEASE)
!endif
CFLAGS_WINSOCK_C=\
!if "$(MOZ_BITS)"=="32"
    $(CFLAGS_DEBUG)
!else
    $(CFLAGS_RELEASE)
!endif
CFLAGS_LIBUTIL_C=\
!if "$(MOZ_BITS)"=="32"
    $(CFLAGS_DEBUG)
!else
    $(CFLAGS_RELEASE)
!endif

OUTDIR=$(MOZ_OUT)\$(PROD)$(VERSTR)

LIB_FLAGS= \
    $(LIBFLAGS_DEBUG) \
	/out:"$(OUTDIR)\nssldap$(LIB_BITS).lib" 

CFLAGS_GENERAL=/c /W3 /Fo"$(OUTDIR)/" /Fd"$(OUTDIR)/" \
!if "$(MOZ_BITS)"=="32"
    /GX 
!else
# phil check for Win16
    /G3 /AL /Gt3 /Gx- /GA /GEf  \
!if defined(MOZ_DEBUG)
    /Zd /Od
!else
    /Gs
!endif
!endif

RCFLAGS_GENERAL= \
!if "$(MOZ_BITS)"=="32"
    /l 0x409
!else
    /r
!endif

DIST_INCLUDE=$(MOZ_SRC)\ns\dist\winnt$(WINVER)_$(BUILD_FLAVOR).obj\include 

CINCLUDES= \
    /I$(MOZ_SRC)\ns\include \
    /I$(MOZ_SRC)\ns\nspr20\pr\include \
    /I$(LDAP_SRC)\ldap\include \
    /I$(DIST_INCLUDE) \
    /I$(DIST_INCLUDE)\nspr20\pr

RCINCLUDES=$(LDAP_SRC)\ldap\include

CDEFINES= \
	/D_WINDOWS \
	/DLDAP_LDBM \
!if defined(MOZ_DEBUG)
	/DLDAP_DEBUG \
	/D_DEBUG \
!endif
	/DNEEDPROTOS \
	/DLDBM_USE_DBBTREE \
!if "$(MOZ_BITS)" == "32"
	/D_WIN32 \
	/D_XP_WIN32 \
!endif
	/DNET_SSL \
	/DUSE_NSPR_MT \
	/DNSPR \
	/DXP_WIN \
	/DXP_PC \
	/DLDAP_SSLIO_HOOKS

RCDEFINES= 

CFILEFLAGS=$(CFLAGS_GENERAL) ^
    $(CDEFINES) ^
    $(CINCLUDES) ^

RCFILEFLAGS=$(RCFLAGS_GENERAL)\
    $(RCFLAGS_DEBUG)\
    $(RCDEFINES)


!ifdef DEPEND

#
# Build dependencies step
#
all: "$(OUTDIR)" $(OUTDIR)\makedep.exe $(OUTDIR)\nssldap.dep

$(OUTDIR)\nssldap.dep: $(LDAP_SRC)\ldap\libraries\libssldap\nssldp.mak
	@rem <<$(PROD)$(VERSTR).dep
	$(CINCLUDES) -O $(OUTDIR)\nssldap.dep
<<
	$(OUTDIR)\makedep @$(PROD)$(VERSTR).dep -F <<
		$(LDAP_SRC)\ldap\libraries\libssldap\clientinit.c
		$(LDAP_SRC)\ldap\libraries\libssldap\dongle.c
		$(LDAP_SRC)\ldap\libraries\libssldap\ldapsinit.c
<<

$(OUTDIR)\makedep.exe: $(MOZ_SRC)\ns\cmd\winfe\mkfiles32\makedep.cpp
!if "$(MOZ_BITS)"=="32"
    @cl -MT -Fo"$(OUTDIR)/" -Fe"$(OUTDIR)/makedep.exe" $(MOZ_SRC)\ns\cmd\winfe\mkfiles32\makedep.cpp
!else
    cl @<<
	/Gs
	/G2
	/Mq
	/W3
	/AL
	/Ox
	/D "NDEBUG"
	/D "_WINDOWS"
	/Fo"$(OUTDIR)\makedep.obj"
	/Fe"$(OUTDIR)\makedep.exe"
	/link lafxcw
	/link oldnames
	/link libw
	/link llibcewq
	$(MOZ_SRC)\ns\cmd\winfe\mkfiles32\makedep.cpp
<<
!endif

!endif 

!ifdef EXPORT

#
# Export to DIST step
#
all : "$(DIST)\include\ldap" install

$(DIST)\include\ldap :
	if not exist "$(DIST)\include\ldap/$(NULL)" mkdir "$(DIST)\include\ldap"

install : \
	$(DIST)\lib\nssldap$(LIB_BITS).lib \
	$(DIST)\include\ldap\lber.h \
	$(DIST)\include\ldap\proto-lber.h \
	$(DIST)\include\ldap\ldap.h \
	$(DIST)\include\ldap\proto-ldap.h \
	$(DIST)\include\ldap\disptmpl.h \

$(DIST)\lib\nssldap$(LIB_BITS).lib : $(OUTDIR)\nssldap$(LIB_BITS).lib
	copy $(OUTDIR)\nssldap$(LIB_BITS).lib $(DIST)\lib\nssldap$(LIB_BITS).lib

$(DIST)\include\ldap\lber.h : $(LDAP_SRC)\ldap\include\lber.h
	copy $(LDAP_SRC)\ldap\include\lber.h $(DIST)\include\ldap\lber.h

$(DIST)\include\ldap\proto-lber.h : $(LDAP_SRC)\ldap\include\proto-lber.h
	copy $(LDAP_SRC)\ldap\include\proto-lber.h $(DIST)\include\ldap\proto-lber.h

$(DIST)\include\ldap\ldap.h : $(LDAP_SRC)\ldap\include\ldap.h
	copy $(LDAP_SRC)\ldap\include\ldap.h $(DIST)\include\ldap\ldap.h

$(DIST)\include\ldap\proto-ldap.h : $(LDAP_SRC)\ldap\include\proto-ldap.h
	copy $(LDAP_SRC)\ldap\include\proto-ldap.h $(DIST)\include\ldap\proto-ldap.h

$(DIST)\include\ldap\disptmpl.h : $(LDAP_SRC)\ldap\include\disptmpl.h
	copy $(LDAP_SRC)\ldap\include\disptmpl.h $(DIST)\include\ldap\disptmpl.h

!endif

!if !defined(EXPORT) && !defined(DEPEND)

#
# Normal build step
#
all : $(OUTDIR)\nssldap.dep "$(OUTDIR)" $(OUTDIR)\nssldap$(LIB_BITS).lib

# Allow makefile to work without dependencies generated.
!if exist("$(OUTDIR)\nssldap.dep")
!include "$(OUTDIR)\nssldap.dep"
!endif

!endif

# 
# utilities needed for more than one build step
#
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(OUTDIR)\nssldap$(LIB_BITS).lib" : "$(OUTDIR)" $(OBJ_FILES)
!if "$(MOZ_BITS)"=="16"
    @if EXIST $(OUTDIR)\makedep.obj del $(OUTDIR)\makedep.obj
!endif
    $(LIB_MAN) @<<
    $(LIB_FLAGS) $(DEF_FLAGS) $(OBJ_FILES)
<<

#
# Build rules
#

{$(LDAP_SRC)\ldap\libraries\libssldap}.c{$(OUTDIR)}.obj:
   @rem <<$(PROD)$(VERSTR).cl
 $(CFILEFLAGS)
<<
   $(CPP) @$(PROD)$(VERSTR).cl $(CFLAGS_LIBSSLDAP_C) $(CFLAGS) %s

