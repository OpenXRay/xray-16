// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElVerInfo.pas' rev: 6.00

#ifndef ElVerInfoHPP
#define ElVerInfoHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ShellAPI.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elverinfo
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TVersionAttribute { vaDebug, vaPatched, vaPreRelease, vaPrivateBuild, vaSpecialBuild };
#pragma option pop

typedef Set<TVersionAttribute, vaDebug, vaSpecialBuild>  TVersionAttributes;

class DELPHICLASS TElVersionInfo;
class PASCALIMPLEMENTATION TElVersionInfo : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
public:
	AnsiString operator[](AnsiString Name) { return Values[Name]; }
	
private:
	AnsiString FBuffer;
	AnsiString FFileName;
	tagVS_FIXEDFILEINFO *FFixedFileInfo;
	AnsiString FLanguage;
	TVersionAttributes __fastcall GetAttributes(void);
	int __fastcall GetBuild(void);
	AnsiString __fastcall GetLanguage();
	int __fastcall GetMajorVersion(void);
	int __fastcall GetMinorVersion(void);
	AnsiString __fastcall GetPredefined(int Index);
	int __fastcall GetRelease(void);
	AnsiString __fastcall GetValue(AnsiString AName);
	void __fastcall SetAttributes(const TVersionAttributes Value);
	void __fastcall SetDummy(const AnsiString Value);
	void __fastcall SetDummyEx(int Index, AnsiString Value);
	void __fastcall SetFileName(const AnsiString Value);
	void __fastcall SetDummyInt(const int Value);
	bool __fastcall StoreFileName(void);
	
public:
	__fastcall virtual TElVersionInfo(Classes::TComponent* AOwner);
	void __fastcall Refresh(void);
	__property AnsiString Values[AnsiString Name] = {read=GetValue/*, default*/};
	
__published:
	__property TVersionAttributes Attributes = {read=GetAttributes, write=SetAttributes, stored=false, nodefault};
	__property int Build = {read=GetBuild, write=SetDummyInt, stored=false, nodefault};
	__property AnsiString Comments = {read=GetPredefined, write=SetDummyEx, stored=false, index=9};
	__property AnsiString CompanyName = {read=GetPredefined, write=SetDummyEx, stored=false, index=0};
	__property AnsiString FileDescription = {read=GetPredefined, write=SetDummyEx, stored=false, index=1};
	__property AnsiString FileName = {read=FFileName, write=SetFileName, stored=StoreFileName};
	__property AnsiString FileVersion = {read=GetPredefined, write=SetDummyEx, stored=false, index=2};
	__property AnsiString InternalName = {read=GetPredefined, write=SetDummyEx, stored=false, index=3};
	__property AnsiString Language = {read=GetLanguage, write=SetDummy, stored=false};
	__property AnsiString LegalCopyright = {read=GetPredefined, write=SetDummyEx, stored=false, index=4};
	__property AnsiString LegalTrademarks = {read=GetPredefined, write=SetDummyEx, stored=false, index=5};
	__property int MajorVersion = {read=GetMajorVersion, write=SetDummyInt, stored=false, nodefault};
	__property int MinorVersion = {read=GetMinorVersion, write=SetDummyInt, stored=false, nodefault};
	__property AnsiString OriginalFilename = {read=GetPredefined, write=SetDummyEx, stored=false, index=6};
	__property AnsiString ProductName = {read=GetPredefined, write=SetDummyEx, stored=false, index=7};
	__property AnsiString ProductVersion = {read=GetPredefined, write=SetDummyEx, stored=false, index=8};
	__property int Release = {read=GetRelease, write=SetDummyInt, stored=false, nodefault};
public:
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~TElVersionInfo(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elverinfo */
using namespace Elverinfo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElVerInfo
