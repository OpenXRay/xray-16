# Microsoft Developer Studio Project File - Name="gt2action" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gt2action - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gt2action.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gt2action.mak" CFG="gt2action - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gt2action - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "gt2action - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Gamespy/GOA/gt2/gt2action", DQOCAAAA"
# PROP Scc_LocalPath "."
CPP=snCl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gt2action - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"gt2action.exe" /ignore:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "gt2action - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=snBsc.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=snLink.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"gt2action.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gt2action - Win32 Release"
# Name "gt2action - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\gt2aClient.c
# End Source File
# Begin Source File

SOURCE=.\gt2aDisplay.c
# End Source File
# Begin Source File

SOURCE=.\gt2aInput.c
# End Source File
# Begin Source File

SOURCE=.\gt2aLogic.c
# End Source File
# Begin Source File

SOURCE=.\gt2aMain.c
# End Source File
# Begin Source File

SOURCE=.\gt2aMath.c
# End Source File
# Begin Source File

SOURCE=.\gt2aParse.c
# End Source File
# Begin Source File

SOURCE=.\gt2aServer.c
# End Source File
# Begin Source File

SOURCE=.\gt2aSound.c
# End Source File
# Begin Source File

SOURCE=.\TGAFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\gt2aClient.h
# End Source File
# Begin Source File

SOURCE=.\gt2aDisplay.h
# End Source File
# Begin Source File

SOURCE=.\gt2aInput.h
# End Source File
# Begin Source File

SOURCE=.\gt2aLogic.h
# End Source File
# Begin Source File

SOURCE=.\gt2aMain.h
# End Source File
# Begin Source File

SOURCE=.\gt2aMath.h
# End Source File
# Begin Source File

SOURCE=.\gt2aParse.h
# End Source File
# Begin Source File

SOURCE=.\gt2aServer.h
# End Source File
# Begin Source File

SOURCE=.\gt2aSound.h
# End Source File
# Begin Source File

SOURCE=.\TGAFile.h
# End Source File
# End Group
# Begin Group "TransportSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gt2.h
# End Source File
# Begin Source File

SOURCE=..\gt2Auth.c
# End Source File
# Begin Source File

SOURCE=..\gt2Auth.h
# End Source File
# Begin Source File

SOURCE=..\gt2Buffer.c
# End Source File
# Begin Source File

SOURCE=..\gt2Buffer.h
# End Source File
# Begin Source File

SOURCE=..\gt2Callback.c
# End Source File
# Begin Source File

SOURCE=..\gt2Callback.h
# End Source File
# Begin Source File

SOURCE=..\gt2Connection.c
# End Source File
# Begin Source File

SOURCE=..\gt2Connection.h
# End Source File
# Begin Source File

SOURCE=..\gt2Encode.c
# End Source File
# Begin Source File

SOURCE=..\gt2Encode.h
# End Source File
# Begin Source File

SOURCE=..\gt2Filter.c
# End Source File
# Begin Source File

SOURCE=..\gt2Filter.h
# End Source File
# Begin Source File

SOURCE=..\gt2Main.c
# End Source File
# Begin Source File

SOURCE=..\gt2Main.h
# End Source File
# Begin Source File

SOURCE=..\gt2Message.c
# End Source File
# Begin Source File

SOURCE=..\gt2Message.h
# End Source File
# Begin Source File

SOURCE=..\gt2Socket.c
# End Source File
# Begin Source File

SOURCE=..\gt2Socket.h
# End Source File
# Begin Source File

SOURCE=..\gt2Utility.c
# End Source File
# Begin Source File

SOURCE=..\gt2Utility.h
# End Source File
# End Group
# Begin Group "HttpSDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ghttp\ghttp.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpASCII.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCallbacks.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpConnection.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpConnection.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpEncryption.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpEncryption.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpMain.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpMain.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpPost.h
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.c
# End Source File
# Begin Source File

SOURCE=..\..\ghttp\ghttpProcess.h
# End Source File
# End Group
# Begin Group "Images"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\images\asteroid0.tga
# End Source File
# Begin Source File

SOURCE=.\images\asteroid1.tga
# End Source File
# Begin Source File

SOURCE=.\images\asteroid2.tga
# End Source File
# Begin Source File

SOURCE=.\images\explosion0.tga
# End Source File
# Begin Source File

SOURCE=.\images\explosion1.tga
# End Source File
# Begin Source File

SOURCE=.\images\mine0.tga
# End Source File
# Begin Source File

SOURCE=.\images\mine1.tga
# End Source File
# Begin Source File

SOURCE=.\images\mine2.tga
# End Source File
# Begin Source File

SOURCE=.\images\rocket0.tga
# End Source File
# Begin Source File

SOURCE=.\images\rocket1.tga
# End Source File
# Begin Source File

SOURCE=.\images\rocket2.tga
# End Source File
# Begin Source File

SOURCE=.\images\rocket3.tga
# End Source File
# Begin Source File

SOURCE=.\images\ship0.tga
# End Source File
# Begin Source File

SOURCE=.\images\ship1.tga
# End Source File
# Begin Source File

SOURCE=.\images\space.tga
# End Source File
# Begin Source File

SOURCE=.\images\spinner0.tga
# End Source File
# Begin Source File

SOURCE=.\images\spinner1.tga
# End Source File
# Begin Source File

SOURCE=.\images\spinner2.tga
# End Source File
# End Group
# Begin Group "Sounds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sounds\die.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\explosion.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\mine.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\pickup.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\rocket.wav
# End Source File
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

SOURCE=..\..\common\gsCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsCrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsDebug.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsLargeInt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsLargeInt.h
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

SOURCE=..\..\common\gsRC4.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsRC4.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSHA1.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSHA1.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSSL.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsSSL.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsStringUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gsXML.c
# End Source File
# Begin Source File

SOURCE=..\..\common\gsXML.h
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
# End Group
# Begin Source File

SOURCE=.\messages.txt
# End Source File
# Begin Source File

SOURCE=.\todo.txt
# End Source File
# End Target
# End Project
