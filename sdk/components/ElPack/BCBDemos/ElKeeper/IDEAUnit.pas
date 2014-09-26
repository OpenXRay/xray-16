unit Ideaunit;
{*****************************************************************************
 UNIT: Ideaunit
 Description:  This unit contains an Object Pascal Object which can be used to
               perform IDEA encryption/decryption. IDEA is a block cipher
               developed by Xuejia Lai and James L. Massey of ETH Zurich.
               The algorithm is considered to be more secure then DES.  It
               uses a 128-bit key. For a complete description of the algorithm
               see 'Applied Cryptography' by Bruce Shneier, ISBN
               0-471-11709-9.

 The IDEA Algorithm patent protected by Ascom-Tech AG.(See LEGAL)
 -----------------------------------------------------------------------------
 Code Author:  Greg Carter, gregc@cryptocard.com
 Organization: CRYPTOCard Corporation, info@cryptocard.com
               R&D Division, Carleton Place, ON, CANADA, K7C 3T2
               1-613-253-3152 Voice, 1-613-253-4685 Fax.
 Date of V.1:  Jan. 3 1996.

 Compatibility & Testing with BP7.0: Anne Marcel Roorda, garfield@xs4all.nl
 -----------------------------------------------------------------------------}
 {Useage: Below is typical usage(for File)of the IDEA Object,
          Follow these steps:
           1) Declare and Create Variable of type TIDEA.
           2) Set InputSource Type, either SourceFile, SourceByteArray, or
              SourceString(Pascal style string).
           3) Set Cipher Mode, optionally IVector.
           4) Point to Input Source and set Input Length(If needed)
           5) Point to Output Structure(array, file).
           6) Set Key;
           7) Call BF_EncipherData Method.
           8) Reference the Output. Thats it.
 **** Note **** Steps 2..6 can occure in any order.
 Here is a procedure in Delphi used to encrypt a file:
procedure Tcryptfrm.OpenCiphButtonClick(Sender: TObject);
var
 IDEA: TIDEA; (*Step 1*)
begin
IDEA := TIDEA.Create;(*Step 1b*)
 try
  If OpenDialog1.Execute then
  begin
   IDEA.InputType := SourceFile; (*Step 2*)
   IDEA.CipherMode := ECBMode;   (*Step 3*)
   IDEA.InputFilePath := OpenDialog1.FileName; (*Step 4*)
   IDEA.OutputFilePath := ChangeFileExt(OpenDialog1.FileName, '.ccc'); (*Step 5*)
   IDEA.Key := 'abcdefghijklmnopqrstuvwxyz'; (*Step 6*)
   IDEA.BF_EncipherData(False);  (*Step 7*)
  end;
 finally
  IDEA.free;
 end;
end;
{-----------------------------------------------------------------------------}
{LEGAL:        The algorithm is patent protected by Ascom-Tech AG.  No license
               fees are required for non-commerical use.  Commerical users should
               contact Ascom Systec AG, Dept CMVV, Gewerbepark, CH-5506,
               Magenwil, Switzerland, voice 41 64 56 59 83, fax 41 64 56 59 90,
               email idea@ascom.ch

               This code is copyright by CRYPTOCard.  CRYPTOCard grants anyone
               who may wish to use, modify or redistribute this code privileges
               to do so, provided the user agrees to the following three(3)
               rules:

               1)Any Applications, (ie exes which make use of this
               Object...), for-profit or non-profit,
               must acknowledge the author of this Object(ie.
               IDEA Implementation provided by Greg Carter, CRYPTOCard
               Corporation) somewhere in the accompanying Application
               documentation(ie AboutBox, HelpFile, readme...).  NO runtime
               or licensing fees are required!

               2)Any Developer Component(ie Delphi Component, Visual Basic VBX,
               DLL) derived from this software must acknowledge that it is
               derived from "IDEA Object Pascal Implementation Originated by
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

CRYPTOCard Corporation is in no way affiliated with Ascom-Tech AG.
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
NOTES:  Make sure to read the LEGAL notes!!!!
------------------------------------------------------------------------------
Revised:  00/00/00 BY: ******* Reason: ******
------------------------------------------------------------------------------
}
interface
{Declare the compiler defines}
{$I CRYPTDEF.INC}
{------Changeable compiler switches-----------------------------------}
{$A+   Word align variables }
{$F+   Force Far calls }
{$K+   Use smart callbacks
{$N+   Allow coprocessor instructions }
{$P+   Open parameters enabled }
{$S+   Stack checking }
{$T-   @ operator is NOT typed }
{$IFDEF DELPHI}
{$U-   Non Pentium safe FDIV }
{$Z-   No automatic word-sized enumerations}
{$ENDIF}
{---------------------------------------------------------------------}
{$DEFINE ORDER_BA}
uses SysUtils, Cryptcon, Classes, Controls;

const
 IDEAKEYSIZE = 16;
 IDEABLOCKSIZE = 8;
 ROUNDS = 8;
 KEYLEN = (6*ROUNDS + 4);
type

UWORD16 = WORD;  {16 unsigned integer}
pUWORD16 = ^UWORD16;
UWORD32 = LongInt; {Turn off Overflow checking}

{Intelx86}
   singleByte = Record
          byte1: BYTE;{LSB}
          byte0: BYTE;{MSB}
   end;{SingleBytes}
{$DEFINE INTEL}

 aword16 = record
  case Integer Of
   0: (SWord: UWORD16);
   1: (fByte: Array[0..1] of BYTE);
   2: (w: singleByte);
 end;{aword, 16bits!}

Paword = ^aword16;

IDEA_BLOCK = record
  X1: UWORD16;
  X2: UWORD16;
  X3: UWORD16;
  X4: UWORD16;
end;

pIDEA_BLOCK = ^IDEA_BLOCK;
KeyArray = array[0..(KEYLEN - 1)] of aword16;
PKeyArray = ^KeyArray;

 TIDEA = class(TCrypto)
 Private
 { Private declarations }
  FpKey: PChar;
  FRounds: BYTE;
{  FActiveBlock: IDEA_BLOCK;}
  FpActiveBlock: pIDEA_BLOCK;
  FEnCiKey: KeyArray;  {Encipher Key}
  FDeCiKey: KeyArray;  {Decipher Key}
  FCiKey:   pUWORD16;  {Pointer to current Key, either Encipher or Decipher Key}
  Function  MUL(pX, pY: pUWORD16): UWORD16;
  Function  INV(pX: pUWORD16): UWORD16;
  Procedure IDEA_ExpandKey;
  Procedure IDEA_InvertKey;
  Procedure IDEA_Cipher;             {En/Deciphers 64bit block depending on Key}
  Procedure EncipherBLOCK;override; {Enciphers BLOCK}
  Procedure DecipherBLOCK;override; {Deciphers BLOCK}
  Procedure SetKeys;      override; {Sets up En\DecipherKey SubKeys}
 protected
    { Protected declarations }
 public
    { Public declarations }
  constructor Create(Owner: TComponent);override;
  destructor  Destroy;override;
 end;{TIDEA}

 procedure Register;{register the component to the Delphi toolbar}

implementation

procedure Register;
  {Registers the Component to the toobar, on the tab named 'Crypto'}
  {Now all a Delphi programmer needs to do is drag n drop to have
   Blowfish encryption}
begin
  RegisterComponents('Crypto', [TIDEA]);
end;

constructor TIDEA.Create(Owner: TComponent);
begin
 FRounds := 8;
 FBLOCKSIZE := SizeOf(IDEA_BLOCK);
 FIVTemp := nil;
 FpKey := StrAlloc(IDEAKEYSIZE + 1);
 inherited Create(Owner);
 FpActiveBlock := @FSmallBuffer;
end;{Create}

destructor TIDEA.Destroy;
begin
  StrDispose(FpKey);
  inherited Destroy;
end;{TBlowFish.Destroy;}

Function  TIDEA.MUL(pX, pY: pUWORD16): UWORD16;
var
 p: UWORD32;
 x, y: UWORD16;
begin
 p := UWORD32(pX^) * pY^;
 if p = 0 then
  x := 65537 - pX^ - pY^
 else begin
  x := UWORD16(p SHR 16);
  y := UWORD16(p);
  x := y - x;

  if (y < x) then Inc(x, 65537);
 end;

 MUL := x;
end;{TIDEA.MUL}

Function  TIDEA.INV(pX: pUWORD16): UWORD16;
var
 t0, t1, q, y, x: UWORD16;
begin
 x := PX^;
 if (x <= 1) then begin
  INV := x;
  exit;
 end;
 t1 := UWORD16(65537 Div x);
 y  := UWORD16(65537 MOD x);
 t0 := 1;
 while y <> 1 do begin
  q := x Div y;
  x := x MOD y;
  t0 := t0 + (t1 * q);
  if x = 1 then begin
   INV := t0;
   exit;
  end;
  q := y Div x;
  y := y MOD x;
  t1 := t1 + (t0 * q);
 end;{while}

 INV := 1 - t1;
end;{TIDEA.INV}


Procedure TIDEA.IDEA_ExpandKey;
var
 i, j : BYTE;
 pKey: PKeyArray;
begin
 pKey := @FEnCiKey;
 j:= 0;
 for i:= 0 to 7 do begin
   FEnCiKey[i].w.byte0 := BYTE(FpKey[j]); {MSB}{do this way so comp with PGP}
   FEnCiKey[i].w.byte1 := BYTE(FpKey[j + 1]);
   j := j + 2;
{ FEnCiKey[i].SWord := i +1;  Test key}
 end;{for}

{ For j := 8 to (KEYLEN - 1) do begin
  Inc(i);
  pKey^[i + 7].SWord := (pKey^[i And 7].SWord SHL 9) Or (pKey^[i + 1 And 7].SWord SHR 7);
  Inc(pKey, (i And 8));
  i := i And 7;
 end;{for}
 For i:= 8 to (KEYLEN - 1) do begin
  if ((i And 7) < 6) then
     FEnCiKey[i].SWord := ((FEnCiKey[i - 7].SWord And 127) SHL 9) Or (FEnCiKey[i - 6].SWord SHR 7)
  else begin
   if ((i And 7) = 6) then
     FEnCiKey[i].SWord := ((FEnCiKey[i - 7].SWord And 127) SHL 9) Or (FEnCiKey[i - 14].SWord SHR 7)
   else
     FEnCiKey[i].SWord := ((FEnCiKey[i - 15].SWord And 127) SHL 9) Or (FEnCiKey[i - 14].SWord SHR 7);
  end;
 end;{for}
end;{TIDEA.IDEA_ExpandKey}

Procedure TIDEA.IDEA_InvertKey;
var
 i : WORD;
 t1, t2, t3: UWORD16;
 pDeKey, pCiKey: pUWORD16;

begin
 pCiKey := @FEnCiKey; {!!!!Expand_Key MUST have been called first!!!!}
 pDeKey := @FDeCiKey;
 Inc(pDeKey, KEYLEN);
 t1 := INV(pCiKey); Inc(pCiKey);
 t2 := 0 - pCiKey^;  Inc(pCiKey);
 t3 := 0 - PCiKey^;  Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := INV(pCiKey); Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := t3;
 Dec(pDeKey); pDeKey^ := t2;
 Dec(pDeKey); pDeKey^ := t1;
 for i := 1 to (FRounds -1) do begin
  t1 := pCiKey^; Inc(pCiKey);
  Dec(pDeKey); pDeKey^ := pCiKey^; Inc(pCiKey);
  Dec(pDeKey); pDeKey^ := t1;
  t1 := INV(pCiKey); Inc(pCiKey);
  t2 := 0 - pCiKey^;  Inc(pCiKey);
  t3 := 0 - PCiKey^;  Inc(pCiKey);
  Dec(pDeKey); pDeKey^ := INV(pCiKey); Inc(pCiKey);
  Dec(pDeKey); pDeKey^ := t2;
  Dec(pDeKey); pDeKey^ := t3;
  Dec(pDeKey); pDeKey^ := t1;
 end;{for}
 t1 := pCiKey^; Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := pCiKey^; Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := t1;
 t1 := INV(pCiKey); Inc(pCiKey);
 t2 := 0 - pCiKey^;  Inc(pCiKey);
 t3 := 0 - PCiKey^;  Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := INV(pCiKey); Inc(pCiKey);
 Dec(pDeKey); pDeKey^ := t3;
 Dec(pDeKey); pDeKey^ := t2;
 Dec(pDeKey); pDeKey^ := t1;
end;{TIDEA.IDEA_InvertKey}

Procedure TIDEA.SetKeys;   {Sets up En\DecipherKey SubKeys}
{var
 i, j: integer;
 dum: string;}
begin
 {Convert a ascii string of 'hex' characters to hex values.}
{ j := 1;
 for i:= 1 to 8 do begin
   dum:= '$' + System.Copy(FKey, j, 2);
   FpKey[i - 1] := char(StrToInt(dum));
   j := j + 2;
 end;{for}

 StrPCopy(FpKey, FKey);
 IDEA_ExpandKey;{Make Encipher Key}
 IDEA_InvertKey;{Make Decipher Key}
end;

Procedure TIDEA.IDEA_Cipher;      {Enciphers 64bit block}
{IDEA CIPHER Alogrithm}
var
 t2, t1: UWORD16; {Save these, for use in steps 11..14}
 pKey: pUWORD16;
 i : BYTE;
begin
pKey:= FCiKey;
 {Flip bytes on Intel!!}
 With FpActiveBlock^ do begin
{Intelx86, IDEA assumes BigEndian}
  X1 := (X1 SHR 8) Or (X1 SHL 8);
  X2 := (X2 SHR 8) Or (X2 SHL 8);
  X3 := (X3 SHR 8) Or (X3 SHL 8);
  X4 := (X4 SHR 8) Or (X4 SHL 8);

  for i:= 0 to (FRounds - 1) do begin
   X1 := MUL(@X1, pKey); (*Step One 1*)   Inc(pKey);
   Inc(X2, pKey^);       (*Step Two 2*)   Inc(pKey);
   Inc(X3, pKey^);       (*Step Three 3*) Inc(pKey);
   X4 := MUL(@X4, pKey); (*Step Four 4*)  Inc(pKey);

   t2 := X3 Xor X1;      (*Step Five 5*)
   t1 := X2 Xor X4;      (*Step Six 6*)

   t2 := MUL(@t2, pKey);  (*Step Seven 7*) Inc(pKey);
   Inc(t1, t2);           (*Step Eight 8*)
   t1 := MUL(@t1, pKey);  (*Step Nine 9*)  Inc(pKey);
   Inc(t2, t1);           (*Step Ten 10*)

   X1 := X1 Xor t1;       (*Step Eleven 11*)
   X4 := X4 Xor t2;       (*Step Fourteen 14*)
   t2 := t2 Xor X2;
   X2 := X3 Xor t1;       (*Step Twelve 12*)
   X3 := t2;              (*Step Thirteen 13*)
  end;{for}

  X1 := MUL(@X1, pKey);  Inc(pKey);
  t2:= X2;
  X2 := X3 + pKey^;  Inc(pKey);
  X3 := t2 + pKey^; Inc(pKey);
  X4 := MUL(@X4, pKey);
  X1 := (X1 SHR 8) Or (X1 SHL 8);
  X2 := (X2 SHR 8) Or (X2 SHL 8);
  X3 := (X3 SHR 8) Or (X3 SHL 8);
  X4 := (X4 SHR 8) Or (X4 SHL 8);
 end;{with FpActiveBlock^}
end;{TIDEA.IDEA_Encipher}

Procedure TIDEA.EncipherBLOCK;
{Private procedure.  Enciphers blocks of data pointed to by FInputArray.}
begin
 FCiKey := @FEnCiKey;  {Point to Encipher Key}
 IDEA_Cipher;
end;{TIDEA.Encipher_Bytes}

Procedure TIDEA.DecipherBLOCK;
begin
 FCiKey := @FDeCiKey;  {Point to Decipher Key}
 IDEA_Cipher;
end;{TIDEA.Decipher_Bytes}
end.
