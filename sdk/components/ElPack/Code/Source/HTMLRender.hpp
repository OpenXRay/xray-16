// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'HTMLRender.pas' rev: 6.00

#ifndef HTMLRenderHPP
#define HTMLRenderHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElStack.hpp>	// Pascal unit
#include <ElList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Htmlrender
{
//-- type declarations -------------------------------------------------------
typedef TElWideStrings TElFStrings;
;

typedef TElWideStringList TElFStringList;
;

typedef WideString TElFString;

typedef void __fastcall (__closure *TElHTMLImageNeededEvent)(System::TObject* Sender, WideString Src, Graphics::TBitmap* &Image);

typedef void __fastcall (__closure *TElHTMLLinkClickEvent)(System::TObject* Sender, WideString HRef);

#pragma option push -b-
enum THTMLItemType { hitChar, hitSoftBreak, hitBreak, hitPara, hitBitmap, hitHR, hitLI, hitUL };
#pragma option pop

typedef Set<THTMLItemType, hitChar, hitUL>  THTMLItemTypes;

class DELPHICLASS TElHTMLItem;
class DELPHICLASS TElHTMLData;
class DELPHICLASS TElHTMLRender;
class PASCALIMPLEMENTATION TElHTMLRender : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TElHTMLImageNeededEvent FOnImageNeeded;
	TElHTMLData* FIntData;
	TElHTMLData* FData;
	Graphics::TCanvas* Canvas;
	Graphics::TBitmap* Bitmap;
	HGDIOBJ FSaveObj;
	
protected:
	virtual void __fastcall TriggerImageNeededEvent(WideString Src, Graphics::TBitmap* &Image);
	void __fastcall CalcTokenSizes(TElHTMLData* FCurData);
	tagSIZE __fastcall GetTextSize();
	TElHTMLItem* __fastcall FindItemAt(const Types::TPoint &Point, const Types::TPoint &SrcPoint, const Types::TRect &R);
	
public:
	__fastcall TElHTMLRender(void);
	__fastcall virtual ~TElHTMLRender(void);
	void __fastcall DestroyData(TElHTMLData* Data);
	void __fastcall SetData(TElHTMLData* NewData);
	TElHTMLData* __fastcall CreateData(void);
	void __fastcall DrawText(Graphics::TCanvas* Canvas, const Types::TPoint &SrcPoint, const Types::TRect &R, Graphics::TColor AdjustFromColor);
	void __fastcall DrawTextEx(Graphics::TCanvas* Canvas, const Types::TPoint &SrcPoint, const Types::TRect &R, bool UseOverColors, Graphics::TColor Color, Graphics::TColor BkColor, Graphics::TColor SelColor, Graphics::TColor SelBkColor, Graphics::TColor AdjustFromColor);
	bool __fastcall IsCursorOverLink(const Types::TPoint &Point, const Types::TPoint &SrcPoint, const Types::TRect &R, WideString &href);
	void __fastcall SelectLinkAt(const Types::TPoint &Point, const Types::TPoint &SrcPoint, const Types::TRect &R);
	void __fastcall SelectPrevLink(void);
	void __fastcall SelectNextLink(void);
	void __fastcall PrepareToData(WideString Text, int MaxWidth, bool AutoWrap, TElHTMLData* CurData);
	void __fastcall PrepareText(WideString Text, int MaxWidth, bool AutoWrap);
	__property TElHTMLData* Data = {read=FData};
	__property TElHTMLImageNeededEvent OnImageNeeded = {read=FOnImageNeeded, write=FOnImageNeeded};
};


class PASCALIMPLEMENTATION TElHTMLData : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TElHTMLRender* FRender;
	Graphics::TColor FLinkColor;
	Graphics::TColor FDefaultColor;
	Graphics::TFontStyles FLinkStyle;
	Graphics::TFontStyles FDefaultStyle;
	int FDefaultHeight;
	Graphics::TFontCharset FCharset;
	AnsiString FDefaultFont;
	Ellist::TElList* FArray;
	Graphics::TColor FDefaultBgColor;
	Graphics::TColor FHighlightBgColor;
	Graphics::TColor FHighlightColor;
	TElHTMLItem* FSelectedItem;
	#pragma pack(push, 1)
	tagSIZE FTextSize;
	#pragma pack(pop)
	
	
public:
	__fastcall TElHTMLData(void);
	__fastcall virtual ~TElHTMLData(void);
	void __fastcall ClearArray(void);
	int __fastcall LineCount(void);
	__property tagSIZE TextSize = {read=FTextSize};
	__property Graphics::TColor LinkColor = {read=FLinkColor, write=FLinkColor, nodefault};
	__property Graphics::TColor DefaultBgColor = {read=FDefaultBgColor, write=FDefaultBgColor, nodefault};
	__property Graphics::TColor DefaultColor = {read=FDefaultColor, write=FDefaultColor, nodefault};
	__property Graphics::TFontStyles LinkStyle = {read=FLinkStyle, write=FLinkStyle, nodefault};
	__property Graphics::TFontStyles DefaultStyle = {read=FDefaultStyle, write=FDefaultStyle, nodefault};
	__property int DefaultHeight = {read=FDefaultHeight, write=FDefaultHeight, nodefault};
	__property AnsiString DefaultFont = {read=FDefaultFont, write=FDefaultFont};
	__property Graphics::TFontCharset Charset = {read=FCharset, write=FCharset, nodefault};
	__property Graphics::TColor HighlightBgColor = {read=FHighlightBgColor, write=FHighlightBgColor, nodefault};
	__property Graphics::TColor HighlightColor = {read=FHighlightColor, write=FHighlightColor, nodefault};
	__property TElHTMLItem* SelectedItem = {read=FSelectedItem, write=FSelectedItem};
};


class PASCALIMPLEMENTATION TElHTMLItem : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TElHTMLData* FOwner;
	THTMLItemType ItemType;
	WideString FText;
	Graphics::TFontStyles FontStyle;
	int FontHeight;
	Graphics::TColor FontBgColor;
	Graphics::TColor FontColor;
	WideString FLinkRef;
	WideString FFontRef;
	int FFntCnt;
	Word TWidth;
	Word Width;
	Word Height;
	int Indent;
	int FBoolState;
	virtual int __fastcall GetWidth(void);
	virtual int __fastcall GetHeight(int &BaseLine);
	bool __fastcall GetIsLink(void);
	void __fastcall SetIsLink(bool Value);
	bool __fastcall GetIsSub(void);
	void __fastcall SetIsSub(bool Value);
	bool __fastcall GetIsSuper(void);
	void __fastcall SetIsSuper(bool Value);
	
public:
	void __fastcall Assign(TElHTMLItem* Source);
	__fastcall TElHTMLItem(TElHTMLData* Owner);
	__fastcall virtual ~TElHTMLItem(void);
	__property WideString Text = {read=FText, write=FText};
	__property WideString LinkRef = {read=FLinkRef, write=FLinkRef};
	__property bool IsLink = {read=GetIsLink, write=SetIsLink, nodefault};
	__property bool IsSub = {read=GetIsSub, write=SetIsSub, nodefault};
	__property bool IsSuper = {read=GetIsSuper, write=SetIsSuper, nodefault};
};


class DELPHICLASS TElHTMLBreakItem;
class PASCALIMPLEMENTATION TElHTMLBreakItem : public TElHTMLItem 
{
	typedef TElHTMLItem inherited;
	
private:
	int FParams;
	int ListLevel;
	int ListItemN;
	virtual int __fastcall GetWidth(void);
	virtual int __fastcall GetHeight(int &BaseLine);
	
public:
	HIDESBASE void __fastcall Assign(TElHTMLItem* Source);
	void __fastcall AssignBreakProps(TElHTMLBreakItem* Source);
public:
	#pragma option push -w-inl
	/* TElHTMLItem.Create */ inline __fastcall TElHTMLBreakItem(TElHTMLData* Owner) : TElHTMLItem(Owner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElHTMLItem.Destroy */ inline __fastcall virtual ~TElHTMLBreakItem(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint flIsLink = 0x1;
static const Shortint flSub = 0x2;
static const Shortint flSuper = 0x4;

}	/* namespace Htmlrender */
using namespace Htmlrender;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// HTMLRender
