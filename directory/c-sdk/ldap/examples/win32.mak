#
# Copyright (c) 1996-1998.  Netscape Communications Corporation.  All
# rights reserved.
#
#
# Win32 GNU Makefile for Directory SDK examples
#
# build all the examples by typing:  gmake -f win32.mak
#

###############################################################################
# Note that the LDAP API library has different names for the SSL-enabled
# and non-SSL versions of the Directory SDK for C:  
# 
# o In the SSL-enabled version, the import library for Microsoft
#   compilers is named nsldapssl32v41.lib, and the DLL is named 
#   nsldapssl32v41.dll.
# 
# o In the non-SSL version, the import library for Microsoft
#   compilers is named nsldap32v41.lib, and the DLL is named
#   nsldap32v41.dll.
#
# Remove the comment character in front of the definition of
# LDAPLIB that matches the name of the LDAP API library for the
# version you've downloaded.
#


# For Win32 (NT)
LDAPLIB=nsldap32v41
#LDAPLIB=nsldapssl32v41
NSPRLIB=libnspr3
EXTRACFLAGS=-nologo -W3 -GT -GX -D_X86_ -Dx386 -DWIN32 -D_WINDOWS -c
EXTRALDFLAGS=/NOLOGO /PDB:NONE /DEBUGTYPE:BOTH /SUBSYSTEM:console


###############################################################################
# You should not need to change anything below here....

INCDIR=../include
LIBDIR=../lib
NSPRINCDIR=../include
NSPRLIBDIR=../lib

SYSLIBS=wsock32.lib kernel32.lib user32.lib shell32.lib
LIBS=$(LIBDIR)/$(LDAPLIB).lib $(NSPRLIBDIR)/$(NSPRLIB).lib

CC=cl
OPTFLAGS=-MD
CFLAGS=$(OPTFLAGS) $(EXTRACFLAGS) -I$(INCDIR) -I$(NSPRINCDIR)
LINK=link
LDFLAGS=$(EXTRALDFLAGS)


PROGS=search asearch csearch psearch rdentry getattrs srvrsort modattrs add del compare modrdn ppolicy getfilt crtfilt
EXES=$(addsuffix .exe,$(PROGS))

SSLPROGS=ssnoauth ssearch
SSLEXES=$(addsuffix .exe,$(SSLPROGS))

NSPRPROGS=nsprio
NSPREXES=$(addsuffix .exe,$(NSPRPROGS))

ALLEXES= $(EXES) $(SSLEXES) $(NSEXES)

standard:	$(EXES)

ssl:		$(SSLEXES)

nspr:		$(NSPREXES)

all:		$(ALLEXES)

search.obj:	examples.h

csearch.obj:	examples.h

psearch.obj:	examples.h

ssearch.obj:	examples.h

ssnoauth.obj:	examples.h

rdentry.obj:	examples.h

getattrs.obj:	examples.h

srvrsort.obj:	examples.h

modattrs.obj:	examples.h

asearch.obj:	examples.h

add.obj:	examples.h

del.obj:	examples.h

compare.obj:	examples.h

modrdn.obj:	examples.h

ppolicy.obj:	examples.h

getfilt.obj:	examples.h

crtfilt.obj:	examples.h

nsprio.obj:	examples.h

runall:		$(EXES)
		@for i in $(PROGS); do \
		    echo '-------------------------------------------------'; \
		    ./$$i; \
		done

clean:
		rm -f $(ALLEXES) *.obj

%.obj : %.c
		$(CC) $(CFLAGS) $< -Fo$@


%.exe : %.obj
		$(LINK) -OUT:$@ $(LDFLAGS) $(SYSLIBS) $< $(LIBS)
