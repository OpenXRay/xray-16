{$Q-}
{$RANGECHECKS OFF}
unit frmMain;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElTree, ElHashList, ShellApi, ElStrUtils,
  {$IFDEF VER120}
  ImgList,
  {$ENDIF}
  ElHeader, ElXPThemedControl;

type
  TMainForm = class(TForm)
    Tree: TElTree;
    ExitBtn: TButton;
    FullPathCB: TCheckBox;
    Images: TImageList;
    Button1: TButton;
    procedure ExitBtnClick(Sender: TObject);
    procedure FullPathCBClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure TreeItemExpand(Sender: TObject; Item: TElTreeItem);
    procedure TreeItemCollapse(Sender: TObject; Item: TElTreeItem);
    procedure TreeItemExpanding(Sender: TObject; Item: TElTreeItem;
      var CanProcess: Boolean);
    procedure TreeHeaderColumnClick(Sender: TObject;
      SectionIndex: Integer);
    procedure TreeCompareItems(Sender: TObject; Item1, Item2: TElTreeItem;
      var res: Integer);
    procedure TreeStartDrag(Sender: TObject; var DragObject: TDragObject);
    procedure TreeDragOver(Sender, Source: TObject; X, Y: Integer;
      State: TDragState; var Accept: Boolean);
    procedure TreeDragDrop(Sender, Source: TObject; X, Y: Integer);
    procedure TreeKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure TreeValidateInplaceEdit(Sender: TObject; Item: TElTreeItem;  Section : TElHeaderSection;
      var Text: String; var Accept: Boolean);
    procedure Button1Click(Sender: TObject);
    procedure TreeShowLineHint(Sender: TObject; Item: TElTreeItem;
      var Text: TElFString; HintWindow: THintWindow; MousePos: TPoint;
      var DoShowHint: Boolean);
  private
    LastSelected, ItemDragging : TElTreeItem;
    Hash : TElHashList;
    { Private declarations }
  public
    { Public declarations }
    procedure FillRoots;
    procedure FillTree(Item:TElTreeItem; Path : string);
  end;

var
  MainForm: TMainForm;

type TElDragObject = class (TDragControlObject)
        function GetDragCursor(Accepted: Boolean; X, Y: Integer): TCursor; override;
     end;

implementation

{$R *.DFM}

procedure TMainForm.ExitBtnClick(Sender: TObject);
begin
  Close;
end;

procedure TMainForm.FillTree;
var SRec : TSearchRec;
    TSI : TElTreeItem;
    b : boolean;
    s, FName : string;
    hn : integer;
    Icon  : TIcon;
    IconHandle : HICON;
    p : pchar;
    SHFI : TSHFileInfo;

begin
  s:=Path;
  if s[length(s)]<>'\' then s:=s+'\';
  b:=(FindFirst(s+'*.*', faAnyFile, SRec) = 0);
  while b do
  begin
    if (strcomp(SRec.FindData.cFileName,'.')<>0) and (strcomp(SRec.FindData.cFileName,'..')<>0) then
    begin
      TSI:=Tree.Items.AddItem(Item);
      TSI.Text:=SRec.FindData.cFileName;
      FName := s+ SRec.FindData.cFileName;
      //hn:=TSI.Parent.ChildrenCount;
      TSI.ColumnText.Add(FName);
      TSI.ColumnText.Add(IntToStr(SRec.Size));
      TSI.ColumnText.Add(DateToStr(FileDateToDateTime(SRec.Time)));
      TSI.ColumnText.Add(TimeToStr(FileDateToDateTime(SRec.Time)));
      if (faDirectory AND SRec.Attr)>0 then
      begin
        TSI.ParentStyle := false;
        TSI.Bold := true;
        TSI.ForceButtons:=true;
      end;
      if (faHidden AND SRec.Attr)>0 then
      begin
        TSI.ParentStyle := false;
        TSI.Italic := true;
        TSI.ParentColors:=false;
        TSI.Color := clGray;
        TSI.BkColor := Tree.BkColor;
        TSI.UseBkColor := false;
      end;
      if (FILE_ATTRIBUTE_COMPRESSED AND SRec.FindData.dwFileAttributes) >0 then
      begin
        TSI.ParentColors:=false;
        TSI.Color := clBlue;
        TSI.BkColor := Tree.BkColor;
      end;
      GetMem(p, 260);
      StrPCopy(p, FName);
      SHGetFileInfo(p, 0, SHFI, SizeOf(SHFI), $400 or $200 or $100 or 4 or 1);
      IconHandle:=SHFI.hIcon;
      if IconHandle<>0 then
      begin
        hn := Hash.GetIndex(SHFI.szTypeName);
        if (hn = -1) or (strcomp(SHFI.szTypeName,'Application')=0) or (strcomp(SHFI.szTypeName,'Icon')=0) then
        begin
          Icon:=TIcon.Create;
          Icon.Handle:=IconHandle;
          TSI.ImageIndex:=Images.AddIcon(Icon);
          Icon.Free;
          Hash.AddItem(SHFI.szTypeName, pointer (TSI.ImageIndex));
        end else TSI.ImageIndex:=integer(Hash.GetByIndex(hn));
        TSI.StateImageIndex:=TSI.ImageIndex;
      end;
      FreeMem(p);
    end;
    b:=(FindNext(SRec) = 0);
  end;
  FindClose(SRec);
end;

procedure TMainForm.FillRoots;
var TSI : TElTreeItem;
    DrivesMask : DWORD;
    i : integer;
    s : string;
    Icon  : TIcon;
    IconHandle : HICON;
    p : pchar;
    SHFI : TSHFileInfo;

begin
  DrivesMask := GetLogicalDrives;
  for I:=0 to 25 do
  begin
    if ((DrivesMask shr i) mod 2) = 1 then
    begin
      s:=chr(i+65)+':';
      TSI:=Tree.Items.AddItem(nil);
      TSI.ParentStyle:=false;
      TSI.Bold:=true;
      TSI.ColumnText.Add(s+'\');
      TSI.ForceButtons := true;
      GetMem(p, 260);
      StrPCopy(p, s+'\');
      SHGetFileInfo(p, 0, SHFI, SizeOf(SHFI), $400 or $200 or $100 or 4 or 1);
      IconHandle:=SHFI.hIcon;
      if IconHandle<>0 then
      begin
        Icon:=TIcon.Create;
        Icon.Handle:=IconHandle;
        TSI.ImageIndex:=Images.AddIcon(Icon);
        TSI.StateImageIndex:=TSI.ImageIndex;
        Icon.Free;
      end;
      FreeMem(p);
      if strlen(SHFI.szDisplayName)>0 then s := StrPas(SHFI.szDisplayName);
      TSI.Text := s;
    end;
  end;
end;

procedure TMainForm.FullPathCBClick(Sender: TObject);
begin
  Tree.HeaderSections.Item[0].Visible:=FullPathCB.Checked;
end;

procedure TMainForm.FormCreate(Sender: TObject);
begin
  LastSelected := nil;
  Hash := TElHashList.Create;
  Tree.IsUpdating:=true;
  FillRoots;
  Tree.IsUpdating:=false;
  Tree.DragImageMode := dimNever;
end;

procedure TMainForm.TreeItemExpand(Sender: TObject; Item: TElTreeItem);
begin
    Tree.IsUpdating:=true;
    FillTree(Item, Item.ColumnText[0]);
    LastSelected:=Item;
    Tree.IsUpdating:=false;
end;

procedure TMainForm.TreeItemCollapse(Sender: TObject; Item: TElTreeItem);
begin
    Tree.IsUpdating:=true;
    Item.Clear;
    LastSelected:=Item.Parent;
    Tree.IsUpdating:=false;
end;

procedure TMainForm.TreeItemExpanding(Sender: TObject; Item: TElTreeItem;
  var CanProcess: Boolean);

var SRec : TSearchRec;
    s : string;

begin
//  Tree.HeaderSections.SectionsOrder := 'i0:w100:vf;i1:w120:vt;i4:w60:vt;i3:w80:vt;i2:w80:vt';
  s:=Item.ColumnText[0];
  if s[length(s)]<>'\' then s:=s+'\';
  s:=s+'*.*';
  FillChar(SRec, sizeof(SRec), #0);
  CanProcess:=(FindFirst(s, faAnyFile, SRec) = 0);
  FindClose(SRec);
end;

procedure TMainForm.TreeHeaderColumnClick(Sender: TObject;
  SectionIndex: Integer);
begin
  Tree.SortSection:=SectionIndex;
  case SectionIndex of
    0: Tree.SortType := stText;
    1: Tree.SortType := stCustom;
    2: Tree.SortType := stNumber;
    3: Tree.SortType := stDate;
    4: Tree.SortType := stTime;
  end;
  if Tree.HeaderSections[SectionIndex].SortMode=hsmAscend then
     Tree.HeaderSections[SectionIndex].SortMode:=hsmDescend else
     Tree.HeaderSections[SectionIndex].SortMode:=hsmAscend;
  if LastSelected <> nil then LastSelected.Sort(false) else Tree.Sort(false);
end;

procedure TMainForm.TreeCompareItems(Sender: TObject; Item1,
  Item2: TElTreeItem; var res: Integer);
var S1, S2 : string;
begin
  S1 := '';
  S2 := '';
  try
    if Item1.ColumnText.Count>0 then S1:=Item1.ColumnText[0];
  except
     on E:Exception do ;
  end;
  try
    if Item2.ColumnText.Count>0 then S2:=Item2.ColumnText[0];
  except
     on E:Exception do ;
  end;
  If Item1.Bold then
  begin
    if Item2.Bold then
    begin
      res:=AnsiCompareText(S1, S2);
    end else res:=-1;
  end else
    if item2.Bold then res:=1 else
    begin
      res:=AnsiCompareText(S1, S2);
    end;
end;

procedure TMainForm.TreeStartDrag(Sender: TObject;
  var DragObject: TDragObject);
begin
  ItemDragging := Tree.ItemFocused;
  DragObject:=TElDragObject.Create(Tree);
end;

procedure TMainForm.TreeDragOver(Sender, Source: TObject; X, Y: Integer;
  State: TDragState; var Accept: Boolean);
var TSI:TElTreeItem;
begin
  Accept:=false;
  if not (Source is TElDragObject) then exit;
  TSI := ((Source as TElDragObject).Control as TElTree).GetItemAtY(Y);
  if (TSI<>nil) and (not TSI.IsUnder(ItemDragging)) then
     Accept:=true;
end;

function TElDragObject.GetDragCursor(Accepted: Boolean; X, Y: Integer): TCursor;
begin
  if Control is TElTree then
  begin
    if ((Control as TElTree).GetItemAtY(Y)<>nil) or (Accepted) then
       Result := (Control as TElTree).DragCursor else
       Result := crNoDrop;
  end else result:=inherited GetDragCursor(Accepted,X,Y);
end;


procedure TMainForm.TreeDragDrop(Sender, Source: TObject; X, Y: Integer);
begin
  MessageBox(0, 'Sorry, but moving a file is not implemented','ElPack Demo', 0);
end;

procedure TMainForm.TreeKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  if Key = VK_DELETE then MessageBox(0, 'Sorry, but deleting a file is not implemented','ElPack Demo', 0);
end;

procedure TMainForm.TreeValidateInplaceEdit(Sender: TObject;
  Item: TElTreeItem; Section : TElHeaderSection; var Text: String; var Accept: Boolean);
begin
  MessageBox(0, 'Sorry, renaming a file is not implemented','ElPack Demo', 0);
  Accept:=false;
end;

procedure TMainForm.Button1Click(Sender: TObject);
begin
(*
  if SearchForm = nil then
     SearchForm := TSearchForm.Create(nil);
  SearchForm.Show;
  SearchForm.BringToFront;
*)  
end;

procedure TMainForm.TreeShowLineHint(Sender: TObject; Item: TElTreeItem;
  var Text: TElFString; HintWindow: THintWindow; MousePos: TPoint;
  var DoShowHint: Boolean);
begin
  Text:=Item.ColumnText[0];
end;

end.

