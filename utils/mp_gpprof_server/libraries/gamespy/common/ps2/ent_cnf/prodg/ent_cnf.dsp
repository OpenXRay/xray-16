# Microsoft Developer Studio Project File - Name="ent_cnf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ent_cnf - Win32 PS2 IOP Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ent_cnf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ent_cnf.mak" CFG="ent_cnf - Win32 PS2 IOP Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ent_cnf - Win32 PS2 IOP Debug" (based on "Win32 (x86) Application")
!MESSAGE "ent_cnf - Win32 PS2 IOP Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/ps2common/ent_cnf/prodg", UXEDAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ent_cnf - Win32 PS2 IOP Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ent_cnf___Win32_PS2_IOP_Debug"
# PROP BASE Intermediate_Dir "ent_cnf___Win32_PS2_IOP_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ent_cnf___Win32_PS2_IOP_Debug"
# PROP Intermediate_Dir "ent_cnf___Win32_PS2_IOP_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /w /W0 /Od /I "C:\usr\local\sce\ee\sample\libeenet\ent_cnf" /D "SN_TARGET_PS2_IOP" /Fo"PS2_IOP_Debug/" /FD -G0   /debug  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 eenetctl.ilb netcnf.ilb C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.o /nologo /pdb:none /debug /machine:IX86 /out:"c:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnf.irx" /D:SN_TARGET_PS2_IOP
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=ioplibgen C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnf.tbl -e C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.s	ps2cc -iop -o C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.o C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.s
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ent_cnf - Win32 PS2 IOP Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ent_cnf___Win32_PS2_IOP_Release"
# PROP BASE Intermediate_Dir "ent_cnf___Win32_PS2_IOP_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ent_cnf___Win32_PS2_IOP_Release"
# PROP Intermediate_Dir "ent_cnf___Win32_PS2_IOP_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /w /W0 /O2 /I "C:\usr\local\sce\ee\sample\libeenet\ent_cnf" /D "SN_TARGET_PS2_IOP" /Fo"PS2_IOP_Release/" /FD -G0   /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 eenetctl.ilb netcnf.ilb C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.o /nologo /pdb:none /machine:IX86 /out:"c:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnf.irx" /D:SN_TARGET_PS2_IOP
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=ioplibgen C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnf.tbl -e C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.s	ps2cc -iop -o C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.o C:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnfent.s
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ent_cnf - Win32 PS2 IOP Debug"
# Name "ent_cnf - Win32 PS2 IOP Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=c:\usr\local\sce\iop\sample\libeenet\ent_cnf\ent_cnf.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=c:\usr\local\sce\iop\sample\libeenet\ent_cnf\ioptypes.h
# End Source File
# Begin Source File

SOURCE=..\..\prodg\PS2_in_VC.h
# End Source File
# End Group
# End Target
# End Project
