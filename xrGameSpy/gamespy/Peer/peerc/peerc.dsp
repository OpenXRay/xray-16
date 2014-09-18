# Microsoft Developer Studio Project File - Name="peerc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=peerc - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "peerc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "peerc.mak" CFG="peerc - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "peerc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "peerc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "peerc - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "peerc - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Peer/peerc", GSUAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "peerc - Win32 Release"

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
# ADD CPP /nologo /W3 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "peerc - Win32 Debug"

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
# ADD CPP /nologo /W3 /WX /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "peerc - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "peerc___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "peerc___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "UnicodeRelease"
# PROP Intermediate_Dir "UnicodeRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_UNICODE" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "peerc - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "peerc___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "peerc___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UnicodeDebug"
# PROP Intermediate_Dir "UnicodeDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /WX /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /WX /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "GSI_UNICODE" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "peerc - Win32 Release"
# Name "peerc - Win32 Debug"
# Name "peerc - Win32 Unicode Release"
# Name "peerc - Win32 Unicode Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\peerc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
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
# Begin Group "PeerSDK"

# PROP Default_Filter ""
# Begin Group "ServerBrowsingSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_crypt.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_queryengine.c
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_server.c
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverbrowsing.c
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverbrowsing.h
# End Source File
# Begin Source File

SOURCE=..\..\serverbrowsing\sb_serverlist.c
# End Source File
# End Group
# Begin Group "QueryReportingSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\qr2\qr2.c
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2.h
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2regkeys.c
# End Source File
# Begin Source File

SOURCE=..\..\qr2\qr2regkeys.h
# End Source File
# End Group
# Begin Group "ChatSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\chat\chat.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatChannel.c
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatChannel.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatHandlers.c
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatHandlers.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatMain.c
# End Source File
# Begin Source File

SOURCE=..\..\chat\chatMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Chat\chatSocket.c
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
# End Source File
# Begin Source File

SOURCE=..\peerAutoMatch.h
# End Source File
# Begin Source File

SOURCE=..\peerCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\peerCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\peerGlobalCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\peerGlobalCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\peerHost.c
# End Source File
# Begin Source File

SOURCE=..\peerHost.h
# End Source File
# Begin Source File

SOURCE=..\peerKeys.c
# End Source File
# Begin Source File

SOURCE=..\peerKeys.h
# End Source File
# Begin Source File

SOURCE=..\peerMain.c
# End Source File
# Begin Source File

SOURCE=..\peerMain.h
# End Source File
# Begin Source File

SOURCE=..\peerMangle.c
# End Source File
# Begin Source File

SOURCE=..\peerMangle.h
# End Source File
# Begin Source File

SOURCE=..\peerOperations.c
# End Source File
# Begin Source File

SOURCE=..\peerOperations.h
# End Source File
# Begin Source File

SOURCE=..\peerPing.c
# End Source File
# Begin Source File

SOURCE=..\peerPing.h
# End Source File
# Begin Source File

SOURCE=..\peerPlayers.c
# End Source File
# Begin Source File

SOURCE=..\peerPlayers.h
# End Source File
# Begin Source File

SOURCE=..\peerQR.c
# End Source File
# Begin Source File

SOURCE=..\peerQR.h
# End Source File
# Begin Source File

SOURCE=..\peerRooms.c
# End Source File
# Begin Source File

SOURCE=..\peerRooms.h
# End Source File
# Begin Source File

SOURCE=..\peerSB.c
# End Source File
# Begin Source File

SOURCE=..\peerSB.h
# End Source File
# End Group
# End Target
# End Project
