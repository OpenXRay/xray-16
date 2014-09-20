# Microsoft Developer Studio Project File - Name="Voice2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Voice2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Voice2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Voice2.mak" CFG="Voice2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Voice2 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Voice2 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Voice2", NONDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Voice2 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "speex-1.0.5\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=snLib.exe
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Voice2 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "speex-1.0.5\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=snLib.exe
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Voice2 - Win32 Release"
# Name "Voice2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\gvCodec.c
# End Source File
# Begin Source File

SOURCE=.\gvCustomDevice.c
# End Source File
# Begin Source File

SOURCE=.\gvDevice.c
# End Source File
# Begin Source File

SOURCE=.\gvDirectSound.c
# End Source File
# Begin Source File

SOURCE=.\gvFrame.c
# End Source File
# Begin Source File

SOURCE=.\gvMain.c
# End Source File
# Begin Source File

SOURCE=.\gvSource.c
# End Source File
# Begin Source File

SOURCE=.\gvSpeex.c
# End Source File
# Begin Source File

SOURCE=.\gvUtil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\gv.h
# End Source File
# Begin Source File

SOURCE=.\gvCodec.h
# End Source File
# Begin Source File

SOURCE=.\gvCustomDevice.h
# End Source File
# Begin Source File

SOURCE=.\gvDevice.h
# End Source File
# Begin Source File

SOURCE=.\gvDirectSound.h
# End Source File
# Begin Source File

SOURCE=.\gvFrame.h
# End Source File
# Begin Source File

SOURCE=.\gvMain.h
# End Source File
# Begin Source File

SOURCE=.\gvSource.h
# End Source File
# Begin Source File

SOURCE=.\gvSpeex.h
# End Source File
# Begin Source File

SOURCE=.\gvUtil.h
# End Source File
# Begin Source File

SOURCE=..\nonport.h
# End Source File
# End Group
# Begin Group "Speex"

# PROP Default_Filter ""
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\bits.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\cb_search.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_10_16_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_10_32_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_20_32_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_5_256_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_5_64_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\exc_8_128_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\filters.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\gain_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\gain_table_lbr.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\hexc_10_32_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\hexc_table.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\high_lsp_tables.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\lpc.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\lsp.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\lsp_tables_nb.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\ltp.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\math_approx.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\misc.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\modes.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\nb_celp.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\quant_lsp.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\sb_celp.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\speex_callbacks.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\speex_header.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\stereo.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\vbr.c"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\vq.c"
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\cb_search.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\disable_warnings.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\filters.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\filters_sse.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\lpc.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\lsp.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\ltp.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\ltp_sse.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\math_approx.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\misc.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\modes.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\nb_celp.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\quant_lsp.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\sb_celp.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\stack_alloc.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\vbr.h"
# End Source File
# Begin Source File

SOURCE=".\speex-1.0.5\libspeex\vq.h"
# End Source File
# End Group
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\darray.c
# End Source File
# Begin Source File

SOURCE=..\darray.h
# End Source File
# Begin Source File

SOURCE=..\common\gsAssert.c
# End Source File
# Begin Source File

SOURCE=..\common\gsAssert.h
# End Source File
# Begin Source File

SOURCE=..\common\gsAvailable.c
# End Source File
# Begin Source File

SOURCE=..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\common\gsMemory.c
# End Source File
# Begin Source File

SOURCE=..\common\gsMemory.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatform.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformUtil.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformUtil.h
# End Source File
# Begin Source File

SOURCE=..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\common\gsStringUtil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\changelog.txt
# End Source File
# End Target
# End Project
