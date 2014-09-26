// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElPromptDlg.pas' rev: 6.00

#ifndef ElPromptDlgHPP
#define ElPromptDlgHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElCaption.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Consts.hpp>	// Pascal unit
#include <ElPanel.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElFrmPers.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElHTMLLbl.hpp>	// Pascal unit
#include <ElImgLst.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ElStrArray.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElStrPool.hpp>	// Pascal unit
#include <ElPopBtn.hpp>	// Pascal unit
#include <ElCheckCtl.hpp>	// Pascal unit
#include <ElBtnCtl.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elpromptdlg
{
//-- type declarations -------------------------------------------------------
typedef TMetaClass*TElPromptFormClass;

typedef void __fastcall (__closure *TPromptCloseEvent)(System::TObject* Sender, int Result);

class DELPHICLASS TElPromptForm;
class PASCALIMPLEMENTATION TElPromptForm : public Forms::TForm 
{
	typedef Forms::TForm inherited;
	
__published:
	Extctrls::TTimer* Timer;
	Elpanel::TElPanel* ElPanel1;
	Stdctrls::TLabel* TimeLabel;
	Extctrls::TImage* Image;
	Elhtmllbl::TElHTMLLabel* MessageLabel;
	Elcheckctl::TElCheckBox* ShowAgainCB;
	Elpopbtn::TElPopupButton* HelpBtn;
	Elpopbtn::TElPopupButton* OkBtn;
	Elpopbtn::TElPopupButton* IgnoreBtn;
	Elpopbtn::TElPopupButton* YesBtn;
	Elpopbtn::TElPopupButton* CancelBtn;
	Elpopbtn::TElPopupButton* NoBtn;
	Elpopbtn::TElPopupButton* NoToAllBtn;
	Elpopbtn::TElPopupButton* AbortBtn;
	Elpopbtn::TElPopupButton* RetryBtn;
	Elpopbtn::TElPopupButton* YesToAllBtn;
	Elfrmpers::TElFormPersist* ElFormPersist1;
	Elimglst::TElImageList* DisabledImages;
	Elimglst::TElImageList* EnabledImages;
	Elcaption::TElFormCaption* Captions;
	void __fastcall TimerTimer(System::TObject* Sender);
	void __fastcall HelpBtnClick(System::TObject* Sender);
	void __fastcall FormShow(System::TObject* Sender);
	void __fastcall FormClose(System::TObject* Sender, Forms::TCloseAction &Action);
	void __fastcall BtnClick(System::TObject* Sender);
	void __fastcall MessageLabelLinkClick(System::TObject* Sender, WideString HRef);
	void __fastcall MessageLabelImageNeeded(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	
private:
	bool FDisableOk;
	int FLeft;
	bool FShowTime;
	WideString FSaveDefText;
	WideString SecondsCaption;
	Elpopbtn::TElPopupButton* DefaultButton;
	bool Modal;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeed;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	Classes::TNotifyEvent FOnTimer;
	TPromptCloseEvent FOnClose;
	HIDESBASE MESSAGE void __fastcall WMSysCommand(Messages::TMessage &Message);
	
public:
	void *CustomData;
public:
	#pragma option push -w-inl
	/* TCustomForm.Create */ inline __fastcall virtual TElPromptForm(Classes::TComponent* AOwner) : Forms::TForm(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.CreateNew */ inline __fastcall virtual TElPromptForm(Classes::TComponent* AOwner, int Dummy) : Forms::TForm(AOwner, Dummy) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomForm.Destroy */ inline __fastcall virtual ~TElPromptForm(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElPromptForm(HWND ParentWindow) : Forms::TForm(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElPromptDialog;
class PASCALIMPLEMENTATION TElPromptDialog : public Dialogs::TCommonDialog 
{
	typedef Dialogs::TCommonDialog inherited;
	
protected:
	bool FTopmost;
	Elunicodestrings::TElWideStringArray* FControlTexts;
	WideString FMessage;
	int FCaptionIdx;
	int FMessageIdx;
	Dialogs::TMsgDlgType FDlgType;
	Elunicodestrings::TElWideStringArray* FCaptions;
	Elunicodestrings::TElWideStringArray* FTexts;
	Dialogs::TMsgDlgButtons FButtons;
	Dialogs::TMsgDlgBtn FDefBtn;
	Dialogs::TMsgDlgBtn FCancelBtn;
	bool FShowGlyphs;
	int FTimeLimit;
	bool FShowOnceMore;
	bool FShowAgainChecked;
	bool FTimedShow;
	int FHelpCtx;
	WideString FShowAgainText;
	WideString FDlgCaption;
	bool FIsHTML;
	Classes::TNotifyEvent FOnBeforeShow;
	Classes::TNotifyEvent FOnTimer;
	TPromptCloseEvent FOnClose;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLLinkClickEvent FOnLinkClick;
	TMetaClass*FClass;
	bool FParentFont;
	Graphics::TFont* FFont;
	TElPromptForm* FForm;
	void __fastcall SetTexts(Elunicodestrings::TElWideStringArray* anArray);
	void __fastcall SetCaptions(Elunicodestrings::TElWideStringArray* anArray);
	void __fastcall SetControlTexts(Elunicodestrings::TElWideStringArray* newValue);
	TElPromptForm* __fastcall CreateWndx(void);
	void __fastcall SetParentFont(bool Value);
	void __fastcall SetFont(Graphics::TFont* Value);
	void __fastcall CloseTransfer(System::TObject* Sender, int Result);
	void __fastcall FontChange(System::TObject* Sender);
	
public:
	void *CustomData;
	__fastcall virtual TElPromptDialog(Classes::TComponent* AOwner);
	__fastcall virtual ~TElPromptDialog(void);
	int __fastcall ShowModal(void);
	void __fastcall Show(void);
	virtual bool __fastcall Execute(void);
	__property TMetaClass* FormClass = {read=FClass, write=FClass};
	
__published:
	__property bool DisableDefault = {read=FTimedShow, write=FTimedShow, nodefault};
	__property Dialogs::TMsgDlgBtn DefaultButton = {read=FDefBtn, write=FDefBtn, nodefault};
	__property Dialogs::TMsgDlgBtn CancelButton = {read=FCancelBtn, write=FCancelBtn, nodefault};
	__property int TimeDelay = {read=FTimeLimit, write=FTimeLimit, nodefault};
	__property bool ShowGlyphs = {read=FShowGlyphs, write=FShowGlyphs, nodefault};
	__property Elunicodestrings::TElWideStringArray* Texts = {read=FTexts, write=SetTexts};
	__property Elunicodestrings::TElWideStringArray* ControlTexts = {read=FControlTexts, write=SetControlTexts};
	__property WideString DialogCaption = {read=FDlgCaption, write=FDlgCaption};
	__property WideString Message = {read=FMessage, write=FMessage};
	__property int MessageIdx = {read=FMessageIdx, write=FMessageIdx, nodefault};
	__property Dialogs::TMsgDlgType DlgType = {read=FDlgType, write=FDlgType, nodefault};
	__property Dialogs::TMsgDlgButtons Buttons = {read=FButtons, write=FButtons, nodefault};
	__property bool ShowAgainCheck = {read=FShowOnceMore, write=FShowOnceMore, nodefault};
	__property bool ShowAgainChecked = {read=FShowAgainChecked, write=FShowAgainChecked, nodefault};
	__property WideString ShowAgainText = {read=FShowAgainText, write=FShowAgainText};
	__property Elunicodestrings::TElWideStringArray* Captions = {read=FCaptions, write=SetCaptions};
	__property int CaptionIdx = {read=FCaptionIdx, write=FCaptionIdx, nodefault};
	__property int HelpContext = {read=FHelpCtx, write=FHelpCtx, nodefault};
	__property bool IsHTML = {read=FIsHTML, write=FIsHTML, nodefault};
	__property bool TopMost = {read=FTopmost, write=FTopmost, nodefault};
	__property Classes::TNotifyEvent OnTimer = {read=FOnTimer, write=FOnTimer};
	__property TPromptCloseEvent OnClose = {read=FOnClose, write=FOnClose};
	__property Classes::TNotifyEvent OnBeforeShow = {read=FOnBeforeShow, write=FOnBeforeShow};
	__property Htmlrender::TElHTMLImageNeededEvent OnHTMLImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property Htmlrender::TElHTMLLinkClickEvent OnLinkClick = {read=FOnLinkClick, write=FOnLinkClick};
	__property bool ParentFont = {read=FParentFont, write=SetParentFont, default=1};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE TElPromptForm* ElPromptForm;
extern PACKAGE System::ResourceString _SShowNextTime;
#define Elpromptdlg_SShowNextTime System::LoadResourceString(&Elpromptdlg::_SShowNextTime)
extern PACKAGE System::ResourceString _SDSecondsLeft;
#define Elpromptdlg_SDSecondsLeft System::LoadResourceString(&Elpromptdlg::_SDSecondsLeft)
extern PACKAGE Word __fastcall ElMessageDlg(const AnsiString Msg, Dialogs::TMsgDlgType DlgType, Dialogs::TMsgDlgButtons Buttons, int HelpCtx);
extern PACKAGE Word __fastcall ElMessageDlgEx2(const AnsiString Msg, Dialogs::TMsgDlgType DlgType, Dialogs::TMsgDlgButtons Buttons, int HelpCtx, bool IsHTML, Htmlrender::TElHTMLLinkClickEvent OnLinkClick);
extern PACKAGE Word __fastcall ElMessageDlgEx(const AnsiString Msg, Dialogs::TMsgDlgType DlgType, Dialogs::TMsgDlgButtons Buttons, int HelpCtx, TMetaClass* FormClass);

}	/* namespace Elpromptdlg */
using namespace Elpromptdlg;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElPromptDlg
