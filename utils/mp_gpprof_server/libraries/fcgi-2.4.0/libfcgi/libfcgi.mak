# Microsoft Developer Studio Generated NMAKE File, Based on libfcgi.dsp

!IF "$(CFG)" == ""
CFG=release
!ENDIF 

!IF "$(CFG)" != "release" && "$(CFG)" != "debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfcgi.mak" CFG="debug"
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

OUTDIR=.\..\libfcgi\Release
INTDIR=.\..\libfcgi\Release
# Begin Custom Macros
OutDir=.\..\libfcgi\Release
# End Custom Macros

ALL : "$(OUTDIR)\libfcgi.dll"


CLEAN :
	-@erase "$(INTDIR)\fcgi_stdio.obj"
	-@erase "$(INTDIR)\fcgiapp.obj"
	-@erase "$(INTDIR)\fcgio.obj"
	-@erase "$(INTDIR)\os_win32.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\libfcgi.dll"
	-@erase "$(OUTDIR)\libfcgi.exp"
	-@erase "$(OUTDIR)\libfcgi.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /Ob2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\libfcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libfcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=Ws2_32.lib /nologo /dll /pdb:none /machine:I386 /out:"$(OUTDIR)\libfcgi.dll" /implib:"$(OUTDIR)\libfcgi.lib" 
LINK32_OBJS= \
	"$(INTDIR)\fcgi_stdio.obj" \
	"$(INTDIR)\fcgiapp.obj" \
	"$(INTDIR)\fcgio.obj" \
	"$(INTDIR)\os_win32.obj"

"$(OUTDIR)\libfcgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "debug"

OUTDIR=.\..\libfcgi\Debug
INTDIR=.\..\libfcgi\Debug
# Begin Custom Macros
OutDir=.\..\libfcgi\Debug
# End Custom Macros

ALL : "$(OUTDIR)\libfcgi.dll" "$(OUTDIR)\libfcgi.bsc"


CLEAN :
	-@erase "$(INTDIR)\fcgi_stdio.obj"
	-@erase "$(INTDIR)\fcgi_stdio.sbr"
	-@erase "$(INTDIR)\fcgiapp.obj"
	-@erase "$(INTDIR)\fcgiapp.sbr"
	-@erase "$(INTDIR)\fcgio.obj"
	-@erase "$(INTDIR)\fcgio.sbr"
	-@erase "$(INTDIR)\os_win32.obj"
	-@erase "$(INTDIR)\os_win32.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\libfcgi.bsc"
	-@erase "$(OUTDIR)\libfcgi.dll"
	-@erase "$(OUTDIR)\libfcgi.exp"
	-@erase "$(OUTDIR)\libfcgi.lib"
	-@erase "$(OUTDIR)\libfcgi.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W4 /Gm /Gi /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libfcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libfcgi.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\fcgi_stdio.sbr" \
	"$(INTDIR)\fcgiapp.sbr" \
	"$(INTDIR)\fcgio.sbr" \
	"$(INTDIR)\os_win32.sbr"

"$(OUTDIR)\libfcgi.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=Ws2_32.lib /nologo /dll /profile /map:"$(INTDIR)\libfcgi.map" /debug /machine:I386 /out:"$(OUTDIR)\libfcgi.dll" /implib:"$(OUTDIR)\libfcgi.lib" 
LINK32_OBJS= \
	"$(INTDIR)\fcgi_stdio.obj" \
	"$(INTDIR)\fcgiapp.obj" \
	"$(INTDIR)\fcgio.obj" \
	"$(INTDIR)\os_win32.obj"

"$(OUTDIR)\libfcgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


..\libfcgi\fcgi_stdio.c : \
	"..\include\fcgi_config.h"\
	"..\include\fcgi_stdio.h"\
	"..\include\fcgiapp.h"\
	"..\include\fcgimisc.h"\
	"..\include\fcgios.h"\
	

..\libfcgi\fcgiapp.c : \
	"..\include\fastcgi.h"\
	"..\include\fcgi_config.h"\
	"..\include\fcgiapp.h"\
	"..\include\fcgimisc.h"\
	"..\include\fcgios.h"\
	

..\libfcgi\fcgio.cpp : \
	"..\include\fcgiapp.h"\
	"..\include\fcgio.h"\
	

..\libfcgi\os_win32.c : \
	"..\include\fcgi_config.h"\
	"..\include\fcgimisc.h"\
	"..\include\fcgios.h"\


!IF "$(CFG)" == "release" || "$(CFG)" == "debug"
SOURCE=..\libfcgi\fcgi_stdio.c

!IF  "$(CFG)" == "release"


"$(INTDIR)\fcgi_stdio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "debug"


"$(INTDIR)\fcgi_stdio.obj"	"$(INTDIR)\fcgi_stdio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\libfcgi\fcgiapp.c

!IF  "$(CFG)" == "release"


"$(INTDIR)\fcgiapp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "debug"


"$(INTDIR)\fcgiapp.obj"	"$(INTDIR)\fcgiapp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\libfcgi\fcgio.cpp

!IF  "$(CFG)" == "release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /Ob2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\libfcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\fcgio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /Gi /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libfcgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\fcgio.obj"	"$(INTDIR)\fcgio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\libfcgi\os_unix.c
SOURCE=..\libfcgi\os_win32.c

!IF  "$(CFG)" == "release"


"$(INTDIR)\os_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "debug"


"$(INTDIR)\os_win32.obj"	"$(INTDIR)\os_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\libfcgi\strerror.c

!ENDIF 

