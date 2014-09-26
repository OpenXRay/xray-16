# Microsoft Developer Studio Project File - Name="voice2benchps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=voice2benchps2prodg - Win32 Release_EENet
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "voice2benchps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "voice2benchps2prodg.mak" CFG="voice2benchps2prodg - Win32 Release_EENet"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "voice2benchps2prodg - Win32 Debug_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "voice2benchps2prodg - Win32 Release_EENet" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Gamespy/GOA/Voice2/voice2bench/voice2benchps2prodg"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "voice2benchps2prodg - Win32 Debug_EENet"

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
# ADD CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "GSI_VOICE" /Fo"PS2_EE_Debug/" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\voice2benchps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libc.a eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\voice2benchps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "voice2benchps2prodg - Win32 Release_EENet"

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
# ADD CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "GSI_VOICE" /Fo"PS2_EE_Release/" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\voice2benchps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 eenetctl.a ent_smap.a ent_eth.a ent_ppp.a libeenet.a libscf.a liblgaud.a liblgcodec.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\voice2benchps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "voice2benchps2prodg - Win32 Debug_EENet"
# Name "voice2benchps2prodg - Win32 Release_EENet"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\gvLogitechPS2Codecs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\nonport.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2common.c
# End Source File
# Begin Source File

SOURCE=..\..\voice2bench\voice2bench.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\gvLogitechPS2Codecs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\nonport.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\PS2_in_VC.h
# End Source File
# Begin Source File

SOURCE=..\..\voice2bench\voicesample.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\..\usr\local\sce\ee\lib\app.cmd
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\usr\local\sce\ee\lib\crt0.s
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\ps2.lk
# End Source File
# End Target
# End Project
