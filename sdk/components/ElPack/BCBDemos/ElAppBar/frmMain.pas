unit frmMain;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElACtrls, ElSpin, ExtCtrls, ElPanel, ElClock, ElBtnCtl,
  ElPopBtn, ElAppBar, Menus, ElToolBar, ElXPThemedControl;

type
  TfrmBar = class(TElAppBar)
    LeftImage: TImage;
    TopImage: TImage;
    PopupMenu1: TPopupMenu;
    miExit: TMenuItem;
    ElToolBar1: TElToolBar;
    ElClock1: TElClock;
    btnOptions: TElPopupButton;
    procedure miExitClick(Sender: TObject);
    procedure btnOptionsClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
    procedure EdgeChange(Sender : TObject);
  protected
    procedure CreateParams(var Params: TCreateParams); override;
    procedure WMNcHitTest(var Msg : TWMNCHitTest); message WM_NCHITTEST;
  public
  end;

var
  frmBar: TfrmBar;

implementation

uses frmOpts;

{$R *.DFM}

procedure TfrmBar.WMNcHitTest(var Msg : TWMNCHitTest); 
var p : TPoint;
begin
  inherited;
  p := ScreenToClient(SmallPointToPoint(Msg.Pos));
  if PtInRect(LeftImage.BoundsRect, p) or PtInRect(TopImage.BoundsRect, p) then
  Msg.result := HTCaption;
end;

procedure TfrmBar.EdgeChange;
begin
  if Edge in [abeLeft, abeRight] then
  begin
    EltoolBar1.Orientation := eboVert;
    LeftImage.Visible := false;
    TopImage.Visible := true;
  end else
  begin
    EltoolBar1.Orientation := eboHorz;
    LeftImage.Visible := true;
    TopImage.Visible := false;
  end;
end;

procedure TfrmBar.miExitClick(Sender: TObject);
begin
  Close;
end;

procedure TfrmBar.btnOptionsClick(Sender: TObject);
var aEdges : TAppBarFlags;
begin
  with OptionsForm do
  begin
    LeftCB.Enabled   := Edge <> abeLeft;
    LeftCB.checked   := abfAllowLeft in Flags;

    RightCB.Enabled  := Edge <> abeRight;
    RightCB.checked  := abfAllowRight in Flags;

    TopCB.Enabled    := Edge <> abeTop;
    TopCB.checked    := abfAllowTop in Flags;

    BottomCB.Enabled := Edge <> abeBottom;
    BottomCB.Checked := abfAllowBottom in Flags;

    FloatingCB.Enabled := Edge <> abeFloat;
    FloatingCB.Checked := abfAllowFloat in Flags;

    KeepSizeCB.Checked := KeepSize;
    AutohideCB.Checked := AutoHide;
    TopmostCB.Checked  := AlwaysOnTop;
    OnScreenCB.Checked := PreventOffScreen;
    TaskBarCB.Checked  := TaskEntry <> abtHide;
  end;
  if OptionsForm.ShowModal = mrOk then
  with OptionsForm do
  begin
    aEdges := [];

    if LeftCB.Checked then include(aEdges, abfAllowLeft);
    if RightCB.Checked then include(aEdges, abfAllowRight);
    if TopCB.Checked then include(aEdges, abfAllowTop);
    if BottomCB.Checked then include(aEdges, abfAllowBottom);
    if FloatingCB.Checked then include(aEdges, abfAllowFloat);

    Flags := aEdges;
    KeepSize := KeepSizeCB.Checked;
    AutoHide := AutohideCB.Checked;
    AlwaysOnTop := TopmostCB.Checked;
    PreventOffScreen := OnScreenCB.Checked;
    if TaskBarCB.Checked  then TaskEntry := abtShow else TaskEntry := abtHide;
  end;
end;

procedure TfrmBar.FormCreate(Sender: TObject);
var r : TRect;
    hb, vb : integer;
begin
  OnEdgeChanged := EdgeChange;
  hb := Width - ClientWidth;
  vb := Height - ClientHeight;
  r.Left := ElToolBar1.BtnWidth + ElToolBar1.BtnOffsHorz * 2 + hb;
  r.Right := ElToolBar1.BtnWidth + ElToolBar1.BtnOffsHorz * 2 + hb;
  r.Top := ElToolBar1.BtnHeight + ElToolBar1.BtnOffsVert * 2 + vb;
  r.Bottom := ElToolBar1.BtnHeight + ElToolBar1.BtnOffsVert * 2 + vb;
  DockDims := r;
end;

procedure TFrmBar.CreateParams(var Params: TCreateParams);
begin
  inherited;
  Params.Style := WS_POPUP or WS_CLIPSIBLINGS or WS_CLIPCHILDREN or WS_THICKFRAME;
  // WS_EX_TOOLWINDOW is MANDATORY, otherwise everything stops working!!!!!!!!!!!
  Params.ExStyle := Params.ExStyle or WS_EX_CONTROLPARENT or WS_EX_TOOLWINDOW;
end;  {CreateParams}

end.

