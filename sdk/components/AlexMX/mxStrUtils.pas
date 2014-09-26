{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{         This unit based on AlexGraf String Library    }
{         by Alexei Lukin (c) 1992                      }
{                                                       }
{*******************************************************}

unit mxStrUtils;

{$I mx.INC}
{$A+,B-,E-,R-}

interface

uses SysUtils {$IFNDEF WIN32}, Str16 {$ENDIF};

type
{$IFNDEF RX_D4}
  TSysCharSet = set of Char;
{$ENDIF}
  TCharSet = TSysCharSet;

{ ** Common string handling routines ** }

function StrToOem(const AnsiStr: string): string;
{ StrToOem translates a string from the Windows character set into the
  OEM character set. }

function OemToAnsiStr(const OemStr: string): string;
{ OemToAnsiStr translates a string from the OEM character set into the
  Windows character set. }

function IsEmptyStr(const S: string; const EmptyChars: TCharSet): Boolean;
{ EmptyStr returns true if the given string contains only character
  from the EmptyChars. }

function ReplaceStr(const S, Srch, Replace: string): string;
{ Returns string with every occurrence of Srch string replaced with
  Replace string. }

function DelSpace(const S: string): string;
{ DelSpace return a string with all white spaces removed. }

function DelChars(const S: string; Chr: Char): string;
{ DelChars return a string with all Chr characters removed. }

function DelBSpace(const S: string): string;
{ DelBSpace trims leading spaces from the given string. }

function DelESpace(const S: string): string;
{ DelESpace trims trailing spaces from the given string. }

function DelRSpace(const S: string): string;
{ DelRSpace trims leading and trailing spaces from the given string. }

function DelSpace1(const S: string): string;
{ DelSpace1 return a string with all non-single white spaces removed. }

function Tab2Space(const S: string; Numb: Byte): string;
{ Tab2Space converts any tabulation character in the given string to the
  Numb spaces characters. }

function NPos(const C: string; S: string; N: Integer): Integer;
{ NPos searches for a N-th position of substring C in a given string. }

function MakeStr(C: Char; N: Integer): string;
function MS(C: Char; N: Integer): string;
{ MakeStr return a string of length N filled with character C. }

function AddChar(C: Char; const S: string; N: Integer): string;
{ AddChar return a string left-padded to length N with characters C. }

function AddCharR(C: Char; const S: string; N: Integer): string;
{ AddCharR return a string right-padded to length N with characters C. }

function LeftStr(const S: string; N: Integer): string;
{ LeftStr return a string right-padded to length N with blanks. }

function RightStr(const S: string; N: Integer): string;
{ RightStr return a string left-padded to length N with blanks. }

function CenterStr(const S: string; Len: Integer): string;
{ CenterStr centers the characters in the string based upon the
  Len specified. }

function CompStr(const S1, S2: string): Integer;
{ CompStr compares S1 to S2, with case-sensitivity. The return value is
  -1 if S1 < S2, 0 if S1 = S2, or 1 if S1 > S2. }

function CompText(const S1, S2: string): Integer;
{ CompText compares S1 to S2, without case-sensitivity. The return value
  is the same as for CompStr. }

function Copy2Symb(const S: string; Symb: Char): string;
{ Copy2Symb returns a substring of a string S from begining to first
  character Symb. }

function Copy2SymbDel(var S: string; Symb: Char): string;
{ Copy2SymbDel returns a substring of a string S from begining to first
  character Symb and removes this substring from S. }

function Copy2Space(const S: string): string;
{ Copy2Symb returns a substring of a string S from begining to first
  white space. }

function Copy2SpaceDel(var S: string): string;
{ Copy2SpaceDel returns a substring of a string S from begining to first
  white space and removes this substring from S. }

function AnsiProperCase(const S: string; const WordDelims: TCharSet): string;
{ Returns string, with the first letter of each word in uppercase,
  all other letters in lowercase. Words are delimited by WordDelims. }

function WordCount(const S: string; const WordDelims: TCharSet): Integer;
{ WordCount given a set of word delimiters, returns number of words in S. }

function WordPosition(const N: Integer; const S: string;
  const WordDelims: TCharSet): Integer;
{ Given a set of word delimiters, returns start position of N'th word in S. }

function ExtractWord(N: Integer; const S: string;
  const WordDelims: TCharSet): string;
function ExtractWordPos(N: Integer; const S: string;
  const WordDelims: TCharSet; var Pos: Integer): string;
function ExtractDelimited(N: Integer; const S: string;
  const Delims: TCharSet): string;
{ ExtractWord, ExtractWordPos and ExtractDelimited given a set of word
  delimiters, return the N'th word in S. }

function ExtractSubstr(const S: string; var Pos: Integer;
  const Delims: TCharSet): string;
{ ExtractSubstr given a set of word delimiters, returns the substring from S,
  that started from position Pos. }

function IsWordPresent(const W, S: string; const WordDelims: TCharSet): Boolean;
{ IsWordPresent given a set of word delimiters, returns True if word W is
  present in string S. }

function QuotedString(const S: string; Quote: Char): string;
{ QuotedString returns the given string as a quoted string, using the
  provided Quote character. }

function ExtractQuotedString(const S: string; Quote: Char): string;
{ ExtractQuotedString removes the Quote characters from the beginning and
  end of a quoted string, and reduces pairs of Quote characters within
  the quoted string to a single character. }

function FindPart(const HelpWilds, InputStr: string): Integer;
{ FindPart compares a string with '?' and another, returns the position of
  HelpWilds in InputStr. }

function IsWild(InputStr, Wilds: string; IgnoreCase: Boolean): Boolean;
{ IsWild compares InputString with WildCard string and returns True
  if corresponds. }

function XorString(const Key, Src: ShortString): ShortString;
function XorEncode(const Key, Source: string): string;
function XorDecode(const Key, Source: string): string;

{ ** Command line routines ** }

{$IFNDEF RX_D4}
function FindCmdLineSwitch(const Switch: string; SwitchChars: TCharSet;
  IgnoreCase: Boolean): Boolean;
{$ENDIF}
function GetCmdLineArg(const Switch: string; SwitchChars: TCharSet): string;

{ ** Numeric string handling routines ** }

function Numb2USA(const S: string): string;
{ Numb2USA converts numeric string S to USA-format. }

function Dec2Hex(N: Longint; A: Byte): string;
function D2H(N: Longint; A: Byte): string;
{ Dec2Hex converts the given value to a hexadecimal string representation
  with the minimum number of digits (A) specified. }

function Hex2Dec(const S: string): Longint;
function H2D(const S: string): Longint;
{ Hex2Dec converts the given hexadecimal string to the corresponding integer
  value. }

function Dec2Numb(N: Longint; A, B: Byte): string;
{ Dec2Numb converts the given value to a string representation with the
  base equal to B and with the minimum number of digits (A) specified. }

function Numb2Dec(S: string; B: Byte): Longint;
{ Numb2Dec converts the given B-based numeric string to the corresponding
  integer value. }

function IntToBin(Value: Longint; Digits, Spaces: Integer): string;
{ IntToBin converts the given value to a binary string representation
  with the minimum number of digits specified. }

function IntToRoman(Value: Longint): string;
{ IntToRoman converts the given value to a roman numeric string
  representation. }

function RomanToInt(const S: string): Longint;
{ RomanToInt converts the given string to an integer value. If the string
  doesn't contain a valid roman numeric value, the 0 value is returned. }

const
  CRLF = #13#10;
  DigitChars = ['0'..'9'];
{$IFNDEF CBUILDER}
  Brackets = ['(',')','[',']','{','}'];
  StdWordDelims = [#0..' ',',','.',';','/','\',':','''','"','`'] + Brackets;
{$ENDIF}

implementation

uses {$IFDEF WIN32} Windows {$ELSE} WinTypes, WinProcs {$ENDIF};

function StrToOem(const AnsiStr: string): string;
begin
  SetLength(Result, Length(AnsiStr));
  if Length(Result) > 0 then
{$IFDEF WIN32}
    CharToOemBuff(PChar(AnsiStr), PChar(Result), Length(Result));
{$ELSE}
    AnsiToOemBuff(@AnsiStr[1], @Result[1], Length(Result));
{$ENDIF}
end;

function OemToAnsiStr(const OemStr: string): string;
begin
  SetLength(Result, Length(OemStr));
  if Length(Result) > 0 then
{$IFDEF WIN32}
    OemToCharBuff(PChar(OemStr), PChar(Result), Length(Result));
{$ELSE}
    OemToAnsiBuff(@OemStr[1], @Result[1], Length(Result));
{$ENDIF}
end;

function IsEmptyStr(const S: string; const EmptyChars: TCharSet): Boolean;
var
  I, SLen: Integer;
begin
  SLen := Length(S);
  I := 1;
  while I <= SLen do begin
    if not (S[I] in EmptyChars) then begin
      Result := False;
      Exit;
    end
    else Inc(I);
  end;
  Result := True;
end;

function ReplaceStr(const S, Srch, Replace: string): string;
var
  I: Integer;
  Source: string;
begin
  Source := S;
  Result := '';
  repeat
    I := Pos(Srch, Source);
    if I > 0 then begin
      Result := Result + Copy(Source, 1, I - 1) + Replace;
      Source := Copy(Source, I + Length(Srch), MaxInt);
    end
    else Result := Result + Source;
  until I <= 0;
end;

function DelSpace(const S: String): string;
begin
  Result := DelChars(S, ' ');
end;

function DelChars(const S: string; Chr: Char): string;
var
  I: Integer;
begin
  Result := S;
  for I := Length(Result) downto 1 do begin
    if Result[I] = Chr then Delete(Result, I, 1);
  end;
end;

function DelBSpace(const S: string): string;
var
  I, L: Integer;
begin
  L := Length(S);
  I := 1;
  while (I <= L) and (S[I] = ' ') do Inc(I);
  Result := Copy(S, I, MaxInt);
end;

function DelESpace(const S: string): string;
var
  I: Integer;
begin
  I := Length(S);
  while (I > 0) and (S[I] = ' ') do Dec(I);
  Result := Copy(S, 1, I);
end;

function DelRSpace(const S: string): string;
begin
  Result := DelBSpace(DelESpace(S));
end;

function DelSpace1(const S: string): string;
var
  I: Integer;
begin
  Result := S;
  for I := Length(Result) downto 2 do begin
    if (Result[I] = ' ') and (Result[I - 1] = ' ') then
      Delete(Result, I, 1);
  end;
end;

function Tab2Space(const S: string; Numb: Byte): string;
var
  I: Integer;
begin
  I := 1;
  Result := S;
  while I <= Length(Result) do begin
    if Result[I] = Chr(9) then begin
      Delete(Result, I, 1);
      Insert(MakeStr(' ', Numb), Result, I);
      Inc(I, Numb);
    end
    else Inc(I);
  end;
end;

function MakeStr(C: Char; N: Integer): string;
begin
  if N < 1 then Result := ''
  else begin
{$IFNDEF WIN32}
    if N > 255 then N := 255;
{$ENDIF WIN32}
    SetLength(Result, N);
    FillChar(Result[1], Length(Result), C);
  end;
end;

function MS(C: Char; N: Integer): string;
begin
  Result := MakeStr(C, N);
end;

function NPos(const C: string; S: string; N: Integer): Integer;
var
  I, P, K: Integer;
begin
  Result := 0;
  K := 0;
  for I := 1 to N do begin
    P := Pos(C, S);
    Inc(K, P);
    if (I = N) and (P > 0) then begin
      Result := K;
      Exit;
    end;
    if P > 0 then Delete(S, 1, P)
    else Exit;
  end;
end;

function AddChar(C: Char; const S: string; N: Integer): string;
begin
  if Length(S) < N then
    Result := MakeStr(C, N - Length(S)) + S
  else Result := S;
end;

function AddCharR(C: Char; const S: string; N: Integer): string;
begin
  if Length(S) < N then
    Result := S + MakeStr(C, N - Length(S))
  else Result := S;
end;

function LeftStr(const S: string; N: Integer): string;
begin
  Result := AddCharR(' ', S, N);
end;

function RightStr(const S: string; N: Integer): string;
begin
  Result := AddChar(' ', S, N);
end;

function CompStr(const S1, S2: string): Integer;
begin
{$IFDEF WIN32}
  Result := CompareString(GetThreadLocale, SORT_STRINGSORT, PChar(S1),
    Length(S1), PChar(S2), Length(S2)) - 2;
{$ELSE}
  Result := CompareStr(S1, S2);
{$ENDIF}
end;

function CompText(const S1, S2: string): Integer;
begin
{$IFDEF WIN32}
  Result := CompareString(GetThreadLocale, SORT_STRINGSORT or NORM_IGNORECASE,
    PChar(S1), Length(S1), PChar(S2), Length(S2)) - 2;
{$ELSE}
  Result := CompareText(S1, S2);
{$ENDIF}
end;

function Copy2Symb(const S: string; Symb: Char): string;
var
  P: Integer;
begin
  P := Pos(Symb, S);
  if P = 0 then P := Length(S) + 1;
  Result := Copy(S, 1, P - 1);
end;

function Copy2SymbDel(var S: string; Symb: Char): string;
begin
  Result := Copy2Symb(S, Symb);
  S := DelBSpace(Copy(S, Length(Result) + 1, Length(S)));
end;

function Copy2Space(const S: string): string;
begin
  Result := Copy2Symb(S, ' ');
end;

function Copy2SpaceDel(var S: string): string;
begin
  Result := Copy2SymbDel(S, ' ');
end;

function AnsiProperCase(const S: string; const WordDelims: TCharSet): string;
var
  SLen, I: Cardinal;
begin
  Result := AnsiLowerCase(S);
  I := 1;
  SLen := Length(Result);
  while I <= SLen do begin
    while (I <= SLen) and (Result[I] in WordDelims) do Inc(I);
    if I <= SLen then Result[I] := AnsiUpperCase(Result[I])[1];
    while (I <= SLen) and not (Result[I] in WordDelims) do Inc(I);
  end;
end;

function WordCount(const S: string; const WordDelims: TCharSet): Integer;
var
  SLen, I: Cardinal;
begin
  Result := 0;
  I := 1;
  SLen := Length(S);
  while I <= SLen do begin
    while (I <= SLen) and (S[I] in WordDelims) do Inc(I);
    if I <= SLen then Inc(Result);
    while (I <= SLen) and not(S[I] in WordDelims) do Inc(I);
  end;
end;

function WordPosition(const N: Integer; const S: string;
  const WordDelims: TCharSet): Integer;
var
  Count, I: Integer;
begin
  Count := 0;
  I := 1;
  Result := 0;
  while (I <= Length(S)) and (Count <> N) do begin
    { skip over delimiters }
    while (I <= Length(S)) and (S[I] in WordDelims) do Inc(I);
    { if we're not beyond end of S, we're at the start of a word }
    if I <= Length(S) then Inc(Count);
    { if not finished, find the end of the current word }
    if Count <> N then
      while (I <= Length(S)) and not (S[I] in WordDelims) do Inc(I)
    else Result := I;
  end;
end;

function ExtractWord(N: Integer; const S: string;
  const WordDelims: TCharSet): string;
var
  I: Integer;
  Len: Integer;
begin
  Len := 0;
  I := WordPosition(N, S, WordDelims);
  if I <> 0 then
    { find the end of the current word }
    while (I <= Length(S)) and not(S[I] in WordDelims) do begin
      { add the I'th character to result }
      Inc(Len);
      SetLength(Result, Len);
      Result[Len] := S[I];
      Inc(I);
    end;
  SetLength(Result, Len);
end;

function ExtractWordPos(N: Integer; const S: string;
  const WordDelims: TCharSet; var Pos: Integer): string;
var
  I, Len: Integer;
begin
  Len := 0;
  I := WordPosition(N, S, WordDelims);
  Pos := I;
  if I <> 0 then
    { find the end of the current word }
    while (I <= Length(S)) and not(S[I] in WordDelims) do begin
      { add the I'th character to result }
      Inc(Len);
      SetLength(Result, Len);
      Result[Len] := S[I];
      Inc(I);
    end;
  SetLength(Result, Len);
end;

function ExtractDelimited(N: Integer; const S: string;
  const Delims: TCharSet): string;
var
  CurWord: Integer;
  I, Len, SLen: Integer;
begin
  CurWord := 0;
  I := 1;
  Len := 0;
  SLen := Length(S);
  SetLength(Result, 0);
  while (I <= SLen) and (CurWord <> N) do begin
    if S[I] in Delims then Inc(CurWord)
    else begin
      if CurWord = N - 1 then begin
        Inc(Len);
        SetLength(Result, Len);
        Result[Len] := S[I];
      end;
    end;
    Inc(I);
  end;
end;

function ExtractSubstr(const S: string; var Pos: Integer;
  const Delims: TCharSet): string;
var
  I: Integer;
begin
  I := Pos;
  while (I <= Length(S)) and not (S[I] in Delims) do Inc(I);
  Result := Copy(S, Pos, I - Pos);
  if (I <= Length(S)) and (S[I] in Delims) then Inc(I);
  Pos := I;
end;

function IsWordPresent(const W, S: string; const WordDelims: TCharSet): Boolean;
var
  Count, I: Integer;
begin
  Result := False;
  Count := WordCount(S, WordDelims);
  for I := 1 to Count do
    if ExtractWord(I, S, WordDelims) = W then begin
      Result := True;
      Exit;
    end;
end;

{$IFDEF WIN32}
  {$IFNDEF VER90}
    { C++Builder or Delphi 3.0 }
    {$DEFINE MBCS}
  {$ENDIF}
{$ENDIF}

function QuotedString(const S: string; Quote: Char): string;
{$IFDEF MBCS}
begin
  Result := AnsiQuotedStr(S, Quote);
{$ELSE}
var
  I: Integer;
begin
  Result := S;
  for I := Length(Result) downto 1 do
    if Result[I] = Quote then Insert(Quote, Result, I);
  Result := Quote + Result + Quote;
{$ENDIF MBCS}
end;

function ExtractQuotedString(const S: string; Quote: Char): string;
var
{$IFDEF MBCS}
  P: PChar;
begin
  P := PChar(S);
  if P^ = Quote then Result := AnsiExtractQuotedStr(P, Quote)
  else Result := S;
{$ELSE}
  I: Integer;
begin
  Result := S;
  I := Length(Result);
  if (I > 0) and (Result[1] = Quote) and
    (Result[I] = Quote) then
  begin
    Delete(Result, I, 1);
    Delete(Result, 1, 1);
    for I := Length(Result) downto 2 do begin
      if (Result[I] = Quote) and (Result[I - 1] = Quote) then
        Delete(Result, I, 1);
    end;
  end;
{$ENDIF MBCS}
end;

function Numb2USA(const S: string): string;
var
  I, NA: Integer;
begin
  I := Length(S);
  Result := S;
  NA := 0;
  while (I > 0) do begin
    if ((Length(Result) - I + 1 - NA) mod 3 = 0) and (I <> 1) then
    begin
      Insert(',', Result, I);
      Inc(NA);
    end;
    Dec(I);
  end;
end;

function CenterStr(const S: string; Len: Integer): string;
begin
  if Length(S) < Len then begin
    Result := MakeStr(' ', (Len div 2) - (Length(S) div 2)) + S;
    Result := Result + MakeStr(' ', Len - Length(Result));
  end
  else Result := S;
end;

function Dec2Hex(N: LongInt; A: Byte): string;
begin
  Result := IntToHex(N, A);
end;

function D2H(N: LongInt; A: Byte): string;
begin
  Result := IntToHex(N, A);
end;

function Hex2Dec(const S: string): Longint;
var
  HexStr: string;
begin
  if Pos('$', S) = 0 then HexStr := '$' + S
  else HexStr := S;
  Result := StrToIntDef(HexStr, 0);
end;

function H2D(const S: string): Longint;
begin
  Result := Hex2Dec(S);
end;

function Dec2Numb(N: Longint; A, B: Byte): string;
var
  C: Integer;
{$IFDEF RX_D4}
  Number: Cardinal;
{$ELSE}
  Number: Longint;
{$ENDIF}
begin
  if N = 0 then Result := '0'
  else begin
{$IFDEF RX_D4}
    Number := Cardinal(N);
{$ELSE}
    Number := N;
{$ENDIF}
    Result := '';
    while Number > 0 do begin
      C := Number mod B;
      if C > 9 then C := C + 55
      else C := C + 48;
      Result := Chr(C) + Result;
      Number := Number div B;
    end;
  end;
  if Result <> '' then Result := AddChar('0', Result, A);
end;

function Numb2Dec(S: string; B: Byte): Longint;
var
  I, P: Longint;
begin
  I := Length(S);
  Result := 0;
  S := UpperCase(S);
  P := 1;
  while (I >= 1) do begin
    if S[I] > '@' then Result := Result + (Ord(S[I]) - 55) * P
    else Result := Result + (Ord(S[I]) - 48) * P;
    Dec(I);
    P := P * B;
  end;
end;

function RomanToInt(const S: string): Longint;
const
  RomanChars = ['C','D','I','L','M','V','X'];
  RomanValues: array['C'..'X'] of Word =
    (100,500,0,0,0,0,1,0,0,50,1000,0,0,0,0,0,0,0,0,5,0,10);
var
  Index, Next: Char;
  I: Integer;
  Negative: Boolean;
begin
  Result := 0;
  I := 0;
  Negative := (Length(S) > 0) and (S[1] = '-');
  if Negative then Inc(I);
  while (I < Length(S)) do begin
    Inc(I);
    Index := UpCase(S[I]);
    if Index in RomanChars then begin
      if Succ(I) <= Length(S) then Next := UpCase(S[I + 1])
      else Next := #0;
      if (Next in RomanChars) and (RomanValues[Index] < RomanValues[Next]) then
      begin
        Inc(Result, RomanValues[Next]);
        Dec(Result, RomanValues[Index]);
        Inc(I);
      end
      else Inc(Result, RomanValues[Index]);
    end
    else begin
      Result := 0;
      Exit;
    end;
  end;
  if Negative then Result := -Result;
end;

function IntToRoman(Value: Longint): string;
Label
  A500, A400, A100, A90, A50, A40, A10, A9, A5, A4, A1;
begin
  Result := '';
{$IFNDEF WIN32}
  if (Value > MaxInt * 2) then Exit;
{$ENDIF}
  while Value >= 1000 do begin
    Dec(Value, 1000); Result := Result + 'M';
  end;
  if Value < 900 then goto A500
  else begin
    Dec(Value, 900); Result := Result + 'CM';
  end;
  goto A90;
A400:
  if Value < 400 then goto A100
  else begin
    Dec(Value, 400); Result := Result + 'CD';
  end;
  goto A90;
A500:
  if Value < 500 then goto A400
  else begin
    Dec(Value, 500); Result := Result + 'D';
  end;
A100:
  while Value >= 100 do begin
    Dec(Value, 100); Result := Result + 'C';
  end;
A90:
  if Value < 90 then goto A50
  else begin
    Dec(Value, 90); Result := Result + 'XC';
  end;
  goto A9;
A40:
  if Value < 40 then goto A10
  else begin
    Dec(Value, 40); Result := Result + 'XL';
  end;
  goto A9;
A50:
  if Value < 50 then goto A40
  else begin
    Dec(Value, 50); Result := Result + 'L';
  end;
A10:
  while Value >= 10 do begin
    Dec(Value, 10); Result := Result + 'X';
  end;
A9:
  if Value < 9 then goto A5
  else begin
    Result := Result + 'IX';
  end;
  Exit;
A4:
  if Value < 4 then goto A1
  else begin
    Result := Result + 'IV';
  end;
  Exit;
A5:
  if Value < 5 then goto A4
  else begin
    Dec(Value, 5); Result := Result + 'V';
  end;
  goto A1;
A1:
  while Value >= 1 do begin
    Dec(Value); Result := Result + 'I';
  end;
end;

function IntToBin(Value: Longint; Digits, Spaces: Integer): string;
begin
  Result := '';
  if Digits > 32 then Digits := 32;
  while Digits > 0 do begin
    if (Digits mod Spaces) = 0 then Result := Result + ' ';
    Dec(Digits);
    Result := Result + IntToStr((Value shr Digits) and 1);
  end;
end;

function FindPart(const HelpWilds, InputStr: string): Integer;
var
  I, J: Integer;
  Diff: Integer;
begin
  I := Pos('?', HelpWilds);
  if I = 0 then begin
    { if no '?' in HelpWilds }
    Result := Pos(HelpWilds, InputStr);
    Exit;
  end;
  { '?' in HelpWilds }
  Diff := Length(InputStr) - Length(HelpWilds);
  if Diff < 0 then begin
    Result := 0;
    Exit;
  end;
  { now move HelpWilds over InputStr }
  for I := 0 to Diff do begin
    for J := 1 to Length(HelpWilds) do begin
      if (InputStr[I + J] = HelpWilds[J]) or
        (HelpWilds[J] = '?') then
      begin
        if J = Length(HelpWilds) then begin
          Result := I + 1;
          Exit;
        end;
      end
      else Break;
    end;
  end;
  Result := 0;
end;

function IsWild(InputStr, Wilds: string; IgnoreCase: Boolean): Boolean;

 function SearchNext(var Wilds: string): Integer;
 { looking for next *, returns position and string until position }
 begin
   Result := Pos('*', Wilds);
   if Result > 0 then Wilds := Copy(Wilds, 1, Result - 1);
 end;

var
  CWild, CInputWord: Integer; { counter for positions }
  I, LenHelpWilds: Integer;
  MaxInputWord, MaxWilds: Integer; { Length of InputStr and Wilds }
  HelpWilds: string;
begin
  if Wilds = InputStr then begin
    Result := True;
    Exit;
  end;
  repeat { delete '**', because '**' = '*' }
    I := Pos('**', Wilds);
    if I > 0 then
      Wilds := Copy(Wilds, 1, I - 1) + '*' + Copy(Wilds, I + 2, MaxInt);
  until I = 0;
  if Wilds = '*' then begin { for fast end, if Wilds only '*' }
    Result := True;
    Exit;
  end;
  MaxInputWord := Length(InputStr);
  MaxWilds := Length(Wilds);
  if IgnoreCase then begin { upcase all letters }
    InputStr := AnsiUpperCase(InputStr);
    Wilds := AnsiUpperCase(Wilds);
  end;
  if (MaxWilds = 0) or (MaxInputWord = 0) then begin
    Result := False;
    Exit;
  end;
  CInputWord := 1;
  CWild := 1;
  Result := True;
  repeat
    if InputStr[CInputWord] = Wilds[CWild] then begin { equal letters }
      { goto next letter }
      Inc(CWild);
      Inc(CInputWord);
      Continue;
    end;
    if Wilds[CWild] = '?' then begin { equal to '?' }
      { goto next letter }
      Inc(CWild);
      Inc(CInputWord);
      Continue;
    end;
    if Wilds[CWild] = '*' then begin { handling of '*' }
      HelpWilds := Copy(Wilds, CWild + 1, MaxWilds);
      I := SearchNext(HelpWilds);
      LenHelpWilds := Length(HelpWilds);
      if I = 0 then begin
        { no '*' in the rest, compare the ends }
        if HelpWilds = '' then Exit; { '*' is the last letter }
        { check the rest for equal Length and no '?' }
        for I := 0 to LenHelpWilds - 1 do begin
          if (HelpWilds[LenHelpWilds - I] <> InputStr[MaxInputWord - I]) and
            (HelpWilds[LenHelpWilds - I]<> '?') then
          begin
            Result := False;
            Exit;
          end;
        end;
        Exit;
      end;
      { handle all to the next '*' }
      Inc(CWild, 1 + LenHelpWilds);
      I := FindPart(HelpWilds, Copy(InputStr, CInputWord, MaxInt));
      if I= 0 then begin
        Result := False;
        Exit;
      end;
      CInputWord := I + LenHelpWilds;
      Continue;
    end;
    Result := False;
    Exit;
  until (CInputWord > MaxInputWord) or (CWild > MaxWilds);
  { no completed evaluation }
  if CInputWord <= MaxInputWord then Result := False;
  if (CWild <= MaxWilds) and (Wilds[MaxWilds] <> '*') then Result := False;
end;

function XorString(const Key, Src: ShortString): ShortString;
var
  I: Integer;
begin
  Result := Src;
  if Length(Key) > 0 then
    for I := 1 to Length(Src) do
      Result[I] := Chr(Byte(Key[1 + ((I - 1) mod Length(Key))]) xor Ord(Src[I]));
end;

function XorEncode(const Key, Source: string): string;
var
  I: Integer;
  C: Byte;
begin
  Result := '';
  for I := 1 to Length(Source) do begin
    if Length(Key) > 0 then
      C := Byte(Key[1 + ((I - 1) mod Length(Key))]) xor Byte(Source[I])
    else
      C := Byte(Source[I]);
    Result := Result + AnsiLowerCase(IntToHex(C, 2));
  end;
end;

function XorDecode(const Key, Source: string): string;
var
  I: Integer;
  C: Char;
begin
  Result := '';
  for I := 0 to Length(Source) div 2 - 1 do begin
    C := Chr(StrToIntDef('$' + Copy(Source, (I * 2) + 1, 2), Ord(' ')));
    if Length(Key) > 0 then
      C := Chr(Byte(Key[1 + (I mod Length(Key))]) xor Byte(C));
    Result := Result + C;
  end;
end;

{$IFNDEF RX_D4}
function FindCmdLineSwitch(const Switch: string; SwitchChars: TCharSet;
  IgnoreCase: Boolean): Boolean;
var
  I: Integer;
  S: string;
begin
  for I := 1 to ParamCount do begin
    S := ParamStr(I);
    if (SwitchChars = []) or ((S[1] in SwitchChars) and (Length(S) > 1)) then
    begin
      S := Copy(S, 2, MaxInt);
      if IgnoreCase then begin
        if (AnsiCompareText(S, Switch) = 0) then begin
          Result := True;
          Exit;
        end;
      end
      else begin
        if (AnsiCompareStr(S, Switch) = 0) then begin
          Result := True;
          Exit;
        end;
      end;
    end;
  end;
  Result := False;
end;
{$ENDIF RX_D4}

function GetCmdLineArg(const Switch: string; SwitchChars: TCharSet): string;
var
  I: Integer;
  S: string;
begin
  I := 1;
  while I <= ParamCount do begin
    S := ParamStr(I);
    if (SwitchChars = []) or ((S[1] in SwitchChars) and (Length(S) > 1)) then
    begin
      if (AnsiCompareText(Copy(S, 2, MaxInt), Switch) = 0) then begin
        Inc(I);
        if I <= ParamCount then begin
          Result := ParamStr(I);
          Exit;
        end;
      end;
    end;
    Inc(I);
  end;
  Result := '';
end;

end.