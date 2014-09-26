// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'MXCtrls.pas' rev: 6.00

#ifndef MXCtrlsHPP
#define MXCtrlsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mxPlacemnt.hpp>	// Pascal unit
#include <mxConst.hpp>	// Pascal unit
#include <IniFiles.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Buttons.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxctrls
{
//-- type declarations -------------------------------------------------------
typedef int TPositiveInt;

class DELPHICLASS TTextListBox;
class PASCALIMPLEMENTATION TTextListBox : public Stdctrls::TCustomListBox 
{
	typedef Stdctrls::TCustomListBox inherited;
	
private:
	int FMaxWidth;
	void __fastcall ResetHorizontalExtent(void);
	void __fastcall SetHorizontalExtent(void);
	int __fastcall GetItemWidth(int Index);
	
protected:
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	
__published:
	__property Align  = {default=0};
	__property BorderStyle  = {default=1};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property ExtendedSelect  = {default=1};
	__property Font ;
	__property IntegralHeight  = {default=0};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property ItemHeight ;
	__property Items ;
	__property MultiSelect  = {default=0};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Sorted  = {default=0};
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property TabWidth  = {default=0};
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property OnContextPopup ;
	__property OnMouseWheelDown ;
	__property OnMouseWheelUp ;
	__property OnEndDock ;
	__property OnStartDock ;
public:
	#pragma option push -w-inl
	/* TCustomListBox.Create */ inline __fastcall virtual TTextListBox(Classes::TComponent* AOwner) : Stdctrls::TCustomListBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomListBox.Destroy */ inline __fastcall virtual ~TTextListBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TTextListBox(HWND ParentWindow) : Stdctrls::TCustomListBox(ParentWindow) { }
	#pragma option pop
	
};


typedef void __fastcall (__closure *TGetItemWidthEvent)(Controls::TWinControl* Control, int Index, int &Width);

class DELPHICLASS TMxCustomListBox;
class PASCALIMPLEMENTATION TMxCustomListBox : public Controls::TWinControl 
{
	typedef Controls::TWinControl inherited;
	
private:
	Classes::TStrings* FItems;
	Forms::TFormBorderStyle FBorderStyle;
	Graphics::TCanvas* FCanvas;
	int FColumns;
	int FItemHeight;
	Stdctrls::TListBoxStyle FStyle;
	bool FIntegralHeight;
	bool FMultiSelect;
	bool FSorted;
	bool FExtendedSelect;
	int FTabWidth;
	Classes::TStringList* FSaveItems;
	int FSaveTopIndex;
	int FSaveItemIndex;
	bool FAutoScroll;
	bool FGraySelection;
	int FMaxItemWidth;
	Stdctrls::TDrawItemEvent FOnDrawItem;
	Stdctrls::TMeasureItemEvent FOnMeasureItem;
	TGetItemWidthEvent FOnGetItemWidth;
	void __fastcall ResetHorizontalExtent(void);
	void __fastcall SetHorizontalExtent(void);
	bool __fastcall GetAutoScroll(void);
	virtual int __fastcall GetItemHeight(void);
	int __fastcall GetItemIndex(void);
	int __fastcall GetSelCount(void);
	bool __fastcall GetSelected(int Index);
	int __fastcall GetTopIndex(void);
	void __fastcall SetAutoScroll(bool Value);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall SetColumnWidth(void);
	void __fastcall SetColumns(int Value);
	void __fastcall SetExtendedSelect(bool Value);
	void __fastcall SetIntegralHeight(bool Value);
	void __fastcall SetItemHeight(int Value);
	void __fastcall SetItemIndex(int Value);
	void __fastcall SetMultiSelect(bool Value);
	void __fastcall SetSelected(int Index, bool Value);
	void __fastcall SetSorted(bool Value);
	void __fastcall SetStyle(Stdctrls::TListBoxStyle Value);
	void __fastcall SetTabWidth(int Value);
	void __fastcall SetTopIndex(int Value);
	void __fastcall SetGraySelection(bool Value);
	void __fastcall SetOnDrawItem(Stdctrls::TDrawItemEvent Value);
	void __fastcall SetOnGetItemWidth(TGetItemWidthEvent Value);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Message);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	MESSAGE void __fastcall CNDrawItem(Messages::TWMDrawItem &Message);
	MESSAGE void __fastcall CNMeasureItem(Messages::TWMMeasureItem &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMNCHitTest(Messages::TWMNCHitTest &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Message);
	
protected:
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	virtual Classes::TStrings* __fastcall CreateItemList(void);
	virtual int __fastcall GetItemWidth(int Index);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	DYNAMIC void __fastcall DragCanceled(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall MeasureItem(int Index, int &Height);
	DYNAMIC int __fastcall GetItemData(int Index);
	DYNAMIC void __fastcall SetItemData(int Index, int AData);
	virtual void __fastcall SetItems(Classes::TStrings* Value);
	DYNAMIC void __fastcall ResetContent(void);
	DYNAMIC void __fastcall DeleteString(int Index);
	__property bool AutoScroll = {read=GetAutoScroll, write=SetAutoScroll, default=0};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property int Columns = {read=FColumns, write=SetColumns, default=0};
	__property bool ExtendedSelect = {read=FExtendedSelect, write=SetExtendedSelect, default=1};
	__property bool GraySelection = {read=FGraySelection, write=SetGraySelection, default=0};
	__property bool IntegralHeight = {read=FIntegralHeight, write=SetIntegralHeight, default=0};
	__property int ItemHeight = {read=GetItemHeight, write=SetItemHeight, nodefault};
	__property bool MultiSelect = {read=FMultiSelect, write=SetMultiSelect, default=0};
	__property ParentColor  = {default=0};
	__property bool Sorted = {read=FSorted, write=SetSorted, default=0};
	__property Stdctrls::TListBoxStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property int TabWidth = {read=FTabWidth, write=SetTabWidth, default=0};
	__property Stdctrls::TDrawItemEvent OnDrawItem = {read=FOnDrawItem, write=SetOnDrawItem};
	__property Stdctrls::TMeasureItemEvent OnMeasureItem = {read=FOnMeasureItem, write=FOnMeasureItem};
	__property TGetItemWidthEvent OnGetItemWidth = {read=FOnGetItemWidth, write=SetOnGetItemWidth};
	
public:
	__fastcall virtual TMxCustomListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TMxCustomListBox(void);
	void __fastcall Clear(void);
	void __fastcall DefaultDrawText(int X, int Y, const AnsiString S);
	int __fastcall ItemAtPos(const Types::TPoint &Pos, bool Existing);
	Types::TRect __fastcall ItemRect(int Index);
	__property Graphics::TCanvas* Canvas = {read=FCanvas};
	__property Classes::TStrings* Items = {read=FItems, write=SetItems};
	__property int ItemIndex = {read=GetItemIndex, write=SetItemIndex, nodefault};
	__property int SelCount = {read=GetSelCount, nodefault};
	__property bool Selected[int Index] = {read=GetSelected, write=SetSelected};
	__property int TopIndex = {read=GetTopIndex, write=SetTopIndex, nodefault};
	
__published:
	__property TabStop  = {default=1};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TMxCustomListBox(HWND ParentWindow) : Controls::TWinControl(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TCheckKind { ckCheckBoxes, ckRadioButtons, ckCheckMarks };
#pragma option pop

typedef void __fastcall (__closure *TChangeStateEvent)(System::TObject* Sender, int Index);

class DELPHICLASS TMxCheckListBox;
class PASCALIMPLEMENTATION TMxCheckListBox : public TMxCustomListBox 
{
	typedef TMxCustomListBox inherited;
	
private:
	bool FAllowGrayed;
	TCheckKind FCheckKind;
	Classes::TList* FSaveStates;
	Graphics::TBitmap* FDrawBitmap;
	int FCheckWidth;
	int FCheckHeight;
	int FReserved;
	bool FInUpdateStates;
	Mxplacemnt::TIniLink* FIniLink;
	Classes::TNotifyEvent FOnClickCheck;
	TChangeStateEvent FOnStateChange;
	void __fastcall ResetItemHeight(void);
	virtual int __fastcall GetItemHeight(void);
	void __fastcall DrawCheck(const Types::TRect &R, Stdctrls::TCheckBoxState AState, bool Enabled);
	void __fastcall SetCheckKind(TCheckKind Value);
	void __fastcall SetChecked(int Index, bool AChecked);
	bool __fastcall GetChecked(int Index);
	void __fastcall SetState(int Index, Stdctrls::TCheckBoxState AState);
	Stdctrls::TCheckBoxState __fastcall GetState(int Index);
	void __fastcall SetItemEnabled(int Index, bool Value);
	bool __fastcall GetItemEnabled(int Index);
	bool __fastcall GetAllowGrayed(void);
	void __fastcall ToggleClickCheck(int Index);
	void __fastcall InvalidateCheck(int Index);
	void __fastcall InvalidateItem(int Index);
	System::TObject* __fastcall CreateCheckObject(int Index);
	System::TObject* __fastcall FindCheckObject(int Index);
	System::TObject* __fastcall GetCheckObject(int Index);
	bool __fastcall IsCheckObject(int Index);
	void __fastcall ReadVersion(Classes::TReader* Reader);
	void __fastcall WriteVersion(Classes::TWriter* Writer);
	void __fastcall ReadCheckData(Classes::TReader* Reader);
	void __fastcall WriteCheckData(Classes::TWriter* Writer);
	void __fastcall InternalSaveStates(System::TObject* IniFile, const AnsiString Section);
	void __fastcall InternalRestoreStates(System::TObject* IniFile, const AnsiString Section);
	Mxplacemnt::TFormPlacement* __fastcall GetStorage(void);
	void __fastcall SetStorage(Mxplacemnt::TFormPlacement* Value);
	void __fastcall IniSave(System::TObject* Sender);
	void __fastcall IniLoad(System::TObject* Sender);
	void __fastcall UpdateCheckStates(void);
	int __fastcall GetCheckedIndex(void);
	void __fastcall SetCheckedIndex(int Value);
	HIDESBASE MESSAGE void __fastcall CNDrawItem(Messages::TWMDrawItem &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	
protected:
	virtual Classes::TStrings* __fastcall CreateItemList(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	virtual int __fastcall GetItemWidth(int Index);
	DYNAMIC int __fastcall GetItemData(int Index);
	DYNAMIC void __fastcall SetItemData(int Index, int AData);
	DYNAMIC void __fastcall KeyPress(char &Key);
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall ResetContent(void);
	DYNAMIC void __fastcall DeleteString(int Index);
	DYNAMIC void __fastcall ClickCheck(void);
	DYNAMIC void __fastcall ChangeItemState(int Index);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	HIDESBASE MESSAGE void __fastcall WMDestroy(Messages::TWMNoParams &Msg);
	int __fastcall GetCheckWidth(void);
	virtual void __fastcall SetItems(Classes::TStrings* Value);
	
public:
	__fastcall virtual TMxCheckListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TMxCheckListBox(void);
	void __fastcall SaveStatesReg(Registry::TRegIniFile* IniFile);
	void __fastcall RestoreStatesReg(Registry::TRegIniFile* IniFile);
	void __fastcall SaveStates(Inifiles::TIniFile* IniFile);
	void __fastcall RestoreStates(Inifiles::TIniFile* IniFile);
	void __fastcall ApplyState(Stdctrls::TCheckBoxState AState, bool EnabledOnly);
	__property bool Checked[int Index] = {read=GetChecked, write=SetChecked};
	__property Stdctrls::TCheckBoxState State[int Index] = {read=GetState, write=SetState};
	__property bool EnabledItem[int Index] = {read=GetItemEnabled, write=SetItemEnabled};
	
__published:
	__property bool AllowGrayed = {read=GetAllowGrayed, write=FAllowGrayed, default=0};
	__property TCheckKind CheckKind = {read=FCheckKind, write=SetCheckKind, default=0};
	__property int CheckedIndex = {read=GetCheckedIndex, write=SetCheckedIndex, default=-1};
	__property Mxplacemnt::TFormPlacement* IniStorage = {read=GetStorage, write=SetStorage};
	__property Align  = {default=0};
	__property AutoScroll  = {default=1};
	__property BorderStyle  = {default=1};
	__property Color  = {default=-2147483643};
	__property Columns  = {default=0};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property ExtendedSelect  = {default=1};
	__property Font ;
	__property GraySelection  = {default=0};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property IntegralHeight  = {default=0};
	__property ItemHeight ;
	__property Items  = {stored=false};
	__property MultiSelect  = {default=0};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Sorted  = {default=0};
	__property Style  = {default=0};
	__property TabOrder  = {default=-1};
	__property TabWidth  = {default=0};
	__property Visible  = {default=1};
	__property TChangeStateEvent OnStateChange = {read=FOnStateChange, write=FOnStateChange};
	__property Classes::TNotifyEvent OnClickCheck = {read=FOnClickCheck, write=FOnClickCheck};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnDrawItem ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnGetItemWidth ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMeasureItem ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property OnContextPopup ;
	__property OnMouseWheelDown ;
	__property OnMouseWheelUp ;
	__property OnEndDock ;
	__property OnStartDock ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TMxCheckListBox(HWND ParentWindow) : TMxCustomListBox(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TShadowPosition { spLeftTop, spLeftBottom, spRightBottom, spRightTop };
#pragma option pop

class DELPHICLASS TMxCustomLabel;
class PASCALIMPLEMENTATION TMxCustomLabel : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	Controls::TWinControl* FFocusControl;
	Classes::TAlignment FAlignment;
	bool FAutoSize;
	Stdctrls::TTextLayout FLayout;
	Graphics::TColor FShadowColor;
	Byte FShadowSize;
	TShadowPosition FShadowPos;
	bool FWordWrap;
	bool FShowAccelChar;
	bool FShowFocus;
	bool FFocused;
	bool FMouseInControl;
	bool FDragging;
	int FLeftMargin;
	int FRightMargin;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	void __fastcall DoDrawText(Types::TRect &Rect, Word Flags);
	bool __fastcall GetTransparent(void);
	void __fastcall UpdateTracking(void);
	void __fastcall SetAlignment(Classes::TAlignment Value);
	HIDESBASE void __fastcall SetAutoSize(bool Value);
	void __fastcall SetFocusControl(Controls::TWinControl* Value);
	void __fastcall SetLayout(Stdctrls::TTextLayout Value);
	void __fastcall SetLeftMargin(int Value);
	void __fastcall SetRightMargin(int Value);
	void __fastcall SetShadowColor(Graphics::TColor Value);
	void __fastcall SetShadowSize(Byte Value);
	void __fastcall SetShadowPos(TShadowPosition Value);
	void __fastcall SetShowAccelChar(bool Value);
	void __fastcall SetTransparent(bool Value);
	void __fastcall SetWordWrap(bool Value);
	void __fastcall SetShowFocus(bool Value);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMFocusChanged(Controls::TCMFocusChanged &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMVisibleChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	
protected:
	void __fastcall AdjustBounds(void);
	virtual Graphics::TColor __fastcall GetDefaultFontColor(void);
	virtual AnsiString __fastcall GetLabelCaption();
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall Paint(void);
	DYNAMIC void __fastcall MouseEnter(void);
	DYNAMIC void __fastcall MouseLeave(void);
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, default=0};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property Controls::TWinControl* FocusControl = {read=FFocusControl, write=SetFocusControl};
	__property Stdctrls::TTextLayout Layout = {read=FLayout, write=SetLayout, default=0};
	__property int LeftMargin = {read=FLeftMargin, write=SetLeftMargin, default=0};
	__property int RightMargin = {read=FRightMargin, write=SetRightMargin, default=0};
	__property Graphics::TColor ShadowColor = {read=FShadowColor, write=SetShadowColor, default=-2147483628};
	__property Byte ShadowSize = {read=FShadowSize, write=SetShadowSize, default=1};
	__property TShadowPosition ShadowPos = {read=FShadowPos, write=SetShadowPos, default=0};
	__property bool ShowAccelChar = {read=FShowAccelChar, write=SetShowAccelChar, default=1};
	__property bool ShowFocus = {read=FShowFocus, write=SetShowFocus, default=0};
	__property bool Transparent = {read=GetTransparent, write=SetTransparent, default=0};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, default=0};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	
public:
	__fastcall virtual TMxCustomLabel(Classes::TComponent* AOwner);
	__property Canvas ;
	__property bool MouseInControl = {read=FMouseInControl, nodefault};
public:
	#pragma option push -w-inl
	/* TGraphicControl.Destroy */ inline __fastcall virtual ~TMxCustomLabel(void) { }
	#pragma option pop
	
};


class DELPHICLASS TMxLabel;
class PASCALIMPLEMENTATION TMxLabel : public TMxCustomLabel 
{
	typedef TMxCustomLabel inherited;
	
__published:
	__property Align  = {default=0};
	__property Alignment  = {default=0};
	__property AutoSize  = {default=1};
	__property Caption ;
	__property Color  = {default=-2147483643};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property FocusControl ;
	__property Font ;
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property Layout  = {default=0};
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShadowColor  = {default=-2147483628};
	__property ShadowSize  = {default=1};
	__property ShadowPos  = {default=0};
	__property ShowAccelChar  = {default=1};
	__property ShowFocus  = {default=0};
	__property ShowHint ;
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
	__property WordWrap  = {default=0};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnStartDrag ;
	__property OnContextPopup ;
	__property OnEndDock ;
	__property OnStartDock ;
public:
	#pragma option push -w-inl
	/* TMxCustomLabel.Create */ inline __fastcall virtual TMxLabel(Classes::TComponent* AOwner) : TMxCustomLabel(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TGraphicControl.Destroy */ inline __fastcall virtual ~TMxLabel(void) { }
	#pragma option pop
	
};


class DELPHICLASS TMxPanel;
class PASCALIMPLEMENTATION TMxPanel : public Extctrls::TCustomPanel 
{
	typedef Extctrls::TCustomPanel inherited;
	
public:
	__property Canvas ;
	__property DockManager ;
	
protected:
	Classes::TNotifyEvent FOnPaint;
	virtual void __fastcall Paint(void);
	
__published:
	__property Align  = {default=0};
	__property Alignment  = {default=2};
	__property Anchors  = {default=3};
	__property AutoSize  = {default=0};
	__property BevelInner  = {default=0};
	__property BevelOuter  = {default=2};
	__property BevelWidth  = {default=1};
	__property BiDiMode ;
	__property BorderWidth  = {default=0};
	__property BorderStyle  = {default=0};
	__property Caption ;
	__property Color  = {default=-2147483633};
	__property Constraints ;
	__property Ctl3D ;
	__property UseDockManager  = {default=1};
	__property DockSite  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property FullRepaint  = {default=1};
	__property Font ;
	__property Locked  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property Visible  = {default=1};
	__property OnCanResize ;
	__property OnClick ;
	__property OnConstrainedResize ;
	__property OnContextPopup ;
	__property OnDockDrop ;
	__property OnDockOver ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnGetSiteInfo ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnResize ;
	__property OnStartDock ;
	__property OnStartDrag ;
	__property OnUnDock ;
	__property Classes::TNotifyEvent OnPaint = {read=FOnPaint, write=FOnPaint};
public:
	#pragma option push -w-inl
	/* TCustomPanel.Create */ inline __fastcall virtual TMxPanel(Classes::TComponent* AOwner) : Extctrls::TCustomPanel(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TCustomControl.Destroy */ inline __fastcall virtual ~TMxPanel(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TMxPanel(HWND ParentWindow) : Extctrls::TCustomPanel(ParentWindow) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TGlyphLayout { glGlyphLeft, glGlyphRight, glGlyphTop, glGlyphBottom };
#pragma option pop

#pragma option push -b-
enum TScrollDirection { sdVertical, sdHorizontal };
#pragma option pop

typedef void __fastcall (__closure *TPanelDrawEvent)(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &Rect);

typedef Shortint TMxNumGlyphs;

#pragma option push -b-
enum TMxDropDownMenuPos { dmpBottom, dmpRight };
#pragma option pop

#pragma option push -b-
enum TMxButtonState { rbsUp, rbsDisabled, rbsDown, rbsExclusive, rbsInactive };
#pragma option pop

class DELPHICLASS TMxSpeedButton;
class PASCALIMPLEMENTATION TMxSpeedButton : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	int FGroupIndex;
	Buttons::TButtonStyle FStyle;
	void *FGlyph;
	Graphics::TBitmap* FDrawImage;
	bool FDown;
	bool FDragging;
	bool FFlat;
	bool FMouseInControl;
	bool FAllowAllUp;
	Buttons::TButtonLayout FLayout;
	int FSpacing;
	int FMargin;
	Controls::TModalResult FModalResult;
	bool FTransparent;
	bool FMarkDropDown;
	Menus::TPopupMenu* FDropDownMenu;
	TMxDropDownMenuPos FMenuPosition;
	bool FInactiveGrayed;
	bool FMenuTracking;
	Extctrls::TTimer* FRepeatTimer;
	bool FAllowTimer;
	Word FInitRepeatPause;
	Word FRepeatPause;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall UpdateExclusive(void);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	TMxNumGlyphs __fastcall GetNumGlyphs(void);
	void __fastcall SetNumGlyphs(TMxNumGlyphs Value);
	bool __fastcall GetWordWrap(void);
	void __fastcall SetWordWrap(bool Value);
	Classes::TAlignment __fastcall GetAlignment(void);
	void __fastcall SetAlignment(Classes::TAlignment Value);
	void __fastcall SetDown(bool Value);
	void __fastcall SetAllowAllUp(bool Value);
	void __fastcall SetGroupIndex(int Value);
	void __fastcall SetLayout(Buttons::TButtonLayout Value);
	void __fastcall SetSpacing(int Value);
	void __fastcall SetMargin(int Value);
	void __fastcall SetDropDownMenu(Menus::TPopupMenu* Value);
	void __fastcall SetFlat(bool Value);
	void __fastcall SetStyle(Buttons::TButtonStyle Value);
	void __fastcall SetInactiveGrayed(bool Value);
	void __fastcall SetTransparent(bool Value);
	void __fastcall SetMarkDropDown(bool Value);
	void __fastcall TimerExpired(System::TObject* Sender);
	void __fastcall SetAllowTimer(bool Value);
	bool __fastcall CheckMenuDropDown(const Types::TSmallPoint Pos, bool Manual);
	HIDESBASE void __fastcall DoMouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	MESSAGE void __fastcall CMButtonPressed(Messages::TMessage &Message);
	MESSAGE void __fastcall CMDialogChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMVisibleChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDblClk(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	
protected:
	TMxButtonState FState;
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	Types::TPoint __fastcall GetDropDownMenuPos();
	DYNAMIC HPALETTE __fastcall GetPalette(void);
	virtual void __fastcall Paint(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall PaintGlyph(Graphics::TCanvas* Canvas, const Types::TRect &ARect, TMxButtonState AState, bool DrawMark);
	DYNAMIC void __fastcall MouseEnter(void);
	DYNAMIC void __fastcall MouseLeave(void);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	__property void * ButtonGlyph = {read=FGlyph};
	
public:
	__fastcall virtual TMxSpeedButton(Classes::TComponent* AOwner);
	__fastcall virtual ~TMxSpeedButton(void);
	void __fastcall ButtonClick(void);
	bool __fastcall CheckBtnMenuDropDown(void);
	DYNAMIC void __fastcall Click(void);
	void __fastcall UpdateTracking(void);
	
__published:
	__property Action ;
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property Classes::TAlignment Alignment = {read=GetAlignment, write=SetAlignment, default=2};
	__property bool AllowAllUp = {read=FAllowAllUp, write=SetAllowAllUp, default=0};
	__property bool AllowTimer = {read=FAllowTimer, write=SetAllowTimer, default=0};
	__property int GroupIndex = {read=FGroupIndex, write=SetGroupIndex, default=0};
	__property bool Down = {read=FDown, write=SetDown, default=0};
	__property Menus::TPopupMenu* DropDownMenu = {read=FDropDownMenu, write=SetDropDownMenu};
	__property TMxDropDownMenuPos MenuPosition = {read=FMenuPosition, write=FMenuPosition, default=0};
	__property Caption ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Font ;
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property bool GrayedInactive = {read=FInactiveGrayed, write=SetInactiveGrayed, default=1};
	__property Word InitPause = {read=FInitRepeatPause, write=FInitRepeatPause, default=500};
	__property Buttons::TButtonLayout Layout = {read=FLayout, write=SetLayout, default=2};
	__property int Margin = {read=FMargin, write=SetMargin, default=-1};
	__property bool MarkDropDown = {read=FMarkDropDown, write=SetMarkDropDown, default=1};
	__property Controls::TModalResult ModalResult = {read=FModalResult, write=FModalResult, default=0};
	__property TMxNumGlyphs NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=0};
	__property Word RepeatInterval = {read=FRepeatPause, write=FRepeatPause, default=100};
	__property ShowHint  = {default=1};
	__property int Spacing = {read=FSpacing, write=SetSpacing, default=1};
	__property Buttons::TButtonStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool WordWrap = {read=GetWordWrap, write=SetWordWrap, default=0};
	__property Visible  = {default=1};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property OnEndDock ;
	__property OnStartDock ;
};


class DELPHICLASS TButtonImage;
class PASCALIMPLEMENTATION TButtonImage : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	System::TObject* FGlyph;
	#pragma pack(push, 1)
	Types::TPoint FButtonSize;
	#pragma pack(pop)
	
	AnsiString FCaption;
	TMxNumGlyphs __fastcall GetNumGlyphs(void);
	void __fastcall SetNumGlyphs(TMxNumGlyphs Value);
	bool __fastcall GetWordWrap(void);
	void __fastcall SetWordWrap(bool Value);
	Classes::TAlignment __fastcall GetAlignment(void);
	void __fastcall SetAlignment(Classes::TAlignment Value);
	Graphics::TBitmap* __fastcall GetGlyph(void);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	
public:
	__fastcall TButtonImage(void);
	__fastcall virtual ~TButtonImage(void);
	void __fastcall Invalidate(void);
	void __fastcall DrawEx(Graphics::TCanvas* Canvas, int X, int Y, int Margin, int Spacing, Buttons::TButtonLayout Layout, Graphics::TFont* AFont, Controls::TImageList* Images, int ImageIndex, Word Flags);
	void __fastcall Draw(Graphics::TCanvas* Canvas, int X, int Y, int Margin, int Spacing, Buttons::TButtonLayout Layout, Graphics::TFont* AFont, Word Flags);
	__property Classes::TAlignment Alignment = {read=GetAlignment, write=SetAlignment, nodefault};
	__property AnsiString Caption = {read=FCaption, write=FCaption};
	__property Graphics::TBitmap* Glyph = {read=GetGlyph, write=SetGlyph};
	__property TMxNumGlyphs NumGlyphs = {read=GetNumGlyphs, write=SetNumGlyphs, nodefault};
	__property Types::TPoint ButtonSize = {read=FButtonSize, write=FButtonSize};
	__property bool WordWrap = {read=GetWordWrap, write=SetWordWrap, nodefault};
};


class DELPHICLASS TMxButtonGlyph;
class PASCALIMPLEMENTATION TMxButtonGlyph : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Graphics::TBitmap* FOriginal;
	Controls::TImageList* FGlyphList;
	int FIndexs[5];
	Graphics::TColor FTransparentColor;
	TMxNumGlyphs FNumGlyphs;
	bool FWordWrap;
	Classes::TAlignment FAlignment;
	Classes::TNotifyEvent FOnChange;
	void __fastcall GlyphChanged(System::TObject* Sender);
	void __fastcall SetGlyph(Graphics::TBitmap* Value);
	void __fastcall SetNumGlyphs(TMxNumGlyphs Value);
	Graphics::TColor __fastcall MapColor(Graphics::TColor Color);
	
protected:
	void __fastcall MinimizeCaption(Graphics::TCanvas* Canvas, const AnsiString Caption, char * Buffer, int MaxLen, int Width);
	int __fastcall CreateButtonGlyph(TMxButtonState State);
	int __fastcall CreateImageGlyph(TMxButtonState State, Controls::TImageList* Images, int Index);
	void __fastcall CalcButtonLayout(Graphics::TCanvas* Canvas, const Types::TRect &Client, AnsiString &Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, bool PopupMark, Types::TPoint &GlyphPos, Types::TRect &TextBounds, Word Flags, Controls::TImageList* Images, int ImageIndex);
	
public:
	__fastcall TMxButtonGlyph(void);
	__fastcall virtual ~TMxButtonGlyph(void);
	void __fastcall Invalidate(void);
	Types::TPoint __fastcall DrawButtonGlyph(Graphics::TCanvas* Canvas, int X, int Y, TMxButtonState State);
	Types::TPoint __fastcall DrawButtonImage(Graphics::TCanvas* Canvas, int X, int Y, Controls::TImageList* Images, int ImageIndex, TMxButtonState State);
	Types::TRect __fastcall DrawEx(Graphics::TCanvas* Canvas, const Types::TRect &Client, const AnsiString Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, bool PopupMark, Controls::TImageList* Images, int ImageIndex, TMxButtonState State, Word Flags);
	void __fastcall DrawButtonText(Graphics::TCanvas* Canvas, const AnsiString Caption, const Types::TRect &TextBounds, TMxButtonState State, Word Flags);
	void __fastcall DrawPopupMark(Graphics::TCanvas* Canvas, int X, int Y, TMxButtonState State);
	Types::TRect __fastcall Draw(Graphics::TCanvas* Canvas, const Types::TRect &Client, const AnsiString Caption, Buttons::TButtonLayout Layout, int Margin, int Spacing, bool PopupMark, TMxButtonState State, Word Flags);
	__property Classes::TAlignment Alignment = {read=FAlignment, write=FAlignment, nodefault};
	__property Graphics::TBitmap* Glyph = {read=FOriginal, write=SetGlyph};
	__property TMxNumGlyphs NumGlyphs = {read=FNumGlyphs, write=SetNumGlyphs, nodefault};
	__property bool WordWrap = {read=FWordWrap, write=FWordWrap, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
};


//-- var, const, procedure ---------------------------------------------------
#define clbDefaultState (Stdctrls::TCheckBoxState)(0)
static const bool clbDefaultEnabled = true;
extern PACKAGE Graphics::TBitmap* __fastcall CheckBitmap(void);
extern PACKAGE int __fastcall DrawShadowText(HDC DC, char * Str, int Count, Types::TRect &Rect, Word Format, Byte ShadowSize, unsigned ShadowColor, TShadowPosition ShadowPos);

}	/* namespace Mxctrls */
using namespace Mxctrls;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// MXCtrls
