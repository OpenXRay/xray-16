// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCheckCtl.pas' rev: 6.00

#ifndef ElCheckCtlHPP
#define ElCheckCtlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElHandPt.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcheckctl
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCheckItem;
class PASCALIMPLEMENTATION TElCheckItem : public Elbtnctl::TElButtonControl 
{
	typedef Elbtnctl::TElButtonControl inherited;
	
private:
	bool FFlat;
	Elsndmap::TElSoundMap* FSoundMap;
	AnsiString FCheckSound;
	bool FUseCustomGlyphs;
	Graphics::TBitmap* FGlyph;
	Classes::TAlignment FAlignment;
	bool FMouseInControl;
	bool FPressed;
	HWND FOldCapture;
	Imglist::TChangeLink* FChLink;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	bool FAutoSize;
	bool FIsHTML;
	Htmlrender::TElHTMLRender* FRender;
	bool FModified;
	Controls::TImageList* FImages;
	bool FUseImageList;
	Controls::TCursor FCursor;
	#pragma pack(push, 1)
	Types::TPoint FTextPos;
	#pragma pack(pop)
	
	Graphics::TColor FLinkColor;
	Menus::TPopupMenu* FLinkPopupMenu;
	Graphics::TFontStyles FLinkStyle;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	void __fastcall AdjustAutoSize(void);
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall ImagesChanged(System::TObject* Sender);
	void __fastcall setImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall setUseCustomGlyphs(bool newValue);
	void __fastcall setGlyph(Graphics::TBitmap* newValue);
	void __fastcall setAlignment(Classes::TLeftRight newValue);
	void __fastcall setSoundMap(Elsndmap::TElSoundMap* newValue);
	void __fastcall SetIsHTML(bool Value);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	MESSAGE void __fastcall CMTextChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMFocusChanged(Controls::TCMFocusChanged &Msg);
	void __fastcall SetFlat(bool newValue);
	void __fastcall SetImages(Controls::TImageList* Value);
	void __fastcall SetUseImageList(bool Value);
	void __fastcall SetLinkPopupMenu(Menus::TPopupMenu* newValue);
	
protected:
	bool FFlatAlways;
	bool FHandleDialogKeys;
	int __fastcall GetPartId(void);
	tagSIZE __fastcall GetCheckBoxSize();
	virtual void __fastcall setAutoSize(bool newValue);
	DYNAMIC void __fastcall DoEnter(void);
	DYNAMIC void __fastcall DoExit(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	virtual void __fastcall GlyphChange(System::TObject* Sender);
	virtual void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R) = 0 ;
	virtual void __fastcall DrawGlyph(Graphics::TCanvas* Canvas, const Types::TRect &DestRect, const Types::TRect &SrcRect);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall Paint(void);
	void __fastcall SetFlatAlways(bool Value);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMWindowPosChanged(Messages::TWMWindowPosMsg &Message);
	MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall WMContextMenu(Messages::TWMContextMenu &Message);
	HIDESBASE MESSAGE void __fastcall WMRButtonUp(Messages::TWMMouse &Message);
	virtual void __fastcall CreateWnd(void);
	virtual int __fastcall CalcAutoHeight(bool Multiline);
	HIDESBASE virtual void __fastcall SetCursor(Controls::TCursor newValue);
	Types::TRect __fastcall CalcTextRect();
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	virtual void __fastcall SetLinkColor(Graphics::TColor newValue);
	virtual void __fastcall SetLinkStyle(Graphics::TFontStyles newValue);
	void __fastcall DoLinkPopup(const Types::TPoint &MousePos);
	void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	virtual void __fastcall TriggerLinkClickEvent(WideString HRef);
	__property AnsiString CheckSound = {read=FCheckSound, write=FCheckSound};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=setSoundMap};
	__property Classes::TLeftRight Alignment = {read=FAlignment, write=setAlignment, default=1};
	__property bool UseCustomGlyphs = {read=FUseCustomGlyphs, write=setUseCustomGlyphs, default=0};
	__property Graphics::TBitmap* Glyph = {read=FGlyph, write=setGlyph};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=setImageForm};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, default=0};
	__property bool FlatAlways = {read=FFlatAlways, write=SetFlatAlways, default=0};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property bool UseImageList = {read=FUseImageList, write=SetUseImageList, default=0};
	
public:
	__fastcall virtual TElCheckItem(Classes::TComponent* AOwner);
	__fastcall virtual ~TElCheckItem(void);
	virtual void __fastcall Loaded(void);
	DYNAMIC void __fastcall Click(void);
	__property bool Modified = {read=FModified, write=FModified, nodefault};
	__property Controls::TCursor Cursor = {read=FCursor, write=SetCursor, nodefault};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=SetLinkColor, nodefault};
	__property Menus::TPopupMenu* LinkPopupMenu = {read=FLinkPopupMenu, write=SetLinkPopupMenu};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=SetLinkStyle, nodefault};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	
__published:
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, default=0};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCheckItem(HWND ParentWindow) : Elbtnctl::TElButtonControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElCheckBox;
class PASCALIMPLEMENTATION TElCheckBox : public TElCheckItem 
{
	typedef TElCheckItem inherited;
	
private:
	Stdctrls::TCheckBoxState FState;
	bool FAllowGrayed;
	void __fastcall SetState(Stdctrls::TCheckBoxState newValue);
	void __fastcall SetAllowGrayed(bool newValue);
	
protected:
	virtual bool __fastcall GetChecked(void);
	virtual void __fastcall SetChecked(bool newValue);
	virtual void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual void __fastcall Toggle(void);
	
__published:
	__property Alignment  = {default=1};
	__property bool AllowGrayed = {read=FAllowGrayed, write=SetAllowGrayed, default=0};
	__property AutoSize  = {default=1};
	__property UseCustomGlyphs  = {default=0};
	__property Checked  = {default=0};
	__property CheckSound ;
	__property SoundMap ;
	__property Glyph ;
	__property ImageForm ;
	__property TextDrawType  = {default=0};
	__property Transparent  = {default=0};
	__property Flat  = {default=0};
	__property FlatAlways  = {default=0};
	__property IsHTML  = {default=0};
	__property Images ;
	__property Stdctrls::TCheckBoxState State = {read=FState, write=SetState, default=0};
	__property Cursor ;
	__property LinkColor ;
	__property LinkPopupMenu ;
	__property LinkStyle ;
	__property OnImageNeeded ;
	__property OnLinkClick ;
	__property MoneyFlat  = {default=0};
	__property MoneyFlatActiveColor ;
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatDownColor ;
	__property UseImageList  = {default=0};
	__property Caption ;
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color ;
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property UseXPThemes  = {default=1};
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnStartDrag ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TElCheckItem.Create */ inline __fastcall virtual TElCheckBox(Classes::TComponent* AOwner) : TElCheckItem(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElCheckItem.Destroy */ inline __fastcall virtual ~TElCheckBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCheckBox(HWND ParentWindow) : TElCheckItem(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElRadioButton;
class PASCALIMPLEMENTATION TElRadioButton : public TElCheckItem 
{
	typedef TElCheckItem inherited;
	
private:
	bool FChecked;
	
protected:
	virtual void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R);
	virtual bool __fastcall GetChecked(void);
	virtual void __fastcall SetChecked(bool newValue);
	
__published:
	__property AutoSize  = {default=1};
	__property Checked  = {default=0};
	__property Cursor ;
	__property LinkColor ;
	__property LinkPopupMenu ;
	__property LinkStyle ;
	__property OnImageNeeded ;
	__property OnLinkClick ;
	__property UseCustomGlyphs  = {default=0};
	__property CheckSound ;
	__property SoundMap ;
	__property Alignment  = {default=1};
	__property Glyph ;
	__property ImageForm ;
	__property MoneyFlat  = {default=0};
	__property MoneyFlatActiveColor ;
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatDownColor ;
	__property Transparent  = {default=0};
	__property TextDrawType  = {default=0};
	__property Flat  = {default=0};
	__property FlatAlways  = {default=0};
	__property IsHTML  = {default=0};
	__property Images ;
	__property UseImageList  = {default=0};
	__property UseXPThemes  = {default=1};
	__property Caption ;
	__property Enabled  = {default=1};
	__property TabStop  = {default=1};
	__property TabOrder  = {default=-1};
	__property PopupMenu ;
	__property Color ;
	__property ParentColor  = {default=1};
	__property Align  = {default=0};
	__property Font ;
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DoubleBuffered ;
	__property DragKind  = {default=0};
	__property OnStartDock ;
	__property OnEndDock ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TElCheckItem.Create */ inline __fastcall virtual TElRadioButton(Classes::TComponent* AOwner) : TElCheckItem(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElCheckItem.Destroy */ inline __fastcall virtual ~TElRadioButton(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElRadioButton(HWND ParentWindow) : TElCheckItem(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcheckctl */
using namespace Elcheckctl;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCheckCtl
