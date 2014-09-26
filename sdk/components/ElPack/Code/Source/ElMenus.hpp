// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMenus.pas' rev: 6.00

#ifndef ElMenusHPP
#define ElMenusHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <HTMLRender.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElHook.hpp>	// Pascal unit
#include <ElColor.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <CommCtrl.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <ActnList.hpp>	// Pascal unit
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

namespace Elmenus
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS EMenuError;
class PASCALIMPLEMENTATION EMenuError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EMenuError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EMenuError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EMenuError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EMenuError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EMenuError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EMenuError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EMenuError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EMenuError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EMenuError(void) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TMenuBreak { mbNone, mbBreak, mbBarBreak };
#pragma option pop

class DELPHICLASS TElMenuItem;
typedef void __fastcall (__closure *TMenuChangeEvent)(System::TObject* Sender, TElMenuItem* Source, bool Rebuild);

typedef void __fastcall (__closure *TMenuDrawItemEvent)(System::TObject* Sender, Graphics::TCanvas* ACanvas, const Types::TRect &ARect, bool Selected);

typedef void __fastcall (__closure *TMenuMeasureItemEvent)(System::TObject* Sender, Graphics::TCanvas* ACanvas, int &Width, int &Height);

class DELPHICLASS THackClass;
class PASCALIMPLEMENTATION THackClass : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Classes::TBiDiMode FBiDiMode;
	Menus::TMenuItem* FItems;
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual THackClass(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TComponent.Destroy */ inline __fastcall virtual ~THackClass(void) { }
	#pragma option pop
	
};


class DELPHICLASS TMenuActionLink;
class PASCALIMPLEMENTATION TMenuActionLink : public Actnlist::TActionLink 
{
	typedef Actnlist::TActionLink inherited;
	
protected:
	TElMenuItem* FClient;
	virtual void __fastcall AssignClient(System::TObject* AClient);
	virtual bool __fastcall IsCaptionLinked(void);
	virtual bool __fastcall IsCheckedLinked(void);
	virtual bool __fastcall IsEnabledLinked(void);
	virtual bool __fastcall IsHelpContextLinked(void);
	virtual bool __fastcall IsHintLinked(void);
	virtual bool __fastcall IsImageIndexLinked(void);
	virtual bool __fastcall IsShortCutLinked(void);
	virtual bool __fastcall IsVisibleLinked(void);
	virtual bool __fastcall IsOnExecuteLinked(void);
	HIDESBASE void __fastcall SetCaption(const WideString Value);
	virtual void __fastcall SetChecked(bool Value);
	virtual void __fastcall SetEnabled(bool Value);
	virtual void __fastcall SetHelpContext(Classes::THelpContext Value);
	HIDESBASE void __fastcall SetHint(const WideString Value);
	virtual void __fastcall SetImageIndex(int Value);
	virtual void __fastcall SetShortCut(Classes::TShortCut Value);
	virtual void __fastcall SetVisible(bool Value);
	virtual void __fastcall SetOnExecute(Classes::TNotifyEvent Value);
public:
	#pragma option push -w-inl
	/* TBasicActionLink.Create */ inline __fastcall virtual TMenuActionLink(System::TObject* AClient) : Actnlist::TActionLink(AClient) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TBasicActionLink.Destroy */ inline __fastcall virtual ~TMenuActionLink(void) { }
	#pragma option pop
	
};


typedef TMetaClass*TMenuActionLinkClass;

class DELPHICLASS TElMenu;
class PASCALIMPLEMENTATION TElMenu : public Menus::TMenu 
{
	typedef Menus::TMenu inherited;
	
public:
	#pragma option push -w-inl
	/* TMenu.Create */ inline __fastcall virtual TElMenu(Classes::TComponent* AOwner) : Menus::TMenu(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TMenu.Destroy */ inline __fastcall virtual ~TElMenu(void) { }
	#pragma option pop
	
};


#pragma option push -b-
enum TDrawStyle { tdsNormal, tdsOfficeXP, tdsWindowsXP };
#pragma option pop

class PASCALIMPLEMENTATION TElMenuItem : public Menus::TMenuItem 
{
	typedef Menus::TMenuItem inherited;
	
public:
	TElMenuItem* operator[](int Index) { return Items[Index]; }
	
private:
	Imglist::TChangeLink* FImageChangeLink;
	Imglist::TCustomImageList* FSubMenuImages;
	WideString FCaption;
	HMENU FHandle;
	bool FChecked;
	bool FEnabled;
	bool FDefault;
	bool FRadioItem;
	bool FVisible;
	Byte FGroupIndex;
	int FImageIndex;
	TMenuActionLink* FActionLink;
	TMenuBreak FBreak;
	Graphics::TBitmap* FBitmap;
	Graphics::TBitmap* FDisBitmap;
	Word FCommand;
	Classes::THelpContext FHelpContext;
	WideString FHint;
	Classes::TList* FItems;
	Classes::TShortCut FShortCut;
	TElMenuItem* FParent;
	TElMenuItem* FMerged;
	TElMenuItem* FMergedWith;
	TElMenu* FMenu;
	bool FStreamedRebuild;
	TMenuChangeEvent FOnChange;
	Classes::TNotifyEvent FOnClick;
	TMenuDrawItemEvent FOnDrawItem;
	TMenuMeasureItemEvent FOnMeasureItem;
	HIDESBASE void __fastcall AppendTo(HMENU Menu, bool ARightToLeft);
	HIDESBASE void __fastcall DoActionChange(System::TObject* Sender);
	HIDESBASE void __fastcall ReadShortCutText(Classes::TReader* Reader);
	HIDESBASE void __fastcall MergeWith(TElMenuItem* Menu);
	HIDESBASE void __fastcall RebuildHandle(void);
	HIDESBASE void __fastcall PopulateMenu(void);
	HIDESBASE void __fastcall SubItemChanged(System::TObject* Sender, TElMenuItem* Source, bool Rebuild);
	HIDESBASE void __fastcall TurnSiblingsOff(void);
	void __fastcall WriteShortCutText(Classes::TWriter* Writer);
	HIDESBASE void __fastcall VerifyGroupIndex(int Position, Byte Value);
	HIDESBASE Classes::TBasicAction* __fastcall GetAction(void);
	HIDESBASE void __fastcall SetAction(Classes::TBasicAction* Value);
	HIDESBASE void __fastcall SetSubMenuImages(Imglist::TCustomImageList* Value);
	HIDESBASE Graphics::TBitmap* __fastcall GetBitmap(void);
	HIDESBASE void __fastcall SetBitmap(Graphics::TBitmap* Value);
	HIDESBASE void __fastcall InitiateActions(void);
	HIDESBASE bool __fastcall IsCaptionStored(void);
	HIDESBASE bool __fastcall IsCheckedStored(void);
	HIDESBASE bool __fastcall IsEnabledStored(void);
	HIDESBASE bool __fastcall IsHintStored(void);
	HIDESBASE bool __fastcall IsHelpContextStored(void);
	HIDESBASE bool __fastcall IsImageIndexStored(void);
	HIDESBASE bool __fastcall IsOnClickStored(void);
	HIDESBASE bool __fastcall IsShortCutStored(void);
	HIDESBASE bool __fastcall IsVisibleStored(void);
	
protected:
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	DYNAMIC void __fastcall ActionChange(System::TObject* Sender, bool CheckDefaults);
	HIDESBASE void __fastcall ImageListChange(System::TObject* Sender);
	virtual void __fastcall AssignTo(Classes::TPersistent* Dest);
	HIDESBASE void __fastcall DoDrawText(Graphics::TCanvas* ACanvas, const WideString ACaption, Types::TRect &Rect, bool Selected, int Flags);
	virtual void __fastcall DrawItem(Graphics::TCanvas* ACanvas, const Types::TRect &ARect, bool Selected);
	HIDESBASE TMetaClass* __fastcall GetActionLinkClass(void);
	HIDESBASE HMENU __fastcall GetHandle(void);
	HIDESBASE int __fastcall GetCount(void);
	DYNAMIC void __fastcall GetChildren(Classes::TGetChildProc Proc, Classes::TComponent* Root);
	HIDESBASE TElMenuItem* __fastcall GetItem(int Index);
	HIDESBASE int __fastcall GetMenuIndex(void);
	HIDESBASE void __fastcall MeasureItem(Graphics::TCanvas* ACanvas, int &Width, int &Height);
	virtual void __fastcall MenuChanged(bool Rebuild);
	virtual void __fastcall Loaded(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	HIDESBASE void __fastcall SetBreak(TMenuBreak Value);
	HIDESBASE void __fastcall SetCaption(const WideString Value);
	HIDESBASE void __fastcall SetChecked(bool Value);
	DYNAMIC void __fastcall SetChildOrder(Classes::TComponent* Child, int Order);
	HIDESBASE void __fastcall SetDefault(bool Value);
	HIDESBASE void __fastcall SetEnabled(bool Value);
	HIDESBASE void __fastcall SetGroupIndex(Byte Value);
	HIDESBASE void __fastcall SetImageIndex(int Value);
	HIDESBASE void __fastcall SetMenuIndex(int Value);
	DYNAMIC void __fastcall SetParentComponent(Classes::TComponent* Value);
	HIDESBASE void __fastcall SetRadioItem(bool Value);
	HIDESBASE void __fastcall SetShortCut(Classes::TShortCut Value);
	HIDESBASE void __fastcall SetVisible(bool Value);
	__property TMenuActionLink* ActionLink = {read=FActionLink, write=FActionLink};
	HIDESBASE void __fastcall UpdateItems(void);
	int __fastcall GetImageWidth(void);
	void __fastcall SetHint(WideString Value);
	
public:
	__fastcall virtual TElMenuItem(Classes::TComponent* AOwner);
	__fastcall virtual ~TElMenuItem(void);
	virtual void __fastcall InitiateAction(void);
	void __fastcall DesignRebuild(void);
	HIDESBASE void __fastcall Insert(int Index, TElMenuItem* Item);
	HIDESBASE void __fastcall Delete(int Index);
	virtual void __fastcall Click(void);
	HIDESBASE TElMenuItem* __fastcall Find(WideString ACaption);
	HIDESBASE int __fastcall IndexOf(TElMenuItem* Item);
	DYNAMIC Classes::TComponent* __fastcall GetParentComponent(void);
	HIDESBASE TElMenu* __fastcall GetParentMenu(void);
	DYNAMIC bool __fastcall HasParent(void);
	HIDESBASE void __fastcall Add(TElMenuItem* Item);
	HIDESBASE void __fastcall Remove(TElMenuItem* Item);
	__property HMENU Handle = {read=GetHandle, nodefault};
	__property int Count = {read=GetCount, nodefault};
	__property TElMenuItem* Items[int Index] = {read=GetItem/*, default*/};
	__property int MenuIndex = {read=GetMenuIndex, write=SetMenuIndex, nodefault};
	__property TElMenuItem* Parent = {read=FParent};
	
__published:
	__property Classes::TBasicAction* Action = {read=GetAction, write=SetAction};
	__property Imglist::TCustomImageList* SubMenuImages = {read=FSubMenuImages, write=SetSubMenuImages};
	__property Graphics::TBitmap* Bitmap = {read=GetBitmap, write=SetBitmap};
	__property TMenuBreak Break = {read=FBreak, write=SetBreak, default=0};
	__property WideString Caption = {read=FCaption, write=SetCaption, stored=IsCaptionStored};
	__property bool Checked = {read=FChecked, write=SetChecked, stored=IsCheckedStored, default=0};
	__property bool Default = {read=FDefault, write=SetDefault, default=0};
	__property bool Enabled = {read=FEnabled, write=SetEnabled, stored=IsEnabledStored, default=1};
	__property Byte GroupIndex = {read=FGroupIndex, write=SetGroupIndex, default=0};
	__property Classes::THelpContext HelpContext = {read=FHelpContext, write=FHelpContext, stored=IsHelpContextStored, default=0};
	__property WideString Hint = {read=FHint, write=SetHint, stored=IsHintStored};
	__property int ImageIndex = {read=FImageIndex, write=SetImageIndex, stored=IsImageIndexStored, default=-1};
	__property bool RadioItem = {read=FRadioItem, write=SetRadioItem, default=0};
	__property Classes::TShortCut ShortCut = {read=FShortCut, write=SetShortCut, stored=IsShortCutStored, default=0};
	__property bool Visible = {read=FVisible, write=SetVisible, stored=IsVisibleStored, default=1};
	__property Classes::TNotifyEvent OnClick = {read=FOnClick, write=FOnClick, stored=IsOnClickStored};
	__property TMenuDrawItemEvent OnDrawItem = {read=FOnDrawItem, write=FOnDrawItem};
	__property TMenuMeasureItemEvent OnMeasureItem = {read=FOnMeasureItem, write=FOnMeasureItem};
	__property TMenuChangeEvent OnChange = {read=FOnChange, write=FOnChange};
};


class DELPHICLASS TElMainMenu;
class PASCALIMPLEMENTATION TElMainMenu : public Menus::TMainMenu 
{
	typedef Menus::TMainMenu inherited;
	
private:
	HMENU FOle2Menu;
	WideString FMenuImage;
	TElMenuItem* FUnicodeItems;
	Elhook::TElHook* FHook;
	TDrawStyle FDrawStyle;
	Graphics::TFont* FFont;
	Forms::TForm* FForm;
	Imglist::TChangeLink* FImageChangeLink;
	Controls::TImageList* FImages;
	bool FOwnerDraw;
	bool FIsHTML;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Htmlrender::TElHTMLRender* FRender;
	void __fastcall SetIsHTML(bool Value);
	HIDESBASE void __fastcall SetOwnerDraw(bool Value);
	HIDESBASE void __fastcall ImageListChange(System::TObject* Sender);
	HIDESBASE void __fastcall SetImages(Controls::TImageList* Value);
	HIDESBASE void __fastcall ItemChanged(void);
	void __fastcall OnBeforeHook(System::TObject* Sender, Messages::TMessage &Message, bool &Handled);
	void __fastcall SetDrawStyle(TDrawStyle Value);
	void __fastcall SetFont(const Graphics::TFont* Value);
	HIDESBASE bool __fastcall UpdateImage(void);
	
protected:
	bool FSystemFont;
	HIDESBASE bool __fastcall IsOwnerDraw(void);
	HIDESBASE void __fastcall ProcessMenuChar(Messages::TWMMenuChar &Message);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	HIDESBASE int __fastcall DoGetMenuString(HMENU Menu, unsigned ItemID, wchar_t * Str, int MaxCount, unsigned Flag);
	virtual void __fastcall MenuChanged(System::TObject* Sender, Menus::TMenuItem* Source, bool Rebuild);
	virtual HMENU __fastcall GetHandle(void);
	HIDESBASE bool __fastcall DispatchCommand(Word ACommand);
	virtual void __fastcall Loaded(void);
	void __fastcall SetSystemFont(bool Value);
	void __fastcall GetFont(void);
	void __fastcall FontChange(System::TObject* Sender);
	void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	
public:
	__fastcall virtual TElMainMenu(Classes::TComponent* AOwner);
	__fastcall virtual ~TElMainMenu(void);
	HIDESBASE TElMenuItem* __fastcall FindItem(int Value, Menus::TFindItemKind Kind);
	DYNAMIC bool __fastcall IsShortCut(Messages::TWMKey &Message);
	HIDESBASE void __fastcall UpdateItems(void);
	
__published:
	__property TElMenuItem* Items = {read=FUnicodeItems};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property TDrawStyle DrawStyle = {read=FDrawStyle, write=SetDrawStyle, default=0};
	__property Controls::TImageList* Images = {read=FImages, write=SetImages};
	__property bool OwnerDraw = {read=FOwnerDraw, write=SetOwnerDraw, default=0};
	__property bool SystemFont = {read=FSystemFont, write=SetSystemFont, default=1};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, default=0};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
};


class DELPHICLASS TElPopupMenu;
class PASCALIMPLEMENTATION TElPopupMenu : public Menus::TPopupMenu 
{
	typedef Menus::TPopupMenu inherited;
	
private:
	bool FIsHTML;
	Htmlrender::TElHTMLRender* FRender;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	Elhook::TElHook* FHook;
	TDrawStyle FDrawStyle;
	TElMenuItem* FUnicodeItems;
	#pragma pack(push, 1)
	Types::TPoint FPopupPoint;
	#pragma pack(pop)
	
	Forms::TForm* FForm;
	Graphics::TFont* FFont;
	void __fastcall SetDrawStyle(TDrawStyle Value);
	void __fastcall OnBeforeHook(System::TObject* Sender, Messages::TMessage &Message, bool &Handled);
	void __fastcall SetFont(const Graphics::TFont* Value);
	void __fastcall SetIsHTML(bool Value);
	
protected:
	bool FSystemFont;
	virtual void __fastcall DoPopup(System::TObject* Sender);
	HIDESBASE bool __fastcall IsOwnerDraw(void);
	HIDESBASE bool __fastcall DispatchCommand(Word ACommand);
	HIDESBASE virtual HMENU __fastcall GetHandle(void);
	void __fastcall GetFont(void);
	void __fastcall SetSystemFont(bool Value);
	void __fastcall FontChange(System::TObject* Sender);
	virtual void __fastcall Loaded(void);
	void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	
public:
	__fastcall virtual TElPopupMenu(Classes::TComponent* AOwner);
	__fastcall virtual ~TElPopupMenu(void);
	virtual void __fastcall Popup(int X, int Y);
	HIDESBASE TElMenuItem* __fastcall FindItem(int Value, Menus::TFindItemKind Kind);
	DYNAMIC bool __fastcall IsShortCut(Messages::TWMKey &Message);
	HIDESBASE void __fastcall ProcessMenuChar(Messages::TWMMenuChar &Message);
	HIDESBASE void __fastcall UpdateItems(void);
	__property Types::TPoint PopupPoint = {read=FPopupPoint};
	__property Handle  = {read=GetHandle};
	
__published:
	__property TElMenuItem* Items = {read=FUnicodeItems};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property TDrawStyle DrawStyle = {read=FDrawStyle, write=SetDrawStyle, default=0};
	__property bool SystemFont = {read=FSystemFont, write=SetSystemFont, default=1};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, default=0};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE unsigned FTheme;
extern PACKAGE HFONT __fastcall GetMenuFont(void);
extern PACKAGE void __fastcall CopyMenuItems(TElMenuItem* Dest, TElMenuItem* Source);
extern PACKAGE void __fastcall InsertItems(Menus::TMenu* &AMenu, TElMenuItem* MainItem, int Index, const TElMenuItem* const * Items, const int Items_Size);
extern PACKAGE void __fastcall InsertMenuItems(Menus::TMenu* &AMenu, int Index, const TElMenuItem* const * Items, const int Items_Size);
extern PACKAGE void __fastcall ElInitMenuItems(Menus::TMenu* AMenu, const TElMenuItem* const * Items, const int Items_Size);
extern PACKAGE TElMainMenu* __fastcall ElNewMenu(Classes::TComponent* Owner, const WideString AName, const TElMenuItem* const * Items, const int Items_Size);
extern PACKAGE TElMenuItem* __fastcall ElNewSubMenu(const WideString ACaption, Word hCtx, const WideString AName, const TElMenuItem* const * Items, const int Items_Size, bool AEnabled);
extern PACKAGE TElMenuItem* __fastcall ElNewItem(const WideString ACaption, Classes::TShortCut AShortCut, bool AChecked, bool AEnabled, Classes::TNotifyEvent AOnClick, Word hCtx, const WideString AName);
extern PACKAGE TElMenuItem* __fastcall ElNewLine(void);
extern PACKAGE WideString __fastcall ElGetHotkey(const WideString Text);
extern PACKAGE WideString __fastcall ElStripHotKey(const WideString Text);

}	/* namespace Elmenus */
using namespace Elmenus;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMenus
