// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElStrPool.pas' rev: 6.00

#ifndef ElStrPoolHPP
#define ElStrPoolHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elstrpool
{
//-- type declarations -------------------------------------------------------
typedef TElWideStringArray TElFStringArray;
;

class DELPHICLASS TElStringPool;
class PASCALIMPLEMENTATION TElStringPool : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Elunicodestrings::TElWideStringArray* FItems;
	void __fastcall SetItems(Elunicodestrings::TElWideStringArray* newValue);
	
public:
	__fastcall virtual TElStringPool(Classes::TComponent* AOwner);
	__fastcall virtual ~TElStringPool(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property Elunicodestrings::TElWideStringArray* Items = {read=FItems, write=SetItems};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elstrpool */
using namespace Elstrpool;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElStrPool
