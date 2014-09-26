// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElMouseHint.pas' rev: 6.00

#ifndef ElMouseHintHPP
#define ElMouseHintHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHintWnd.hpp>	// Pascal unit
#include <HTMLRender.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elmousehint
{
//-- type declarations -------------------------------------------------------
typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

#pragma option push -b-
enum TElMouseHintLocation { hlLeftTop, hlLeftCenter, hlLeftBottom, hlRightTop, hlRightCenter, hlRightBottom, hlCenterTop, hlCenterBottom };
#pragma option pop

typedef Set<TElMouseHintLocation, hlLeftTop, hlCenterBottom>  TElMousehintLocations;

class DELPHICLASS TElMouseHint;
class PASCALIMPLEMENTATION TElMouseHint : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Elhintwnd::TElHintWindow* FHintWindow;
	
protected:
	bool FActive;
	bool FAutoSize;
	bool FIsHTML;
	Htmlrender::TElHTMLImageNeededEvent FOnImageNeeded;
	bool FUseSystemHintSettings;
	Elunicodestrings::TElWideStrings* FLines;
	bool FWordWrap;
	Graphics::TColor FColor;
	int FWidth;
	int FHeight;
	TElMouseHintLocation FLocation;
	Graphics::TFont* FFont;
	Controls::TControl* FBoundingControl;
	bool FVisible;
	int FDistanceToHint;
	bool FKeepWithinWindow;
	int FHideCount;
	TElMousehintLocations FLocations;
	bool FAutoAdjustLocation;
	void __fastcall SetActive(bool Value);
	void __fastcall SetAutoSize(bool Value);
	void __fastcall SetCaption(WideString Value);
	void __fastcall SetIsHTML(bool Value);
	virtual void __fastcall TriggerImageNeededEvent(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);
	void __fastcall SetLines(Elunicodestrings::TElWideStrings* Value);
	void __fastcall LinesChange(System::TObject* Sender);
	WideString __fastcall GetCaption();
	void __fastcall BuildHint(void);
	void __fastcall SetWordWrap(bool Value);
	void __fastcall SetColor(Graphics::TColor Value);
	bool __fastcall GetVisible(void);
	void __fastcall SetWidth(int Value);
	void __fastcall SetHeight(int Value);
	void __fastcall SetLocation(TElMouseHintLocation Value);
	void __fastcall SetFont(Graphics::TFont* Value);
	void __fastcall FontChange(System::TObject* Sender);
	void __fastcall SetBoundingControl(Controls::TControl* Value);
	Controls::TControl* __fastcall GetBoundingControl(void);
	void __fastcall UpdateHintPos(const Types::TPoint &MousePos, Controls::TControl* Control);
	Types::TRect __fastcall DoUpdatePos(const Types::TPoint &MousePos, TElMouseHintLocation Location);
	void __fastcall SetDistanceToHint(int Value);
	void __fastcall SetUseSystemHintSettings(bool Value);
	void __fastcall DoShowHintWindow(void);
	void __fastcall DoHideHintWindow(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation operation);
	void __fastcall SetKeepWithinWindow(bool Value);
	void __fastcall SetAutoAdjustLocation(bool Value);
	
public:
	__fastcall virtual TElMouseHint(Classes::TComponent* AOwner);
	__fastcall virtual ~TElMouseHint(void);
	void __fastcall ShowHint(void);
	void __fastcall HideHint(void);
	virtual void __fastcall Loaded(void);
	void __fastcall ShowLines(Elunicodestrings::TElWideStrings* Lines);
	void __fastcall ShowString(WideString Caption);
	__property bool Visible = {read=GetVisible, nodefault};
	
__published:
	__property bool Active = {read=FActive, write=SetActive, nodefault};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=1};
	__property WideString Caption = {read=GetCaption, write=SetCaption};
	__property bool IsHTML = {read=FIsHTML, write=SetIsHTML, nodefault};
	__property Htmlrender::TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
	__property bool UseSystemHintSettings = {read=FUseSystemHintSettings, write=SetUseSystemHintSettings, default=1};
	__property Elunicodestrings::TElWideStrings* Lines = {read=FLines, write=SetLines};
	__property bool WordWrap = {read=FWordWrap, write=SetWordWrap, default=0};
	__property Graphics::TColor Color = {read=FColor, write=SetColor, nodefault};
	__property int Width = {read=FWidth, write=SetWidth, nodefault};
	__property int Height = {read=FHeight, write=SetHeight, nodefault};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property Controls::TControl* BoundingControl = {read=FBoundingControl, write=SetBoundingControl};
	__property int DistanceToHint = {read=FDistanceToHint, write=SetDistanceToHint, default=2};
	__property bool KeepWithinWindow = {read=FKeepWithinWindow, write=SetKeepWithinWindow, default=0};
	__property bool AutoAdjustLocation = {read=FAutoAdjustLocation, write=SetAutoAdjustLocation, default=0};
	__property TElMouseHintLocation Location = {read=FLocation, write=SetLocation, default=0};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elmousehint */
using namespace Elmousehint;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElMouseHint
