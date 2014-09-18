# Microsoft Developer Studio Project File - Name="sctestmatchless" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sctestmatchless - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sctestmatchless.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sctestmatchless.mak" CFG="sctestmatchless - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sctestmatchless - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "sctestmatchless - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "sctestmatchless - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "sctestmatchless - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sctestmatchless - Win32 Release"

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
# ADD CPP /nologo /W4 /GX /O2 /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Debug"

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
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "HTTP_LOG" /D "GSI_COMMON_DEBUG" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sctestmatchless___Win32_Unicode_Debug0"
# PROP BASE Intermediate_Dir "sctestmatchless___Win32_Unicode_Debug0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "sctestmatchless___Win32_Unicode_Debug0"
# PROP Intermediate_Dir "sctestmatchless___Win32_Unicode_Debug0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /Gm /GX /ZI /Od /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "HTTP_LOG" /D "GSI_COMMON_DEBUG" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "HTTP_LOG" /D "GSI_COMMON_DEBUG" /D "GSI_UNICODE" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sctestmatchless___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "sctestmatchless___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "sctestmatchless___Win32_Unicode_Release"
# PROP Intermediate_Dir "sctestmatchless___Win32_Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /GX /O2 /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W4 /GX /O2 /I ".." /I "../SCSoap" /I "../.." /I "../../gsoap" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_UNICODE" /YX /FD /c
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

# Name "sctestmatchless - Win32 Release"
# Name "sctestmatchless - Win32 Debug"
# Name "sctestmatchless - Win32 Unicode Debug"
# Name "sctestmatchless - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\sctestmatchless.c

!IF  "$(CFG)" == "sctestmatchless - Win32 Release"

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Debug"

# SUBTRACT CPP /WX

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Unicode Debug"

# SUBTRACT BASE CPP /WX
# SUBTRACT CPP /WX

!ELSEIF  "$(CFG)" == "sctestmatchless - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\win32\Win32Common.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "GOA"

# PROP Default_Filter ""
# Begin Group "SC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc.h
# End Source File
# Begin Source File

SOURCE=..\sci.h
# End Source File
# Begin Source File

SOURCE=..\sciInterface.c
# End Source File
# Begin Source File

SOURCE=..\sciInterface.h
# End Source File
# Begin Source File

SOURCE=..\sciMain.c
# End Source File
# Begin Source File

SOURCE=..\sciReport.c
# End Source File
# Begin Source File

SOURCE=..\sciReport.h
# End Source File
# Begin Source File

SOURCE=..\sciSerialize.c
# End Source File
# Begin Source File

SOURCE=..\sciSerialize.h
# End Source File
# Begin Source File

SOURCE=..\sciWebServices.c
# End Source File
# Begin Source File

SOURCE=..\sciWebServices.h
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

SOURCE=..\..\nonport.h
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
# Begin Group "webservices"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\webservices\AuthService.c
# End Source File
# Begin Source File

SOURCE=..\..\webservices\AuthService.h
# End Source File
# End Group
# End Group
# End Target
# End Project
