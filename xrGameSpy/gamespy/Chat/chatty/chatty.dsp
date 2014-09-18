# Microsoft Developer Studio Project File - Name="chatty" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=chatty - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "chatty.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "chatty.mak" CFG="chatty - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "chatty - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "chatty - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Chat/chatty", ELLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "chatty - Win32 Release"

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

!ELSEIF  "$(CFG)" == "chatty - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FD /GZ /c
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
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBCD" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "chatty - Win32 Release"
# Name "chatty - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ChannelListDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ChannelModeDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\chatty.cpp
# End Source File
# Begin Source File

SOURCE=.\chatty.rc
# End Source File
# Begin Source File

SOURCE=.\chattyDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\chattyView.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ConnectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EnterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GetUserInfoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\KickReasonDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\SendRawDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SetPasswordDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SetTopicDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TalkDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChannelListDlg.h
# End Source File
# Begin Source File

SOURCE=.\ChannelModeDlg.h
# End Source File
# Begin Source File

SOURCE=.\chatty.h
# End Source File
# Begin Source File

SOURCE=.\chattyDoc.h
# End Source File
# Begin Source File

SOURCE=.\chattyView.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\ConnectDlg.h
# End Source File
# Begin Source File

SOURCE=.\EnterDlg.h
# End Source File
# Begin Source File

SOURCE=.\GetUserInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\KickReasonDlg.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SendRawDlg.h
# End Source File
# Begin Source File

SOURCE=.\SetPasswordDlg.h
# End Source File
# Begin Source File

SOURCE=.\SetTopicDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TalkDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\chatty.ico
# End Source File
# Begin Source File

SOURCE=.\res\chatty.rc2
# End Source File
# Begin Source File

SOURCE=.\res\chattyDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "Chat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\chat.h
# End Source File
# Begin Source File

SOURCE=..\chatCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\chatChannel.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatChannel.h
# End Source File
# Begin Source File

SOURCE=..\chatCrypt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatCrypt.h
# End Source File
# Begin Source File

SOURCE=..\chatHandlers.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatHandlers.h
# End Source File
# Begin Source File

SOURCE=..\chatMain.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatMain.h
# End Source File
# Begin Source File

SOURCE=..\chatSocket.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\chatSocket.h
# End Source File
# End Group
# Begin Group "common"

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

SOURCE=..\..\common\gsDebug.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.c
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=..\..\common\gsStringUtil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.h
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
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
