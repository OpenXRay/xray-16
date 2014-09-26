// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElUxTheme.pas' rev: 6.00

#ifndef ElUxThemeHPP
#define ElUxThemeHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTmSchema.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eluxtheme
{
//-- type declarations -------------------------------------------------------
#pragma pack(push, 4)
struct TMargins
{
	int cxLeftWidth;
	int cxRightWidth;
	int cyTopHeight;
	int cyBottomHeight;
} ;
#pragma pack(pop)

typedef TMargins *PMargins;

#pragma pack(push, 4)
struct TIntList
{
	int iValueCount;
	int iValues[10];
} ;
#pragma pack(pop)

typedef TIntList *PIntList;

#pragma pack(push, 4)
struct TThemeErrorContext
{
	unsigned dwSize;
	HRESULT hr;
	wchar_t szMsgParam1[260];
	wchar_t szMsgParam2[260];
	wchar_t szFileName[260];
	wchar_t szSourceLine[260];
	int iLineNum;
} ;
#pragma pack(pop)

typedef TThemeErrorContext *PThemeErrorContext;

typedef unsigned HTheme;

typedef bool __stdcall (*IsThemeActiveProc)(void);

typedef HRESULT __stdcall (*EnableThemingProc)(bool fEnable);

typedef unsigned __stdcall (*OpenThemeDataProc)(HWND hwnd, wchar_t * pszClassList);

typedef HRESULT __stdcall (*CloseThemeDataProc)(unsigned Theme);

typedef HRESULT __stdcall (*DrawThemeParentBackgroundProc)(HWND hwnd, HDC hdc, const Types::TRect &Rect);

typedef HRESULT __stdcall (*DrawThemeBackgroundProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, const Types::TRect &Rect, Types::PRect pClipRect);

typedef HRESULT __stdcall (*DrawThemeTextProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, wchar_t * pszText, int iCharCount, unsigned dwTextFlags, unsigned dwTextFlags2, Types::TRect &Rect);

typedef HRESULT __stdcall (*GetThemeBackgroundContentRectProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, const Types::TRect &pBoundingRect, Types::TRect &pContentRect);

typedef HRESULT __stdcall (*GetThemeBackgroundExtentProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, Types::PRect pBoundingRect, Types::TRect &pContentRect);

typedef HRESULT __stdcall (*GetThemePartSizeProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, Types::PRect rect, int eSize, tagSIZE &psz);

typedef HRESULT __stdcall (*GetThemeTextExtentProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, wchar_t * pszText, int iCharCount, unsigned dwTextFlags, Types::PRect pBoundingRect, Types::TRect &pExtentRect);

typedef HRESULT __stdcall (*GetThemeTextMetricsProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, tagTEXTMETRICA &ptm);

typedef HRESULT __stdcall (*GetThemeBackgroundRegionProc)(unsigned Theme, int iPartId, int iStateId, Types::PRect pRect, HRGN &pRegion);

typedef HRESULT __stdcall (*HitTestThemeBackgroundProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, unsigned dwOptions, Types::PRect pRect, HRGN hrgn, const Types::TPoint ptTest, Word &pwHitTestCode);

typedef HRESULT __stdcall (*DrawThemeEdgeProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, const Types::TRect &pDestRect, unsigned uEdge, unsigned uFlags, Types::PRect pContentRect);

typedef HRESULT __stdcall (*DrawThemeIconProc)(unsigned Theme, HDC hdc, int iPartId, int iStateId, Types::PRect pRect, unsigned himl, int iImageIndex);

typedef bool __stdcall (*IsThemePartDefinedProc)(unsigned Theme, int iPartId, int iStateId);

typedef bool __stdcall (*IsThemeBackgroundPartiallyTransparentProc)(unsigned Theme, int iPartId, int iStateId);

typedef HRESULT __stdcall (*GetThemeColorProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, unsigned &Color);

typedef HRESULT __stdcall (*GetThemeMetricProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, int &piVal);

typedef HRESULT __stdcall (*GetThemeStringProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, wchar_t * pszBuff, int cchMaxBuffChars);

typedef HRESULT __stdcall (*GetThemeBoolProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, BOOL &pfVal);

typedef HRESULT __stdcall (*GetThemeIntProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, int &pfVal);

typedef HRESULT __stdcall (*GetThemeEnumValueProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, int &pfVal);

typedef HRESULT __stdcall (*GetThemePositionProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, Types::TPoint &pPoint);

typedef HRESULT __stdcall (*GetThemeFontProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, tagLOGFONTA &pPoint);

typedef HRESULT __stdcall (*GetThemeRectProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, Types::TRect &pRect);

typedef HRESULT __stdcall (*GetThemeMarginsProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, TMargins &Margins);

typedef HRESULT __stdcall (*GetThemeIntListProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, TIntList &pIntList);

typedef HRESULT __stdcall (*GetThemePropertyOriginProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, int &pOrigin);

typedef HRESULT __stdcall (*SetWindowThemeProc)(HWND hwnd, wchar_t * pszSubAppName, wchar_t * pszSubIdList);

typedef HRESULT __stdcall (*GetThemeFilenameProc)(unsigned Theme, int iPartId, int iStateId, int iPropId, wchar_t * pszThemeFileName, int cchMaxBuffChars);

typedef unsigned __stdcall (*GetThemeSysColorProc)(unsigned Theme, int iColorId);

typedef HBRUSH __stdcall (*GetThemeSysColorBrushProc)(unsigned Theme, int iColorId);

typedef int __stdcall (*GetThemeSysSizeProc)(unsigned Theme, int iSizeId);

typedef int __stdcall (*GetThemeSysBoolProc)(unsigned Theme, int iBoolId);

typedef HRESULT __stdcall (*GetThemeSysFontProc)(unsigned Theme, int iFontId, tagLOGFONTA &plf);

typedef HRESULT __stdcall (*GetThemeSysStringProc)(unsigned Theme, int iStringId, wchar_t * pszStringBuff, int cchMaxStringChars);

typedef HRESULT __stdcall (*GetThemeSysIntProc)(unsigned Theme, int iIntId, int &piValue);

typedef bool __stdcall (*IsAppThemedProc)(void);

typedef unsigned __stdcall (*GetWindowThemeProc)(HWND hwnd);

typedef HRESULT __stdcall (*EnableThemeDialogTextureProc)(HWND hwnd, BOOL fEnable);

typedef bool __stdcall (*IsThemeDialogTextureEnabledProc)(HWND hwnd);

typedef unsigned __stdcall (*GetThemeAppPropertiesProc)(void);

typedef void __stdcall (*SetThemeAppPropertiesProc)(unsigned dwFlags);

typedef HRESULT __stdcall (*GetCurrentThemeNameProc)(wchar_t * pszThemeFileName, int cchMaxNameChars, wchar_t * pszColorBuff, int cchMaxColorChars, wchar_t * pszSizeBuff, int cchMaxSizeChars);

typedef HRESULT __stdcall (*GetThemeDocumentationPropertyProc)(wchar_t * pszThemeName, wchar_t * pszPropertyName, wchar_t * pszValueBuff, int cchMaxValChars);

//-- var, const, procedure ---------------------------------------------------
static const Shortint DTT_GRAYED = 0x1;
static const Shortint HTTB_BACKGROUNDSEG = 0x0;
static const Shortint HTTB_FIXEDBORDER = 0x2;
static const Shortint HTTB_CAPTION = 0x4;
static const Shortint HTTB_RESIZINGBORDER_LEFT = 0x10;
static const Shortint HTTB_RESIZINGBORDER_TOP = 0x20;
static const Shortint HTTB_RESIZINGBORDER_RIGHT = 0x40;
static const Byte HTTB_RESIZINGBORDER_BOTTOM = 0x80;
static const Byte HTTB_RESIZINGBORDER = 0xf0;
static const Word HTTB_USESIZINGTEMPLATE = 0x100;
static const Shortint DTL_LEFT = 0x1;
static const Shortint DTL_TOP = 0x2;
static const Shortint DTL_RIGHT = 0x4;
static const Shortint DTL_BOTTOM = 0x8;
static const Shortint MAX_INTLIST_COUNT = 0xa;
static const Shortint PO_STATE = 0x0;
static const Shortint PO_PART = 0x1;
static const Shortint PO_CLASS = 0x2;
static const Shortint PO_GLOBAL = 0x3;
static const Shortint PO_NOTFOUND = 0x4;
static const Shortint STAP_ALLOW_NONCLIENT = 0x1;
static const Shortint STAP_ALLOW_CONTROLS = 0x2;
static const Shortint STAP_ALLOW_WEBCONTENT = 0x4;
#define SZ_THDOCPROP_DISPLAYNAME "DisplayName"
#define SZ_THDOCPROP_CANONICALNAME "ThemeName"
#define SZ_THDOCPROP_TOOLTIP "ToolTip"
#define SZ_THDOCPROP_AUTHOR "author"
static const Word WM_THEMECHANGED = 0x31a;
static const Shortint TS_MIN = 0x0;
static const Shortint TS_TRUE = 0x1;
static const Shortint TS_DRAW = 0x2;
extern PACKAGE IsThemeActiveProc IsThemeActive;
extern PACKAGE EnableThemingProc EnableTheming;
extern PACKAGE OpenThemeDataProc OpenThemeData;
extern PACKAGE CloseThemeDataProc CloseThemeData;
extern PACKAGE DrawThemeParentBackgroundProc DrawThemeParentBackground;
extern PACKAGE DrawThemeBackgroundProc DrawThemeBackground;
extern PACKAGE DrawThemeTextProc DrawThemeText;
extern PACKAGE GetThemeBackgroundContentRectProc GetThemeBackgroundContentRect;
extern PACKAGE GetThemeBackgroundExtentProc GetThemeBackgroundExtent;
extern PACKAGE GetThemePartSizeProc GetThemePartSize;
extern PACKAGE GetThemeTextExtentProc GetThemeTextExtent;
extern PACKAGE GetThemeTextMetricsProc GetThemeTextMetrics;
extern PACKAGE GetThemeBackgroundRegionProc GetThemeBackgroundRegion;
extern PACKAGE HitTestThemeBackgroundProc HitTestThemeBackground;
extern PACKAGE DrawThemeEdgeProc DrawThemeEdge;
extern PACKAGE DrawThemeIconProc DrawThemeIcon;
extern PACKAGE IsThemePartDefinedProc IsThemePartDefined;
extern PACKAGE IsThemeBackgroundPartiallyTransparentProc IsThemeBackgroundPartiallyTransparent;
extern PACKAGE GetThemeColorProc GetThemeColor;
extern PACKAGE GetThemeMetricProc GetThemeMetric;
extern PACKAGE GetThemeStringProc GetThemeString;
extern PACKAGE GetThemeBoolProc GetThemeBool;
extern PACKAGE GetThemeIntProc GetThemeInt;
extern PACKAGE GetThemeEnumValueProc GetThemeEnumValue;
extern PACKAGE GetThemePositionProc GetThemePosition;
extern PACKAGE GetThemeFontProc GetThemeFont;
extern PACKAGE GetThemeRectProc GetThemeRect;
extern PACKAGE GetThemeMarginsProc GetThemeMargins;
extern PACKAGE GetThemeIntListProc GetThemeIntList;
extern PACKAGE GetThemePropertyOriginProc GetThemePropertyOrigin;
extern PACKAGE SetWindowThemeProc SetWindowTheme;
extern PACKAGE GetThemeFilenameProc GetThemeFilename;
extern PACKAGE GetThemeSysColorProc GetThemeSysColor;
extern PACKAGE GetThemeSysColorBrushProc GetThemeSysColorBrush;
extern PACKAGE GetThemeSysSizeProc GetThemeSysSize;
extern PACKAGE GetThemeSysBoolProc GetThemeSysBool;
extern PACKAGE GetThemeSysFontProc GetThemeSysFont;
extern PACKAGE GetThemeSysStringProc GetThemeSysString;
extern PACKAGE GetThemeSysIntProc GetThemeSysInt;
extern PACKAGE IsAppThemedProc IsAppThemed;
extern PACKAGE GetWindowThemeProc GetWindowTheme;
extern PACKAGE EnableThemeDialogTextureProc EnableThemeDialogTexture;
extern PACKAGE IsThemeDialogTextureEnabledProc IsThemeDialogTextureEnabled;
extern PACKAGE GetThemeAppPropertiesProc GetThemeAppProperties;
extern PACKAGE SetThemeAppPropertiesProc SetThemeAppProperties;
extern PACKAGE GetCurrentThemeNameProc GetCurrentThemeName;
extern PACKAGE GetThemeDocumentationPropertyProc GetThemeDocumentationProperty;
extern PACKAGE bool ThemesAvailable;
extern PACKAGE HRESULT __fastcall GetThemePartSizeTo(wchar_t * pszClassList, HDC hdc, int iPartId, int iStateId, Types::PRect rect, int eSize, tagSIZE &psz);
extern PACKAGE HRESULT __fastcall DrawThemeTextTo(wchar_t * pszClassList, HDC hdc, int iPartId, int iStateId, wchar_t * pszText, int iCharCount, unsigned dwTextFlags, unsigned dwTextFlags2, Types::TRect &Rect);
extern PACKAGE HRESULT __fastcall DrawThemeBackgroundTo(wchar_t * pszClassList, HDC hdc, int iPartId, int iStateId, const Types::TRect &pRect, Types::PRect pClipRect);

}	/* namespace Eluxtheme */
using namespace Eluxtheme;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElUxTheme
