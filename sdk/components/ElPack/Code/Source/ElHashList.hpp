// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHashList.pas' rev: 6.00

#ifndef ElHashListHPP
#define ElHashListHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElCRC32.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhashlist
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EElHashListError;
class PASCALIMPLEMENTATION EElHashListError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElHashListError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElHashListError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElHashListError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElHashListError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElHashListError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElHashListError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElHashListError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElHashListError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElHashListError(void) { }
	#pragma option pop
	
};


typedef int THash[4];

typedef int *PHash;

struct THashRecord;
typedef THashRecord *PHashRecord;

#pragma pack(push, 4)
struct THashRecord
{
	int *Hash;
	void *ItemData;
} ;
#pragma pack(pop)

typedef THashRecord *THashList[134217727];

typedef PHashRecord *PHashList;

#pragma option push -b-
enum TElHashType { ehtMD5, ehtQuick, ehtCRC32 };
#pragma option pop

class DELPHICLASS TElHashList;
typedef void __fastcall (__closure *OnHashDeleteEvent)(TElHashList* Sender, void * Data);

#pragma option push -b-
enum THashInsertDupesMode { himInsert, himRaise, himReplace, himIgnore, himMove };
#pragma option pop

class PASCALIMPLEMENTATION TElHashList : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	bool FAutoClearObjects;
	bool FNoCase;
	TElHashType FHashType;
	PHashRecord *FList;
	int FCount;
	int FCapacity;
	OnHashDeleteEvent FOnDelete;
	bool FQuickHash;
	THashInsertDupesMode FInsertDupesMode;
	bool FRaiseOnAbsence;
	void __fastcall Grow(void);
	void __fastcall SetCapacity(int NewCapacity);
	void * __fastcall GetItem(AnsiString Hash);
	void __fastcall SetQuickHash(bool newValue);
	void __fastcall SetHashType(TElHashType newValue);
	void __fastcall SetAutoClearObjects(bool newValue);
	
protected:
	virtual int __fastcall CalcQuickHash(AnsiString Hash);
	
public:
	__fastcall virtual ~TElHashList(void);
	void __fastcall AddItem(AnsiString Hash, void * Value);
	void __fastcall DeleteItem(AnsiString Hash);
	void __fastcall InsertItem(int Index, AnsiString Hash, void * Value);
	int __fastcall GetIndex(AnsiString Hash);
	void __fastcall Clear(void);
	__property int Count = {read=FCount, nodefault};
	__property int Capacity = {read=FCapacity, nodefault};
	void * __fastcall GetByIndex(int Index);
	__fastcall TElHashList(void);
	__property void * Item[AnsiString Hash] = {read=GetItem};
	__property OnHashDeleteEvent OnDelete = {read=FOnDelete, write=FOnDelete};
	__property bool QuickHash = {read=FQuickHash, write=SetQuickHash, nodefault};
	__property bool RaiseOnAbsence = {read=FRaiseOnAbsence, write=FRaiseOnAbsence, default=0};
	__property THashInsertDupesMode InsertDupesMode = {read=FInsertDupesMode, write=FInsertDupesMode, nodefault};
	__property TElHashType HashType = {read=FHashType, write=SetHashType, nodefault};
	__property bool NoCase = {read=FNoCase, write=FNoCase, nodefault};
	__property bool AutoClearObjects = {read=FAutoClearObjects, write=SetAutoClearObjects, nodefault};
};


//-- var, const, procedure ---------------------------------------------------
static const int MaxHashListSize = 0x7ffffff;

}	/* namespace Elhashlist */
using namespace Elhashlist;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHashList
