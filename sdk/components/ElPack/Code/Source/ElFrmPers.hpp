// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElFrmPers.pas' rev: 6.00

#ifndef ElFrmPersHPP
#define ElFrmPersHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElIni.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <ElMTree.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elfrmpers
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElPersistOption { epoState, epoPosition, epoProperties, epoActiveControl, epoSize };
#pragma option pop

typedef Set<TElPersistOption, epoState, epoSize>  TElPersistOptions;

class DELPHICLASS TElMinMaxInfo;
class DELPHICLASS TElFormPersist;
class DELPHICLASS TElStoredProps;
class PASCALIMPLEMENTATION TElStoredProps : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElStoredProps(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElStoredProps(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElFormPersist : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FTopMost;
	TElPersistOptions FPersistOptions;
	bool FActive;
	TElMinMaxInfo* FMinMaxInfo;
	Elmtree::TElMTree* FProps;
	Elini::TElIniFile* FStorage;
	AnsiString FStoragePath;
	Classes::TNotifyEvent FOnSave;
	Classes::TNotifyEvent FOnRestore;
	Elhook::TElHook* FHook;
	TElStoredProps* FStoredProps;
	#pragma pack(push, 1)
	Types::TRect FSaveSize;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TRect RealDims;
	#pragma pack(pop)
	
	void __fastcall SetStorage(Elini::TElIniFile* newValue);
	void __fastcall SetMinMaxInfo(TElMinMaxInfo* newValue);
	void __fastcall SetActive(bool newValue);
	void __fastcall OnHook(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall OnAfterHook(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall ReadPropsList(Classes::TStream* Stream);
	void __fastcall WritePropsList(Classes::TStream* Stream);
	void __fastcall OnSavePropData(System::TObject* Sender, Elmtree::TElMTreeItem Item, Classes::TStream* Stream);
	void __fastcall OnLoadPropData(System::TObject* Sender, Elmtree::TElMTreeItem Item, Classes::TStream* Stream);
	void __fastcall OnDelPropData(System::TObject* Sender, Elmtree::TElMTreeItem Item, void * Data);
	Elmtree::TElMTree* __fastcall GetPropsToStore(void);
	void __fastcall SetTopMost(bool newValue);
	void __fastcall UpdateTopmost(void);
	
protected:
	bool FFlipped;
	virtual void __fastcall TriggerSaveEvent(void);
	virtual void __fastcall TriggerRestoreEvent(void);
	void __fastcall InfoChanged(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall UpdatePosition(void);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	virtual void __fastcall Loaded(void);
	void __fastcall SetFlipped(bool Value);
	void __fastcall DoFlip(bool Flip);
	
public:
	__fastcall virtual TElFormPersist(Classes::TComponent* AOwner);
	__fastcall virtual ~TElFormPersist(void);
	void __fastcall Save(void);
	void __fastcall Restore(void);
	void __fastcall SavePosition(void);
	void __fastcall RestorePosition(void);
	void __fastcall SaveProps(void);
	void __fastcall RestoreProps(void);
	__property Elmtree::TElMTree* PropsToStore = {read=GetPropsToStore};
	
__published:
	__property Elini::TElIniFile* Storage = {read=FStorage, write=SetStorage};
	__property AnsiString StoragePath = {read=FStoragePath, write=FStoragePath};
	__property Classes::TNotifyEvent OnSave = {read=FOnSave, write=FOnSave};
	__property Classes::TNotifyEvent OnRestore = {read=FOnRestore, write=FOnRestore};
	__property TElMinMaxInfo* MinMaxInfo = {read=FMinMaxInfo, write=SetMinMaxInfo};
	__property bool Active = {read=FActive, write=SetActive, default=1};
	__property TElPersistOptions PersistOptions = {read=FPersistOptions, write=FPersistOptions, nodefault};
	__property TElStoredProps* StoredProps = {read=FStoredProps};
	__property bool TopMost = {read=FTopMost, write=SetTopMost, nodefault};
	__property bool Flipped = {read=FFlipped, write=SetFlipped, nodefault};
};


class PASCALIMPLEMENTATION TElMinMaxInfo : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	#pragma pack(push, 1)
	tagMINMAXINFO FInfo;
	#pragma pack(pop)
	
	TElFormPersist* FOwner;
	void __fastcall SetInfo(int index, int Value);
	int __fastcall GetInfo(int index);
	
__published:
	__property int MaxPosX = {read=GetInfo, write=SetInfo, index=0, default=0};
	__property int MaxPosY = {read=GetInfo, write=SetInfo, index=1, default=0};
	__property int MaxSizeX = {read=GetInfo, write=SetInfo, index=2, default=0};
	__property int MaxSizeY = {read=GetInfo, write=SetInfo, index=3, default=0};
	__property int MaxTrackX = {read=GetInfo, write=SetInfo, index=4, default=0};
	__property int MaxTrackY = {read=GetInfo, write=SetInfo, index=5, default=0};
	__property int MinTrackX = {read=GetInfo, write=SetInfo, index=6, default=0};
	__property int MinTrackY = {read=GetInfo, write=SetInfo, index=7, default=0};
public:
	#pragma option push -w-inl
	/* TPersistent.Destroy */ inline __fastcall virtual ~TElMinMaxInfo(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElMinMaxInfo(void) : Classes::TPersistent() { }
	#pragma option pop
	
};


#pragma option push -b-
enum TElStoreType { estComp, estProp, estCollection };
#pragma option pop

#pragma pack(push, 4)
struct TElPropData
{
	TElStoreType PropType;
	bool Store;
	AnsiString Name;
} ;
#pragma pack(pop)

typedef TElPropData *PElPropData;

//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elfrmpers */
using namespace Elfrmpers;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElFrmPers
