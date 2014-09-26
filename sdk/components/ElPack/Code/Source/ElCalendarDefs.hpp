// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCalendarDefs.pas' rev: 6.00

#ifndef ElCalendarDefsHPP
#define ElCalendarDefsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Controls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcalendardefs
{
//-- type declarations -------------------------------------------------------
typedef Shortint TDayOfWeek;

#pragma option push -b-
enum TElWeekEndDay { Sun, Mon, Tue, Wed, Thu, Fri, Sat };
#pragma option pop

typedef Set<TElWeekEndDay, Sun, Sat>  TElWeekEndDays;

class DELPHICLASS TElHoliday;
class PASCALIMPLEMENTATION TElHoliday : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	AnsiString FDescription;
	bool FFixedDate;
	Word FDay;
	Word FDayOfWeek;
	Word FMonth;
	bool FIsRest;
	void __fastcall SetFixedDate(bool newValue);
	void __fastcall SetDay(Word newValue);
	void __fastcall SetDayOfWeek(Word newValue);
	void __fastcall SetMonth(Word newValue);
	void __fastcall SetIsRest(bool newValue);
	
public:
	__fastcall virtual TElHoliday(Classes::TCollection* Collection);
	__fastcall virtual ~TElHoliday(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	void __fastcall SaveToStream(Classes::TStream* Stream);
	void __fastcall LoadFromStream(Classes::TStream* Stream);
	
__published:
	__property bool FixedDate = {read=FFixedDate, write=SetFixedDate, default=1};
	__property Word Day = {read=FDay, write=SetDay, nodefault};
	__property Word DayOfWeek = {read=FDayOfWeek, write=SetDayOfWeek, nodefault};
	__property Word Month = {read=FMonth, write=SetMonth, nodefault};
	__property bool IsRest = {read=FIsRest, write=SetIsRest, nodefault};
	__property AnsiString Description = {read=FDescription, write=FDescription};
};


class DELPHICLASS TElHolidays;
class PASCALIMPLEMENTATION TElHolidays : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElHoliday* operator[](int Index) { return Items[Index]; }
	
private:
	Classes::TPersistent* FOwner;
	TElHoliday* __fastcall GetItems(int Index);
	void __fastcall SetItems(int Index, TElHoliday* newValue);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	__fastcall TElHolidays(Classes::TComponent* AOwner);
	HIDESBASE TElHoliday* __fastcall Add(void);
	void __fastcall SaveToStream(Classes::TStream* Stream);
	void __fastcall LoadFromStream(Classes::TStream* Stream);
	__property TElHoliday* Items[int Index] = {read=GetItems, write=SetItems/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElHolidays(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcalendardefs */
using namespace Elcalendardefs;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCalendarDefs
