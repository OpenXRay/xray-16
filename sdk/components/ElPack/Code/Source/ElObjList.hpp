// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElObjList.pas' rev: 6.00

#ifndef ElObjListHPP
#define ElObjListHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElIni.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elobjlist
{
//-- type declarations -------------------------------------------------------
typedef TMetaClass*TElObjectListItemClass;

class DELPHICLASS TElObjectListItem;
typedef int __fastcall (*TElObjListSortCompare)(TElObjectListItem* Item1, TElObjectListItem* Item2, void * Cargo);

class DELPHICLASS TElObjectList;
class PASCALIMPLEMENTATION TElObjectList : public Ellist::TElList 
{
	typedef Ellist::TElList inherited;
	
public:
	TElObjectListItem* operator[](int Index) { return Items[Index]; }
	
private:
	unsigned FLastID;
	TMetaClass*FListItemClass;
	Classes::TPersistent* FOwner;
	TElObjectListItem* __fastcall GetItems(int Index);
	void __fastcall SetItems(int Index, TElObjectListItem* Value);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	
public:
	__fastcall TElObjectList(Classes::TPersistent* Owner, TMetaClass* ListItemClass);
	HIDESBASE TElObjectListItem* __fastcall Add(void);
	void __fastcall AddItem(TElObjectListItem* Item);
	virtual void __fastcall AfterLoad(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall AfterSave(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	virtual void __fastcall BeforeLoad(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall BeforeSave(Elini::TElIniFile* IniFile, AnsiString KeyName);
	HIDESBASE TElObjectListItem* __fastcall First(void);
	HIDESBASE int __fastcall IndexOf(TElObjectListItem* Item);
	HIDESBASE int __fastcall IndexOfBack(int StartIndex, TElObjectListItem* Item);
	HIDESBASE int __fastcall IndexOfFrom(int StartIndex, TElObjectListItem* Item);
	HIDESBASE void __fastcall Insert(int Index, TElObjectListItem* Item);
	HIDESBASE TElObjectListItem* __fastcall Last(void);
	HIDESBASE void __fastcall Sort(TElObjListSortCompare Compare, void * Cargo);
	__property TElObjectListItem* Items[int Index] = {read=GetItems, write=SetItems/*, default*/};
	__property TMetaClass* ListItemClass = {read=FListItemClass, write=FListItemClass};
	
__published:
	__property unsigned LastID = {read=FLastID, nodefault};
public:
	#pragma option push -w-inl
	/* TElList.Destroy */ inline __fastcall virtual ~TElObjectList(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElObjectListItem : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	unsigned FID;
	TElObjectList* FList;
	bool FLoading;
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	
public:
	__fastcall virtual TElObjectListItem(TElObjectList* List);
	__fastcall virtual ~TElObjectListItem(void);
	virtual void __fastcall AfterLoad(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall AfterSave(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall BeforeLoad(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall BeforeSave(Elini::TElIniFile* IniFile, AnsiString KeyName);
	__property unsigned ID = {read=FID, nodefault};
	__property TElObjectList* List = {read=FList};
	__property bool Loading = {read=FLoading, write=FLoading, nodefault};
};


class DELPHICLASS TElHeteroObjectList;
class PASCALIMPLEMENTATION TElHeteroObjectList : public TElObjectList 
{
	typedef TElObjectList inherited;
	
public:
	virtual void __fastcall BeforeLoad(Elini::TElIniFile* IniFile, AnsiString KeyName);
	virtual void __fastcall BeforeSave(Elini::TElIniFile* IniFile, AnsiString KeyName);
public:
	#pragma option push -w-inl
	/* TElObjectList.Create */ inline __fastcall TElHeteroObjectList(Classes::TPersistent* Owner, TMetaClass* ListItemClass) : TElObjectList(Owner, ListItemClass) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TElList.Destroy */ inline __fastcall virtual ~TElHeteroObjectList(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elobjlist */
using namespace Elobjlist;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElObjList
