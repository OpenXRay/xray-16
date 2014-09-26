unit EntryData;

interface

uses ElAES, ElMTree, ElStack, Windows, Classes, SysUtils, ElTools;

type PEntryRec = ^TEntryRec;
     TEntryRec = record
       ParentID,
       RecID     : DWORD;
       Group     : boolean;
       Expanded  : boolean;
       Site,
       Location,
       Location2,
       UName,
       Acct,
       Pswd,
       Info      : string;
       Added,
       Modified,
       Expires    : TDateTime;
       DoExpires  : boolean;
       WarnDays   : integer;
       ExpWarned  : boolean;
       BinDataSize: integer;
       BinData    : Pointer;
     end;

var
    FMTree     : TElMTree;
    LoadStack  : TElStack;
    FileVersion: integer;
    AssignedID : integer;

const FILE_VERSION  = 5;

function UniqueID: Integer;
function GetItemByID(ID : integer): PEntryRec;

procedure DoItemLoad(Item : TElMTreeItem; Stream : TStream);
procedure DoItemSave(Item : TElMTreeItem; Stream : TStream);

function AESEncrypt(Data : PChar; DataLen : integer; Key : Pointer): string;
function AESDecrypt(Data : PChar; DataLen : integer; Key : pointer) : string;

implementation

function GetItemByID(ID : integer): PEntryRec;
type TSearchRec = record
       Id     : DWORD;
       Result : PEntryRec;
     end;
     PSearchRec = ^TSearchRec;

var SearchRec : TSearchRec;

    procedure IntProc(Item : TElMTreeItem; Index : integer; var ContinueIterate : boolean;
              IterateData : pointer);
    var Entry : PEntryRec;
    begin
      if Item <> nil then 
      begin
        Entry := PEntryRec(Item.Data);
        if Entry.RecID = PSearchRec(IterateData).ID then
        begin
          PSearchRec(IterateData).Result := Entry;
          ContinueIterate := false;
          exit;
        end;
      end;
    end;

begin
  SearchRec.Result := nil;
  FMTree.Iterate(@IntProc, @SearchRec);
  result := SearchRec.Result;
end;

function UniqueID: Integer;
begin
  repeat
    Result := AssignedID;
    Inc(AssignedID);
  until	GetItemByID(Result) = nil;
end;

procedure DoItemLoad(Item : TElMTreeItem; Stream : TStream);
var
    Entry : PEntryRec;
    i : integer;
    p : PChar;
    b, version : byte;

begin
  try
    New(Entry);
    FillMemory(Entry, sizeof(Entry^), 0);
    Item^.Data := Entry;
    Stream.ReadBuffer(b, sizeof(byte));
    if b < 2 then
    begin
      version := 1;
      Entry^.Group := boolean(b);
    end else
    begin
      version := b;
      Stream.ReadBuffer(b, sizeof(byte));
      Entry^.Group := boolean(b);
    end;

    Stream.ReadBuffer(i, sizeof (integer));
    if (i < 0) or (i > 65535) then
       raise EOutOfMemory.Create('');
    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.Site := StrPas(p);
    FreeMem(p);

    if (Version < 5) and Entry.Group then
    begin
      exit;
    end;

    Stream.ReadBuffer(i, sizeof (integer));
    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.Location := StrPas(p);
    FreeMem(p);

    if version > 1 then
    begin
      Stream.ReadBuffer(i, sizeof (integer));
      if (i < 0) or (i > 65535) then
         raise EOutOfMemory.Create('');
      GetMem(P, i+1);
      P[i]:=#0;
      Stream.ReadBuffer(p^, i);
      Entry^.Location2 := StrPas(p);
      FreeMem(p);
    end;

    Stream.ReadBuffer(i, sizeof (integer));
    if (i < 0) or (i > 65535) then
       raise EOutOfMemory.Create('');

    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.UName := StrPas(p);
    FreeMem(p);

    Stream.ReadBuffer(i, sizeof (integer));
    if (i < 0) or (i > 65535) then
       raise EOutOfMemory.Create('');
    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.Acct := StrPas(p);
    FreeMem(p);

    Stream.ReadBuffer(i, sizeof (integer));
    if (i < 0) or (i > 65535) then
       raise EOutOfMemory.Create('');

    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.Pswd := StrPas(p);
    FreeMem(p);

    Stream.ReadBuffer(i, sizeof (integer));
    if (i < 0) or (i > 65535) then
       raise EOutOfMemory.Create('');

    GetMem(P, i+1);
    P[i]:=#0;
    Stream.ReadBuffer(p^, i);
    Entry^.Info := StrPas(p);
    FreeMem(p);

    if Version <= 3 then
      Entry.RecID := UniqueID;

    if version > 2 then
    begin
      Stream.ReadBuffer(Entry.Added, sizeof(Entry.Added));
      Stream.ReadBuffer(Entry.Modified, sizeof(Entry.Modified));
      Stream.ReadBuffer(Entry.Expires, sizeof(Entry.Expires));
      Stream.ReadBuffer(Entry.DoExpires, sizeof(Entry.DoExpires));
      Stream.ReadBuffer(Entry.WarnDays, sizeof(Entry.WarnDays));
      Stream.ReadBuffer(Entry.ExpWarned, sizeof(Entry.ExpWarned));
      if Version > 3 then
      begin
        Stream.ReadBuffer(Entry.Expanded, sizeof(Entry.Expanded));

        // Read IDs
        Stream.ReadBuffer(Entry.ParentID, sizeof(Entry.ParentID));
        Stream.ReadBuffer(Entry.RecID, sizeof(Entry.RecID));

        // Read binary data
        Stream.ReadBuffer(Entry.BinDataSize, sizeof(Entry.BinDataSize));
        if Entry.BinDataSize > 0 then
        begin
          GetMem(Entry.BinData, Entry.BinDataSize);
          Stream.ReadBuffer(PChar(Entry.BinData)^, Entry.BinDataSize);
        end;
      end;
    end else
    begin
      Entry^.Added := Now;
      Entry^.Modified := Now;
    end;
  except
    on E : Exception do
    begin
      Item.Data := nil;
      raise;
    end;
  end;
end;  { OnItemLoad }

procedure DoItemSave(Item : TElMTreeItem; Stream : TStream);
var Entry : PEntryRec;
    P : PChar;
    i : integer;
    b : byte;
begin
  Entry := PEntryRec(Item^.Data);
  b := FILE_VERSION;
  Stream.WriteBuffer(b, Sizeof(boolean));
  Stream.WriteBuffer(Entry^.Group, Sizeof(boolean));
  i := Length(Entry^.Site);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Site);
  Stream.WriteBuffer(P^, i);

  i := Length(Entry^.Location);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Location);
  Stream.WriteBuffer(P^, i);
      i := Length(Entry^.Location2);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Location2);
  Stream.WriteBuffer(P^, i);
      i := Length(Entry^.UName);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.UName);
  Stream.WriteBuffer(P^, i);
      i := Length(Entry^.Acct);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Acct);
  Stream.WriteBuffer(P^, i);
      i := Length(Entry^.Pswd);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Pswd);
  Stream.WriteBuffer(P^, i);
      i := Length(Entry^.Info);
  Stream.WriteBuffer(i, sizeof(integer));
  P := PChar(Entry^.Info);
  Stream.WriteBuffer(P^, i);
  // version 3
  Stream.WriteBuffer(Entry.Added, sizeof(Entry.Added));
  Stream.WriteBuffer(Entry.Modified, sizeof(Entry.Modified));
  Stream.WriteBuffer(Entry.Expires, sizeof(Entry.Expires));
  Stream.WriteBuffer(Entry.DoExpires, sizeof(Entry.DoExpires));
  Stream.WriteBuffer(Entry.WarnDays, sizeof(Entry.WarnDays));
  Stream.WriteBuffer(Entry.ExpWarned, sizeof(Entry.ExpWarned));
  // version 4

  Stream.WriteBuffer(Entry.Expanded, sizeof(Entry.Expanded));

  // Write IDs
  Stream.WriteBuffer(Entry.ParentID, sizeof(Entry.ParentID));
  Stream.WriteBuffer(Entry.RecID, sizeof(Entry.RecID));

  // Read binary data
  Stream.WriteBuffer(Entry.BinDataSize, sizeof(Entry.BinDataSize));
  if Entry.BinDataSize > 0 then
  begin
    Stream.WriteBuffer(PChar(Entry.BinData)^, Entry.BinDataSize);
  end;
end;  { OnItemSave }

function AESDecrypt(Data : PChar; DataLen : integer; Key : pointer) : string;
var l, rl : integer;
    InStream  : TElMemoryStream;
    OutStream : TStringStream;

type
    PAESKey128 = ^TAESKey128;

    procedure DecryptStream(Source: TStream; Dest: TStream; Key: TAESKey128);
    var
      Count: integer;
      DPos: integer;
    begin
      Source.Position := 0;
      DPos := Dest.Position;
      Source.ReadBuffer(Count, SizeOf(Count));  // read original size of data
                                                // stream
      DecryptAESStreamECB(Source, Source.Size - Source.Position, Key, Dest);
      Dest.Size := DPos + Count;  // restore the original size of data
      Dest.Position := DPos;
    end;

begin
  result := '';
  l := DataLen;

  MoveMemory(@rl, PChar(Data), sizeof(integer));
  if (l - sizeof(integer) >= rl) then
  begin
    InStream := TElMemoryStream.Create;
    try
      try
        InStream.SetPointer(Data, l);
        OutStream := TStringStream.Create('');
        try
          DecryptStream(InStream, OutStream, PAESKey128(Key)^);
          result := OutStream.DataString;
        finally
          OutStream.Free;
        end;
      finally
        InStream.SetPointer(nil, 0);
      end;
    finally
      InStream.Free;
    end;
  end;
end;

function AESEncrypt(Data : PChar; DataLen : integer; Key : Pointer): string;
var InStream  : TElMemoryStream;
    OutStream : TStringStream;

type
    PAESKey128 = ^TAESKey128;

    procedure EncryptStream(Source: TStream; Dest: TStream; Key: TAESKey128);
    var
      Count: integer;
    begin
      Source.Position := 0;
      Count := Source.Size;
      Dest.Write(Count, SizeOf(Count));  // store the source stream size to
                                         // restore after decryption
      EncryptAESStreamECB(Source, 0, Key, Dest);
    end;

begin
  if Key = nil then
    result := #0#0#0#0
  else
  begin
    InStream := TElMemoryStream.Create;
    try
      try
        InStream.SetPointer(Data, DataLen);
        OutStream := TStringStream.Create('');
        try
          EncryptStream(InStream, OutStream, PAESKey128(Key)^);
          result := OutStream.DataString;
        finally
          OutStream.Free;
        end;
      finally
        InStream.SetPointer(nil, 0);
      end;
    finally
      InStream.Free;
    end;
  end;
end;

end.

