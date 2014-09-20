# Microsoft Developer Studio Project File - Name="libspeex" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libspeex - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libspeex.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libspeex.mak" CFG="libspeex - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libspeex - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libspeex - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/IGNCP/Gamespy/GOA/Voice2/libspeex"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libspeex - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W1 /GX /O2 /I "..\speex-1.0.5\include" /I "..\speex-1.0.5\win32" /I "..\speex-1.0.5\include\speex" /I "..\speex-1.0.5\libspeex" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libspeex - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W1 /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /I "..\speex-1.0.5\win32" /I "..\speex-1.0.5\include\speex" /I "..\speex-1.0.5\libspeex" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libspeex - Win32 Release"
# Name "libspeex - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\bits.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\cb_search.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\cb_search.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_10_16_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_10_32_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_20_32_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_5_256_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_5_64_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\exc_8_128_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\filters.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\filters.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\filters_sse.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\gain_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\gain_table_lbr.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\hexc_10_32_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\hexc_table.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\high_lsp_tables.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\lpc.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\lpc.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\lsp.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\lsp.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\lsp_tables_nb.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\ltp.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\ltp.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\ltp_sse.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\math_approx.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\math_approx.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\misc.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\misc.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\modes.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\modes.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\nb_celp.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\nb_celp.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\quant_lsp.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\quant_lsp.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\sb_celp.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\sb_celp.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\speex_callbacks.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\speex_header.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\stack_alloc.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\stereo.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\vbr.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\vbr.h"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\vq.c"
# End Source File
# Begin Source File

SOURCE="..\speex-1.0.5\libspeex\vq.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
