// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElIni.pas' rev: 6.00

#ifndef ElIniHPP
#define ElIniHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <ElRegUtils.hpp>	// Pascal unit
#include <Registry.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elini
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TElValueType { evtUnknown, evtBoolean, evtInt, evtString, evtMultiString, evtBinary, evtDouble };
#pragma option pop

struct TElValueData
{
	
	TElValueType ValueType;
	union
	{
		struct 
		{
			double DoubleValue;
			
		};
		struct 
		{
			void *DataValue;
			int DataLength;
			
		};
		struct 
		{
			Classes::TStringList* MStrValue;
			
		};
		struct 
		{
			char *StrValue;
			
		};
		struct 
		{
			int IntValue;
			
		};
		struct 
		{
			bool BoolValue;
			
		};
		
	};
} ;

typedef TElValueData *PELValueData;

class DELPHICLASS EElIniError;
class PASCALIMPLEMENTATION EElIniError : public Sysutils::Exception 
{
	typedef Sysutils::Exception inherited;
	
public:
	#pragma option push -w-inl
	/* Exception.Create */ inline __fastcall EElIniError(const AnsiString Msg) : Sysutils::Exception(Msg) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmt */ inline __fastcall EElIniError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateRes */ inline __fastcall EElIniError(int Ident)/* overload */ : Sysutils::Exception(Ident) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmt */ inline __fastcall EElIniError(int Ident, const System::TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateHelp */ inline __fastcall EElIniError(const AnsiString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateFmtHelp */ inline __fastcall EElIniError(const AnsiString Msg, const System::TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResHelp */ inline __fastcall EElIniError(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext) { }
	#pragma option pop
	#pragma option push -w-inl
	/* Exception.CreateResFmtHelp */ inline __fastcall EElIniError(System::PResStringRec ResStringRec, const System::TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::Exception(ResStringRec, Args, Args_Size, AHelpContext) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~EElIniError(void) { }
	#pragma option pop
	
};


class DELPHICLASS TElIniEntry;
class PASCALIMPLEMENTATION TElIniEntry : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	Ellist::TElList* FChildren;
	bool FIsKey;
	TElIniEntry* FParent;
	TElValueData FValueData;
	AnsiString FValueName;
	int __fastcall GetSubCount(void);
	void __fastcall OnValueDelete(System::TObject* Sender, void * Data);
	void __fastcall SetParent(TElIniEntry* Value);
	
protected:
	__fastcall TElIniEntry(void);
	TElIniEntry* __fastcall GetValue(AnsiString Name);
	__property TElIniEntry* Parent = {read=FParent, write=SetParent};
	
public:
	__fastcall virtual ~TElIniEntry(void);
	void __fastcall Invalidate(void);
	__property bool IsKey = {read=FIsKey, nodefault};
	__property int SubCount = {read=GetSubCount, nodefault};
};


class DELPHICLASS TElIniFile;
class PASCALIMPLEMENTATION TElIniFile : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	Classes::TNotifyEvent FOnBeforeLoad;
	Classes::TNotifyEvent FOnAfterSave;
	Classes::TNotifyEvent FOnBeforeSave;
	Classes::TNotifyEvent FOnAfterLoad;
	bool FBinaryMode;
	AnsiString FComment;
	TElIniEntry* FCurEntry;
	AnsiString FCurrentKey;
	char FDelimiter;
	char FDivChar;
	bool FLazyWrite;
	bool FModified;
	AnsiString FPath;
	Registry::TRegistry* FRegistry;
	unsigned FRegRoot;
	bool FUseRegistry;
	TElIniEntry* FRoot;
	bool FSimple;
	AnsiString FWarningMessage;
	Elregutils::TRegRootType __fastcall GetRegRoot(void);
	void __fastcall SetCurrentKey(const AnsiString newValue);
	void __fastcall SetDelimiter(char newValue);
	void __fastcall SetLazyWrite(bool newValue);
	void __fastcall SetPath(AnsiString newValue);
	void __fastcall SetSimple(bool newValue);
	void __fastcall SetRegRoot(Elregutils::TRegRootType newValue);
	void __fastcall SetUseRegistry(bool newValue);
	void __fastcall IntLoadKey(Classes::TStringList* SL, int CurLine);
	void __fastcall IntLoadBinKey(Classes::TStream* F);
	void __fastcall IntSaveKey(Classes::TStream* F, AnsiString KeyName, TElIniEntry* KeyEntry);
	void __fastcall IntSaveBinKey(Classes::TStream* F, AnsiString KeyName, TElIniEntry* KeyEntry);
	
protected:
	virtual TElIniEntry* __fastcall GetValueEntry(AnsiString Key, AnsiString ValueName);
	virtual void __fastcall ParseLine(AnsiString S, AnsiString &Name, AnsiString &Value, bool &HasName);
	virtual void __fastcall TriggerBeforeSaveEvent(void);
	virtual void __fastcall TriggerAfterLoadEvent(void);
	virtual void __fastcall TriggerBeforeLoadEvent(void);
	virtual void __fastcall TriggerAfterSaveEvent(void);
	
public:
	__fastcall virtual TElIniFile(Classes::TComponent* AOwner);
	__fastcall virtual ~TElIniFile(void);
	bool __fastcall Clear(void);
	virtual bool __fastcall ClearKey(AnsiString Key);
	TElIniEntry* __fastcall CreateValue(AnsiString Key, AnsiString ValueName);
	bool __fastcall Delete(AnsiString Key, AnsiString ValueName);
	bool __fastcall EnumSubKeys(AnsiString Key, Classes::TStrings* Strings);
	bool __fastcall EnumValues(AnsiString Key, Classes::TStrings* Strings);
	AnsiString __fastcall FullKey(AnsiString Key);
	TElValueType __fastcall GetValueType(AnsiString Key, AnsiString ValueName);
	bool __fastcall KeyExists(AnsiString Key);
	void __fastcall LoadFromStream(Classes::TStream* Stream);
	void __fastcall SaveToStream(Classes::TStream* Stream);
	bool __fastcall Load(void);
	virtual void __fastcall Loaded(void);
	bool __fastcall MoveEntry(AnsiString Key, AnsiString ValueName, AnsiString NewKey);
	bool __fastcall OpenKey(AnsiString Key, bool CanCreate);
	AnsiString __fastcall OwnKey(AnsiString Key);
	AnsiString __fastcall ParentKey(AnsiString Key);
	bool __fastcall ReadBinary(AnsiString Key, AnsiString ValueName, void *Buffer, int &BufferLen);
	bool __fastcall ReadBool(AnsiString Key, AnsiString ValueName, bool DefValue, bool &Value);
	bool __fastcall ReadDouble(AnsiString Key, AnsiString ValueName, double DefValue, double &Value);
	bool __fastcall ReadInteger(AnsiString Key, AnsiString ValueName, int DefValue, int &Value);
	bool __fastcall ReadMultiString(AnsiString Key, AnsiString ValueName, Classes::TStrings* Strings);
	bool __fastcall ReadObject(AnsiString Key, Classes::TPersistent* AnObject);
	bool __fastcall ReadString(AnsiString Key, AnsiString ValueName, AnsiString DefValue, AnsiString &Value);
	bool __fastcall ReadColor(AnsiString Key, AnsiString ValueName, Graphics::TColor DefValue, Graphics::TColor &Value);
	bool __fastcall ReadRect(AnsiString Key, AnsiString ValueName, const Types::TRect &DefValue, Types::TRect &Value);
	virtual bool __fastcall RenameKey(AnsiString Key, AnsiString NewName);
	virtual bool __fastcall RenameValue(AnsiString Key, AnsiString ValueName, AnsiString NewName);
	bool __fastcall Save(void);
	virtual void __fastcall SetValueType(AnsiString Key, AnsiString ValueName, TElValueType NewType);
	int __fastcall SubKeysCount(AnsiString Key);
	bool __fastcall ValueExists(AnsiString Key, AnsiString ValueName);
	int __fastcall ValuesCount(AnsiString Key);
	bool __fastcall WriteBinary(AnsiString Key, AnsiString ValueName, void *Buffer, int BufferLen);
	bool __fastcall WriteBool(AnsiString Key, AnsiString ValueName, bool Value);
	bool __fastcall WriteDouble(AnsiString Key, AnsiString ValueName, double Value);
	bool __fastcall WriteInteger(AnsiString Key, AnsiString ValueName, int Value);
	bool __fastcall WriteMultiString(AnsiString Key, AnsiString ValueName, Classes::TStrings* Strings);
	bool __fastcall WriteObject(AnsiString Key, Classes::TPersistent* AnObject);
	bool __fastcall WriteString(AnsiString Key, AnsiString ValueName, AnsiString Value);
	bool __fastcall WriteColor(AnsiString Key, AnsiString ValueName, Graphics::TColor Value);
	bool __fastcall WriteRect(AnsiString Key, AnsiString ValueName, const Types::TRect &Value);
	__property AnsiString CurrentKey = {read=FCurrentKey, write=SetCurrentKey};
	__property bool Modified = {read=FModified, nodefault};
	
__published:
	__property bool BinaryMode = {read=FBinaryMode, write=FBinaryMode, nodefault};
	__property AnsiString Comment = {read=FComment, write=FComment};
	__property char Delimiter = {read=FDelimiter, write=SetDelimiter, nodefault};
	__property char DivChar = {read=FDivChar, write=FDivChar, default=61};
	__property bool LazyWrite = {read=FLazyWrite, write=SetLazyWrite, default=1};
	__property AnsiString Path = {read=FPath, write=SetPath};
	__property bool Simple = {read=FSimple, write=SetSimple, default=0};
	__property Elregutils::TRegRootType RegRoot = {read=GetRegRoot, write=SetRegRoot, nodefault};
	__property bool UseRegistry = {read=FUseRegistry, write=SetUseRegistry, default=0};
	__property AnsiString WarningMessage = {read=FWarningMessage, write=FWarningMessage};
	__property Classes::TNotifyEvent OnBeforeSave = {read=FOnBeforeSave, write=FOnBeforeSave};
	__property Classes::TNotifyEvent OnAfterLoad = {read=FOnAfterLoad, write=FOnAfterLoad};
	__property Classes::TNotifyEvent OnBeforeLoad = {read=FOnBeforeLoad, write=FOnBeforeLoad};
	__property Classes::TNotifyEvent OnAfterSave = {read=FOnAfterSave, write=FOnAfterSave};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elini */
using namespace Elini;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElIni
