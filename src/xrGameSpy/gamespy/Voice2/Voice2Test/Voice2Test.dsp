# Microsoft Developer Studio Project File - Name="Voice2Test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Voice2Test - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Voice2Test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Voice2Test.mak" CFG="Voice2Test - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Voice2Test - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Voice2Test - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Voice2Test - Win32 Debug Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE "Voice2Test - Win32 Release Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/IGNCP/Gamespy/GOA/Voice2/Voice2Test"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Voice2Test - Win32 Release"

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
# ADD CPP /nologo /W3 /WX /GX /O2 /I "..\speex-1.0.5\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /D "HAVE_CONFIG_H" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libspeex.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\libspeex\Release"

!ELSEIF  "$(CFG)" == "Voice2Test - Win32 Debug"

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
# ADD CPP /nologo /W3 /WX /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /D "HAVE_CONFIG_H" /Fr /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libspeex.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\libspeex\Debug"

!ELSEIF  "$(CFG)" == "Voice2Test - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Voice2Test___Win32_Debug_Unicode0"
# PROP BASE Intermediate_Dir "Voice2Test___Win32_Debug_Unicode0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Voice2Test___Win32_Debug_Unicode0"
# PROP Intermediate_Dir "Voice2Test___Win32_Debug_Unicode0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /YX /FD /GZ /c
# SUBTRACT BASE CPP /WX /Fr
# ADD CPP /nologo /W3 /WX /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /D "GSI_UNICODE" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libspeex.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\libspeex\Debug"

!ELSEIF  "$(CFG)" == "Voice2Test - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Voice2Test___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "Voice2Test___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Voice2Test___Win32_Release_Unicode"
# PROP Intermediate_Dir "Voice2Test___Win32_Release_Unicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\speex-1.0.5\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /YX /FD /c
# SUBTRACT BASE CPP /WX /Fr
# ADD CPP /nologo /W3 /WX /GX /O2 /I "..\speex-1.0.5\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /D "GSI_UNICODE" /D "HAVE_CONFIG_H" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 libspeex.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\libspeex\Release"

!ENDIF 

# Begin Target

# Name "Voice2Test - Win32 Release"
# Name "Voice2Test - Win32 Debug"
# Name "Voice2Test - Win32 Debug Unicode"
# Name "Voice2Test - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Voice2Test.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "VoiceSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gv.h
# End Source File
# Begin Source File

SOURCE=..\gvCodec.c
# End Source File
# Begin Source File

SOURCE=..\gvCodec.h
# End Source File
# Begin Source File

SOURCE=..\gvCustomDevice.c
# End Source File
# Begin Source File

SOURCE=..\gvCustomDevice.h
# End Source File
# Begin Source File

SOURCE=..\gvDevice.c
# End Source File
# Begin Source File

SOURCE=..\gvDevice.h
# End Source File
# Begin Source File

SOURCE=..\gvDirectSound.c
# ADD CPP /W3
# SUBTRACT CPP /WX
# End Source File
# Begin Source File

SOURCE=..\gvDirectSound.h
# End Source File
# Begin Source File

SOURCE=..\gvFrame.c
# End Source File
# Begin Source File

SOURCE=..\gvFrame.h
# End Source File
# Begin Source File

SOURCE=..\gvMain.c
# End Source File
# Begin Source File

SOURCE=..\gvMain.h
# End Source File
# Begin Source File

SOURCE=..\gvSource.c
# End Source File
# Begin Source File

SOURCE=..\gvSource.h
# End Source File
# Begin Source File

SOURCE=..\gvSpeex.c
# End Source File
# Begin Source File

SOURCE=..\gvSpeex.h
# End Source File
# Begin Source File

SOURCE=..\gvUtil.c
# End Source File
# Begin Source File

SOURCE=..\gvUtil.h
# End Source File
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

SOURCE=..\..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\common\win32\Win32Common.c
# End Source File
# End Group
# End Target
# End Project
