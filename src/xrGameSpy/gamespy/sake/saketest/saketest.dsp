# Microsoft Developer Studio Project File - Name="saketest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=saketest - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "saketest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "saketest.mak" CFG="saketest - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "saketest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "saketest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "saketest - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "saketest - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Gamespy/GOA/sake/saketest"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "saketest - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "saketest - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_MEM_MANAGED" /D "GSI_COMMON_DEBUG" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "saketest - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "saketest___Win32_Unicode_Debug1"
# PROP BASE Intermediate_Dir "saketest___Win32_Unicode_Debug1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "saketest___Win32_Unicode_Debug1"
# PROP Intermediate_Dir "saketest___Win32_Unicode_Debug1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_MEM_MANAGED" /D "GSI_COMMON_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_MEM_MANAGED" /D "GSI_COMMON_DEBUG" /D "GSI_UNICODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "saketest - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "saketest___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "saketest___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "saketest___Win32_Unicode_Release"
# PROP Intermediate_Dir "saketest___Win32_Unicode_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../" /I "../../" /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ENDIF 

# Begin Target

# Name "saketest - Win32 Release"
# Name "saketest - Win32 Debug"
# Name "saketest - Win32 Unicode Debug"
# Name "saketest - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\saketest.c
# End Source File
# End Group
# Begin Group "GOA"

# PROP Default_Filter ""
# Begin Group "sake"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sake.h
# End Source File
# Begin Source File

SOURCE=..\sakeMain.c
# End Source File
# Begin Source File

SOURCE=..\sakeMain.h
# End Source File
# Begin Source File

SOURCE=..\sakeRequest.c
# End Source File
# Begin Source File

SOURCE=..\sakeRequest.h
# End Source File
# Begin Source File

SOURCE=..\sakeRequestInternal.h
# End Source File
# Begin Source File

SOURCE=..\sakeRequestMisc.c
# End Source File
# Begin Source File

SOURCE=..\sakeRequestModify.c
# End Source File
# Begin Source File

SOURCE=..\sakeRequestRead.c
# End Source File
# End Group
# Begin Group "Common"

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

SOURCE=..\..\common\gsCore.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCore.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsLargeInt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsLargeInt.h
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

SOURCE=..\..\common\gsRC4.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsRC4.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSHA1.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSHA1.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSoap.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSoap.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSSL.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSSL.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsUdpEngine.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsUdpEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsXML.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsXML.h
# End Source File
# Begin Source File

SOURCE=..\..\hashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\md5c.c
# End Source File
# Begin Source File

SOURCE=..\..\common\win32\Win32Common.c
# End Source File
# End Group
# Begin Group "ghttp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ghttp\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpASCII.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpConnection.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpEncryption.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpEncryption.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpMain.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.h
# End Source File
# End Group
# Begin Group "gp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\GP\gp.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gp.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpi.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpi.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiBuddy.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiBuddy.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiCallback.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiConnect.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiConnect.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiInfo.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiKeys.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiKeys.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiOperation.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiOperation.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiPeer.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiPeer.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiProfile.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiSearch.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiSearch.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiTransfer.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiTransfer.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUnique.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUnique.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUtility.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUtility.h
# End Source File
# End Group
# Begin Group "gt2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gt2\gt2.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Auth.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Auth.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Callback.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Callback.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Connection.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Connection.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Encode.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Encode.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Filter.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Filter.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Main.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Main.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Message.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Message.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Socket.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Socket.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Utility.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Utility.h
# End Source File
# End Group
# End Group
# End Target
# End Project
