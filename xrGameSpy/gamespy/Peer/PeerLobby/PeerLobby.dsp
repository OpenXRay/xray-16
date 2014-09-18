# Microsoft Developer Studio Project File - Name="PeerLobby" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PeerLobby - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PeerLobby.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PeerLobby.mak" CFG="PeerLobby - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PeerLobby - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PeerLobby - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Peer/PeerLobby", MDWAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /ignore:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "IRC_LOG" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PeerLobby - Win32 Release"
# Name "PeerLobby - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ConnectPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CreatePage.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupPage.cpp
# End Source File
# Begin Source File

SOURCE=.\LobbyWizard.cpp
# End Source File
# Begin Source File

SOURCE=.\PeerLobby.cpp
# End Source File
# Begin Source File

SOURCE=.\PeerLobby.rc
# End Source File
# Begin Source File

SOURCE=.\StagingPage.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TitlePage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ConnectPage.h
# End Source File
# Begin Source File

SOURCE=.\CreatePage.h
# End Source File
# Begin Source File

SOURCE=.\GroupPage.h
# End Source File
# Begin Source File

SOURCE=.\LobbyWizard.h
# End Source File
# Begin Source File

SOURCE=.\PeerLobby.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StagingPage.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TitlePage.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\PeerLobby.ico
# End Source File
# Begin Source File

SOURCE=.\res\PeerLobby.rc2
# End Source File
# End Group
# Begin Group "PeerSDK"

# PROP Default_Filter ""
# Begin Group "ChatSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\chat\chat.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatChannel.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatChannel.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCrypt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatHandlers.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatHandlers.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatMain.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatSocket.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatSocket.h
# End Source File
# End Group
# Begin Group "Pinger"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\pinger\pinger.h
# End Source File
# Begin Source File

SOURCE=..\..\pinger\pingerMain.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "QueryReporting2SDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\qr2\qr2.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2.h
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2regkeys.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2regkeys.h
# End Source File
# End Group
# Begin Group "ServerBrowsingSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_crypt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_crypt.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_queryengine.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_server.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverbrowsing.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverbrowsing.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverlist.c
# SUBTRACT CPP /YX /Yc /Yu
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
# Begin Source File

SOURCE=..\peer.h
# End Source File
# Begin Source File

SOURCE=..\peerAutoMatch.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerAutoMatch.h
# End Source File
# Begin Source File

SOURCE=..\peerCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\peerGlobalCallbacks.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerGlobalCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\peerHost.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerHost.h
# End Source File
# Begin Source File

SOURCE=..\peerKeys.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerKeys.h
# End Source File
# Begin Source File

SOURCE=..\peerMain.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerMain.h
# End Source File
# Begin Source File

SOURCE=..\peerMangle.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerMangle.h
# End Source File
# Begin Source File

SOURCE=..\peerOperations.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerOperations.h
# End Source File
# Begin Source File

SOURCE=..\peerPing.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerPing.h
# End Source File
# Begin Source File

SOURCE=..\peerPlayers.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerPlayers.h
# End Source File
# Begin Source File

SOURCE=..\peerQR.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerQR.h
# End Source File
# Begin Source File

SOURCE=..\peerRooms.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerRooms.h
# End Source File
# Begin Source File

SOURCE=..\peerSB.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\peerSB.h
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

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\gsPlatformUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c

!IF  "$(CFG)" == "PeerLobby - Win32 Release"

!ELSEIF  "$(CFG)" == "PeerLobby - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

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
# End Target
# End Project
