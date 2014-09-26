// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElArray.pas' rev: 6.00

#ifndef ElArrayHPP
#define ElArrayHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElContBase.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elarray
{
//-- type declarations -------------------------------------------------------
typedef int __fastcall (*TElArraySortCompare)(void * Item1, void * Item2, void * Cargo);

typedef void __fastcall (__closure *TElArrayDeleteEvent)(System::TObject* Sender, void * Item);

class DELPHICLASS TElArray;
class PASCALIMPLEMENTATION TElArray : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	void * operator[](int Index) { return Items[Index]; }
	
protected:
	void * *FList;
	int FCount;
	int FCapacity;
	bool FAutoClearObjects;
	TElArrayDeleteEvent FOnDelete;
	virtual void * __fastcall Get(int Index);
	virtual void __fastcall Grow(void);
	virtual void __fastcall Put(int Index, void * Item);
	void __fastcall SetCapacity(int NewCapacity);
	void __fastcall SetCount(int NewCount);
	virtual void __fastcall TriggerDeleteEvent(void * Item);
	/*         class method */ static void __fastcall Error(TMetaClass* vmt, const AnsiString Msg, int Data);
	
public:
	__fastcall TElArray(void);
	__fastcall virtual ~TElArray(void);
	int __fastcall Add(void * Item);
	void __fastcall Clear(void);
	void __fastcall Assign(TElArray* AList);
	virtual void __fastcall Delete(int Index);
	void __fastcall Exchange(int Index1, int Index2);
	TElArray* __fastcall Expand(void);
	void * __fastcall First(void);
	int __fastcall IndexOf(void * Item);
	int __fastcall IndexOfFrom(int StartIndex, void * Item);
	int __fastcall IndexOfBack(int StartIndex, void * Item);
	void __fastcall Insert(int Index, void * Item);
	void * __fastcall Last(void);
	void __fastcall Move(int CurIndex, int NewIndex);
	void __fastcall MoveRange(int CurStart, int CurEnd, int NewStart);
	int __fastcall Remove(void * Item);
	void __fastcall Pack(void);
	void __fastcall Sort(TElArraySortCompare Compare, void * Cargo);
	__property int Capacity = {read=FCapacity, write=SetCapacity, default=0};
	__property int Count = {read=FCount, write=SetCount, default=0};
	__property void * Items[int Index] = {read=Get, write=Put/*, default*/};
	__property Elcontbase::PPointerList List = {read=FList};
	__property bool AutoClearObjects = {read=FAutoClearObjects, write=FAutoClearObjects, default=0};
	__property TElArrayDeleteEvent OnDelete = {read=FOnDelete, write=FOnDelete};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elarray */
using namespace Elarray;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElArray
