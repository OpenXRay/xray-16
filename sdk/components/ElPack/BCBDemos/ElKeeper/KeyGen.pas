unit KeyGen;

interface

  function DecodeUName : string;
  procedure Start;

implementation

uses
  Windows, SysUtils, Classes, CryptCon, ElTools, ElStrUtils, IdeaUnit, MD5Unit;

function DecodeUName : string;
var Stream : TFileStream;
    FileName : string;
    MD5    : TMD5;
    MemStream : TStringStream;
    arr : array [1..17] of char;
    IDEA: TIDEA;

begin
  result := '';
  FileName := ExtractFilePath(ParamStr(0))+'ElKeeper.key.bin';
  if not FileExists(FileName) then exit;

  MD5:=TMD5.Create;
  MD5.InputType:=SourceString;
  MD5.InputString := 'EldoS Keeper';
  MD5.pOutputArray:= @arr;
  MD5.MD5_Hash;
  arr[17]:=#0;
  MD5.Free;
  try
    MemStream := nil;
    Stream := nil;
    IDEA := nil;
    try
      MemStream := TStringStream.Create('');
      Stream :=TFileStream.Create(FileName, fmOpenRead or fmShareDenyWrite);
      IDEA := TIDEA.Create(nil);
      IDEA.InputType := SourceStream;
      IDEA.CipherMode := ECBMode;
      IDEA.IVector:='EldoS Keeper';
      IDEA.Key:=StrPas(@arr);
      IDEA.InputStream:=Stream;
      IDEA.OutputStream:=MemStream;
      IDEA.DecipherData(False);
      MemStream.Seek(0, soFromBeginning);
      result := MemStream.DataString;
    finally
      IDEA.free;
      MemStream.Free;
      Stream.Free;
    end;
  except
  end;
end;

procedure Start;

var UserName : string;
var Stream : TStream;
    MD5    : TMD5;
    MemStream : TStringStream;
    arr : array [1..17] of char;
    IDEA: TIDEA;
    FileName : string;
    S : string;
    p : pointer;
    i : integer;
begin
  UserName := ParamStr(1);
  FileName := ExtractFilePath(ParamStr(0))+'ElKeeper.key';
  MD5:=TMD5.Create;
  MD5.InputType:=SourceString;
  MD5.InputString:='EldoS Keeper';
  MD5.pOutputArray:=@arr;
  MD5.MD5_Hash;
  arr[17]:=#0;
  MD5.Free;
  MemStream := nil;
  Stream := nil;
  IDEA := nil;
  try
    MemStream := TStringStream.Create(UserName);
    MemStream.Seek(0, soFromBeginning);
    if FileExists(FileName) then DeleteFile(FileName);
    Stream := TDirectMemoryStream.Create;
    IDEA := TIDEA.Create(nil);
    try
      IDEA.InputType  := SourceStream;
      IDEA.CipherMode := ECBMode;
      IDEA.IVector:='EldoS Keeper';
      IDEA.Key:=StrPas(@arr);
      IDEA.InputStream := MemStream;
      IDEA.OutputStream := Stream;
      IDEA.EncipherData(False);
      S := Data2Str(TDirectMemoryStream(Stream).Memory, Stream.Size);
      Stream.Free;

      Stream := TFileStream.Create(Filename, fmCreate or fmShareDenyWrite);
      WriteTextToStream(Stream, S);
      Stream.Free;
      Str2Data(S, p, i);                

      DeleteFile(FileName + '.bin');
      Stream := TFileStream.Create(Filename + '.bin', fmCreate or fmShareDenyWrite);
      Stream.WriteBuffer(Pchar(p)^, i);
      Stream.Free;
    finally
      IDEA.free;
      MemStream.Free;
      writeln(output, 'The key was generated for ', DecodeUName);
    end;
  except
    writeln('Failed to create a key');
  end;
end;

end.