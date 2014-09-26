// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxVCLUtils.pas' rev: 6.00

#ifndef mxVCLUtilsHPP
#define mxVCLUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Dialogs.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxvclutils
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TFillDirection { fdTopToBottom, fdBottomToTop, fdLeftToRight, fdRightToLeft };
#pragma option pop

#pragma option push -b-
enum TVertAlignment { vaTopJustify, vaCenter, vaBottomJustify };
#pragma option pop

class DELPHICLASS TScreenCanvas;
class PASCALIMPLEMENTATION TScreenCanvas : public Graphics::TCanvas 
{
	typedef Graphics::TCanvas inherited;
	
private:
	HDC FDeviceContext;
	
protected:
	virtual void __fastcall CreateHandle(void);
	
public:
	__fastcall virtual ~TScreenCanvas(void);
	void __fastcall SetOrigin(int X, int Y);
	void __fastcall FreeHandle(void);
public:
	#pragma option push -w-inl
	/* TCanvas.Create */ inline __fastcall TScreenCanvas(void) : Graphics::TCanvas() { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Graphics::TColor clCream = 0xa6caf0;
static const Graphics::TColor clMoneyGreen = 0xc0dcc0;
static const Graphics::TColor clSkyBlue = 0xfffbf0;
extern PACKAGE Controls::TCursor WaitCursor;
extern PACKAGE void __fastcall ResourceNotFound(char * ResID);
extern PACKAGE Graphics::TBitmap* __fastcall MakeModuleBitmap(unsigned Module, char * ResID);
extern PACKAGE Graphics::TBitmap* __fastcall MakeBitmap(char * ResID);
extern PACKAGE Graphics::TBitmap* __fastcall MakeBitmapID(Word ResID);
extern PACKAGE void __fastcall AssignBitmapCell(Graphics::TGraphic* Source, Graphics::TBitmap* Dest, int Cols, int Rows, int Index);
extern PACKAGE void __fastcall CopyParentImage(Controls::TControl* Control, Graphics::TCanvas* Dest);
extern PACKAGE void __fastcall StretchBltTransparent(HDC DstDC, int DstX, int DstY, int DstW, int DstH, HDC SrcDC, int SrcX, int SrcY, int SrcW, int SrcH, HPALETTE Palette, unsigned TransparentColor);
extern PACKAGE void __fastcall DrawTransparentBitmap(HDC DC, HBITMAP Bitmap, int DstX, int DstY, unsigned TransparentColor);
extern PACKAGE void __fastcall StretchBitmapRectTransparent(Graphics::TCanvas* Dest, int DstX, int DstY, int DstW, int DstH, const Types::TRect &SrcRect, Graphics::TBitmap* Bitmap, Graphics::TColor TransparentColor);
extern PACKAGE void __fastcall DrawBitmapRectTransparent(Graphics::TCanvas* Dest, int DstX, int DstY, const Types::TRect &SrcRect, Graphics::TBitmap* Bitmap, Graphics::TColor TransparentColor);
extern PACKAGE void __fastcall DrawBitmapTransparent(Graphics::TCanvas* Dest, int DstX, int DstY, Graphics::TBitmap* Bitmap, Graphics::TColor TransparentColor);
extern PACKAGE Graphics::TBitmap* __fastcall ChangeBitmapColor(Graphics::TBitmap* Bitmap, Graphics::TColor Color, Graphics::TColor NewColor);
extern PACKAGE Graphics::TBitmap* __fastcall CreateDisabledBitmapEx(Graphics::TBitmap* FOriginal, Graphics::TColor OutlineColor, Graphics::TColor BackColor, Graphics::TColor HighlightColor, Graphics::TColor ShadowColor, bool DrawHighlight);
extern PACKAGE Graphics::TBitmap* __fastcall CreateDisabledBitmap(Graphics::TBitmap* FOriginal, Graphics::TColor OutlineColor);
extern PACKAGE void __fastcall ImageListDrawDisabled(Controls::TImageList* Images, Graphics::TCanvas* Canvas, int X, int Y, int Index, Graphics::TColor HighlightColor, Graphics::TColor GrayColor, bool DrawHighlight);
extern PACKAGE Graphics::TBitmap* __fastcall CreateTwoColorsBrushPattern(Graphics::TColor Color1, Graphics::TColor Color2);
extern PACKAGE Graphics::TIcon* __fastcall MakeIcon(char * ResID);
extern PACKAGE Graphics::TIcon* __fastcall MakeIconID(Word ResID);
extern PACKAGE Graphics::TIcon* __fastcall MakeModuleIcon(unsigned Module, char * ResID);
extern PACKAGE Graphics::TBitmap* __fastcall CreateBitmapFromIcon(Graphics::TIcon* Icon, Graphics::TColor BackColor);
extern PACKAGE Graphics::TIcon* __fastcall CreateIconFromBitmap(Graphics::TBitmap* Bitmap, Graphics::TColor TransparentColor);
extern PACKAGE Word __fastcall DialogUnitsToPixelsX(Word DlgUnits);
extern PACKAGE Word __fastcall DialogUnitsToPixelsY(Word DlgUnits);
extern PACKAGE Word __fastcall PixelsToDialogUnitsX(Word PixUnits);
extern PACKAGE Word __fastcall PixelsToDialogUnitsY(Word PixUnits);
extern PACKAGE unsigned __fastcall LoadDLL(const AnsiString LibName);
extern PACKAGE bool __fastcall RegisterServer(const AnsiString ModuleName);
extern PACKAGE void __fastcall Beep(void);
extern PACKAGE void __fastcall FreeUnusedOle(void);
extern PACKAGE void __fastcall NotImplemented(void);
extern PACKAGE void __fastcall PaintInverseRect(const Types::TPoint &RectOrg, const Types::TPoint &RectEnd);
extern PACKAGE void __fastcall DrawInvertFrame(const Types::TRect &ScreenRect, int Width);
extern PACKAGE int __fastcall WidthOf(const Types::TRect &R);
extern PACKAGE int __fastcall HeightOf(const Types::TRect &R);
extern PACKAGE bool __fastcall PointInRect(const Types::TPoint &P, const Types::TRect &R);
extern PACKAGE bool __fastcall PointInPolyRgn(const Types::TPoint &P, const Types::TPoint * Points, const int Points_Size);
extern PACKAGE int __fastcall PaletteColor(Graphics::TColor Color);
extern PACKAGE void __fastcall KillMessage(HWND Wnd, unsigned Msg);
extern PACKAGE HFONT __fastcall CreateRotatedFont(Graphics::TFont* Font, int Angle);
extern PACKAGE void __fastcall Delay(int MSecs);
extern PACKAGE int __fastcall PaletteEntries(HPALETTE Palette);
extern PACKAGE void __fastcall CenterControl(Controls::TControl* Control);
extern PACKAGE void __fastcall CenterWindow(HWND Wnd);
extern PACKAGE void __fastcall MergeForm(Controls::TWinControl* AControl, Forms::TForm* AForm, Controls::TAlign Align, bool Show);
extern PACKAGE void __fastcall ShowMDIClientEdge(unsigned ClientHandle, bool ShowEdge);
extern PACKAGE Variant __fastcall MakeVariant(const Variant * Values, const int Values_Size);
extern PACKAGE void __fastcall ShadeRect(HDC DC, const Types::TRect &Rect);
extern PACKAGE Types::TRect __fastcall ScreenWorkArea();
extern PACKAGE AnsiString __fastcall WindowClassName(HWND Wnd);
extern PACKAGE void __fastcall ShowWinNoAnimate(HWND Handle, int CmdShow);
extern PACKAGE void __fastcall SwitchToWindow(HWND Wnd, bool Restore);
extern PACKAGE void __fastcall ActivateWindow(HWND Wnd);
extern PACKAGE HWND __fastcall FindPrevInstance(const System::ShortString &MainFormClass, const AnsiString ATitle);
extern PACKAGE bool __fastcall ActivatePrevInstance(const System::ShortString &MainFormClass, const AnsiString ATitle);
extern PACKAGE int __fastcall MsgBox(const AnsiString Caption, const AnsiString Text, int Flags);
extern PACKAGE Word __fastcall MsgDlg(const AnsiString Msg, Dialogs::TMsgDlgType AType, Dialogs::TMsgDlgButtons AButtons, int HelpCtx);
extern PACKAGE void __fastcall GradientFillRect(Graphics::TCanvas* Canvas, const Types::TRect &ARect, Graphics::TColor StartColor, Graphics::TColor EndColor, TFillDirection Direction, Byte Colors);
extern PACKAGE AnsiString __fastcall MinimizeText(const AnsiString Text, Graphics::TCanvas* Canvas, int MaxWidth);
extern PACKAGE Types::TPoint __fastcall GetAveCharSize(Graphics::TCanvas* Canvas);
extern PACKAGE void * __fastcall AllocMemo(int Size);
extern PACKAGE void * __fastcall ReallocMemo(void * fpBlock, int Size);
extern PACKAGE void __fastcall FreeMemo(void * &fpBlock);
extern PACKAGE int __fastcall GetMemoSize(void * fpBlock);
extern PACKAGE bool __fastcall CompareMem(void * fpBlock1, void * fpBlock2, unsigned Size);
extern PACKAGE void __fastcall HugeInc(void * &HugePtr, int Amount);
extern PACKAGE void __fastcall HugeDec(void * &HugePtr, int Amount);
extern PACKAGE void * __fastcall HugeOffset(void * HugePtr, int Amount);
extern PACKAGE void __fastcall HMemCpy(void * DstPtr, void * SrcPtr, int Amount);
extern PACKAGE void __fastcall HugeMove(void * Base, int Dst, int Src, int Size);
extern PACKAGE AnsiString __fastcall GetEnvVar(const AnsiString VarName);
extern PACKAGE void __fastcall SplitCommandLine(const AnsiString CmdLine, AnsiString &ExeName, AnsiString &Params);
extern PACKAGE AnsiString __fastcall AnsiUpperFirstChar(const AnsiString S);
extern PACKAGE char * __fastcall StrPAlloc(const AnsiString S);
extern PACKAGE char * __fastcall StringToPChar(AnsiString &S);
extern PACKAGE AnsiString __fastcall DropT(const AnsiString S);
extern PACKAGE HICON __fastcall LoadAniCursor(unsigned Instance, char * ResID);
extern PACKAGE Controls::TCursor __fastcall DefineCursor(unsigned Instance, char * ResID);
extern PACKAGE void __fastcall StartWait(void);
extern PACKAGE void __fastcall StopWait(void);
extern PACKAGE void __fastcall WriteText(Graphics::TCanvas* ACanvas, const Types::TRect &ARect, int DX, int DY, const AnsiString Text, Classes::TAlignment Alignment, bool WordWrap, bool ARightToLeft = false);
extern PACKAGE void __fastcall DrawCellTextEx(Controls::TCustomControl* Control, int ACol, int ARow, const AnsiString S, const Types::TRect &ARect, Classes::TAlignment Align, TVertAlignment VertAlign, bool WordWrap, bool ARightToLeft)/* overload */;
extern PACKAGE void __fastcall DrawCellText(Controls::TCustomControl* Control, int ACol, int ARow, const AnsiString S, const Types::TRect &ARect, Classes::TAlignment Align, TVertAlignment VertAlign, bool ARightToLeft)/* overload */;
extern PACKAGE void __fastcall DrawCellTextEx(Controls::TCustomControl* Control, int ACol, int ARow, const AnsiString S, const Types::TRect &ARect, Classes::TAlignment Align, TVertAlignment VertAlign, bool WordWrap)/* overload */;
extern PACKAGE void __fastcall DrawCellText(Controls::TCustomControl* Control, int ACol, int ARow, const AnsiString S, const Types::TRect &ARect, Classes::TAlignment Align, TVertAlignment VertAlign)/* overload */;
extern PACKAGE void __fastcall DrawCellBitmap(Controls::TCustomControl* Control, int ACol, int ARow, Graphics::TGraphic* Bmp, const Types::TRect &Rect);
extern PACKAGE void __fastcall RaiseWin32Error(unsigned ErrorCode);
extern PACKAGE bool __fastcall CheckWin32(bool OK);
extern PACKAGE AnsiString __fastcall ResStr(const AnsiString Ident);
extern PACKAGE bool __fastcall IsForegroundTask(void);
extern PACKAGE AnsiString __fastcall GetWindowsVersion();

}	/* namespace Mxvclutils */
using namespace Mxvclutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxVCLUtils
