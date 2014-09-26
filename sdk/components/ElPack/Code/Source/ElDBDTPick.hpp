// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDBDTPick.pas' rev: 6.00

#ifndef ElDBDTPickHPP
#define ElDBDTPickHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ElDTPick.hpp>	// Pascal unit
#include <Variants.hpp>	// Pascal unit
#include <DBCtrls.hpp>	// Pascal unit
#include <DB.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldbdtpick
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElDBDateTimePicker;
class PASCALIMPLEMENTATION TElDBDateTimePicker : public Eldtpick::TElDateTimePicker 
{
	typedef Eldtpick::TElDateTimePicker inherited;
	
private:
	Dbctrls::TFieldDataLink* FDataLink;
	bool FNowForNULLValues;
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Message);
	MESSAGE void __fastcall CMGetDataLink(Messages::TMessage &Message);
	void __fastcall DataChange(System::TObject* Sender);
	AnsiString __fastcall GetDataField();
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TField* __fastcall GetField(void);
	void __fastcall SetDataField(const AnsiString Value);
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall UpdateData(System::TObject* Sender);
	
protected:
	virtual bool __fastcall GetReadOnly(void);
	virtual void __fastcall SetReadOnly(bool Value);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall SetModified(bool newValue);
	virtual void __fastcall TriggerChangeEvent(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TMessage &Message);
	virtual void __fastcall CloseUp(bool AcceptValue);
	virtual void __fastcall DropDown(void);
	
public:
	__fastcall virtual TElDBDateTimePicker(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBDateTimePicker(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UseRightToLeftAlignment(void);
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property AnsiString DataField = {read=GetDataField, write=SetDataField};
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property bool NowForNULLValues = {read=FNowForNULLValues, write=FNowForNULLValues, nodefault};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBDateTimePicker(HWND ParentWindow) : Eldtpick::TElDateTimePicker(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldbdtpick */
using namespace Eldbdtpick;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDBDTPick
