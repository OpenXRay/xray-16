{ ----------------------------------------------------------------------------}
{ A Gradient Fill component for Delphi.                                       }
{ TGradientFill, Copyright 1995, Curtis White.  All Rights Reserved.          }
{ TGradient, Copyright 1997, Heiko Webers.  All Rights Reserved.           }
{ This component can be freely used and distributed in commercial and private }
{ environments, provied this notice is not modified in any way.               }
{ ----------------------------------------------------------------------------}
{ Feel free to contact me if you have any questions, comments or suggestions  }
{ at cwhite@teleport.com 						      }
{ Or me at heikowebers@usa.net                                                }
{ ----------------------------------------------------------------------------}
{ Date last modified:  12/06/97                                               }
{ ----------------------------------------------------------------------------}
{ ----------------------------------------------------------------------------}
{ TGradient v1.00                                                          }
{ ----------------------------------------------------------------------------}
{ Description:                                                                }
{   A gradient fill like in the new Netscape Communicator Options Box.        }
{ Features:                                                                   }
{   The begin and end colors can be any colors.                               }
{   The fill direction can be set to Right-To-Left or Left-To-Right.          }
{   The number of colors, between 1 and 255 can be set for the fill.          }
{   The Caption can be anything and anywhere on TGradient.		      }
{ ----------------------------------------------------------------------------}
{ ----------------------------------------------------------------------------}
{ Revision History:                                                           }
{ 1.00:  Initial release                                                      }
{ 1.00:  Changed to TGradient                                              }
{ ----------------------------------------------------------------------------}

unit Gradient;

interface

uses
  	Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs, extctrls;

type
{ Direction of fill }
TFillDirection = (fdLeftToRight, fdRightToLeft, fdUpToBottom, fdBottomToUp);
{ Range of valid colors }
TNumberOfColors = 1..255;

TGradient = class(TCustomPanel)
private
    { Variables for properties }
    FDirection: TFillDirection;
    FBeginColor: TColor;
    FEndColor: TColor;
    FAutoSize: Boolean;
    FNumberOfColors: TNumberOfColors;
    FFont : TFont;
    FCaption : String;
    FTextTop : Integer;
    FTextLeft: Integer;
    FBorder: boolean;
    FBorderWidth: integer;
    FBorderColor: TColor;

    { Procedures for setting property values }
    procedure SetFillDirection(Value: TFillDirection);
    procedure SetAutoSize(Value: Boolean);
    procedure SetBeginColor(Value: TColor);
    procedure SetEndColor(Value: TColor);
    procedure SetNumberOfColors(Value: TNumberOfColors);
    procedure SetFont(AFont: TFont);
    procedure SetCaption(Value: String);
    procedure SetTextTop(Value: Integer);
    procedure SetTextLeft(Value: Integer);

    procedure SetBorder(Value: boolean);
    procedure SetBorderWidth(Value: Integer);
    procedure SetBorderColor(Value: Tcolor);

    { Fill procedure }
    procedure GradientFill;

protected
	FOnPaint: TNotifyEvent;
    procedure Paint; override;
public
    property Canvas;
    constructor Create(AOwner: TComponent); override;
published
    { Repaint when autosized }
    property AutoSize: Boolean read FAutoSize write SetAutoSize default False;
    { Starting color of fill }
    property BeginColor: TColor read FBeginColor write SetBeginColor default clBlue;
    { Ending color of fill }
    property EndColor: TColor read FEndColor write SetEndColor default clBlack;
    { Direction of fill }
    property FillDirection: TFillDirection read FDirection write SetFillDirection default fdLeftToRight;
    { Number of colors to use in the fill (1 - 256) - default is 255.  If 1 }
    { then it uses the Begin Color.                                        }
    property NumberOfColors: TNumberOfColors read FNumberOfColors write SetNumberOfColors default 255;
    { Enable standard properties }
    property Font: TFont read FFont write SetFont;
    property Caption: String read FCaption write SetCaption;
    property TextTop: Integer read FTextTop write SetTextTop;
    property TextLeft: Integer read FTextLeft write SetTextLeft;
    property Border: boolean read FBorder write SetBorder;
    property BorderWidth: integer read FBorderWidth write SetBorderWidth;
    property BorderColor: TColor read FBorderColor write SetBorderColor;
    property Color;
    property Align;
    property DragCursor;
    property DragMode;
    property Enabled;
    property ParentShowHint;
    property PopupMenu;
    property ShowHint;
    property Visible;
    property OnClick;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
    property BorderStyle;
	property OnPaint: TNotifyEvent read FOnPaint write FOnPaint;
end;

implementation

{ TGradient }

{ Override the constructor to initialize variables }
constructor TGradient.Create(AOwner: TComponent);
begin
  	{ Inherit original constructor }
  	inherited Create(AOwner);
  	{ Add new initializations }
    Height := 25;
    Width := 400;
    FBeginColor := clGray;
    FEndColor := clBtnFace;
    FDirection := fdLeftToRight;
    FNumberOfColors := 255;
    FTextLeft := 12;
    FTextTop := 5;
    FFont := TFont.Create;
    FFOnt.Style := [fsbold];
    FCaption := 'TGradient';
    FBorder := false;
    FBorderWidth := 1;
    FBorderColor := clBlack;
end;

{ Set begin color when property is changed }
procedure TGradient.SetBeginColor(Value: TColor);
begin
    FBeginColor := Value;
    GradientFill;
    Invalidate;
end;

{ Set end color when property is changed }
procedure TGradient.SetEndColor(Value: TColor);
begin
    FEndColor := Value;
    GradientFill;
    Invalidate;
end;

{ Repaint the screen upon a resize }
procedure TGradient.SetAutoSize(Value: Boolean);
begin
  	FAutoSize := Value;
  	Invalidate;
end;

{ Set the number of colors to be used in the fill }
procedure TGradient.SetNumberOfColors(Value: TNumberOfColors);
begin
  	FNumberOfColors := Value;
  	Invalidate;
end;

// Set the Font
procedure TGradient.SetFont(AFont: TFont);
begin
	if AFont <> FFont then
  	begin
    	FFont.Assign(AFont);
    	GradientFill;
  	end;
end;

// Set the Caption on NG
procedure TGradient.SetCaption(Value: String);
begin
 	FCaption:= Value;
 	GradientFill;
end;

// Set the Position of the Caption  (Top)
procedure TGradient.SetTextTop(Value: Integer);
begin
 	FTextTop:= Value;
 	GradientFill;
end;

// Set the Position of the Caption (Left)
procedure TGradient.SetTextLeft(Value: Integer);
begin
 	FTextLeft:= Value;
 	GradientFill;
end;


{ Perform the fill when paint is called }
procedure TGradient.Paint;
begin
  	GradientFill;
	if Assigned(FOnPaint) then FOnPaint(Self);
end;

{ Gradient fill procedure - the actual routine }
procedure TGradient.GradientFill;
var
  { Set up working variables }
  	BeginRGBValue  : array[0..2] of Byte;    { Begin RGB values }
  	RGBDifference  : array[0..2] of integer; { Difference between begin and end }
                                           { RGB values                       }
  	ColorBand 	: TRect;    { Color band rectangular coordinates }
  	I         	: Integer;  { Color band index }
  	R         	: Byte;     { Color band Red value }
  	G         	: Byte;     { Color band Green value }
  	B         	: Byte;     { Color band Blue value }
  	WorkBmp   	: TBitmap;  { Off screen working bitmap }
    l,t,w,h     : integer;
    hbw			: integer;
begin

	if (FBorder) then begin
    	l := FBorderWidth;
        t := FBorderWidth;
        w := Width-FBorderWidth*2;
        h := Height-FBorderWidth*2;
        hbw:=round(FBorderWidth/2);
    end else begin
    	l := 0;
        t := 0;
        w := Width;
        h := Height;
        hbw:=0;
    end;


    { Create the working bitmap and set its width and height }
    WorkBmp 		:= TBitmap.Create;
    WorkBmp.Width 	:= w;
    WorkBmp.Height	:= h;

    { Use working bitmap to draw the gradient }
    with WorkBmp do begin
    	{ Extract the begin RGB values }
    	case FDirection of
     		fdUpToBottom, fdLeftToRight:
      		begin
                { Set the Red, Green and Blue colors }
                BeginRGBValue[0] := GetRValue (ColorToRGB (FBeginColor));
                BeginRGBValue[1] := GetGValue (ColorToRGB (FBeginColor));
                BeginRGBValue[2] := GetBValue (ColorToRGB (FBeginColor));
                { Calculate the difference between begin and end RGB values }
                RGBDifference[0] := GetRValue (ColorToRGB (FEndColor)) -
                                    BeginRGBValue[0];
                RGBDifference[1] := GetGValue (ColorToRGB (FEndColor)) -
                                    BeginRGBValue[1];
                RGBDifference[2] := GetBValue (ColorToRGB (FEndColor)) -
                                    BeginRGBValue[2];
            end;

            fdBottomToUp, fdRightToLeft:
            begin
                { Set the Red, Green and Blue colors }
                BeginRGBValue[0] := GetRValue (ColorToRGB (FEndColor));
                BeginRGBValue[1] := GetGValue (ColorToRGB (FEndColor));
                BeginRGBValue[2] := GetBValue (ColorToRGB (FEndColor));
                { Calculate the difference between begin and end RGB values }
                RGBDifference[0] := GetRValue (ColorToRGB (FBeginColor)) -
                                    BeginRGBValue[0];
                RGBDifference[1] := GetGValue (ColorToRGB (FBeginColor)) -
                                    BeginRGBValue[1];
                RGBDifference[2] := GetBValue (ColorToRGB (FBeginColor)) -
                                    BeginRGBValue[2];
            end;
        end;

        { Set the pen style and mode }
        Canvas.Pen.Style := psSolid;
        Canvas.Pen.Mode := pmCopy;

        case FDirection of

        { Calculate the color band's left and right coordinates }
        { for LeftToRight and RightToLeft fills }
            fdLeftToRight, fdRightToLeft:
            begin
                ColorBand.Top := 0;
                ColorBand.Bottom := h;
            end;
            fdUpToBottom, fdBottomToUp:
            begin
                ColorBand.Left := 0;
                ColorBand.Right := w;
            end;
        end;

            { Perform the fill }
        for I := 0 to FNumberOfColors do begin
            case FDirection of
                { Calculate the color band's left and right coordinates }
                fdLeftToRight, fdRightToLeft:
                begin
                    ColorBand.Left    := MulDiv (I    , w, FNumberOfColors);
                    ColorBand.Right := MulDiv (I + 1, w, FNumberOfColors);
                end;
                fdUpToBottom, fdBottomToUp:
                begin
                    ColorBand.Top    := MulDiv (I    , h, FNumberOfColors);
                    ColorBand.Bottom := MulDiv (I + 1, h, FNumberOfColors);
                end;
            end;

            { Calculate the color band's color }
            if FNumberOfColors > 1 then begin
                R := BeginRGBValue[0] + MulDiv (I, RGBDifference[0], FNumberOfColors - 1);
                G := BeginRGBValue[1] + MulDiv (I, RGBDifference[1], FNumberOfColors - 1);
                B := BeginRGBValue[2] + MulDiv (I, RGBDifference[2], FNumberOfColors - 1);
            end else begin { Set to the Begin Color if set to only one color }
                R := BeginRGBValue[0];
                G := BeginRGBValue[1];
                B := BeginRGBValue[2];
            end;

            { Select the brush and paint the color band }
            Canvas.Brush.Color := RGB (R, G, B);
            Canvas.FillRect (ColorBand);
    	end;
	end;

    if (FBorder) then begin
        Canvas.Pen.Color := FBorderColor;
        Canvas.Pen.Style := psSolid;
        Canvas.Pen.Width := FBorderWidth;
        Canvas.Rectangle(hbw, hbw, Width-hbw, Height-hbw);
    end;

    { Copy the working bitmap to the main canvas }
    Canvas.Draw(l, t, WorkBmp);

    // <TextOut>
    Canvas.Brush.Style:= bsClear;
    Canvas.Font.Assign(FFont);
    Canvas.Textout(FTextLeft, FTextTop, FCaption);
    // </TextOut>

    { Release the working bitmap resources }
    WorkBmp.Free;
end;

{ Set the fill direction }
procedure TGradient.SetFillDirection(Value: TFillDirection);
begin
  	if Value <> FDirection then
  	begin
    	FDirection := Value;
    	GradientFill;
    	Invalidate;
  	end;
end;

procedure TGradient.SetBorder(Value: boolean);
begin
  	if Value <> FBorder then
  	begin
    	FBorder := Value;
		Invalidate;
  	end;
end;

procedure TGradient.SetBOrderWidth(Value: Integer);
begin
  	if Value <> FBorderWidth then
  	begin
    	FBorderWidth := Value;
        GradientFill;
		Invalidate;
  	end;
end;

procedure TGradient.SetBorderColor(Value: Tcolor);
begin
  	if Value <> FBorderColor then
  	begin
    	FBorderColor := Value;
		Invalidate;
  	end;
end;

end.
