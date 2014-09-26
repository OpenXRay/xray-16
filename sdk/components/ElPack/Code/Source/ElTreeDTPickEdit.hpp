// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeDTPickEdit.pas' rev: 6.00

#ifndef ElTreeDTPickEditHPP
#define ElTreeDTPickEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElDTPick.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreedtpickedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElTreeInplaceDateTimePicker;
class PASCALIMPLEMENTATION TElTreeInplaceDateTimePicker : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Eldtpick::TElDateTimePicker* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceDateTimePicker(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceDateTimePicker(void);
	__property Eldtpick::TElDateTimePicker* Editor = {read=FEditor};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreedtpickedit */
using namespace Eltreedtpickedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeDTPickEdit
