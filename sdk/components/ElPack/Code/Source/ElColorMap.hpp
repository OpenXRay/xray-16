// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElColorMap.pas' rev: 6.00

#ifndef ElColorMapHPP
#define ElColorMapHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElCRC32.hpp>	// Pascal unit
#include <ElIni.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcolormap
{
//-- type declarations -------------------------------------------------------
struct TColorEntry;
typedef TColorEntry *PColorEntry;

#pragma pack(push, 4)
struct TColorEntry
{
	int Id;
	AnsiString Name;
	AnsiString Group;
	bool UseFg;
	bool UseBk;
	Graphics::TColor FgColor;
	Graphics::TColor BkColor;
} ;
#pragma pack(pop)

class DELPHICLASS TMapChangeLink;
class PASCALIMPLEMENTATION TMapChangeLink : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Classes::TNotifyEvent FOnChange;
	
protected:
	virtual void __fastcall TriggerChangeEvent(void);
	
__published:
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TMapChangeLink(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TMapChangeLink(void) { }
	#pragma option pop
	
};


typedef int TCustomColArray[16];

class DELPHICLASS TElColorEntries;
class PASCALIMPLEMENTATION TElColorEntries : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElColorEntries(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElColorEntries(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElColorMap;
class PASCALIMPLEMENTATION TElColorMap : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
public:
	TColorEntry operator[](int index) { return Items[index]; }
	
private:
	Elini::TElIniFile* FStorage;
	Ellist::TElList* FList;
	Ellist::TElList* FLinkList;
	Classes::TNotifyEvent FOnChange;
	TElColorEntries* FEntries;
	bool FChanging;
	int FUpdCount;
	TColorEntry __fastcall GetItems(int index);
	void __fastcall SetItems(int index, const TColorEntry &newValue);
	void __fastcall NotifyLinks(void);
	int __fastcall GetCount(void);
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	
protected:
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall TriggerChangeEvent(void);
	virtual void __fastcall ReadData(Classes::TStream* Stream);
	virtual void __fastcall WriteData(Classes::TStream* Stream);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	
public:
	Classes::TStringList* CustomCols;
	__fastcall virtual TElColorMap(Classes::TComponent* AOwner);
	__fastcall virtual ~TElColorMap(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	virtual bool __fastcall RegisterNotifyChange(TMapChangeLink* Link);
	virtual bool __fastcall UnregisterNotifyChange(TMapChangeLink* Link);
	virtual bool __fastcall Edit(AnsiString ACaption);
	int __fastcall AddItem(TColorEntry &Entry);
	int __fastcall InsertItem(int Index, TColorEntry &Entry);
	void __fastcall DeleteItem(int index);
	void __fastcall ClearItems(void);
	int __fastcall EntryByID(int ID);
	int __fastcall MakeID(const TColorEntry &Entry);
	virtual void __fastcall BeginUpdate(void);
	virtual void __fastcall EndUpdate(void);
	__property TColorEntry Items[int index] = {read=GetItems, write=SetItems/*, default*/};
	__property int Count = {read=GetCount, nodefault};
	void __fastcall Restore(void);
	void __fastcall Save(void);
	
__published:
	__property TElColorEntries* ItemsList = {read=FEntries};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcolormap */
using namespace Elcolormap;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElColorMap
