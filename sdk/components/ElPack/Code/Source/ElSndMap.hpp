// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElSndMap.pas' rev: 6.00

#ifndef ElSndMapHPP
#define ElSndMapHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <TypInfo.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <MMSystem.hpp>	// Pascal unit
#include <ElRegUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elsndmap
{
//-- type declarations -------------------------------------------------------
typedef AnsiString TElSoundName;

class DELPHICLASS TElSoundMap;
class PASCALIMPLEMENTATION TElSoundMap : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Classes::TStringList* FSchemes;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	AnsiString FScheme;
	AnsiString FApplicationKey;
	AnsiString FApplicationName;
	Elini::TElIniFile* FRegIni;
	Elini::TElIniFile* ARegIni;
	Classes::TStringList* FEventKeys;
	bool FMute;
	AnsiString __fastcall GetEventLabels(AnsiString EventKey);
	void __fastcall SetEventLabels(AnsiString EventKey, AnsiString newValue);
	bool __fastcall GetEnabled(AnsiString EventKey);
	void __fastcall SetEnabled(AnsiString EventKey, bool newValue);
	Classes::TStringList* __fastcall GetSchemes(void);
	Classes::TStringList* __fastcall GetEventKeys(void);
	AnsiString __fastcall GetEventValues(AnsiString EventKey);
	void __fastcall SetEventValues(AnsiString EventKey, AnsiString newValue);
	void __fastcall SetApplicationName(AnsiString newValue);
	void __fastcall SetApplicationKey(AnsiString newValue);
	void __fastcall SetScheme(AnsiString newValue);
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	void __fastcall SetStoragePath(AnsiString newValue);
	
protected:
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	
public:
	__fastcall virtual TElSoundMap(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSoundMap(void);
	void __fastcall Play(AnsiString EventKey);
	void __fastcall Add(AnsiString EventKey, AnsiString EventLabel, AnsiString EventValue, bool Enabled);
	void __fastcall Delete(AnsiString EventKey);
	virtual void __fastcall Loaded(void);
	__property Classes::TStringList* EventKeys = {read=GetEventKeys};
	__property AnsiString EventLabel[AnsiString EventKey] = {read=GetEventLabels, write=SetEventLabels};
	__property AnsiString EventValue[AnsiString EventKey] = {read=GetEventValues, write=SetEventValues};
	__property bool EventEnabled[AnsiString EventKey] = {read=GetEnabled, write=SetEnabled};
	__property Classes::TStringList* Schemes = {read=GetSchemes};
	
__published:
	__property bool Mute = {read=FMute, write=FMute, nodefault};
	__property AnsiString ApplicationName = {read=FApplicationName, write=SetApplicationName};
	__property AnsiString ApplicationKey = {read=FApplicationKey, write=SetApplicationKey};
	__property AnsiString Scheme = {read=FScheme, write=SetScheme};
	__property AnsiString StoragePath = {read=FStoragePath, write=SetStoragePath};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elsndmap */
using namespace Elsndmap;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElSndMap
