unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElTree, ExtCtrls, StdCtrls;

type
  TForm1 = class(TForm)
    Tree: TElTree;
    ElPanel1: TPanel;
    QuickEditCheckBox: TCheckBox;
    procedure TreeKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure TreeClick(Sender: TObject);
    procedure TreeDblClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

procedure TForm1.TreeKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
var i : integer;
    Item : TElTreeItem;
begin
  if (Shift = []) then
  begin
    if (Tree.ItemFocused <> nil) then
    begin
      if Key = VK_UP then
      begin
        if QuickEditCheckBox.Checked and Tree.IsEditing then
           Tree.EndEdit(true);

        i := Tree.ItemFocused.AbsoluteIndex;
        if i > 0 then
        begin
          Tree.ItemFocused := Tree.Items[i - 1];
          Tree.EnsureVisible(Tree.ItemFocused);
        end;

        if QuickEditCheckBox.Checked then
           Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
        Key := 0;
      end
      else
      if Key = VK_DOWN then
      begin
        if QuickEditCheckBox.Checked and Tree.IsEditing then
           Tree.EndEdit(true);

        i := Tree.ItemFocused.AbsoluteIndex;
        if i < Tree.Items.Count - 1 then
        begin
          Tree.ItemFocused := Tree.Items[i + 1];
          Tree.EnsureVisible(Tree.ItemFocused);
        end;

        if QuickEditCheckBox.Checked then
           Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
        Key := 0;
      end
      else
      if Key = VK_LEFT then
      begin
        if QuickEditCheckBox.Checked and Tree.IsEditing then
           Tree.EndEdit(true);
        if Tree.SelectColumn = 1 then
           Tree.SelectColumn := 0
        else
        begin

          i := Tree.ItemFocused.AbsoluteIndex;
          if i > 0 then
          begin
            Tree.ItemFocused := Tree.Items[i - 1];
            Tree.EnsureVisible(Tree.ItemFocused);
          end;
          Tree.SelectColumn := 1;
        end;
        if QuickEditCheckBox.Checked then
           Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
        Key := 0;
      end
      else
      if Key = VK_RIGHT then
      begin
        if Tree.SelectColumn = 0 then
           Tree.SelectColumn := 1
        else
        begin
          i := Tree.ItemFocused.AbsoluteIndex;
          if i < Tree.Items.Count - 1 then
          begin
            Tree.ItemFocused := Tree.Items[i + 1];
            Tree.EnsureVisibleBottom(Tree.ItemFocused);
          end;
          Tree.SelectColumn := 0;
        end;
        if QuickEditCheckBox.Checked then
           Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
        Key := 0;
      end
    end;
    if Key = VK_INSERT then
    begin
      Item := Tree.Items.AddItem(nil);
      Tree.EnsureVisibleBottom(Item);
      if Tree.ItemFocused = nil then
      begin
        Tree.ItemFocused := Item;
        if QuickEditCheckBox.Checked then
           Tree.EditItem(Item, Tree.SelectColumn);
      end;
      Key := 0;
    end
    else
    if Key = VK_DELETE then
    begin
      Tree.Items.DeleteItem(Tree.ItemFocused);
      Key := 0;
    end;
  end;
end;

procedure TForm1.TreeClick(Sender: TObject);
var Item : TElTreeItem;
    HCol : integer;
    IP   : TSTItemPart;
    P    : TPoint;
begin
  GetCursorPos(P);
  P := Tree.ScreenToClient(P);
  Item := Tree.GetItemAt(P.X, P.Y, IP, HCol);
  if (Item <> nil) and ((HCol = 0) or (HCol = 1)) then
  begin
    if QuickEditCheckBox.Checked and Tree.IsEditing then
       Tree.EndEdit(true);
    Tree.ItemFocused := Item;
    Tree.SelectColumn := HCol;
    if QuickEditCheckBox.Checked then
       Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
  end;
end;

procedure TForm1.TreeDblClick(Sender: TObject);
var Item : TElTreeItem;
    HCol : integer;
    IP   : TSTItemPart;
    P    : TPoint;
begin
  GetCursorPos(P);
  P := Tree.ScreenToClient(P);
  Item := Tree.GetItemAt(P.X, P.Y, IP, HCol);
  if (Item <> nil) and ((HCol = 0) or (HCol = 1)) then
  begin
    if QuickEditCheckBox.Checked and Tree.IsEditing then
       Tree.EndEdit(true);
    Tree.ItemFocused := Item;
    Tree.SelectColumn := HCol;
    Tree.EditItem(Tree.ItemFocused, Tree.SelectColumn);
  end;
end;

end.

