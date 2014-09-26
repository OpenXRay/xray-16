// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElGauge.pas' rev: 6.00

#ifndef ElGaugeHPP
#define ElGaugeHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <ElCGControl.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elgauge
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElGaugeKind { egkNeedle, egkPie };
#pragma option pop

class DELPHICLASS TElGauge;
class PASCALIMPLEMENTATION TElGauge : public Elcgcontrol::TElCustomGraphicControl 
{
	typedef Elcgcontrol::TElCustomGraphicControl inherited;
	
private:
	bool FTransparent;
	TElGaugeKind FGaugeKind;
	AnsiString FText;
	int FValue;
	int FMinValue;
	int FMaxValue;
	bool FShowPoints;
	int FPoints;
	Graphics::TColor FBackColor;
	Graphics::TColor FForeColor;
	int FCriticalValue;
	Graphics::TColor FCriticalColor;
	Graphics::TBitmap* Bitmap;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	void __fastcall SetValue(int newValue);
	void __fastcall SetMinValue(int newValue);
	void __fastcall SetMaxValue(int newValue);
	void __fastcall SetShowPoints(bool newValue);
	void __fastcall SetPoints(int newValue);
	void __fastcall SetBackColor(Graphics::TColor newValue);
	void __fastcall SetForeColor(Graphics::TColor newValue);
	void __fastcall SetCriticalValue(int newValue);
	void __fastcall SetCriticalColor(Graphics::TColor newValue);
	void __fastcall SetGaugeKind(TElGaugeKind newValue);
	void __fastcall SetTransparent(bool newValue);
	
protected:
	HIDESBASE void __fastcall SetText(AnsiString Value);
	virtual void __fastcall Paint(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElGauge(Classes::TComponent* AOwner);
	__fastcall virtual ~TElGauge(void);
	
__published:
	__property int Value = {read=FValue, write=SetValue, nodefault};
	__property int MinValue = {read=FMinValue, write=SetMinValue, nodefault};
	__property int MaxValue = {read=FMaxValue, write=SetMaxValue, nodefault};
	__property bool ShowPoints = {read=FShowPoints, write=SetShowPoints, nodefault};
	__property int Points = {read=FPoints, write=SetPoints, default=11};
	__property Graphics::TColor BackColor = {read=FBackColor, write=SetBackColor, nodefault};
	__property Graphics::TColor ForeColor = {read=FForeColor, write=SetForeColor, nodefault};
	__property int CriticalValue = {read=FCriticalValue, write=SetCriticalValue, nodefault};
	__property Graphics::TColor CriticalColor = {read=FCriticalColor, write=SetCriticalColor, nodefault};
	__property bool Transparent = {read=FTransparent, write=SetTransparent, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property Align  = {default=0};
	__property Color ;
	__property Enabled  = {default=1};
	__property Font ;
	__property ParentColor  = {default=1};
	__property ParentFont  = {default=1};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property ShowHint ;
	__property Visible  = {default=1};
	__property AnsiString Text = {read=FText, write=SetText};
	__property TElGaugeKind GaugeKind = {read=FGaugeKind, write=SetGaugeKind, nodefault};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property Anchors  = {default=3};
	__property Action ;
	__property Constraints ;
	__property DockOrientation ;
	__property Floating ;
	__property DragKind  = {default=0};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elgauge */
using namespace Elgauge;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElGauge
