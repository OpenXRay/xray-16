// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStrArray.pas' rev: 6.00

#ifndef ElStrArrayHPP
#define ElStrArrayHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElArray.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elstrarray
{
//-- type declarations -------------------------------------------------------
struct TElStringItem;
typedef TElStringItem *PElStringItem;

#pragma pack(push, 4)
struct TElStringItem
{
	char *FString;
	System::TObject* FObject;
} ;
#pragma pack(pop)

class DELPHICLASS TElStringArray;
class PASCALIMPLEMENTATION TElStringArray : public Classes::TStrings 
{
	typedef Classes::TStrings inherited;
	
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
	virtual void __fastcall InsertItem(int Index, const AnsiString S);
	virtual AnsiString __fastcall Get(int Index);
	virtual int __fastcall GetCount(void);
	virtual System::TObject* __fastcall GetObject(int Index);
	virtual void __fastcall Put(int Index, const AnsiString S);
	virtual void __fastcall PutObject(int Index, System::TObject* AObject);
	virtual void __fastcall PutStringEntry(int Index, const AnsiString S, System::TObject* AObject);
	virtual void __fastcall SetUpdateState(bool Updating);
	
public:
	virtual void __fastcall Clear(void);
	virtual int __fastcall Add(const AnsiString S);
	virtual int __fastcall AddStringEntry(const AnsiString S, System::TObject* AObject);
	virtual void __fastcall Delete(int Index);
	virtual void __fastcall Insert(int Index, const AnsiString S);
	virtual int __fastcall IndexOf(const AnsiString S);
	int __fastcall IIndexOf(const AnsiString S);
	virtual bool __fastcall Find(const AnsiString S, int &Index);
	virtual void __fastcall Sort(void);
	virtual void __fastcall Exchange(int Index1, int Index2);
	virtual void __fastcall SaveToBinaryStream(Classes::TStream* Stream);
	virtual void __fastcall LoadFromBinaryStream(Classes::TStream* Stream);
	__fastcall TElStringArray(void);
	__fastcall virtual ~TElStringArray(void);
	__property Classes::TDuplicates Duplicates = {read=FDuplicates, write=FDuplicates, nodefault};
	__property bool Sorted = {read=FSorted, write=SetSorted, nodefault};
	
__published:
	__property Classes::TNotifyEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property bool StoreAssociatedData = {read=FStoreAssociatedData, write=FStoreAssociatedData, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elstrarray */
using namespace Elstrarray;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStrArray
