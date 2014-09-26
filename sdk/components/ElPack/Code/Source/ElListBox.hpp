// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElListBox.pas' rev: 6.00

#ifndef ElListBoxHPP
#define ElListBoxHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElHintWnd.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <RTLConsts.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Ellistbox
{
//-- type declarations -------------------------------------------------------
typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

typedef int TIntArray[536870911];

typedef int *PIntArray;

class DELPHICLASS TElListBoxStrings;
class DELPHICLASS TCustomElListBox;
class PASCALIMPLEMENTATION TCustomElListBox : public Controls::TWinControl 
{
	typedef Controls::TWinControl inherited;
	
protected:
	int *FImageIndex;
	int FImagesSize;
	char *FStates;
	int FStatesSize;
	Forms::TFormBorderStyle FBorderStyle;
	Graphics::TCanvas* FCanvas;
	int FColumns;
	bool FExtendedSelect;
	bool FIntegralHeight;
	int FItemHeight;
	bool FMultiSelect;
	bool FSorted;
	int FTabWidth;
	int FCurHintItem;
	int FLastTopIndex;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	Elunicodestrings::TElWideStrings* FListBoxStrings;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Graphics::TBitmap* FBackground;
	Elvclutils::TElBorderSides FBorderSides;
	bool FFlat;
	bool FFlatFocusedScrollBars;
	bool FHorizontalScroll;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FInvertSelection;
	Graphics::TColor FSelectedColor;
	Graphics::TFont* FSelectedFont;
	bool FShowLineHint;
	unsigned FTheme;
	bool FTransparent;
	bool FTransparentSelection;
	bool FUseBackground;
	Elimgfrm::TElImageForm* FImgForm;
	bool FMouseOver;
	Extctrls::TTimer* FHintTimer;
	Elhintwnd::TElHintWindow* FHintWnd;
	Classes::TWndMethod FHintWndProc;
	int FMaxWidth;
	bool FInVScroll;
	bool FInHScroll;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	bool FUseXPThemes;
	bool FMoving;
	bool FShowCheckBox;
	bool FAllowGrayed;
	Controls::TImageList* FImages;
	Imglist::TChangeLink* FImageChangeLink;
	int FSaveTopIndex;
	int FSaveItemIndex;
	WideString FHint;
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetBackground(const Graphics::TBitmap* Value);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	void __fastcall SetFlat(const bool Value);
	void __fastcall SetFlatFocusedScrollBars(const bool Value);
	void __fastcall SetHorizontalScroll(bool Value);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetInvertSelection(const bool Value);
	void __fastcall SetSelectedColor(const Graphics::TColor Value);
	void __fastcall SetSelectedFont(const Graphics::TFont* Value);
	void __fastcall SetTransparent(const bool Value);
	void __fastcall SetTransparentSelection(bool Value);
	void __fastcall SetUseBackground(const bool Value);
	void __fastcall BackgroundChanged(System::TObject* Sender);
	void __fastcall CancelLineHint(void);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentColorChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMParentFontChanged(Messages::TMessage &Msg);
	void __fastcall DrawBackground(HDC DC, const Types::TRect &R);
	void __fastcall DrawBackgroundEx(HDC DC, const Types::TRect &R, const Types::TRect &SubR);
	void __fastcall DrawFlatBorder(HDC DC, bool HDragging, bool VDragging);
	void __fastcall DrawParentControl(HDC DC);
	void __fastcall DrawParentControlEx(HDC DC, const Types::TRect &R);
	void __fastcall HintWndProc(Messages::TMessage &Message);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall IntMouseMove(short XPos, short YPos);
	MESSAGE void __fastcall LBGetTopIndex(Messages::TMessage &Msg);
	void __fastcall OnLineHintTimer(System::TObject* Sender);
	void __fastcall ResetHorizontalExtent(void);
	void __fastcall SelectedFontChanged(System::TObject* Sender);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMHScroll(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	MESSAGE void __fastcall WMNCMouseMove(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMVScroll(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	void __fastcall ResetHorizontalExtent1(void);
	void __fastcall SetHorizontalExtent(void);
	void __fastcall SetColumnWidth(void);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Message);
	MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	MESSAGE void __fastcall CNDrawItem(Messages::TWMDrawItem &Message);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	int __fastcall GetItemHeight(void);
	int __fastcall GetItemIndex(void);
	int __fastcall GetSelCount(void);
	bool __fastcall GetSelected(int Index);
	int __fastcall GetTopIndex(void);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall SetColumns(int Value);
	void __fastcall SetExtendedSelect(bool Value);
	void __fastcall SetIntegralHeight(bool Value);
	void __fastcall SetItemHeight(int Value);
	void __fastcall SetItemIndex(int Value);
	void __fastcall SetItems(Elunicodestrings::TElWideStrings* Value);
	void __fastcall SetMultiSelect(bool Value);
	void __fastcall SetSelected(int Index, bool Value);
	void __fastcall SetSorted(bool Value);
	void __fastcall SetTabWidth(int Value);
	void __fastcall SetTopIndex(int Value);
	virtual Graphics::TBitmap* __fastcall GetBackground(void);
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetUseXPThemes(bool Value);
	virtual Elhintwnd::TElHintWindow* __fastcall CreateHintWindow(void);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateThemeHandle(void);
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	void __fastcall DrawItem(int Index, const Types::TRect &R, Windows::TOwnerDrawState State);
	virtual void __fastcall FreeThemeHandle(void);
	virtual int __fastcall GetItemWidth(int Index);
	virtual int __fastcall GetParentCtlHeight(void);
	virtual int __fastcall GetParentCtlWidth(void);
	WideString __fastcall GetThemedClassName();
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	bool __fastcall IsThemeApplied(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual Types::TPoint __fastcall RealScreenToClient(const Types::TPoint &APoint);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	DYNAMIC int __fastcall InternalGetItemData(int Index);
	DYNAMIC void __fastcall InternalSetItemData(int Index, int AData);
	DYNAMIC int __fastcall GetItemData(int Index);
	DYNAMIC void __fastcall SetItemData(int Index, int AData);
	DYNAMIC void __fastcall ResetContent(void);
	DYNAMIC void __fastcall DeleteString(int Index);
	void __fastcall SetShowCheckBox(bool Value);
	Stdctrls::TCheckBoxState __fastcall GetState(int Index);
	void __fastcall SetState(int Index, Stdctrls::TCheckBoxState Value);
	tagSIZE __fastcall GetCheckBoxSize();
	void __fastcall SetAllowGrayed(bool Value);
	void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R);
	void __fastcall OnImageListChange(System::TObject* Sender);
	void __fastcall SetImages(Controls::TImageList* newValue);
	virtual void __fastcall AdjustItemHeight(void);
	int __fastcall GetImageIndex(int Index);
	void __fastcall SetImageIndex(int Index, int Value);
	void __fastcall SetStatesSize(int aSize);
	void __fastcall SetImagesSize(int aSize);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	void __fastcall SetHint(WideString Value);
	void __fastcall ItemsChange(System::TObject* Sender);
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property int Columns = {read=FColumns, write=SetColumns, default=0};
	__property bool ExtendedSelect = {read=FExtendedSelect, write=SetExtendedSelect, default=1};
	__property bool IntegralHeight = {read=FIntegralHeight, write=SetIntegralHeight, default=0};
	__property int ItemHeight = {read=GetItemHeight, write=SetItemHeight, nodefault};
	__property bool MultiSelect = {read=FMultiSelect, write=SetMultiSelect, default=0};
	__property ParentColor  = {default=0};
	__property bool Sorted = {read=FSorted, write=SetSorted, default=0};
	__property int TabWidth = {read=FTabWidth, write=SetTabWidth, default=0};
	__property int ItemIndex = {read=GetItemIndex, write=SetItemIndex, nodefault};
	__property Elunicodestrings::TElWideStrings* Items = {read=FListBoxStrings, write=SetItems};
	__property int SelCount = {read=GetSelCount, nodefault};
	__property int TopIndex = {read=GetTopIndex, write=SetTopIndex, nodefault};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Graphics::TBitmap* Background = {read=GetBackground, write=SetBackground};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property bool FlatFocusedScrollBars = {read=FFlatFocusedScrollBars, write=SetFlatFocusedScrollBars, default=0};
	__property bool HorizontalScroll = {read=FHorizontalScroll, write=SetHorizontalScroll, nodefault};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=3};
	__property bool InvertSelection = {read=FInvertSelection, write=SetInvertSelection, default=0};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property Graphics::TColor SelectedColor = {read=FSelectedColor, write=SetSelectedColor, default=-2147483635};
	__property Graphics::TFont* SelectedFont = {read=FSelectedFont, write=SetSelectedFont};
	__property bool ShowLineHint = {read=FShowLineHint, write=FShowLineHint, default=0};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property bool TransparentSelection = {read=FTransparentSelection, write=SetTransparentSelection, default=0};
	__property bool UseBackground = {read=FUseBackground, write=SetUseBackground, default=0};
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	__property TabStop  = {default=1};
	__property bool ShowCheckBox = {read=FShowCheckBox, write=SetShowCheckBox, default=0};
	__property bool AllowGrayed = {read=FAllowGrayed, write=SetAllowGrayed, default=1};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	
public:
	__fastcall virtual TCustomElListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElListBox(void);
	int __fastcall ItemAtPos(const Types::TPoint &Pos, bool Existing);
	Types::TRect __fastcall ItemRect(int Index);
	__property unsigned Theme = {read=FTheme, nodefault};
	__property Graphics::TCanvas* Canvas = {read=FCanvas};
	__property bool Selected[int Index] = {read=GetSelected, write=SetSelected};
	__property Stdctrls::TCheckBoxState State[int Index] = {read=GetState, write=SetState};
	__property int ImageIndex[int Index] = {read=GetImageIndex, write=SetImageIndex};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElListBox(HWND ParentWindow) : Controls::TWinControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElListBoxStrings : public Elunicodestrings::TElWideStringList 
{
	typedef Elunicodestrings::TElWideStringList inherited;
	
private:
	TCustomElListBox* ListBox;
	
protected:
	virtual WideString __fastcall Get(int Index);
	virtual int __fastcall GetCount(void);
	virtual System::TObject* __fastcall GetObject(int Index);
	virtual void __fastcall Put(int Index, const WideString S);
	virtual void __fastcall PutObject(int Index, System::TObject* AObject);
	virtual void __fastcall SetUpdateState(bool Updating);
	void __fastcall ResetBox(void);
	
public:
	virtual int __fastcall Add(const WideString S);
	virtual void __fastcall Clear(void);
	virtual void __fastcall Delete(int Index);
	virtual void __fastcall Exchange(int Index1, int Index2);
	virtual int __fastcall IndexOf(const WideString S);
	virtual void __fastcall Insert(int Index, const WideString S);
	virtual void __fastcall Move(int CurIndex, int NewIndex);
public:
	#pragma option push -w-inl
	/* TElWideStringList.Destroy */ inline __fastcall virtual ~TElListBoxStrings(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TElListBoxStrings(void) : Elunicodestrings::TElWideStringList() { }
	#pragma option pop
	
};


class DELPHICLASS TElListBox;
class PASCALIMPLEMENTATION TElListBox : public TCustomElListBox 
{
	typedef TCustomElListBox inherited;
	
__published:
	__property AllowGrayed  = {default=1};
	__property BorderStyle  = {default=1};
	__property Columns  = {default=0};
	__property ExtendedSelect  = {default=1};
	__property IntegralHeight  = {default=0};
	__property ItemHeight ;
	__property MultiSelect  = {default=0};
	__property ParentColor  = {default=0};
	__property Sorted  = {default=0};
	__property TabWidth  = {default=0};
	__property ItemIndex ;
	__property Items ;
	__property SelCount ;
	__property TopIndex ;
	__property ShowCheckBox  = {default=0};
	__property ActiveBorderType  = {default=1};
	__property Background ;
	__property BorderSides ;
	__property Flat  = {default=0};
	__property Ctl3D ;
	__property ParentCtl3D  = {default=1};
	__property Font ;
	__property FlatFocusedScrollBars  = {default=0};
	__property HorizontalScroll ;
	__property Images ;
	__property ImageForm ;
	__property InactiveBorderType  = {default=3};
	__property InvertSelection  = {default=0};
	__property LineBorderActiveColor ;
	__property LineBorderInactiveColor ;
	__property SelectedColor  = {default=-2147483635};
	__property SelectedFont ;
	__property ShowLineHint  = {default=0};
	__property Transparent  = {default=0};
	__property TransparentSelection  = {default=0};
	__property UseBackground  = {default=0};
	__property UseXPThemes  = {default=1};
	__property TabStop  = {default=1};
	__property ParentFont  = {default=1};
	__property OnClick ;
	__property OnContextPopup ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDock ;
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
	__property OnStartDrag ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Color  = {default=-2147483643};
	__property Constraints ;
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property ParentBiDiMode  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property Visible  = {default=1};
public:
	#pragma option push -w-inl
	/* TCustomElListBox.Create */ inline __fastcall virtual TElListBox(Classes::TComponent* AOwner) : TCustomElListBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElListBox.Destroy */ inline __fastcall virtual ~TElListBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElListBox(HWND ParentWindow) : TCustomElListBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Ellistbox */
using namespace Ellistbox;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElListBox
