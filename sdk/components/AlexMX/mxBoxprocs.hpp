// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxBoxProcs.pas' rev: 6.00

#ifndef mxBoxProcsHPP
#define mxBoxProcsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <MXCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxboxprocs
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void __fastcall BoxMoveSelected(Controls::TWinControl* List, Classes::TStrings* Items);
extern PACKAGE int __fastcall BoxGetFirstSelection(Controls::TWinControl* List);
extern PACKAGE void __fastcall BoxSetItem(Controls::TWinControl* List, int Index);
extern PACKAGE void __fastcall BoxMoveSelectedItems(Controls::TWinControl* SrcList, Controls::TWinControl* DstList);
extern PACKAGE void __fastcall BoxMoveAllItems(Controls::TWinControl* SrcList, Controls::TWinControl* DstList);
extern PACKAGE bool __fastcall BoxCanDropItem(Controls::TWinControl* List, int X, int Y, int &DragIndex);
extern PACKAGE void __fastcall BoxDragOver(Controls::TWinControl* List, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept, bool Sorted);
extern PACKAGE void __fastcall BoxMoveFocusedItem(Controls::TWinControl* List, int DstIndex);

}	/* namespace Mxboxprocs */
using namespace Mxboxprocs;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxBoxProcs
