// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElBiProgr.pas' rev: 6.00

#ifndef ElBiProgrHPP
#define ElBiProgrHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elbiprogr
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TProgrShowMode { psmAllFull, psmLightHalf, psmDarkHalf, psmAllHalf };
#pragma option pop

#pragma option push -b-
enum TElBevelStyle { ebsNone, ebsLowered, ebsRaised };
#pragma option pop

class DELPHICLASS TElBiProgressBar;
class PASCALIMPLEMENTATION TElBiProgressBar : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	bool FLightTextFullLine;
	bool FDarkTextFullLine;
	bool FLightButtonStyle;
	bool FDarkButtonStyle;
	int FMinValue;
	TElBevelStyle FBorderStyle;
	AnsiString FDarkText;
	AnsiString FLightText;
	Graphics::TColor FDarkTextColor;
	Graphics::TColor FLightTextColor;
	Graphics::TColor FLightColor;
	Graphics::TColor FDarkColor;
	int FScale;
	int FLightValue;
	int FDarkValue;
	Graphics::TBitmap* Bitmap;
	bool FAdditive;
	TProgrShowMode FProgrShowMode;
	AnsiString FCaption;
	void __fastcall SetLightColor(Graphics::TColor aValue);
	void __fastcall SetDarkColor(Graphics::TColor aValue);
	void __fastcall SetScale(int aValue);
	void __fastcall SetLightValue(int aValue);
	void __fastcall SetDarkValue(int aValue);
	void __fastcall SetAdditive(bool aValue);
	void __fastcall SetProgrShowMode(TProgrShowMode aValue);
	void __fastcall SetDarkText(AnsiString newValue);
	void __fastcall SetLightText(AnsiString newValue);
	void __fastcall SetCaption(AnsiString newValue);
	void __fastcall SetDarkTextColor(Graphics::TColor newValue);
	void __fastcall SetLightTextColor(Graphics::TColor newValue);
	void __fastcall SetBorderStyle(TElBevelStyle newValue);
	void __fastcall SetMinValue(int newValue);
	void __fastcall SetLightButtonStyle(bool newValue);
	void __fastcall SetDarkButtonStyle(bool newValue);
	void __fastcall SetTransparent(bool newValue);
	void __fastcall SetLightTextFullLine(bool newValue);
	void __fastcall SetDarkTextFullLine(bool newValue);
	bool __fastcall GetTransparent(void);
	
protected:
	virtual void __fastcall Paint(void);
	
public:
	__fastcall virtual TElBiProgressBar(Classes::TComponent* AOwner);
	__fastcall virtual ~TElBiProgressBar(void);
	void __fastcall SetValues(int ALightValue, int ADarkValue, int AScale, bool AAdditive);
	
__published:
	__property Graphics::TColor LightColor = {read=FLightColor, write=SetLightColor, default=255};
	__property Graphics::TColor DarkColor = {read=FDarkColor, write=SetDarkColor, default=128};
	__property int Scale = {read=FScale, write=SetScale, default=100};
	__property int LightValue = {read=FLightValue, write=SetLightValue, nodefault};
	__property int DarkValue = {read=FDarkValue, write=SetDarkValue, nodefault};
	__property bool Additive = {read=FAdditive, write=SetAdditive, nodefault};
	__property AnsiString Caption = {read=FCaption, write=SetCaption};
	__property TProgrShowMode ProgressShowMode = {read=FProgrShowMode, write=SetProgrShowMode, nodefault};
	__property bool LightTextFullLine = {read=FLightTextFullLine, write=SetLightTextFullLine, nodefault};
	__property bool DarkTextFullLine = {read=FDarkTextFullLine, write=SetDarkTextFullLine, nodefault};
	__property AnsiString DarkText = {read=FDarkText, write=SetDarkText};
	__property AnsiString LightText = {read=FLightText, write=SetLightText};
	__property Graphics::TColor DarkTextColor = {read=FDarkTextColor, write=SetDarkTextColor, nodefault};
	__property Graphics::TColor LightTextColor = {read=FLightTextColor, write=SetLightTextColor, nodefault};
	__property TElBevelStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, nodefault};
	__property int MinValue = {read=FMinValue, write=SetMinValue, default=0};
	__property bool LightButtonStyle = {read=FLightButtonStyle, write=SetLightButtonStyle, nodefault};
	__property bool DarkButtonStyle = {read=FDarkButtonStyle, write=SetDarkButtonStyle, nodefault};
	__property bool Transparent = {read=GetTransparent, write=SetTransparent, nodefault};
	__property Align  = {default=0};
	__property Color  = {default=-2147483643};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=0};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Visible  = {default=1};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elbiprogr */
using namespace Elbiprogr;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElBiProgr
