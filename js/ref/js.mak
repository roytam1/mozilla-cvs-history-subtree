# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=fdlibm - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to fdlibm - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "jsshell - Win32 Release" && "$(CFG)" !=\
 "jsshell - Win32 Debug" && "$(CFG)" != "js32 - Win32 Release" && "$(CFG)" !=\
 "js32 - Win32 Debug" && "$(CFG)" != "fdlibm - Win32 Release" && "$(CFG)" !=\
 "fdlibm - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "js.mak" CFG="fdlibm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jsshell - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "jsshell - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "js32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "js32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fdlibm - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "fdlibm - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "fdlibm - Win32 Debug"

!IF  "$(CFG)" == "jsshell - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "jsshell_"
# PROP BASE Intermediate_Dir "jsshell_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "js32 - Win32 Release" "$(OUTDIR)\jsshell.exe"

CLEAN : 
	-@erase "$(INTDIR)\js.obj"
	-@erase "$(OUTDIR)\jsshell.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_CONSOLE" /D "_WIN32" /D "WIN32" /D "XP_PC" /D "_WINDOWS" /D "JSFILE" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_CONSOLE" /D "_WIN32" /D\
 "WIN32" /D "XP_PC" /D "_WINDOWS" /D "JSFILE" /Fp"$(INTDIR)/js.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/js.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"Release\jsshell.exe"
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/jsshell.pdb" /machine:I386 /out:"$(OUTDIR)/jsshell.exe" 
LINK32_OBJS= \
	"$(INTDIR)\js.obj" \
	"$(OUTDIR)\js32.lib"

"$(OUTDIR)\jsshell.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "jsshell - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "jsshell0"
# PROP BASE Intermediate_Dir "jsshell0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "js32 - Win32 Debug" "$(OUTDIR)\jsshell.exe"

CLEAN : 
	-@erase "$(INTDIR)\js.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\jsshell.exe"
	-@erase "$(OUTDIR)\jsshell.ilk"
	-@erase "$(OUTDIR)\jsshell.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "_CONSOLE" /D "_WIN32" /D "WIN32" /D "DEBUG" /D "XP_PC" /D "_WINDOWS" /D "JSFILE" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "_CONSOLE" /D "_WIN32"\
 /D "WIN32" /D "DEBUG" /D "XP_PC" /D "_WINDOWS" /D "JSFILE"\
 /Fp"$(INTDIR)/js.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/js.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug\jsshell.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/jsshell.pdb" /debug /machine:I386 /out:"$(OUTDIR)/jsshell.exe" 
LINK32_OBJS= \
	"$(INTDIR)\js.obj" \
	"$(OUTDIR)\js32.lib"

"$(OUTDIR)\jsshell.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "js32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "js32\Release"
# PROP BASE Intermediate_Dir "js32\Release"
# PROP BASE Target_Dir "js32"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir "js32"
OUTDIR=.\Release
INTDIR=.\Release

ALL : "fdlibm - Win32 Release" "$(OUTDIR)\js32.dll"

CLEAN : 
	-@erase "$(INTDIR)\jsapi.obj"
	-@erase "$(INTDIR)\jsarray.obj"
	-@erase "$(INTDIR)\jsatom.obj"
	-@erase "$(INTDIR)\jsbool.obj"
	-@erase "$(INTDIR)\jscntxt.obj"
	-@erase "$(INTDIR)\jsdate.obj"
	-@erase "$(INTDIR)\jsdbgapi.obj"
	-@erase "$(INTDIR)\jsemit.obj"
	-@erase "$(INTDIR)\jsfun.obj"
	-@erase "$(INTDIR)\jsgc.obj"
	-@erase "$(INTDIR)\jsinterp.obj"
	-@erase "$(INTDIR)\jslock.obj"
	-@erase "$(INTDIR)\jsmath.obj"
	-@erase "$(INTDIR)\jsnum.obj"
	-@erase "$(INTDIR)\jsobj.obj"
	-@erase "$(INTDIR)\jsopcode.obj"
	-@erase "$(INTDIR)\jsparse.obj"
	-@erase "$(INTDIR)\jsregexp.obj"
	-@erase "$(INTDIR)\jsscan.obj"
	-@erase "$(INTDIR)\jsscope.obj"
	-@erase "$(INTDIR)\jsscript.obj"
	-@erase "$(INTDIR)\jsstr.obj"
	-@erase "$(INTDIR)\jsxdrapi.obj"
	-@erase "$(INTDIR)\prarena.obj"
	-@erase "$(INTDIR)\prassert.obj"
	-@erase "$(INTDIR)\prdtoa.obj"
	-@erase "$(INTDIR)\prhash.obj"
	-@erase "$(INTDIR)\prlog2.obj"
	-@erase "$(INTDIR)\prlong.obj"
	-@erase "$(INTDIR)\prprintf.obj"
	-@erase "$(INTDIR)\prtime.obj"
	-@erase "$(OUTDIR)\js32.dll"
	-@erase "$(OUTDIR)\js32.exp"
	-@erase "$(OUTDIR)\js32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "XP_PC" /D "JSFILE" /D "EXPORT_JS_API" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_WIN32" /D\
 "WIN32" /D "XP_PC" /D "JSFILE" /D "EXPORT_JS_API" /Fp"$(INTDIR)/js32.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/js32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt"
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/js32.pdb" /machine:I386 /nodefaultlib:"libcmt"\
 /out:"$(OUTDIR)/js32.dll" /implib:"$(OUTDIR)/js32.lib" 
LINK32_OBJS= \
	"$(INTDIR)\jsapi.obj" \
	"$(INTDIR)\jsarray.obj" \
	"$(INTDIR)\jsatom.obj" \
	"$(INTDIR)\jsbool.obj" \
	"$(INTDIR)\jscntxt.obj" \
	"$(INTDIR)\jsdate.obj" \
	"$(INTDIR)\jsdbgapi.obj" \
	"$(INTDIR)\jsemit.obj" \
	"$(INTDIR)\jsfun.obj" \
	"$(INTDIR)\jsgc.obj" \
	"$(INTDIR)\jsinterp.obj" \
	"$(INTDIR)\jslock.obj" \
	"$(INTDIR)\jsmath.obj" \
	"$(INTDIR)\jsnum.obj" \
	"$(INTDIR)\jsobj.obj" \
	"$(INTDIR)\jsopcode.obj" \
	"$(INTDIR)\jsparse.obj" \
	"$(INTDIR)\jsregexp.obj" \
	"$(INTDIR)\jsscan.obj" \
	"$(INTDIR)\jsscope.obj" \
	"$(INTDIR)\jsscript.obj" \
	"$(INTDIR)\jsstr.obj" \
	"$(INTDIR)\jsxdrapi.obj" \
	"$(INTDIR)\prarena.obj" \
	"$(INTDIR)\prassert.obj" \
	"$(INTDIR)\prdtoa.obj" \
	"$(INTDIR)\prhash.obj" \
	"$(INTDIR)\prlog2.obj" \
	"$(INTDIR)\prlong.obj" \
	"$(INTDIR)\prprintf.obj" \
	"$(INTDIR)\prtime.obj" \
	".\fdlibm\Release\fdlibm.lib"

"$(OUTDIR)\js32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "js32\Debug"
# PROP BASE Intermediate_Dir "js32\Debug"
# PROP BASE Target_Dir "js32"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir "js32"
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "fdlibm - Win32 Debug" "$(OUTDIR)\js32.dll" "$(OUTDIR)\js32.bsc"

CLEAN : 
	-@erase "$(INTDIR)\jsapi.obj"
	-@erase "$(INTDIR)\jsapi.sbr"
	-@erase "$(INTDIR)\jsarray.obj"
	-@erase "$(INTDIR)\jsarray.sbr"
	-@erase "$(INTDIR)\jsatom.obj"
	-@erase "$(INTDIR)\jsatom.sbr"
	-@erase "$(INTDIR)\jsbool.obj"
	-@erase "$(INTDIR)\jsbool.sbr"
	-@erase "$(INTDIR)\jscntxt.obj"
	-@erase "$(INTDIR)\jscntxt.sbr"
	-@erase "$(INTDIR)\jsdate.obj"
	-@erase "$(INTDIR)\jsdate.sbr"
	-@erase "$(INTDIR)\jsdbgapi.obj"
	-@erase "$(INTDIR)\jsdbgapi.sbr"
	-@erase "$(INTDIR)\jsemit.obj"
	-@erase "$(INTDIR)\jsemit.sbr"
	-@erase "$(INTDIR)\jsfun.obj"
	-@erase "$(INTDIR)\jsfun.sbr"
	-@erase "$(INTDIR)\jsgc.obj"
	-@erase "$(INTDIR)\jsgc.sbr"
	-@erase "$(INTDIR)\jsinterp.obj"
	-@erase "$(INTDIR)\jsinterp.sbr"
	-@erase "$(INTDIR)\jslock.obj"
	-@erase "$(INTDIR)\jslock.sbr"
	-@erase "$(INTDIR)\jsmath.obj"
	-@erase "$(INTDIR)\jsmath.sbr"
	-@erase "$(INTDIR)\jsnum.obj"
	-@erase "$(INTDIR)\jsnum.sbr"
	-@erase "$(INTDIR)\jsobj.obj"
	-@erase "$(INTDIR)\jsobj.sbr"
	-@erase "$(INTDIR)\jsopcode.obj"
	-@erase "$(INTDIR)\jsopcode.sbr"
	-@erase "$(INTDIR)\jsparse.obj"
	-@erase "$(INTDIR)\jsparse.sbr"
	-@erase "$(INTDIR)\jsregexp.obj"
	-@erase "$(INTDIR)\jsregexp.sbr"
	-@erase "$(INTDIR)\jsscan.obj"
	-@erase "$(INTDIR)\jsscan.sbr"
	-@erase "$(INTDIR)\jsscope.obj"
	-@erase "$(INTDIR)\jsscope.sbr"
	-@erase "$(INTDIR)\jsscript.obj"
	-@erase "$(INTDIR)\jsscript.sbr"
	-@erase "$(INTDIR)\jsstr.obj"
	-@erase "$(INTDIR)\jsstr.sbr"
	-@erase "$(INTDIR)\jsxdrapi.obj"
	-@erase "$(INTDIR)\jsxdrapi.sbr"
	-@erase "$(INTDIR)\prarena.obj"
	-@erase "$(INTDIR)\prarena.sbr"
	-@erase "$(INTDIR)\prassert.obj"
	-@erase "$(INTDIR)\prassert.sbr"
	-@erase "$(INTDIR)\prdtoa.obj"
	-@erase "$(INTDIR)\prdtoa.sbr"
	-@erase "$(INTDIR)\prhash.obj"
	-@erase "$(INTDIR)\prhash.sbr"
	-@erase "$(INTDIR)\prlog2.obj"
	-@erase "$(INTDIR)\prlog2.sbr"
	-@erase "$(INTDIR)\prlong.obj"
	-@erase "$(INTDIR)\prlong.sbr"
	-@erase "$(INTDIR)\prprintf.obj"
	-@erase "$(INTDIR)\prprintf.sbr"
	-@erase "$(INTDIR)\prtime.obj"
	-@erase "$(INTDIR)\prtime.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\js32.bsc"
	-@erase "$(OUTDIR)\js32.dll"
	-@erase "$(OUTDIR)\js32.exp"
	-@erase "$(OUTDIR)\js32.ilk"
	-@erase "$(OUTDIR)\js32.lib"
	-@erase "$(OUTDIR)\js32.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "DEBUG" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "XP_PC" /D "JSFILE" /D "EXPORT_JS_API" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "DEBUG" /D "_WINDOWS"\
 /D "_WIN32" /D "WIN32" /D "XP_PC" /D "JSFILE" /D "EXPORT_JS_API"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/js32.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/js32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\jsapi.sbr" \
	"$(INTDIR)\jsarray.sbr" \
	"$(INTDIR)\jsatom.sbr" \
	"$(INTDIR)\jsbool.sbr" \
	"$(INTDIR)\jscntxt.sbr" \
	"$(INTDIR)\jsdate.sbr" \
	"$(INTDIR)\jsdbgapi.sbr" \
	"$(INTDIR)\jsemit.sbr" \
	"$(INTDIR)\jsfun.sbr" \
	"$(INTDIR)\jsgc.sbr" \
	"$(INTDIR)\jsinterp.sbr" \
	"$(INTDIR)\jslock.sbr" \
	"$(INTDIR)\jsmath.sbr" \
	"$(INTDIR)\jsnum.sbr" \
	"$(INTDIR)\jsobj.sbr" \
	"$(INTDIR)\jsopcode.sbr" \
	"$(INTDIR)\jsparse.sbr" \
	"$(INTDIR)\jsregexp.sbr" \
	"$(INTDIR)\jsscan.sbr" \
	"$(INTDIR)\jsscope.sbr" \
	"$(INTDIR)\jsscript.sbr" \
	"$(INTDIR)\jsstr.sbr" \
	"$(INTDIR)\jsxdrapi.sbr" \
	"$(INTDIR)\prarena.sbr" \
	"$(INTDIR)\prassert.sbr" \
	"$(INTDIR)\prdtoa.sbr" \
	"$(INTDIR)\prhash.sbr" \
	"$(INTDIR)\prlog2.sbr" \
	"$(INTDIR)\prlong.sbr" \
	"$(INTDIR)\prprintf.sbr" \
	"$(INTDIR)\prtime.sbr"

"$(OUTDIR)\js32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcmtd"
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/js32.pdb" /debug /machine:I386 /nodefaultlib:"libcmtd"\
 /out:"$(OUTDIR)/js32.dll" /implib:"$(OUTDIR)/js32.lib" 
LINK32_OBJS= \
	"$(INTDIR)\jsapi.obj" \
	"$(INTDIR)\jsarray.obj" \
	"$(INTDIR)\jsatom.obj" \
	"$(INTDIR)\jsbool.obj" \
	"$(INTDIR)\jscntxt.obj" \
	"$(INTDIR)\jsdate.obj" \
	"$(INTDIR)\jsdbgapi.obj" \
	"$(INTDIR)\jsemit.obj" \
	"$(INTDIR)\jsfun.obj" \
	"$(INTDIR)\jsgc.obj" \
	"$(INTDIR)\jsinterp.obj" \
	"$(INTDIR)\jslock.obj" \
	"$(INTDIR)\jsmath.obj" \
	"$(INTDIR)\jsnum.obj" \
	"$(INTDIR)\jsobj.obj" \
	"$(INTDIR)\jsopcode.obj" \
	"$(INTDIR)\jsparse.obj" \
	"$(INTDIR)\jsregexp.obj" \
	"$(INTDIR)\jsscan.obj" \
	"$(INTDIR)\jsscope.obj" \
	"$(INTDIR)\jsscript.obj" \
	"$(INTDIR)\jsstr.obj" \
	"$(INTDIR)\jsxdrapi.obj" \
	"$(INTDIR)\prarena.obj" \
	"$(INTDIR)\prassert.obj" \
	"$(INTDIR)\prdtoa.obj" \
	"$(INTDIR)\prhash.obj" \
	"$(INTDIR)\prlog2.obj" \
	"$(INTDIR)\prlong.obj" \
	"$(INTDIR)\prprintf.obj" \
	"$(INTDIR)\prtime.obj" \
	".\fdlibm\Debug\fdlibm.lib"

"$(OUTDIR)\js32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fdlibm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "fdlibm\fdlibm_0"
# PROP BASE Intermediate_Dir "fdlibm\fdlibm_0"
# PROP BASE Target_Dir "fdlibm"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "fdlibm\Release"
# PROP Intermediate_Dir "fdlibm\Release"
# PROP Target_Dir "fdlibm"
OUTDIR=.\fdlibm\Release
INTDIR=.\fdlibm\Release

ALL : "$(OUTDIR)\fdlibm.lib"

CLEAN : 
	-@erase "$(INTDIR)\e_acos.obj"
	-@erase "$(INTDIR)\e_acosh.obj"
	-@erase "$(INTDIR)\e_asin.obj"
	-@erase "$(INTDIR)\e_atan2.obj"
	-@erase "$(INTDIR)\e_atanh.obj"
	-@erase "$(INTDIR)\e_cosh.obj"
	-@erase "$(INTDIR)\e_exp.obj"
	-@erase "$(INTDIR)\e_fmod.obj"
	-@erase "$(INTDIR)\e_gamma.obj"
	-@erase "$(INTDIR)\e_gamma_r.obj"
	-@erase "$(INTDIR)\e_hypot.obj"
	-@erase "$(INTDIR)\e_j0.obj"
	-@erase "$(INTDIR)\e_j1.obj"
	-@erase "$(INTDIR)\e_jn.obj"
	-@erase "$(INTDIR)\e_lgamma.obj"
	-@erase "$(INTDIR)\e_lgamma_r.obj"
	-@erase "$(INTDIR)\e_log.obj"
	-@erase "$(INTDIR)\e_log10.obj"
	-@erase "$(INTDIR)\e_pow.obj"
	-@erase "$(INTDIR)\e_rem_pio2.obj"
	-@erase "$(INTDIR)\e_remainder.obj"
	-@erase "$(INTDIR)\e_scalb.obj"
	-@erase "$(INTDIR)\e_sinh.obj"
	-@erase "$(INTDIR)\e_sqrt.obj"
	-@erase "$(INTDIR)\k_cos.obj"
	-@erase "$(INTDIR)\k_rem_pio2.obj"
	-@erase "$(INTDIR)\k_sin.obj"
	-@erase "$(INTDIR)\k_standard.obj"
	-@erase "$(INTDIR)\k_tan.obj"
	-@erase "$(INTDIR)\s_asinh.obj"
	-@erase "$(INTDIR)\s_atan.obj"
	-@erase "$(INTDIR)\s_cbrt.obj"
	-@erase "$(INTDIR)\s_ceil.obj"
	-@erase "$(INTDIR)\s_copysign.obj"
	-@erase "$(INTDIR)\s_cos.obj"
	-@erase "$(INTDIR)\s_erf.obj"
	-@erase "$(INTDIR)\s_expm1.obj"
	-@erase "$(INTDIR)\s_fabs.obj"
	-@erase "$(INTDIR)\s_finite.obj"
	-@erase "$(INTDIR)\s_floor.obj"
	-@erase "$(INTDIR)\s_frexp.obj"
	-@erase "$(INTDIR)\s_ilogb.obj"
	-@erase "$(INTDIR)\s_isnan.obj"
	-@erase "$(INTDIR)\s_ldexp.obj"
	-@erase "$(INTDIR)\s_lib_version.obj"
	-@erase "$(INTDIR)\s_log1p.obj"
	-@erase "$(INTDIR)\s_logb.obj"
	-@erase "$(INTDIR)\s_matherr.obj"
	-@erase "$(INTDIR)\s_modf.obj"
	-@erase "$(INTDIR)\s_nextafter.obj"
	-@erase "$(INTDIR)\s_rint.obj"
	-@erase "$(INTDIR)\s_scalbn.obj"
	-@erase "$(INTDIR)\s_signgam.obj"
	-@erase "$(INTDIR)\s_significand.obj"
	-@erase "$(INTDIR)\s_sin.obj"
	-@erase "$(INTDIR)\s_tan.obj"
	-@erase "$(INTDIR)\s_tanh.obj"
	-@erase "$(INTDIR)\w_acos.obj"
	-@erase "$(INTDIR)\w_acosh.obj"
	-@erase "$(INTDIR)\w_asin.obj"
	-@erase "$(INTDIR)\w_atan2.obj"
	-@erase "$(INTDIR)\w_atanh.obj"
	-@erase "$(INTDIR)\w_cosh.obj"
	-@erase "$(INTDIR)\w_exp.obj"
	-@erase "$(INTDIR)\w_fmod.obj"
	-@erase "$(INTDIR)\w_gamma.obj"
	-@erase "$(INTDIR)\w_gamma_r.obj"
	-@erase "$(INTDIR)\w_hypot.obj"
	-@erase "$(INTDIR)\w_j0.obj"
	-@erase "$(INTDIR)\w_j1.obj"
	-@erase "$(INTDIR)\w_jn.obj"
	-@erase "$(INTDIR)\w_lgamma.obj"
	-@erase "$(INTDIR)\w_lgamma_r.obj"
	-@erase "$(INTDIR)\w_log.obj"
	-@erase "$(INTDIR)\w_log10.obj"
	-@erase "$(INTDIR)\w_pow.obj"
	-@erase "$(INTDIR)\w_remainder.obj"
	-@erase "$(INTDIR)\w_scalb.obj"
	-@erase "$(INTDIR)\w_sinh.obj"
	-@erase "$(INTDIR)\w_sqrt.obj"
	-@erase "$(OUTDIR)\fdlibm.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/fdlibm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\fdlibm\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/fdlibm.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /nodefaultlib:"libc"
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/fdlibm.lib" /nodefaultlib:"libc" 
LIB32_OBJS= \
	"$(INTDIR)\e_acos.obj" \
	"$(INTDIR)\e_acosh.obj" \
	"$(INTDIR)\e_asin.obj" \
	"$(INTDIR)\e_atan2.obj" \
	"$(INTDIR)\e_atanh.obj" \
	"$(INTDIR)\e_cosh.obj" \
	"$(INTDIR)\e_exp.obj" \
	"$(INTDIR)\e_fmod.obj" \
	"$(INTDIR)\e_gamma.obj" \
	"$(INTDIR)\e_gamma_r.obj" \
	"$(INTDIR)\e_hypot.obj" \
	"$(INTDIR)\e_j0.obj" \
	"$(INTDIR)\e_j1.obj" \
	"$(INTDIR)\e_jn.obj" \
	"$(INTDIR)\e_lgamma.obj" \
	"$(INTDIR)\e_lgamma_r.obj" \
	"$(INTDIR)\e_log.obj" \
	"$(INTDIR)\e_log10.obj" \
	"$(INTDIR)\e_pow.obj" \
	"$(INTDIR)\e_rem_pio2.obj" \
	"$(INTDIR)\e_remainder.obj" \
	"$(INTDIR)\e_scalb.obj" \
	"$(INTDIR)\e_sinh.obj" \
	"$(INTDIR)\e_sqrt.obj" \
	"$(INTDIR)\k_cos.obj" \
	"$(INTDIR)\k_rem_pio2.obj" \
	"$(INTDIR)\k_sin.obj" \
	"$(INTDIR)\k_standard.obj" \
	"$(INTDIR)\k_tan.obj" \
	"$(INTDIR)\s_asinh.obj" \
	"$(INTDIR)\s_atan.obj" \
	"$(INTDIR)\s_cbrt.obj" \
	"$(INTDIR)\s_ceil.obj" \
	"$(INTDIR)\s_copysign.obj" \
	"$(INTDIR)\s_cos.obj" \
	"$(INTDIR)\s_erf.obj" \
	"$(INTDIR)\s_expm1.obj" \
	"$(INTDIR)\s_fabs.obj" \
	"$(INTDIR)\s_finite.obj" \
	"$(INTDIR)\s_floor.obj" \
	"$(INTDIR)\s_frexp.obj" \
	"$(INTDIR)\s_ilogb.obj" \
	"$(INTDIR)\s_isnan.obj" \
	"$(INTDIR)\s_ldexp.obj" \
	"$(INTDIR)\s_lib_version.obj" \
	"$(INTDIR)\s_log1p.obj" \
	"$(INTDIR)\s_logb.obj" \
	"$(INTDIR)\s_matherr.obj" \
	"$(INTDIR)\s_modf.obj" \
	"$(INTDIR)\s_nextafter.obj" \
	"$(INTDIR)\s_rint.obj" \
	"$(INTDIR)\s_scalbn.obj" \
	"$(INTDIR)\s_signgam.obj" \
	"$(INTDIR)\s_significand.obj" \
	"$(INTDIR)\s_sin.obj" \
	"$(INTDIR)\s_tan.obj" \
	"$(INTDIR)\s_tanh.obj" \
	"$(INTDIR)\w_acos.obj" \
	"$(INTDIR)\w_acosh.obj" \
	"$(INTDIR)\w_asin.obj" \
	"$(INTDIR)\w_atan2.obj" \
	"$(INTDIR)\w_atanh.obj" \
	"$(INTDIR)\w_cosh.obj" \
	"$(INTDIR)\w_exp.obj" \
	"$(INTDIR)\w_fmod.obj" \
	"$(INTDIR)\w_gamma.obj" \
	"$(INTDIR)\w_gamma_r.obj" \
	"$(INTDIR)\w_hypot.obj" \
	"$(INTDIR)\w_j0.obj" \
	"$(INTDIR)\w_j1.obj" \
	"$(INTDIR)\w_jn.obj" \
	"$(INTDIR)\w_lgamma.obj" \
	"$(INTDIR)\w_lgamma_r.obj" \
	"$(INTDIR)\w_log.obj" \
	"$(INTDIR)\w_log10.obj" \
	"$(INTDIR)\w_pow.obj" \
	"$(INTDIR)\w_remainder.obj" \
	"$(INTDIR)\w_scalb.obj" \
	"$(INTDIR)\w_sinh.obj" \
	"$(INTDIR)\w_sqrt.obj"

"$(OUTDIR)\fdlibm.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "fdlibm\fdlibm_0"
# PROP BASE Intermediate_Dir "fdlibm\fdlibm_0"
# PROP BASE Target_Dir "fdlibm"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "fdlibm\Debug"
# PROP Intermediate_Dir "fdlibm\Debug"
# PROP Target_Dir "fdlibm"
OUTDIR=.\fdlibm\Debug
INTDIR=.\fdlibm\Debug

ALL : "$(OUTDIR)\fdlibm.lib"

CLEAN : 
	-@erase "$(INTDIR)\e_acos.obj"
	-@erase "$(INTDIR)\e_acosh.obj"
	-@erase "$(INTDIR)\e_asin.obj"
	-@erase "$(INTDIR)\e_atan2.obj"
	-@erase "$(INTDIR)\e_atanh.obj"
	-@erase "$(INTDIR)\e_cosh.obj"
	-@erase "$(INTDIR)\e_exp.obj"
	-@erase "$(INTDIR)\e_fmod.obj"
	-@erase "$(INTDIR)\e_gamma.obj"
	-@erase "$(INTDIR)\e_gamma_r.obj"
	-@erase "$(INTDIR)\e_hypot.obj"
	-@erase "$(INTDIR)\e_j0.obj"
	-@erase "$(INTDIR)\e_j1.obj"
	-@erase "$(INTDIR)\e_jn.obj"
	-@erase "$(INTDIR)\e_lgamma.obj"
	-@erase "$(INTDIR)\e_lgamma_r.obj"
	-@erase "$(INTDIR)\e_log.obj"
	-@erase "$(INTDIR)\e_log10.obj"
	-@erase "$(INTDIR)\e_pow.obj"
	-@erase "$(INTDIR)\e_rem_pio2.obj"
	-@erase "$(INTDIR)\e_remainder.obj"
	-@erase "$(INTDIR)\e_scalb.obj"
	-@erase "$(INTDIR)\e_sinh.obj"
	-@erase "$(INTDIR)\e_sqrt.obj"
	-@erase "$(INTDIR)\k_cos.obj"
	-@erase "$(INTDIR)\k_rem_pio2.obj"
	-@erase "$(INTDIR)\k_sin.obj"
	-@erase "$(INTDIR)\k_standard.obj"
	-@erase "$(INTDIR)\k_tan.obj"
	-@erase "$(INTDIR)\s_asinh.obj"
	-@erase "$(INTDIR)\s_atan.obj"
	-@erase "$(INTDIR)\s_cbrt.obj"
	-@erase "$(INTDIR)\s_ceil.obj"
	-@erase "$(INTDIR)\s_copysign.obj"
	-@erase "$(INTDIR)\s_cos.obj"
	-@erase "$(INTDIR)\s_erf.obj"
	-@erase "$(INTDIR)\s_expm1.obj"
	-@erase "$(INTDIR)\s_fabs.obj"
	-@erase "$(INTDIR)\s_finite.obj"
	-@erase "$(INTDIR)\s_floor.obj"
	-@erase "$(INTDIR)\s_frexp.obj"
	-@erase "$(INTDIR)\s_ilogb.obj"
	-@erase "$(INTDIR)\s_isnan.obj"
	-@erase "$(INTDIR)\s_ldexp.obj"
	-@erase "$(INTDIR)\s_lib_version.obj"
	-@erase "$(INTDIR)\s_log1p.obj"
	-@erase "$(INTDIR)\s_logb.obj"
	-@erase "$(INTDIR)\s_matherr.obj"
	-@erase "$(INTDIR)\s_modf.obj"
	-@erase "$(INTDIR)\s_nextafter.obj"
	-@erase "$(INTDIR)\s_rint.obj"
	-@erase "$(INTDIR)\s_scalbn.obj"
	-@erase "$(INTDIR)\s_signgam.obj"
	-@erase "$(INTDIR)\s_significand.obj"
	-@erase "$(INTDIR)\s_sin.obj"
	-@erase "$(INTDIR)\s_tan.obj"
	-@erase "$(INTDIR)\s_tanh.obj"
	-@erase "$(INTDIR)\w_acos.obj"
	-@erase "$(INTDIR)\w_acosh.obj"
	-@erase "$(INTDIR)\w_asin.obj"
	-@erase "$(INTDIR)\w_atan2.obj"
	-@erase "$(INTDIR)\w_atanh.obj"
	-@erase "$(INTDIR)\w_cosh.obj"
	-@erase "$(INTDIR)\w_exp.obj"
	-@erase "$(INTDIR)\w_fmod.obj"
	-@erase "$(INTDIR)\w_gamma.obj"
	-@erase "$(INTDIR)\w_gamma_r.obj"
	-@erase "$(INTDIR)\w_hypot.obj"
	-@erase "$(INTDIR)\w_j0.obj"
	-@erase "$(INTDIR)\w_j1.obj"
	-@erase "$(INTDIR)\w_jn.obj"
	-@erase "$(INTDIR)\w_lgamma.obj"
	-@erase "$(INTDIR)\w_lgamma_r.obj"
	-@erase "$(INTDIR)\w_log.obj"
	-@erase "$(INTDIR)\w_log10.obj"
	-@erase "$(INTDIR)\w_pow.obj"
	-@erase "$(INTDIR)\w_remainder.obj"
	-@erase "$(INTDIR)\w_scalb.obj"
	-@erase "$(INTDIR)\w_sinh.obj"
	-@erase "$(INTDIR)\w_sqrt.obj"
	-@erase "$(OUTDIR)\fdlibm.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gi- /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gi- /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/fdlibm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\fdlibm\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/fdlibm.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /nodefaultlib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/fdlibm.lib" /nodefaultlib 
LIB32_OBJS= \
	"$(INTDIR)\e_acos.obj" \
	"$(INTDIR)\e_acosh.obj" \
	"$(INTDIR)\e_asin.obj" \
	"$(INTDIR)\e_atan2.obj" \
	"$(INTDIR)\e_atanh.obj" \
	"$(INTDIR)\e_cosh.obj" \
	"$(INTDIR)\e_exp.obj" \
	"$(INTDIR)\e_fmod.obj" \
	"$(INTDIR)\e_gamma.obj" \
	"$(INTDIR)\e_gamma_r.obj" \
	"$(INTDIR)\e_hypot.obj" \
	"$(INTDIR)\e_j0.obj" \
	"$(INTDIR)\e_j1.obj" \
	"$(INTDIR)\e_jn.obj" \
	"$(INTDIR)\e_lgamma.obj" \
	"$(INTDIR)\e_lgamma_r.obj" \
	"$(INTDIR)\e_log.obj" \
	"$(INTDIR)\e_log10.obj" \
	"$(INTDIR)\e_pow.obj" \
	"$(INTDIR)\e_rem_pio2.obj" \
	"$(INTDIR)\e_remainder.obj" \
	"$(INTDIR)\e_scalb.obj" \
	"$(INTDIR)\e_sinh.obj" \
	"$(INTDIR)\e_sqrt.obj" \
	"$(INTDIR)\k_cos.obj" \
	"$(INTDIR)\k_rem_pio2.obj" \
	"$(INTDIR)\k_sin.obj" \
	"$(INTDIR)\k_standard.obj" \
	"$(INTDIR)\k_tan.obj" \
	"$(INTDIR)\s_asinh.obj" \
	"$(INTDIR)\s_atan.obj" \
	"$(INTDIR)\s_cbrt.obj" \
	"$(INTDIR)\s_ceil.obj" \
	"$(INTDIR)\s_copysign.obj" \
	"$(INTDIR)\s_cos.obj" \
	"$(INTDIR)\s_erf.obj" \
	"$(INTDIR)\s_expm1.obj" \
	"$(INTDIR)\s_fabs.obj" \
	"$(INTDIR)\s_finite.obj" \
	"$(INTDIR)\s_floor.obj" \
	"$(INTDIR)\s_frexp.obj" \
	"$(INTDIR)\s_ilogb.obj" \
	"$(INTDIR)\s_isnan.obj" \
	"$(INTDIR)\s_ldexp.obj" \
	"$(INTDIR)\s_lib_version.obj" \
	"$(INTDIR)\s_log1p.obj" \
	"$(INTDIR)\s_logb.obj" \
	"$(INTDIR)\s_matherr.obj" \
	"$(INTDIR)\s_modf.obj" \
	"$(INTDIR)\s_nextafter.obj" \
	"$(INTDIR)\s_rint.obj" \
	"$(INTDIR)\s_scalbn.obj" \
	"$(INTDIR)\s_signgam.obj" \
	"$(INTDIR)\s_significand.obj" \
	"$(INTDIR)\s_sin.obj" \
	"$(INTDIR)\s_tan.obj" \
	"$(INTDIR)\s_tanh.obj" \
	"$(INTDIR)\w_acos.obj" \
	"$(INTDIR)\w_acosh.obj" \
	"$(INTDIR)\w_asin.obj" \
	"$(INTDIR)\w_atan2.obj" \
	"$(INTDIR)\w_atanh.obj" \
	"$(INTDIR)\w_cosh.obj" \
	"$(INTDIR)\w_exp.obj" \
	"$(INTDIR)\w_fmod.obj" \
	"$(INTDIR)\w_gamma.obj" \
	"$(INTDIR)\w_gamma_r.obj" \
	"$(INTDIR)\w_hypot.obj" \
	"$(INTDIR)\w_j0.obj" \
	"$(INTDIR)\w_j1.obj" \
	"$(INTDIR)\w_jn.obj" \
	"$(INTDIR)\w_lgamma.obj" \
	"$(INTDIR)\w_lgamma_r.obj" \
	"$(INTDIR)\w_log.obj" \
	"$(INTDIR)\w_log10.obj" \
	"$(INTDIR)\w_pow.obj" \
	"$(INTDIR)\w_remainder.obj" \
	"$(INTDIR)\w_scalb.obj" \
	"$(INTDIR)\w_sinh.obj" \
	"$(INTDIR)\w_sqrt.obj"

"$(OUTDIR)\fdlibm.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "jsshell - Win32 Release"
# Name "jsshell - Win32 Debug"

!IF  "$(CFG)" == "jsshell - Win32 Release"

!ELSEIF  "$(CFG)" == "jsshell - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\js.c
DEP_CPP_JS_C0=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsdbgapi.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JS_C0=\
	".\jsdebug.h"\
	".\jsdjava.h"\
	".\jsjava.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\js.obj" : $(SOURCE) $(DEP_CPP_JS_C0) "$(INTDIR)"


# End Source File
################################################################################
# Begin Project Dependency

# Project_Dep_Name "js32"

!IF  "$(CFG)" == "jsshell - Win32 Release"

"js32 - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\js.mak" CFG="js32 - Win32 Release" 

!ELSEIF  "$(CFG)" == "jsshell - Win32 Debug"

"js32 - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\js.mak" CFG="js32 - Win32 Debug" 

!ENDIF 

# End Project Dependency
# End Target
################################################################################
# Begin Target

# Name "js32 - Win32 Release"
# Name "js32 - Win32 Debug"

!IF  "$(CFG)" == "js32 - Win32 Release"

!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\prtime.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRTIM=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prtime.obj" : $(SOURCE) $(DEP_CPP_PRTIM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRTIM=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prtime.obj" : $(SOURCE) $(DEP_CPP_PRTIM) "$(INTDIR)"

"$(INTDIR)\prtime.sbr" : $(SOURCE) $(DEP_CPP_PRTIM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsarray.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSARR=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSARR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsarray.obj" : $(SOURCE) $(DEP_CPP_JSARR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSARR=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSARR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsarray.obj" : $(SOURCE) $(DEP_CPP_JSARR) "$(INTDIR)"

"$(INTDIR)\jsarray.sbr" : $(SOURCE) $(DEP_CPP_JSARR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsatom.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSATO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSATO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsatom.obj" : $(SOURCE) $(DEP_CPP_JSATO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSATO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSATO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsatom.obj" : $(SOURCE) $(DEP_CPP_JSATO) "$(INTDIR)"

"$(INTDIR)\jsatom.sbr" : $(SOURCE) $(DEP_CPP_JSATO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsbool.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSBOO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSBOO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsbool.obj" : $(SOURCE) $(DEP_CPP_JSBOO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSBOO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSBOO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsbool.obj" : $(SOURCE) $(DEP_CPP_JSBOO) "$(INTDIR)"

"$(INTDIR)\jsbool.sbr" : $(SOURCE) $(DEP_CPP_JSBOO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jscntxt.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSCNT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSCNT=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jscntxt.obj" : $(SOURCE) $(DEP_CPP_JSCNT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSCNT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSCNT=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jscntxt.obj" : $(SOURCE) $(DEP_CPP_JSCNT) "$(INTDIR)"

"$(INTDIR)\jscntxt.sbr" : $(SOURCE) $(DEP_CPP_JSCNT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsdate.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSDAT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdate.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSDAT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsdate.obj" : $(SOURCE) $(DEP_CPP_JSDAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSDAT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdate.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSDAT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsdate.obj" : $(SOURCE) $(DEP_CPP_JSDAT) "$(INTDIR)"

"$(INTDIR)\jsdate.sbr" : $(SOURCE) $(DEP_CPP_JSDAT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsdbgapi.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSDBG=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSDBG=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsdbgapi.obj" : $(SOURCE) $(DEP_CPP_JSDBG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSDBG=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSDBG=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsdbgapi.obj" : $(SOURCE) $(DEP_CPP_JSDBG) "$(INTDIR)"

"$(INTDIR)\jsdbgapi.sbr" : $(SOURCE) $(DEP_CPP_JSDBG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsemit.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSEMI=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSEMI=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsemit.obj" : $(SOURCE) $(DEP_CPP_JSEMI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSEMI=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSEMI=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsemit.obj" : $(SOURCE) $(DEP_CPP_JSEMI) "$(INTDIR)"

"$(INTDIR)\jsemit.sbr" : $(SOURCE) $(DEP_CPP_JSEMI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsfun.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSFUN=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSFUN=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsfun.obj" : $(SOURCE) $(DEP_CPP_JSFUN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSFUN=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSFUN=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsfun.obj" : $(SOURCE) $(DEP_CPP_JSFUN) "$(INTDIR)"

"$(INTDIR)\jsfun.sbr" : $(SOURCE) $(DEP_CPP_JSFUN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsgc.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSGC_=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSGC_=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsgc.obj" : $(SOURCE) $(DEP_CPP_JSGC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSGC_=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSGC_=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsgc.obj" : $(SOURCE) $(DEP_CPP_JSGC_) "$(INTDIR)"

"$(INTDIR)\jsgc.sbr" : $(SOURCE) $(DEP_CPP_JSGC_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsinterp.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSINT=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSINT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsinterp.obj" : $(SOURCE) $(DEP_CPP_JSINT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSINT=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSINT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsinterp.obj" : $(SOURCE) $(DEP_CPP_JSINT) "$(INTDIR)"

"$(INTDIR)\jsinterp.sbr" : $(SOURCE) $(DEP_CPP_JSINT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jslock.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSLOC=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSLOC=\
	".\prcvar.h"\
	".\prlock.h"\
	".\prthread.h"\
	

"$(INTDIR)\jslock.obj" : $(SOURCE) $(DEP_CPP_JSLOC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSLOC=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSLOC=\
	".\prcvar.h"\
	".\prlock.h"\
	".\prthread.h"\
	

"$(INTDIR)\jslock.obj" : $(SOURCE) $(DEP_CPP_JSLOC) "$(INTDIR)"

"$(INTDIR)\jslock.sbr" : $(SOURCE) $(DEP_CPP_JSLOC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsmath.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSMAT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsmath.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\libmath.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSMAT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsmath.obj" : $(SOURCE) $(DEP_CPP_JSMAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSMAT=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsmath.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\libmath.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtime.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSMAT=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsmath.obj" : $(SOURCE) $(DEP_CPP_JSMAT) "$(INTDIR)"

"$(INTDIR)\jsmath.sbr" : $(SOURCE) $(DEP_CPP_JSMAT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsnum.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSNUM=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSNUM=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsnum.obj" : $(SOURCE) $(DEP_CPP_JSNUM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSNUM=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSNUM=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsnum.obj" : $(SOURCE) $(DEP_CPP_JSNUM) "$(INTDIR)"

"$(INTDIR)\jsnum.sbr" : $(SOURCE) $(DEP_CPP_JSNUM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsobj.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSOBJ=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSOBJ=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsobj.obj" : $(SOURCE) $(DEP_CPP_JSOBJ) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSOBJ=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSOBJ=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsobj.obj" : $(SOURCE) $(DEP_CPP_JSOBJ) "$(INTDIR)"

"$(INTDIR)\jsobj.sbr" : $(SOURCE) $(DEP_CPP_JSOBJ) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsopcode.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSOPC=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSOPC=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsopcode.obj" : $(SOURCE) $(DEP_CPP_JSOPC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSOPC=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSOPC=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsopcode.obj" : $(SOURCE) $(DEP_CPP_JSOPC) "$(INTDIR)"

"$(INTDIR)\jsopcode.sbr" : $(SOURCE) $(DEP_CPP_JSOPC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsparse.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSPAR=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSPAR=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsparse.obj" : $(SOURCE) $(DEP_CPP_JSPAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSPAR=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSPAR=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsparse.obj" : $(SOURCE) $(DEP_CPP_JSPAR) "$(INTDIR)"

"$(INTDIR)\jsparse.sbr" : $(SOURCE) $(DEP_CPP_JSPAR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsregexp.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSREG=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSREG=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsregexp.obj" : $(SOURCE) $(DEP_CPP_JSREG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSREG=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSREG=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsregexp.obj" : $(SOURCE) $(DEP_CPP_JSREG) "$(INTDIR)"

"$(INTDIR)\jsregexp.sbr" : $(SOURCE) $(DEP_CPP_JSREG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsscan.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSSCA=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCA=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscan.obj" : $(SOURCE) $(DEP_CPP_JSSCA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSSCA=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCA=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscan.obj" : $(SOURCE) $(DEP_CPP_JSSCA) "$(INTDIR)"

"$(INTDIR)\jsscan.sbr" : $(SOURCE) $(DEP_CPP_JSSCA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsscope.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSSCO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscope.obj" : $(SOURCE) $(DEP_CPP_JSSCO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSSCO=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCO=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscope.obj" : $(SOURCE) $(DEP_CPP_JSSCO) "$(INTDIR)"

"$(INTDIR)\jsscope.sbr" : $(SOURCE) $(DEP_CPP_JSSCO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsscript.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSSCR=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscript.obj" : $(SOURCE) $(DEP_CPP_JSSCR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSSCR=\
	".\jsapi.h"\
	".\jsatom.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdbgapi.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSCR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsscript.obj" : $(SOURCE) $(DEP_CPP_JSSCR) "$(INTDIR)"

"$(INTDIR)\jsscript.sbr" : $(SOURCE) $(DEP_CPP_JSSCR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsstr.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSSTR=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSTR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsstr.obj" : $(SOURCE) $(DEP_CPP_JSSTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSSTR=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscope.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSSTR=\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsstr.obj" : $(SOURCE) $(DEP_CPP_JSSTR) "$(INTDIR)"

"$(INTDIR)\jsstr.sbr" : $(SOURCE) $(DEP_CPP_JSSTR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prarena.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRARE=\
	".\jsstddef.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prarena.obj" : $(SOURCE) $(DEP_CPP_PRARE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRARE=\
	".\jsstddef.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prarena.obj" : $(SOURCE) $(DEP_CPP_PRARE) "$(INTDIR)"

"$(INTDIR)\prarena.sbr" : $(SOURCE) $(DEP_CPP_PRARE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prassert.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRASS=\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prassert.obj" : $(SOURCE) $(DEP_CPP_PRASS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRASS=\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prassert.obj" : $(SOURCE) $(DEP_CPP_PRASS) "$(INTDIR)"

"$(INTDIR)\prassert.sbr" : $(SOURCE) $(DEP_CPP_PRASS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdtoa.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRDTO=\
	".\jsstddef.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_PRDTO=\
	".\prlock.h"\
	

"$(INTDIR)\prdtoa.obj" : $(SOURCE) $(DEP_CPP_PRDTO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRDTO=\
	".\jsstddef.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prdtoa.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_PRDTO=\
	".\prlock.h"\
	

"$(INTDIR)\prdtoa.obj" : $(SOURCE) $(DEP_CPP_PRDTO) "$(INTDIR)"

"$(INTDIR)\prdtoa.sbr" : $(SOURCE) $(DEP_CPP_PRDTO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prhash.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRHAS=\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prhash.obj" : $(SOURCE) $(DEP_CPP_PRHAS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRHAS=\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prhash.obj" : $(SOURCE) $(DEP_CPP_PRHAS) "$(INTDIR)"

"$(INTDIR)\prhash.sbr" : $(SOURCE) $(DEP_CPP_PRHAS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prlog2.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRLOG=\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prlog2.obj" : $(SOURCE) $(DEP_CPP_PRLOG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRLOG=\
	".\prcpucfg.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prlog2.obj" : $(SOURCE) $(DEP_CPP_PRLOG) "$(INTDIR)"

"$(INTDIR)\prlog2.sbr" : $(SOURCE) $(DEP_CPP_PRLOG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prlong.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRLON=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prlong.obj" : $(SOURCE) $(DEP_CPP_PRLON) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRLON=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prlong.obj" : $(SOURCE) $(DEP_CPP_PRLON) "$(INTDIR)"

"$(INTDIR)\prlong.sbr" : $(SOURCE) $(DEP_CPP_PRLON) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prprintf.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_PRPRI=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prprintf.obj" : $(SOURCE) $(DEP_CPP_PRPRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_PRPRI=\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prprintf.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\prprintf.obj" : $(SOURCE) $(DEP_CPP_PRPRI) "$(INTDIR)"

"$(INTDIR)\prprintf.sbr" : $(SOURCE) $(DEP_CPP_PRPRI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsapi.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSAPI=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdate.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsmath.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSAPI=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsapi.obj" : $(SOURCE) $(DEP_CPP_JSAPI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSAPI=\
	".\jsapi.h"\
	".\jsarray.h"\
	".\jsatom.h"\
	".\jsbool.h"\
	".\jscntxt.h"\
	".\jsconfig.h"\
	".\jsdate.h"\
	".\jsemit.h"\
	".\jsfun.h"\
	".\jsgc.h"\
	".\jsinterp.h"\
	".\jslock.h"\
	".\jsmath.h"\
	".\jsnum.h"\
	".\jsobj.h"\
	".\jsopcode.def"\
	".\jsopcode.h"\
	".\jsparse.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsregexp.h"\
	".\jsscan.h"\
	".\jsscope.h"\
	".\jsscript.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\os\aix.h"\
	".\os\bsdi.h"\
	".\os\hpux.h"\
	".\os\irix.h"\
	".\os\linux.h"\
	".\os\osf1.h"\
	".\os\scoos.h"\
	".\os\solaris.h"\
	".\os\sunos.h"\
	".\os\unixware.h"\
	".\os\win16.h"\
	".\os\win32.h"\
	".\prarena.h"\
	".\prassert.h"\
	".\prclist.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prlong.h"\
	".\prmacos.h"\
	".\prosdep.h"\
	".\prpcos.h"\
	".\prtypes.h"\
	".\prunixos.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_JSAPI=\
	".\jsdebug.h"\
	".\prcvar.h"\
	".\prlock.h"\
	

"$(INTDIR)\jsapi.obj" : $(SOURCE) $(DEP_CPP_JSAPI) "$(INTDIR)"

"$(INTDIR)\jsapi.sbr" : $(SOURCE) $(DEP_CPP_JSAPI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\jsxdrapi.c

!IF  "$(CFG)" == "js32 - Win32 Release"

DEP_CPP_JSXDR=\
	".\jsapi.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\jsxdrapi.obj" : $(SOURCE) $(DEP_CPP_JSXDR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

DEP_CPP_JSXDR=\
	".\jsapi.h"\
	".\jsobj.h"\
	".\jsprvtd.h"\
	".\jspubtd.h"\
	".\jsstddef.h"\
	".\jsstr.h"\
	".\jsxdrapi.h"\
	".\prassert.h"\
	".\prcpucfg.h"\
	".\prhash.h"\
	".\prtypes.h"\
	".\sunos4.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\jsxdrapi.obj" : $(SOURCE) $(DEP_CPP_JSXDR) "$(INTDIR)"

"$(INTDIR)\jsxdrapi.sbr" : $(SOURCE) $(DEP_CPP_JSXDR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Project Dependency

# Project_Dep_Name "fdlibm"

!IF  "$(CFG)" == "js32 - Win32 Release"

"fdlibm - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\js.mak" CFG="fdlibm - Win32 Release" 

!ELSEIF  "$(CFG)" == "js32 - Win32 Debug"

"fdlibm - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\js.mak" CFG="fdlibm - Win32 Debug" 

!ENDIF 

# End Project Dependency
# End Target
################################################################################
# Begin Target

# Name "fdlibm - Win32 Release"
# Name "fdlibm - Win32 Debug"

!IF  "$(CFG)" == "fdlibm - Win32 Release"

!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_sqrt.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_SQR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_sqrt.obj" : $(SOURCE) $(DEP_CPP_W_SQR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_SQR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_sqrt.obj" : $(SOURCE) $(DEP_CPP_W_SQR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_acosh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_ACO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_acosh.obj" : $(SOURCE) $(DEP_CPP_E_ACO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_ACO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_acosh.obj" : $(SOURCE) $(DEP_CPP_E_ACO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_asin.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_asin.obj" : $(SOURCE) $(DEP_CPP_E_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_asin.obj" : $(SOURCE) $(DEP_CPP_E_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_atan2.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_atan2.obj" : $(SOURCE) $(DEP_CPP_E_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_atan2.obj" : $(SOURCE) $(DEP_CPP_E_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_atanh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_ATAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_atanh.obj" : $(SOURCE) $(DEP_CPP_E_ATAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_ATAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_atanh.obj" : $(SOURCE) $(DEP_CPP_E_ATAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_cosh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_cosh.obj" : $(SOURCE) $(DEP_CPP_E_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_cosh.obj" : $(SOURCE) $(DEP_CPP_E_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_exp.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_exp.obj" : $(SOURCE) $(DEP_CPP_E_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_exp.obj" : $(SOURCE) $(DEP_CPP_E_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_fmod.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_FMO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_fmod.obj" : $(SOURCE) $(DEP_CPP_E_FMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_FMO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_fmod.obj" : $(SOURCE) $(DEP_CPP_E_FMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_gamma.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_GAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_gamma.obj" : $(SOURCE) $(DEP_CPP_E_GAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_GAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_gamma.obj" : $(SOURCE) $(DEP_CPP_E_GAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_gamma_r.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_GAMM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_gamma_r.obj" : $(SOURCE) $(DEP_CPP_E_GAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_GAMM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_gamma_r.obj" : $(SOURCE) $(DEP_CPP_E_GAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_hypot.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_HYP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_hypot.obj" : $(SOURCE) $(DEP_CPP_E_HYP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_HYP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_hypot.obj" : $(SOURCE) $(DEP_CPP_E_HYP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_j0.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_J0_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_j0.obj" : $(SOURCE) $(DEP_CPP_E_J0_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_J0_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_j0.obj" : $(SOURCE) $(DEP_CPP_E_J0_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_j1.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_J1_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_j1.obj" : $(SOURCE) $(DEP_CPP_E_J1_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_J1_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_j1.obj" : $(SOURCE) $(DEP_CPP_E_J1_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_jn.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_JN_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_jn.obj" : $(SOURCE) $(DEP_CPP_E_JN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_JN_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_jn.obj" : $(SOURCE) $(DEP_CPP_E_JN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_lgamma.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_LGA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_lgamma.obj" : $(SOURCE) $(DEP_CPP_E_LGA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_LGA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_lgamma.obj" : $(SOURCE) $(DEP_CPP_E_LGA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_lgamma_r.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_LGAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_lgamma_r.obj" : $(SOURCE) $(DEP_CPP_E_LGAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_LGAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_lgamma_r.obj" : $(SOURCE) $(DEP_CPP_E_LGAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_log.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_log.obj" : $(SOURCE) $(DEP_CPP_E_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_log.obj" : $(SOURCE) $(DEP_CPP_E_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_log10.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_LOG1=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_log10.obj" : $(SOURCE) $(DEP_CPP_E_LOG1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_LOG1=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_log10.obj" : $(SOURCE) $(DEP_CPP_E_LOG1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_pow.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_POW=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_pow.obj" : $(SOURCE) $(DEP_CPP_E_POW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_POW=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_pow.obj" : $(SOURCE) $(DEP_CPP_E_POW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_rem_pio2.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_rem_pio2.obj" : $(SOURCE) $(DEP_CPP_E_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_rem_pio2.obj" : $(SOURCE) $(DEP_CPP_E_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_remainder.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_REMA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_remainder.obj" : $(SOURCE) $(DEP_CPP_E_REMA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_REMA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_remainder.obj" : $(SOURCE) $(DEP_CPP_E_REMA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_scalb.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_scalb.obj" : $(SOURCE) $(DEP_CPP_E_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_scalb.obj" : $(SOURCE) $(DEP_CPP_E_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_sinh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_sinh.obj" : $(SOURCE) $(DEP_CPP_E_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_sinh.obj" : $(SOURCE) $(DEP_CPP_E_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_sqrt.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_SQR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_sqrt.obj" : $(SOURCE) $(DEP_CPP_E_SQR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_SQR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_sqrt.obj" : $(SOURCE) $(DEP_CPP_E_SQR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\k_cos.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_K_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_cos.obj" : $(SOURCE) $(DEP_CPP_K_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_K_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_cos.obj" : $(SOURCE) $(DEP_CPP_K_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\k_rem_pio2.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_K_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_rem_pio2.obj" : $(SOURCE) $(DEP_CPP_K_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_K_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_rem_pio2.obj" : $(SOURCE) $(DEP_CPP_K_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\k_sin.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_K_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_sin.obj" : $(SOURCE) $(DEP_CPP_K_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_K_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_sin.obj" : $(SOURCE) $(DEP_CPP_K_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\k_standard.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_K_STA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_standard.obj" : $(SOURCE) $(DEP_CPP_K_STA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_K_STA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_standard.obj" : $(SOURCE) $(DEP_CPP_K_STA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\k_tan.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_K_TAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_tan.obj" : $(SOURCE) $(DEP_CPP_K_TAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_K_TAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\k_tan.obj" : $(SOURCE) $(DEP_CPP_K_TAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_asinh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_asinh.obj" : $(SOURCE) $(DEP_CPP_S_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_asinh.obj" : $(SOURCE) $(DEP_CPP_S_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_atan.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_atan.obj" : $(SOURCE) $(DEP_CPP_S_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_atan.obj" : $(SOURCE) $(DEP_CPP_S_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_cbrt.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_CBR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_cbrt.obj" : $(SOURCE) $(DEP_CPP_S_CBR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_CBR=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_cbrt.obj" : $(SOURCE) $(DEP_CPP_S_CBR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_ceil.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_CEI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ceil.obj" : $(SOURCE) $(DEP_CPP_S_CEI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_CEI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ceil.obj" : $(SOURCE) $(DEP_CPP_S_CEI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_copysign.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_COP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_copysign.obj" : $(SOURCE) $(DEP_CPP_S_COP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_COP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_copysign.obj" : $(SOURCE) $(DEP_CPP_S_COP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_cos.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_cos.obj" : $(SOURCE) $(DEP_CPP_S_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_cos.obj" : $(SOURCE) $(DEP_CPP_S_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_erf.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_ERF=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_erf.obj" : $(SOURCE) $(DEP_CPP_S_ERF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_ERF=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_erf.obj" : $(SOURCE) $(DEP_CPP_S_ERF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_expm1.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_expm1.obj" : $(SOURCE) $(DEP_CPP_S_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_expm1.obj" : $(SOURCE) $(DEP_CPP_S_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_fabs.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_FAB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_fabs.obj" : $(SOURCE) $(DEP_CPP_S_FAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_FAB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_fabs.obj" : $(SOURCE) $(DEP_CPP_S_FAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_finite.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_FIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_finite.obj" : $(SOURCE) $(DEP_CPP_S_FIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_FIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_finite.obj" : $(SOURCE) $(DEP_CPP_S_FIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_floor.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_FLO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_floor.obj" : $(SOURCE) $(DEP_CPP_S_FLO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_FLO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_floor.obj" : $(SOURCE) $(DEP_CPP_S_FLO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_frexp.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_FRE=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_frexp.obj" : $(SOURCE) $(DEP_CPP_S_FRE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_FRE=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_frexp.obj" : $(SOURCE) $(DEP_CPP_S_FRE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_ilogb.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_ILO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ilogb.obj" : $(SOURCE) $(DEP_CPP_S_ILO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_ILO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ilogb.obj" : $(SOURCE) $(DEP_CPP_S_ILO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_isnan.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_ISN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_isnan.obj" : $(SOURCE) $(DEP_CPP_S_ISN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_ISN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_isnan.obj" : $(SOURCE) $(DEP_CPP_S_ISN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_ldexp.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_LDE=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ldexp.obj" : $(SOURCE) $(DEP_CPP_S_LDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_LDE=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_ldexp.obj" : $(SOURCE) $(DEP_CPP_S_LDE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_lib_version.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_LIB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_lib_version.obj" : $(SOURCE) $(DEP_CPP_S_LIB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_LIB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_lib_version.obj" : $(SOURCE) $(DEP_CPP_S_LIB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_log1p.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_log1p.obj" : $(SOURCE) $(DEP_CPP_S_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_log1p.obj" : $(SOURCE) $(DEP_CPP_S_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_logb.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_LOGB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_logb.obj" : $(SOURCE) $(DEP_CPP_S_LOGB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_LOGB=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_logb.obj" : $(SOURCE) $(DEP_CPP_S_LOGB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_matherr.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_MAT=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_matherr.obj" : $(SOURCE) $(DEP_CPP_S_MAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_MAT=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_matherr.obj" : $(SOURCE) $(DEP_CPP_S_MAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_modf.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_MOD=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_modf.obj" : $(SOURCE) $(DEP_CPP_S_MOD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_MOD=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_modf.obj" : $(SOURCE) $(DEP_CPP_S_MOD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_nextafter.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_NEX=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_nextafter.obj" : $(SOURCE) $(DEP_CPP_S_NEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_NEX=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_nextafter.obj" : $(SOURCE) $(DEP_CPP_S_NEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_rint.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_RIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_rint.obj" : $(SOURCE) $(DEP_CPP_S_RIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_RIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_rint.obj" : $(SOURCE) $(DEP_CPP_S_RIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_scalbn.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_scalbn.obj" : $(SOURCE) $(DEP_CPP_S_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_scalbn.obj" : $(SOURCE) $(DEP_CPP_S_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_signgam.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_SIG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_signgam.obj" : $(SOURCE) $(DEP_CPP_S_SIG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_SIG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_signgam.obj" : $(SOURCE) $(DEP_CPP_S_SIG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_significand.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_SIGN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_significand.obj" : $(SOURCE) $(DEP_CPP_S_SIGN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_SIGN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_significand.obj" : $(SOURCE) $(DEP_CPP_S_SIGN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_sin.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_sin.obj" : $(SOURCE) $(DEP_CPP_S_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_sin.obj" : $(SOURCE) $(DEP_CPP_S_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_tan.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_TAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_tan.obj" : $(SOURCE) $(DEP_CPP_S_TAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_TAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_tan.obj" : $(SOURCE) $(DEP_CPP_S_TAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\s_tanh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_S_TANH=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_tanh.obj" : $(SOURCE) $(DEP_CPP_S_TANH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_S_TANH=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\s_tanh.obj" : $(SOURCE) $(DEP_CPP_S_TANH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_acos.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_ACO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_acos.obj" : $(SOURCE) $(DEP_CPP_W_ACO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_ACO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_acos.obj" : $(SOURCE) $(DEP_CPP_W_ACO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_acosh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_ACOS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_acosh.obj" : $(SOURCE) $(DEP_CPP_W_ACOS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_ACOS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_acosh.obj" : $(SOURCE) $(DEP_CPP_W_ACOS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_asin.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_asin.obj" : $(SOURCE) $(DEP_CPP_W_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_ASI=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_asin.obj" : $(SOURCE) $(DEP_CPP_W_ASI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_atan2.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_atan2.obj" : $(SOURCE) $(DEP_CPP_W_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_ATA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_atan2.obj" : $(SOURCE) $(DEP_CPP_W_ATA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_atanh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_ATAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_atanh.obj" : $(SOURCE) $(DEP_CPP_W_ATAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_ATAN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_atanh.obj" : $(SOURCE) $(DEP_CPP_W_ATAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_cosh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_cosh.obj" : $(SOURCE) $(DEP_CPP_W_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_COS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_cosh.obj" : $(SOURCE) $(DEP_CPP_W_COS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_exp.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_exp.obj" : $(SOURCE) $(DEP_CPP_W_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_EXP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_exp.obj" : $(SOURCE) $(DEP_CPP_W_EXP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_fmod.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_FMO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_fmod.obj" : $(SOURCE) $(DEP_CPP_W_FMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_FMO=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_fmod.obj" : $(SOURCE) $(DEP_CPP_W_FMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_gamma.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_GAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_gamma.obj" : $(SOURCE) $(DEP_CPP_W_GAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_GAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_gamma.obj" : $(SOURCE) $(DEP_CPP_W_GAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_gamma_r.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_GAMM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_gamma_r.obj" : $(SOURCE) $(DEP_CPP_W_GAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_GAMM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_gamma_r.obj" : $(SOURCE) $(DEP_CPP_W_GAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_hypot.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_HYP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_hypot.obj" : $(SOURCE) $(DEP_CPP_W_HYP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_HYP=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_hypot.obj" : $(SOURCE) $(DEP_CPP_W_HYP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_j0.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_J0_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_j0.obj" : $(SOURCE) $(DEP_CPP_W_J0_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_J0_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_j0.obj" : $(SOURCE) $(DEP_CPP_W_J0_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_j1.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_J1_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_j1.obj" : $(SOURCE) $(DEP_CPP_W_J1_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_J1_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_j1.obj" : $(SOURCE) $(DEP_CPP_W_J1_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_jn.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_JN_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_jn.obj" : $(SOURCE) $(DEP_CPP_W_JN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_JN_=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_jn.obj" : $(SOURCE) $(DEP_CPP_W_JN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_lgamma.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_LGA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_lgamma.obj" : $(SOURCE) $(DEP_CPP_W_LGA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_LGA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_lgamma.obj" : $(SOURCE) $(DEP_CPP_W_LGA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_lgamma_r.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_LGAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_lgamma_r.obj" : $(SOURCE) $(DEP_CPP_W_LGAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_LGAM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_lgamma_r.obj" : $(SOURCE) $(DEP_CPP_W_LGAM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_log.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_log.obj" : $(SOURCE) $(DEP_CPP_W_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_LOG=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_log.obj" : $(SOURCE) $(DEP_CPP_W_LOG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_log10.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_LOG1=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_log10.obj" : $(SOURCE) $(DEP_CPP_W_LOG1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_LOG1=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_log10.obj" : $(SOURCE) $(DEP_CPP_W_LOG1) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_pow.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_POW=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_pow.obj" : $(SOURCE) $(DEP_CPP_W_POW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_POW=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_pow.obj" : $(SOURCE) $(DEP_CPP_W_POW) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_remainder.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_remainder.obj" : $(SOURCE) $(DEP_CPP_W_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_REM=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_remainder.obj" : $(SOURCE) $(DEP_CPP_W_REM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_scalb.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_scalb.obj" : $(SOURCE) $(DEP_CPP_W_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_SCA=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_scalb.obj" : $(SOURCE) $(DEP_CPP_W_SCA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\w_sinh.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_W_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_sinh.obj" : $(SOURCE) $(DEP_CPP_W_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_W_SIN=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\w_sinh.obj" : $(SOURCE) $(DEP_CPP_W_SIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\fdlibm\e_acos.c

!IF  "$(CFG)" == "fdlibm - Win32 Release"

DEP_CPP_E_ACOS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_acos.obj" : $(SOURCE) $(DEP_CPP_E_ACOS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "fdlibm - Win32 Debug"

DEP_CPP_E_ACOS=\
	".\fdlibm\fdlibm.h"\
	

"$(INTDIR)\e_acos.obj" : $(SOURCE) $(DEP_CPP_E_ACOS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
