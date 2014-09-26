// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxFileUtil.pas' rev: 6.00

#ifndef mxFileUtilHPP
#define mxFileUtilHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <RTLConsts.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxfileutil
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall BrowseDirectory(AnsiString &AFolderName, const AnsiString DlgText, Classes::THelpContext AHelpContext);
extern PACKAGE bool __fastcall BrowseComputer(AnsiString &ComputerName, const AnsiString DlgText, Classes::THelpContext AHelpContext);
extern PACKAGE AnsiString __fastcall NormalDir(const AnsiString DirName);
extern PACKAGE AnsiString __fastcall RemoveBackSlash(const AnsiString DirName);
extern PACKAGE bool __fastcall DirExists(AnsiString Name);
extern PACKAGE void __fastcall ForceDirectories(AnsiString Dir);
extern PACKAGE void __fastcall CopyFile(const AnsiString FileName, const AnsiString DestName, Controls::TControl* ProgressControl);
extern PACKAGE void __fastcall CopyFileEx(const AnsiString FileName, const AnsiString DestName, bool OverwriteReadOnly, bool ShellDialog, Controls::TControl* ProgressControl);
extern PACKAGE void __fastcall MoveFile(const AnsiString FileName, const AnsiString DestName);
extern PACKAGE void __fastcall MoveFileEx(const AnsiString FileName, const AnsiString DestName, bool ShellDialog);
extern PACKAGE __int64 __fastcall GetFileSize(const AnsiString FileName);
extern PACKAGE System::TDateTime __fastcall FileDateTime(const AnsiString FileName);
extern PACKAGE bool __fastcall HasAttr(const AnsiString FileName, int Attr);
extern PACKAGE bool __fastcall DeleteFiles(const AnsiString FileMask);
extern PACKAGE bool __fastcall DeleteFilesEx(const AnsiString * FileMasks, const int FileMasks_Size);
extern PACKAGE bool __fastcall ClearDir(const AnsiString Path, bool Delete);
extern PACKAGE AnsiString __fastcall GetTempDir();
extern PACKAGE AnsiString __fastcall GetWindowsDir();
extern PACKAGE AnsiString __fastcall GetSystemDir();
extern PACKAGE bool __fastcall ValidFileName(const AnsiString FileName);
extern PACKAGE int __fastcall FileLock(int Handle, int Offset, int LockSize)/* overload */;
extern PACKAGE int __fastcall FileUnlock(int Handle, int Offset, int LockSize)/* overload */;
extern PACKAGE int __fastcall FileLock(int Handle, __int64 Offset, __int64 LockSize)/* overload */;
extern PACKAGE int __fastcall FileUnlock(int Handle, __int64 Offset, __int64 LockSize)/* overload */;
extern PACKAGE AnsiString __fastcall ShortToLongFileName(const AnsiString ShortName);
extern PACKAGE AnsiString __fastcall LongToShortFileName(const AnsiString LongName);
extern PACKAGE AnsiString __fastcall ShortToLongPath(const AnsiString ShortName);
extern PACKAGE AnsiString __fastcall LongToShortPath(const AnsiString LongName);
extern PACKAGE void __fastcall CreateFileLink(const AnsiString FileName, const AnsiString DisplayName, int Folder);
extern PACKAGE void __fastcall DeleteFileLink(const AnsiString DisplayName, int Folder);

}	/* namespace Mxfileutil */
using namespace Mxfileutil;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxFileUtil
