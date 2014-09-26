unit MxShortcut;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls,
  Forms, Dialogs, ComObj, ShlObj, ShellAPI,
  ComCtrls, mask, stdctrls, consts;

type
    TMxHotKey = class(TCustomEdit)
    private
    	FHotKey:	TShortCut;
    protected
    	procedure 	KeyDown		(var Key: Word; Shift: TShiftState); override;
    	procedure 	KeyUp		(var Key: Word; Shift: TShiftState); override;
	    procedure 	KeyPress	(var Key: Char); override;
	    procedure 	SetHotKey	(Value: TShortCut);
    public
	    constructor Create(AOwner: TComponent); override;
	    destructor 	Destroy; override;
    published
        property 	HotKey: 	TShortCut read FHotKey write SetHotKey;
        property Anchors;
        property AutoSelect;
        property AutoSize;
        property BevelEdges;
        property BevelInner;
        property BevelOuter;
        property BevelKind;
        property BevelWidth;
        property BiDiMode;
        property BorderStyle;
        property CharCase;
        property Color;
        property Constraints;
        property Ctl3D;
        property DragCursor;
        property DragKind;
        property DragMode;
        property Enabled;
        property Font;
        property ImeMode;
        property ImeName;
        property ParentBiDiMode;
        property ParentColor;
        property ParentCtl3D;
        property ParentFont;
        property ParentShowHint;
        property ShowHint;
        property TabOrder;
        property Visible;
        property OnChange;
        property OnClick;
        property OnDblClick;
        property OnDragDrop;
        property OnDragOver;
        property OnEndDock;
        property OnEndDrag;
        property OnEnter;
        property OnExit;
        property OnKeyDown;
        property OnKeyPress;
        property OnKeyUp;
        property OnMouseDown;
        property OnMouseMove;
        property OnMouseUp;
        property OnStartDock;
        property OnStartDrag;
    end;

function MxShortCutToText	(ShortCut: TShortCut): string;
function MxMakeShortCut		(Key: Word; Shift: TShiftState): TShortCut;

implementation

uses
	menus;

function MxMakeShortCut(Key: Word; Shift: TShiftState): TShortCut;
begin
    Result := 0;
    if WordRec(Key).Hi <> 0 then Exit;
    if (not((Key>=$10) and (Key<=$12))) then Result := Key;
    if ssShift in Shift then Inc(Result, scShift);
    if ssCtrl in Shift then Inc(Result, scCtrl);
    if ssAlt in Shift then Inc(Result, scAlt);
end;

type
  TMenuKeyCap = (mkcBkSp, mkcTab, mkcEsc, mkcEnter, mkcSpace, mkcPgUp,
    mkcPgDn, mkcEnd, mkcHome, mkcLeft, mkcUp, mkcRight, mkcDown, mkcIns,
    mkcDel, mkcShift, mkcCtrl, mkcAlt);

var
  MenuKeyCaps: array[TMenuKeyCap] of string = (
    SmkcBkSp, SmkcTab, SmkcEsc, SmkcEnter, SmkcSpace, SmkcPgUp,
    SmkcPgDn, SmkcEnd, SmkcHome, SmkcLeft, SmkcUp, SmkcRight,
    SmkcDown, SmkcIns, SmkcDel, SmkcShift, SmkcCtrl, SmkcAlt);

function MxGetSpecialName(ShortCut: TShortCut): string;
var
  ScanCode: Integer;
  KeyName: array[0..255] of Char;
begin
  Result := '';
  ScanCode := MapVirtualKey(WordRec(ShortCut).Lo, 0) shl 16;
  if ScanCode <> 0 then
  begin
    GetKeyNameText(ScanCode, KeyName, SizeOf(KeyName));
    MxGetSpecialName := KeyName;
  end;
end;

function MxShortCutToText(ShortCut: TShortCut): string;
var
  Name: string;
begin
    case WordRec(ShortCut).Lo of
    $08, $09:	Name:= MenuKeyCaps[TMenuKeyCap(Ord(mkcBkSp) + WordRec(ShortCut).Lo - $08)];
    $0D: 		Name:= MenuKeyCaps[mkcEnter];
    $1B: 		Name:= MenuKeyCaps[mkcEsc];
    $20..$28:   Name:= MenuKeyCaps[TMenuKeyCap(Ord(mkcSpace) + WordRec(ShortCut).Lo - $20)];
    $2D..$2E:	Name:= MenuKeyCaps[TMenuKeyCap(Ord(mkcIns) + WordRec(ShortCut).Lo - $2D)];
    $30..$39: 	Name:= Chr(WordRec(ShortCut).Lo - $30 + Ord('0'));
    $41..$5A: 	Name:= Chr(WordRec(ShortCut).Lo - $41 + Ord('A'));
    $60..$69: 	Name:= Chr(WordRec(ShortCut).Lo - $60 + Ord('0'));
    $70..$87: 	Name:= 'F' + IntToStr(WordRec(ShortCut).Lo - $6F);
    else    	Name:= MxGetSpecialName(ShortCut);
    end;
    if ((0=ShortCut) and (Name='')) then Name := 'None';
    
    Result := '';
    if ShortCut and scShift <> 0	then Result := Result + MenuKeyCaps[mkcShift];
    if ShortCut and scCtrl <> 0 	then Result := Result + MenuKeyCaps[mkcCtrl];
    if ShortCut and scAlt <> 0 		then Result := Result + MenuKeyCaps[mkcAlt];
    Result 			:= Result + Name;
end;

constructor TMxHotKey.Create(AOwner: TComponent);
begin
	inherited Create(AOwner);
    ReadOnly		:= true;
    TabStop			:= false;
    FHotKey		 	:= 0;
    Text			:= MxShortCutToText	(FHotKey);
    PopupMenu		:= TPopupMenu.Create(self);
	ControlStyle 	:= ControlStyle - [csSetCaption,csDoubleClicks,csClickEvents];
    HideSelection	:= false;
end;

destructor TMxHotKey.Destroy();
begin
	inherited;
end;

procedure TMxHotKey.SetHotKey(Value: TShortCut);
begin
	if (FHotKey<>Value) then
    begin
    	FHotKey 	:= Value;
        Text 		:= MxShortCutToText(FHotKey);
    end;
end;

procedure TMxHotKey.KeyDown	(var Key: Word; Shift: TShiftState);
var 
	HK: 	TShortCut;
begin
	inherited;
    HK				:= MxMakeShortCut(Key,Shift);
    SetHotKey		(HK);
end; 

procedure TMxHotKey.KeyUp (var Key: Word; Shift: TShiftState);
begin 
	inherited;
    if (0=WordRec(FHotKey).Lo) then
    	SetHotKey  	(0);
end;

procedure TMxHotKey.KeyPress(var Key: Char);
begin 
	inherited;
end;

end.


