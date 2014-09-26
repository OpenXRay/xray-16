// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElEdits.pas' rev: 6.00

#ifndef ElEditsHPP
#define ElEditsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Imm.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElScrollBar.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Clipbrd.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eledits
{
//-- type declarations -------------------------------------------------------
typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

class DELPHICLASS TElEditStrings;
class DELPHICLASS TCustomElEdit;
#pragma option push -b-
enum TElEditCharCase { eecNormal, eecUpperCase, eecLowerCase };
#pragma option pop

class PASCALIMPLEMENTATION TCustomElEdit : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	bool FModified;
	#pragma pack(push, 1)
	Types::TRect FEditRect;
	#pragma pack(pop)
	
	int FLeftMargin;
	int FTopMargin;
	int FRightMargin;
	bool FMouseClick;
	Forms::TFormBorderStyle FBorderStyle;
	bool FAutoSelect;
	bool FHideSelection;
	Classes::TAlignment FAlignment;
	bool FReadOnly;
	bool FWantTabs;
	WideString FText;
	Elstrutils::TElFChar FPasswordChar;
	int FMaxLength;
	int FSelStart;
	int FSelLength;
	bool FTransparent;
	WideString FTabString;
	int FTabSpaces;
	bool FHasCaret;
	#pragma pack(push, 1)
	Types::TPoint FCaretPos;
	#pragma pack(pop)
	
	#pragma pack(push, 1)
	Types::TPoint FUndoSel;
	#pragma pack(pop)
	
	WideString FUndoText;
	int FModifyCount;
	int FLineHeight;
	int FLeftChar;
	int FCharsInView;
	bool FSelecting;
	Classes::TNotifyEvent FOnChange;
	Classes::TNotifyEvent FOnSelectionChange;
	Classes::TNotifyEvent FOnResize;
	bool ForceLeftAlignment;
	Graphics::TBitmap* FBackground;
	bool FUseBackground;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elvclutils::TElBorderSides FBorderSides;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	bool FFlat;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FHandleDialogKeys;
	bool FMouseOver;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	TElEditStrings* FElEditList;
	int FTopLine;
	bool FWordWrap;
	Stdctrls::TScrollStyle FScrollBars;
	int FLinesInRect;
	int FCharset;
	HKL FKeybLayout;
	Elscrollbar::TElScrollBar* scbVert;
	Elscrollbar::TElScrollBar* scbHorz;
	bool FFlatFocusedScrollBars;
	bool FUseCustomScrollBars;
	Elscrollbar::TElScrollBarStyles* FVertScrollBarStyles;
	Elscrollbar::TElScrollBarStyles* FHorzScrollBarStyles;
	bool FAlienFocus;
	bool FAlignBottom;
	WideString FKeys;
	bool FKeyDown;
	WideString FHint;
	MESSAGE void __fastcall WMCaptureChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Msg);
	MESSAGE void __fastcall WMCut(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMCopy(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMPaste(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMClear(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMSetText(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TWMSetFocus &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TWMKillFocus &Msg);
	MESSAGE void __fastcall WMEnable(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TWMScroll &Msg);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TWMScroll &Msg);
	MESSAGE void __fastcall WMInputLangChange(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMCommand(Messages::TWMCommand &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseWheel(Messages::TWMMouseWheel &Msg);
	MESSAGE void __fastcall WMImeStartComposition(Messages::TMessage &Message);
	MESSAGE void __fastcall WMImeComposition(Messages::TMessage &Message);
	void __fastcall OnHScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	void __fastcall OnVScroll(System::TObject* Sender, Elscrollbar::TElScrollCode ScrollCode, int &ScrollPos, bool &DoChange);
	void __fastcall SBChanged(System::TObject* Sender);
	void __fastcall SetReadOnly(bool newValue);
	void __fastcall SetAlignment(Classes::TAlignment newValue);
	void __fastcall SetLeftMargin(int newValue);
	void __fastcall SetRightMargin(int newValue);
	void __fastcall SetBorderStyle(Forms::TBorderStyle newValue);
	void __fastcall SetHideSelection(bool newValue);
	WideString __fastcall GetPasswordChar();
	void __fastcall SetPasswordChar(WideString newValue);
	void __fastcall SetTransparent(bool newValue);
	void __fastcall SetEditRect(const Types::TRect &newValue);
	void __fastcall SetTabSpaces(int newValue);
	void __fastcall SetModified(bool newValue);
	HIDESBASE void __fastcall SetText(WideString newValue);
	WideString __fastcall GetSelectedText();
	void __fastcall SetBackground(const Graphics::TBitmap* Value);
	void __fastcall SetUseBackground(const bool Value);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall DrawBackground(HDC DC, const Types::TRect &R);
	void __fastcall DrawFlatBorder(void);
	void __fastcall DrawParentControl(HDC DC);
	void __fastcall SetScrollBars(const Stdctrls::TScrollStyle Value);
	void __fastcall SetTopLine(const int Value);
	void __fastcall AdjustHeight(void);
	void __fastcall UpdateHeight(void);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	void __fastcall BackgroundChanged(System::TObject* Sender);
	void __fastcall SetFlatFocusedScrollBars(const bool Value);
	
protected:
	bool FMultiLine;
	bool FRTLContent;
	bool FAutoSize;
	Elunicodestrings::TElWideStrings* FLines;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	bool NotifyUserChangeOnly;
	TElEditCharCase FCharCase;
	void __fastcall SetVertScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	void __fastcall SetHorzScrollBarStyles(Elscrollbar::TElScrollBarStyles* newValue);
	void __fastcall SetUseCustomScrollBars(bool newValue);
	virtual void __fastcall SetMaxLength(int newValue);
	virtual void __fastcall SetSelStart(int newValue);
	virtual void __fastcall SetSelLength(int newValue);
	virtual void __fastcall SetSelText(const WideString newValue);
	WideString __fastcall StringToPassword(WideString AString);
	WideString __fastcall ExpandTabbedString(WideString Text);
	tagSIZE __fastcall TextSize(WideString ALine);
	void __fastcall SetScrollBarsInfo(void);
	void __fastcall MoveCaret(int CharNum);
	void __fastcall MakeCaret(void);
	void __fastcall RepaintText(const Types::TRect &Rect);
	void __fastcall DrawTabbedText(HDC DC, int X, int Y, WideString AText, tagSIZE &Size);
	WideString __fastcall ConvertBreaksFormat(WideString Text);
	int __fastcall CharsFitRight(int AWidth, WideString FText, int StartPos);
	int __fastcall CharsFitLeft(int AWidth, WideString FText, int StartPos);
	virtual void __fastcall Change(void);
	virtual void __fastcall TriggerSelectionChangeEvent(void);
	virtual void __fastcall TriggerResizeEvent(void);
	DYNAMIC void __fastcall KeyPress(char &Key);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall Paint(void);
	void __fastcall PaintText(WideString AText, Graphics::TCanvas* Canvas);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	virtual void __fastcall SetFlat(const bool Value);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual WideString __fastcall GetThemedClassName();
	int __fastcall GetLinesCount(void);
	void __fastcall SetWordWrap(bool Value);
	void __fastcall SetLeftChar(int Value);
	virtual void __fastcall SetAutoSize(bool Value);
	virtual void __fastcall CreateWnd(void);
	MESSAGE void __fastcall WMGetText(Messages::TMessage &Message);
	void __fastcall SetLines(Elunicodestrings::TElWideStrings* Value);
	Elunicodestrings::TElWideStrings* __fastcall GetLines(void);
	void __fastcall SetTopMargin(int Value);
	void __fastcall SetAlignBottom(bool Value);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	virtual void __fastcall DoMouseEnter(void);
	virtual void __fastcall DoMouseLeave(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	MESSAGE void __fastcall EMSetRect(Messages::TMessage &Message);
	MESSAGE void __fastcall EMSetRectNP(Messages::TMessage &Message);
	void __fastcall SetMultiLine(bool Value);
	virtual void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	virtual void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	void __fastcall SetCharCase(TElEditCharCase Value);
	HIDESBASE MESSAGE void __fastcall WMSetCursor(Messages::TWMSetCursor &Message);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	void __fastcall SetBottomAlign(void);
	__property bool RTLContent = {read=FRTLContent, write=FRTLContent, nodefault};
	__property Graphics::TBitmap* Background = {read=FBackground, write=SetBackground};
	__property bool UseBackground = {read=FUseBackground, write=SetUseBackground, default=0};
	__property WideString PasswordChar = {read=GetPasswordChar, write=SetPasswordChar};
	__property int MaxLength = {read=FMaxLength, write=SetMaxLength, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property Classes::TNotifyEvent OnSelectionChange = {read=FOnSelectionChange, write=FOnSelectionChange};
	__property bool ReadOnly = {read=FReadOnly, write=SetReadOnly, default=0};
	__property bool WantTabs = {read=FWantTabs, write=FWantTabs, default=0};
	__property Classes::TAlignment Alignment = {read=FAlignment, write=SetAlignment, nodefault};
	__property int LeftMargin = {read=FLeftMargin, write=SetLeftMargin, default=1};
	__property int RightMargin = {read=FRightMargin, write=SetRightMargin, default=2};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, nodefault};
	__property bool AutoSelect = {read=FAutoSelect, write=FAutoSelect, default=0};
	__property bool HideSelection = {read=FHideSelection, write=SetHideSelection, default=1};
	__property Classes::TNotifyEvent OnResize = {read=FOnResize, write=FOnResize};
	__property Types::TRect EditRect = {read=FEditRect, write=SetEditRect};
	__property int TabSpaces = {read=FTabSpaces, write=SetTabSpaces, default=4};
	__property bool Multiline = {read=FMultiLine, write=SetMultiLine, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, default=1};
	__property Stdctrls::TScrollStyle ScrollBars = {read=FScrollBars, write=SetScrollBars, default=0};
	__property int TopLine = {read=FTopLine, write=SetTopLine, default=0};
	__property int LeftChar = {read=FLeftChar, write=SetLeftChar, default=0};
	__property Classes::TNotifyEvent OnMouseEnter = {read=FOnMouseEnter, write=FOnMouseEnter};
	__property Classes::TNotifyEvent OnMouseLeave = {read=FOnMouseLeave, write=FOnMouseLeave};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property Elunicodestrings::TElWideStrings* Lines = {read=GetLines, write=SetLines};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property bool FlatFocusedScrollBars = {read=FFlatFocusedScrollBars, write=SetFlatFocusedScrollBars, default=0};
	__property TElEditCharCase CharCase = {read=FCharCase, write=SetCharCase, default=0};
	__property TabStop  = {default=1};
	
public:
	__fastcall virtual TCustomElEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElEdit(void);
	int __fastcall GetNextWord(WideString FText, int CharNum);
	int __fastcall GetPrevWord(WideString FText, int CharNum);
	Types::TPoint __fastcall PosFromChar(WideString FText, int CharNum);
	int __fastcall CharFromPos(WideString FText, const Types::TPoint &APos);
	void __fastcall LineFromChar(WideString FText, int CharNum, int &LineNum, int &ColNum);
	int __fastcall CharFromLine(int LineNum, int ColNum);
	void __fastcall SelectAll(void);
	void __fastcall CutToClipboard(void);
	void __fastcall CopyToClipboard(void);
	void __fastcall PasteFromClipboard(void);
	void __fastcall Undo(void);
	__property int SelStart = {read=FSelStart, write=SetSelStart, nodefault};
	__property int SelLength = {read=FSelLength, write=SetSelLength, nodefault};
	__property WideString SelText = {read=GetSelectedText, write=SetSelText};
	__property bool Modified = {read=FModified, write=SetModified, default=0};
	__property WideString SelectedText = {read=GetSelectedText};
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property int LinesCount = {read=GetLinesCount, nodefault};
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, default=0};
	__property WideString Text = {read=FText, write=SetText};
	__property int TopMargin = {read=FTopMargin, write=SetTopMargin, default=1};
	__property bool AlignBottom = {read=FAlignBottom, write=SetAlignBottom, default=1};
	
__published:
	__property Elscrollbar::TElScrollBarStyles* VertScrollBarStyles = {read=FVertScrollBarStyles, write=SetVertScrollBarStyles};
	__property Elscrollbar::TElScrollBarStyles* HorzScrollBarStyles = {read=FHorzScrollBarStyles, write=SetHorzScrollBarStyles};
	__property bool UseCustomScrollBars = {read=FUseCustomScrollBars, write=SetUseCustomScrollBars, nodefault};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property UseXPThemes  = {default=1};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElEdit(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElEditStrings : public Elunicodestrings::TElWideStringList 
{
	typedef Elunicodestrings::TElWideStringList inherited;
	
private:
	TCustomElEdit* FElEdit;
	Classes::TStringList* RealStrings;
	bool FInReformat;
	bool FToText;
	void __fastcall ListChange(System::TObject* Sender);
	
protected:
	int FMaxLen;
	int FIdxMaxLen;
	WideString FSaveStr;
	void __fastcall Reformat(void);
	virtual WideString __fastcall Get(int Index);
	WideString __fastcall GetCR(int Index);
	
public:
	__fastcall TElEditStrings(void);
	__fastcall virtual ~TElEditStrings(void);
	virtual void __fastcall SetTextStr(const WideString Value);
	WideString __fastcall GetReText();
	WideString __fastcall CutString(WideString &S, int Len, bool &RealStr);
	virtual int __fastcall Add(const WideString S);
	virtual void __fastcall Insert(int Index, const WideString S);
};


class DELPHICLASS TElEdit;
class PASCALIMPLEMENTATION TElEdit : public TCustomElEdit 
{
	typedef TCustomElEdit inherited;
	
__published:
	__property AutoSize  = {default=1};
	__property Alignment ;
	__property Background ;
	__property BorderSides ;
	__property CharCase  = {default=0};
	__property UseBackground  = {default=0};
	__property RTLContent ;
	__property PasswordChar ;
	__property MaxLength  = {default=0};
	__property Transparent ;
	__property FlatFocusedScrollBars  = {default=0};
	__property ReadOnly  = {default=0};
	__property WantTabs  = {default=0};
	__property LeftMargin  = {default=1};
	__property RightMargin  = {default=2};
	__property TopMargin  = {default=1};
	__property BorderStyle ;
	__property AutoSelect  = {default=0};
	__property HandleDialogKeys  = {default=0};
	__property HideSelection  = {default=1};
	__property TabSpaces  = {default=4};
	__property Lines  = {stored=false};
	__property Text ;
	__property Multiline  = {default=0};
	__property ImageForm ;
	__property ActiveBorderType  = {default=1};
	__property Flat  = {default=0};
	__property InactiveBorderType  = {default=3};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property WordWrap  = {default=1};
	__property ScrollBars  = {default=0};
	__property VertScrollBarStyles ;
	__property HorzScrollBarStyles ;
	__property UseCustomScrollBars ;
	__property OnMouseEnter ;
	__property OnMouseLeave ;
	__property OnResize ;
	__property OnChange ;
	__property OnSelectionChange ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
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
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElEdit.Create */ inline __fastcall virtual TElEdit(Classes::TComponent* AOwner) : TCustomElEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElEdit.Destroy */ inline __fastcall virtual ~TElEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElEdit(HWND ParentWindow) : TCustomElEdit(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool RepaintAll;
static const wchar_t ElFSpace = wchar_t(0x20);
static const wchar_t ElFTab = wchar_t(0x9);
static const wchar_t ElFCR = wchar_t(0xd);
static const wchar_t ElFLF = wchar_t(0xa);
static const Word ID_UNDO = 0x304;
static const Word ID_CUT = 0x300;
static const Word ID_COPY = 0x301;
static const Word ID_PASTE = 0x302;
static const Word ID_DELETE = 0x303;
extern PACKAGE void __fastcall Register(void);

}	/* namespace Eledits */
using namespace Eledits;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElEdits
