// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHstgrm.pas' rev: 6.00

#ifndef ElHstgrmHPP
#define ElHstgrmHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Menus.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ElImgFrm.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhstgrm
{
//-- type declarations -------------------------------------------------------
typedef int THistoBuf[16384];

typedef int *PHistoBuf;

#pragma option push -b-
enum THistoDoubleMode { hdmCumulative, hdmHSplitOppositeIn, hdmHSplitOppositeOut, hdmHSplitSingle, hdmVSplitOpposite, hdmVSplitSingle };
#pragma option pop

class DELPHICLASS THistoBuffer;
class DELPHICLASS TElHistogram;
class PASCALIMPLEMENTATION TElHistogram : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
private:
	Forms::TFormBorderStyle FBorderStyle;
	Graphics::TBitmap* Bitmap;
	Graphics::TColor FBgColor;
	THistoBuffer* FBuffer;
	THistoBuffer* FBuffer2;
	Graphics::TColor FColor2;
	THistoDoubleMode FDoubleMode;
	THistoBuffer* FExtBuffer;
	THistoBuffer* FExtBuffer2;
	Graphics::TColor FGrColor;
	bool FHGrid;
	int FScale;
	bool FShowZeroValues;
	bool FSmooth;
	bool FUseBuffer2;
	bool FVGrid;
	Elimgfrm::TElImageForm* FImgForm;
	Elimgfrm::TImgFormChangeLink* FImgFormChLink;
	void __fastcall ImageFormChange(System::TObject* Sender);
	void __fastcall SetImageForm(Elimgfrm::TElImageForm* newValue);
	int __fastcall GetBufferSize(void);
	bool __fastcall GetTransparent(void);
	void __fastcall SetBgColor(Graphics::TColor aValue);
	void __fastcall SetBufferSize(int newValue);
	void __fastcall SetColor2(Graphics::TColor Value);
	void __fastcall SetDoubleMode(THistoDoubleMode Value);
	void __fastcall SetGrColor(Graphics::TColor aValue);
	void __fastcall SetHGrid(bool aValue);
	void __fastcall SetScale(int aValue);
	void __fastcall SetShowZeroValues(bool newValue);
	void __fastcall SetSmooth(bool aValue);
	void __fastcall SetTransparent(bool newValue);
	void __fastcall SetUseBuffer2(bool Value);
	void __fastcall SetVGrid(bool aValue);
	
protected:
	virtual void __fastcall Paint(void);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall SetBorderStyle(Forms::TBorderStyle newValue);
	MESSAGE void __fastcall IFMRepaintChildren(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElHistogram(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHistogram(void);
	__property THistoBuffer* Buffer = {read=FBuffer};
	__property THistoBuffer* Buffer2 = {read=FBuffer2};
	__property THistoBuffer* ExternalBuffer = {read=FExtBuffer, write=FExtBuffer};
	__property THistoBuffer* ExternalBuffer2 = {read=FExtBuffer2, write=FExtBuffer2};
	
__published:
	__property Action ;
	__property Align  = {default=0};
	__property Anchors  = {default=3};
	__property Graphics::TColor BgColor = {read=FBgColor, write=SetBgColor, default=0};
	__property int BufferSize = {read=GetBufferSize, write=SetBufferSize, default=16384};
	__property Color  = {default=16776960};
	__property Graphics::TColor Color2 = {read=FColor2, write=SetColor2, nodefault};
	__property Constraints ;
	__property DockOrientation ;
	__property THistoDoubleMode DoubleMode = {read=FDoubleMode, write=SetDoubleMode, nodefault};
	__property DragCursor  = {default=-12};
	__property DragKind  = {default=0};
	__property DragMode  = {default=0};
	__property Enabled  = {default=1};
	__property Floating ;
	__property Graphics::TColor GridColor = {read=FGrColor, write=SetGrColor, default=12632256};
	__property Height  = {default=30};
	__property bool HGrid = {read=FHGrid, write=SetHGrid, default=0};
	__property Elimgfrm::TElImageForm* ImageForm = {read=FImgForm, write=SetImageForm};
	__property OnClick ;
	__property OnDblClick ;
	__property OnDragDrop ;
	__property OnDragOver ;
	__property OnEndDrag ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnStartDrag ;
	__property ParentColor  = {default=0};
	__property ParentShowHint  = {default=1};
	__property PopupMenu ;
	__property int Scale = {read=FScale, write=SetScale, default=4096};
	__property ShowHint ;
	__property bool ShowZeroValues = {read=FShowZeroValues, write=SetShowZeroValues, nodefault};
	__property bool Smooth = {read=FSmooth, write=SetSmooth, default=0};
	__property bool Transparent = {read=GetTransparent, write=SetTransparent, nodefault};
	__property bool UseBuffer2 = {read=FUseBuffer2, write=SetUseBuffer2, nodefault};
	__property bool VGrid = {read=FVGrid, write=SetVGrid, default=0};
	__property Visible  = {default=1};
	__property Width  = {default=100};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, nodefault};
};


class PASCALIMPLEMENTATION THistoBuffer : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	int *FBuf;
	int FBufSize;
	int FElements;
	TElHistogram* FOwner;
	void __fastcall SetBufSize(int newValue);
	
public:
	__fastcall THistoBuffer(void);
	__fastcall virtual ~THistoBuffer(void);
	void __fastcall Add(int aValue);
	int __fastcall Average(int aSamples);
	void __fastcall Clear(void);
	void __fastcall Push(int aValue);
	__property int BufSize = {read=FBufSize, write=SetBufSize, nodefault};
};


//-- var, const, procedure ---------------------------------------------------
static const Word HISTOBUFFERSIZE = 0x4000;

}	/* namespace Elhstgrm */
using namespace Elhstgrm;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHstgrm
