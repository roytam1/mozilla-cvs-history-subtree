
#
# jband - 11/08/97 - 
#

#
# jband - 22 Sept 1998 - ripped out JSD stuff just to get this compiling
#

PROJ  = jsdraw
JSSRC = ..\..\SRC
OBJ = Debug
RUN = Run

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od\
         /I $(JSSRC)\
         /DDEBUG /DWIN32 /D_CONSOLE /DXP_PC /D_WINDOWS /D_WIN32\
         /c /Fp$(OBJ)\$(PROJ).pch /Fd$(OBJ)\$(PROJ).pdb /YX -Fo$@ $<

LFLAGS = /nologo /subsystem:windows /entry:WinMainCRTStartup /incremental:no\
         /machine:I386 /DEBUG\
         /pdb:$(OBJ)\$(PROJ).pdb -out:$(OBJ)\$(PROJ).exe

LLIBS = kernel32.lib advapi32.lib $(JSSRC)\Debug\js32.lib \
        user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib                                    

RCFLAGS = /r -DWIN32 -D_WIN32

CPP=cl.exe
LINK32=link.exe
RC=rc.exe

all: $(OBJ) $(RUN) $(RUN)\$(PROJ).exe $(RUN)\$(PROJ).pdb \
     $(RUN)\js32.dll $(RUN)\js32.pdb


$(OBJ)\$(PROJ).exe:         \
        $(OBJ)\jsdraw.obj   \
        $(OBJ)\maindlg.obj  \
        $(OBJ)\prompt.obj   \
        $(OBJ)\data.obj     \
        $(OBJ)\draw.obj     \
        $(OBJ)\jsdraw.res
  $(LINK32) $(LFLAGS) $** $(LLIBS)

$(OBJ)\$(PROJ).res: $(PROJ).rc resource.h

.rc{$(OBJ)}.res:
    $(RC) $(RCFLAGS) /fo$@  $<

.c{$(OBJ)}.obj:
  $(CPP) $(CFLAGS)

$(OBJ) :
    mkdir $(OBJ)

$(RUN) :
    mkdir $(RUN)

$(RUN)\$(PROJ).exe: $(OBJ)\$(PROJ).exe
    copy $(OBJ)\$(PROJ).exe $(RUN)

$(RUN)\$(PROJ).pdb: $(OBJ)\$(PROJ).pdb
    copy $(OBJ)\$(PROJ).pdb $(RUN)

$(RUN)\js32.dll :
    copy $(JSSRC)\Debug\js32.dll $(RUN)

$(RUN)\js32.pdb :
    copy $(JSSRC)\Debug\js32.pdb $(RUN)

clean:
    del $(OBJ)\*.pch
    del $(OBJ)\*.obj
    del $(OBJ)\*.exp
    del $(OBJ)\*.lib
    del $(OBJ)\*.res
    del $(OBJ)\*.idb
    del $(OBJ)\*.pdb
    del $(OBJ)\*.exe
    del $(RUN)\*.pdb
    del $(RUN)\*.exe




##
## jband - 11/08/97 - 
##
#
#TARGETOS = WINNT
#
#!include <win32.mak>
#
#all: jsdraw.exe
#
## Update the resources if necessary
#
#jsdraw.res: jsdraw.rc resource.h
#    $(rc) $(rcflags) $(rcvars) jsdraw.rc
#
#jsdraw.exe:         \
#        jsdraw.obj  \
#        maindlg.obj \
#        prompt.obj  \
#        data.obj    \
#        draw.obj    \
#        jsdraw.res
#  $(link) $(linkdebug) $(guiflags) -out:$*.exe $** ..\JSSRC\Debug\js32.lib $(guilibs)
#
#.c.obj:
#  $(cc) $(cdebug) $(cflags) $(cvars) /I..\JSSRC /YX $*.c
#
#clean:
#    del *.obj
#    del *.pch
#    del jsdraw.res
#    del jsdraw.exe
