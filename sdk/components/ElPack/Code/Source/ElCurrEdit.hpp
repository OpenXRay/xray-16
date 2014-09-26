// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCurrEdit.pas' rev: 6.00

#ifndef ElCurrEditHPP
#define ElCurrEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Math.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcurredit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCurrPartEdit;
class PASCALIMPLEMENTATION TElCurrPartEdit : public Eledits::TElEdit 
{
	typedef Eledits::TElEdit inherited;
	
private:
	bool FIsIntegerPart;
	Classes::TNotifyEvent OnPoint;
	Classes::TNotifyEvent OnLeftPoint;
	
protected:
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyDown(Messages::TWMKey &Message);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
public:
	#pragma option push -w-inl
	/* TCustomElEdit.Create */ inline __fastcall virtual TElCurrPartEdit(Classes::TComponent* AOwner) : Eledits::TElEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElEdit.Destroy */ inline __fastcall virtual ~TElCurrPartEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCurrPartEdit(HWND ParentWindow) : Eledits::TElEdit(ParentWindow) { }
	#pragma option pop
	
};


typedef TElCurrPartEdit IntEditClass;
;

#pragma option push -b-
enum TElCurrencySymbolPosition { ecsPosLeft, ecsPosRight };
#pragma option pop

class DELPHICLASS TElCurrencyEdit;
class PASCALIMPLEMENTATION TElCurrencyEdit : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
protected:
	System::Currency FAbsValue;
	Byte FDecimalPlaces;
	bool FUseSystemDecimalPlaces;
	WideString FCurrencySymbol;
	TElCurrencySymbolPosition FCurrencySymbolPosition;
	bool FUseSystemCurrencySymbol;
	Forms::TFormBorderStyle FBorderStyle;
	bool FFlat;
	Elvclutils::TElFlatBorderType FActiveBorderType;
	Elvclutils::TElFlatBorderType FInactiveBorderType;
	bool FMouseOver;
	bool FModified;
	int FDWidth;
	int FSWidth;
	int FBWidth;
	int FSignWidth;
	int FDSWidth;
	Classes::TNotifyEvent FOnChange;
	Classes::TNotifyEvent FOnMouseEnter;
	Classes::TNotifyEvent FOnMouseLeave;
	Elvclutils::TElBorderSides FBorderSides;
	TElCurrPartEdit* FPartEditors[2];
	Elpopbtn::TElGraphicButton* FButtons[1];
	Classes::TNotifyEvent FButtonClicks[1];
	bool FEnableSign;
	bool FSign;
	Graphics::TColor FNegativeValueTextColor;
	Graphics::TColor FNegativeSignColor;
	bool FHandleDialogKeys;
	Graphics::TColor FLineBorderActiveColor;
	Graphics::TColor FLineBorderInactiveColor;
	WideString FHint;
	AnsiString __fastcall FracValue(System::Currency AValue);
	void __fastcall SetDecimalPlaces(const Byte Value);
	void __fastcall SetUseSystemDecimalPlaces(const bool Value);
	int __fastcall FindPart(TElCurrPartEdit* Editor);
	void __fastcall SetFlat(const bool Value);
	void __fastcall SetActiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetInactiveBorderType(const Elvclutils::TElFlatBorderType Value);
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	void __fastcall UpdateFrame(void);
	void __fastcall SetModified(bool Value);
	void __fastcall UpdateParts(void);
	void __fastcall UpdatePartsWidth(void);
	void __fastcall SetCurrencySymbol(const WideString Value);
	void __fastcall SetUseSystemCurrencySymbol(const bool Value);
	void __fastcall SetCurrencySymbolPosition(const TElCurrencySymbolPosition Value);
	WideString __fastcall GetButtonCaption(int Index);
	Graphics::TColor __fastcall GetButtonColor(int Index);
	bool __fastcall GetButtonDown(int Index);
	bool __fastcall GetButtonEnabled(int Index);
	bool __fastcall GetButtonFlat(int Index);
	bool __fastcall GetButtonVisible(int Index);
	int __fastcall GetButtonWidth(int Index);
	bool __fastcall GetButtonUseIcon(int Index);
	Graphics::TBitmap* __fastcall GetButtonGlyph(int Index);
	AnsiString __fastcall GetButtonHint(int Index);
	Graphics::TIcon* __fastcall GetButtonIcon(int Index);
	int __fastcall GetButtonNumGlyphs(int Index);
	Elpopbtn::TPopupPlace __fastcall GetButtonPopupPlace(int Index);
	Menus::TPopupMenu* __fastcall GetButtonPullDownMenu(int Index);
	bool __fastcall GetButtonUseImageList(int Index);
	Classes::TNotifyEvent __fastcall GetOnButtonClick(int Index);
	void __fastcall SetButtonCaption(int Index, WideString Value);
	void __fastcall SetButtonColor(const int Index, const Graphics::TColor Value);
	void __fastcall SetButtonDown(const int Index, const bool Value);
	void __fastcall SetButtonEnabled(const int Index, const bool Value);
	void __fastcall SetButtonFlat(const int Index, const bool Value);
	void __fastcall SetButtonVisible(const int Index, const bool Value);
	void __fastcall SetButtonWidth(const int Index, const int Value);
	void __fastcall SetButtonUseIcon(const int Index, const bool Value);
	void __fastcall SetButtonGlyph(const int Index, const Graphics::TBitmap* Value);
	void __fastcall SetButtonHint(const int Index, const AnsiString Value);
	void __fastcall SetButtonIcon(const int Index, const Graphics::TIcon* Value);
	void __fastcall SetButtonNumGlyphs(const int Index, const int Value);
	void __fastcall SetButtonPopupPlace(const int Index, const Elpopbtn::TPopupPlace Value);
	void __fastcall SetButtonPullDownMenu(const int Index, const Menus::TPopupMenu* Value);
	void __fastcall SetButtonUseImageList(const int Index, const bool Value);
	void __fastcall SetOnButtonClick(const int Index, const Classes::TNotifyEvent Value);
	Controls::TImageList* __fastcall GetButtonDisabledImages(int Index);
	Controls::TImageList* __fastcall GetButtonDownImages(int Index);
	Controls::TImageList* __fastcall GetButtonHotImages(int Index);
	Controls::TImageList* __fastcall GetButtonImageList(int Index);
	Imglist::TImageIndex __fastcall GetButtonImageIndex(int Index);
	void __fastcall SetButtonDisabledImages(const int Index, const Controls::TImageList* Value);
	void __fastcall SetButtonDownImages(const int Index, const Controls::TImageList* Value);
	void __fastcall SetButtonHotImages(const int Index, const Controls::TImageList* Value);
	void __fastcall SetButtonImageList(const int Index, const Controls::TImageList* Value);
	void __fastcall SetButtonImageIndex(const int Index, const Imglist::TImageIndex Value);
	void __fastcall ButtonClickTransfer(System::TObject* Sender);
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TWMNCCalcSize &Message);
	void __fastcall SetEnableSign(const bool Value);
	System::Currency __fastcall GetSignValue(void);
	void __fastcall SetSign(const bool Value);
	void __fastcall SetSignValue(const System::Currency Value);
	void __fastcall SetAutoSelect(const bool Value);
	bool __fastcall GetAutoSelect(void);
	void __fastcall SetNegativeSignColor(const Graphics::TColor Value);
	void __fastcall SetNegativeValueTextColor(const Graphics::TColor Value);
	void __fastcall SetupPartsFont(void);
	bool __fastcall GetReadOnly(void);
	void __fastcall SetReadOnly(const bool Value);
	void __fastcall OnEditorClick(System::TObject* Sender);
	void __fastcall OnEditorDblClick(System::TObject* Sender);
	void __fastcall OnEditorEndDrag(System::TObject* Sender, System::TObject* Target, int X, int Y);
	void __fastcall OnEditorKeyUp(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	void __fastcall OnEditorMouseDown(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall OnEditorMouseMove(System::TObject* Sender, Classes::TShiftState Shift, int X, int Y);
	void __fastcall OnEditorMouseUp(System::TObject* Sender, Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	void __fastcall OnEditorStartDrag(System::TObject* Sender, Controls::TDragObject* &DragObject);
	void __fastcall OnEditorKeyPress(System::TObject* Sender, char &Key);
	void __fastcall OnEditorDragDrop(System::TObject* Sender, System::TObject* Source, int X, int Y);
	void __fastcall OnEditorDragOver(System::TObject* Sender, System::TObject* Source, int X, int Y, Controls::TDragState State, bool &Accept);
	bool __fastcall StoreCurrencySymbol(void);
	bool __fastcall StoreDecimalPlaces(void);
	void __fastcall SetAbsValue(const System::Currency Value);
	void __fastcall DrawFlatBorder(HDC DC);
	virtual void __fastcall AdjustEditorPositions(void);
	HIDESBASE MESSAGE void __fastcall CMCtl3DChanged(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMColorChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMSysColorChange(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMMouseEnter(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSetFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMKillFocus(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMWinIniChange(Messages::TMessage &Msg);
	DYNAMIC void __fastcall DoOnChange(void);
	DYNAMIC void __fastcall DoMouseEnter(void);
	DYNAMIC void __fastcall DoMouseLeave(void);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	void __fastcall OnEditorChange(System::TObject* Sender);
	void __fastcall OnEditorEnter(System::TObject* Sender);
	void __fastcall OnEditorExit(System::TObject* Sender);
	void __fastcall OnEditorPoint(System::TObject* Sender);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	void __fastcall OnEditorLeftPoint(System::TObject* Sender);
	void __fastcall OnEditorKeyDown(System::TObject* Sender, Word &Key, Classes::TShiftState Shift);
	virtual WideString __fastcall GetThemedClassName();
	void __fastcall SetLineBorderActiveColor(Graphics::TColor Value);
	void __fastcall SetLineBorderInactiveColor(Graphics::TColor Value);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElCurrencyEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElCurrencyEdit(void);
	virtual void __fastcall Paint(void);
	virtual void __fastcall Loaded(void);
	__property bool MouseOver = {read=FMouseOver, nodefault};
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	__property System::Currency AbsoluteValue = {read=FAbsValue, write=SetAbsValue};
	
__published:
	__property WideString Hint = {read=FHint, write=SetHint};
	__property bool ReadOnly = {read=GetReadOnly, write=SetReadOnly, default=0};
	__property bool AutoSelect = {read=GetAutoSelect, write=SetAutoSelect, default=1};
	__property System::Currency Value = {read=GetSignValue, write=SetSignValue};
	__property bool Sign = {read=FSign, write=SetSign, stored=false, nodefault};
	__property bool EnableSign = {read=FEnableSign, write=SetEnableSign, default=0};
	__property Graphics::TColor NegativeSignColor = {read=FNegativeSignColor, write=SetNegativeSignColor, default=536870911};
	__property Graphics::TColor NegativeValueTextColor = {read=FNegativeValueTextColor, write=SetNegativeValueTextColor, default=536870911};
	__property Byte DecimalPlaces = {read=FDecimalPlaces, write=SetDecimalPlaces, stored=StoreDecimalPlaces, nodefault};
	__property bool UseSystemDecimalPlaces = {read=FUseSystemDecimalPlaces, write=SetUseSystemDecimalPlaces, default=1};
	__property WideString CurrencySymbol = {read=FCurrencySymbol, write=SetCurrencySymbol, stored=StoreCurrencySymbol};
	__property TElCurrencySymbolPosition CurrencySymbolPosition = {read=FCurrencySymbolPosition, write=SetCurrencySymbolPosition, stored=StoreCurrencySymbol, nodefault};
	__property bool UseSystemCurrencySymbol = {read=FUseSystemCurrencySymbol, write=SetUseSystemCurrencySymbol, default=0};
	__property WideString ButtonCaption = {read=GetButtonCaption, write=SetButtonCaption, index=0};
	__property Graphics::TColor ButtonColor = {read=GetButtonColor, write=SetButtonColor, index=0, default=-2147483633};
	__property bool ButtonDown = {read=GetButtonDown, write=SetButtonDown, index=0, default=0};
	__property bool ButtonEnabled = {read=GetButtonEnabled, write=SetButtonEnabled, index=0, default=1};
	__property bool ButtonFlat = {read=GetButtonFlat, write=SetButtonFlat, index=0, default=0};
	__property bool ButtonUseIcon = {read=GetButtonUseIcon, write=SetButtonUseIcon, index=0, default=0};
	__property bool ButtonVisible = {read=GetButtonVisible, write=SetButtonVisible, index=0, default=0};
	__property int ButtonWidth = {read=GetButtonWidth, write=SetButtonWidth, index=0, default=15};
	__property Graphics::TBitmap* ButtonGlyph = {read=GetButtonGlyph, write=SetButtonGlyph, index=0};
	__property AnsiString ButtonHint = {read=GetButtonHint, write=SetButtonHint, index=0};
	__property Graphics::TIcon* ButtonIcon = {read=GetButtonIcon, write=SetButtonIcon, index=0};
	__property int ButtonNumGlyphs = {read=GetButtonNumGlyphs, write=SetButtonNumGlyphs, index=0, default=1};
	__property Elpopbtn::TPopupPlace ButtonPopupPlace = {read=GetButtonPopupPlace, write=SetButtonPopupPlace, index=0, nodefault};
	__property Menus::TPopupMenu* ButtonPullDownMenu = {read=GetButtonPullDownMenu, write=SetButtonPullDownMenu, index=0};
	__property bool ButtonUseImageList = {read=GetButtonUseImageList, write=SetButtonUseImageList, index=0, default=0};
	__property Controls::TImageList* ButtonImages = {read=GetButtonImageList, write=SetButtonImageList, index=0};
	__property Controls::TImageList* ButtonDownImages = {read=GetButtonDownImages, write=SetButtonDownImages, index=0};
	__property Controls::TImageList* ButtonHotImages = {read=GetButtonHotImages, write=SetButtonHotImages, index=0};
	__property Controls::TImageList* ButtonDisabledImages = {read=GetButtonDisabledImages, write=SetButtonDisabledImages, index=0};
	__property Imglist::TImageIndex ButtonImageIndex = {read=GetButtonImageIndex, write=SetButtonImageIndex, index=0, nodefault};
	__property Classes::TNotifyEvent OnButtonClick = {read=GetOnButtonClick, write=SetOnButtonClick, index=0};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	__property bool Flat = {read=FFlat, write=SetFlat, default=0};
	__property Elvclutils::TElFlatBorderType ActiveBorderType = {read=FActiveBorderType, write=SetActiveBorderType, default=1};
	__property Elvclutils::TElFlatBorderType InactiveBorderType = {read=FInactiveBorderType, write=SetInactiveBorderType, default=0};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool HandleDialogKeys = {read=FHandleDialogKeys, write=FHandleDialogKeys, nodefault};
	__property Graphics::TColor LineBorderActiveColor = {read=FLineBorderActiveColor, write=SetLineBorderActiveColor, nodefault};
	__property Graphics::TColor LineBorderInactiveColor = {read=FLineBorderInactiveColor, write=SetLineBorderInactiveColor, nodefault};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
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
	__property TabStop  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
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
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCurrencyEdit(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint CURRBTNCOUNT = 0x1;
static const Shortint CURRBTNMIN = 0x0;
static const Shortint CURRBTNMAX = 0x0;

}	/* namespace Elcurredit */
using namespace Elcurredit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCurrEdit
