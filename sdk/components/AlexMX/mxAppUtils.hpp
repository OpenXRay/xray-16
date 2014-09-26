// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxAppUtils.pas' rev: 6.00

#ifndef mxAppUtilsHPP
#define mxAppUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mxVCLUtils.hpp>	// Pascal unit
#include <Grids.hpp>	// Pascal unit
#include <IniFiles.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxapputils
{
//-- type declarations -------------------------------------------------------
typedef AnsiString __fastcall (*TOnGetDefaultIniName)(void);

//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TOnGetDefaultIniName OnGetDefaultIniName;
extern PACKAGE AnsiString DefCompanyName;
extern PACKAGE bool RegUseAppTitle;
extern PACKAGE AnsiString __fastcall GetDefaultSection(Classes::TComponent* Component);
extern PACKAGE AnsiString __fastcall GetDefaultIniName();
extern PACKAGE AnsiString __fastcall GetDefaultIniRegKey();
extern PACKAGE void __fastcall GetDefaultIniData(Controls::TControl* Control, AnsiString &IniFileName, AnsiString &Section, bool UseRegistry);
extern PACKAGE Forms::TForm* __fastcall FindForm(TMetaClass* FormClass);
extern PACKAGE Forms::TForm* __fastcall FindShowForm(TMetaClass* FormClass, const AnsiString Caption);
extern PACKAGE bool __fastcall ShowDialog(TMetaClass* FormClass);
extern PACKAGE Forms::TForm* __fastcall InstantiateForm(TMetaClass* FormClass, void *Reference);
extern PACKAGE AnsiString __fastcall StrToIniStr(const AnsiString Str);
extern PACKAGE AnsiString __fastcall IniStrToStr(const AnsiString Str);
extern PACKAGE AnsiString __fastcall IniReadString(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, const AnsiString Default);
extern PACKAGE void __fastcall IniWriteString(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, const AnsiString Value);
extern PACKAGE int __fastcall IniReadInteger(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, int Default);
extern PACKAGE void __fastcall IniWriteInteger(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, int Value);
extern PACKAGE bool __fastcall IniReadBool(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, bool Default);
extern PACKAGE void __fastcall IniWriteBool(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident, bool Value);
extern PACKAGE void __fastcall IniEraseSection(System::TObject* IniFile, const AnsiString Section);
extern PACKAGE void __fastcall IniDeleteKey(System::TObject* IniFile, const AnsiString Section, const AnsiString Ident);
extern PACKAGE void __fastcall IniReadSections(System::TObject* IniFile, Classes::TStrings* Strings);
extern PACKAGE void __fastcall InternalSaveMDIChildren(Forms::TForm* MainForm, System::TObject* IniFile);
extern PACKAGE void __fastcall InternalRestoreMDIChildren(Forms::TForm* MainForm, System::TObject* IniFile);
extern PACKAGE void __fastcall SaveMDIChildrenReg(Forms::TForm* MainForm, Registry::TRegIniFile* IniFile);
extern PACKAGE void __fastcall RestoreMDIChildrenReg(Forms::TForm* MainForm, Registry::TRegIniFile* IniFile);
extern PACKAGE void __fastcall SaveMDIChildren(Forms::TForm* MainForm, Inifiles::TIniFile* IniFile);
extern PACKAGE void __fastcall RestoreMDIChildren(Forms::TForm* MainForm, Inifiles::TIniFile* IniFile);
extern PACKAGE void __fastcall InternalSaveGridLayout(Grids::TCustomGrid* Grid, System::TObject* IniFile, const AnsiString Section);
extern PACKAGE void __fastcall InternalRestoreGridLayout(Grids::TCustomGrid* Grid, System::TObject* IniFile, const AnsiString Section);
extern PACKAGE void __fastcall RestoreGridLayoutReg(Grids::TCustomGrid* Grid, Registry::TRegIniFile* IniFile);
extern PACKAGE void __fastcall SaveGridLayoutReg(Grids::TCustomGrid* Grid, Registry::TRegIniFile* IniFile);
extern PACKAGE void __fastcall RestoreGridLayout(Grids::TCustomGrid* Grid, Inifiles::TIniFile* IniFile);
extern PACKAGE void __fastcall SaveGridLayout(Grids::TCustomGrid* Grid, Inifiles::TIniFile* IniFile);
extern PACKAGE void __fastcall WriteFormPlacementReg(Forms::TForm* Form, Registry::TRegIniFile* IniFile, const AnsiString Section);
extern PACKAGE void __fastcall WriteFormPlacement(Forms::TForm* Form, Inifiles::TIniFile* IniFile, const AnsiString Section);
extern PACKAGE void __fastcall SaveFormPlacement(Forms::TForm* Form, const AnsiString IniFileName, bool UseRegistry);
extern PACKAGE void __fastcall ReadFormPlacementReg(Forms::TForm* Form, Registry::TRegIniFile* IniFile, const AnsiString Section, bool LoadState, bool LoadPosition);
extern PACKAGE void __fastcall ReadFormPlacement(Forms::TForm* Form, Inifiles::TIniFile* IniFile, const AnsiString Section, bool LoadState, bool LoadPosition);
extern PACKAGE void __fastcall RestoreFormPlacement(Forms::TForm* Form, const AnsiString IniFileName, bool UseRegistry);
extern PACKAGE AnsiString __fastcall GetUniqueFileNameInDir(const AnsiString Path, const AnsiString FileNameMask);
extern PACKAGE void __fastcall AppBroadcast(int Msg, int wParam, int lParam);
extern PACKAGE void __fastcall AppTaskbarIcons(bool AppOnly);

}	/* namespace Mxapputils */
using namespace Mxapputils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxAppUtils
