unit Md5unit;
{*****************************************************************************
 UNIT: MD5Unit
 Description:  This unit contains an Object Pascal Object which can be used to
               perform an MD5 Hashing of byte array, file, or Pascal String.  An
               MD5 Hashing or Message Digest is a 'finger print' of the
               input. This is 100% PASCAL!!!

               "It is conjectured that it is computationally infeasible
               to produce two messages having the same message digest"....
               "The MD5 algorithm is intended for digital signature
               applications, where a large file must be "compressed" in a
               secure manner before being encrypted with a private (secret) key
               under a public-key cryptosystem such as RSA." R. Rivest
               RfC: 1321, RSA Data Security, Inc. April 1992

 The MD5 Algorithm was produced by RSA Data Security Inc.(See LEGAL)
 -----------------------------------------------------------------------------
 Code Author:  Greg Carter, gregc@cryptocard.com
 Organization: CRYPTOCard Corporation, info@cryptocard.com
               R&D Division, Carleton Place, ON, CANADA, K7C 3T2
               1-613-253-3152 Voice, 1-613-253-4685 Fax.
 Date of V.1:  Jan. 3 1996.

 Compatibility & Testing with BP7.0: Anne Marcel Roorda, garfield@xs4all.nl
 -----------------------------------------------------------------------------}
 {Useage:  Below is typical usage(for File)of the MD5 Object, Follow these steps:
        Step 1: Declare and Create a New TMD5 object.  This can be done by
                'Drag N Drop' a TMD5 off the Delphi Tool Pallet,
                or explicitly in code.
        Step 2: Set the InputType.
        Step 3: Point to the input(InputString, InputFilePath, pInputArray).
        Step 4: Point to the output Array(pOutputArray).
        Step 5: Call the MD5_Hash procedure.
                Your Done!

Example
procedure Tcryptfrm.Button1Click(Sender: TObject);
var
 md5hash: TMD5;                  (* Step 1a *)
 outarray: array[0..15] of char;
 InputFile: File;
 startTime: LongInt;
begin
 md5hash := TMD5.Create(Self);   (* Step 1b *)
 try
  If OpenDialog1.Execute then
  begin
    md5hash.InputType := SourceFile;  (* Step 2 *)
    md5hash.InputFilePath := OpenDialog1.FileName; (* Step 3 *)
    md5hash.pOutputArray := @outarray;             (* Step 4 *)
    startTime := timeGetTime;
    md5hash.MD5_Hash;                              (* Step 5 *)
    LEDLabel1.Caption := IntToStr(timeGetTime - startTime);
    Label2.Caption := StrPas(outarray);     (* Do something with output *)
  end;(* if *)
 finally
  md5hash.free;
 end;
end;
{-----------------------------------------------------------------------------}
{LEGAL:        The algorithm was placed into the public domain, hence requires
               no license or runtime fees.  However this code is copyright by
               CRYPTOCard.  CRYPTOCard grants anyone who may wish to use, modify
               or redistribute this code privileges to do so, provided the user
               agrees to the following three(3) rules:

               1)Any Applications, (ie exes which make use of this
               Object...), for-profit or non-profit,
               must acknowledge the author of this Object(ie.
               MD5 Implementation provided by Greg Carter, CRYPTOCard
               Corporation) somewhere in the accompanying Application
               documentation(ie AboutBox, HelpFile, readme...).  NO runtime
               or licensing fees are required!

               2)Any Developer Component(ie Delphi Component, Visual Basic VBX,
               DLL) derived from this software must acknowledge that it is
               derived from "MD5 Object Pascal Implementation Originated by
               Greg Carter, CRYPTOCard Corporation 1996". Also all efforts should
               be made to point out any changes from the original.
               !!!!!Further, any Developer Components based on this code
               *MAY NOT* be sold for profit.  This Object was placed into the
               public domain, and therefore any derived components should
               also.!!!!!

               3)CRYPTOCard Corporation makes no representations concerning this
               software or the suitability of this software for any particular
               purpose. It is provided "as is" without express or implied
               warranty of any kind. CRYPTOCard accepts no liability from any
               loss or damage as a result of using this software.

CRYPTOCard Corporation is in no way affiliated with RSA Data Security Inc.
The MD5 Algorithm was produced by RSA Data Security Inc.
-----------------------------------------------------------------------------
Why Use this instead of a freely available C DLL?

The goal was to provide a number of Encryption/Hash implementations in Object
Pascal, so that the Pascal Developer has considerably more freedom.  These
Implementations are geared toward the PC(Intel) Microsoft Windows developer,
who will be using Borland's New 32bit developement environment(Delphi32).  The
code generated by this new compiler is considerablely faster then 16bit versions.
And should provide the Developer with faster implementations then those using
C DLLs.
-----------------------------------------------------------------------------
NOTES: Version 1 does not contain any cross-platform considerations.  If trying
       to use this code on a Big Endian style processor you will need to write
       additional code to reorder the bytes.
------------------------------------------------------------------------------
Revised:  00/00/00 BY: ******* Reason: ******
------------------------------------------------------------------------------
}
{Declare the compiler defines}
{------Changeable compiler switches-----------------------------------}
{$A+   Word align variables }
{$F+   Force Far calls }
{$K+   Use smart callbacks
{$N+   Allow coprocessor instructions }
{$P+   Open parameters enabled }
{$S+   Stack checking }
{$T-   @ operator is NOT typed }
{$U-   Non Pentium safe FDIV }
{$Z-   No automatic word-sized enumerations}
{$H+   Huge Strings}
{$Q-   No Integer overflow checking}
{---------------------------------------------------------------------}

interface
uses SysUtils, Classes, Windows
{$IFNDEF MD5ONLY}
, CryptCon
{$ENDIF}
;

 {An enumerated typt which tells the object what type the input to the cipher is}
{$IFDEF MD5ONLY}
type
 TSourceType = (SourceFile, SourceByteArray,SourceString);
{$ENDIF}

Type
ULONG32 = record
 LoWord16: WORD;
 HiWord16: WORD;
end;

PULONG32 = ^ULONG32;
PLong = ^LongInt;

hashDigest = record
  A: DWORD;
  B: DWORD;
  C: DWORD;
  D: DWORD;
end;{hashArray}

PTR_Hash = ^hashDigest;

 TMD5 = class
 Private
 { Private declarations }

  FType : TSourceType;                     {Source type, whether its a file or ByteArray, or
                                            a Pascal String}
  FInputFilePath: String;                  {Full Path to Input File}
  FInputArray: PByte;                      {Point to input array}
  FInputString: String;                    {Input String}
  FOutputDigest: PTR_Hash;                 {output MD5 Digest}
  FSourceLength: LongInt;                  {input length in BYTES}
  FActiveBlock: Array[0..15] of DWORD;     {the 64Byte block being transformed}
  FA, FB, FC, FD, FAA, FBB, FCC, FDD: DWORD;
  {FA..FDD are used during Step 4, the transform.  I made them part of the
   Object to cut down on time used to pass variables.}
  {FF, GG, HH, II are used in Step 4, the transform}
  Procedure FF(var a, b, c, d, x: DWORD; s: BYTE; ac: DWORD);
  Procedure GG(var a, b, c, d, x: DWORD; s: BYTE; ac: DWORD);
  Procedure HH(var a, b, c, d, x: DWORD; s: BYTE; ac: DWORD);
  Procedure II(var a, b, c, d, x: DWORD; s: BYTE; ac: DWORD);

 protected
    { Protected declarations }
 public
    { Public declarations }
  {Initialize is used in Step 3, this fills FA..FD with init. values
   and points FpA..FpD to FA..FD}
  Procedure MD5_Initialize;
  {this is where all the magic happens}
  Procedure MD5_Transform;
  Procedure MD5_Finish;
  Procedure MD5_Hash_Bytes;
{  Procedure MD5_Hash_String;(Pascal Style strings???)}
  Procedure MD5_Hash_File;
  {This procedure sends the data 64Bytes at a time to MD5_Transform}
  Procedure MD5_Hash;
  Property pInputArray: PByte read FInputArray write FInputArray;
  Property pOutputArray: PTR_Hash read FOutputDigest write FOutputDigest;{!!See FOutputArray}
 Published
  Property InputType: TSourceType read FType write FType;
  Property InputFilePath: String read FInputFilePath write FInputFilePath;
  Property InputString: String read FInputString write FInputString;
  Property InputLength: LongInt read FSourceLength write FSourceLength;
end;{TMD5}

Const
{Constants for MD5Transform routine.}
 S11 = 7;
 S12 = 12;
 S13 = 17;
 S14 = 22;
 S21 = 5;
 S22 = 9;
 S23 = 14;
 S24 = 20;
 S31 = 4;
 S32 = 11;
 S33 = 16;
 S34 = 23;
 S41 = 6;
 S42 = 10;
 S43 = 15;
 S44 = 21;

implementation

{This will only work on an intel}

{$warnings off}
Function ROL(A: Longint; Amount: BYTE): Longint;
begin
 asm
   mov cl, Amount
   mov eax, a
   rol eax, cl
   mov result, eax
 end;
end;
{$warnings on}

Procedure TMD5.MD5_Initialize;
begin
 FA := $67452301; FB:=$efcdab89; FC:=$98badcfe; FD:=$10325476;
end;{MD5_Initialize}

Procedure TMD5.FF;
{Purpose:  Round 1 of the Transform.
           Equivalent to a = b + ((a + F(b,c,d) + x + ac) <<< s)
           Where F(b,c,d) = b And c Or Not(b) And d
}
begin
 a := a + ((b and c) Or (not(b) And (d))) + x + ac;
 a:= ROL(a, s);
 Inc(a, b);
end;{FF}

Procedure TMD5.GG;
{Purpose:  Round 2 of the Transform.
           Equivalent to a = b + ((a + G(b,c,d) + x + ac) <<< s)
           Where G(b,c,d) = b And d Or c Not d
}
begin
 a := a + ((b And d) Or ( c And (Not d))) + x + ac;
 a:= ROL(a, s);
 Inc(a, b);
end;{GG}

Procedure TMD5.HH;
{Purpose:  Round 3 of the Transform.
           Equivalent to a = b + ((a + H(b,c,d) + x + ac) <<< s)
           Where H(b,c,d) = b Xor c Xor d
}
begin
 a := a + (b Xor c Xor d) + x + ac;
 a := ROL(a, s);
 a := b + a;
end;{HH}

Procedure TMD5.II;
{Purpose:  Round 4 of the Transform.
           Equivalent to a = b + ((a + I(b,c,d) + x + ac) <<< s)
           Where I(b,c,d) = C Xor (b Or Not(d))
}
begin
 a := a + (c Xor (b Or (Not d))) + x + ac;
 a := ROL(a, s);
 a := b + a;
end;{II}

Procedure TMD5.MD5_Transform;
{Purpose:  Perform Step 4 of the algorithm.  This is where all the important
           stuff happens.  This performs the rounds on a 64Byte Block.  This
           procedure should be called in a loop until all input data has been
           transformed.
}

begin
  FAA := FA;
  FBB := FB;
  FCC := FC;
  FDD := FD;

  { Round 1 }
  FF (FA, FB, FC, FD, FActiveBlock[ 0], S11, $d76aa478); { 1 }
  FF (FD, FA, FB, FC, FActiveBlock[ 1], S12, $e8c7b756); { 2 }
  FF (FC, FD, FA, FB, FActiveBlock[ 2], S13, $242070db); { 3 }
  FF (FB, FC, FD, FA, FActiveBlock[ 3], S14, $c1bdceee); { 4 }
  FF (FA, FB, FC, FD, FActiveBlock[ 4], S11, $f57c0faf); { 5 }
  FF (FD, FA, FB, FC, FActiveBlock[ 5], S12, $4787c62a); { 6 }
  FF (FC, FD, FA, FB, FActiveBlock[ 6], S13, $a8304613); { 7 }
  FF (FB, FC, FD, FA, FActiveBlock[ 7], S14, $fd469501); { 8 }
  FF (FA, FB, FC, FD, FActiveBlock[ 8], S11, $698098d8); { 9 }
  FF (FD, FA, FB, FC, FActiveBlock[ 9], S12, $8b44f7af); { 10 }
  FF (FC, FD, FA, FB, FActiveBlock[10], S13, $ffff5bb1); { 11 }
  FF (FB, FC, FD, FA, FActiveBlock[11], S14, $895cd7be); { 12 }
  FF (FA, FB, FC, FD, FActiveBlock[12], S11, $6b901122); { 13 }
  FF (FD, FA, FB, FC, FActiveBlock[13], S12, $fd987193); { 14 }
  FF (FC, FD, FA, FB, FActiveBlock[14], S13, $a679438e); { 15 }
  FF (FB, FC, FD, FA, FActiveBlock[15], S14, $49b40821); { 16 }

 { Round 2 }
  GG (FA, FB, FC, FD, FActiveBlock[ 1], S21, $f61e2562); { 17 }
  GG (FD, FA, FB, FC, FActiveBlock[ 6], S22, $c040b340); { 18 }
  GG (FC, FD, FA, FB, FActiveBlock[11], S23, $265e5a51); { 19 }
  GG (FB, FC, FD, FA, FActiveBlock[ 0], S24, $e9b6c7aa); { 20 }
  GG (FA, FB, FC, FD, FActiveBlock[ 5], S21, $d62f105d); { 21 }
  GG (FD, FA, FB, FC, FActiveBlock[10], S22,  $2441453); { 22 }
  GG (FC, FD, FA, FB, FActiveBlock[15], S23, $d8a1e681); { 23 }
  GG (FB, FC, FD, FA, FActiveBlock[ 4], S24, $e7d3fbc8); { 24 }
  GG (FA, FB, FC, FD, FActiveBlock[ 9], S21, $21e1cde6); { 25 }
  GG (FD, FA, FB, FC, FActiveBlock[14], S22, $c33707d6); { 26 }
  GG (FC, FD, FA, FB, FActiveBlock[ 3], S23, $f4d50d87); { 27 }
  GG (FB, FC, FD, FA, FActiveBlock[ 8], S24, $455a14ed); { 28 }
  GG (FA, FB, FC, FD, FActiveBlock[13], S21, $a9e3e905); { 29 }
  GG (FD, FA, FB, FC, FActiveBlock[ 2], S22, $fcefa3f8); { 30 }
  GG (FC, FD, FA, FB, FActiveBlock[ 7], S23, $676f02d9); { 31 }
  GG (FB, FC, FD, FA, FActiveBlock[12], S24, $8d2a4c8a); { 32 }

  { Round 3 }
  HH (FA, FB, FC, FD, FActiveBlock[ 5], S31, $fffa3942); { 33 }
  HH (FD, FA, FB, FC, FActiveBlock[ 8], S32, $8771f681); { 34 }
  HH (FC, FD, FA, FB, FActiveBlock[11], S33, $6d9d6122); { 35 }
  HH (FB, FC, FD, FA, FActiveBlock[14], S34, $fde5380c); { 36 }
  HH (FA, FB, FC, FD, FActiveBlock[ 1], S31, $a4beea44); { 37 }
  HH (FD, FA, FB, FC, FActiveBlock[ 4], S32, $4bdecfa9); { 38 }
  HH (FC, FD, FA, FB, FActiveBlock[ 7], S33, $f6bb4b60); { 39 }
  HH (FB, FC, FD, FA, FActiveBlock[10], S34, $bebfbc70); { 40 }
  HH (FA, FB, FC, FD, FActiveBlock[13], S31, $289b7ec6); { 41 }
  HH (FD, FA, FB, FC, FActiveBlock[ 0], S32, $eaa127fa); { 42 }
  HH (FC, FD, FA, FB, FActiveBlock[ 3], S33, $d4ef3085); { 43 }
  HH (FB, FC, FD, FA, FActiveBlock[ 6], S34,  $4881d05); { 44 }
  HH (FA, FB, FC, FD, FActiveBlock[ 9], S31, $d9d4d039); { 45 }
  HH (FD, FA, FB, FC, FActiveBlock[12], S32, $e6db99e5); { 46 }
  HH (FC, FD, FA, FB, FActiveBlock[15], S33, $1fa27cf8); { 47 }
  HH (FB, FC, FD, FA, FActiveBlock[ 2], S34, $c4ac5665); { 48 }

  { Round 4 }
  II (FA, FB, FC, FD, FActiveBlock[ 0], S41, $f4292244); { 49 }
  II (FD, FA, FB, FC, FActiveBlock[ 7], S42, $432aff97); { 50 }
  II (FC, FD, FA, FB, FActiveBlock[14], S43, $ab9423a7); { 51 }
  II (FB, FC, FD, FA, FActiveBlock[ 5], S44, $fc93a039); { 52 }
  II (FA, FB, FC, FD, FActiveBlock[12], S41, $655b59c3); { 53 }
  II (FD, FA, FB, FC, FActiveBlock[ 3], S42, $8f0ccc92); { 54 }
  II (FC, FD, FA, FB, FActiveBlock[10], S43, $ffeff47d); { 55 }
  II (FB, FC, FD, FA, FActiveBlock[ 1], S44, $85845dd1); { 56 }
  II (FA, FB, FC, FD, FActiveBlock[ 8], S41, $6fa87e4f); { 57 }
  II (FD, FA, FB, FC, FActiveBlock[15], S42, $fe2ce6e0); { 58 }
  II (FC, FD, FA, FB, FActiveBlock[ 6], S43, $a3014314); { 59 }
  II (FB, FC, FD, FA, FActiveBlock[13], S44, $4e0811a1); { 60 }
  II (FA, FB, FC, FD, FActiveBlock[ 4], S41, $f7537e82); { 61 }
  II (FD, FA, FB, FC, FActiveBlock[11], S42, $bd3af235); { 62 }
  II (FC, FD, FA, FB, FActiveBlock[ 2], S43, $2ad7d2bb); { 63 }
  II (FB, FC, FD, FA, FActiveBlock[ 9], S44, $eb86d391); { 64 }

  Inc(FA, FAA);
  Inc(FB, FBB);
  Inc(FC, FCC);
  Inc(FD, FDD);
  { Zeroize sensitive information}
  FillChar(FActiveBlock, SizeOf(FActiveBlock), #0);
end;{TMD5.MD5_Transform}

Procedure TMD5.MD5_Hash;
var
 pStr: PChar;
begin
  MD5_Initialize;
  case FType of
   SourceFile:
   begin
    MD5_Hash_File;
   end;{SourceFile}
   SourceByteArray:
   begin
    MD5_Hash_Bytes;
   end;{SourceByteArray}
   SourceString:
   begin
    {Convert Pascal String to Byte Array}
    pStr:=nil;
    try {protect dyanmic memory allocation}
      GetMem(pStr, Length(FInputString)+1);
      StrPCopy(pStr, FInputString);
     FSourceLength := Length(FInputString);
     FInputArray := Pointer(pStr);
     MD5_Hash_Bytes;
    finally
      if pStr<>nil then FreeMem(pStr, Length(FInputString)+1);
    end;
   end;{SourceString}
  end;{case}
  MD5_Finish;
end;{TMD5.MD5_Hash}

Procedure TMD5.MD5_Hash_Bytes;
var
  Buffer: array[0..4159] of Byte;
  Count64: Comp;
  index: longInt;
begin
  Move(FInputArray^, Buffer, FSourceLength);
  Count64 := FSourceLength * 8;     {Save the Length(in bits) before padding}
  Buffer[FSourceLength] := $80;     {Must always pad with at least a '1'}
  inc(FSourceLength);

  while (FSourceLength mod 64)<>56 do begin
   Buffer[FSourceLength] := 0;
   Inc(FSourceLength);
  end;
  Move(Count64,Buffer[FSourceLength],SizeOf(Count64){This better be 64bits});
  index := 0;
  Inc(FSourceLength, 8);
  repeat
    MoveMemory(@FActiveBlock,@Buffer[Index],64);
    MD5_Transform;
    Inc(Index,64);
  until Index = FSourceLength;
end;{TMD5.Hash_Bytes}

Procedure TMD5.MD5_Hash_File;
var
  Buffer:array[0..4159] of BYTE;
  InputFile: File;
  Count64: Comp;
  DoneFile : Boolean;
  Index: LongInt;
  NumRead: integer;
begin
DoneFile := False;
{$IFDEF DELPHI}
 AssignFile(InputFile, FInputFilePath);
{$ENDIF}
{$IFDEF BP7}
 Assign(InputFile, FInputFilePath);
{$ENDIF}

Reset(InputFile, 1);
Count64 := 0;
repeat
    BlockRead(InputFile,Buffer,4096,NumRead);
    Count64 := Count64 + NumRead;
    if NumRead<>4096 {reached end of file}
      then begin
          Buffer[NumRead]:= $80;
          Inc(NumRead);
          while (NumRead mod 64)<>56
            do begin
               Buffer[ NumRead ] := 0;
               Inc(NumRead);
              end;
          Count64 := Count64 * 8;
          Move(Count64,Buffer[NumRead],8);
          Inc(NumRead,8);
          DoneFile := True;
        end;
    Index := 0;
    repeat
     Move(Buffer[Index], FActiveBlock, 64);
     {Flip bytes here on a Mac(I think)}

     MD5_Transform;
     Inc(Index,64);
    until Index = NumRead;
  until DoneFile;
{$IFDEF DELPHI}
  CloseFile(InputFile);
{$ENDIF}
{$IFDEF BP7}
  Close(InputFile);
{$ENDIF}
end;{TMD5.MD5_Hash_File}


Procedure TMD5.MD5_Finish;
begin
 FOutputDigest^.A := FA;
 FOutputDigest^.B := FB;
 FOutputDigest^.C := FC;
 FOutputDigest^.D := FD;
end;
end.
