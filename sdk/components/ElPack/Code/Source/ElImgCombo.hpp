// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElImgCombo.pas' rev: 6.00

#ifndef ElImgComboHPP
#define ElImgComboHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elimgcombo
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElImageNameEvent)(System::TObject* Sender, int Index, AnsiString &Text);

class DELPHICLASS TElImageComboBox;
class PASCALIMPLEMENTATION TElImageComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
private:
	Imglist::TChangeLink* FChLink;
	Controls::TImageList* FImages;
	bool FModified;
	int FDummyInt;
	int IOffs;
	bool OwnMoveFlag;
	void __fastcall ImagesChanged(System::TObject* Sender);
	void __fastcall SetImages(const Controls::TImageList* Value);
	void __fastcall Remake(void);
	int __fastcall GetImageIndex(void);
	void __fastcall SetImageIndex(const int Value);
	void __fastcall SetModified(const bool Value);
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Msg);
	HIDESBASE MESSAGE void __fastcall CNCommand(Messages::TWMCommand &Message);
	
protected:
	TElImageNameEvent FOnImageName;
	bool FManualEdit;
	AnsiString FEmptyValueText;
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWnd(void);
	DYNAMIC void __fastcall DropDown(void);
	DYNAMIC void __fastcall DblClick(void);
	DYNAMIC void __fastcall Change(void);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	virtual void __fastcall ComboWndProc(Messages::TMessage &Message, HWND ComboWnd, void * ComboProc);
	virtual void __fastcall TriggerImageNameEvent(int Index, AnsiString &Text);
	void __fastcall SetManualEdit(bool Value);
	bool __fastcall GetShowEmptyValue(void);
	void __fastcall SetShowEmptyValue(bool Value);
	HIDESBASE MESSAGE void __fastcall WMNCCalcSize(Messages::TMessage &Message);
	virtual void __fastcall EditWndProc(Messages::TMessage &Message);
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	void __fastcall UpdateEditSize(void);
	virtual void __fastcall Loaded(void);
	void __fastcall SetEmptyValueText(const AnsiString Value);
	
public:
	__fastcall virtual TElImageComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElImageComboBox(void);
	
__published:
	__property int Items = {read=FDummyInt, nodefault};
	__property int Style = {read=FDummyInt, nodefault};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property int ImageIndex = {read=GetImageIndex, write=SetImageIndex, default=-1};
	__property bool Modified = {read=FModified, write=SetModified, nodefault};
	__property bool ManualEdit = {read=FManualEdit, write=SetManualEdit, default=1};
	__property bool ShowEmptyValue = {read=GetShowEmptyValue, write=SetShowEmptyValue, default=1};
	__property AnsiString EmptyValueText = {read=FEmptyValueText, write=SetEmptyValueText};
	__property TElImageNameEvent OnImageName = {read=FOnImageName, write=FOnImageName};
	__property Anchors  = {default=3};
	__property BiDiMode ;
	__property Constraints ;
	__property ParentBiDiMode  = {default=1};
	__property Color  = {default=-2147483643};
	__property Ctl3D ;
	__property DropDownCount  = {default=8};
	__property Enabled  = {default=1};
	__property Font ;
	__property ImeMode  = {default=3};
	__property ImeName ;
	__property ParentColor  = {default=0};
	__property ParentCtl3D  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property TabStop  = {default=1};
	__property Visible  = {default=1};
	__property OnChange ;
	__property OnClick ;
	__property OnDblClick ;
	__property OnEnter ;
	__property OnExit ;
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnContextPopup ;
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElImageComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elimgcombo */
using namespace Elimgcombo;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElImgCombo
