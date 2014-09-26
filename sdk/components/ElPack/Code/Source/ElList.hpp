// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElList.pas' rev: 6.00

#ifndef ElListHPP
#define ElListHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <ElContBase.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Ellist
{
//-- type declarations -------------------------------------------------------
typedef int __fastcall (*TElListSortCompare)(void * Item1, void * Item2, void * Cargo);

typedef int __fastcall (__closure *TElListSortCompareEx)(void * Item1, void * Item2, void * Cargo);

typedef void __fastcall (__closure *TElListDeleteEvent)(System::TObject* Sender, void * Item);

class DELPHICLASS TElList;
class PASCALIMPLEMENTATION TElList : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
public:
	void * operator[](int Index) { return Items[Index]; }
	
protected:
	bool FAutoClearObjects;
	int FCapacity;
	int FCount;
	void * *FList;
	TElListDeleteEvent FOnDelete;
	/*         class method */ static void __fastcall Error(TMetaClass* vmt, const AnsiString Msg, int Data);
	virtual void * __fastcall Get(int Index);
	virtual void __fastcall Grow(void);
	virtual void __fastcall Put(int Index, void * Item);
	void __fastcall SetCapacity(int NewCapacity);
	void __fastcall SetCount(int NewCount);
	void __fastcall IntDelete(int Index);
	virtual void __fastcall TriggerDeleteEvent(void * Item);
	
public:
	__fastcall TElList(void);
	__fastcall virtual ~TElList(void);
	void * __fastcall FastGet(int Index);
	int __fastcall Add(void * Item);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	virtual void __fastcall Clear(void);
	virtual void __fastcall Delete(int Index);
	virtual void __fastcall DeleteRange(int StartIndex, int EndIndex);
	void __fastcall Exchange(int Index1, int Index2);
	TElList* __fastcall Expand(void);
	void * __fastcall First(void);
	int __fastcall IndexOf(void * Item);
	int __fastcall IndexOfBack(int StartIndex, void * Item);
	int __fastcall IndexOfFrom(int StartIndex, void * Item);
	void __fastcall Insert(int Index, void * Item);
	void * __fastcall Last(void);
	void __fastcall Move(int CurIndex, int NewIndex);
	void __fastcall MoveRange(int CurStart, int CurEnd, int NewStart);
	void __fastcall Pack(void);
	int __fastcall Remove(void * Item);
	void __fastcall Sort(TElListSortCompare Compare, void * Cargo);
	void __fastcall SortC(TElListSortCompareEx Compare, void * Cargo);
	__property bool AutoClearObjects = {read=FAutoClearObjects, write=FAutoClearObjects, nodefault};
	__property int Capacity = {read=FCapacity, write=SetCapacity, nodefault};
	__property int Count = {read=FCount, write=SetCount, nodefault};
	__property void * Items[int Index] = {read=Get, write=Put/*, default*/};
	__property Classes::PPointerList List = {read=FList};
	__property TElListDeleteEvent OnDelete = {read=FOnDelete, write=FOnDelete};
};


//-- var, const, procedure ---------------------------------------------------
static const Word AlignMem = 0x100;

}	/* namespace Ellist */
using namespace Ellist;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElList
