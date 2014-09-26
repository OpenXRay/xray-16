# Microsoft Developer Studio Project File - Name="ghttpps2prodg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ghttpps2prodg - Win32 PS2 EE Debug Insock
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ghttpps2prodg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ghttpps2prodg.mak" CFG="ghttpps2prodg - Win32 PS2 EE Debug Insock"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Debug EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Debug Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Debug SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Release EENet" (based on "Win32 (x86) Console Application")
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Release Insock" (based on "Win32 (x86) Console Application")
!MESSAGE "ghttpps2prodg - Win32 PS2 EE Release SNSystems" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/ghttp/ghttpc/ghttpps2prodg", JSEDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Debug EENet"

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
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Debug Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ghttpps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Intermediate_Dir "ghttpps2prodg___Win32_PS2_EE_Debug_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Insock"
# PROP Intermediate_Dir "Debug_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "EENET" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD CPP /nologo /W4 /Od /I "c:\usr\local\sce\ee\include\libeenet" /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "INSOCK" /D "SN_TARGET_PS2" /D "_DEBUG" /FD /debug /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_EENet\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_Insock\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Debug SNSystems"

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
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"PS2_EE_Debug\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libcdvd.a sneetcp.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /debug /machine:IX86 /out:"Debug_SNSystems\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Release EENet"

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
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 ent_smap.a ent_eth.a ent_ppp.a eenetctl.a libeenet.a libscf.a libcdvd.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_EENet\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Release Insock"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ghttpps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Intermediate_Dir "ghttpps2prodg___Win32_PS2_EE_Release_Insock"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release_Insock"
# PROP Intermediate_Dir "Release_Insock"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "SN_SYSTEMS" /FD /c
# ADD CPP /nologo /W4 /WX /O2 /I "C:\usr\local\sce\ee\include" /I "C:\usr\local\sce\common\include" /D "SN_TARGET_PS2" /D "INSOCK" /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 libcdvd.a sneetcp.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libinsck.a libnet.a libmrpc.a libkernl.a libcdvd.a libnetif.a netcnfif.a /nologo /pdb:none /machine:IX86 /out:"Release_Insock\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ELSEIF  "$(CFG)" == "ghttpps2prodg - Win32 PS2 EE Release SNSystems"

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
# ADD BASE LINK32 libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"PS2_EE_Release\ghttpps2prodg.elf" /D:SN_TARGET_PS2
# ADD LINK32 libcdvd.a sneetcp.a libsn.a libgraph.a libdma.a libdev.a libpad.a libpkt.a libvu0.a /nologo /pdb:none /machine:IX86 /out:"Release_SNSystems\ghttpps2prodg.elf" /D:SN_TARGET_PS2

!ENDIF 

# Begin Target

# Name "ghttpps2prodg - Win32 PS2 EE Debug EENet"
# Name "ghttpps2prodg - Win32 PS2 EE Debug Insock"
# Name "ghttpps2prodg - Win32 PS2 EE Debug SNSystems"
# Name "ghttpps2prodg - Win32 PS2 EE Release EENet"
# Name "ghttpps2prodg - Win32 PS2 EE Release Insock"
# Name "ghttpps2prodg - Win32 PS2 EE Release SNSystems"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\ghttpc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\common\ps2\prodg\PS2_in_VC.h
# End Source File
# End Group
# Begin Group "HttpSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpASCII.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpCommon.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpConnection.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpEncryption.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpEncryption.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpMain.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpPost.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttpProcess.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttpProcess.h
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

SOURCE=..\..\..\common\gsCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsLargeInt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsLargeInt.h
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

SOURCE=..\..\..\common\gsRC4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsRC4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsSHA1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsSHA1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsSSL.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsSSL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsXML.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\gsXML.h
# End Source File
# Begin Source File

SOURCE=..\..\..\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\md5c.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\ps2\ps2common.c
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
