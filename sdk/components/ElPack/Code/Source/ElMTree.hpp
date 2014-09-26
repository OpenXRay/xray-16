// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMTree.pas' rev: 6.00

#ifndef ElMTreeHPP
#define ElMTreeHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElList.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elmtree
{
//-- type declarations -------------------------------------------------------
struct RElMTreeItem;
typedef RElMTreeItem *TElMTreeItem;

#pragma pack(push, 4)
struct RElMTreeItem
{
	RElMTreeItem *Parent;
	void *Data;
	Ellist::TElList* List;
} ;
#pragma pack(pop)

typedef void __fastcall (*TIterProc)(TElMTreeItem Item, int Index, bool &ContinueIterate, void * IterateData);

typedef void __fastcall (__closure *TItemSaveEvent)(System::TObject* Sender, TElMTreeItem Item, Classes::TStream* Stream);

typedef void __fastcall (__closure *TElMTreeItemDelete)(System::TObject* Sender, TElMTreeItem Item, void * Data);

class DELPHICLASS TElMTree;
class PASCALIMPLEMENTATION TElMTree : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	RElMTreeItem *FRoot;
	int FCount;
	TItemSaveEvent FOnItemSave;
	TItemSaveEvent FOnItemLoad;
	TElMTreeItemDelete FOnItemDelete;
	TElMTreeItem __fastcall GetItem(int index);
	
protected:
	virtual void __fastcall TriggerItemSaveEvent(TElMTreeItem Item, Classes::TStream* Stream);
	virtual void __fastcall TriggerItemLoadEvent(TElMTreeItem Item, Classes::TStream* Stream);
	virtual void __fastcall TriggerItemDeleteEvent(TElMTreeItem Item, void * Data);
	
public:
	__fastcall TElMTree(void);
	__fastcall virtual ~TElMTree(void);
	TElMTreeItem __fastcall AddItem(TElMTreeItem Parent, void * Value);
	TElMTreeItem __fastcall InsertItem(TElMTreeItem Parent, int Index, void * Value);
	void __fastcall DeleteItem(TElMTreeItem Item);
	void __fastcall MoveTo(TElMTreeItem Item, TElMTreeItem NewParent);
	void __fastcall Clear(void);
	int __fastcall GetIndex(TElMTreeItem Item);
	int __fastcall GetAbsIndex(TElMTreeItem Item);
	void __fastcall Iterate(TIterProc IterateProc, void * IterateData);
	virtual void __fastcall SaveToStream(Classes::TStream* Stream);
	virtual void __fastcall LoadFromStream(Classes::TStream* Stream);
	virtual void __fastcall SaveSubTreeToStream(TElMTreeItem Item, Classes::TStream* Stream);
	virtual void __fastcall LoadSubTreeFromStream(TElMTreeItem Item, Classes::TStream* Stream);
	void __fastcall MoveToIns(TElMTreeItem Item, TElMTreeItem NewParent, int Index);
	__property int Count = {read=FCount, nodefault};
	__property TElMTreeItem Item[int index] = {read=GetItem};
	__property TItemSaveEvent OnItemSave = {read=FOnItemSave, write=FOnItemSave};
	__property TItemSaveEvent OnItemLoad = {read=FOnItemLoad, write=FOnItemLoad};
	__property TElMTreeItemDelete OnItemDelete = {read=FOnItemDelete, write=FOnItemDelete};
	__property TElMTreeItem Root = {read=FRoot};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elmtree */
using namespace Elmtree;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMTree
