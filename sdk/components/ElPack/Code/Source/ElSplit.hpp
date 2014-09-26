// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElSplit.pas' rev: 6.00

#ifndef ElSplitHPP
#define ElSplitHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elsplit
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElSplitterEvent)(System::TObject* Sender, int &NewPos, bool &Accept);

class DELPHICLASS TElSplitter;
class PASCALIMPLEMENTATION TElSplitter : public Elpanel::TCustomElPanel 
{
	typedef Elpanel::TCustomElPanel inherited;
	
private:
	bool FSnapTopLeft;
	bool FSnapBottomRight;
	bool FAutoHide;
	HWND FOldFocused;
	bool FDragging;
	int FOffset;
	Controls::TControl* FControlTopLeft;
	Controls::TControl* FControlBottomRight;
	int FMinSizeTopLeft;
	int FMinSizeBottomRight;
	bool FAutoSnap;
	bool FLineVisible;
	HDC FLineDC;
	int FCurPos;
	int FSizeBeforeSnap;
	bool FLeftSnapButtonPushed;
	bool FLeftSnapButtonPushing;
	bool FRightSnapButtonPushed;
	bool FRightSnapButtonPushing;
	bool FShowSnapButton;
	Controls::TCursor FSnapButtonCursor;
	Graphics::TColor FSnapButtonColor;
	Graphics::TColor FSnapButtonDotColor;
	Graphics::TColor FSnapButtonArrowColor;
	Elhook::TElHook* FLeftHook;
	Elhook::TElHook* FRightHook;
	Classes::TNotifyEvent FOnPositionChanged;
	TElSplitterEvent FOnPositionChanging;
	unsigned FArrowTheme;
	void __fastcall AfterMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall SetMinSizeTopLeft(int newValue);
	void __fastcall SetControlTopLeft(Controls::TControl* newValue);
	void __fastcall SetControlBottomRight(Controls::TControl* newValue);
	void __fastcall DrawLine(void);
	void __fastcall AllocateLineDC(void);
	void __fastcall ReleaseLineDC(void);
	void __fastcall SetAutoHide(bool newValue);
	void __fastcall UpdateAutoVis(void);
	Types::TRect __fastcall GetLeftSnapButtonRect();
	void __fastcall RecalcCurPos(int X, int Y);
	void __fastcall StopMode(void);
	void __fastcall UpdateShowSnapButton(void);
	void __fastcall SetSnapBottomRight(const bool Value);
	void __fastcall SetSnapTopLeft(const bool Value);
	void __fastcall SetShowSnapButton(const bool Value);
	void __fastcall DoSizing(int L);
	void __fastcall SetSnapButtonArrowColor(const Graphics::TColor Value);
	void __fastcall SetSnapButtonColor(const Graphics::TColor Value);
	void __fastcall SetSnapButtonDotColor(const Graphics::TColor Value);
	Types::TRect __fastcall GetRightSnapButtonRect();
	
protected:
	bool FSnappedLeft;
	bool FSnappedRight;
	bool FInvertSnapButtons;
	virtual void __fastcall TriggerPositionChangedEvent(void);
	virtual void __fastcall TriggerPositionChangingEvent(int &NewPos, bool &Accept);
	HIDESBASE MESSAGE void __fastcall WMCancelMode(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetCursor(Messages::TWMSetCursor &Msg);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall StopSizing(int X, int Y, bool Accept);
	virtual void __fastcall Loaded(void);
	virtual WideString __fastcall GetThemedClassName();
	DYNAMIC void __fastcall RequestAlign(void);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall CreateArrowThemeHandle(void);
	virtual void __fastcall FreeArrowThemeHandle(void);
	void __fastcall SetInvertSnapButtons(bool Value);
	
public:
	__fastcall virtual TElSplitter(Classes::TComponent* AOwner);
	__fastcall virtual ~TElSplitter(void);
	virtual void __fastcall Paint(void);
	void __fastcall Snap(bool SnapLeft);
	__property unsigned ArrowTheme = {read=FArrowTheme, nodefault};
	__property bool SnappedLeft = {read=FSnappedLeft, nodefault};
	__property bool SnappedRight = {read=FSnappedRight, nodefault};
	
__published:
	__property int MinSizeTopLeft = {read=FMinSizeTopLeft, write=SetMinSizeTopLeft, default=20};
	__property int MinSizeBottomRight = {read=FMinSizeBottomRight, write=FMinSizeBottomRight, default=20};
	__property bool AutoSnap = {read=FAutoSnap, write=FAutoSnap, default=0};
	__property bool SnapTopLeft = {read=FSnapTopLeft, write=SetSnapTopLeft, nodefault};
	__property bool SnapBottomRight = {read=FSnapBottomRight, write=SetSnapBottomRight, nodefault};
	__property Controls::TControl* ControlTopLeft = {read=FControlTopLeft, write=SetControlTopLeft};
	__property Controls::TControl* ControlBottomRight = {read=FControlBottomRight, write=SetControlBottomRight};
	__property bool AutoHide = {read=FAutoHide, write=SetAutoHide, nodefault};
	__property bool ShowSnapButton = {read=FShowSnapButton, write=SetShowSnapButton, default=0};
	__property Controls::TCursor SnapButtonCursor = {read=FSnapButtonCursor, write=FSnapButtonCursor, default=0};
	__property Graphics::TColor SnapButtonColor = {read=FSnapButtonColor, write=SetSnapButtonColor, default=-2147483628};
	__property Graphics::TColor SnapButtonDotColor = {read=FSnapButtonDotColor, write=SetSnapButtonDotColor, default=-2147483632};
	__property Graphics::TColor SnapButtonArrowColor = {read=FSnapButtonArrowColor, write=SetSnapButtonArrowColor, default=-2147483632};
	__property Align  = {default=0};
	__property BevelInner  = {default=0};
	__property BevelOuter  = {default=2};
	__property BevelWidth  = {default=1};
	__property BorderStyle  = {default=0};
	__property BorderWidth  = {default=0};
	__property Color  = {default=-2147483633};
	__property Enabled  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property Classes::TNotifyEvent OnPositionChanged = {read=FOnPositionChanged, write=FOnPositionChanged};
	__property TElSplitterEvent OnPositionChanging = {read=FOnPositionChanging, write=FOnPositionChanging};
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
	__property bool InvertSnapButtons = {read=FInvertSnapButtons, write=SetInvertSnapButtons, default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElSplitter(HWND ParentWindow) : Elpanel::TCustomElPanel(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elsplit */
using namespace Elsplit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElSplit
