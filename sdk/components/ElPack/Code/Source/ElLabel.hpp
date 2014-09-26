// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElLabel.pas' rev: 6.00

#ifndef ElLabelHPP
#define ElLabelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElCLabel.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Ellabel
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElLabelTextStyle { ltsNormal, ltsEmbossed, ltsRecessed, ltsOutlined };
#pragma option pop

#pragma option push -b-
enum TElLabelEffectStyle { lesNone, lesShadow, lesExtrude };
#pragma option pop

#pragma option push -b-
enum TElLabelExtrudePosition { lepLeft, lepLeftTop, lepTop, lepRightTop, lepRight, lepRightBottom, lepBottom, lepLeftBottom };
#pragma option pop

class DELPHICLASS TElLabel;
class PASCALIMPLEMENTATION TElLabel : public Elclabel::TElCustomLabel 
{
	typedef Elclabel::TElCustomLabel inherited;
	
private:
	int FAngle;
	Graphics::TBitmap* FBuffer;
	Graphics::TColor FDarkColor;
	int FDepth;
	Graphics::TColor FDisabledDarkColor;
	Graphics::TColor FDisabledLightColor;
	TElLabelEffectStyle FEffect;
	Graphics::TColor FFarColor;
	Graphics::TColor FLightColor;
	Graphics::TColor FNearColor;
	Graphics::TColor FOutlineColor;
	TElLabelExtrudePosition FPosition;
	Graphics::TColor FShadowColor;
	bool FStriated;
	TElLabelTextStyle FStyle;
	int FXOffset;
	int FYOffset;
	void __fastcall SetAngle(int Value);
	void __fastcall SetDarkColor(const Graphics::TColor Value);
	void __fastcall SetDepth(const int Value);
	void __fastcall SetDisabledDarkColor(const Graphics::TColor Value);
	void __fastcall SetDisabledLightColor(const Graphics::TColor Value);
	void __fastcall SetEffect(const TElLabelEffectStyle Value);
	void __fastcall SetExtrudePosition(const TElLabelExtrudePosition Value);
	void __fastcall SetFarColor(const Graphics::TColor Value);
	void __fastcall SetLightColor(const Graphics::TColor Value);
	void __fastcall SetNearColor(const Graphics::TColor Value);
	void __fastcall SetOutlineColor(const Graphics::TColor Value);
	void __fastcall SetShadowColor(const Graphics::TColor Value);
	void __fastcall SetStriated(const bool Value);
	void __fastcall SetStyle(const TElLabelTextStyle Value);
	void __fastcall SetXOffset(const int Value);
	void __fastcall SetYOffset(const int Value);
	
protected:
	WideString FCaption;
	WideString FHint;
	DYNAMIC void __fastcall AdjustBounds(void);
	DYNAMIC void __fastcall DoDrawText(Types::TRect &Rect, int Flags);
	virtual void __fastcall DrawDisabledText(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags);
	DYNAMIC void __fastcall DrawEffect(Graphics::TCanvas* Canvas, Types::TRect &Rect, int Flags);
	void __fastcall DrawExtrusion(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags, Graphics::TColor NearColor, Graphics::TColor FarColor);
	void __fastcall DrawNormalText(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags);
	void __fastcall DrawOutlinedText(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags, Graphics::TColor OutlineColor);
	void __fastcall DrawRaisedText(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags, Graphics::TColor LeftTop, Graphics::TColor RightBottom);
	void __fastcall DrawShadow(Graphics::TCanvas* Canvas, Types::TRect &Rect, WideString Text, int Flags, int X, int Y, Graphics::TColor ShadowColor);
	DYNAMIC void __fastcall DrawText(Graphics::TCanvas* Canvas, Types::TRect &Rect, int Flags);
	virtual void __fastcall Paint(void);
	virtual void __fastcall SetCaption(WideString newValue);
	virtual void __fastcall SetAutoSize(bool newValue);
	void __fastcall SetHint(WideString Value);
	HIDESBASE MESSAGE void __fastcall CMHintShow(Messages::TMessage &Message);
	virtual void __fastcall SetName(const AnsiString Value);
	__property int Angle = {read=FAngle, write=SetAngle, default=0};
	
public:
	__fastcall virtual TElLabel(Classes::TComponent* AOwner);
	__fastcall virtual ~TElLabel(void);
	
__published:
	__property WideString Caption = {read=FCaption, write=SetCaption};
	__property Graphics::TColor DarkColor = {read=FDarkColor, write=SetDarkColor, default=-2147483632};
	__property int Depth = {read=FDepth, write=SetDepth, default=10};
	__property Graphics::TColor DisabledDarkColor = {read=FDisabledDarkColor, write=SetDisabledDarkColor, default=-2147483632};
	__property Graphics::TColor DisabledLightColor = {read=FDisabledLightColor, write=SetDisabledLightColor, default=-2147483628};
	__property TElLabelEffectStyle Effect = {read=FEffect, write=SetEffect, default=0};
	__property TElLabelExtrudePosition ExtrudePosition = {read=FPosition, write=SetExtrudePosition, default=5};
	__property Graphics::TColor FarColor = {read=FFarColor, write=SetFarColor, default=0};
	__property Graphics::TColor LightColor = {read=FLightColor, write=SetLightColor, default=-2147483628};
	__property Graphics::TColor NearColor = {read=FNearColor, write=SetNearColor, default=0};
	__property Graphics::TColor OutlineColor = {read=FOutlineColor, write=SetOutlineColor, default=16777215};
	__property Graphics::TColor ShadowColor = {read=FShadowColor, write=SetShadowColor, default=-2147483632};
	__property bool Striated = {read=FStriated, write=SetStriated, default=0};
	__property TElLabelTextStyle Style = {read=FStyle, write=SetStyle, default=0};
	__property int XOffset = {read=FXOffset, write=SetXOffset, default=2};
	__property int YOffset = {read=FYOffset, write=SetYOffset, default=2};
	__property WideString Hint = {read=FHint, write=SetHint};
	__property Transparent  = {default=1};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Ellabel */
using namespace Ellabel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElLabel
