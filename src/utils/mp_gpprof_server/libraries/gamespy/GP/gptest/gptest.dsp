# Microsoft Developer Studio Project File - Name="gptest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=gptest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gptest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gptest.mak" CFG="gptest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gptest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "gptest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gptest - Win32 Release"

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
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "gptest - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\\" /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /FD /GZ /c
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
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gptest - Win32 Release"
# Name "gptest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\gptest.cpp
# End Source File
# Begin Source File

SOURCE=.\gptest.rc
# End Source File
# Begin Source File

SOURCE=.\gptestDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\gptest.h
# End Source File
# Begin Source File

SOURCE=.\gptestDlg.h
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

SOURCE=.\res\gptest.ico
# End Source File
# Begin Source File

SOURCE=.\res\gptest.rc2
# End Source File
# End Group
# Begin Group "PresenceSDK"

# PROP Default_Filter "c,h"
# Begin Source File

SOURCE=..\gp.c
# End Source File
# Begin Source File

SOURCE=..\gpi.c
# End Source File
# Begin Source File

SOURCE=..\gpi.h
# End Source File
# Begin Source File

SOURCE=..\gpiBuddy.c
# End Source File
# Begin Source File

SOURCE=..\gpiBuddy.h
# End Source File
# Begin Source File

SOURCE=..\gpiBuffer.c
# End Source File
# Begin Source File

SOURCE=..\gpiBuffer.h
# End Source File
# Begin Source File

SOURCE=..\gpiCallback.c
# End Source File
# Begin Source File

SOURCE=..\gpiCallback.h
# End Source File
# Begin Source File

SOURCE=..\gpiConnect.c
# End Source File
# Begin Source File

SOURCE=..\gpiConnect.h
# End Source File
# Begin Source File

SOURCE=..\gpiInfo.c
# End Source File
# Begin Source File

SOURCE=..\gpiInfo.h
# End Source File
# Begin Source File

SOURCE=..\gpiKeys.c
# End Source File
# Begin Source File

SOURCE=..\gpiKeys.h
# End Source File
# Begin Source File

SOURCE=..\gpiOperation.c
# End Source File
# Begin Source File

SOURCE=..\gpiOperation.h
# End Source File
# Begin Source File

SOURCE=..\gpiPeer.c
# End Source File
# Begin Source File

SOURCE=..\gpiPeer.h
# End Source File
# Begin Source File

SOURCE=..\gpiProfile.c
# End Source File
# Begin Source File

SOURCE=..\gpiProfile.h
# End Source File
# Begin Source File

SOURCE=..\gpiSearch.c
# End Source File
# Begin Source File

SOURCE=..\gpiSearch.h
# End Source File
# Begin Source File

SOURCE=..\gpiTransfer.c
# End Source File
# Begin Source File

SOURCE=..\gpiTransfer.h
# End Source File
# Begin Source File

SOURCE=..\gpiUnique.c
# End Source File
# Begin Source File

SOURCE=..\gpiUnique.h
# End Source File
# Begin Source File

SOURCE=..\gpiUtility.c
# End Source File
# Begin Source File

SOURCE=..\gpiUtility.h
# End Source File
# End Group
# Begin Group "TransportSDK"

# PROP Default_Filter "c,h"
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
# Begin Group "GsCommon"

# PROP Default_Filter "c,h"
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

SOURCE=..\..\common\gsAvailable.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsUdpEngine.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsUdpEngine.h
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
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
