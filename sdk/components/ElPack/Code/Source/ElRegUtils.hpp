// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElRegUtils.pas' rev: 6.00

#ifndef ElRegUtilsHPP
#define ElRegUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elregutils
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TRegRootType { rrtUnknown, rrtHKEY_CLASSES_ROOT, rrtHKEY_CURRENT_USER, rrtHKEY_LOCAL_MACHINE, rrtHKEY_USERS, rrtHKEY_CURRENT_CONFIG };
#pragma option pop

typedef Set<TRegRootType, rrtUnknown, rrtHKEY_CURRENT_CONFIG>  TRegRoots;

//-- var, const, procedure ---------------------------------------------------
extern PACKAGE AnsiString __fastcall GetLastRegError();
extern PACKAGE bool __fastcall IsValidKeyName(AnsiString Name);
extern PACKAGE bool __fastcall KeyClear(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName);
extern PACKAGE bool __fastcall KeyHasValue(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString ValueName, bool &Exists);
extern PACKAGE bool __fastcall KeyRenameValue(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString ValueName, const AnsiString NewName);
extern PACKAGE bool __fastcall KeyDeleteValue(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString ValueName);
extern PACKAGE bool __fastcall KeySetValue(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString ValueName, int ValueType, void * Value, int ValueSize);
extern PACKAGE bool __fastcall KeyCreateSubKey(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString SubKeyName, const AnsiString NewClassName);
extern PACKAGE bool __fastcall KeyDelete(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName);
extern PACKAGE bool __fastcall CopyKey(const AnsiString OldComputerName, const AnsiString NewComputerName, TRegRootType OldRT, TRegRootType NewRT, const AnsiString OldKeyName, const AnsiString NewKeyName);
extern PACKAGE bool __fastcall KeyGetClassName(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, AnsiString &ClassName);
extern PACKAGE bool __fastcall KeyEnumValues(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, Classes::TStringList* SL);
extern PACKAGE bool __fastcall KeyGetValueInfo(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, const AnsiString ValueName, int &ValueType, AnsiString &ValueString, int &ValueSize);
extern PACKAGE bool __fastcall KeyEnumSubKeys(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, Classes::TStringList* SL);
extern PACKAGE bool __fastcall KeyHasSubKeys(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName);
extern PACKAGE TRegRootType __fastcall NameToRootType(const AnsiString Name);
extern PACKAGE AnsiString __fastcall RootTypeName(TRegRootType RT);
extern PACKAGE AnsiString __fastcall RootTypeShortName(TRegRootType RT);
extern PACKAGE AnsiString __fastcall ValueTypeToString(int VT);
extern PACKAGE HKEY __fastcall RootTypeToHandle(TRegRootType RT);
extern PACKAGE bool __fastcall KeyHasSubKeys0(HKEY Key, const AnsiString KeyName);
extern PACKAGE bool __fastcall KeyEnumSubKeys0(HKEY Key, const AnsiString KeyName, Classes::TStringList* SL);
extern PACKAGE bool __fastcall OpenRegKey(const AnsiString ComputerName, TRegRootType RT, const AnsiString KeyName, HKEY &KeyRes);

}	/* namespace Elregutils */
using namespace Elregutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElRegUtils
