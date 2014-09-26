unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElTree, StdCtrls, ElHeader, ElXPThemedControl;

type
  TForm1 = class(TForm)
    ElTree1: TElTree;
    ElTree2: TElTree;
    Label1: TLabel;
    Label2: TLabel;
    procedure FormShow(Sender: TObject);
    procedure ElTree1ItemDraw(Sender: TObject; Item: TElTreeItem;
      Surface: TCanvas; R: TRect; SectionIndex: Integer);
    procedure ElTree2ItemDraw(Sender: TObject; Item: TElTreeItem;
      Surface: TCanvas; R: TRect; SectionIndex: Integer);
    procedure ElTree1HeaderColumnDraw(Sender: TCustomElHeader;
      Canvas: TCanvas; Section: TElHeaderSection; R: TRect;
      Pressed: Boolean);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

procedure TForm1.FormShow(Sender: TObject);
var TI : TElTreeItem;
    CS : TElCellStyle;
begin
  TI := ElTree2.Items[0];
  TI.UseStyles := true;
  CS := TI.AddStyle;
  CS.OwnerProps := true;
  CS.Style := elhsOwnerDraw;
end;

procedure TForm1.ElTree1ItemDraw(Sender: TObject; Item: TElTreeItem;
  Surface: TCanvas; R: TRect; SectionIndex: Integer);
begin
  Surface.Brush.Style := bsClear;
  DrawText(Surface.Handle, 'Owner-drawn cell', -1, R, DT_SINGLELINE or DT_CENTER);
end;

procedure TForm1.ElTree2ItemDraw(Sender: TObject; Item: TElTreeItem;
  Surface: TCanvas; R: TRect; SectionIndex: Integer);
begin
  Surface.Brush.Style := bsClear;
  if Item.StylesCount > 0 then
    DrawText(Surface.Handle, 'Owner-draw style defined by ElCellStyle', -1, R, DT_SINGLELINE or DT_LEFT)
  else
    DrawText(Surface.Handle, PChar(Format('cell #%d, Item #%d', [SectionIndex, Item.AbsoluteIndex])), -1, R, DT_SINGLELINE or DT_CENTER);
end;

procedure TForm1.ElTree1HeaderColumnDraw(Sender: TCustomElHeader;
  Canvas: TCanvas; Section: TElHeaderSection; R: TRect; Pressed: Boolean);
begin
  Canvas.Brush.Style := bsClear;
  DrawText(Canvas.Handle, 'Owner-drawn section', -1, R, DT_SINGLELINE or DT_CENTER or DT_VCENTER);
end;

end.

