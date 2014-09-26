unit multi_edit;
{$S-,W-,R-,H+,X+}
{$C PRELOAD}
interface
uses Messages, Windows, SysUtils, Classes, Controls, Forms, Menus, Graphics,
  	StdCtrls, ExtCtrls, ComCtrls, CommCtrl;
type
TSpinButtonState = (sbNotDown, sbTopDown, sbBottomDown);
TValueType = (vtInt, vtFloat, vtHex);
TSpinButtonKind = (bkStandard, bkDiagonal, bkLightWave);
TLWButtonState = (sbLWNotDown, sbLWDown);

//TMultiObjSpinEdit=class;// forward;

{ TMultiObjSpinButton }
TMultiObjSpinButton = class(TGraphicControl)
private
    FDown: TSpinButtonState;
    FUpBitmap: TBitmap;
    FDownBitmap: TBitmap;
    FDragging: Boolean;
    FInvalidate: Boolean;
    FTopDownBtn: TBitmap;
    FBottomDownBtn: TBitmap;
    FRepeatTimer: TTimer;
    FNotDownBtn: TBitmap;
    FLastDown: TSpinButtonState;
    FFocusControl: TWinControl;
    FOnTopClick: TNotifyEvent;
    FOnBottomClick: TNotifyEvent;
    procedure TopClick;
    procedure BottomClick;
    procedure GlyphChanged(Sender: TObject);
    function GetUpGlyph: TBitmap;
    function GetDownGlyph: TBitmap;
    procedure SetUpGlyph(Value: TBitmap);
    procedure SetDownGlyph(Value: TBitmap);
    procedure SetDown(Value: TSpinButtonState);
    procedure SetFocusControl(Value: TWinControl);
    procedure DrawAllBitmap;
    procedure DrawBitmap(ABitmap: TBitmap; ADownState: TSpinButtonState);
    procedure TimerExpired(Sender: TObject);
    procedure CMEnabledChanged(var Message: TMessage); message CM_ENABLEDCHANGED;
protected
    procedure Paint; override;
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure MouseMove(Shift: TShiftState; X, Y: Integer); override;
    procedure MouseUp(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure Notification(AComponent: TComponent;
      Operation: TOperation); override;
public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    property Down: TSpinButtonState read FDown write SetDown default sbNotDown;
published
    property DragCursor;
    property DragMode;
    property Enabled;
    property Visible;
    property DownGlyph: TBitmap read GetDownGlyph write SetDownGlyph;
    property UpGlyph: TBitmap read GetUpGlyph write SetUpGlyph;
    property FocusControl: TWinControl read FFocusControl write SetFocusControl;
    property ShowHint;
    property ParentShowHint;
    property OnBottomClick: TNotifyEvent read FOnBottomClick write FOnBottomClick;
    property OnTopClick: TNotifyEvent read FOnTopClick write FOnTopClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnStartDrag;
end;

TLWNotifyEvent = procedure(Sender: TObject; Val: integer) of object;

TMultiObjLWButton = class(TGraphicControl)
private
    FDown: TLWButtonState;
    FLWBitmap: TBitmap;
    FDragging: Boolean;
    FInvalidate: Boolean;
    FLWDownBtn: TBitmap;
    FLWNotDownBtn: TBitmap;
    FFocusControl: TWinControl;
    FOnLWChange: TLWNotifyEvent;
    FSens: real;
    FAccum: real;
    procedure LWChange(Sender: TObject; Val: integer);
    procedure GlyphChanged(Sender: TObject);
    function  GetGlyph: TBitmap;
    procedure SetGlyph(Value: TBitmap);
    procedure SetDown(Value: TLWButtonState);
    procedure SetFocusControl(Value: TWinControl);
    procedure DrawAllBitmap;
    procedure DrawBitmap(ABitmap: TBitmap; ADownState: TLWButtonState);
    procedure CMEnabledChanged(var Message: TMessage); message CM_ENABLEDCHANGED;
protected
    procedure Paint; override;
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure MouseMove(Shift: TShiftState; X, Y: Integer); override;
    procedure MouseUp(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure Notification(AComponent: TComponent;
      Operation: TOperation); override;
    procedure SetSens(Value: real);
public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    property Down: TLWButtonState read FDown write SetDown default sbLWNotDown;
    procedure Invalidate; override;
published
    property LWSensitivity: real read FSens write SetSens;
    property DragCursor;
    property DragMode;
    property Enabled;
    property Visible;
    property LWGlyph: TBitmap read GetGlyph write SetGlyph;
    property FocusControl: TWinControl read FFocusControl write SetFocusControl;
    property OnLWChange: TLWNotifyEvent read FOnLWChange write FOnLWChange;
    property ShowHint;
    property ParentShowHint;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnStartDrag;
end;

TMultiObjSpinEdit = class(TCustomEdit)
private
    bIsMulti: boolean;
    bChanged: boolean;
    BeforeValue: Extended;
    StartColor: TColor;
    FBtnColor: TColor;
    FAlignment: TAlignment;
    FMinValue: Extended;
    FMaxValue: Extended;
    FIncrement: Extended;
    FButtonWidth: Integer;
    FDecimal: Byte;
    FChanging: Boolean;
    FEditorEnabled: Boolean;
    FValueType: TValueType;
    FButton: TMultiObjSpinButton;
    FBtnWindow: TWinControl;
    FArrowKeys: Boolean;
    FOnTopClick: TNotifyEvent;
    FOnBottomClick: TNotifyEvent;
    FButtonKind: TSpinButtonKind;
    FUpDown: TCustomUpDown;
    FLWButton: TMultiObjLWButton;
    FOnLWChange: TLWNotifyEvent;
    FSens: real;
    function GetButtonKind: TSpinButtonKind;
    procedure SetButtonKind(Value: TSpinButtonKind);
    procedure UpDownClick(Sender: TObject; Button: TUDBtnType);
    procedure LWChange(Sender: TObject; Val: integer);
    function GetValue: Extended;
    function CheckValue(NewValue: Extended): Extended;
    function GetAsInteger: Longint;
    function IsIncrementStored: Boolean;
    function IsMaxStored: Boolean;
    function IsMinStored: Boolean;
    function IsValueStored: Boolean;
    procedure SetArrowKeys(Value: Boolean);
    procedure SetAsInteger(NewValue: Longint);
    procedure SetValue(NewValue: Extended);
    procedure SetValueType(NewType: TValueType);
	procedure SetButtonWidth(NewValue: integer);
    procedure SetDecimal(NewValue: Byte);
    function GetButtonWidth: Integer;
    procedure RecreateButton;
    procedure ResizeButton;
    procedure SetEditRect;
    procedure SetAlignment(Value: TAlignment);
    procedure WMSize(var Message: TWMSize); message WM_SIZE;
    procedure CMEnter(var Message: TMessage); message CM_ENTER;
    procedure CMExit(var Message: TCMExit); message CM_EXIT;
    procedure WMPaste(var Message: TWMPaste); message WM_PASTE;
    procedure WMCut(var Message: TWMCut); message WM_CUT;
    procedure CMCtl3DChanged(var Message: TMessage); message CM_CTL3DCHANGED;
    procedure CMEnabledChanged(var Message: TMessage); message CM_ENABLEDCHANGED;
    procedure CMFontChanged(var Message: TMessage); message CM_FONTCHANGED;
	procedure SetBtnColor(Value: TColor);
protected
    procedure Change; override;
    function IsValidChar(Key: Char): Boolean; virtual;
    procedure UpClick(Sender: TObject); virtual;
    procedure DownClick(Sender: TObject); virtual;
    procedure KeyDown(var Key: Word; Shift: TShiftState); override;
    procedure KeyPress(var Key: Char); override;
    procedure CreateParams(var Params: TCreateParams); override;
    procedure CreateWnd; override;
    procedure SetSens(Val: real);
    function  GetSens():real;
public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    property AsInteger: Longint read GetAsInteger write SetAsInteger default 0;
    property Text;
    procedure ObjFirstInit( v: Single );
    procedure ObjNextInit( v: Single );
    procedure ObjApplyFloat( var _to: Single );
    procedure ObjApplyInt( var _to: integer );
published
    property LWSensitivity: real read GetSens write SetSens;
    property Alignment: TAlignment read FAlignment write SetAlignment
      default taLeftJustify;
    property ArrowKeys: Boolean read FArrowKeys write SetArrowKeys default True;
    property BtnColor: TColor read FBtnColor write SetBtnColor default clBtnFace;
    property ButtonKind: TSpinButtonKind read FButtonKind write SetButtonKind default bkStandard;
    property Decimal: Byte read FDecimal write SetDecimal default 2;
    property ButtonWidth: Integer read FButtonWidth write SetButtonWidth default 14;
    property EditorEnabled: Boolean read FEditorEnabled write FEditorEnabled default True;
    property Increment: Extended read FIncrement write FIncrement stored IsIncrementStored;
    property MaxValue: Extended read FMaxValue write FMaxValue stored IsMaxStored;
    property MinValue: Extended read FMinValue write FMinValue stored IsMinStored;
    property ValueType: TValueType read FValueType write SetValueType default vtInt;
    property Value: Extended read GetValue write SetValue stored IsValueStored;
    property AutoSelect;
    property AutoSize;
    property BorderStyle;
    property Color;
    property Ctl3D;
    property DragCursor;
    property DragMode;
    property Enabled;
    property Font;
    property Anchors;
    property BiDiMode;
    property Constraints;
    property DragKind;
    property ParentBiDiMode;
    property ImeMode;
    property ImeName;
    property MaxLength;
    property ParentColor;
    property ParentCtl3D;
    property ParentFont;
    property ParentShowHint;
    property PopupMenu;
    property ReadOnly;
    property ShowHint;
    property TabOrder;
    property TabStop;
    property Visible;
    property OnLWChange: TLWNotifyEvent read FOnLWChange write FOnLWChange;
    property OnBottomClick: TNotifyEvent read FOnBottomClick write FOnBottomClick;
    property OnTopClick: TNotifyEvent read FOnTopClick write FOnTopClick;
    property OnChange;
    property OnClick;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnEnter;
    property OnExit;
    property OnKeyDown;
    property OnKeyPress;
    property OnKeyUp;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
    property OnStartDrag;
    property OnContextPopup;
    property OnMouseWheelDown;
    property OnMouseWheelUp;
    property OnEndDock;
    property OnStartDock;
end;

implementation
{$R *.res}
//------------------------------------------------------------------------------

const
  sSpinUpBtn    = 'MOSPINUP';
  sSpinDownBtn  = 'MOSPINDOWN';
  sSpinLW  		= 'MOSPINLW';

const
  InitRepeatPause = 400; { pause before repeat timer (ms) }
  RepeatPause     = 100;

{ TMultiObjLWButton }
constructor TMultiObjLWButton.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FLWBitmap := TBitmap.Create;
  FLWBitmap.Handle := LoadBitmap(HInstance, sSpinLW);
  FLWBitmap.OnChange := GlyphChanged;
  Height := 20;
  Width := 20;
  FLWDownBtn := TBitmap.Create;
  FLWNotDownBtn := TBitmap.Create;
  FAccum := 0;
  DrawAllBitmap;
end;

destructor TMultiObjLWButton.Destroy;
begin
  FLWDownBtn.Free;
  FLWNotDownBtn.Free;
  FLWBitmap.Free;
  inherited Destroy;
end;

procedure TMultiObjLWButton.Invalidate;
begin
  FInvalidate := true;
  inherited;
end;

procedure TMultiObjLWButton.GlyphChanged(Sender: TObject);
begin
  FInvalidate := True;
  Invalidate;
end;

function TMultiObjLWButton.GetGlyph: TBitmap;
begin
  Result := FLWBitmap;
end;

procedure TMultiObjLWButton.SetGlyph(Value: TBitmap);
begin
  if Value <> nil then FLWBitmap.Assign(Value)
  else FLWBitmap.Handle := LoadBitmap(HInstance, sSpinLW);
end;

procedure TMultiObjLWButton.SetDown(Value: TLWButtonState);
var
  OldState: TLWButtonState;
begin
  OldState := FDown;
  FDown := Value;
  if OldState <> FDown then Repaint;
end;

procedure TMultiObjLWButton.SetSens(Value: real);
begin
  if (Value<0) then FSens := 0
  else if (Value>=100000) then FSens := 100000
  else FSens := Value;
end;

procedure TMultiObjLWButton.SetFocusControl(Value: TWinControl);
begin
  FFocusControl := Value;
  if Value <> nil then Value.FreeNotification(Self);
end;

procedure TMultiObjLWButton.Notification(AComponent: TComponent;
  Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (Operation = opRemove) and (AComponent = FFocusControl) then
    FFocusControl := nil;
end;

procedure TMultiObjLWButton.Paint;
begin
  if not Enabled and not (csDesigning in ComponentState) then
    FDragging := False;
  if (FLWNotDownBtn.Height <> Height) or (FLWNotDownBtn.Width <> Width) or
    FInvalidate then DrawAllBitmap;
  FInvalidate := False;

  with Canvas do
    case FDown of
      sbLWNotDown: Draw(0, 0, FLWNotDownBtn);
      sbLWDown: Draw(0, 0, FLWDownBtn);
    end;
end;

procedure TMultiObjLWButton.DrawAllBitmap;
begin
  DrawBitmap(FLWDownBtn, sbLWDown);
  DrawBitmap(FLWNotDownBtn, sbLWNotDown);
end;

procedure TMultiObjLWButton.DrawBitmap(ABitmap: TBitmap; ADownState: TLWButtonState);
var
  R, RSrc: TRect;
  dRect: Integer;
begin
  ABitmap.Height := Height;
  ABitmap.Width := Width;
  with ABitmap.Canvas do begin
    R := Bounds(0, 0, Width, Height);
    Pen.Width := 1;
    Brush.Color := TMultiObjSpinEdit(Owner).FBtnColor;
    Brush.Style := bsSolid;
    FillRect(R);
    { lw button }
    if ADownState = sbLWDown then Pen.Color := clBtnShadow
    else Pen.Color := clBtnHighlight;
    MoveTo(0, Height - 2);
    LineTo(0, 0);
    LineTo(Width - 1, 0);
	if ADownState = sbLWDown then Pen.Color := clBtnHighlight
    else Pen.Color := clBtnShadow;
    MoveTo(0, Height - 1);
    LineTo(Width - 1, Height - 1);
    LineTo(Width - 1, 0);
    { lw glyph }
    dRect := 0;
    if ADownState = sbLWDown then Inc(dRect);
    R := Bounds(Round((Width / 2) - (FLWBitmap.Width / 2)) + dRect,
      Round((Height / 2) - (FLWBitmap.Height / 2)) + dRect, FLWBitmap.Width,
      FLWBitmap.Height);
    RSrc := Bounds(0, 0, FLWBitmap.Width, FLWBitmap.Height);

    BrushCopy(R, FLWBitmap, RSrc, FLWBitmap.TransparentColor);
  end;
end;

procedure TMultiObjLWButton.CMEnabledChanged(var Message: TMessage);
begin
  inherited;
  FInvalidate := True;
  Invalidate;
end;

var
    centerPos,savePos: TPoint;

procedure TMultiObjLWButton.MouseDown(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  	inherited MouseDown(Button, Shift, X, Y);
  	if (Button = mbLeft) and Enabled then 
    begin
    	if (FFocusControl <> nil) and FFocusControl.TabStop and
      		FFocusControl.CanFocus and (GetFocus <> FFocusControl.Handle) then
        FFocusControl.SetFocus;
    	if FDown = sbLWNotDown then 
    	begin
      		FDown := sbLWDown;
            GetCursorPos(savePos);
			ShowCursor(false);
            centerPos.x := round(GetSystemMetrics(SM_CXSCREEN)/2);
            centerPos.y := round(GetSystemMetrics(SM_CYSCREEN)/2);
            SetCursorPos(centerPos.x,centerPos.y);
	        Repaint;
      	end;
	end;
    FDragging := True;
end;

procedure TMultiObjLWButton.MouseMove(Shift: TShiftState; X, Y: Integer);
var
  dx: real;
  tp: TPoint;
  
begin
  	inherited MouseMove(Shift, X, Y);
  	if FDragging then 
  	begin
		GetCursorPos(tp);

    	dx := tp.x-centerPos.x;
        FAccum := FAccum+FSens*dx;
        
        if ((FAccum>1)or(FAccum<-1)) then 
        begin
			LWChange(self, round(FAccum));
            FAccum := 0;
            SetCursorPos(centerPos.x,centerPos.y);
			Application.ProcessMessages();
        end;
    end;
end;

procedure TMultiObjLWButton.MouseUp(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  	inherited MouseUp(Button, Shift, X, Y);
  	if FDragging then begin
    	FDragging := False;
      	FDown := sbLWNotDown;
      	Repaint;
		SetCursorPos(savePos.x,savePos.y);
		ShowCursor(true);
    end;
end;

procedure TMultiObjLWButton.LWChange(Sender: Tobject; Val: integer);
begin
  if Assigned(FOnLWChange) then begin
    FOnLWChange(Sender,Val);
    if not (csLButtonDown in ControlState) then FDown := sbLWNotDown;
  end;
end;


{ TMultiObjSpinButton }
constructor TMultiObjSpinButton.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FUpBitmap := TBitmap.Create;
  FDownBitmap := TBitmap.Create;
  FUpBitmap.Handle := LoadBitmap(HInstance, sSpinUpBtn);
  FDownBitmap.Handle := LoadBitmap(HInstance, sSpinDownBtn);
  FUpBitmap.OnChange := GlyphChanged;
  FDownBitmap.OnChange := GlyphChanged;
  Height := 20;
  Width := 20;
  FTopDownBtn := TBitmap.Create;
  FBottomDownBtn := TBitmap.Create;
  FNotDownBtn := TBitmap.Create;
  DrawAllBitmap;
  FLastDown := sbNotDown;
end;

destructor TMultiObjSpinButton.Destroy;
begin
  FTopDownBtn.Free;
  FBottomDownBtn.Free;
  FNotDownBtn.Free;
  FUpBitmap.Free;
  FDownBitmap.Free;
  FRepeatTimer.Free;
  inherited Destroy;
end;

procedure TMultiObjSpinButton.GlyphChanged(Sender: TObject);
begin
  FInvalidate := True;
  Invalidate;
end;

function TMultiObjSpinButton.GetUpGlyph: TBitmap;
begin
  Result := FUpBitmap;
end;

procedure TMultiObjSpinButton.SetUpGlyph(Value: TBitmap);
begin
  if Value <> nil then FUpBitmap.Assign(Value)
  else FUpBitmap.Handle := LoadBitmap(HInstance, sSpinUpBtn);
end;

function TMultiObjSpinButton.GetDownGlyph: TBitmap;
begin
  Result := FDownBitmap;
end;

procedure TMultiObjSpinButton.SetDownGlyph(Value: TBitmap);
begin
  if Value <> nil then FDownBitmap.Assign(Value)
  else FDownBitmap.Handle := LoadBitmap(HInstance, sSpinDownBtn);
end;

procedure TMultiObjSpinButton.SetDown(Value: TSpinButtonState);
var
  OldState: TSpinButtonState;
begin
  OldState := FDown;
  FDown := Value;
  if OldState <> FDown then Repaint;
end;

procedure TMultiObjSpinButton.SetFocusControl(Value: TWinControl);
begin
  FFocusControl := Value;
  if Value <> nil then Value.FreeNotification(Self);
end;

procedure TMultiObjSpinButton.Notification(AComponent: TComponent;
  Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (Operation = opRemove) and (AComponent = FFocusControl) then
    FFocusControl := nil;
end;

procedure TMultiObjSpinButton.Paint;
begin
  if not Enabled and not (csDesigning in ComponentState) then
    FDragging := False;
  if (FNotDownBtn.Height <> Height) or (FNotDownBtn.Width <> Width) or
    FInvalidate then DrawAllBitmap;
  FInvalidate := False;
  with Canvas do
    case FDown of
      sbNotDown: Draw(0, 0, FNotDownBtn);
      sbTopDown: Draw(0, 0, FTopDownBtn);
      sbBottomDown: Draw(0, 0, FBottomDownBtn);
    end;
end;

procedure TMultiObjSpinButton.DrawAllBitmap;
begin
  DrawBitmap(FTopDownBtn, sbTopDown);
  DrawBitmap(FBottomDownBtn, sbBottomDown);
  DrawBitmap(FNotDownBtn, sbNotDown);
end;

procedure TMultiObjSpinButton.DrawBitmap(ABitmap: TBitmap; ADownState: TSpinButtonState);
var
  R, RSrc: TRect;
  dRect: Integer;
  {Temp: TBitmap;}
begin
  ABitmap.Height := Height;
  ABitmap.Width := Width;
  with ABitmap.Canvas do begin
    R := Bounds(0, 0, Width, Height);
    Pen.Width := 1;
    Brush.Color := TMultiObjSpinEdit(Owner).FBtnColor;
    Brush.Style := bsSolid;
    FillRect(R);
    { buttons frame }
//    Pen.Color := clWindowFrame;
//    Rectangle(0, 0, Width, Height);
//    MoveTo(-1, Height);
//    LineTo(Width, -1);
    { top button }
    if ADownState = sbTopDown then Pen.Color := clBtnShadow
    else Pen.Color := clBtnHighlight;
    MoveTo(0, Height - 3);
    LineTo(0, 0);
    LineTo(Width - 2, 0);
    if ADownState = sbTopDown then Pen.Color := clBtnHighlight
      else Pen.Color := clBtnShadow;
    if ADownState <> sbTopDown then begin
      MoveTo(0, Height - 2);
      LineTo(Width - 2, 0);
    end;
    { bottom button }
    if ADownState = sbBottomDown then Pen.Color := clBtnHighlight
      else Pen.Color := clBtnShadow;
    MoveTo(1, Height - 1);
    LineTo(Width - 1, Height - 1);
    LineTo(Width - 1, 0);
    if ADownState = sbBottomDown then Pen.Color := clBtnShadow
      else Pen.Color := clBtnHighlight;
    MoveTo(2, Height - 2);
    LineTo(Width - 1, 1);
    { top glyph }
    dRect := 1;
    if ADownState = sbTopDown then Inc(dRect);
    R := Bounds(Round((Width / 4) - (FUpBitmap.Width / 2)) + dRect,
      Round((Height / 4) - (FUpBitmap.Height / 2)) + dRect, FUpBitmap.Width,
      FUpBitmap.Height);
    RSrc := Bounds(0, 0, FUpBitmap.Width, FUpBitmap.Height);
    {
    if Self.Enabled or (csDesigning in ComponentState) then
      BrushCopy(R, FUpBitmap, RSrc, FUpBitmap.TransparentColor)
    else begin
      Temp := CreateDisabledBitmap(FUpBitmap, clBlack);
      try
        BrushCopy(R, Temp, RSrc, Temp.TransparentColor);
      finally
        Temp.Free;
      end;
    end;
    }
    BrushCopy(R, FUpBitmap, RSrc, FUpBitmap.TransparentColor);
    { bottom glyph }
    R := Bounds(Round((3 * Width / 4) - (FDownBitmap.Width / 2)) - 1,
      Round((3 * Height / 4) - (FDownBitmap.Height / 2)) - 1,
      FDownBitmap.Width, FDownBitmap.Height);
    RSrc := Bounds(0, 0, FDownBitmap.Width, FDownBitmap.Height);
    {
    if Self.Enabled or (csDesigning in ComponentState) then
      BrushCopy(R, FDownBitmap, RSrc, FDownBitmap.TransparentColor)
    else begin
      Temp := CreateDisabledBitmap(FDownBitmap, clBlack);
      try
        BrushCopy(R, Temp, RSrc, Temp.TransparentColor);
      finally
        Temp.Free;
      end;
    end;
    }
    BrushCopy(R, FDownBitmap, RSrc, FDownBitmap.TransparentColor);
    if ADownState = sbBottomDown then begin
      Pen.Color := clBtnShadow;
      MoveTo(3, Height - 2);
      LineTo(Width - 1, 2);
    end;
  end;
end;

procedure TMultiObjSpinButton.CMEnabledChanged(var Message: TMessage);
begin
  inherited;
  FInvalidate := True;
  Invalidate;
end;

procedure TMultiObjSpinButton.TopClick;
begin
  if Assigned(FOnTopClick) then begin
    FOnTopClick(Self);
    if not (csLButtonDown in ControlState) then FDown := sbNotDown;
  end;
end;

procedure TMultiObjSpinButton.BottomClick;
begin
  if Assigned(FOnBottomClick) then begin
    FOnBottomClick(Self);
    if not (csLButtonDown in ControlState) then FDown := sbNotDown;
  end;
end;

procedure TMultiObjSpinButton.MouseDown(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  inherited MouseDown(Button, Shift, X, Y);
  if (Button = mbLeft) and Enabled then begin
    if (FFocusControl <> nil) and FFocusControl.TabStop and
      FFocusControl.CanFocus and (GetFocus <> FFocusControl.Handle) then
        FFocusControl.SetFocus;
    if FDown = sbNotDown then begin
      FLastDown := FDown;
      if Y > (-(Height/Width) * X + Height) then begin
        FDown := sbBottomDown;
        BottomClick;
      end
      else begin
        FDown := sbTopDown;
        TopClick;
      end;
      if FLastDown <> FDown then begin
        FLastDown := FDown;
        Repaint;
      end;
      if FRepeatTimer = nil then FRepeatTimer := TTimer.Create(Self);
      FRepeatTimer.OnTimer := TimerExpired;
      FRepeatTimer.Interval := InitRepeatPause;
      FRepeatTimer.Enabled := True;
    end;
    FDragging := True;
  end;
end;

procedure TMultiObjSpinButton.MouseMove(Shift: TShiftState; X, Y: Integer);
var
  NewState: TSpinButtonState;
begin
  inherited MouseMove(Shift, X, Y);
  if FDragging then begin
    if (X >= 0) and (X <= Width) and (Y >= 0) and (Y <= Height) then begin
      NewState := FDown;
      if Y > (-(Width / Height) * X + Height) then begin
        if (FDown <> sbBottomDown) then begin
          if FLastDown = sbBottomDown then FDown := sbBottomDown
          else FDown := sbNotDown;
          if NewState <> FDown then Repaint;
        end;
      end
      else begin
        if (FDown <> sbTopDown) then begin
          if (FLastDown = sbTopDown) then FDown := sbTopDown
          else FDown := sbNotDown;
          if NewState <> FDown then Repaint;
        end;
      end;
    end else
      if FDown <> sbNotDown then begin
        FDown := sbNotDown;
        Repaint;
      end;
  end;
end;

procedure TMultiObjSpinButton.MouseUp(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  inherited MouseUp(Button, Shift, X, Y);
  if FDragging then begin
    FDragging := False;
    if (X >= 0) and (X <= Width) and (Y >= 0) and (Y <= Height) then begin
      FDown := sbNotDown;
      FLastDown := sbNotDown;
      Repaint;
    end;
  end;
end;

procedure TMultiObjSpinButton.TimerExpired(Sender: TObject);
begin
  FRepeatTimer.Interval := RepeatPause;
  if (FDown <> sbNotDown) and MouseCapture then begin
    try
      if FDown = sbBottomDown then BottomClick else TopClick;
    except
      FRepeatTimer.Enabled := False;
      raise;
    end;
  end;
end;

function DefBtnWidth: Integer;
begin
  Result := GetSystemMetrics(SM_CXVSCROLL);
  if Result > 15 then Result := 15;
end;

type
    TMultiObjUpDown = class(TCustomUpDown)
    private
    	FChanging: Boolean;
    	procedure ScrollMessage(var Message: TWMVScroll);
    	procedure WMHScroll(var Message: TWMHScroll); message CN_HSCROLL;
	    procedure WMVScroll(var Message: TWMVScroll); message CN_VSCROLL;
    	procedure WMSize(var Message: TWMSize); message WM_SIZE;
    public
	    constructor Create(AOwner: TComponent); override;
    	destructor Destroy; override;
    published
	    property OnClick;
    end;

constructor TMultiObjUpDown.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  Orientation := udVertical;
  Min := -1;
  Max := 1;
  Position := 0;
end;

destructor TMultiObjUpDown.Destroy;
begin
  OnClick := nil;
  inherited Destroy;
end;

procedure TMultiObjUpDown.ScrollMessage(var Message: TWMVScroll);
begin
  if Message.ScrollCode = SB_THUMBPOSITION then begin
    if not FChanging then begin
      FChanging := True;
      try
        if Message.Pos > 0 then Click(btNext)
        else if Message.Pos < 0 then Click(btPrev);
        if HandleAllocated then
          SendMessage(Handle, UDM_SETPOS, 0, 0);
      finally
        FChanging := False;
      end;
    end;
  end;
end;

procedure TMultiObjUpDown.WMHScroll(var Message: TWMHScroll);
begin
  ScrollMessage(TWMVScroll(Message));
end;

procedure TMultiObjUpDown.WMVScroll(var Message: TWMVScroll);
begin
  ScrollMessage(Message);
end;

procedure TMultiObjUpDown.WMSize(var Message: TWMSize);
begin
  inherited;
  if Width <> DefBtnWidth then Width := DefBtnWidth;
end;
{ TMultiObjSpinEdit }

constructor TMultiObjSpinEdit.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  Text := '0';
  ControlStyle := ControlStyle - [csSetCaption];
  FIncrement := 1.0;
  FDecimal := 2;
  FButtonWidth := 14;
  FEditorEnabled := True;
  FButtonKind := bkStandard;
  FArrowKeys := True;
  StartColor := 0;
  FBtnColor := clBtnFace;
  LWSensitivity := 1;
  RecreateButton;
end;

destructor TMultiObjSpinEdit.Destroy;
begin
  	Destroying;
  	FChanging := True;
  	if FButton <> nil then
    begin
    	FButton.Free;
    	FButton := nil;
    	FBtnWindow.Free;
    	FBtnWindow := nil;
  	end;
  	if FUpDown <> nil then
    begin
    	FUpDown.Free;
    	FUpDown := nil;
  	end;
    if FLWButton <> nil then
    begin
    	FLWButton.Free;
        FLWButton := nil;
    	FBtnWindow.Free;
    	FBtnWindow := nil;
    end;
  	inherited Destroy;
end;

procedure TMultiObjSpinEdit.RecreateButton;
begin
  	if (csDestroying in ComponentState) then Exit;
  	FButton.Free;
  	FButton := nil;
  	FUpDown.Free;
  	FUpDown := nil;
  	FLWButton.Free;
  	FLWButton := nil;
  	FBtnWindow.Free;
  	FBtnWindow := nil;
  	case (GetButtonKind) of
	  	bkStandard:  begin
            FUpDown := TMultiObjUpDown.Create(Self);
            with TMultiObjUpDown(FUpDown) do begin
              	Visible := True;
              	SetBounds(0, 0, DefBtnWidth, Self.Height);
              	Align := alRight;
              	Parent := Self;
              	OnClick := UpDownClick;
            end;
  		end;
  		bkDiagonal:  begin
            FBtnWindow := TWinControl.Create(Self);
            FBtnWindow.Visible := True;
            FBtnWindow.Parent := Self;
            FBtnWindow.SetBounds(0, 0, Height, Height);
            FButton := TMultiObjSpinButton.Create(Self);
            FButton.Visible := True;
            FButton.Parent := FBtnWindow;
            FButton.FocusControl := Self;
            FButton.OnTopClick := UpClick;
            FButton.OnBottomClick := DownClick;
            FButton.SetBounds(0, 0, FBtnWindow.Width, FBtnWindow.Height);
        end;
 		bkLightWave: begin
            FBtnWindow := TWinControl.Create(Self);
            FBtnWindow.Visible := True;
            FBtnWindow.Parent := Self;
            FBtnWindow.SetBounds(0, 0, Height, Height);
            FLWButton := TMultiObjLWButton.Create(Self);
            FLWButton.Visible := True;
            FLWButton.Parent := FBtnWindow;
            FLWButton.FocusControl := Self;
            FLWButton.OnLWChange := LWChange;
            FLWButton.SetBounds(0, 0, FBtnWindow.Width, FBtnWindow.Height);
            FLWButton.LWSensitivity := FSens;
        end;
  	end;
end;

procedure TMultiObjSpinEdit.SetArrowKeys(Value: Boolean);
begin
  FArrowKeys := Value;
  ResizeButton;
end;

function TMultiObjSpinEdit.GetButtonKind: TSpinButtonKind;
begin
  if NewStyleControls then Result := FButtonKind
  else Result := bkStandard;
end;

procedure TMultiObjSpinEdit.SetButtonKind(Value: TSpinButtonKind);
var
  OldKind: TSpinButtonKind;
begin
  OldKind := FButtonKind;
  FButtonKind := Value;
  if OldKind <> GetButtonKind then begin
    RecreateButton;
    ResizeButton;
    SetEditRect;
  end;
end;

procedure TMultiObjSpinEdit.UpDownClick(Sender: TObject; Button: TUDBtnType);
begin
  if TabStop and CanFocus then SetFocus;
  case Button of
    btNext: UpClick(Sender);
    btPrev: DownClick(Sender);
  end;
end;

procedure TMultiObjSpinEdit.LWChange(Sender: TObject; val: integer);
var
  OldText: string;
begin
  if TabStop and CanFocus then SetFocus;
  if ReadOnly then MessageBeep(0)
  else begin
    FChanging := True;
    try
      OldText := inherited Text;
      Value := Value + FIncrement*val;
    finally
      FChanging := False;
    end;
    if CompareText(inherited Text, OldText) <> 0 then begin
      Modified := True;
      Change;
      bChanged := true;
      if (StartColor<>0) then Color := StartColor;
    end;
    if Assigned(FOnLWChange) then FOnLWChange(Self,val);
  end;
end;

function TMultiObjSpinEdit.GetButtonWidth: Integer;
begin
  if FUpDown <> nil then Result := FUpDown.Width else
  if FButton <> nil then Result := FButton.Width else
  if FLWButton<>nil then Result := FLWButton.Width
  else Result := DefBtnWidth;
end;

procedure TMultiObjSpinEdit.ResizeButton;
var
  R: TRect;
begin
  	if FUpDown <> nil then begin
    	FUpDown.Width := DefBtnWidth;
    	FUpDown.Align := alRight;
  	end else if FButton <> nil then
  	begin { bkDiagonal }
    	if NewStyleControls and Ctl3D and (BorderStyle = bsSingle) then
      		R := Bounds(Width - FButtonWidth - 4, 0, FButtonWidth, Height - 4)
    	else
      		R := Bounds(Width - FButtonWidth, 0, FButtonWidth, Height);
    	with R do
      		FBtnWindow.SetBounds(Left, Top, Right - Left, Bottom - Top);
    	FButton.SetBounds(0, 0, FBtnWindow.Width, FBtnWindow.Height);
  	end else if FLWButton <> nil then
    begin {bkLightWave}
    	if NewStyleControls and Ctl3D and (BorderStyle = bsSingle) then
      		R := Bounds(Width - FButtonWidth - 4, 0, FButtonWidth, Height - 4)
    	else
      		R := Bounds(Width - FButtonWidth, 0, FButtonWidth, Height);
    	with R do
      		FBtnWindow.SetBounds(Left, Top, Right - Left, Bottom - Top);
    	FLWButton.SetBounds(0, 0, FBtnWindow.Width, FBtnWindow.Height);
    end;
end;

procedure TMultiObjSpinEdit.KeyDown(var Key: Word; Shift: TShiftState);
begin
  inherited KeyDown(Key, Shift);
  if ArrowKeys and (Key in [VK_UP, VK_DOWN]) then begin
    if Key = VK_UP then UpClick(Self)
    else if Key = VK_DOWN then DownClick(Self);
    Key := 0;
  end;
end;

procedure TMultiObjSpinEdit.Change;
begin
  if not FChanging then inherited Change;
end;

procedure TMultiObjSpinEdit.KeyPress(var Key: Char);
begin
  if not IsValidChar(Key) then begin
    Key := #0;
    MessageBeep(0)
  end;
  if Key <> #0 then begin
    bChanged := true;
    if (StartColor<>0) then Color := StartColor;
    inherited KeyPress(Key);
    if (Key = Char(VK_RETURN)) or (Key = Char(VK_ESCAPE)) then begin
      { must catch and remove this, since is actually multi-line }
      GetParentForm(Self).Perform(CM_DIALOGKEY, Byte(Key), 0);
      if Key = Char(VK_RETURN) then Key := #0;
    end;
  end;
end;

function TMultiObjSpinEdit.IsValidChar(Key: Char): Boolean;
var
  ValidChars: set of Char;
begin
	ValidChars := ['+', '-', '0'..'9'];
  	if ValueType = vtFloat then begin
  		// если нет точки вообще
    	if Pos(DecimalSeparator, Text) = 0 then
			ValidChars := ValidChars + [DecimalSeparator];
  		// если есть выделение
    	if (SelLength>0) and (Pos(DecimalSeparator, Text) <> 0) and (Pos(DecimalSeparator, SelText) <> 0) then
        	ValidChars := ValidChars + [DecimalSeparator];
		if Pos('E', AnsiUpperCase(Text)) = 0 then
      		ValidChars := ValidChars + ['e', 'E'];
  	end else if ValueType = vtHex then begin
    	ValidChars := ValidChars + ['A'..'F', 'a'..'f'];
  	end;
  	Result := (Key in ValidChars) or (Key < #32);
  	if not FEditorEnabled and Result and ((Key >= #32) or
    	(Key = Char(VK_BACK)) or (Key = Char(VK_DELETE))) then Result := False;
end;

procedure TMultiObjSpinEdit.CreateParams(var Params: TCreateParams);
const
  Alignments: array[TAlignment] of Longint = (ES_LEFT, ES_RIGHT, ES_CENTER);
begin
  inherited CreateParams(Params);
  Params.Style := Params.Style or ES_MULTILINE or WS_CLIPCHILDREN or Alignments[FAlignment];
end;

procedure TMultiObjSpinEdit.CreateWnd;
begin
  inherited CreateWnd;
  SetEditRect;
end;

procedure TMultiObjSpinEdit.SetEditRect;
var
  Loc: TRect;
begin
  SetRect(Loc, 0, 0, ClientWidth - GetButtonWidth - 2, ClientHeight + 1);
  SendMessage(Handle, EM_SETRECTNP, 0, Longint(@Loc));
end;

procedure TMultiObjSpinEdit.SetAlignment(Value: TAlignment);
begin
  if FAlignment <> Value then begin
    FAlignment := Value;
    RecreateWnd;
  end;
end;

procedure TMultiObjSpinEdit.WMSize(var Message: TWMSize);
var
  MinHeight: Integer;
begin
  inherited;
  MinHeight := 0;//GetMinHeight;
  { text edit bug: if size to less than minheight, then edit ctrl does
    not display the text }
  if Height < MinHeight then
    Height := MinHeight
  else begin
    ResizeButton;
    SetEditRect;
  end;
end;

procedure TMultiObjSpinEdit.UpClick(Sender: TObject);
var
  OldText: string;
begin
  if ReadOnly then MessageBeep(0)
  else begin
    FChanging := True;
    try
      OldText := inherited Text;
      Value := Value + FIncrement;
    finally
      FChanging := False;
    end;
    if CompareText(inherited Text, OldText) <> 0 then begin
      Modified := True;
      Change;
      bChanged := true;
      if (StartColor<>0) then Color := StartColor;
    end;
    if Assigned(FOnTopClick) then FOnTopClick(Self);
  end;
end;

procedure TMultiObjSpinEdit.DownClick(Sender: TObject);
var
  OldText: string;
begin
  if ReadOnly then MessageBeep(0)
  else begin
    FChanging := True;
    try
      OldText := inherited Text;
      Value := Value - FIncrement;
    finally
      FChanging := False;
    end;
    if CompareText(inherited Text, OldText) <> 0 then begin
      Modified := True;
      Change;
      bChanged := true;
      if (StartColor<>0) then Color := StartColor;
    end;
    if Assigned(FOnBottomClick) then FOnBottomClick(Self);
  end;
end;

procedure TMultiObjSpinEdit.CMFontChanged(var Message: TMessage);
begin
  inherited;
  ResizeButton;
  SetEditRect;
end;

procedure TMultiObjSpinEdit.CMCtl3DChanged(var Message: TMessage);
begin
  inherited;
  ResizeButton;
  SetEditRect;
end;

procedure TMultiObjSpinEdit.CMEnabledChanged(var Message: TMessage);
begin
  inherited;
  if FUpDown <> nil then begin
    FUpDown.Enabled := Enabled;
    ResizeButton;
  end;
  if FButton <> nil then FButton.Enabled := Enabled;
  if FLWButton <> nil then FLWButton.Enabled := Enabled;
end;

procedure TMultiObjSpinEdit.WMPaste(var Message: TWMPaste);
begin
  if not FEditorEnabled or ReadOnly then Exit;
  inherited;
end;

procedure TMultiObjSpinEdit.WMCut(var Message: TWMCut);
begin
  if not FEditorEnabled or ReadOnly then Exit;
  inherited;
end;

procedure TMultiObjSpinEdit.CMExit(var Message: TCMExit);
begin
  inherited;
  if CheckValue(Value) <> Value then SetValue(Value);
end;

procedure TMultiObjSpinEdit.CMEnter(var Message: TMessage);
begin
  if AutoSelect and not (csLButtonDown in ControlState) then SelectAll;
  inherited;
end;

function TMultiObjSpinEdit.GetValue: Extended;
begin
  try
    if ValueType = vtFloat then Result := StrToFloat(Text)
    else if ValueType = vtHex then Result := StrToInt('$' + Text)
    else Result := StrToInt(Text);
  except
    if ValueType = vtFloat then Result := FMinValue
    else Result := Trunc(FMinValue);
  end;
end;

procedure TMultiObjSpinEdit.SetValue(NewValue: Extended);
begin
  if ValueType = vtFloat then
    Text := FloatToStrF(CheckValue(NewValue), ffFixed, 15, FDecimal)
  else if ValueType = vtHex then
    Text := IntToHex(Round(CheckValue(NewValue)), 1)
  else
    Text := IntToStr(Round(CheckValue(NewValue)));
  if (BeforeValue<>NewValue) then bChanged := true;
end;

function TMultiObjSpinEdit.GetAsInteger: Longint;
begin
  Result := Trunc(GetValue);
end;

procedure TMultiObjSpinEdit.SetAsInteger(NewValue: Longint);
begin
  SetValue(NewValue);
end;

procedure TMultiObjSpinEdit.SetValueType(NewType: TValueType);
begin
  if FValueType <> NewType then begin
    FValueType := NewType;
    Value := GetValue;
    if FValueType in [vtInt, vtHex] then
    begin
      FIncrement := Round(FIncrement);
      if FIncrement = 0 then FIncrement := 1;
    end;
  end;
end;

function TMultiObjSpinEdit.IsIncrementStored: Boolean;
begin
  Result := FIncrement <> 1.0;
end;

function TMultiObjSpinEdit.IsMaxStored: Boolean;
begin
  Result := (MaxValue <> 0.0);
end;

function TMultiObjSpinEdit.IsMinStored: Boolean;
begin
  Result := (MinValue <> 0.0);
end;

function TMultiObjSpinEdit.IsValueStored: Boolean;
begin
  Result := (GetValue <> 0.0);
end;

procedure TMultiObjSpinEdit.SetDecimal(NewValue: Byte);
begin
  if FDecimal <> NewValue then begin
    FDecimal := NewValue;
    Value := GetValue;
  end;
end;

procedure TMultiObjSpinEdit.SetButtonWidth(NewValue: integer);
begin
  if FButtonWidth <> NewValue then begin
    FButtonWidth := NewValue;
	ResizeButton;
  	SetEditRect;
  end;
end;

function TMultiObjSpinEdit.CheckValue(NewValue: Extended): Extended;
begin
  Result := NewValue;
  if (FMaxValue <> FMinValue) then begin
    if NewValue < FMinValue then
      Result := FMinValue
    else if NewValue > FMaxValue then
      Result := FMaxValue;
  end;
end;

procedure TMultiObjSpinEdit.ObjFirstInit( v: Single );
begin
  	value         := v;
  	bIsMulti      := false;
  	bChanged      := false;
  	BeforeValue   := value;
    if (StartColor<>0) then Color := StartColor;
end;

procedure TMultiObjSpinEdit.ObjNextInit( v: Single );
begin
  bIsMulti := true;
  if (ABS(v-value)>0.000001) then 
  begin 
  	StartColor := Color;
  	Color := clGray;
  end;
end;

procedure TMultiObjSpinEdit.ObjApplyFloat( var _to: Single );
begin
  if (bChanged) then _to := value;
end;

procedure TMultiObjSpinEdit.ObjApplyInt( var _to: integer );
begin
  if (bChanged) then _to := round(value);
end;

procedure TMultiObjSpinEdit.SetSens(Val: real);
begin
  FSens := Val;
  if (FLWButton<>nil) then
  begin
    FLWButton.LWSensitivity := Val;
  end;
end;

function TMultiObjSpinEdit.GetSens():real;
begin
  result := FSens;
end;

procedure TMultiObjSpinEdit.SetBtnColor(Value: TColor);
begin
  if Value <> FBtnColor then
  begin
    FBtnColor := Value;
    Invalidate;
    if (FLWButton<>nil) then FLWButton.Invalidate;
    if (FButton<>nil) 	then FButton.Invalidate;
  end;
end;

end.

