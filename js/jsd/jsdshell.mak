
PROJ = jsdshell
JSD = .
JSDJAVA = $(JSD)\JAVA
JSSRC = $(JSD)\..\src
OBJ = Debug
RUN = Run

JSDEBUGGER_JAVA_UI = 1

JSSRCPROJ   = js32
JSSRCOBJ    = $(JSSRC)\$(OBJ)
JSDPROJ     = jsd
JSDOBJ      = $(OBJ)
JSDJAVAPROJ = jsdjava
JSDJAVAOBJ  = $(JSDJAVA)\$(OBJ)

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od\
         /I $(JSSRC)\
         /I $(JSD)\
         /I $(JSDJAVA)\
         /DDEBUG /DWIN32 /D_CONSOLE /DXP_PC /D_WINDOWS /D_WIN32\
         /DJSDEBUGGER\
!IF "$(JSDEBUGGER_JAVA_UI)" != ""
         /DJSDEBUGGER_JAVA_UI\
         /DJSD_STANDALONE_JAVA_VM\
!ENDIF 
         /DJSD_LOWLEVEL_SOURCE\
         /DJSFILE\
         /c /Fp$(OBJ)\$(PROJ).pch /Fd$(OBJ)\$(PROJ).pdb /YX -Fo$@ $<

LFLAGS = /nologo /subsystem:console /incremental:no /machine:I386 /DEBUG\
         /pdb:$(OBJ)\$(PROJ).pdb -out:$(OBJ)\$(PROJ).exe

LLIBS = kernel32.lib advapi32.lib \
        $(JSSRCOBJ)\$(JSSRCPROJ).lib \
        $(OBJ)\$(JSDPROJ).lib \
        $(JSDJAVAOBJ)\$(JSDJAVAPROJ).lib

CPP=cl.exe
LINK32=link.exe

all: $(OBJ) $(RUN) \
     $(RUN)\$(JSSRCPROJ).dll $(RUN)\$(JSSRCPROJ).pdb \
     $(RUN)\$(JSDPROJ).dll $(RUN)\$(JSDPROJ).pdb \
     $(JSDJAVAOBJ)\$(JSDJAVAPROJ).lib $(RUN)\$(JSDJAVAPROJ).pdb \
     $(RUN)\$(PROJ).exe $(RUN)\$(PROJ).pdb

$(OBJ)\$(PROJ).exe:         \
        $(OBJ)\js.obj
  $(LINK32) $(LFLAGS) $** $(LLIBS)

$(JSSRCOBJ)\$(JSSRCPROJ).pdb : $(JSSRCOBJ)\$(JSSRCPROJ).dll

$(JSSRCOBJ)\$(JSSRCPROJ).lib : $(JSSRCOBJ)\$(JSSRCPROJ).dll

$(JSSRCOBJ)\$(JSSRCPROJ).dll : 
    @cd ..\src
    @nmake -f js.mak CFG="js - Win32 Debug"
    @cd ..\jsd

$(JSDOBJ)\$(JSDPROJ).pdb : $(JSDOBJ)\$(JSDPROJ).dll

$(JSDOBJ)\$(JSDPROJ).lib : $(JSDOBJ)\$(JSDPROJ).dll

$(JSDOBJ)\$(JSDPROJ).dll : 
    @nmake -f jsd.mak JSD_THREADSAFE=1

$(JSDJAVAOBJ)\$(JSDJAVAPROJ).pdb : $(JSDJAVAOBJ)\$(JSDJAVAPROJ).lib

$(JSDJAVAOBJ)\$(JSDJAVAPROJ).lib : 
    @cd java
    @nmake -f jsdjava.mak JSD_STANDALONE_JAVA_VM=1
    @cd ..

{$(JSSRC)}.c{$(OBJ)}.obj :
  $(CPP) $(CFLAGS)

$(OBJ) :
    mkdir $(OBJ)

$(RUN) :
    mkdir $(RUN)

$(RUN)\$(PROJ).exe: $(OBJ)\$(PROJ).exe
    copy $(OBJ)\$(PROJ).exe $(RUN)

$(RUN)\$(PROJ).pdb: $(OBJ)\$(PROJ).pdb
    copy $(OBJ)\$(PROJ).pdb $(RUN)

$(RUN)\$(JSSRCPROJ).dll : $(JSSRCOBJ)\$(JSSRCPROJ).dll
    copy $(JSSRCOBJ)\$(JSSRCPROJ).dll $(RUN)

$(RUN)\$(JSSRCPROJ).pdb : $(JSSRCOBJ)\$(JSSRCPROJ).pdb
    copy $(JSSRCOBJ)\$(JSSRCPROJ).pdb $(RUN)

$(RUN)\$(JSDPROJ).dll : $(JSDOBJ)\$(JSDPROJ).dll
    copy $(JSDOBJ)\$(JSDPROJ).dll $(RUN)

$(RUN)\$(JSDPROJ).pdb : $(JSDOBJ)\$(JSDPROJ).pdb
    copy $(JSDOBJ)\$(JSDPROJ).pdb $(RUN)

$(RUN)\$(JSDJAVAPROJ).pdb : $(JSDJAVAOBJ)\$(JSDJAVAPROJ).pdb
    copy $(JSDJAVAOBJ)\$(JSDJAVAPROJ).pdb $(RUN)


clean:
    del $(OBJ)\*.pch
    del $(OBJ)\*.obj
    del $(OBJ)\*.exp
    del $(OBJ)\*.lib
    del $(OBJ)\*.dll
    del $(OBJ)\*.idb
    del $(OBJ)\*.pdb
    del $(OBJ)\*.exe
    del $(RUN)\*.pdb
    del $(RUN)\*.exe
    del $(RUN)\*.dll

deep_clean: clean
    @cd ..\src
    @nmake -f js.mak CFG="js - Win32 Debug" clean
    @cd ..\jsd
    @nmake -f jsd.mak clean
    @cd java
    @nmake -f jsdjava.mak clean
    @cd ..
