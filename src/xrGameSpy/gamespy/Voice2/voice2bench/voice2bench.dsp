# Microsoft Developer Studio Project File - Name="voice2bench" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=voice2bench - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "voice2bench.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "voice2bench.mak" CFG="voice2bench - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "voice2bench - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "voice2bench - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/IGNCP/Gamespy/GOA/Voice2/voice2bench"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /WX /GX /O2 /I "..\speex-1.0.5\include" /I "..\gsm-1.0-pl12\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libgsm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\libgsm\Release" /libpath:"..\libspeex\Release" /ignore:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /WX /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /I "..\gsm-1.0-pl12\inc" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "LTP_CUT" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libspeex.lib libgsm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\libgsm\Debug" /libpath:"..\libspeex\Debug"

!ENDIF 

# Begin Target

# Name "voice2bench - Win32 Release"
# Name "voice2bench - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\voice2bench.c

!IF  "$(CFG)" == "voice2bench - Win32 Release"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\voicesample.h
# End Source File
# End Group
# Begin Group "VoiceSDK"

# PROP Default_Filter ""
# Begin Group "VoiceHeaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gv.h
# End Source File
# Begin Source File

SOURCE=..\gvCodec.h
# End Source File
# Begin Source File

SOURCE=..\gvCustomDevice.h
# End Source File
# Begin Source File

SOURCE=..\gvDevice.h
# End Source File
# Begin Source File

SOURCE=..\gvFrame.h
# End Source File
# Begin Source File

SOURCE=..\gvMain.h
# End Source File
# Begin Source File

SOURCE=..\gvSource.h
# End Source File
# Begin Source File

SOURCE=..\gvSpeex.h
# End Source File
# Begin Source File

SOURCE=..\gvUtil.h
# End Source File
# End Group
# Begin Group "VoiceSource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gvCodec.c
# End Source File
# Begin Source File

SOURCE=..\gvCustomDevice.c
# End Source File
# Begin Source File

SOURCE=..\gvDevice.c
# End Source File
# Begin Source File

SOURCE=..\gvFrame.c
# End Source File
# Begin Source File

SOURCE=..\gvSource.c
# End Source File
# Begin Source File

SOURCE=..\gvSpeex.c
# End Source File
# Begin Source File

SOURCE=..\gvUtil.c
# End Source File
# End Group
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\darray.c
# End Source File
# Begin Source File

SOURCE=..\..\darray.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsAssert.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsAssert.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsAvailable.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.h
# End Source File
# End Group
# Begin Group "Gsm"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\add.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\code.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\debug.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\decode.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\gsm_create.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\gsm_decode.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\gsm_destroy.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\gsm_encode.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\gsm_option.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\long_term.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\lpc.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\preprocess.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\rpe.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\short_term.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\gsm-1.0-pl12\src\table.c"

!IF  "$(CFG)" == "voice2bench - Win32 Release"

# ADD CPP /D "LTP_CUT"
# SUBTRACT CPP /D "HAVE_CONFIG_H"

!ELSEIF  "$(CFG)" == "voice2bench - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /WX /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
