{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1996 AO ROSNO                   }
{                                                       }
{*******************************************************}

{$I mx.INC}
{$N+}

unit mxMaxMin;

interface

function Max(A, B: Longint): Longint;
function Min(A, B: Longint): Longint;
function MaxInteger(const Values: array of Longint): Longint;
function MinInteger(const Values: array of Longint): Longint;
{$IFDEF RX_D4}
function MaxInt64(const Values: array of int64): int64;
function MinInt64(const Values: array of int64): int64;
{$ENDIF}

function MaxFloat(const Values: array of Extended): Extended;
function MinFloat(const Values: array of Extended): Extended;
function MaxDateTime(const Values: array of TDateTime): TDateTime;
function MinDateTime(const Values: array of TDateTime): TDateTime;
{$IFDEF WIN32}
function MaxOf(const Values: array of Variant): Variant;
function MinOf(const Values: array of Variant): Variant;
{$ENDIF}

procedure SwapLong(var Int1, Int2: Longint);
procedure SwapInt(var Int1, Int2: Integer);
{$IFDEF RX_D4}
procedure SwapInt64(var Int1, Int2: Int64);
{$ENDIF}

{$IFNDEF WIN32}
function MakeWord(A, B: Byte): Word;
{$ENDIF}

implementation

{$IFNDEF WIN32}
function MakeWord(A, B: Byte): Word;
begin
  Result := A or B shl 8;
end;
{$ENDIF}

procedure SwapInt(var Int1, Int2: Integer);
var
  I: Integer;
begin
  I := Int1; Int1 := Int2; Int2 := I;
end;

{$IFDEF RX_D4}
procedure SwapInt64(var Int1, Int2: Int64);
var
  I: Int64;
begin
  I := Int1; Int1 := Int2; Int2 := I;
end;
{$ENDIF}

procedure SwapLong(var Int1, Int2: Longint);
var
  I: Longint;
begin
  I := Int1; Int1 := Int2; Int2 := I;
end;

function Max(A, B: Longint): Longint;
begin
  if A > B then Result := A
  else Result := B;
end;

function Min(A, B: Longint): Longint;
begin
  if A < B then Result := A
  else Result := B;
end;

function MaxInteger(const Values: array of Longint): Longint;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] > Result then Result := Values[I];
end;

function MinInteger(const Values: array of Longint): Longint;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;

{$IFDEF RX_D4}

function MaxInt64(const Values: array of int64): int64; 
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] > Result then Result := Values[I];
end;

function MinInt64(const Values: array of int64): int64;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;

{$ENDIF RX_D4}

function MaxFloat(const Values: array of Extended): Extended;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] > Result then Result := Values[I];
end;

function MinFloat(const Values: array of Extended): Extended;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;

function MaxDateTime(const Values: array of TDateTime): TDateTime;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;

function MinDateTime(const Values: array of TDateTime): TDateTime;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;

{$IFDEF WIN32}
function MaxOf(const Values: array of Variant): Variant;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] > Result then Result := Values[I];
end;

function MinOf(const Values: array of Variant): Variant;
var
  I: Cardinal;
begin
  Result := Values[0];
  for I := 0 to High(Values) do
    if Values[I] < Result then Result := Values[I];
end;
{$ENDIF WIN32}

end.