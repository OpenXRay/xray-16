// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElGraphs.pas' rev: 6.00

#ifndef ElGraphsHPP
#define ElGraphsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElCGControl.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElQueue.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Printers.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
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

namespace Elgraphs
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TDataEntry;
class DELPHICLASS TElGraph;
class PASCALIMPLEMENTATION TElGraph : public Elcgcontrol::TElCustomGraphicControl 
{
	typedef Elcgcontrol::TElCustomGraphicControl inherited;
	
private:
	bool FShowTimeouts;
	AnsiString FStatus;
	TDataEntry* FColumnEntry;
	int FHGridLines;
	int FVGridLines;
	Classes::TNotifyEvent FOnResize;
	Graphics::TColor FLegendBkColor;
	int FLegendWidth;
	bool FLegendAtRight;
	Classes::TList* FEntryList;
	bool FShowLegend;
	bool FShowMinMax;
	bool FShowGrid;
	TDataEntry* FMinMaxEntry;
	Elimgfrm::TElImageForm* FImgForm;
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	TDataEntry* __fastcall GetDataList(int index);
	void __fastcall SetShowLegend(bool newValue);
	void __fastcall SetShowMinMax(bool newValue);
	int __fastcall GetEntriesCount(void);
	void __fastcall SetShowGrid(bool newValue);
	void __fastcall SetLegendAtRight(bool newValue);
	void __fastcall SetLegendWidth(int newValue);
	void __fastcall SetLegendBkColor(Graphics::TColor newValue);
	void __fastcall SetMinMaxEntry(TDataEntry* newValue);
	void __fastcall SetHGridLines(int newValue);
	void __fastcall SetVGridLines(int newValue);
	TDataEntry* __fastcall GetColumnEntry(void);
	void __fastcall SetColumnEntry(TDataEntry* newValue);
	MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	void __fastcall SetShowTimeouts(bool newValue);
	bool __fastcall GetTransparent(void);
	void __fastcall SetTransparent(bool newValue);
	
protected:
	virtual TDataEntry* __fastcall CreateEntry(void);
	virtual void __fastcall DoDrawGraph(Graphics::TCanvas* Surface);
	virtual void __fastcall Paint(void);
	virtual Types::TRect __fastcall GetLegendRect();
	virtual Types::TRect __fastcall GetMinMaxRect();
	virtual Types::TRect __fastcall GetMainRect();
	virtual void __fastcall TriggerResizeEvent(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	
public:
	virtual void __fastcall SetBounds(int ALeft, int ATop, int AWidth, int AHeight);
	__fastcall virtual TElGraph(Classes::TComponent* AOwner);
	__fastcall virtual ~TElGraph(void);
	void __fastcall Print(void);
	TDataEntry* __fastcall AddEntry(void);
	TDataEntry* __fastcall InsertEntry(int index);
	void __fastcall DeleteEntry(int index);
	HIDESBASE void __fastcall Refresh(void);
	__property TDataEntry* DataList[int index] = {read=GetDataList};
	__property int EntriesCount = {read=GetEntriesCount, nodefault};
	__property TDataEntry* MinMaxEntry = {read=FMinMaxEntry, write=SetMinMaxEntry};
	__property TDataEntry* ColumnEntry = {read=GetColumnEntry, write=SetColumnEntry};
	
__published:
	__property bool ShowLegend = {read=FShowLegend, write=SetShowLegend, nodefault};
	__property bool ShowMinMax = {read=FShowMinMax, write=SetShowMinMax, nodefault};
	__property bool ShowGrid = {read=FShowGrid, write=SetShowGrid, nodefault};
	__property Align  = {default=0};
	__property Canvas ;
	__property Color ;
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property bool LegendAtRight = {read=FLegendAtRight, write=SetLegendAtRight, default=1};
	__property int LegendWidth = {read=FLegendWidth, write=SetLegendWidth, default=100};
	__property Graphics::TColor LegendBkColor = {read=FLegendBkColor, write=SetLegendBkColor, nodefault};
	__property Classes::TNotifyEvent OnResize = {read=FOnResize, write=FOnResize};
	__property int HGridLines = {read=FHGridLines, write=SetHGridLines, default=5};
	__property int VGridLines = {read=FVGridLines, write=SetVGridLines, default=0};
	__property AnsiString Status = {read=FStatus, write=FStatus};
	__property bool ShowTimeouts = {read=FShowTimeouts, write=SetShowTimeouts, default=1};
	__property bool Transparent = {read=GetTransparent, write=SetTransparent, nodefault};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
};


class PASCALIMPLEMENTATION TDataEntry : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	int operator[](int index) { return Value[index]; }
	
private:
	Elqueue::TElQueue* FValues;
	Graphics::TColor FColor;
	AnsiString FName;
	int FMinGrid;
	int FMaxGrid;
	TElGraph* FOwner;
	bool FVisible;
	bool FAutoGrid;
	int FFaults;
	int __fastcall GetLimit(void);
	void __fastcall SetLimit(int newValue);
	void __fastcall SetColor(Graphics::TColor value);
	void __fastcall SetName(AnsiString value);
	void __fastcall SetMinGrid(int newValue);
	void __fastcall SetMaxGrid(int newValue);
	void __fastcall SetVisible(bool value);
	int __fastcall GetValueCount(void);
	
public:
	void __fastcall CalcMinMax(int &Min, int &Max, int &Avg);
	__fastcall TDataEntry(void);
	__fastcall virtual ~TDataEntry(void);
	void __fastcall AddValue(int value);
	int __fastcall GetValue(int index);
	void __fastcall Reset(void);
	__property AnsiString Name = {read=FName, write=SetName};
	__property TElGraph* Owner = {read=FOwner};
	__property Elqueue::TElQueue* Values = {read=FValues};
	__property int Value[int index] = {read=GetValue/*, default*/};
	__property int ValueCount = {read=GetValueCount, nodefault};
	__property Graphics::TColor Color = {read=FColor, write=SetColor, nodefault};
	__property int MinGrid = {read=FMinGrid, write=SetMinGrid, nodefault};
	__property int MaxGrid = {read=FMaxGrid, write=SetMaxGrid, nodefault};
	__property bool Visible = {read=FVisible, write=SetVisible, nodefault};
	__property int Limit = {read=GetLimit, write=SetLimit, default=1000};
	__property bool AutoGrid = {read=FAutoGrid, write=FAutoGrid, nodefault};
	__property int Faults = {read=FFaults, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elgraphs */
using namespace Elgraphs;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElGraphs
