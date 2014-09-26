// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMRU.pas' rev: 6.00

#ifndef ElMRUHPP
#define ElMRUHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElIni.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elmru
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElMRUSection;
typedef void __fastcall (__closure *TMRUChangeEvent)(System::TObject* Sender, TElMRUSection* Section);

class DELPHICLASS TElMRUEntry;
typedef void __fastcall (__closure *TMRUClickEvent)(System::TObject* Sender, TElMRUEntry* Entry);

typedef void __fastcall (__closure *TMRUStreamEvent)(System::TObject* Sender, Classes::TStream* Stream, TElMRUEntry* Entry);

#pragma option push -b-
enum TMRUAddMode { mamAdd, mamInsert };
#pragma option pop

class PASCALIMPLEMENTATION TElMRUEntry : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	AnsiString FName;
	int FData;
	TElMRUSection* FOwner;
	void __fastcall SetName(AnsiString value);
	void __fastcall SetData(int Value);
	
public:
	__property AnsiString Name = {read=FName, write=SetName};
	__property int Data = {read=FData, write=SetData, nodefault};
	__property TElMRUSection* Section = {read=FOwner};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElMRUEntry(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElMRUEntry(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElMRU;
class DELPHICLASS TElMRUSections;
class PASCALIMPLEMENTATION TElMRUSections : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElMRUSection* operator[](int index) { return Items[index]; }
	
private:
	TElMRU* FMRU;
	HIDESBASE TElMRUSection* __fastcall GetItem(int index);
	HIDESBASE void __fastcall SetItem(int index, TElMRUSection* newValue);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	__fastcall TElMRUSections(TElMRU* MRU);
	HIDESBASE TElMRUSection* __fastcall Add(void);
	__property TElMRUSection* Items[int index] = {read=GetItem, write=SetItem/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElMRUSections(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElMRU : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Elini::TElIniFile* FStorage;
	bool FShowAccel;
	bool FAutoUpdate;
	bool FAutoEnable;
	TElMRUSections* FSections;
	Menus::TMenuItem* FRecentMenu;
	TMRUAddMode FAddMode;
	bool FRemoveOnClick;
	char FAccelDelimiter;
	TMRUChangeEvent FOnChange;
	TMRUClickEvent FOnClick;
	TMRUStreamEvent FOnSaveEntry;
	TMRUStreamEvent FOnLoadEntry;
	Menus::TPopupMenu* FPopupMenu;
	AnsiString FStoragePath;
	bool FIgnoreDuplicates;
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	void __fastcall SetIgnoreDuplicates(bool newValue);
	void __fastcall SetPopupMenu(Menus::TPopupMenu* newValue);
	void __fastcall SetSections(TElMRUSections* newValue);
	void __fastcall SetShowAccel(bool newValue);
	void __fastcall SetAutoUpdate(bool newValue);
	void __fastcall SetAutoEnable(bool newValue);
	void __fastcall SetRecentMenu(Menus::TMenuItem* newValue);
	void __fastcall OnItemClick(System::TObject* Sender);
	void __fastcall SetAccelDelimiter(char newValue);
	
protected:
	virtual void __fastcall TriggerChangeEvent(TElMRUSection* Section);
	virtual void __fastcall TriggerClickEvent(TElMRUEntry* Entry);
	virtual void __fastcall TriggerSaveEntryEvent(Classes::TStream* Stream, TElMRUEntry* Entry);
	virtual void __fastcall TriggerLoadEntryEvent(Classes::TStream* Stream, TElMRUEntry* Entry);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TElMRU(Classes::TComponent* AOwner);
	__fastcall virtual ~TElMRU(void);
	virtual void __fastcall RebuildMenu(void);
	void __fastcall Restore(void);
	void __fastcall Save(void);
	TElMRUSection* __fastcall SectionByName(AnsiString Name);
	
__published:
	__property char AccelDelimiter = {read=FAccelDelimiter, write=SetAccelDelimiter, default=32};
	__property TMRUAddMode AddMode = {read=FAddMode, write=FAddMode, nodefault};
	__property TElMRUSections* Sections = {read=FSections, write=SetSections};
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property bool ShowAccel = {read=FShowAccel, write=SetShowAccel, default=1};
	__property bool AutoUpdate = {read=FAutoUpdate, write=SetAutoUpdate, default=1};
	__property bool AutoEnable = {read=FAutoEnable, write=SetAutoEnable, default=1};
	__property Menus::TMenuItem* RecentMenu = {read=FRecentMenu, write=SetRecentMenu};
	__property Menus::TPopupMenu* PopupMenu = {read=FPopupMenu, write=SetPopupMenu};
	__property bool RemoveOnClick = {read=FRemoveOnClick, write=FRemoveOnClick, default=1};
	__property bool IgnoreDuplicates = {read=FIgnoreDuplicates, write=SetIgnoreDuplicates, default=1};
	__property TMRUChangeEvent OnChange = {read=FOnChange, write=FOnChange};
	__property TMRUClickEvent OnClick = {read=FOnClick, write=FOnClick};
	__property TMRUStreamEvent OnSaveEntry = {read=FOnSaveEntry, write=FOnSaveEntry};
	__property TMRUStreamEvent OnLoadEntry = {read=FOnLoadEntry, write=FOnLoadEntry};
};


class PASCALIMPLEMENTATION TElMRUSection : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
protected:
	int FTag;
	bool FAutoHide;
	AnsiString FCaption;
	Ellist::TElList* FValues;
	int FCapacity;
	AnsiString FName;
	bool FShowName;
	bool FVisible;
	TElMRU* FOwner;
	void __fastcall SetName(AnsiString newValue);
	void __fastcall SetShowName(bool newValue);
	void __fastcall SetVisible(bool newValue);
	TElMRUEntry* __fastcall GetValue(int index);
	void __fastcall SetCapacity(int newValue);
	int __fastcall GetCount(void);
	void __fastcall OnEntryDelete(System::TObject* Sender, void * Item);
	void __fastcall SetCaption(AnsiString newValue);
	void __fastcall SetAutoHide(bool newValue);
	
public:
	__fastcall virtual TElMRUSection(Classes::TCollection* Collection);
	__fastcall virtual ~TElMRUSection(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	void __fastcall Clear(void);
	__property int Count = {read=GetCount, nodefault};
	virtual TElMRUEntry* __fastcall Add(AnsiString Name, int Data);
	virtual void __fastcall Remove(TElMRUEntry* Entry);
	TElMRUEntry* __fastcall EntryByName(AnsiString Name);
	__property TElMRUEntry* Entries[int index] = {read=GetValue};
	
__published:
	__property AnsiString Name = {read=FName, write=SetName};
	__property bool ShowCaption = {read=FShowName, write=SetShowName, default=0};
	__property bool Visible = {read=FVisible, write=SetVisible, default=1};
	__property int Capacity = {read=FCapacity, write=SetCapacity, default=10};
	__property AnsiString Caption = {read=FCaption, write=SetCaption};
	__property bool AutoHide = {read=FAutoHide, write=SetAutoHide, nodefault};
	__property int Tag = {read=FTag, write=FTag, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elmru */
using namespace Elmru;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMRU
