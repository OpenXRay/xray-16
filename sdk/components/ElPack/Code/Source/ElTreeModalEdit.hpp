// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeModalEdit.pas' rev: 6.00

#ifndef ElTreeModalEditHPP
#define ElTreeModalEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTree.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreemodaledit
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TModalEditExecuteEvent)(System::TObject* Sender, bool &Accepted);

class DELPHICLASS TElTreeInplaceModalEdit;
class PASCALIMPLEMENTATION TElTreeInplaceModalEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	void __fastcall WndProc(Messages::TMessage &Message);
	
protected:
	unsigned RegMsg;
	HWND FWnd;
	bool FInProgress;
	TModalEditExecuteEvent FOnExecute;
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall StartOperation(void);
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall Execute(bool &Accepted);
	
public:
	__fastcall virtual TElTreeInplaceModalEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceModalEdit(void);
	
__published:
	__property TModalEditExecuteEvent OnExecute = {read=FOnExecute, write=FOnExecute};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreemodaledit */
using namespace Eltreemodaledit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeModalEdit
