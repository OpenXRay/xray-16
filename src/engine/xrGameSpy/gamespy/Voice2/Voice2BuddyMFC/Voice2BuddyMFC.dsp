# Microsoft Developer Studio Project File - Name="Voice2BuddyMFC" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Voice2BuddyMFC - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Voice2BuddyMFC.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Voice2BuddyMFC.mak" CFG="Voice2BuddyMFC - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Voice2BuddyMFC - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Voice2BuddyMFC - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName "$/IGNCP/Gamespy/GOA/Voice2/voice2buddymfc"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Voice2BuddyMFC - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /WX /GX /O2 /I "..\speex-1.0.5\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 dsound.lib dxguid.lib libspeex.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\libspeex\Release"

!ELSEIF  "$(CFG)" == "Voice2BuddyMFC - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\speex-1.0.5\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FD /GZ /c
# SUBTRACT CPP /WX /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dsound.lib dxguid.lib libspeex.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\libspeex\Debug"

!ENDIF 

# Begin Target

# Name "Voice2BuddyMFC - Win32 Release"
# Name "Voice2BuddyMFC - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SetupDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\Voice2BuddyMFC.cpp
# End Source File
# Begin Source File

SOURCE=.\Voice2BuddyMFC.rc
# End Source File
# Begin Source File

SOURCE=.\Voice2BuddyMFCDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\VoiceSessionDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SetupDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Voice2BuddyMFC.h
# End Source File
# Begin Source File

SOURCE=.\Voice2BuddyMFCDlg.h
# End Source File
# Begin Source File

SOURCE=.\VoiceSessionDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\gamespyl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logo270x83.bmp
# End Source File
# Begin Source File

SOURCE=".\res\microsoft-microphone.bmp"
# End Source File
# Begin Source File

SOURCE=".\res\microsoft-speaker.bmp"
# End Source File
# Begin Source File

SOURCE=.\res\speaking.bmp
# End Source File
# Begin Source File

SOURCE=.\res\speaking2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Voice2BuddyMFC.ico
# End Source File
# Begin Source File

SOURCE=.\res\Voice2BuddyMFC.rc2
# End Source File
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
# Begin Group "PresenceSDK"

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
# Begin Group "TransportSDK"

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
# Begin Group "NatNegSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\natneg\NATify.c
# End Source File
# Begin Source File

SOURCE=..\..\natneg\NATify.h
# End Source File
# Begin Source File

SOURCE=..\..\natneg\natneg.c
# End Source File
# Begin Source File

SOURCE=..\..\natneg\natneg.h
# End Source File
# Begin Source File

SOURCE=..\..\natneg\nninternal.h
# End Source File
# End Group
# End Target
# End Project
