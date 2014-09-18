# Microsoft Developer Studio Project File - Name="gvps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gvps2prodg - Win32 Release_SNSystems
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gvps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gvps2prodg.mak" CFG="gvps2prodg - Win32 Release_SNSystems"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gvps2prodg - Win32 Debug_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gvps2prodg - Win32 Release_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "gvps2prodg - Win32 Debug_SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "gvps2prodg - Win32 Release_SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/Voice2/Voice2Test/gvps2prodg", XPODAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gvps2prodg - Win32 Debug_EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gvps2prodg___Win32_Debug_EENet"
# PROP BASE Intermediate_Dir "gvps2prodg___Win32_Debug_EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_EENet"
# PROP Intermediate_Dir "Debug_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "_DEBUG" /D "SN_TARGET_PS2" /D "GSI_VOICE" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\gvps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libc.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libipu.a libsdr.a liblgvid.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gvps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gvps2prodg - Win32 Release_EENet"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gvps2prodg___Win32_Release_EENet"
# PROP BASE Intermediate_Dir "gvps2prodg___Win32_Release_EENet"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_EENet"
# PROP Intermediate_Dir "Release_EENet"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /Fo"PS2_EE_Release/" /FD /c
# ADD CPP /nologo /W4 /WX /O2 /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "GSI_VOICE" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\gvps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libipu.a libsdr.a liblgvid.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\gvps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gvps2prodg - Win32 Debug_SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gvps2prodg___Win32_Debug_SNSystems"
# PROP BASE Intermediate_Dir "gvps2prodg___Win32_Debug_SNSystems"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_SNSystems"
# PROP Intermediate_Dir "Debug_SNSystems"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD CPP /nologo /W4 /WX /Od /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "_DEBUG" /D "SN_SYSTEMS" /D "SN_TARGET_PS2" /D "GSI_VOICE" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libc.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\gvps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libc.a sneetcp.a libsdr.a liblgvid.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a libipu.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_SNSystems\gvps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "gvps2prodg - Win32 Release_SNSystems"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gvps2prodg___Win32_Release_SNSystems"
# PROP BASE Intermediate_Dir "gvps2prodg___Win32_Release_SNSystems"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_SNSystems"
# PROP Intermediate_Dir "Release_SNSystems"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /O2 /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "EENET" /Fo"PS2_EE_Release/" /FD /c
# ADD CPP /nologo /W4 /WX /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_SYSTEMS" /D "SN_TARGET_PS2" /D "GSI_VOICE" /D GV_CUSTOM_SOURCE_TYPE=SOCKADDR_IN /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\gvps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 sneetcp.a libipu.a libsdr.a liblgvid.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\gvps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "gvps2prodg - Win32 Debug_EENet"
# Name "gvps2prodg - Win32 Release_EENet"
# Name "gvps2prodg - Win32 Debug_SNSystems"
# Name "gvps2prodg - Win32 Release_SNSystems"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2pad.c
# End Source File
# Begin Source File

SOURCE=..\Voice2Test.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\PS2_in_VC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2pad.h
# End Source File
# End Group
# Begin Group "GSI"

# PROP Default_Filter ""
# Begin Group "VoiceSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gv.h
# End Source File
# Begin Source File

SOURCE=..\..\gvCodec.c
# End Source File
# Begin Source File

SOURCE=..\..\gvCodec.h
# End Source File
# Begin Source File

SOURCE=..\..\gvCustomDevice.c
# End Source File
# Begin Source File

SOURCE=..\..\gvCustomDevice.h
# End Source File
# Begin Source File

SOURCE=..\..\gvDevice.c
# End Source File
# Begin Source File

SOURCE=..\..\gvDevice.h
# End Source File
# Begin Source File

SOURCE=..\..\gvFrame.c
# End Source File
# Begin Source File

SOURCE=..\..\gvFrame.h
# End Source File
# Begin Source File

SOURCE=..\..\gvLogitechPS2Codecs.c
# End Source File
# Begin Source File

SOURCE=..\..\gvLogitechPS2Codecs.h
# End Source File
# Begin Source File

SOURCE=..\..\gvMain.c
# End Source File
# Begin Source File

SOURCE=..\..\gvMain.h
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Audio.c
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Audio.h
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Eyetoy.c
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Eyetoy.h
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Headset.c
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Headset.h
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Spu2.c
# End Source File
# Begin Source File

SOURCE=..\..\gvPS2Spu2.h
# End Source File
# Begin Source File

SOURCE=..\..\gvSource.c
# End Source File
# Begin Source File

SOURCE=..\..\gvSource.h
# End Source File
# Begin Source File

SOURCE=..\..\gvUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\gvUtil.h
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

SOURCE=..\..\..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsPlatformUtil.c
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=c:\usr\local\sce\ee\lib\app.cmd
# End Source File
# Begin Source File

SOURCE=..\..\changelog.txt
# End Source File
# Begin Source File

SOURCE=c:\usr\local\sce\ee\lib\crt0.s
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\ps2.lk
# End Source File
# End Target
# End Project
