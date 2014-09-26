// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDragDrop.pas' rev: 6.00

#ifndef ElDragDropHPP
#define ElDragDropHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHook.hpp>	// Pascal unit
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <ElCBFmts.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ActiveX.hpp>	// Pascal unit
#include <Clipbrd.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------
DECLARE_DINTERFACE_TYPE(IDropTarget)
DECLARE_DINTERFACE_TYPE(IDropSource)
DECLARE_DINTERFACE_TYPE(IEnumFORMATETC)

namespace Eldragdrop
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TDragType { dtCopy, dtMove, dtLink, dtNone };
#pragma option pop

typedef Set<TDragType, dtCopy, dtNone>  TDragTypes;

#pragma option push -b-
enum TDragResult { drDropCopy, drDropMove, drDropLink, drCancel, drOutMemory, drUnknown };
#pragma option pop

#pragma option push -b-
enum TDragContent { edcText, edcBitmap, edcMetafile, edcFileList, edcOther };
#pragma option pop

class DELPHICLASS TOleDragObject;
typedef void __fastcall (__closure *TTargetDragEvent)(System::TObject* Sender, Controls::TDragState State, TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, TDragType &DragType);

typedef void __fastcall (__closure *TTargetDropEvent)(System::TObject* Sender, TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, TDragType &DragType);

typedef void __fastcall (__closure *TSourceDragEvent)(System::TObject* Sender, TDragType DragType, Classes::TShiftState shift, bool &ContinueDrop);

typedef void __fastcall (__closure *TSourceDropEvent)(System::TObject* Sender, TDragResult DragResult);

typedef void __fastcall (__closure *TOleStartDragEvent)(System::TObject* Sender, void * &DragData, int &DragDataType, int &DragDataSize);

typedef tagFORMATETC TFormatList[256];

typedef tagFORMATETC *pFormatList;

class DELPHICLASS TEnumFormatEtc;
class PASCALIMPLEMENTATION TEnumFormatEtc : public System::TInterfacedObject 
{
	typedef System::TInterfacedObject inherited;
	
private:
	tagFORMATETC *fFormatList;
	int fFormatCount;
	int fIndex;
	
public:
	__fastcall TEnumFormatEtc(pFormatList FormatList, int FormatCount, int Index);
	HRESULT __stdcall Next(int Celt, /* out */ void *Elt, PLongint pCeltFetched);
	HRESULT __stdcall Skip(int Celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(/* out */ _di_IEnumFORMATETC &Enum);
	__fastcall virtual ~TEnumFormatEtc(void);
private:
	void *__IEnumFORMATETC;	/* IEnumFORMATETC */
	
public:
	operator IEnumFORMATETC*(void) { return (IEnumFORMATETC*)&__IEnumFORMATETC; }
	
};


class PASCALIMPLEMENTATION TOleDragObject : public Controls::TDragObject 
{
	typedef Controls::TDragObject inherited;
	
private:
	_di_IDataObject dataObj;
	int Fkeys;
	bool FDown;
	AnsiString FString;
	Classes::TStringList* FList;
	
protected:
	Classes::TStringList* __fastcall GetFileList(void);
	AnsiString __fastcall GetString();
	Graphics::TBitmap* __fastcall GetBitmap(void);
	TDragContent __fastcall GetDragContent(void);
	__property int Keys = {read=Fkeys, nodefault};
	
public:
	__fastcall TOleDragObject(void);
	__fastcall virtual ~TOleDragObject(void);
	void * __fastcall GetFormatData(int Format);
	bool __fastcall HasDataFormat(int Format);
	_di_IDataObject __fastcall DataObject();
	__property TDragContent Content = {read=GetDragContent, nodefault};
	__property Classes::TStringList* FileList = {read=GetFileList};
	__property AnsiString StringData = {read=GetString};
	__property Graphics::TBitmap* Bitmap = {read=GetBitmap};
};


class DELPHICLASS IElDropSource;
class DELPHICLASS TElDragDrop;
class PASCALIMPLEMENTATION TElDragDrop : public Elxpthemedcontrol::TElXPThemedControl 
{
	typedef Elxpthemedcontrol::TElXPThemedControl inherited;
	
private:
	Controls::TCursor FOldCursor;
	bool FPressed;
	TTargetDropEvent FOnTargetDrop;
	TTargetDragEvent FOnTargetDrag;
	Classes::TNotifyEvent FOnPaint;
	bool FIsDropSource;
	bool FIsDropTarget;
	Graphics::TPicture* FPicture;
	bool FAutoSize;
	int FContentLength;
	void *FContent;
	int FContentType;
	TOleStartDragEvent FOnOleStartDrag;
	TSourceDragEvent FOnSourceDrag;
	TSourceDropEvent FOnSourceDrop;
	_di_IDropTarget FElDropTarget;
	_di_IDropSource FElDropSource;
	TDragTypes FDragTypes;
	Classes::TStrings* FDataFormats;
	void __fastcall SetContentType(int newValue);
	void __fastcall SetContentLength(int newValue);
	void __fastcall SetPicture(Graphics::TPicture* newValue);
	void __fastcall SetIsDropTarget(bool newValue);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMLButtonUp(Messages::TWMMouse &Msg);
	HIDESBASE MESSAGE void __fastcall CMMouseLeave(Messages::TMessage &Msg);
	
protected:
	virtual void __fastcall SetAutoSize(bool newValue);
	virtual void __fastcall TriggerSourceDropEvent(TDragResult DragResult);
	virtual void __fastcall TriggerOleStartDragEvent(void * &DragData, int &DragDataType, int &DragDataSize);
	virtual void __fastcall TriggerSourceDragEvent(TDragType DragType, Classes::TShiftState Shift, bool &ContinueDrop);
	virtual void __fastcall Paint(void);
	virtual void __fastcall OnPictureChange(System::TObject* Sender);
	virtual void __fastcall TriggerPaintEvent(void);
	virtual void __fastcall TriggerTargetDragEvent(Controls::TDragState State, TOleDragObject* Source, Classes::TShiftState Shift, int X, int Y, TDragType &DragType);
	virtual void __fastcall TriggerTargetDropEvent(Classes::TShiftState Shift, TOleDragObject* Source, int X, int Y, TDragType &DragType);
	MESSAGE void __fastcall WMCreate(Messages::TMessage &Message);
	virtual WideString __fastcall GetThemedClassName();
	
public:
	__fastcall virtual TElDragDrop(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDragDrop(void);
	__property void * Content = {read=FContent};
	__property int ContentType = {read=FContentType, write=SetContentType, nodefault};
	__property int ContentLength = {read=FContentLength, write=SetContentLength, nodefault};
	__property Classes::TStrings* DataFormats = {read=FDataFormats};
	__property Canvas ;
	
__published:
	__property DragCursor  = {default=-12};
	__property Visible  = {default=1};
	__property Enabled  = {default=1};
	__property TDragTypes DragTypes = {read=FDragTypes, write=FDragTypes, nodefault};
	__property TOleStartDragEvent OnOleStartDrag = {read=FOnOleStartDrag, write=FOnOleStartDrag};
	__property TSourceDragEvent OnOleSourceDrag = {read=FOnSourceDrag, write=FOnSourceDrag};
	__property TSourceDropEvent OnSourceDrop = {read=FOnSourceDrop, write=FOnSourceDrop};
	__property Graphics::TPicture* Picture = {read=FPicture, write=SetPicture};
	__property bool AutoSize = {read=FAutoSize, write=SetAutoSize, nodefault};
	__property bool IsDropSource = {read=FIsDropSource, write=FIsDropSource, nodefault};
	__property bool IsDropTarget = {read=FIsDropTarget, write=SetIsDropTarget, nodefault};
	__property UseXPThemes  = {default=1};
	__property Classes::TNotifyEvent OnPaint = {read=FOnPaint, write=FOnPaint};
	__property TTargetDragEvent OnTargetDrag = {read=FOnTargetDrag, write=FOnTargetDrag};
	__property TTargetDropEvent OnTargetDrop = {read=FOnTargetDrop, write=FOnTargetDrop};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDragDrop(HWND ParentWindow) : Elxpthemedcontrol::TElXPThemedControl(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION IElDropSource : public System::TInterfacedObject 
{
	typedef System::TInterfacedObject inherited;
	
private:
	TElDragDrop* FOwner;
	
protected:
	HRESULT __stdcall QueryContinueDrag(BOOL FEscapePressed, int GrfKeyState);
	HRESULT __stdcall GiveFeedback(int dwEffect);
	HRESULT __stdcall GetData(const tagFORMATETC &FormatEtcIn, /* out */ tagSTGMEDIUM &Medium);
	HRESULT __stdcall GetDataHere(const tagFORMATETC &FormatEtcIn, /* out */ tagSTGMEDIUM &Medium);
	HRESULT __stdcall QueryGetData(const tagFORMATETC &FormatEtc);
	HRESULT __stdcall GetCanonicalFormatEtc(const tagFORMATETC &FormatEtc, /* out */ tagFORMATETC &FormatEtcOut);
	HRESULT __stdcall SetData(const tagFORMATETC &FormatEtc, tagSTGMEDIUM &Medium, BOOL fRelease);
	HRESULT __stdcall EnumFormatEtc(int dwDirection, /* out */ _di_IEnumFORMATETC &EnumFormatEtc);
	HRESULT __stdcall dAdvise(const tagFORMATETC &FormatEtc, int advf, const _di_IAdviseSink advsink, /* out */ int &dwConnection);
	HRESULT __stdcall dUnadvise(int dwConnection);
	HRESULT __stdcall EnumdAdvise(/* out */ _di_IEnumSTATDATA &EnumAdvise);
	__fastcall IElDropSource(TElDragDrop* aOwner);
	
public:
	__fastcall virtual ~IElDropSource(void);
private:
	void *__IDropSource;	/* IDropSource */
	void *__IDataObject;	/* IDataObject */
	
public:
	operator IDataObject*(void) { return (IDataObject*)&__IDataObject; }
	operator IDropSource*(void) { return (IDropSource*)&__IDropSource; }
	
};


class DELPHICLASS IElDropTarget;
class PASCALIMPLEMENTATION IElDropTarget : public System::TInterfacedObject 
{
	typedef System::TInterfacedObject inherited;
	
private:
	TElDragDrop* FOwner;
	TOleDragObject* FdragObj;
	
public:
	HRESULT __stdcall DragEnter(const _di_IDataObject dataObj, int grfKeyState, const Types::TPoint pt, int &dwEffect);
	HRESULT __stdcall DragOver(int grfKeyState, const Types::TPoint pt, int &dwEffect);
	HRESULT __stdcall DragLeave(void);
	HRESULT __stdcall Drop(const _di_IDataObject dataObj, int grfKeyState, const Types::TPoint pt, int &dwEffect);
	__fastcall IElDropTarget(TElDragDrop* aOwner);
	__fastcall virtual ~IElDropTarget(void);
private:
	void *__IDropTarget;	/* IDropTarget */
	
public:
	operator IDropTarget*(void) { return (IDropTarget*)&__IDropTarget; }
	
};


class DELPHICLASS TElDropTarget;
class PASCALIMPLEMENTATION TElDropTarget : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	int FRefCount;
	Controls::TWinControl* FTarget;
	TOleDragObject* FDragObj;
	TTargetDropEvent FOnTargetDrop;
	TTargetDragEvent FOnTargetDrag;
	Classes::TStrings* FDataFormats;
	Elhook::TElHook* FHook;
	void __fastcall SetTarget(Controls::TWinControl* newValue);
	void __fastcall AfterMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	void __fastcall BeforeMessage(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);
	
protected:
	virtual HRESULT __stdcall QueryInterface(const GUID &IID, /* out */ void *Obj);
	HIDESBASE int __stdcall _AddRef(void);
	HIDESBASE int __stdcall _Release(void);
	HRESULT __stdcall DragEnter(const _di_IDataObject dataObj, int grfKeyState, const Types::TPoint pt, int &dwEffect);
	HRESULT __stdcall DragOver(int grfKeyState, const Types::TPoint pt, int &dwEffect);
	HRESULT __stdcall DragLeave(void);
	HRESULT __stdcall Drop(const _di_IDataObject dataObj, int grfKeyState, const Types::TPoint pt, int &dwEffect);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TElDropTarget(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDropTarget(void);
	__property Classes::TStrings* DataFormats = {read=FDataFormats};
	bool __fastcall HasDataFormat(int Format);
	
__published:
	__property TTargetDragEvent OnTargetDrag = {read=FOnTargetDrag, write=FOnTargetDrag};
	__property TTargetDropEvent OnTargetDrop = {read=FOnTargetDrop, write=FOnTargetDrop};
	__property Controls::TWinControl* Target = {read=FTarget, write=SetTarget};
private:
	void *__IDropTarget;	/* IDropTarget */
	
public:
	operator IDropTarget*(void) { return (IDropTarget*)&__IDropTarget; }
	operator IInterface*(void) { return (IInterface*)&__IDropTarget; }
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldragdrop */
using namespace Eldragdrop;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDragDrop
