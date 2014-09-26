//==============================================================================
// hxGrid framework
// Copyright (C) 2007 by Roman Lut
// hax@deep-shadows.com
// http://www.deep-shadows.com/hax/
//==============================================================================

unit I_GenericStream;

interface
uses windows, sysutils;

const IID_IGENERICSTREAM      = $762EFFFE;
const IGENERICSTREAM_VERSION  = $00010000;

//===========================================================
// TGENERICSTREAMCREATIONPARAMETERS
//===========================================================
type TGENERICSTREAMCREATIONPARAMETERS = packed record
       ptr: pointer;
       length : DWORD;
      end;

//===========================================================
// IGenericStream
//===========================================================
type IGenericStream = interface(IUnknown)

  function GetVersion(): DWORD;

  function GetBasePointer(): PByteArray; stdcall;

  function GetCurPointer(): PByteArray; stdcall;

  function isReadOnly(): boolean; stdcall;

  function GetLength(): DWORD; stdcall;

  procedure Write(var data; count: DWORD); stdcall;

  function  Read(var data; count: DWORD): DWORD; stdcall;

  procedure Seek(pos: DWORD); stdcall;

  function GetPos(): DWORD; stdcall;

  procedure Clear(); stdcall;

  procedure FastClear(); stdcall;

  procedure GrowToPos(DestSize : integer = -1); stdcall;

  procedure Skip(count: DWORD); stdcall;

  procedure SetLength(newLength: DWORD); stdcall;

  procedure Compact(); stdcall;
end;

implementation

end.
