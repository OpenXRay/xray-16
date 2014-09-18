# Microsoft Developer Studio Project File - Name="ScRaceSample" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ScRaceSample - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scRaceSample.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scRaceSample.mak" CFG="ScRaceSample - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ScRaceSample - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ScRaceSample - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/sc/ScRaceSample", YNHCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ScRaceSample - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
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

!ELSEIF  "$(CFG)" == "ScRaceSample - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "RECV_LOG" /D "GSI_COMMON_DEBUG" /FD /GZ /c
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

# Name "ScRaceSample - Win32 Release"
# Name "ScRaceSample - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\HostOrJoinDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ScRaceSample.cpp
# End Source File
# Begin Source File

SOURCE=.\ScRaceSample.rc
# End Source File
# Begin Source File

SOURCE=.\ScRaceSampleDlg.cpp

!IF  "$(CFG)" == "ScRaceSample - Win32 Release"

!ELSEIF  "$(CFG)" == "ScRaceSample - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\WaitingDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\atlas_Competition_Race_Sample_App_v1.h
# End Source File
# Begin Source File

SOURCE=.\HostOrJoinDlg.h
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ScRaceSample.h
# End Source File
# Begin Source File

SOURCE=.\ScRaceSampleDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WaitingDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ScRaceSample.ico
# End Source File
# Begin Source File

SOURCE=.\res\ScRaceSample.rc2
# End Source File
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter ""
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

SOURCE=..\..\common\gsSoap.c
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
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\md5c.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "PresenceSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gp\gp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gp.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpi.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpi.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiBuddy.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiBuddy.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiBuffer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiCallback.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiConnect.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiConnect.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiInfo.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiKeys.c
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiKeys.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiOperation.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiOperation.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiPeer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiPeer.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiProfile.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiSearch.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiSearch.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiTransfer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiTransfer.h
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUnique.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\GP\gpiUnique.h
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiUtility.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gp\gpiUtility.h
# End Source File
# End Group
# Begin Group "HttpSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ghttp\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpConnection.c
# SUBTRACT CPP /YX /Yc /Yu
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
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.h
# End Source File
# End Group
# Begin Group "TransportSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gt2\gt2.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Auth.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Auth.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Buffer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Callback.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Callback.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Connection.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Connection.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Encode.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Encode.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Filter.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Filter.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Main.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Main.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Message.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Message.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Socket.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Socket.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Utility.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\gt2\gt2Utility.h
# End Source File
# End Group
# Begin Group "CompetitionSDK"

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
# Begin Group "AuthServiceSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\webservices\AuthService.c
# End Source File
# Begin Source File

SOURCE=..\..\webservices\AuthService.h
# End Source File
# End Group
# End Target
# End Project
