// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTimers.pas' rev: 6.00

#ifndef ElTimersHPP
#define ElTimersHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltimers
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomElTimer;
class PASCALIMPLEMENTATION TCustomElTimer : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	bool FEnabled;
	unsigned FInterval;
	bool FOneTime;
	Classes::TNotifyEvent FOnTimer;
	int FTag;
	void __fastcall SetInterval(const unsigned Value);
	
protected:
	virtual void __fastcall DoTick(void);
	virtual void __fastcall DoTimer(void);
	virtual void __fastcall SetEnabled(const bool Value);
	__property bool Enabled = {read=FEnabled, write=SetEnabled, nodefault};
	__property unsigned Interval = {read=FInterval, write=SetInterval, default=1000};
	__property bool OneTime = {read=FOneTime, write=FOneTime, nodefault};
	__property Classes::TNotifyEvent OnTimer = {read=FOnTimer, write=FOnTimer};
	__property int Tag = {read=FTag, write=FTag, nodefault};
	
public:
	__fastcall virtual TCustomElTimer(void);
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCustomElTimer(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElTimer;
class PASCALIMPLEMENTATION TElTimer : public TCustomElTimer 
{
	typedef TCustomElTimer inherited;
	
private:
	int FTimerID;
	HWND FWnd;
	void __fastcall WndProc(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall SetEnabled(const bool Value);
	
public:
	__fastcall virtual TElTimer(void);
	__fastcall virtual ~TElTimer(void);
	__property Enabled ;
	__property Interval  = {default=1000};
	__property OneTime ;
	__property OnTimer ;
	__property Tag ;
};


class DELPHICLASS TElPoolTimer;
class DELPHICLASS TElTimerPoolItem;
class PASCALIMPLEMENTATION TElTimerPoolItem : public Classes::TCollectionItem 
{
	typedef Classes::TCollectionItem inherited;
	
private:
	TElPoolTimer* FTimer;
	Classes::TNotifyEvent FOnTimer;
	bool __fastcall GetEnabled(void);
	unsigned __fastcall GetInterval(void);
	bool __fastcall GetOneTime(void);
	Classes::TNotifyEvent __fastcall GetOnTimer();
	int __fastcall GetTag(void);
	void __fastcall SetEnabled(const bool Value);
	void __fastcall SetInterval(const unsigned Value);
	void __fastcall SetOneTime(const bool Value);
	void __fastcall SetOnTimer(const Classes::TNotifyEvent Value);
	void __fastcall SetTag(const int Value);
	
public:
	__fastcall virtual TElTimerPoolItem(Classes::TCollection* Collection);
	__fastcall virtual ~TElTimerPoolItem(void);
	virtual void __fastcall Assign(Classes::TPersistent* Source);
	__property TElPoolTimer* Timer = {read=FTimer};
	
__published:
	__property bool Enabled = {read=GetEnabled, write=SetEnabled, nodefault};
	__property unsigned Interval = {read=GetInterval, write=SetInterval, default=1000};
	__property bool OneTime = {read=GetOneTime, write=SetOneTime, nodefault};
	__property Classes::TNotifyEvent OnTimer = {read=GetOnTimer, write=SetOnTimer};
	__property int Tag = {read=GetTag, write=SetTag, nodefault};
};


class PASCALIMPLEMENTATION TElPoolTimer : public TCustomElTimer 
{
	typedef TCustomElTimer inherited;
	
private:
	unsigned FElapsed;
	TElTimerPoolItem* FOwner;
	
protected:
	virtual void __fastcall SetEnabled(const bool Value);
	
public:
	void __fastcall Tick(int TickCount);
	__property unsigned Elapsed = {read=FElapsed, write=FElapsed, nodefault};
	__property Enabled ;
	__property Interval  = {default=1000};
	__property OneTime ;
	__property OnTimer ;
	__property TElTimerPoolItem* Owner = {read=FOwner};
	__property Tag ;
public:
	#pragma option push -w-inl
	/* TCustomElTimer.Create */ inline __fastcall virtual TElPoolTimer(void) : TCustomElTimer() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TElPoolTimer(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElTimerPoolItems;
class DELPHICLASS TElTimerPool;
class PASCALIMPLEMENTATION TElTimerPool : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	int FEnableCount;
	TElTimerPoolItems* FItems;
	int FTimerID;
	unsigned FLastTick;
	bool FPrecise;
	HWND FWnd;
	void __fastcall SetItems(TElTimerPoolItems* Value);
	void __fastcall WndProc(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall SetPrecise(bool newValue);
	virtual void __fastcall EnableTimer(bool Enable);
	virtual void __fastcall Loaded(void);
	
public:
	__fastcall virtual TElTimerPool(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTimerPool(void);
	void __fastcall Tick(int TickCount);
	
__published:
	__property TElTimerPoolItems* Items = {read=FItems, write=SetItems};
	__property bool Precise = {read=FPrecise, write=SetPrecise, nodefault};
};


class PASCALIMPLEMENTATION TElTimerPoolItems : public Classes::TCollection 
{
	typedef Classes::TCollection inherited;
	
public:
	TElTimerPoolItem* operator[](int Index) { return Items[Index]; }
	
private:
	TElTimerPool* FOwner;
	HIDESBASE TElTimerPoolItem* __fastcall GetItem(int Index);
	HIDESBASE void __fastcall SetItem(int Index, const TElTimerPoolItem* Value);
	
protected:
	DYNAMIC Classes::TPersistent* __fastcall GetOwner(void);
	virtual void __fastcall Update(Classes::TCollectionItem* Item);
	
public:
	__fastcall TElTimerPoolItems(TElTimerPool* AOwner);
	HIDESBASE TElTimerPoolItem* __fastcall Add(void);
	__property TElTimerPoolItem* Items[int Index] = {read=GetItem, write=SetItem/*, default*/};
public:
	#pragma option push -w-inl
	/* TCollection.Destroy */ inline __fastcall virtual ~TElTimerPoolItems(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltimers */
using namespace Eltimers;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTimers
