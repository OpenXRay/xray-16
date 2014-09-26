// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElGroupBox.pas' rev: 6.00

#ifndef ElGroupBoxHPP
#define ElGroupBoxHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <ElSndMap.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
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

namespace Elgroupbox
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomElGroupBox;
class PASCALIMPLEMENTATION TCustomElGroupBox : public Elpanel::TCustomElPanel 
{
	typedef Elpanel::TCustomElPanel inherited;
	
protected:
	bool FReading;
	Htmlrender::TElHTMLRender* FRender;
	bool FIsHTML;
	Elvclutils::TElBorderSides FBorderSides;
	#pragma pack(push, 1)
	Types::TRect FCheckRect;
	#pragma pack(pop)
	
	AnsiString FCheckSound;
	Graphics::TBitmap* FGlyph;
	Controls::TImageList* FImages;
	Elsndmap::TElSoundMap* FSoundMap;
	bool FUseCustomGlyphs;
	bool FUseImageList;
	Imglist::TChangeLink* FChLink;
	bool FShowCheckBox;
	bool FCheckBoxChecked;
	bool FShowFocus;
	Graphics::TColor FCaptionColor;
	bool FMouseInCheckBox;
	bool FFlat;
	bool FFlatAlways;
	bool FAutoDisableChildren;
	bool FMoneyFlat;
	Graphics::TColor FMoneyFlatInactiveColor;
	Graphics::TColor FMoneyFlatActiveColor;
	Graphics::TColor FMoneyFlatDownColor;
	void __fastcall SetBorderSides(Elvclutils::TElBorderSides Value);
	void __fastcall ImagesChanged(System::TObject* Sender);
	virtual void __fastcall SetIsHTML(bool Value);
	virtual void __fastcall ReadState(Classes::TReader* Reader);
	virtual bool __fastcall CanModify(void);
	virtual void __fastcall Paint(void);
	virtual WideString __fastcall GetThemedClassName();
	void __fastcall SetShowCheckBox(bool Value);
	virtual void __fastcall SetCheckBoxChecked(bool Value);
	HIDESBASE void __fastcall SetShowFocus(bool Value);
	void __fastcall SetCaptionColor(Graphics::TColor Value);
	virtual void __fastcall SetCheckSound(AnsiString Value);
	virtual void __fastcall SetGlyph(Graphics::TBitmap* Value);
	virtual void __fastcall SetSoundMap(Elsndmap::TElSoundMap* Value);
	virtual void __fastcall SetUseCustomGlyphs(bool Value);
	virtual void __fastcall SetImages(Controls::TImageList* Value);
	virtual void __fastcall SetUseImageList(bool Value);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	virtual void __fastcall GlyphChange(System::TObject* Sender);
	tagSIZE __fastcall GetCheckBoxSize();
	virtual void __fastcall DrawGlyph(Graphics::TCanvas* Canvas, const Types::TRect &DestRect, const Types::TRect &SrcRect);
	virtual void __fastcall SetFlat(bool newValue);
	void __fastcall DrawFlatFrame(Graphics::TCanvas* Canvas, const Types::TRect &R);
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseUp(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState Shift);
	virtual void __fastcall AdjustClientRect(Types::TRect &Rect);
	int __fastcall GetTopOffset(void);
	HIDESBASE MESSAGE void __fastcall WMEraseBkGnd(Messages::TWMEraseBkgnd &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall CMEnter(Messages::TWMNoParams &Msg);
	HIDESBASE MESSAGE void __fastcall CMExit(Messages::TWMNoParams &Msg);
	void __fastcall SetFlatAlways(bool Value);
	Types::TRect __fastcall GetCaptionRect();
	Types::TRect __fastcall GetCheckRect();
	int __fastcall GetLineTopOffset(void);
	HIDESBASE MESSAGE void __fastcall CMEnabledChanged(Messages::TMessage &Message);
	void __fastcall SetAutoDisableChildren(bool Value);
	void __fastcall SetMoneyFlat(bool Value);
	virtual void __fastcall SetMoneyFlatInactiveColor(Graphics::TColor Value);
	virtual void __fastcall SetMoneyFlatActiveColor(Graphics::TColor Value);
	virtual void __fastcall SetMoneyFlatDownColor(Graphics::TColor Value);
	bool __fastcall GetMoneyFlat(void);
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, default=0};
	__property Elvclutils::TElBorderSides BorderSides = {read=FBorderSides, write=SetBorderSides, nodefault};
	__property bool ShowCheckBox = {read=FShowCheckBox, write=SetShowCheckBox, default=0};
	__property bool CheckBoxChecked = {read=FCheckBoxChecked, write=SetCheckBoxChecked, default=1};
	__property bool ShowFocus = {read=FShowFocus, write=SetShowFocus, nodefault};
	__property Graphics::TColor CaptionColor = {read=FCaptionColor, write=SetCaptionColor, default=536870911};
	__property AnsiString CheckSound = {read=FCheckSound, write=SetCheckSound};
	__property Graphics::TBitmap* Glyph = {read=FGlyph, write=SetGlyph};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property Elsndmap::TElSoundMap* SoundMap = {read=FSoundMap, write=SetSoundMap};
	__property bool UseCustomGlyphs = {read=FUseCustomGlyphs, write=SetUseCustomGlyphs, default=0};
	__property bool UseImageList = {read=FUseImageList, write=SetUseImageList, default=0};
	__property bool Flat = {read=FFlat, write=SetFlat, nodefault};
	__property bool FlatAlways = {read=FFlatAlways, write=SetFlatAlways, nodefault};
	__property bool AutoDisableChildren = {read=FAutoDisableChildren, write=SetAutoDisableChildren, nodefault};
	__property bool MoneyFlat = {read=GetMoneyFlat, write=SetMoneyFlat, default=0};
	__property Graphics::TColor MoneyFlatInactiveColor = {read=FMoneyFlatInactiveColor, write=SetMoneyFlatInactiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatActiveColor = {read=FMoneyFlatActiveColor, write=SetMoneyFlatActiveColor, stored=GetMoneyFlat, nodefault};
	__property Graphics::TColor MoneyFlatDownColor = {read=FMoneyFlatDownColor, write=SetMoneyFlatDownColor, stored=GetMoneyFlat, nodefault};
	
public:
	__fastcall virtual TCustomElGroupBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElGroupBox(void);
	DYNAMIC void __fastcall FlipChildren(bool AllLevels);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomElGroupBox(HWND ParentWindow) : Elpanel::TCustomElPanel(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElGroupBox;
class PASCALIMPLEMENTATION TElGroupBox : public TCustomElGroupBox 
{
	typedef TCustomElGroupBox inherited;
	
__published:
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property AutoDisableChildren ;
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
	__property CheckSound ;
	__property Color  = {default=-2147483633};
	__property Ctl3D ;
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Flat ;
	__property FlatAlways ;
	__property Font ;
	__property Glyph ;
	__property ImageForm ;
	__property Images ;
	__property MoneyFlat  = {default=0};
	__property MoneyFlatInactiveColor ;
	__property MoneyFlatActiveColor ;
	__property MoneyFlatDownColor ;
	__property IsHTML  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentColor  = {default=0};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowCheckBox  = {default=0};
	__property ShowFocus ;
	__property ShowHint ;
	__property SoundMap ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Transparent  = {default=0};
	__property UseCustomGlyphs  = {default=0};
	__property UseImageList  = {default=0};
	__property UseXPThemes  = {default=1};
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnEnter ;
	__property OnExit ;
	__property OnMouseDown ;
	__property OnMouseUp ;
	__property OnMouseMove ;
	__property OnStartDrag ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TCustomElGroupBox.Create */ inline __fastcall virtual TElGroupBox(Classes::TComponent* AOwner) : TCustomElGroupBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElGroupBox.Destroy */ inline __fastcall virtual ~TElGroupBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElGroupBox(HWND ParentWindow) : TCustomElGroupBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elgroupbox */
using namespace Elgroupbox;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElGroupBox
