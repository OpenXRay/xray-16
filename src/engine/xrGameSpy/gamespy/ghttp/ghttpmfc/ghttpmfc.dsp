# Microsoft Developer Studio Project File - Name="ghttpmfc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ghttpmfc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ghttpmfc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ghttpmfc.mak" CFG="ghttpmfc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ghttpmfc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ghttpmfc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/ghttp/ghttpmfc", PUZAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ghttpmfc - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "ghttpmfc - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "HTTP_LOG" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ghttpmfc - Win32 Release"
# Name "ghttpmfc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ghttpmfc.cpp
# End Source File
# Begin Source File

SOURCE=.\ghttpmfc.rc
# End Source File
# Begin Source File

SOURCE=.\ghttpmfcDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ghttpmfc.h
# End Source File
# Begin Source File

SOURCE=.\ghttpmfcDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ghttpmfc.ico
# End Source File
# Begin Source File

SOURCE=.\res\ghttpmfc.rc2
# End Source File
# End Group
# Begin Group "HttpSDK"

# PROP Default_Filter "c,h"
# Begin Source File

SOURCE=..\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\ghttpBuffer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\ghttpCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\ghttpCommon.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\ghttpConnection.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpConnection.h
# End Source File
# Begin Source File

SOURCE=..\ghttpEncryption.c
# End Source File
# Begin Source File

SOURCE=..\ghttpEncryption.h
# End Source File
# Begin Source File

SOURCE=..\ghttpMain.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\ghttpPost.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\ghttpProcess.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ghttpProcess.h
# End Source File
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter "c,h"
# Begin Source File

SOURCE=..\..\darray.c
# SUBTRACT CPP /YX /Yc /Yu
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
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c
# SUBTRACT CPP /YX /Yc /Yu
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
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.c
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=..\..\common\gsSSL.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSSL.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=..\..\md5c.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
