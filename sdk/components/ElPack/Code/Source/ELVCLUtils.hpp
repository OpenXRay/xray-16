// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElVCLUtils.pas' rev: 6.00

#ifndef ElVCLUtilsHPP
#define ElVCLUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTools.hpp>	// Pascal unit
#include <ElUnicodeStrings.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elvclutils
{
//-- type declarations -------------------------------------------------------
#pragma pack(push, 1)
struct TBlendFunction
{
	Byte BlendOp;
	Byte BlendFlags;
	Byte SourceConstantAlpha;
	Byte AlphaFormat;
} ;
#pragma pack(pop)

typedef BOOL __stdcall (*TAlphaBlend)(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, int blendFunction);

#pragma option push -b-
enum TElBkGndType { bgtTileBitmap, bgtStretchBitmap, bgtColorFill, bgtCenterBitmap, bgtHorzGradient, bgtVertGradient, bgtTopLeftBitmap };
#pragma option pop

#pragma option push -b-
enum TElBorderSide { ebsLeft, ebsRight, ebsTop, ebsBottom };
#pragma option pop

typedef Set<TElBorderSide, ebsLeft, ebsBottom>  TElBorderSides;

#pragma option push -b-
enum TElFlatBorderType { fbtFlat, fbtSunken, fbtSunkenInner, fbtSunkenOuter, fbtRaised, fbtRaisedInner, fbtRaisedOuter, fbtBump, fbtBigBump, fbtEtched, fbtFramed, fbtLine, fbtLineBorder, fbtNone, fbtColorLineBorder };
#pragma option pop

#pragma option push -b-
enum TElTextDrawType { tdtNormal, tdtShadowed, tdtRaised };
#pragma option pop

#pragma option push -b-
enum TElArrowDir { eadLeft, eadUp, eadRight, eadDown };
#pragma option pop

#pragma option push -b-
enum TTaskbarEdge { tbeBottom, tbeLeft, tbeTop, tbeRight };
#pragma option pop

#pragma option push -b-
enum TElTextCase { etcNoChange, etcUppercase, etcLowercase };
#pragma option pop

//-- var, const, procedure ---------------------------------------------------
static const Shortint MOUSE_WHEEL_DELTA = 0x78;
static const Word SC_DRAGMOVE = 0xf012;
extern PACKAGE unsigned ParentControlRepaintedMessage;
extern PACKAGE TElBorderSides AllBorderSides;
extern PACKAGE unsigned smXEdge[2];
extern PACKAGE unsigned smYEdge[2];
extern PACKAGE Controls::TCursor WaitCursor;
extern PACKAGE char __fastcall GetTimeAMChar(void);
extern PACKAGE char __fastcall GetTimePMChar(void);
extern PACKAGE int __fastcall IncColor(const Graphics::TColor Color, int RInc, int GInc, int BInc);
extern PACKAGE Graphics::TColor __fastcall InvertColor(Graphics::TColor aColor);
extern PACKAGE Types::TRect __fastcall GetWorkSpaceRect();
extern PACKAGE int __fastcall GetWorkSpaceTop(void);
extern PACKAGE int __fastcall GetWorkSpaceLeft(void);
extern PACKAGE int __fastcall GetWorkSpaceBottom(void);
extern PACKAGE int __fastcall GetWorkSpaceRight(void);
extern PACKAGE Types::TRect __fastcall GetDesktopRect();
extern PACKAGE int __fastcall GetDesktopTop(void);
extern PACKAGE int __fastcall GetDesktopLeft(void);
extern PACKAGE int __fastcall GetDesktopBottom(void);
extern PACKAGE int __fastcall GetDesktopRight(void);
extern PACKAGE void __fastcall MinimizeToTray(HWND Wnd);
extern PACKAGE Types::TRect __fastcall GetSysTrayRect();
extern PACKAGE Types::TRect __fastcall GetTaskbarRect();
extern PACKAGE TTaskbarEdge __fastcall GetTaskbarEdge(void);
extern PACKAGE int __fastcall GetKeybTimes(int TimeKind);
extern PACKAGE void __fastcall GradientFillEx(HDC DC, const Types::TRect &DCRect, const Types::TRect &R, const Types::TPoint &Origin, Graphics::TColor StartColor, Graphics::TColor EndColor, int Steps, bool Vertical);
extern PACKAGE void __fastcall GradientFill(HDC DC, const Types::TRect &R, Graphics::TColor StartColor, Graphics::TColor EndColor, int Steps, bool Vertical);
extern PACKAGE void __fastcall DrawArrow(Graphics::TCanvas* Canvas, TElArrowDir Dir, const Types::TRect &R, Graphics::TColor Color, bool Enabled);
extern PACKAGE bool __fastcall RectsIntersect(const Types::TRect &R1, const Types::TRect &R2);
extern PACKAGE Controls::TWinControl* __fastcall FindVCLChild(Controls::TWinControl* Control, HWND ChildHandle);
extern PACKAGE void __fastcall DrawTransparentBitmapEx(HDC DC, Graphics::TBitmap* Bitmap, int X, int Y, const Types::TRect &Src, Graphics::TColor Transparent);
extern PACKAGE void __fastcall DrawTypedTextW(Graphics::TCanvas* Canvas, const Types::TRect &Bounds, WideString Text, int Flags, TElTextDrawType DrawType);
extern PACKAGE void __fastcall DrawTypedText(Graphics::TCanvas* Canvas, const Types::TRect &Bounds, AnsiString Text, int Flags, TElTextDrawType DrawType);
extern PACKAGE void __fastcall FillSolidRect2(HDC DC, const Types::TRect &Rect, Graphics::TColor Color);
extern PACKAGE void __fastcall FillSolidRect(HDC DC, int x, int y, int cx, int cy, Graphics::TColor Color);
extern PACKAGE void __fastcall Draw3dRectEx(HDC DC, int x, int y, int cx, int cy, unsigned clrTopLeft, unsigned clrBottomRight, TElBorderSides BorderSides);
extern PACKAGE void __fastcall Draw3dBorder(HDC DC, const Types::TRect &rc, Graphics::TColor nColor1, Graphics::TColor nColor2, Graphics::TColor nColor3, Graphics::TColor nColor4);
extern PACKAGE int __fastcall RGBtoHLS(int rgbc);
extern PACKAGE int __fastcall HLStoRGB(int hlsc);
extern PACKAGE void __fastcall DrawButtonFrameEx3(HDC DC, const Types::TRect &rc, bool Focused, bool Pushed, Graphics::TColor ButtonColor, bool Thin, TElBorderSides BorderSides);
extern PACKAGE void __fastcall DrawButtonFrameEx(HDC DC, const Types::TRect &rc, bool Focused, bool Pushed, Graphics::TColor ButtonColor, bool Thin);
extern PACKAGE void __fastcall DrawButtonFrame(HDC DC, const Types::TRect &rc, bool Focused, bool Pushed);
extern PACKAGE void __fastcall DrawButtonFrameEx2(HDC DC, const Types::TRect &rc, bool Focused, bool Pushed, Graphics::TColor ButtonColor, bool Thin, Graphics::TColor clrHighlight, Graphics::TColor clrDkShadow, Graphics::TColor clrFace, Graphics::TColor clrShadow);
extern PACKAGE void __fastcall DrawFlatFrameEx2(HDC DC, const Types::TRect &R, Graphics::TColor Color, Graphics::TColor BkColor, bool Focused, bool Enabled, TElBorderSides BorderSides, TElFlatBorderType BorderType);
extern PACKAGE void __fastcall DrawFlatFrameEx(HDC DC, const Types::TRect &R, Graphics::TColor BkColor, bool Focused, bool Enabled);
extern PACKAGE Types::TRect __fastcall DrawFlatFrame(HDC DC, const Types::TRect &R, Graphics::TColor BkColor, bool Focused);
extern PACKAGE Types::TRect __fastcall DrawFlatFrame2(HDC DC, const Types::TRect &R, Graphics::TColor BkColor, bool Focused, TElBorderSides BorderSides);
extern PACKAGE void __fastcall TiledPaint(Graphics::TCanvas* Canvas, Graphics::TBitmap* Bitmap, const Types::TRect &Rect);
extern PACKAGE int __fastcall HitTest(const Types::TRect &R, const Types::TPoint &Pt, int CornerSize, int BorderSize);
extern PACKAGE Controls::TControl* __fastcall GetTopOwnerControl(Classes::TComponent* Component);
extern PACKAGE Forms::TForm* __fastcall GetOwnerForm(Classes::TComponent* Component);
extern PACKAGE void __fastcall StartWait(void);
extern PACKAGE void __fastcall StopWait(void);
extern PACKAGE void __fastcall DrawFlatScrollbarThumb(HDC DC, const Types::TRect &rc, bool Focused);
extern PACKAGE bool __fastcall AlphaBlend(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, Byte SourceConstantAlpha, Byte srcAlpha);
extern PACKAGE Types::TRect __fastcall DrawFlatScrollbars(HWND Wnd, HDC DC, const Types::TRect &Rect, bool Focused, Stdctrls::TScrollStyle ScrollBars, bool DragHorz, bool DragVert, bool IsControl, int Style, int ExStyle);
extern PACKAGE void __fastcall DrawFlatScrollBarEx(HWND Wnd, HDC DC, const Types::TRect &Rect, int nType, bool bScrollbarCtrl, bool Dragging, bool Focused, Graphics::TColor BkColor, Graphics::TColor DitherColor, Graphics::TColor ButtonColor, Graphics::TColor ArrowColor, Graphics::TColor HotButtonColor, bool DrawFrames, bool DitherBack);
extern PACKAGE void __fastcall DrawFlatScrollBarsEx(HWND Wnd, HDC DC, const Types::TRect &Rect, bool Focused, Stdctrls::TScrollStyle ScrollBars, bool DragHorz, bool DragVert, bool IsControl, Graphics::TColor BkColor, Graphics::TColor DitherColor, Graphics::TColor ButtonColor, Graphics::TColor ArrowColor, Graphics::TColor HotButtonColor, bool DrawFrames, bool DitherBack);
extern PACKAGE Types::TRect __fastcall DrawBevel(HDC DC, const Types::TRect &R, Graphics::TColor Color1, Graphics::TColor Color2, TElBorderSides Sides);
extern PACKAGE void __fastcall AlphaCopyRect(Graphics::TCanvas* DestCanvas, const Types::TRect &Dest, Graphics::TCanvas* SourceCanvas, const Types::TRect &Source, Byte AlphaLevel, bool UseAlphaLevel);
extern PACKAGE void __fastcall AlphaFillRect(Graphics::TCanvas* Canvas, const Types::TRect &Rect, Graphics::TColor Color, Byte AlphaLevel);
extern PACKAGE int __fastcall DrawTextW(HDC hDC, wchar_t * lpString, int nCount, Types::TRect &lpRect, unsigned uFormat);
extern PACKAGE HPEN __fastcall GetSysColorPen(unsigned Color);
extern PACKAGE void __fastcall DrawFocus(Graphics::TCanvas* Canvas, const Types::TRect &R);
extern PACKAGE bool __fastcall Win2KHideUIState(void);
extern PACKAGE bool __fastcall ModalFormVisible(void);
extern PACKAGE int __fastcall ShiftStateToKeyData(Classes::TShiftState Shift);
extern PACKAGE WideString __fastcall GetShortHintW(WideString Hint);

}	/* namespace Elvclutils */
using namespace Elvclutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElVCLUtils
