// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCheckItemGrp.pas' rev: 6.00

#ifndef ElCheckItemGrpHPP
#define ElCheckItemGrpHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <ElGroupBox.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcheckitemgrp
{
//-- type declarations -------------------------------------------------------
typedef TMetaClass*TElCheckItemClass;

class DELPHICLASS TElCheckItemGroup;
class PASCALIMPLEMENTATION TElCheckItemGroup : public Elgroupbox::TCustomElGroupBox 
{
	typedef Elgroupbox::TCustomElGroupBox inherited;
	
protected:
	Classes::TAlignment FAlignment;
	Ellist::TElList* FButtons;
	int FColumns;
	Elunicodestrings::TElWideStrings* FHints;
	Elunicodestrings::TElWideStrings* FItems;
	bool FReading;
	bool FUpdating;
	void __fastcall ArrangeButtons(void);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall CMFontChanged(Messages::TMessage &Message);
	void __fastcall HintsChange(System::TObject* Sender);
	virtual void __fastcall ItemsChange(System::TObject* Sender);
	HIDESBASE void __fastcall SetAlignment(Classes::TLeftRight newValue);
	void __fastcall SetButtonCount(int Value);
	void __fastcall SetColumns(int Value);
	virtual void __fastcall SetFlat(bool newValue);
	void __fastcall SetHints(Elunicodestrings::TElWideStrings* Value);
	void __fastcall SetItems(Elunicodestrings::TElWideStrings* Value);
	virtual void __fastcall UpdateButtons(void);
	HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Message);
	DYNAMIC void __fastcall GetChildren(Classes::TGetChildProc Proc, Classes::TComponent* Root);
	virtual void __fastcall ReadState(Classes::TReader* Reader);
	virtual void __fastcall SetImageForm(Elimgfrm::TElImageForm* Value);
	virtual void __fastcall SetIsHTML(bool Value);
	virtual void __fastcall SetTransparent(bool newValue);
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual void __fastcall IntCreateItem(void) = 0 ;
	virtual void __fastcall ButtonClick(System::TObject* Sender);
	virtual void __fastcall SetCheckSound(AnsiString Value);
	virtual void __fastcall SetGlyph(Graphics::TBitmap* Value);
	virtual void __fastcall SetImages(Controls::TImageList* Value);
	virtual void __fastcall SetSoundMap(Elsndmap::TElSoundMap* Value);
	virtual void __fastcall SetUseCustomGlyphs(bool Value);
	virtual void __fastcall SetUseImageList(bool Value);
	virtual void __fastcall SetCheckboxChecked(bool Value);
	HIDESBASE void __fastcall SetFlatAlways(bool Value);
	virtual void __fastcall SetMoneyFlatInactiveColor(Graphics::TColor Value);
	virtual void __fastcall SetMoneyFlatActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetMoneyFlatDownColor(Graphics::TColor Value);
	bool __fastcall GetItemEnabled(int Index);
	void __fastcall SetItemEnabled(int Index, bool Value);
	__property Classes::TLeftRight Alignment = {read=FAlignment, write=SetAlignment, default=1};
	__property int Columns = {read=FColumns, write=SetColumns, default=1};
	__property Elunicodestrings::TElWideStrings* Hints = {read=FHints, write=SetHints};
	__property Elunicodestrings::TElWideStrings* Items = {read=FItems, write=SetItems};
	__property bool ItemEnabled[int Index] = {read=GetItemEnabled, write=SetItemEnabled};
	
public:
	__fastcall virtual TElCheckItemGroup(Classes::TComponent* AOwner);
	__fastcall virtual ~TElCheckItemGroup(void);
	DYNAMIC void __fastcall FlipChildren(bool AllLevels);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCheckItemGroup(HWND ParentWindow) : Elgroupbox::TCustomElGroupBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TCustomElRadioGroup;
class PASCALIMPLEMENTATION TCustomElRadioGroup : public TElCheckItemGroup 
{
	typedef TElCheckItemGroup inherited;
	
private:
	int FItemIndex;
	void __fastcall SetItemIndex(int Value);
	
protected:
	virtual void __fastcall IntCreateItem(void);
	virtual void __fastcall UpdateButtons(void);
	virtual void __fastcall ButtonClick(System::TObject* Sender);
	virtual void __fastcall ItemsChange(System::TObject* Sender);
	__property int ItemIndex = {read=FItemIndex, write=SetItemIndex, default=-1};
	
public:
	__fastcall virtual TCustomElRadioGroup(Classes::TComponent* AOwner);
public:
	#pragma option push -w-inl
	/* TElCheckItemGroup.Destroy */ inline __fastcall virtual ~TCustomElRadioGroup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElRadioGroup(HWND ParentWindow) : TElCheckItemGroup(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElRadioGroup;
class PASCALIMPLEMENTATION TElRadioGroup : public TCustomElRadioGroup 
{
	typedef TCustomElRadioGroup inherited;
	
public:
	__property ItemEnabled ;
	
__published:
	__property Align  = {default=0};
	__property Alignment  = {default=1};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property OnEndDock ;
	__property OnStartDock ;
	__property BorderSides ;
	__property Caption ;
	__property CaptionColor  = {default=536870911};
	__property CheckBoxChecked  = {default=1};
	__property Color  = {default=-2147483633};
	__property Columns  = {default=1};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat ;
	__property FlatAlways ;
	__property Font ;
	__property Hints ;
	__property ImageForm ;
	__property IsHTML  = {default=0};
	__property ItemIndex  = {default=-1};
	__property Items ;
	__property MoneyFlat  = {default=0};
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatActiveColor ;
	__property MoneyFlatDownColor ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowCheckBox  = {default=0};
	__property ShowFocus ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property CheckSound ;
	__property SoundMap ;
	__property Glyph ;
	__property Images ;
	__property UseCustomGlyphs  = {default=0};
	__property UseImageList  = {default=0};
	__property OnClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElRadioGroup.Create */ inline __fastcall virtual TElRadioGroup(Classes::TComponent* AOwner) : TCustomElRadioGroup(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TElCheckItemGroup.Destroy */ inline __fastcall virtual ~TElRadioGroup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElRadioGroup(HWND ParentWindow) : TCustomElRadioGroup(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TCustomElCheckGroup;
class PASCALIMPLEMENTATION TCustomElCheckGroup : public TElCheckItemGroup 
{
	typedef TElCheckItemGroup inherited;
	
private:
	bool __fastcall GetChecked(int Index);
	void __fastcall SetChecked(int Index, bool Value);
	Stdctrls::TCheckBoxState __fastcall GetState(int Index);
	void __fastcall SetState(int Index, Stdctrls::TCheckBoxState Value);
	
protected:
	bool FAllowGrayed;
	void __fastcall SetAllowGrayed(bool Value);
	virtual void __fastcall IntCreateItem(void);
	
public:
	__fastcall virtual TCustomElCheckGroup(Classes::TComponent* AOwner);
	__property bool Checked[int Index] = {read=GetChecked, write=SetChecked};
	__property Stdctrls::TCheckBoxState State[int Index] = {read=GetState, write=SetState};
	
__published:
	__property bool AllowGrayed = {read=FAllowGrayed, write=SetAllowGrayed, default=1};
public:
	#pragma option push -w-inl
	/* TElCheckItemGroup.Destroy */ inline __fastcall virtual ~TCustomElCheckGroup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElCheckGroup(HWND ParentWindow) : TElCheckItemGroup(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElCheckGroup;
class PASCALIMPLEMENTATION TElCheckGroup : public TCustomElCheckGroup 
{
	typedef TCustomElCheckGroup inherited;
	
public:
	__property ItemEnabled ;
	
__published:
	__property Align  = {default=0};
	__property Alignment  = {default=1};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property DragKind  = {default=0};
	__property ParentBiDiMode  = {default=1};
	__property OnEndDock ;
	__property OnStartDock ;
	__property BorderSides ;
	__property Caption ;
	__property CaptionColor  = {default=536870911};
	__property CheckBoxChecked  = {default=1};
	__property Color  = {default=-2147483633};
	__property Columns  = {default=1};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat ;
	__property FlatAlways ;
	__property Font ;
	__property Hints ;
	__property ImageForm ;
	__property IsHTML  = {default=0};
	__property Items ;
	__property MoneyFlat  = {default=0};
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatActiveColor ;
	__property MoneyFlatDownColor ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowCheckBox  = {default=0};
	__property ShowFocus ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property Visible  = {default=1};
	__property UseXPThemes  = {default=1};
	__property CheckSound ;
	__property SoundMap ;
	__property Glyph ;
	__property Images ;
	__property UseCustomGlyphs  = {default=0};
	__property UseImageList  = {default=0};
	__property OnClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnStartDrag ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElCheckGroup.Create */ inline __fastcall virtual TElCheckGroup(Classes::TComponent* AOwner) : TCustomElCheckGroup(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TElCheckItemGroup.Destroy */ inline __fastcall virtual ~TElCheckGroup(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElCheckGroup(HWND ParentWindow) : TCustomElCheckGroup(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcheckitemgrp */
using namespace Elcheckitemgrp;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCheckItemGrp
