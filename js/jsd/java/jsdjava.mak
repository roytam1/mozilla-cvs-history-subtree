
PROJ = jsdjava
JSDJAVA = .
JSD = $(JSDJAVA)\..
JSSRC = $(JSD)\..\src
OBJ = Debug

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od\
         /I $(JSSRC)\
         /I $(JSD)\
         /I jre\
         /I jre\win32\
         /DDEBUG /DWIN32 /DXP_PC /D_WINDOWS /D_WIN32\
         /DEXPORT_JSD_API\
         /DJSDEBUGGER\
!IF "$(JSD_STANDALONE_JAVA_VM)" != ""
         /DJSD_STANDALONE_JAVA_VM\
!ENDIF 
         /DJSD_THREADSAFE\
         /c /Fp$(OBJ)\$(PROJ).pch /Fd$(OBJ)\$(PROJ).pdb /YX -Fo$@ $<

LIBFLAGS = /nologo /subsystem:console /machine:I386 /out:$(OBJ)\$(PROJ).lib

CPP=cl.exe
LIB32=lib.exe

all: $(OBJ) $(OBJ)\$(PROJ).lib
     

$(OBJ)\$(PROJ).lib:         \
!IF "$(JSD_STANDALONE_JAVA_VM)" != ""
        $(OBJ)\jsd_jvm.obj  \
        $(OBJ)\jre.obj      \
        $(OBJ)\jre_md.obj   \
!ENDIF 
        $(OBJ)\jsdjava.obj  \
        $(OBJ)\jsd_jntv.obj
  $(LIB32) $(LIBFLAGS) $** $(LLIBS)

{$(JSDJAVA)}.c{$(OBJ)}.obj :
  $(CPP) $(CFLAGS)

{$(JSDJAVA)\jre}.c{$(OBJ)}.obj :
  $(CPP) $(CFLAGS)

{$(JSDJAVA)\jre\win32}.c{$(OBJ)}.obj :
  $(CPP) $(CFLAGS)

$(OBJ) :
    mkdir $(OBJ)

clean:
    del $(OBJ)\*.pch
    del $(OBJ)\*.obj
    del $(OBJ)\*.exp
    del $(OBJ)\*.lib
    del $(OBJ)\*.idb
    del $(OBJ)\*.pdb
    del $(OBJ)\*.exe
