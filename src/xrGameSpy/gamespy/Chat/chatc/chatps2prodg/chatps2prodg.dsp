# Microsoft Developer Studio Project File - Name="chatps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=chatps2prodg - Win32 PS2 EE Release Insock
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "chatps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "chatps2prodg.mak" CFG="chatps2prodg - Win32 PS2 EE Release Insock"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "chatps2prodg - Win32 PS2 EE Debug EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "chatps2prodg - Win32 PS2 EE Debug SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "chatps2prodg - Win32 PS2 EE Debug Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "chatps2prodg - Win32 PS2 EE Release EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "chatps2prodg - Win32 PS2 EE Release SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "chatps2prodg - Win32 PS2 EE Release Insock" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Chat/chatc/chatps2prodg", DSEDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Debug EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PS2 EE Debug EENet"
# PROP BASE Intermediate_Dir "PS2 EE Debug EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_EENet"
# PROP Intermediate_Dir "Debug_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /I "C:\usr\local\sce\ee\include\libeenet" /D "SN_TARGET_PS2" /D "_DEBUG" /D "EENET" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libcdvd.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\chatps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Debug SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PS2 EE Debug SNSystems"
# PROP BASE Intermediate_Dir "PS2 EE Debug SNSystems"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_SNSystems"
# PROP Intermediate_Dir "Debug_SNSystems"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "_DEBUG" /D "SN_SYSTEMS" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_SNSystems\chatps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Debug Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "chatps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Intermediate_Dir "chatps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Insock"
# PROP Intermediate_Dir "Debug_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /I "C:\usr\local\sce\ee\include\libeenet" /D "SN_TARGET_PS2" /D "_DEBUG" /D "EENET" /FD /debug /c
# ADD CPP /nologo /W3 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "_DEBUG" /D "INSOCK" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libcdvd.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_Insock\chatps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Release EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PS2 EE Release EENet"
# PROP BASE Intermediate_Dir "PS2 EE Release EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_EENet"
# PROP Intermediate_Dir "Release_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Release/" /FD /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /I "C:\usr\local\sce\ee\include\libeenet" /D "SN_TARGET_PS2" /D "EENET" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libcdvd.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\chatps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Release SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PS2 EE Release SNSystems"
# PROP BASE Intermediate_Dir "PS2 EE Release SNSystems"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_SNSystems"
# PROP Intermediate_Dir "Release_SNSystems"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Release/" /FD /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "SN_SYSTEMS" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\chatps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "chatps2prodg - Win32 PS2 EE Release Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "chatps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Intermediate_Dir "chatps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "chatps2prodg___Win32_PS2_EE_Release_Insock"
# PROP Intermediate_Dir "chatps2prodg___Win32_PS2_EE_Release_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "_DEBUG" /D "INSOCK" /Fo"Debug_Insock/" /Fd"Debug_Insock/" /FD /debug /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "INSOCK" /Fo"Release_Insock/" /Fd"Release_Insock/" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_Insock\chatps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Release_Insock\chatps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "chatps2prodg - Win32 PS2 EE Debug EENet"
# Name "chatps2prodg - Win32 PS2 EE Debug SNSystems"
# Name "chatps2prodg - Win32 PS2 EE Debug Insock"
# Name "chatps2prodg - Win32 PS2 EE Release EENet"
# Name "chatps2prodg - Win32 PS2 EE Release SNSystems"
# Name "chatps2prodg - Win32 PS2 EE Release Insock"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\chatc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2common.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\PS2_in_VC.h
# End Source File
# End Group
# Begin Group "ChatSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\chat.h
# End Source File
# Begin Source File

SOURCE=..\..\chatCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\chatCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\chatChannel.c
# End Source File
# Begin Source File

SOURCE=..\..\chatChannel.h
# End Source File
# Begin Source File

SOURCE=..\..\chatCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\chatCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\chatHandlers.c
# End Source File
# Begin Source File

SOURCE=..\..\chatHandlers.h
# End Source File
# Begin Source File

SOURCE=..\..\chatMain.c
# End Source File
# Begin Source File

SOURCE=..\..\chatMain.h
# End Source File
# Begin Source File

SOURCE=..\..\chatSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\chatSocket.h
# End Source File
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\darray.c
# End Source File
# Begin Source File

SOURCE=..\..\..\darray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsAssert.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsAssert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsAvailable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsMemory.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\hashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\md5c.c
# End Source File
# End Group
# Begin Source File

SOURCE=c:\usr\local\sce\ee\lib\app.cmd
# End Source File
# Begin Source File

SOURCE=c:\usr\local\sce\ee\lib\crt0.s
# End Source File
# Begin Source File

SOURCE=..\..\..\ps2common\prodg\ps2.lk
# End Source File
# End Target
# End Project
