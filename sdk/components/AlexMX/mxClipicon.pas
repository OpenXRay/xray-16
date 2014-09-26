{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995 AO ROSNO                   }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{*******************************************************}

unit mxClipIcon;

{$I mx.INC}
{$P+,W-,R-}

interface

uses {$IFDEF WIN32} Windows, {$ELSE} WinTypes, WinProcs, {$ENDIF}
  SysUtils, Classes, Graphics, Controls;

{ Icon clipboard routines }

var
  CF_ICON: Word;

procedure CopyIconToClipboard(Icon: TIcon; BackColor: TColor);
procedure AssignClipboardIcon(Icon: TIcon);
function CreateIconFromClipboard: TIcon;

{ Real-size icons support routines (32-bit only) }

procedure GetIconSize(Icon: HIcon; var W, H: Integer);
function CreateRealSizeIcon(Icon: TIcon): HIcon;
procedure DrawRealSizeIcon(Canvas: TCanvas; Icon: TIcon; X, Y: Integer);

implementation

uses Consts, Clipbrd, mxVCLUtils;

{ Icon clipboard routines }

function CreateBitmapFromIcon(Icon: TIcon; BackColor: TColor): TBitmap;
{$IFDEF WIN32}
var
  Ico: HIcon;
  W, H: Integer;
begin
  Ico := CreateRealSizeIcon(Icon);
  try
    GetIconSize(Ico, W, H);
    Result := TBitmap.Create;
    try
      Result.Width := W; Result.Height := H;
      with Result.Canvas do begin
        Brush.Color := BackColor;
        FillRect(Rect(0, 0, W, H));
        DrawIconEx(Handle, 0, 0, Ico, W, H, 0, 0, DI_NORMAL);
      end;
    except
      Result.Free;
      raise;
    end;
  finally
    DestroyIcon(Ico);
  end;
{$ELSE}
begin
  Result := VclUtils.CreateBitmapFromIcon(Icon, BackColor);
{$ENDIF}
end;

procedure CopyIconToClipboard(Icon: TIcon; BackColor: TColor);
var
  Bmp: TBitmap;
  Stream: TStream;
  Data: THandle;
  Format: Word;
  Palette: HPalette;
  Buffer: Pointer;
begin
  Bmp := CreateBitmapFromIcon(Icon, BackColor);
  try
    Stream := TMemoryStream.Create;
    try
      Icon.SaveToStream(Stream);
      Palette := 0;
      with Clipboard do begin
        Open;
        try
          Clear;
          Bmp.SaveToClipboardFormat(Format, Data, Palette);
          SetClipboardData(Format, Data);
          if Palette <> 0 then SetClipboardData(CF_PALETTE, Palette);
          Data := GlobalAlloc(HeapAllocFlags, Stream.Size);
          try
            if Data <> 0 then begin
              Buffer := GlobalLock(Data);
              try
                Stream.Seek(0, 0);
                Stream.Read(Buffer^, Stream.Size);
                SetClipboardData(CF_ICON, Data);
              finally
                GlobalUnlock(Data);
              end;
            end;
          except
            GlobalFree(Data);
            raise;
          end;
        finally
          Close;
        end;
      end;
    finally
      Stream.Free;
    end;
  finally
    Bmp.Free;
  end;
end;

procedure AssignClipboardIcon(Icon: TIcon);
var
  Stream: TStream;
  Data: THandle;
  Buffer: Pointer;
begin
  if not Clipboard.HasFormat(CF_ICON) then Exit;
  with Clipboard do begin
    Open;
    try
      Data := GetClipboardData(CF_ICON);
      Buffer := GlobalLock(Data);
      try
        Stream := TMemoryStream.Create;
        try
          Stream.Write(Buffer^, GlobalSize(Data));
          Stream.Seek(0, 0);
          Icon.LoadFromStream(Stream);
        finally
          Stream.Free;
        end;
      finally
        GlobalUnlock(Data);
      end;
    finally
      Close;
    end;
  end;
end;

function CreateIconFromClipboard: TIcon;
begin
  Result := nil;
  if not Clipboard.HasFormat(CF_ICON) then Exit;
  Result := TIcon.Create;
  try
    AssignClipboardIcon(Result);
  except
    Result.Free;
    raise;
  end;
end;

{ Real-size icons support routines }

const
  rc3_StockIcon = 0;
  rc3_Icon = 1;
  rc3_Cursor = 2;

type
  PCursorOrIcon = ^TCursorOrIcon;
  TCursorOrIcon = packed record
    Reserved: Word;
    wType: Word;
    Count: Word;
  end;

  PIconRec = ^TIconRec;
  TIconRec = packed record
    Width: Byte;
    Height: Byte;
    Colors: Word;
    Reserved1: Word;
    Reserved2: Word;
    DIBSize: Longint;
    DIBOffset: Longint;
  end;

procedure OutOfResources; near;
begin
  raise EOutOfResources.Create(ResStr(SOutOfResources));
end;

{$IFDEF WIN32}

function WidthBytes(I: Longint): Longint;
begin
  Result := ((I + 31) div 32) * 4;
end;

function GetDInColors(BitCount: Word): Integer;
begin
  case BitCount of
    1, 4, 8: Result := 1 shl BitCount;
    else Result := 0;
  end;
end;

function DupBits(Src: HBITMAP; Size: TPoint; Mono: Boolean): HBITMAP;
var
  DC, Mem1, Mem2: HDC;
  Old1, Old2: HBITMAP;
  Bitmap: Windows.TBitmap;
begin
  Mem1 := CreateCompatibleDC(0);
  Mem2 := CreateCompatibleDC(0);
  GetObject(Src, SizeOf(Bitmap), @Bitmap);
  if Mono then
    Result := CreateBitmap(Size.X, Size.Y, 1, 1, nil)
  else begin
    DC := GetDC(0);
    if DC = 0 then OutOfResources;
    try
      Result := CreateCompatibleBitmap(DC, Size.X, Size.Y);
      if Result = 0 then OutOfResources;
    finally
      ReleaseDC(0, DC);
    end;
  end;
  if Result <> 0 then begin
    Old1 := SelectObject(Mem1, Src);
    Old2 := SelectObject(Mem2, Result);
    StretchBlt(Mem2, 0, 0, Size.X, Size.Y, Mem1, 0, 0, Bitmap.bmWidth,
      Bitmap.bmHeight, SrcCopy);
    if Old1 <> 0 then SelectObject(Mem1, Old1);
    if Old2 <> 0 then SelectObject(Mem2, Old2);
  end;
  DeleteDC(Mem1);
  DeleteDC(Mem2);
end;

procedure TwoBitsFromDIB(var BI: TBitmapInfoHeader; var XorBits, AndBits: HBITMAP);
type
  PLongArray = ^TLongArray;
  TLongArray = array[0..1] of Longint;
var
  Temp: HBITMAP;
  NumColors: Integer;
  DC: HDC;
  Bits: Pointer;
  Colors: PLongArray;
  IconSize: TPoint;
  BM: Windows.TBitmap;
begin
  IconSize.X := GetSystemMetrics(SM_CXICON);
  IconSize.Y := GetSystemMetrics(SM_CYICON);
  with BI do begin
    biHeight := biHeight shr 1; { Size in record is doubled }
    biSizeImage := WidthBytes(Longint(biWidth) * biBitCount) * biHeight;
    NumColors := GetDInColors(biBitCount);
  end;
  DC := GetDC(0);
  if DC = 0 then OutOfResources;
  try
    Bits := Pointer(Longint(@BI) + SizeOf(BI) + NumColors * SizeOf(TRGBQuad));
    Temp := CreateDIBitmap(DC, BI, CBM_INIT, Bits, PBitmapInfo(@BI)^, DIB_RGB_COLORS);
    if Temp = 0 then OutOfResources;
    try
      GetObject(Temp, SizeOf(BM), @BM);
      IconSize.X := BM.bmWidth;
      IconSize.Y := BM.bmHeight;
      XorBits := DupBits(Temp, IconSize, False);
    finally
      DeleteObject(Temp);
    end;
    with BI do begin
      Inc(Longint(Bits), biSizeImage);
      biBitCount := 1;
      biSizeImage := WidthBytes(Longint(biWidth) * biBitCount) * biHeight;
      biClrUsed := 2;
      biClrImportant := 2;
    end;
    Colors := Pointer(Longint(@BI) + SizeOf(BI));
    Colors^[0] := 0;
    Colors^[1] := $FFFFFF;
    Temp := CreateDIBitmap(DC, BI, CBM_INIT, Bits, PBitmapInfo(@BI)^, DIB_RGB_COLORS);
    if Temp = 0 then OutOfResources;
    try
      AndBits := DupBits(Temp, IconSize, True);
    finally
      DeleteObject(Temp);
    end;
  finally
    ReleaseDC(0, DC);
  end;
end;

procedure ReadIcon(Stream: TStream; var Icon: HICON; ImageCount: Integer;
  StartOffset: Integer);
type
  PIconRecArray = ^TIconRecArray;
  TIconRecArray = array[0..300] of TIconRec;
var
  List: PIconRecArray;
  HeaderLen, Length: Integer;
  Colors, BitsPerPixel: Word;
  C1, C2, N, Index: Integer;
  IconSize: TPoint;
  DC: HDC;
  BI: PBitmapInfoHeader;
  ResData: Pointer;
  XorBits, AndBits: HBITMAP;
  XorInfo, AndInfo: Windows.TBitmap;
  XorMem, AndMem: Pointer;
  XorLen, AndLen: Integer;
begin
  HeaderLen := SizeOf(TIconRec) * ImageCount;
  List := AllocMem(HeaderLen);
  try
    Stream.Read(List^, HeaderLen);
    IconSize.X := GetSystemMetrics(SM_CXICON);
    IconSize.Y := GetSystemMetrics(SM_CYICON);
    DC := GetDC(0);
    if DC = 0 then OutOfResources;
    try
      BitsPerPixel := GetDeviceCaps(DC, PLANES) * GetDeviceCaps(DC, BITSPIXEL);
      if BitsPerPixel = 24 then Colors := 0
      else Colors := 1 shl BitsPerPixel;
    finally
      ReleaseDC(0, DC);
    end;
    Index := -1;
    { the following code determines which image most closely matches the
      current device. It is not meant to absolutely match Windows
      (known broken) algorithm }
    C2 := 0;
    for N := 0 to ImageCount - 1 do begin
      C1 := List^[N].Colors;
      if C1 = Colors then begin
        Index := N;
        Break;
      end
      else if Index = -1 then begin
        if C1 <= Colors then begin
          Index := N;
          C2 := List^[N].Colors;
        end;
      end
      else if C1 > C2 then Index := N;
    end;
    if Index = -1 then Index := 0;
    with List^[Index] do begin
      BI := AllocMem(DIBSize);
      try
        Stream.Seek(DIBOffset  - (HeaderLen + StartOffset), 1);
        Stream.Read(BI^, DIBSize);
        TwoBitsFromDIB(BI^, XorBits, AndBits);
        GetObject(AndBits, SizeOf(Windows.TBitmap), @AndInfo);
        GetObject(XorBits, SizeOf(Windows.TBitmap), @XorInfo);
        IconSize.X := AndInfo.bmWidth;
        IconSize.Y := AndInfo.bmHeight;
        with AndInfo do
          AndLen := bmWidthBytes * bmHeight * bmPlanes;
        with XorInfo do
          XorLen :=  bmWidthBytes * bmHeight * bmPlanes;
        Length := AndLen + XorLen;
        ResData := AllocMem(Length);
        try
          AndMem := ResData;
          with AndInfo do
            XorMem := Pointer(Longint(ResData) + AndLen);
          GetBitmapBits(AndBits, AndLen, AndMem);
          GetBitmapBits(XorBits, XorLen, XorMem);
          DeleteObject(XorBits);
          DeleteObject(AndBits);
          Icon := CreateIcon(HInstance, IconSize.X, IconSize.Y,
            XorInfo.bmPlanes, XorInfo.bmBitsPixel, AndMem, XorMem);
          if Icon = 0 then OutOfResources;
        finally
          FreeMem(ResData, Length);
        end;
      finally
        FreeMem(BI, DIBSize);
      end;
    end;
  finally
    FreeMem(List, HeaderLen);
  end;
end;

procedure GetIconSize(Icon: HIcon; var W, H: Integer);
var
  IconInfo: TIconInfo;
  BM: Windows.TBitmap;
begin
  if GetIconInfo(Icon, IconInfo) then begin
    try
      if IconInfo.hbmColor <> 0 then begin
        GetObject(IconInfo.hbmColor, SizeOf(BM), @BM);
        W := BM.bmWidth;
        H := BM.bmHeight;
      end
      else if IconInfo.hbmMask <> 0 then begin { Monochrome icon }
        GetObject(IconInfo.hbmMask, SizeOf(BM), @BM);
        W := BM.bmWidth;
        H := BM.bmHeight shr 1; { Size in record is doubled }
      end
      else begin
        W := GetSystemMetrics(SM_CXICON);
        H := GetSystemMetrics(SM_CYICON);
      end;
    finally
      if IconInfo.hbmColor <> 0 then DeleteObject(IconInfo.hbmColor);
      if IconInfo.hbmMask <> 0 then DeleteObject(IconInfo.hbmMask);
    end;
  end
  else begin
    W := GetSystemMetrics(SM_CXICON);
    H := GetSystemMetrics(SM_CYICON);
  end;
end;

{$ELSE}

procedure GetIconSize(Icon: HICON; var W, H: Integer);
begin
  W := GetSystemMetrics(SM_CXICON);
  H := GetSystemMetrics(SM_CYICON);
end;

{$ENDIF WIN32}

function CreateRealSizeIcon(Icon: TIcon): HIcon;
{$IFDEF WIN32}
var
  Mem: TMemoryStream;
  CI: TCursorOrIcon;
begin
  Result := 0;
  Mem := TMemoryStream.Create;
  try
    Icon.SaveToStream(Mem);
    Mem.Position := 0;
    Mem.ReadBuffer(CI, SizeOf(CI));
    case CI.wType of
      RC3_STOCKICON: Result := LoadIcon(0, IDI_APPLICATION);
      RC3_ICON: ReadIcon(Mem, Result, CI.Count, SizeOf(CI));
      else Result := CopyIcon(Icon.Handle);
    end;
  finally
    Mem.Free;
  end;
{$ELSE}
begin
  Result := CopyIcon(hInstance, Icon.Handle);
{$ENDIF}
end;

procedure DrawRealSizeIcon(Canvas: TCanvas; Icon: TIcon; X, Y: Integer);
{$IFDEF WIN32}
var
  Ico: HIcon;
  W, H: Integer;
begin
  Ico := CreateRealSizeIcon(Icon);
  try
    GetIconSize(Ico, W, H);
    DrawIconEx(Canvas.Handle, X, Y, Ico, W, H, 0, 0, DI_NORMAL);
  finally
    DestroyIcon(Ico);
  end;
{$ELSE}
begin
  Canvas.Draw(X, Y, Icon);
{$ENDIF}
end;

{ Module initialization part }

initialization
  { The following string should not be localized }
  CF_ICON := RegisterClipboardFormat('Delphi Icon');
  TPicture.RegisterClipboardFormat(CF_ICON, TIcon);
end.