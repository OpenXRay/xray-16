unit RenderWindow;

interface

uses
  Windows, Classes, extctrls, Messages, stdctrls, controls, 
  SysUtils, Forms, Menus, Graphics, Dialogs;

type
TD3DWindow = class(TCustomControl)
private
	FOnPaint: TNotifyEvent;
	procedure WMPaint(var Message: TWMPaint); message WM_PAINT;
protected
	FTBar: TPanel;
	FBBar: TPanel;
	FLBar: TPanel;
	FRBar: TPanel;

	procedure Paint; virtual;
protected
	FOnChangeFocus: TNotifyEvent;
	FBorderStyle: TBorderStyle;
	FFocusedColor: TColor;
    FUnfocusedColor: TColor;
    FDrawFocusRect: Boolean;
    procedure SetBorderStyle(Value: TBorderStyle);
	procedure CreateParams(var Params: TCreateParams); override;
	procedure CreateWindowHandle(const Params: TCreateParams); override;
    procedure ChangeFocus(p: boolean);
	property ParentColor default False;
	procedure SetDrawFocusRect(Value: Boolean);
	procedure SetFocusedColor(Value: TColor);
	procedure SetUnfocusedColor(Value: TColor);
public
	constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
	procedure DefaultHandler(var Message); override;
published
    property FocusedColor: TColor read FFocusedColor write SetFocusedColor default clYellow;
    property UnfocusedColor: TColor read FUnfocusedColor write SetUnfocusedColor default clGray;
    property DrawFocusRect: Boolean read FDrawFocusRect write SetDrawFocusRect default True;
	property TabStop default True;
	property OnChangeFocus: TNotifyEvent read FOnChangeFocus write FOnChangeFocus;
published
	property Align;
    property BorderStyle: TBorderStyle read FBorderStyle write SetBorderStyle default bsSingle;
	property Color;
	property Enabled;
	property Font;
	property PopupMenu;
	property ShowHint;
	property TabOrder;
	property Visible;
	property OnKeyDown;
	property OnKeyPress;
	property OnKeyUp;
	property OnMouseDown;
	property OnMouseMove;
	property OnMouseUp;
	property OnResize;
	property OnPaint: TNotifyEvent read FOnPaint write FOnPaint;
end;


const
	RWStyle = [csAcceptsControls, csCaptureMouse, csClickEvents, csOpaque, csReplicatable];
    
implementation

constructor TD3DWindow.Create(AOwner: TComponent);
begin
  	inherited Create(AOwner);
    FTBar			:= TPanel.Create(self);
    FTBar.Parent	:= self;
    FTBar.Align		:= alTop;
    FTBar.BevelInner:= bvNone;
    FTBar.BevelOuter:= bvNone;
    FTBar.Height	:= 1;
    
    FBBar			:= TPanel.Create(self);
    FBBar.Parent	:= self;
    FBBar.Align		:= alBottom;
    FBBar.BevelInner:= bvNone;
    FBBar.BevelOuter:= bvNone;
    FBBar.Height	:= 1;
    
    FLBar			:= TPanel.Create(self);
    FLBar.Parent	:= self;
    FLBar.Align		:= alLeft;
    FLBar.BevelInner:= bvNone;
    FLBar.BevelOuter:= bvNone;
    FLBar.Width		:= 1;
    
    FRBar			:= TPanel.Create(self);
    FRBar.Parent	:= self;
    FRBar.Align		:= alRight;
    FRBar.BevelInner:= bvNone;
    FRBar.BevelOuter:= bvNone;
    FRBar.Width		:= 1;
    
	ControlStyle 	:= RWStyle;
    color 			:= $00555555;
    width			:= 120;
    height			:= 120;
    TabStop 		:= True;              
    ParentColor 	:= False;
    FBorderStyle 	:= bsSingle;
    FDrawFocusRect 	:= true;
    FFocusedColor 	:= clYellow;
    FUnfocusedColor	:= clGray;
end;

destructor TD3DWindow.Destroy;
begin
  FTBar.Free;
  FBBar.Free;
  FLBar.Free;
  FRBar.Free;
  inherited Destroy;
end;

procedure TD3DWindow.ChangeFocus(p: boolean);
begin
  	if Assigned(FOnChangeFocus) then FOnChangeFocus(Self);
    Paint;
end;

procedure TD3DWindow.SetBorderStyle(Value: TBorderStyle);
begin
  if FBorderStyle <> Value then
  begin
    FBorderStyle := Value;
    RecreateWnd;
  end;
end;

procedure TD3DWindow.CreateParams(var Params: TCreateParams);
const
  BorderStyles: array[TBorderStyle] of DWORD = (0, WS_BORDER);
  CSHREDRAW: array[Boolean] of DWORD = (CS_HREDRAW, 0);
begin
  inherited CreateParams(Params);
  CreateSubClass(Params, 'LISTBOX');
  with Params do
  begin
    Style := Style or (WS_HSCROLL or WS_VSCROLL or LBS_HASSTRINGS or
      LBS_NOTIFY) or LBS_OWNERDRAWVARIABLE or LBS_USETABSTOPS or BorderStyles[FBorderStyle];
    if NewStyleControls and Ctl3D and (FBorderStyle = bsSingle) then
    begin
      Style := Style and not WS_BORDER;
      ExStyle := ExStyle or WS_EX_CLIENTEDGE;
    end;
    WindowClass.style := WindowClass.style and not (CSHREDRAW[UseRightToLeftAlignment] or CS_VREDRAW);
  end;
end;

procedure TD3DWindow.CreateWindowHandle(const Params: TCreateParams);
var
  P: TCreateParams;
begin
  if SysLocale.FarEast and (Win32Platform <> VER_PLATFORM_WIN32_NT) and
    ((Params.Style and ES_READONLY) <> 0) then
  begin
    // Work around Far East Win95 API/IME bug.
    P := Params;
    P.Style := P.Style and (not ES_READONLY);
    inherited CreateWindowHandle(P);
    if WindowHandle <> 0 then
      SendMessage(WindowHandle, EM_SETREADONLY, Ord(True), 0);
  end
  else
    inherited CreateWindowHandle(Params);
end;

procedure TD3DWindow.DefaultHandler(var Message);
begin
  case TMessage(Message).Msg of
    WM_SETFOCUS:begin
      if (Win32Platform = VER_PLATFORM_WIN32_WINDOWS) and
        not IsWindow(TWMSetFocus(Message).FocusedWnd) then
        TWMSetFocus(Message).FocusedWnd := 0;
      ChangeFocus(true);
    end;
    WM_KILLFOCUS:begin
      ChangeFocus(false);
    end;
  end;
  inherited;
end;
//------------------------------------------------------------------------------

procedure TD3DWindow.WMPaint(var Message: TWMPaint);
begin
	inherited;
    Paint;
end;

procedure TD3DWindow.Paint;
begin
	inherited;
	if Assigned(FOnPaint) then FOnPaint(Self);
    if (FDrawFocusRect) then
    begin
    	if (Focused) then 	
        begin
        	FTBar.Color := FFocusedColor;
        	FBBar.Color := FFocusedColor;
        	FLBar.Color := FFocusedColor;
        	FRBar.Color := FFocusedColor;
        end 
        else 
        begin
        	FTBar.Color := FUnfocusedColor;
        	FBBar.Color := FUnfocusedColor;
        	FLBar.Color := FUnfocusedColor;
        	FRBar.Color := FUnfocusedColor;
        end;
    end;
end;

procedure TD3DWindow.SetDrawFocusRect(Value: Boolean);
begin
  	if Value <> FDrawFocusRect then
  	begin
    	FDrawFocusRect 	:= Value;
        FTBar.Visible 	:= Value;
        FBBar.Visible 	:= Value;
        FLBar.Visible 	:= Value;
        FRBar.Visible 	:= Value;
        Repaint();
  	end;
end;

procedure TD3DWindow.SetFocusedColor(Value: TColor);
begin
  	if Value <> FFocusedColor then
  	begin
    	FFocusedColor := Value;
        Repaint();
  	end;
end;

procedure TD3DWindow.SetUnfocusedColor(Value: TColor);
begin
  	if Value <> FUnfocusedColor then
  	begin
    	FUnfocusedColor := Value;
        Repaint();
  	end;
end;

end.

