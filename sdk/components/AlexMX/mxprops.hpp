// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'MXProps.pas' rev: 6.00

#ifndef MXPropsHPP
#define MXPropsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mxVCLUtils.hpp>	// Pascal unit
#include <TypInfo.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxprops
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TPropInfoList;
class PASCALIMPLEMENTATION TPropInfoList : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	Typinfo::PPropInfo operator[](int Index) { return Items[Index]; }
	
private:
	Typinfo::PPropInfo *FList;
	int FCount;
	int FSize;
	Typinfo::PPropInfo __fastcall Get(int Index);
	
public:
	__fastcall TPropInfoList(System::TObject* AObject, Typinfo::TTypeKinds Filter);
	__fastcall virtual ~TPropInfoList(void);
	bool __fastcall Contains(Typinfo::PPropInfo P);
	Typinfo::PPropInfo __fastcall Find(const AnsiString AName);
	void __fastcall Delete(int Index);
	void __fastcall Intersect(TPropInfoList* List);
	__property int Count = {read=FCount, nodefault};
	__property Typinfo::PPropInfo Items[int Index] = {read=Get/*, default*/};
};


typedef AnsiString __fastcall (__closure *TReadStrEvent)(const AnsiString ASection, const AnsiString Item, const AnsiString Default);

typedef void __fastcall (__closure *TWriteStrEvent)(const AnsiString ASection, const AnsiString Item, const AnsiString Value);

typedef void __fastcall (__closure *TEraseSectEvent)(const AnsiString ASection);

class DELPHICLASS TPropsStorage;
class PASCALIMPLEMENTATION TPropsStorage : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	System::TObject* FObject;
	Classes::TComponent* FOwner;
	AnsiString FPrefix;
	AnsiString FSection;
	TReadStrEvent FOnReadString;
	TWriteStrEvent FOnWriteString;
	TEraseSectEvent FOnEraseSection;
	AnsiString __fastcall StoreIntegerProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreCharProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreEnumProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreFloatProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreStringProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreSetProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreClassProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreStringsProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreComponentProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreLStringProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreWCharProperty(Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreVariantProperty(Typinfo::PPropInfo PropInfo);
	void __fastcall LoadLStringProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadWCharProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadVariantProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	AnsiString __fastcall StoreInt64Property(Typinfo::PPropInfo PropInfo);
	void __fastcall LoadInt64Property(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadIntegerProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadCharProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadEnumProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadFloatProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadStringProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadSetProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadClassProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadStringsProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	void __fastcall LoadComponentProperty(const AnsiString S, Typinfo::PPropInfo PropInfo);
	Classes::TStrings* __fastcall CreateInfoList(Classes::TComponent* AComponent, Classes::TStrings* StoredList);
	void __fastcall FreeInfoLists(Classes::TStrings* Info);
	
protected:
	virtual AnsiString __fastcall ReadString(const AnsiString ASection, const AnsiString Item, const AnsiString Default);
	virtual void __fastcall WriteString(const AnsiString ASection, const AnsiString Item, const AnsiString Value);
	virtual void __fastcall EraseSection(const AnsiString ASection);
	virtual AnsiString __fastcall GetItemName(const AnsiString APropName);
	virtual TPropsStorage* __fastcall CreateStorage(void);
	
public:
	void __fastcall StoreAnyProperty(Typinfo::PPropInfo PropInfo);
	void __fastcall LoadAnyProperty(Typinfo::PPropInfo PropInfo);
	void __fastcall StoreProperties(Classes::TStrings* PropList);
	void __fastcall LoadProperties(Classes::TStrings* PropList);
	void __fastcall LoadObjectsProps(Classes::TComponent* AComponent, Classes::TStrings* StoredList);
	void __fastcall StoreObjectsProps(Classes::TComponent* AComponent, Classes::TStrings* StoredList);
	__property System::TObject* AObject = {read=FObject, write=FObject};
	__property AnsiString Prefix = {read=FPrefix, write=FPrefix};
	__property AnsiString Section = {read=FSection, write=FSection};
	__property TReadStrEvent OnReadString = {read=FOnReadString, write=FOnReadString};
	__property TWriteStrEvent OnWriteString = {read=FOnWriteString, write=FOnWriteString};
	__property TEraseSectEvent OnEraseSection = {read=FOnEraseSection, write=FOnEraseSection};
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TPropsStorage(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPropsStorage(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE AnsiString sPropNameDelimiter;
extern PACKAGE AnsiString __fastcall CreateStoredItem(const AnsiString CompName, const AnsiString PropName);
extern PACKAGE bool __fastcall ParseStoredItem(const AnsiString Item, AnsiString &CompName, AnsiString &PropName);
extern PACKAGE void __fastcall UpdateStoredList(Classes::TComponent* AComponent, Classes::TStrings* AStoredList, bool FromForm);

}	/* namespace Mxprops */
using namespace Mxprops;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// MXProps
