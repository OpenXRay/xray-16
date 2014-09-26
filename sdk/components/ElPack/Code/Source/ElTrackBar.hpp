// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTrackBar.pas' rev: 6.00

#ifndef ElTrackBarHPP
#define ElTrackBarHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltrackbar
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElTrackBarOrientation { toHorizontal, toVertical };
#pragma option pop

#pragma option push -b-
enum TElTrackTickPosition { tpNone, tpAbove, tpBelow, tpBoth };
#pragma option pop

#pragma option push -b-
enum TElTrackBarPart { tbpNowhere, tbSelMarkStart, tbSelMarkEnd, tbpTicksAbove, tbpTicksBelow, tbpTrack, tbpThumb };
#pragma option pop

#pragma option push -b-
enum TElTrackThumbType { tstPointerTop, tstPointerBottom, tstBox };
#pragma option pop

#pragma option push -b-
enum TTickMarkMode { tmmAuto, tmmManual };
#pragma option pop

typedef void __fastcall (__closure *TElTrackHitTestEvent)(System::TObject* Sender, int X, int Y, TElTrackBarPart &Part, bool &DefaultTest);

typedef void __fastcall (__closure *TElTrackDrawPartEvent)(System::TObject* Sender, Graphics::TCanvas* Canvas, const Types::TRect &R, TElTrackBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);

typedef void __fastcall (__closure *TElTrackChangeEvent)(System::TObject* Sender, int NewPos);

typedef void __fastcall (__closure *TElTrackChangingEvent)(System::TObject* Sender, int &NewPos, bool &AllowChange);

typedef void __fastcall (__closure *TElTickMarkPositionEvent)(System::TObject* Sender, int PrevPos, int &TickPos);

class DELPHICLASS TElTrackBar;
class PASCALIMPLEMENTATION TElTrackBar : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	TElTrackBarPart FMouseOverPart;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	
protected:
	bool FTransparent;
	TElTrackBarOrientation FOrientation;
	int FMax;
	int FMin;
	int FPage;
	bool FShowTrackHint;
	int FThumbPos;
	bool FMouseDown;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elimgfrm::TElImageForm* FImgForm;
	TElTrackTickPosition FTickPosition;
	int FOffsetLeft;
	int FOffsetRight;
	TElTrackDrawPartEvent FOnDrawPart;
	bool FOwnerDraw;
	TElTrackChangeEvent FOnChange;
	TElTrackChangingEvent FOnChanging;
	TElTrackThumbType FThumbType;
	int FTickWidth;
	int FSelStart;
	int FSelEnd;
	TElTickMarkPositionEvent FOnTickMark;
	int FFrequency;
	TTickMarkMode FTickMarkMode;
	Graphics::TColor FTickColor;
	Graphics::TColor FTrackColor;
	Graphics::TColor FTrackFrameColor;
	int FTrackWidth;
	Graphics::TColor FTrackSelColor;
	bool FShowSelection;
	int FSelectionMarkSize;
	int FThumbWidth;
	bool FThumbVisible;
	bool FDown;
	bool FDragging;
	#pragma pack(push, 1)
	Types::TPoint FDragPos;
	#pragma pack(pop)
	
	int FSavePosition;
	#pragma pack(push, 1)
	Types::TRect FSaveThumb;
	#pragma pack(pop)
	
	HWND FSaveCapture;
	int FPosition;
	TElTrackHitTestEvent FOnHitTest;
	Extctrls::TTimer* FDownTimer;
	Graphics::TColor FThumbColor;
	TElTrackChangeEvent FOnTrack;
	bool FFlat;
	WideString FHint;
	int __fastcall GetPosition(void);
	void __fastcall SetOrientation(TElTrackBarOrientation newValue);
	void __fastcall SetMax(int newValue);
	void __fastcall SetMin(int newValue);
	void __fastcall SetPage(int newValue);
	void __fastcall SetPosition(int newValue);
	void __fastcall DoSetMax(int newValue, bool Redraw);
	void __fastcall DoSetMin(int newValue, bool Redraw);
	void __fastcall DoSetPage(int newValue, bool Redraw);
	void __fastcall DoSetPosition(int newValue, bool Redraw);
	int __fastcall AdjustThumbPos(void);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	void __fastcall SetTickPosition(TElTrackTickPosition Value);
	void __fastcall SetOffsetLeft(int Value);
	void __fastcall SetOffsetRight(int Value);
	virtual void __fastcall TriggerDrawPartEvent(Graphics::TCanvas* Canvas, const Types::TRect &R, TElTrackBarPart Part, bool Enabled, bool Focused, bool Pressed, bool &DefaultDraw);
	void __fastcall SetOwnerDraw(bool Value);
	virtual void __fastcall TriggerChangeEvent(int NewPos);
	virtual void __fastcall TriggerChangingEvent(int &NewPos, bool &AllowChange);
	virtual int __fastcall GetAutoThumbSize(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall SetTransparent(bool newValue);
	void __fastcall SetThumbType(TElTrackThumbType Value);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	void __fastcall SetTickWidth(int Value);
	void __fastcall SetSelStart(int Value);
	void __fastcall SetSelEnd(int Value);
	virtual void __fastcall TriggerTickMarkPositionEvent(int PrevPos, int &TickPos);
	void __fastcall SetFrequency(int Value);
	void __fastcall SetTickMarkMode(TTickMarkMode Value);
	void __fastcall SetTickColor(Graphics::TColor Value);
	void __fastcall SetTrackColor(Graphics::TColor Value);
	void __fastcall SetTrackFrameColor(Graphics::TColor Value);
	void __fastcall SetTrackWidth(int Value);
	void __fastcall SetTrackSelColor(Graphics::TColor Value);
	void __fastcall SetShowSelection(bool Value);
	void __fastcall SetSelectionMarkSize(int Value);
	Types::TRect __fastcall CalcThumbRect();
	void __fastcall SetThumbWidth(int Value);
	void __fastcall SetThumbVisible(bool Value);
	void __fastcall DrawThumb(Graphics::TCanvas* Canvas, const Types::TRect &Rect);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall TriggerHitTestEvent(int X, int Y, TElTrackBarPart &Part, bool &DefaultTest);
	void __fastcall OnDownTimer(System::TObject* Sender);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetThumbColor(Graphics::TColor Value);
	virtual void __fastcall TriggerTrackEvent(int NewPos);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TMessage &Msg);
	virtual WideString __fastcall GetThemedClassName();
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	void __fastcall SetFlat(bool Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElTrackBar(Classes::TComponent* AOwner);
	virtual void __fastcall Paint(void);
	__fastcall virtual ~TElTrackBar(void);
	TElTrackBarPart __fastcall GetHitTest(int X, int Y);
	__property bool Tracking = {read=FDragging, nodefault};
	
__published:
	__property TElTrackBarOrientation Orientation = {read=FOrientation, write=SetOrientation, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property int Max = {read=FMax, write=SetMax, default=20};
	__property int Min = {read=FMin, write=SetMin, default=0};
	__property int Page = {read=FPage, write=SetPage, default=5};
	__property int Position = {read=GetPosition, write=SetPosition, default=0};
	__property bool ShowTrackHint = {read=FShowTrackHint, write=FShowTrackHint, default=0};
	__property TElTrackTickPosition TickPosition = {read=FTickPosition, write=SetTickPosition, default=1};
	__property int OffsetLeft = {read=FOffsetLeft, write=SetOffsetLeft, default=0};
	__property int OffsetRight = {read=FOffsetRight, write=SetOffsetRight, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool OwnerDraw = {read=FOwnerDraw, write=SetOwnerDraw, default=0};
	__property TElTrackThumbType ThumbType = {read=FThumbType, write=SetThumbType, default=0};
	__property int TickWidth = {read=FTickWidth, write=SetTickWidth, default=3};
	__property int SelStart = {read=FSelStart, write=SetSelStart, default=0};
	__property int SelEnd = {read=FSelEnd, write=SetSelEnd, nodefault};
	__property int Frequency = {read=FFrequency, write=SetFrequency, default=2};
	__property TTickMarkMode TickMarkMode = {read=FTickMarkMode, write=SetTickMarkMode, default=0};
	__property Graphics::TColor TickColor = {read=FTickColor, write=SetTickColor, default=-2147483630};
	__property Graphics::TColor TrackColor = {read=FTrackColor, write=SetTrackColor, default=-2147483643};
	__property Graphics::TColor TrackFrameColor = {read=FTrackFrameColor, write=SetTrackFrameColor, default=-2147483632};
	__property int TrackWidth = {read=FTrackWidth, write=SetTrackWidth, default=10};
	__property Graphics::TColor TrackSelColor = {read=FTrackSelColor, write=SetTrackSelColor, default=-2147483635};
	__property bool ShowSelection = {read=FShowSelection, write=SetShowSelection, default=0};
	__property int SelectionMarkSize = {read=FSelectionMarkSize, write=SetSelectionMarkSize, default=5};
	__property int ThumbWidth = {read=FThumbWidth, write=SetThumbWidth, default=9};
	__property bool ThumbVisible = {read=FThumbVisible, write=SetThumbVisible, default=1};
	__property TElTrackDrawPartEvent OnDrawPart = {read=FOnDrawPart, write=FOnDrawPart};
	__property TElTrackChangeEvent OnChange = {read=FOnChange, write=FOnChange};
	__property TElTrackChangingEvent OnChanging = {read=FOnChanging, write=FOnChanging};
	__property TElTickMarkPositionEvent OnTickMark = {read=FOnTickMark, write=FOnTickMark};
	__property TElTrackHitTestEvent OnHitTest = {read=FOnHitTest, write=FOnHitTest};
	__property Graphics::TColor ThumbColor = {read=FThumbColor, write=SetThumbColor, default=-2147483633};
	__property TElTrackChangeEvent OnTrack = {read=FOnTrack, write=FOnTrack};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Anchors  = {default=3};
	__property Constraints ;
	__property DragKind  = {default=0};
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property Enabled  = {default=1};
	__property ParentColor  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnMouseDown ;
	__property OnMouseUp ;
	__property OnMouseMove ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElTrackBar(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltrackbar */
using namespace Eltrackbar;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTrackBar
