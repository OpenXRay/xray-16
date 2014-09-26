// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'Gradient.pas' rev: 6.00

#ifndef GradientHPP
#define GradientHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
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

namespace Gradient
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TFillDirection { fdLeftToRight, fdRightToLeft, fdUpToBottom, fdBottomToUp };
#pragma option pop

typedef Byte TNumberOfColors;

class DELPHICLASS TGradient;
class PASCALIMPLEMENTATION TGradient : public Extctrls::TCustomPanel 
{
	typedef Extctrls::TCustomPanel inherited;
	
private:
	TFillDirection FDirection;
	Graphics::TColor FBeginColor;
	Graphics::TColor FEndColor;
	bool FAutoSize;
	TNumberOfColors FNumberOfColors;
	Graphics::TFont* FFont;
	AnsiString FCaption;
	int FTextTop;
	int FTextLeft;
	bool FBorder;
	int FBorderWidth;
	Graphics::TColor FBorderColor;
	void __fastcall SetFillDirection(TFillDirection Value);
	HIDESBASE void __fastcall SetAutoSize(bool Value);
	void __fastcall SetBeginColor(Graphics::TColor Value);
	void __fastcall SetEndColor(Graphics::TColor Value);
	void __fastcall SetNumberOfColors(TNumberOfColors Value);
	HIDESBASE void __fastcall SetFont(Graphics::TFont* AFont);
	void __fastcall SetCaption(AnsiString Value);
	void __fastcall SetTextTop(int Value);
	void __fastcall SetTextLeft(int Value);
	void __fastcall SetBorder(bool Value);
	HIDESBASE void __fastcall SetBorderWidth(int Value);
	void __fastcall SetBorderColor(Graphics::TColor Value);
	void __fastcall GradientFill(void);
	
protected:
	Classes::TNotifyEvent FOnPaint;
	virtual void __fastcall Paint(void);
	
public:
	__property Canvas ;
	__fastcall virtual TGradient(Classes::TComponent* AOwner);
	
__published:
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, default=0};
	__property Graphics::TColor BeginColor = {read=FBeginColor, write=SetBeginColor, default=16711680};
	__property Graphics::TColor EndColor = {read=FEndColor, write=SetEndColor, default=0};
	__property TFillDirection FillDirection = {read=FDirection, write=SetFillDirection, default=0};
	__property TNumberOfColors NumberOfColors = {read=FNumberOfColors, write=SetNumberOfColors, default=255};
	__property Graphics::TFont* Font = {read=FFont, write=SetFont};
	__property AnsiString Caption = {read=FCaption, write=SetCaption};
	__property int TextTop = {read=FTextTop, write=SetTextTop, nodefault};
	__property int TextLeft = {read=FTextLeft, write=SetTextLeft, nodefault};
	__property bool Border = {read=FBorder, write=SetBorder, nodefault};
	__property int BorderWidth = {read=FBorderWidth, write=SetBorderWidth, nodefault};
	__property Graphics::TColor BorderColor = {read=FBorderColor, write=SetBorderColor, nodefault};
	__property Color  = {default=-2147483633};
	__property Align  = {default=0};
	__property DragCursor  = {default=-12};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
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
	__property BorderStyle  = {default=0};
	__property Classes::TNotifyEvent OnPaint = {read=FOnPaint, write=FOnPaint};
public:
	#pragma option push -w-inl
	/* TCustomControl.Destroy */ inline __fastcall virtual ~TGradient(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TGradient(HWND ParentWindow) : Extctrls::TCustomPanel(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Gradient */
using namespace Gradient;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// Gradient
