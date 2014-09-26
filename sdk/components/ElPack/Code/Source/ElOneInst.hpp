// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElOneInst.pas' rev: 6.00

#ifndef ElOneInstHPP
#define ElOneInstHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElBaseComp.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eloneinst
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TInstanceRunEvent)(System::TObject* Sender, Classes::TStrings* Parameters);

struct TElMemMapArr;
typedef TElMemMapArr *PElMemMapArr;

#pragma pack(push, 4)
struct TElMemMapArr
{
	unsigned hPrevInst;
	unsigned hPrevAppWin;
	unsigned hPrevMainWin;
	unsigned hMonWin;
} ;
#pragma pack(pop)

typedef void __fastcall (__closure *TExistsEvent)(System::TObject* Sender, unsigned hPrevInst, HWND hPrevAppWin, HWND hPrevMainWin, bool &Switch);

class DELPHICLASS TElOneInstance;
class PASCALIMPLEMENTATION TElOneInstance : public Elbasecomp::TElBaseComponent 
{
	typedef Elbasecomp::TElBaseComponent inherited;
	
private:
	unsigned FMapHandle;
	unsigned FPrevInst;
	unsigned FPrevAppWin;
	unsigned FPrevMainWin;
	TElMemMapArr *FView;
	AnsiString FMapName;
	bool FNoAutoTerminate;
	TExistsEvent FOnExists;
	TInstanceRunEvent FOnInstanceRun;
	MESSAGE void __fastcall WMCopyData(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall TriggerExistsEvent(unsigned hPrevInst, unsigned hPrevAppWin, unsigned hPrevMainWin, bool &Switch);
	virtual void __fastcall TriggerInstanceRunEvent(Classes::TStrings* Parameters);
	virtual void __fastcall SetEnabled(bool AEnabled);
	virtual void __fastcall DoSetEnabled(bool AEnabled);
	void __fastcall CreateMapping(void);
	
public:
	__fastcall virtual ~TElOneInstance(void);
	__property unsigned PrevInstance = {read=FPrevInst, nodefault};
	__property unsigned FPrevMainWindow = {read=FPrevMainWin, nodefault};
	__property unsigned FPrevAppWindow = {read=FPrevAppWin, nodefault};
	
__published:
	__property AnsiString MapName = {read=FMapName, write=FMapName};
	__property bool NoAutoTerminate = {read=FNoAutoTerminate, write=FNoAutoTerminate, nodefault};
	__property TExistsEvent OnExists = {read=FOnExists, write=FOnExists};
	__property TInstanceRunEvent OnInstanceRun = {read=FOnInstanceRun, write=FOnInstanceRun};
	__property Enabled  = {default=0};
public:
	#pragma option push -w-inl
	/* TElBaseComponent.Create */ inline __fastcall virtual TElOneInstance(Classes::TComponent* AOwner) : Elbasecomp::TElBaseComponent(AOwner) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE AnsiString rs_OneInstAlreadyExists;

}	/* namespace Eloneinst */
using namespace Eloneinst;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElOneInst
