// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElUnicodeStrings.pas' rev: 6.00

#ifndef ElUnicodeStringsHPP
#define ElUnicodeStringsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <RTLConsts.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElArray.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elunicodestrings
{
//-- type declarations -------------------------------------------------------
struct TWideStringItem;
typedef TWideStringItem *PWideStringItem;

#pragma pack(push, 4)
struct TWideStringItem
{
	WideString FString;
	System::TObject* FObject;
} ;
#pragma pack(pop)

typedef TWideStringItem TWideStringItemList[134217728];

typedef TWideStringItem *PWideStringItemList;

class DELPHICLASS TElWideStrings;
class PASCALIMPLEMENTATION TElWideStrings : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
public:
	WideString operator[](int Index) { return Strings[Index]; }
	
private:
	int FUpdateCount;
	WideString __fastcall GetCommaText();
	WideString __fastcall GetName(int Index);
	WideString __fastcall GetValue(const WideString Name);
	void __fastcall ReadData(Classes::TReader* Reader);
	void __fastcall SetCommaText(WideString Value);
	void __fastcall SetValue(const WideString Name, const WideString Value);
	void __fastcall WriteData(Classes::TWriter* Writer);
	
protected:
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	void __fastcall Error(const AnsiString Msg, int Data)/* overload */;
	virtual WideString __fastcall Get(int Index) = 0 ;
	virtual int __fastcall GetCapacity(void);
	virtual int __fastcall GetCount(void) = 0 ;
	virtual System::TObject* __fastcall GetObject(int Index);
	virtual WideString __fastcall GetTextStr();
	virtual void __fastcall Put(int Index, const WideString S);
	virtual void __fastcall PutObject(int Index, System::TObject* AObject);
	virtual void __fastcall SetCapacity(int NewCapacity);
	virtual void __fastcall SetTextStr(const WideString Value);
	virtual void __fastcall SetUpdateState(bool Updating);
	
public:
	__fastcall virtual ~TElWideStrings(void);
	virtual int __fastcall Add(const WideString S);
	virtual int __fastcall AddObject(const WideString S, System::TObject* AObject);
	virtual void __fastcall AddStrings(TElWideStrings* Strings);
	void __fastcall Append(const WideString S);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	void __fastcall BeginUpdate(void);
	virtual void __fastcall Clear(void) = 0 ;
	virtual void __fastcall Delete(int Index) = 0 ;
	void __fastcall EndUpdate(void);
	bool __fastcall Equals(TElWideStrings* Strings);
	virtual void __fastcall Exchange(int Index1, int Index2);
	virtual wchar_t * __fastcall GetText(void);
	virtual int __fastcall IndexOf(const WideString S);
	int __fastcall IndexOfName(const WideString Name);
	int __fastcall IndexOfObject(System::TObject* AObject);
	virtual void __fastcall Insert(int Index, const WideString S) = 0 ;
	void __fastcall InsertObject(int Index, const WideString S, System::TObject* AObject);
	virtual void __fastcall LoadFromFile(const AnsiString FileName);
	virtual void __fastcall LoadFromStream(Classes::TStream* Stream);
	virtual void __fastcall Move(int CurIndex, int NewIndex);
	virtual void __fastcall SaveToFile(const AnsiString FileName);
	virtual void __fastcall SaveToStream(Classes::TStream* Stream);
	virtual void __fastcall SetText(wchar_t * Text);
	virtual void __fastcall AssignTo(Classes::TPersistent* Dest);
	__property int Capacity = {read=GetCapacity, write=SetCapacity, nodefault};
	__property WideString CommaText = {read=GetCommaText, write=SetCommaText};
	__property int Count = {read=GetCount, nodefault};
	__property WideString Names[int Index] = {read=GetName};
	__property System::TObject* Objects[int Index] = {read=GetObject, write=PutObject};
	__property WideString Strings[int Index] = {read=Get, write=Put/*, default*/};
	__property WideString Text = {read=GetTextStr, write=SetTextStr};
	__property WideString Values[WideString Name] = {read=GetValue, write=SetValue};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElWideStrings(void) : Classes::TPersistent() { }
	#pragma option pop
	
};


class DELPHICLASS TElWideStringList;
typedef int __fastcall (*TElWideStringListSortCompare)(TElWideStringList* List, int Index1, int Index2);

class PASCALIMPLEMENTATION TElWideStringList : public TElWideStrings 
{
	typedef TElWideStrings inherited;
	
private:
	Classes::TDuplicates FDuplicates;
	Classes::TNotifyEvent FOnChange;
	Classes::TNotifyEvent FOnChanging;
	bool FSorted;
	int FCapacity;
	int FCount;
	TWideStringItem *FList;
	void __fastcall ExchangeItems(int Index1, int Index2);
	void __fastcall Grow(void);
	void __fastcall InsertItem(int Index, const WideString S);
	void __fastcall QuickSort(int L, int R, TElWideStringListSortCompare SCompare);
	void __fastcall SetSorted(bool Value);
	
protected:
	virtual void __fastcall Changed(void);
	virtual void __fastcall Changing(void);
	virtual WideString __fastcall Get(int Index);
	virtual int __fastcall GetCapacity(void);
	virtual int __fastcall GetCount(void);
	virtual System::TObject* __fastcall GetObject(int Index);
	virtual void __fastcall Put(int Index, const WideString S);
	virtual void __fastcall PutObject(int Index, System::TObject* AObject);
	virtual void __fastcall SetCapacity(int NewCapacity);
	virtual void __fastcall SetUpdateState(bool Updating);
	
public:
	__fastcall virtual ~TElWideStringList(void);
	virtual int __fastcall Add(const WideString S);
	virtual void __fastcall Clear(void);
	virtual void __fastcall CustomSort(TElWideStringListSortCompare Compare);
	virtual void __fastcall Delete(int Index);
	virtual void __fastcall Exchange(int Index1, int Index2);
	virtual bool __fastcall Find(const WideString S, int &Index);
	virtual int __fastcall IndexOf(const WideString S);
	virtual void __fastcall Insert(int Index, const WideString S);
	virtual void __fastcall Sort(void);
	__property Classes::TDuplicates Duplicates = {read=FDuplicates, write=FDuplicates, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Classes::TNotifyEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property bool Sorted = {read=FSorted, write=SetSorted, nodefault};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElWideStringList(void) : TElWideStrings() { }
	#pragma option pop
	
};


class DELPHICLASS TElWideStringArray;
class PASCALIMPLEMENTATION TElWideStringArray : public TElWideStrings 
{
	typedef TElWideStrings inherited;
	
private:
	bool FStoreAssociatedData;
	Classes::TDuplicates FDuplicates;
	bool FSorted;
	Classes::TNotifyEvent FOnChanging;
	Classes::TNotifyEvent FOnChange;
	Elarray::TElArray* FArray;
	int FUpdateCount;
	void __fastcall OnItemDelete(System::TObject* Sender, void * Item);
	void __fastcall SetSorted(bool newValue);
	void __fastcall ExchangeItems(int Index1, int Index2);
	
protected:
	virtual void __fastcall QuickSort(int L, int R);
	virtual void __fastcall Changed(void);
	virtual void __fastcall Changing(void);
	virtual void __fastcall TriggerChangingEvent(void);
	virtual void __fastcall TriggerChangeEvent(void);
	virtual void __fastcall InsertItem(int Index, const WideString S);
	virtual WideString __fastcall Get(int Index);
	virtual int __fastcall GetCount(void);
	virtual System::TObject* __fastcall GetObject(int Index);
	virtual void __fastcall Put(int Index, const WideString S);
	virtual void __fastcall PutObject(int Index, System::TObject* AObject);
	virtual void __fastcall PutStringEntry(int Index, const WideString S, System::TObject* AObject);
	virtual void __fastcall SetUpdateState(bool Updating);
	
public:
	virtual void __fastcall Clear(void);
	virtual int __fastcall Add(const WideString S);
	virtual int __fastcall AddStringEntry(const WideString S, System::TObject* AObject);
	virtual void __fastcall Delete(int Index);
	virtual void __fastcall Insert(int Index, const WideString S);
	virtual int __fastcall IndexOf(const WideString S);
	virtual bool __fastcall Find(const WideString S, int &Index);
	virtual void __fastcall Sort(void);
	virtual void __fastcall Exchange(int Index1, int Index2);
	virtual void __fastcall SaveToBinaryStream(Classes::TStream* Stream);
	virtual void __fastcall LoadFromBinaryStream(Classes::TStream* Stream);
	__fastcall TElWideStringArray(void);
	__fastcall virtual ~TElWideStringArray(void);
	__property Classes::TDuplicates Duplicates = {read=FDuplicates, write=FDuplicates, nodefault};
	__property bool Sorted = {read=FSorted, write=SetSorted, nodefault};
	
__published:
	__property Classes::TNotifyEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property bool StoreAssociatedData = {read=FStoreAssociatedData, write=FStoreAssociatedData, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elunicodestrings */
using namespace Elunicodestrings;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElUnicodeStrings
