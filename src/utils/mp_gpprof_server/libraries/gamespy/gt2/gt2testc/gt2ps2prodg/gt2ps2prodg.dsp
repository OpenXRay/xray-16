# Microsoft Developer Studio Project File - Name="gt2ps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gt2ps2prodg - Win32 Debug_Insock
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gt2ps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gt2ps2prodg.mak" CFG="gt2ps2prodg - Win32 Debug_Insock"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gt2ps2prodg - Win32 Debug_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2ps2prodg - Win32 Debug_Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2ps2prodg - Win32 Debug_SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2ps2prodg - Win32 Release_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2ps2prodg - Win32 Release_Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2ps2prodg - Win32 Release_SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/gt2/gt2testc/gt2ps2prodg", XREDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gt2ps2prodg - Win32 Debug_EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_EENet"
# PROP BASE Intermediate_Dir "Debug_EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_EENet"
# PROP Intermediate_Dir "Debug_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /W4 /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gt2ps2prodg - Win32 Debug_Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gt2ps2prodg___Win32_Debug_Insock"
# PROP BASE Intermediate_Dir "gt2ps2prodg___Win32_Debug_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Insock"
# PROP Intermediate_Dir "Debug_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD CPP /nologo /W4 /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "INSOCK" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_Insock\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gt2ps2prodg - Win32 Debug_SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_SNSystems"
# PROP BASE Intermediate_Dir "Debug_SNSystems"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_SNSystems"
# PROP Intermediate_Dir "Debug_SNSystems"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /W4 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "_DEBUG" /D "SN_TARGET_PS2" /D "SN_SYSTEMS" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_SNSystems\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gt2ps2prodg - Win32 Release_EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Release_EENet"
# PROP BASE Intermediate_Dir "Release_EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_EENet"
# PROP Intermediate_Dir "Release_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Release/" /FD /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "EENET" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gt2ps2prodg - Win32 Release_Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gt2ps2prodg___Win32_Release_Insock"
# PROP BASE Intermediate_Dir "gt2ps2prodg___Win32_Release_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_Insock"
# PROP Intermediate_Dir "Release_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "SN_SYSTEMS" /FD /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "INSOCK" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /machine:IX86 /out:"Release_Insock\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gt2ps2prodg - Win32 Release_SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Release_SNSystems"
# PROP BASE Intermediate_Dir "Release_SNSystems"
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
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\gt2ps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\gt2ps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "gt2ps2prodg - Win32 Debug_EENet"
# Name "gt2ps2prodg - Win32 Debug_Insock"
# Name "gt2ps2prodg - Win32 Debug_SNSystems"
# Name "gt2ps2prodg - Win32 Release_EENet"
# Name "gt2ps2prodg - Win32 Release_Insock"
# Name "gt2ps2prodg - Win32 Release_SNSystems"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\gt2testc.c
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
# Begin Group "TransportSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gt2.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Auth.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Auth.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Callback.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Callback.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Connection.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Connection.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Encode.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Encode.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Filter.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Filter.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Main.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Main.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Message.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Message.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Socket.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Socket.h
# End Source File
# Begin Source File

SOURCE=..\..\gt2Utility.c
# End Source File
# Begin Source File

SOURCE=..\..\gt2Utility.h
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

SOURCE=..\..\..\hashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hashtable.h
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
