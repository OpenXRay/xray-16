# Microsoft Developer Studio Project File - Name="gpps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gpps2prodg - Win32 PS2 EE Release Insock
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gpps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gpps2prodg.mak" CFG="gpps2prodg - Win32 PS2 EE Release Insock"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gpps2prodg - Win32 PS2 EE Debug EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gpps2prodg - Win32 PS2 EE Debug SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "gpps2prodg - Win32 PS2 EE Debug Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "gpps2prodg - Win32 PS2 EE Release EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gpps2prodg - Win32 PS2 EE Release SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "gpps2prodg - Win32 PS2 EE Release Insock" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/GP/gptestc/gpps2prodg", PSEDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Debug EENet"

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
# ADD CPP /nologo /W4 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libcdvd.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Debug SNSystems"

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
# ADD CPP /nologo /W4 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_SYSTEMS" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_SNSystems\gpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Debug Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gpps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Intermediate_Dir "gpps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Insock"
# PROP Intermediate_Dir "Debug_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD CPP /nologo /W4 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "_DEBUG" /D "INSOCK" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libcdvd.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_Insock\gpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Release EENet"

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
# ADD CPP /nologo /W4 /WX /O2 /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "EENET" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libcdvd.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\gpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Release SNSystems"

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
# ADD CPP /nologo /W4 /WX /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "SN_SYSTEMS" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\gpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gpps2prodg - Win32 PS2 EE Release Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gpps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Intermediate_Dir "gpps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_Insock"
# PROP Intermediate_Dir "Release_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "_DEBUG" /D "INSOCK" /FD /debug /c
# ADD CPP /nologo /W4 /WX /O2 /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "INSOCK" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Release_Insock\gpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Release_Insock\gpps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "gpps2prodg - Win32 PS2 EE Debug EENet"
# Name "gpps2prodg - Win32 PS2 EE Debug SNSystems"
# Name "gpps2prodg - Win32 PS2 EE Debug Insock"
# Name "gpps2prodg - Win32 PS2 EE Release EENet"
# Name "gpps2prodg - Win32 PS2 EE Release SNSystems"
# Name "gpps2prodg - Win32 PS2 EE Release Insock"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\gptestc.c
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
# Begin Group "PresenceSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gp.c
# End Source File
# Begin Source File

SOURCE=..\..\gp.h
# End Source File
# Begin Source File

SOURCE=..\..\gpi.c
# End Source File
# Begin Source File

SOURCE=..\..\gpi.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiBuddy.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiBuddy.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiCallback.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiConnect.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiConnect.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiInfo.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiOperation.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiOperation.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiPeer.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiPeer.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiProfile.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiSearch.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiSearch.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiTransfer.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiTransfer.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiUnique.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiUnique.h
# End Source File
# Begin Source File

SOURCE=..\..\gpiUtility.c
# End Source File
# Begin Source File

SOURCE=..\..\gpiUtility.h
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
