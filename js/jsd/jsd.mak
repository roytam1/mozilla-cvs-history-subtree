
PROJ = jsd
JSD = .
OBJ = Debug
RUN = Run
JSSRC = $(JSD)\..\src
JSSRCPROJ = js32
JSSRCOBJ = $(JSSRC)\$(OBJ)

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od\
         /I $(JSSRC)\
         /I $(JSD)\
         /DDEBUG /DWIN32 /D_CONSOLE /DXP_PC /D_WINDOWS /D_WIN32\
         /DJSDEBUGGER\
!IF "$(JSD_THREADSAFE)" != ""
         /DJSD_THREADSAFE\
!ENDIF 
         /DEXPORT_JSD_API\
         /c /Fp$(OBJ)\$(PROJ).pch /Fd$(OBJ)\$(PROJ).pdb /YX -Fo$@ $<

LFLAGS = /nologo /subsystem:console /DLL /incremental:no /machine:I386 /DEBUG\
         /pdb:$(OBJ)\$(PROJ).pdb -out:$(OBJ)\$(PROJ).dll

LLIBS = kernel32.lib advapi32.lib $(JSSRCOBJ)\$(JSSRCPROJ).lib
# unused... user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib                                    

CPP=cl.exe
LINK32=link.exe

all: $(OBJ) $(OBJ)\$(PROJ).dll
     

$(OBJ)\$(PROJ).dll:         \
        $(OBJ)\jsdebug.obj  \
        $(OBJ)\jsd_high.obj \
        $(OBJ)\jsd_hook.obj \
        $(OBJ)\jsd_scpt.obj \
        $(OBJ)\jsd_stak.obj \
        $(OBJ)\jsd_step.obj \
        $(OBJ)\jsd_text.obj \
        $(OBJ)\jsd_lock.obj \
        $(OBJ)\jsd_val.obj 
  $(LINK32) $(LFLAGS) $** $(LLIBS)

{$(JSD)}.c{$(OBJ)}.obj :
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
    del $(OBJ)\*.dll
