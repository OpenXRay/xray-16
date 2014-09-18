# Microsoft Developer Studio Project File - Name="pt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=pt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pt.mak" CFG="pt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/pt", FCGBAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=snLib.exe
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "pt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=snLib.exe
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "pt - Win32 Release"
# Name "pt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ptMain.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\pt.h
# End Source File
# End Group
# Begin Group "GsCommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\darray.c
# End Source File
# Begin Source File

SOURCE=..\darray.h
# End Source File
# Begin Source File

SOURCE=..\common\gsAssert.c
# End Source File
# Begin Source File

SOURCE=..\common\gsAssert.h
# End Source File
# Begin Source File

SOURCE=..\common\gsAvailable.c
# End Source File
# Begin Source File

SOURCE=..\common\gsAvailable.h
# End Source File
# Begin Source File

SOURCE=..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\common\gsCrypt.h
# End Source File
# Begin Source File

SOURCE=..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\common\gsLargeInt.h
# End Source File
# Begin Source File

SOURCE=..\common\gsMemory.c
# End Source File
# Begin Source File

SOURCE=..\common\gsMemory.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatform.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatform.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformSocket.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformSocket.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformThread.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformThread.h
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformUtil.c
# End Source File
# Begin Source File

SOURCE=..\common\gsPlatformUtil.h
# End Source File
# Begin Source File

SOURCE=..\common\gsRC4.h
# End Source File
# Begin Source File

SOURCE=..\common\gsSHA1.h
# End Source File
# Begin Source File

SOURCE=..\common\gsSSL.h
# End Source File
# Begin Source File

SOURCE=..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\hashtable.c
# End Source File
# Begin Source File

SOURCE=..\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\md5.h
# End Source File
# Begin Source File

SOURCE=..\md5c.c
# End Source File
# End Group
# Begin Group "HttpSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ghttp\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpASCII.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpBuffer.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpCommon.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpConnection.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpConnection.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpEncryption.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpMain.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpPost.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpProcess.c
# End Source File
# Begin Source File

SOURCE=..\ghttp\ghttpProcess.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\changelog.txt
# End Source File
# End Target
# End Project
