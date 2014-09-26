// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxPlacemnt.pas' rev: 6.00

#ifndef mxPlacemntHPP
#define mxPlacemntHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mxHook.hpp>	// Pascal unit
#include <mxVCLUtils.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <IniFiles.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxplacemnt
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TPlacementOption { fpState, fpPosition, fpActiveControl };
#pragma option pop

typedef Set<TPlacementOption, fpState, fpActiveControl>  TPlacementOptions;

#pragma option push -b-
enum TPlacementOperation { poSave, poRestore };
#pragma option pop

#pragma option push -b-
enum TPlacementRegRoot { prCurrentUser, prLocalMachine, prCurrentConfig, prClassesRoot, prUsers, prDynData };
#pragma option pop

class DELPHICLASS TWinMinMaxInfo;
class DELPHICLASS TFormPlacement;
class DELPHICLASS TIniLink;
class PASCALIMPLEMENTATION TFormPlacement : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FActive;
	AnsiString *FIniFileName;
	AnsiString *FIniSection;
	Inifiles::TIniFile* FIniFile;
	bool FUseRegistry;
	Registry::TRegIniFile* FRegIniFile;
	TPlacementRegRoot FRegistryRoot;
	Classes::TList* FLinks;
	TPlacementOptions FOptions;
	int FVersion;
	bool FSaved;
	bool FRestored;
	bool FDestroying;
	bool FPreventResize;
	TWinMinMaxInfo* FWinMinMaxInfo;
	bool FDefMaximize;
	Mxhook::TRxWindowHook* FWinHook;
	Classes::TNotifyEvent FSaveFormShow;
	Classes::TNotifyEvent FSaveFormDestroy;
	Forms::TCloseQueryEvent FSaveFormCloseQuery;
	Classes::TNotifyEvent FOnSavePlacement;
	Classes::TNotifyEvent FOnRestorePlacement;
	void __fastcall SetEvents(void);
	void __fastcall RestoreEvents(void);
	void __fastcall SetHook(void);
	void __fastcall ReleaseHook(void);
	void __fastcall CheckToggleHook(void);
	bool __fastcall CheckMinMaxInfo(void);
	void __fastcall MinMaxInfoModified(void);
	void __fastcall SetWinMinMaxInfo(TWinMinMaxInfo* Value);
	AnsiString __fastcall GetIniSection();
	void __fastcall SetIniSection(const AnsiString Value);
	AnsiString __fastcall GetIniFileName();
	void __fastcall SetIniFileName(const AnsiString Value);
	System::TObject* __fastcall GetIniFile(void);
	void __fastcall SetPreventResize(bool Value);
	void __fastcall UpdatePreventResize(void);
	void __fastcall UpdatePlacement(void);
	void __fastcall IniNeeded(bool ReadOnly);
	void __fastcall IniFree(void);
	void __fastcall AddLink(TIniLink* ALink);
	void __fastcall NotifyLinks(TPlacementOperation Operation);
	void __fastcall RemoveLink(TIniLink* ALink);
	void __fastcall WndMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall FormCloseQuery(System::TObject* Sender, bool &CanClose);
	void __fastcall FormDestroy(System::TObject* Sender);
	Forms::TForm* __fastcall GetForm(void);
	
protected:
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall Save(void);
	DYNAMIC void __fastcall Restore(void);
	virtual void __fastcall SavePlacement(void);
	virtual void __fastcall RestorePlacement(void);
	virtual AnsiString __fastcall DoReadString(const AnsiString Section, const AnsiString Ident, const AnsiString Default);
	virtual void __fastcall DoWriteString(const AnsiString Section, const AnsiString Ident, const AnsiString Value);
	__property Forms::TForm* Form = {read=GetForm};
	
public:
	__fastcall virtual TFormPlacement(Classes::TComponent* AOwner);
	__fastcall virtual ~TFormPlacement(void);
	void __fastcall SaveFormPlacement(void);
	void __fastcall RestoreFormPlacement(void);
	AnsiString __fastcall ReadString(const AnsiString Ident, const AnsiString Default);
	void __fastcall WriteString(const AnsiString Ident, const AnsiString Value);
	int __fastcall ReadInteger(const AnsiString Ident, int Default);
	void __fastcall WriteInteger(const AnsiString Ident, int Value);
	void __fastcall EraseSections(void);
	__property System::TObject* IniFileObject = {read=GetIniFile};
	__property Inifiles::TIniFile* IniFile = {read=FIniFile};
	__property Registry::TRegIniFile* RegIniFile = {read=FRegIniFile};
	
__published:
	__property bool Active = {read=FActive, write=FActive, default=1};
	__property AnsiString IniFileName = {read=GetIniFileName, write=SetIniFileName};
	__property AnsiString IniSection = {read=GetIniSection, write=SetIniSection};
	__property TWinMinMaxInfo* MinMaxInfo = {read=FWinMinMaxInfo, write=SetWinMinMaxInfo};
	__property TPlacementOptions Options = {read=FOptions, write=FOptions, default=3};
	__property bool PreventResize = {read=FPreventResize, write=SetPreventResize, default=0};
	__property TPlacementRegRoot RegistryRoot = {read=FRegistryRoot, write=FRegistryRoot, default=0};
	__property bool UseRegistry = {read=FUseRegistry, write=FUseRegistry, default=0};
	__property int Version = {read=FVersion, write=FVersion, default=0};
	__property Classes::TNotifyEvent OnSavePlacement = {read=FOnSavePlacement, write=FOnSavePlacement};
	__property Classes::TNotifyEvent OnRestorePlacement = {read=FOnRestorePlacement, write=FOnRestorePlacement};
};


class PASCALIMPLEMENTATION TWinMinMaxInfo : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	TFormPlacement* FOwner;
	#pragma pack(push, 1)
	tagMINMAXINFO FMinMaxInfo;
	#pragma pack(pop)
	
	int __fastcall GetMinMaxInfo(int Index);
	void __fastcall SetMinMaxInfo(int Index, int Value);
	
public:
	bool __fastcall DefaultMinMaxInfo(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	
__published:
	__property int MaxPosLeft = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=0, default=0};
	__property int MaxPosTop = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=1, default=0};
	__property int MaxSizeHeight = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=2, default=0};
	__property int MaxSizeWidth = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=3, default=0};
	__property int MaxTrackHeight = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=4, default=0};
	__property int MaxTrackWidth = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=5, default=0};
	__property int MinTrackHeight = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=6, default=0};
	__property int MinTrackWidth = {read=GetMinMaxInfo, write=SetMinMaxInfo, index=7, default=0};
public:
	#pragma option push -w-inl
	/* TPersistent.Destroy */ inline __fastcall virtual ~TWinMinMaxInfo(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TWinMinMaxInfo(void) : Classes::TPersistent() { }
	#pragma option pop
	
};


class DELPHICLASS TFormStorage;
class DELPHICLASS TStoredValues;
class DELPHICLASS TStoredValue;
class PASCALIMPLEMENTATION TStoredValues : public Classes::TOwnedCollection 
{
	typedef Classes::TOwnedCollection inherited;
	
public:
	TStoredValue* operator[](int Index) { return Items[Index]; }
	
private:
	TFormPlacement* FStorage;
	TStoredValue* __fastcall GetValue(const AnsiString Name);
	void __fastcall SetValue(const AnsiString Name, TStoredValue* StoredValue);
	Variant __fastcall GetStoredValue(const AnsiString Name);
	void __fastcall SetStoredValue(const AnsiString Name, const Variant &Value);
	HIDESBASE TStoredValue* __fastcall GetItem(int Index);
	HIDESBASE void __fastcall SetItem(int Index, TStoredValue* StoredValue);
	
public:
	__fastcall TStoredValues(Classes::TPersistent* AOwner);
	int __fastcall IndexOf(const AnsiString Name);
	virtual void __fastcall SaveValues(void);
	virtual void __fastcall RestoreValues(void);
	__property TFormPlacement* Storage = {read=FStorage, write=FStorage};
	__property TStoredValue* Items[int Index] = {read=GetItem, write=SetItem/*, default*/};
	__property TStoredValue* Values[AnsiString Name] = {read=GetValue, write=SetValue};
	__property Variant StoredValue[AnsiString Name] = {read=GetStoredValue, write=SetStoredValue};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TStoredValues(void) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TFormStorage : public TFormPlacement 
{
	typedef TFormPlacement inherited;
	
private:
	Classes::TStrings* FStoredProps;
	TStoredValues* FStoredValues;
	void __fastcall SetStoredProps(Classes::TStrings* Value);
	void __fastcall SetStoredValues(TStoredValues* Value);
	Variant __fastcall GetStoredValue(const AnsiString Name);
	void __fastcall SetStoredValue(const AnsiString Name, const Variant &Value);
	
protected:
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall SavePlacement(void);
	virtual void __fastcall RestorePlacement(void);
	virtual void __fastcall SaveProperties(void);
	virtual void __fastcall RestoreProperties(void);
	virtual void __fastcall WriteState(Classes::TWriter* Writer);
	
public:
	__fastcall virtual TFormStorage(Classes::TComponent* AOwner);
	__fastcall virtual ~TFormStorage(void);
	void __fastcall SetNotification(void);
	__property Variant StoredValue[AnsiString Name] = {read=GetStoredValue, write=SetStoredValue};
	
__published:
	__property Classes::TStrings* StoredProps = {read=FStoredProps, write=SetStoredProps};
	__property TStoredValues* StoredValues = {read=FStoredValues, write=SetStoredValues};
};


class PASCALIMPLEMENTATION TIniLink : public Classes::TPersistent 
{
	typedef Classes::TPersistent inherited;
	
private:
	TFormPlacement* FStorage;
	Classes::TNotifyEvent FOnSave;
	Classes::TNotifyEvent FOnLoad;
	System::TObject* __fastcall GetIniObject(void);
	AnsiString __fastcall GetRootSection();
	void __fastcall SetStorage(TFormPlacement* Value);
	
protected:
	virtual void __fastcall SaveToIni(void);
	virtual void __fastcall LoadFromIni(void);
	
public:
	__fastcall virtual ~TIniLink(void);
	__property System::TObject* IniObject = {read=GetIniObject};
	__property TFormPlacement* Storage = {read=FStorage, write=SetStorage};
	__property AnsiString RootSection = {read=GetRootSection};
	__property Classes::TNotifyEvent OnSave = {read=FOnSave, write=FOnSave};
	__property Classes::TNotifyEvent OnLoad = {read=FOnLoad, write=FOnLoad};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TIniLink(void) : Classes::TPersistent() { }
	#pragma option pop
	
};


typedef void __fastcall (__closure *TStoredValueEvent)(TStoredValue* Sender, Variant &Value);

class PASCALIMPLEMENTATION TStoredValue : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	AnsiString FName;
	Variant FValue;
	AnsiString FKeyString;
	TStoredValueEvent FOnSave;
	TStoredValueEvent FOnRestore;
	bool __fastcall IsValueStored(void);
	TStoredValues* __fastcall GetStoredValues(void);
	
protected:
	virtual AnsiString __fastcall GetDisplayName();
	virtual void __fastcall SetDisplayName(const AnsiString Value);
	
public:
	__fastcall virtual TStoredValue(Classes::TCollection* Collection);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	void __fastcall Clear(void);
	virtual void __fastcall Save(void);
	virtual void __fastcall Restore(void);
	__property TStoredValues* StoredValues = {read=GetStoredValues};
	
__published:
	__property AnsiString Name = {read=FName, write=SetDisplayName};
	__property Variant Value = {read=FValue, write=FValue, stored=IsValueStored};
	__property AnsiString KeyString = {read=FKeyString, write=FKeyString};
	__property TStoredValueEvent OnSave = {read=FOnSave, write=FOnSave};
	__property TStoredValueEvent OnRestore = {read=FOnRestore, write=FOnRestore};
public:
	#pragma option push -w-inl
	/* TCollectionItem.Destroy */ inline __fastcall virtual ~TStoredValue(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Mxplacemnt */
using namespace Mxplacemnt;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxPlacemnt
