unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElTree, ElTools, StdCtrls, ElHeader, ElVCLUtils, ElTreeComboBox,
  ElXPThemedControl, ElUxTheme, ElTmSchema;

type
  TForm1 = class(TForm)
    Tree: TElTree;
    InplaceCombo: TElTreeInplaceComboBox;
    procedure FormShow(Sender: TObject);
    procedure TreeItemDraw(Sender: TObject; Item: TElTreeItem;
      Surface: TCanvas; R: TRect; SectionIndex: Integer);
    procedure TreeClick(Sender: TObject);
    procedure InplaceComboValidateResult(Sender: TObject;
      var InputValid: Boolean);
    procedure InplaceComboBeforeOperation(Sender: TObject;
      var DefaultConversion: Boolean);
  private

  public
    { Public declarations }
  end;

var
  Form1: TForm1;

const EnumData : array[0..4] of string = ('1', '2', '3', '4','5');

type
    PDataRec = ^TDataRec;
    TDataRec = record
      Checked  : boolean;
      ComboIdx : integer;
    end;

var DataRec : TDataRec;

implementation

{$R *.DFM}

procedure TForm1.FormShow(Sender: TObject);
var TI : TElTreeItem;
    CS : TElCellStyle;
begin
  TI := Tree.Items[0];
  TI.UseStyles := true;
  CS := TI.AddStyle;
  CS.OwnerProps := true;
  CS.Style := elhsOwnerDraw;
  CS.CellType := sftEnum;
  CS := TI.AddStyle;
  CS.OwnerProps := true;
  CS.Style := elhsOwnerDraw;
  TI.Data := @DataRec;
  DataRec.Checked := false;
  DataRec.ComboIdx := 0;
end;

procedure TForm1.TreeItemDraw(Sender: TObject; Item: TElTreeItem;
  Surface: TCanvas; R: TRect; SectionIndex: Integer);
var R1 : TRect;
    sid: integer;
    ATheme: HTheme;
const CheckStates : array[boolean] of integer = (0, DFCS_CHECKED);
begin
  Surface.Brush.Style := bsClear;
  if SectionIndex = 1 then
  begin
    if Tree.IsThemeApplied then
    begin
      ATheme := OpenThemeData(Handle, 'COMBOBOX');
      if ATheme <> 0 then
      begin
        Dec(R.Right, 16);
        DrawText(Surface.Handle, Pchar(EnumData[PDataRec(Item.Data).ComboIdx]), -1, R, DT_LEFT or DT_SINGLELINE or DT_VCENTER);
        R.Left := R.Right;
        R.Right := R.Right + 16;

        if PDataRec(Item.Data).Checked then
          sid := CBS_CHECKEDNORMAL
        else
          sid := CBS_UNCHECKEDNORMAL;
        DrawThemeBackground(ATheme, Surface.Handle, CP_DROPDOWNBUTTON, CBXS_NORMAL, R, @R);
        CloseThemeData(ATheme);
        exit;
      end
    end;
    Dec(R.Right, 10);
    DrawText(Surface.Handle, Pchar(EnumData[PDataRec(Item.Data).ComboIdx]), -1, R, DT_LEFT or DT_SINGLELINE or DT_VCENTER);
    R.Left := R.Right;
    R.Right := R.Right + 10;

    ElVCLUtils.DrawArrow(Surface, eadDown, R, clWindowText, true);
  end else
  if SectionIndex = 2 then
  begin
    ElTools.CenterRects(14, R.Right - R.Left, 14, R.Bottom - R.Top, R1);
    OffsetRect(R1, R.Left, R.Top);
    if Tree.IsThemeApplied then
    begin
      ATheme := OpenThemeData(Handle, 'BUTTON');
      if ATheme <> 0 then
      begin
        if PDataRec(Item.Data).Checked then
          sid := CBS_CHECKEDNORMAL
        else
          sid := CBS_UNCHECKEDNORMAL;
        DrawThemeBackground(ATheme, Surface.Handle, BP_CHECKBOX, sid, R1, @R);
        CloseThemeData(ATheme);
        exit;
      end
    end;
    DrawFrameControl(Surface.Handle, R1, DFC_BUTTON, DFCS_BUTTONCHECK or CheckStates[PDataRec(Item.Data).Checked]);
  end;
end;

procedure TForm1.TreeClick(Sender: TObject);
var HS : integer;
    Item : TElTreeItem;
    ItemPart: TSTItemPart;
    P       : TPoint;
    Data    : PDataRec;
begin
  GetCursorPos(P);
  P := Tree.ScreenToClient(P);
  Item := Tree.GetItemAt(P.x, P.Y, ItemPart, HS);
  if HS = 2 then
  begin
    Data := PDataRec(Item.Data);
    Data.Checked := not Data.Checked;
    Item.RedrawItemPart(true, Tree.HeaderSections[HS].Left, Tree.HeaderSections[HS].Right);
  end else
  if HS = 1 then
  begin
    if P.X > Tree.HeaderSections[HS].Right -10 then
      Tree.EditItem(Item, HS); 
  end;
end;

procedure TForm1.InplaceComboValidateResult(Sender: TObject;
  var InputValid: Boolean);
var comboBox : TCombobox;
begin
  ComboBox := InplaceCombo.Editor;
  if ComboBox.ItemIndex >= 0 then
     PDataRec(InplaceCombo.Item.Data).ComboIdx := ComboBox.ItemIndex
  else
     InputValid := false;
end;

procedure TForm1.InplaceComboBeforeOperation(Sender: TObject;
  var DefaultConversion: Boolean);
var comboBox : TCombobox;
    i : integer;
begin
  ComboBox := InplaceCombo.Editor;
  Combobox.Style := csDropDownList;
  Combobox.Items.Clear;
  for i := 0 to 4 do
    Combobox.Items.Add(EnumData[i]);
  ComboBox.ItemIndex := PDataRec(InplaceCombo.Item.Data).ComboIdx;
end;

end.

