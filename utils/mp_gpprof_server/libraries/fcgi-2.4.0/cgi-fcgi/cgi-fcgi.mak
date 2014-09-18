# Microsoft Developer Studio Generated NMAKE File, Based on cgifcgi.dsp

!IF "$(CFG)" == ""
CFG=release
!ENDIF 

!IF "$(CFG)" != "release" && "$(CFG)" != "debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cgifcgi.mak" CFG="debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "release"

OUTDIR=.\..\cgi-fcgi\Release
INTDIR=.\..\cgi-fcgi\Release
# Begin Custom Macros
OutDir=.\..\cgi-fcgi\Release
# End Custom Macros

ALL : "$(OUTDIR)\cgi-fcgi.exe"

CLEAN :
	-@erase "$(INTDIR)\cgi-fcgi.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\cgi-fcgi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Gi /O2 /Ob2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\cgifcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cgifcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libfcgi.lib /nologo /pdb:none /machine:IX86 /out:"$(OUTDIR)\cgi-fcgi.exe" /libpath:"..\libfcgi\Release" 
LINK32_OBJS= \
	"$(INTDIR)\cgi-fcgi.obj" \
	"..\libfcgi\Release\libfcgi.lib"

"$(OUTDIR)\cgi-fcgi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "debug"

OUTDIR=.\../cgi-fcgi/Debug
INTDIR=.\../cgi-fcgi/Debug
# Begin Custom Macros
OutDir=.\../cgi-fcgi/Debug
# End Custom Macros

ALL : "$(OUTDIR)\cgi-fcgi.exe" "$(OUTDIR)\cgifcgi.bsc"

CLEAN :
	-@erase "$(INTDIR)\cgi-fcgi.obj"
	-@erase "$(INTDIR)\cgi-fcgi.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\cgi-fcgi.exe"
	-@erase "$(OUTDIR)\cgifcgi.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W4 /Gm /Gi /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\cgifcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cgifcgi.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cgi-fcgi.sbr"

"$(OUTDIR)\cgifcgi.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=libfcgi.lib /nologo /profile /debug /machine:IX86 /out:"$(OUTDIR)\cgi-fcgi.exe" /libpath:"..\libfcgi\Debug" 
LINK32_OBJS= \
	"$(INTDIR)\cgi-fcgi.obj" \
	"..\libfcgi\Debug\libfcgi.lib"

"$(OUTDIR)\cgi-fcgi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


"..\cgi-fcgi\cgi-fcgi.c" : \
	"..\include\fastcgi.h"\
	"..\include\fcgi_config.h"\
	"..\include\fcgiapp.h"\
	"..\include\fcgimisc.h"\
	"..\include\fcgios.h"\


!IF "$(CFG)" == "release" || "$(CFG)" == "debug"
SOURCE="..\cgi-fcgi\cgi-fcgi.c"

!IF  "$(CFG)" == "release"


"$(INTDIR)\cgi-fcgi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "debug"


"$(INTDIR)\cgi-fcgi.obj"	"$(INTDIR)\cgi-fcgi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

!ENDIF 

