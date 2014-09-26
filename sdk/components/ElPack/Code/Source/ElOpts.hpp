// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElOpts.pas' rev: 6.00

#ifndef ElOptsHPP
#define ElOptsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <TypInfo.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <IniFiles.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elopts
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElStorageType { eosRegistry, eosIni, eosElIni };
#pragma option pop

class DELPHICLASS TElOptions;
class PASCALIMPLEMENTATION TElOptions : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
protected:
	bool FAutoSave;
	AnsiString FIniName;
	AnsiString FIniSection;
	bool FLoading;
	Elini::TElIniFile* FStorage;
	TElStorageType FStorageType;
	virtual void __fastcall SetAutoSave(bool Value);
	
public:
	__fastcall virtual TElOptions(Classes::TComponent* AOwner);
	__fastcall virtual ~TElOptions(void);
	virtual void __fastcall Load(void);
	virtual void __fastcall Save(void);
	__property bool Loading = {read=FLoading, nodefault};
	
__published:
	__property bool AutoSave = {read=FAutoSave, write=SetAutoSave, nodefault};
	__property AnsiString IniName = {read=FIniName, write=FIniName};
	__property AnsiString IniSection = {read=FIniSection, write=FIniSection};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=FStorage};
	__property TElStorageType StorageType = {read=FStorageType, write=FStorageType, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elopts */
using namespace Elopts;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElOpts
