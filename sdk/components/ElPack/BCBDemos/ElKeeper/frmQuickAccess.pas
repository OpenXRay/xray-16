unit frmQuickAccess;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElSplit, ExtCtrls, ElPanel, ElTree, LogoMain, ElFrmPers, StdCtrls, frmRecProp,
  ElVCLUtils, ElACtrls, ElCaption, ElMTree, ComCtrls, ElDragDrop, ElImgLst,
  ImgList, EntryData, ElXPThemedControl;

type
  TQuickAccessForm = class(TForm)
    Tree: TElTree;
    InfoPanel: TElPanel;
    ElFormPersist: TElFormPersist;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    UserNameSource: TElDragDrop;
    ImgList: TElImageList;
    AccountSource: TElDragDrop;
    PasswordSource: TElDragDrop;
    LocationSource: TElDragDrop;
    UserNameText: TElAdvancedEdit;
    AccountText: TElAdvancedEdit;
    PasswordText: TElAdvancedEdit;
    LocationText: TElAdvancedEdit;
    FormPersist: TElFormPersist;
    FormCaption: TElFormCaption;
    procedure TreeItemFocused(Sender: TObject);
    procedure TreeItemPicDraw(Sender: TObject; Item: TElTreeItem;
      var ImageIndex: Integer);
    procedure TreeDblClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure UserNameSourceOleStartDrag(Sender: TObject;
      var DragData: Pointer; var DragDataType, DragDataSize: Integer);
    procedure UserNameSourceOleSourceDrag(Sender: TObject;
      DragType: TDragType; shift: TShiftState; var ContinueDrop: Boolean);
    procedure UserNameSourceTargetDrop(Sender: TObject;
      Source: TOleDragObject; Shift: TShiftState; X, Y: Integer;
      var DragType: TDragType);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormCreate(Sender: TObject);
    procedure UserNameSourcePaint(Sender: TObject);
    procedure FormCaptionButtonClick(Sender: TObject;
      Button: TElCaptionButton);
    procedure TreeCompareItems(Sender: TObject; Item1, Item2: TElTreeItem;
      var res: Integer);
  private
    FEnBmp  : TBitmap;
    FDisBmp : TBitmap;
    procedure UpdateImages;
  public
    procedure UpdateTree(MTree : TElMTree);
  end;

var
  QuickAccessForm: TQuickAccessForm;

implementation

{$R *.DFM}

procedure TQuickAccessForm.UpdateImages;
begin
  UsernameSource.Enabled := Length(UsernameText.Text) > 0;
  AccountSource.Enabled := Length(AccountText.Text) > 0;
  LocationSource.Enabled := Length(LocationText.Text) > 0;
  PasswordSource.Enabled := Length(PasswordText.Text) > 0;

  UsernameSource.Invalidate;
  AccountSource.Invalidate;
  LocationSource.Invalidate;
  PasswordSource.Invalidate;
end;

procedure TQuickAccessForm.UpdateTree(MTree : TElMTree);

  function DataPresent(Data : Pointer; Branch : TElMTreeItem) : TElMTreeItem;
  var i : integer;
  begin
    result := nil;
    for i := 0 to Branch.List.Count - 1 do
    begin
      if TElMTreeItem(Branch.List[i]).Data = Data then
      begin
        result := TElMTreeItem(Branch.List[i]);
        exit;
      end;
    end;
  end;

  procedure ClearItems(AnItem : TElTreeItem; MTreeItem : TElMTreeItem);
  var i : integer;
  begin
    i := 0;
    while i < AnItem.Count do
    begin
      if (DataPresent(AnItem.Children[i].Data, MTreeItem) = nil) then
        Tree.Items.DeleteItem(AnItem.Children[i])
      else
        inc(i);
    end;
  end;

  procedure UpdateItems(AnItem : TElTreeItem; MTreeItem : TElMTreeItem);
  var i,
      j : integer;
      P : TElMTreeItem;
      T : TElTreeItem;
      b : boolean;
  begin
    i := 0;
    while i < MTreeItem.List.Count do
    begin
      b := false;
      T := nil;
      P := TElMTreeItem(MTreeItem.List[i]).Data;
      for j := 0 to AnItem.Count - 1 do
      begin
        T := AnItem.Children[j];
        if T.Data = P then
        begin
          b := true;
          break;
        end;
      end;
      if not b then
      begin
        T := Tree.Items.AddItem(AnItem);
        AnItem.Expand(false);
        T.ImageIndex := -1;
        T.StateImageIndex := -1;
        T.Data := P;
      end;
      T.Text := PEntryRec(P).Site;
      UpdateItems(T, MTreeItem.List[i]);
      inc(i);
    end;
  end;


var P : TElMTreeItem;
    T : TElTreeItem;
    i,
    j : integer;
    b : boolean;
begin
  if QuickAccessForm = nil then
    exit;
  Tree.Items.BeginUpdate;
  try
    i := 0;
    while i < Tree.Items.RootCount do
    begin
      P := DataPresent(Tree.Items.RootItem[i].Data, MTree.Root);
      if (P = nil) then
        Tree.Items.DeleteItem(Tree.Items.RootItem[i])
      else
      begin
        ClearItems(Tree.Items.RootItem[i], P);
        inc(i);
      end;
    end;
    for i := 0 to MTree.Root.List.Count - 1 do
    begin
      b := false;
      T := nil;
      P := TElMTreeItem(MTree.Root.List[i]).Data;
      for j := 0 to Tree.Items.RootCount - 1 do
      begin
        T := Tree.Items.RootItem[j];
        if T.Data = P then
        begin
          b := true;
          break;
        end;
      end;
      if not b then
      begin
        T := Tree.Items.AddItem(nil);
        T.ImageIndex := -1;
        T.StateImageIndex := -1;
        T.Data := P;
      end;
      T.Text := PEntryRec(P).Site;
      T.ColumnText.Clear;
      UpdateItems(T, MTree.Root.List[i]);
    end;
  finally
    Tree.Items.EndUpdate;
  end;
  Tree.Sort(true);
  UpdateImages;
end;

procedure TQuickAccessForm.TreeItemFocused(Sender: TObject);
var Entry : PEntryRec;
begin
  if (QuickAccessForm = nil) or (csDestroying in ComponentState) then
    exit;
  if (Tree.ItemFocused = nil) or (PEntryRec(Tree.ItemFocused.Data).Group) then
  begin
    UserNameText.Text := '';
    AccountText.Text := '';
    PasswordText.Text := '';
    LocationText.Text := '';
  end
  else
  begin
    Entry := PEntryRec(Tree.ItemFocused.Data);
    UserNameText.Text := Entry.UName;
    AccountText.Text  := Entry.Acct;
    PasswordText.Text := Entry.Pswd;
    LocationText.Text := Entry.Location ;
  end;
  UpdateImages;
end;

procedure TQuickAccessForm.TreeItemPicDraw(Sender: TObject;
  Item: TElTreeItem; var ImageIndex: Integer);
begin
  if PEntryRec(Item.Data).Group then
  begin
    if Item.Expanded then
      ImageIndex := 1
    else
      ImageIndex := 0;
  end
  else
    ImageIndex := 2;
end;

procedure TQuickAccessForm.TreeDblClick(Sender: TObject);
var Item : TElTreeItem;
    P    : TPoint;
begin
  GetCursorPos(P);
  P := Tree.ScreenToClient(P);
  Item := Tree.GetItemAtY(P.Y);
  if Item <> nil then
  begin
    //Item := LogoAppForm.Tree.Items.LookForItem(nil, );
    if Item <> nil then
      LogoAppForm.Tree.EnsureVisible(Item);
  end;
end;

procedure TQuickAccessForm.FormDestroy(Sender: TObject);
begin
  QuickAccessForm := nil;
  FEnBmp.Free;
  FDisBmp.Free;
end;

procedure TQuickAccessForm.UserNameSourceOleStartDrag(Sender: TObject;
  var DragData: Pointer; var DragDataType, DragDataSize: Integer);
var P : PChar;
    S : String;
    Edt : TElAdvancedEdit;
begin
  DragDataType := CF_TEXT;
  Edt := nil;
  if Sender = UserNameSource then
  begin
    Edt := UserNameText;
  end
  else
  if Sender = AccountSource then
  begin
    Edt := AccountText;
  end
  else
  if Sender = PasswordSource then
  begin
    Edt := PasswordText;
  end
  else
  if Sender = LocationSource then
  begin
    Edt := LocationText;
  end;
  if Edt = nil then exit;
  if Edt.SelLength > 0 then
    S := Copy(Edt.Text, Edt.SelStart, Edt.SelLength)
  else
    S := Edt.Text;
  if Length(S) = 0 then
  begin
    DragData := nil;
    exit;
  end;
  GetMem(P, Length(S) + 1);
  StrPCopy(P, S);
  DragData := P;
  DragDataSize := Length(S) + 1;
end;

procedure TQuickAccessForm.UserNameSourceOleSourceDrag(Sender: TObject;
  DragType: TDragType; shift: TShiftState; var ContinueDrop: Boolean);
begin
  ContinueDrop := true;
end;

procedure TQuickAccessForm.UserNameSourceTargetDrop(Sender: TObject;
  Source: TOleDragObject; Shift: TShiftState; X, Y: Integer;
  var DragType: TDragType);
begin
  DragType := dtCopy;
end;

procedure TQuickAccessForm.FormClose(Sender: TObject;
  var Action: TCloseAction);
begin
  if (LogoAppForm <> nil) and LogoAppForm.HandleAllocated and not IsWindowVisible(LogoAppForm.Handle) then
  begin
    Hide;
    LogoAppForm.QVis := false;
    Application.Minimize;
    LogoAppForm.MVis := true;
    LogoAppForm.QVis := false;
  end;
end;

procedure TQuickAccessForm.FormCreate(Sender: TObject);
begin
  FEnBmp  := TBitmap.Create;
  FDisBmp := TBitmap.Create;
  FEnBmp.Width := ImgList.Width;
  FEnBmp.Height := ImgList.Height;
  FDisBmp.Width := ImgList.Width;
  FDisBmp.Height := ImgList.Height;

  ImgList.Draw(FEnBmp.Canvas, 0, 0, 0);
  ImgList.Draw(FDisBmp.Canvas, 0, 0, 1);
end;

procedure TQuickAccessForm.UserNameSourcePaint(Sender: TObject);
var ABitmap : TBitmap;
    ACanvas : TCanvas;
begin
  if Sender = UserNameSource then
  begin
    if Length(UserNameText.Text) > 0 then
      ABitmap := FEnBmp
    else
      ABitmap := FDisBmp;
    ACanvas := UsernameSource.Canvas;
  end
  else
  if Sender = PasswordSource then
  begin
    if Length(PasswordText.Text) > 0 then
      ABitmap := FEnBmp
    else
      ABitmap := FDisBmp;
    ACanvas := PasswordSource.Canvas;
  end
  else
  if Sender = LocationSource then
  begin
    if Length(LocationText.Text) > 0 then
      ABitmap := FEnBmp
    else
      ABitmap := FDisBmp;
    ACanvas := LocationSource.Canvas;
  end
  else
  if Sender = AccountSource then
  begin
    if Length(AccountText.Text) > 0 then
      ABitmap := FEnBmp
    else
      ABitmap := FDisBmp;
    ACanvas := AccountSource.Canvas;
  end
  else
    exit;

  DrawTransparentBitmapEx(ACanvas.Handle, ABitmap, 0, 0,
    Rect(0, 0, ABitmap.Width - 1, ABitmap.Height - 1), ABitmap.Canvas.Pixels[0, ABitmap.Height - 1]);
end;

procedure TQuickAccessForm.FormCaptionButtonClick(Sender: TObject;
  Button: TElCaptionButton);
begin
  FormPersist.TopMost := Button.Down;
end;

procedure TQuickAccessForm.TreeCompareItems(Sender: TObject; Item1,
  Item2: TElTreeItem; var res: Integer);
var i1,
    i2  : integer;

begin
  i1 := LogoAppForm.GetDataIndex(Item1.Data);
  i2 := LogoAppForm.GetDataIndex(Item2.Data);
  if i2 > i1 then
    res := -1
  else
  if i2 < i1 then
    res := 1
  else
    res := 0;
end;

end.

