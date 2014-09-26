// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPropTools.pas' rev: 6.00

#ifndef ElPropToolsHPP
#define ElPropToolsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elproptools
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void __fastcall LoadSetProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadFloatProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE Typinfo::PPropInfo __fastcall GetPropertyRecord(System::TObject* Comp, AnsiString PropertyName);
extern PACKAGE bool __fastcall HasProperty(System::TObject* Comp, AnsiString PropertyName);
extern PACKAGE void __fastcall LoadObject(System::TObject* Comp, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadCollection(Classes::TCollection* Collection, AnsiString Name, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadIntegerProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadEnumProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadStringProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall LoadStringList(Classes::TStrings* Strings, AnsiString Name, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreStringList(Classes::TStrings* Strings, AnsiString Name, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreObject(System::TObject* Comp, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreCollection(Classes::TCollection* Collection, AnsiString Name, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreIntegerProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreEnumProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreStringProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreSetProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);
extern PACKAGE void __fastcall StoreFloatProperty(System::TObject* Comp, Typinfo::PPropInfo Prop, Elini::TElIniFile* Storage);

}	/* namespace Elproptools */
using namespace Elproptools;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPropTools
