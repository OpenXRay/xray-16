// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElImgFrm.pas' rev: 6.00

#ifndef ElImgFrmHPP
#define ElImgFrmHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElExtBkgnd.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elimgfrm
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TImgFormChangeLink;
class DELPHICLASS TCustomElImageForm;
class PASCALIMPLEMENTATION TCustomElImageForm : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Controls::TGraphicControl* FCaptionControl;
	bool FChangeRegion;
	Graphics::TBitmap* FBkImage;
	Extctrls::TImage* FImage;
	Graphics::TBitmap* FBmp;
	bool FMoveForm;
	Elhook::TElHook* FMoveHook;
	Classes::TNotifyEvent FOldImageEvent;
	HRGN FRegion;
	Elvclutils::TElBkGndType FBackgroundType;
	int FGradientSteps;
	Graphics::TColor FGradientStartColor;
	Graphics::TColor FGradientEndColor;
	bool FNoBk;
	Ellist::TElList* FCLients;
	void __fastcall AfterMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall BeforeMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall PictureChanged(System::TObject* Sender);
	void __fastcall BkImageChange(System::TObject* Sender);
	void __fastcall SetCaptionControl(const Controls::TGraphicControl* Value);
	void __fastcall SetChangeRegion(const bool Value);
	void __fastcall SetImage(const Extctrls::TImage* Value);
	void __fastcall SetBkImage(const Graphics::TBitmap* Value);
	void __fastcall SetMoveForm(const bool Value);
	void __fastcall SetGradientStartColor(Graphics::TColor newValue);
	void __fastcall SetGradientEndColor(Graphics::TColor newValue);
	void __fastcall SetGradientSteps(int newValue);
	void __fastcall SetBackgroundType(Elvclutils::TElBkGndType newValue);
	void __fastcall Change(void);
	
protected:
	Controls::TWinControl* FControl;
	Graphics::TColor FTransparentColor;
	void __fastcall CreateHook(void);
	void __fastcall CreateRegion(void);
	void __fastcall DestroyHook(void);
	void __fastcall DestroyRegion(void);
	Graphics::TColor __fastcall GetTransparentColor(void);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	void __fastcall SetControl(Controls::TWinControl* Value);
	void __fastcall SetTransparentColor(Graphics::TColor Value);
	__property Controls::TGraphicControl* CaptionControl = {read=FCaptionControl, write=SetCaptionControl};
	__property bool ChangeFormRegion = {read=FChangeRegion, write=SetChangeRegion, nodefault};
	__property bool MoveForm = {read=FMoveForm, write=SetMoveForm, default=0};
	__property Extctrls::TImage* FormImage = {read=FImage, write=SetImage};
	__property Graphics::TBitmap* Background = {read=FBkImage, write=SetBkImage};
	__property Elvclutils::TElBkGndType BackgroundType = {read=FBackgroundType, write=SetBackgroundType, nodefault};
	__property Graphics::TColor GradientStartColor = {read=FGradientStartColor, write=SetGradientStartColor, nodefault};
	__property Graphics::TColor GradientEndColor = {read=FGradientEndColor, write=SetGradientEndColor, nodefault};
	__property int GradientSteps = {read=FGradientSteps, write=SetGradientSteps, nodefault};
	
public:
	__fastcall virtual TCustomElImageForm(Classes::TComponent* AOwner);
	__fastcall virtual ~TCustomElImageForm(void);
	void __fastcall RegisterChanges(TImgFormChangeLink* Value);
	void __fastcall UnregisterChanges(TImgFormChangeLink* Value);
	void __fastcall PaintBkgnd(HDC DC, const Types::TRect &R, const Types::TPoint &Origin, bool Direct);
	Controls::TWinControl* __fastcall GetRealControl(void);
	
__published:
	__property Controls::TWinControl* Control = {read=FControl, write=SetControl};
	__property Graphics::TColor TransparentColor = {read=FTransparentColor, write=SetTransparentColor, nodefault};
};


class PASCALIMPLEMENTATION TImgFormChangeLink : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TCustomElImageForm* FSender;
	Classes::TNotifyEvent FOnChange;
	
public:
	__fastcall virtual ~TImgFormChangeLink(void);
	DYNAMIC void __fastcall Change(void);
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
	__property TCustomElImageForm* Sender = {read=FSender, write=FSender};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TImgFormChangeLink(void) : System::TObject() { }
	#pragma option pop
	
};


class DELPHICLASS TElImageForm;
class PASCALIMPLEMENTATION TElImageForm : public TCustomElImageForm 
{
	typedef TCustomElImageForm inherited;
	
__published:
	__property CaptionControl ;
	__property ChangeFormRegion ;
	__property FormImage ;
	__property MoveForm  = {default=0};
	__property Background ;
	__property BackgroundType ;
	__property GradientStartColor ;
	__property GradientEndColor ;
	__property GradientSteps ;
public:
	#pragma option push -w-inl
	/* TCustomElImageForm.Create */ inline __fastcall virtual TElImageForm(Classes::TComponent* AOwner) : TCustomElImageForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElImageForm.Destroy */ inline __fastcall virtual ~TElImageForm(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Word IFM_EFFECTIVESIZE = 0x92e;
static const Word IFM_REPAINTCHILDREN = 0x1cf5;
static const Word IFM_CANPAINTBKGND = 0x1cf7;

}	/* namespace Elimgfrm */
using namespace Elimgfrm;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElImgFrm
