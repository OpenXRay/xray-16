{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{*******************************************************}

unit MXCtrls;

{$I MX.INC}
{$W-,T-}

interface

uses {$IFDEF WIN32} Windows, Registry, {$ELSE} WinTypes, WinProcs, {$ENDIF}
  Messages, Classes, Controls, Graphics, StdCtrls, ExtCtrls, Forms,
  Buttons, Menus, IniFiles, MxConst, mxPlacemnt;//, RxTimer, 

type
  TPositiveInt = 1..MaxInt;

{ TTextListBox }

  TTextListBox = class(TCustomListBox)
  private
    FMaxWidth: Integer;
{$IFNDEF WIN32}
    FTabWidth: Integer;
    procedure SetTabWidth(Value: Integer);
{$ENDIF}
    procedure ResetHorizontalExtent;
    procedure SetHorizontalExtent;
    function GetItemWidth(Index: Integer): Integer;
  protected
{$IFNDEF WIN32}
    procedure CreateParams(var Params: TCreateParams); override;
    procedure CreateWnd; override;
{$ENDIF}
    procedure WndProc(var Message: TMessage); override;
  published
    property Align;
    property BorderStyle;
    property Color;
    property Ctl3D;
    property DragCursor;
    property DragMode;
    property Enabled;
    property ExtendedSelect;
    property Font;
    property IntegralHeight;
{$IFDEF RX_D4}
    property Anchors;
    property BiDiMode;
    property Constraints;
    property DragKind;
    property ParentBiDiMode;
{$ENDIF}
{$IFDEF WIN32}
  {$IFNDEF VER90}
    property ImeMode;
    property ImeName;
  {$ENDIF}
{$ENDIF}
    property ItemHeight;
    property Items;
    property MultiSelect;
    property ParentColor;
    property ParentCtl3D;
    property ParentFont;
    property ParentShowHint;
    property PopupMenu;
    property ShowHint;
    property Sorted;
    property TabOrder;
    property TabStop;
{$IFDEF WIN32}
    property TabWidth;
{$ELSE}
    property TabWidth: Integer read FTabWidth write SetTabWidth default 0;
{$ENDIF}
    property Visible;
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
{$IFDEF WIN32}
    property OnStartDrag;
{$ENDIF}
{$IFDEF RX_D5}
    property OnContextPopup;
{$ENDIF}
{$IFDEF RX_D4}
    property OnMouseWheelDown;
    property OnMouseWheelUp;
    property OnEndDock;
    property OnStartDock;
{$ENDIF}
  end;

{ TMxCustomListBox }

  TGetItemWidthEvent = procedure(Control: TWinControl; Index: Integer;
    var Width: Integer) of object;

  TMxCustomListBox = class(TWinControl)
  private
    FItems: TStrings;
    FBorderStyle: TBorderStyle;
    FCanvas: TCanvas;
    FColumns: Integer;
    FItemHeight: Integer;
    FStyle: TListBoxStyle;
    FIntegralHeight: Boolean;
    FMultiSelect: Boolean;
    FSorted: Boolean;
    FExtendedSelect: Boolean;
    FTabWidth: Integer;
    FSaveItems: TStringList;
    FSaveTopIndex: Integer;
    FSaveItemIndex: Integer;
    FAutoScroll: Boolean;
    FGraySelection: Boolean;
    FMaxItemWidth: Integer;
    FOnDrawItem: TDrawItemEvent;
    FOnMeasureItem: TMeasureItemEvent;
    FOnGetItemWidth: TGetItemWidthEvent;
    procedure ResetHorizontalExtent;
    procedure SetHorizontalExtent;
    function GetAutoScroll: Boolean;
    function GetItemHeight: Integer; virtual;
    function GetItemIndex: Integer;
    function GetSelCount: Integer;
    function GetSelected(Index: Integer): Boolean;
    function GetTopIndex: Integer;
    procedure SetAutoScroll(Value: Boolean);
    procedure SetBorderStyle(Value: TBorderStyle);
    procedure SetColumnWidth;
    procedure SetColumns(Value: Integer);
    procedure SetExtendedSelect(Value: Boolean);
    procedure SetIntegralHeight(Value: Boolean);
    procedure SetItemHeight(Value: Integer);
    procedure SetItemIndex(Value: Integer);
    procedure SetMultiSelect(Value: Boolean);
    procedure SetSelected(Index: Integer; Value: Boolean);
    procedure SetSorted(Value: Boolean);
    procedure SetStyle(Value: TListBoxStyle);
    procedure SetTabWidth(Value: Integer);
    procedure SetTopIndex(Value: Integer);
    procedure SetGraySelection(Value: Boolean);
    procedure SetOnDrawItem(Value: TDrawItemEvent);
    procedure SetOnGetItemWidth(Value: TGetItemWidthEvent);
    procedure WMPaint(var Message: TWMPaint); message WM_PAINT;
    procedure WMSize(var Message: TWMSize); message WM_SIZE;
    procedure CNCommand(var Message: TWMCommand); message CN_COMMAND;
    procedure CNDrawItem(var Message: TWMDrawItem); message CN_DRAWITEM;
    procedure CNMeasureItem(var Message: TWMMeasureItem); message CN_MEASUREITEM;
    procedure WMLButtonDown(var Message: TWMLButtonDown); message WM_LBUTTONDOWN;
    procedure WMNCHitTest(var Msg: TWMNCHitTest); message WM_NCHITTEST;
    procedure WMKillFocus(var Msg: TWMKillFocus); message WM_KILLFOCUS;
    procedure WMSetFocus(var Msg: TWMSetFocus); message WM_SETFOCUS;
{$IFDEF WIN32}
    procedure CMCtl3DChanged(var Message: TMessage); message CM_CTL3DCHANGED;
{$ENDIF}
  protected
    procedure CreateParams(var Params: TCreateParams); override;
    procedure CreateWnd; override;
    procedure DestroyWnd; override;
    function CreateItemList: TStrings; virtual;
    function GetItemWidth(Index: Integer): Integer; virtual;
    procedure WndProc(var Message: TMessage); override;
    procedure DragCanceled; override;
    procedure DrawItem(Index: Integer; Rect: TRect;
      State: TOwnerDrawState); virtual;
    procedure MeasureItem(Index: Integer; var Height: Integer); virtual;
    function GetItemData(Index: Integer): Longint; dynamic;
    procedure SetItemData(Index: Integer; AData: LongInt); dynamic;
    procedure SetItems(Value: TStrings); virtual;
    procedure ResetContent; dynamic;
    procedure DeleteString(Index: Integer); dynamic;
    property AutoScroll: Boolean read GetAutoScroll write SetAutoScroll default False;
    property BorderStyle: TBorderStyle read FBorderStyle write SetBorderStyle default bsSingle;
    property Columns: Integer read FColumns write SetColumns default 0;
    property ExtendedSelect: Boolean read FExtendedSelect write SetExtendedSelect default True;
    property GraySelection: Boolean read FGraySelection write SetGraySelection default False;
    property IntegralHeight: Boolean read FIntegralHeight write SetIntegralHeight default False;
    property ItemHeight: Integer read GetItemHeight write SetItemHeight;
    property MultiSelect: Boolean read FMultiSelect write SetMultiSelect default False;
    property ParentColor default False;
    property Sorted: Boolean read FSorted write SetSorted default False;
    property Style: TListBoxStyle read FStyle write SetStyle default lbStandard;
    property TabWidth: Integer read FTabWidth write SetTabWidth default 0;
    property OnDrawItem: TDrawItemEvent read FOnDrawItem write SetOnDrawItem;
    property OnMeasureItem: TMeasureItemEvent read FOnMeasureItem write FOnMeasureItem;
    property OnGetItemWidth: TGetItemWidthEvent read FOnGetItemWidth write SetOnGetItemWidth;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure Clear;
    procedure DefaultDrawText(X, Y: Integer; const S: string);
    function ItemAtPos(Pos: TPoint; Existing: Boolean): Integer;
    function ItemRect(Index: Integer): TRect;
    property Canvas: TCanvas read FCanvas;
    property Items: TStrings read FItems write SetItems;
    property ItemIndex: Integer read GetItemIndex write SetItemIndex;
    property SelCount: Integer read GetSelCount;
    property Selected[Index: Integer]: Boolean read GetSelected write SetSelected;
    property TopIndex: Integer read GetTopIndex write SetTopIndex;
  published
    property TabStop default True;
  end;

{ TMxCheckListBox }

  TCheckKind = (ckCheckBoxes, ckRadioButtons, ckCheckMarks);
  TChangeStateEvent = procedure (Sender: TObject; Index: Integer) of object;

  TMxCheckListBox = class(TMxCustomListBox)
  private
    FAllowGrayed: Boolean;
    FCheckKind: TCheckKind;
    FSaveStates: TList;
    FDrawBitmap: TBitmap;
    FCheckWidth, FCheckHeight: Integer;
    FReserved: Integer;
    FInUpdateStates: Boolean;
    FIniLink: TIniLink;
    FOnClickCheck: TNotifyEvent;
    FOnStateChange: TChangeStateEvent;
    procedure ResetItemHeight;
    function GetItemHeight: Integer; override;
    procedure DrawCheck(R: TRect; AState: TCheckBoxState; Enabled: Boolean);
    procedure SetCheckKind(Value: TCheckKind);
    procedure SetChecked(Index: Integer; AChecked: Boolean);
    function GetChecked(Index: Integer): Boolean;
    procedure SetState(Index: Integer; AState: TCheckBoxState);
    function GetState(Index: Integer): TCheckBoxState;
    procedure SetItemEnabled(Index: Integer; Value: Boolean);
    function GetItemEnabled(Index: Integer): Boolean;
    function GetAllowGrayed: Boolean;
    procedure ToggleClickCheck(Index: Integer);
    procedure InvalidateCheck(Index: Integer);
    procedure InvalidateItem(Index: Integer);
    function CreateCheckObject(Index: Integer): TObject;
    function FindCheckObject(Index: Integer): TObject;
    function GetCheckObject(Index: Integer): TObject;
    function IsCheckObject(Index: Integer): Boolean;
    procedure ReadVersion(Reader: TReader);
    procedure WriteVersion(Writer: TWriter);
    procedure ReadCheckData(Reader: TReader);
    procedure WriteCheckData(Writer: TWriter);
    procedure InternalSaveStates(IniFile: TObject; const Section: string);
    procedure InternalRestoreStates(IniFile: TObject; const Section: string);
    function GetStorage: TFormPlacement;
    procedure SetStorage(Value: TFormPlacement);
    procedure IniSave(Sender: TObject);
    procedure IniLoad(Sender: TObject);
    procedure UpdateCheckStates;
    function GetCheckedIndex: Integer;
    procedure SetCheckedIndex(Value: Integer);
    procedure CNDrawItem(var Message: TWMDrawItem); message CN_DRAWITEM;
    procedure CMFontChanged(var Message: TMessage); message CM_FONTCHANGED;
  protected
    function CreateItemList: TStrings; override;
    procedure DrawItem(Index: Integer; Rect: TRect;
      State: TOwnerDrawState); override;
    procedure DefineProperties(Filer: TFiler); override;
    function GetItemWidth(Index: Integer): Integer; override;
    function GetItemData(Index: Integer): LongInt; override;
    procedure SetItemData(Index: Integer; AData: LongInt); override;
    procedure KeyPress(var Key: Char); override;
    procedure Loaded; override;
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure ResetContent; override;
    procedure DeleteString(Index: Integer); override;
    procedure ClickCheck; dynamic;
    procedure ChangeItemState(Index: Integer); dynamic;
    procedure CreateParams(var Params: TCreateParams); override;
    procedure CreateWnd; override;
    procedure DestroyWnd; override;
    procedure WMDestroy(var Msg: TWMDestroy); message WM_DESTROY;
    function GetCheckWidth: Integer;
    procedure SetItems(Value: TStrings); override;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
{$IFDEF WIN32}
    procedure SaveStatesReg(IniFile: TRegIniFile);
    procedure RestoreStatesReg(IniFile: TRegIniFile);
{$ENDIF WIN32}
    procedure SaveStates(IniFile: TIniFile);
    procedure RestoreStates(IniFile: TIniFile);
    procedure ApplyState(AState: TCheckBoxState; EnabledOnly: Boolean);
    property Checked[Index: Integer]: Boolean read GetChecked write SetChecked;
    property State[Index: Integer]: TCheckBoxState read GetState write SetState;
    property EnabledItem[Index: Integer]: Boolean read GetItemEnabled write SetItemEnabled;
  published
    property AllowGrayed: Boolean read GetAllowGrayed write FAllowGrayed default False;
    property CheckKind: TCheckKind read FCheckKind write SetCheckKind default ckCheckBoxes;
    property CheckedIndex: Integer read GetCheckedIndex write SetCheckedIndex default -1;
    property IniStorage: TFormPlacement read GetStorage write SetStorage;
    property Align;
    property AutoScroll default True;
    property BorderStyle;
    property Color;
    property Columns;
    property Ctl3D;
    property DragCursor;
    property DragMode;
    property Enabled;
    property ExtendedSelect;
    property Font;
    property GraySelection;
{$IFDEF RX_D4}
    property Anchors;
    property BiDiMode;
    property Constraints;
    property DragKind;
    property ParentBiDiMode;
{$ENDIF}
{$IFDEF WIN32}
  {$IFNDEF VER90}
    property ImeMode;
    property ImeName;
  {$ENDIF}
{$ENDIF}
    property IntegralHeight;
    property ItemHeight;
    property Items stored False;
    property MultiSelect;
    property ParentColor;
    property ParentCtl3D;
    property ParentFont;
    property ParentShowHint;
    property PopupMenu;
    property ShowHint;
    property Sorted;
    property Style;
    property TabOrder;
    property TabWidth;
    property Visible;
    property OnStateChange: TChangeStateEvent read FOnStateChange write FOnStateChange;
    property OnClickCheck: TNotifyEvent read FOnClickCheck write FOnClickCheck;
    property OnClick;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnDrawItem;
    property OnEndDrag;
    property OnEnter;
    property OnExit;
    property OnGetItemWidth;
    property OnKeyDown;
    property OnKeyPress;
    property OnKeyUp;
    property OnMeasureItem;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
{$IFDEF WIN32}
    property OnStartDrag;
{$ENDIF}
{$IFDEF RX_D5}
    property OnContextPopup;
{$ENDIF}
{$IFDEF RX_D4}
    property OnMouseWheelDown;
    property OnMouseWheelUp;
    property OnEndDock;
    property OnStartDock;
{$ENDIF}
  end;

const
  clbDefaultState = cbUnchecked;
  clbDefaultEnabled = True;

{ TMxCustomLabel }

type
  TShadowPosition = (spLeftTop, spLeftBottom, spRightBottom, spRightTop);
{$IFNDEF RX_D3}
  TTextLayout = (tlTop, tlCenter, tlBottom);
{$ENDIF}

  TMxCustomLabel = class(TGraphicControl)
  private
    FFocusControl: TWinControl;
    FAlignment: TAlignment;
    FAutoSize: Boolean;
    FLayout: TTextLayout;
    FShadowColor: TColor;
    FShadowSize: Byte;
    FShadowPos: TShadowPosition;
    FWordWrap: Boolean;
    FShowAccelChar: Boolean;
    FShowFocus: Boolean;
    FFocused: Boolean;
    FMouseInControl: Boolean;
    FDragging: Boolean;
    FLeftMargin: Integer;
    FRightMargin: Integer;
    FOnMouseEnter: TNotifyEvent;
    FOnMouseLeave: TNotifyEvent;
    procedure DoDrawText(var Rect: TRect; Flags: Word);
    function GetTransparent: Boolean;
    procedure UpdateTracking;
    procedure SetAlignment(Value: TAlignment);
    procedure SetAutoSize(Value: Boolean);
    procedure SetFocusControl(Value: TWinControl);
    procedure SetLayout(Value: TTextLayout);
    procedure SetLeftMargin(Value: Integer);
    procedure SetRightMargin(Value: Integer);
    procedure SetShadowColor(Value: TColor);
    procedure SetShadowSize(Value: Byte);
    procedure SetShadowPos(Value: TShadowPosition);
    procedure SetShowAccelChar(Value: Boolean);
    procedure SetTransparent(Value: Boolean);
    procedure SetWordWrap(Value: Boolean);
    procedure SetShowFocus(Value: Boolean);
    procedure CMTextChanged(var Message: TMessage); message CM_TEXTCHANGED;
    procedure CMFocusChanged(var Message: TCMFocusChanged); message CM_FOCUSCHANGED;
    procedure CMFontChanged(var Message: TMessage); message CM_FONTCHANGED;
    procedure CMDialogChar(var Message: TCMDialogChar); message CM_DIALOGCHAR;
    procedure CMEnabledChanged(var Message: TMessage); message CM_ENABLEDCHANGED;
    procedure CMMouseEnter(var Message: TMessage); message CM_MOUSEENTER;
    procedure CMMouseLeave(var Message: TMessage); message CM_MOUSELEAVE;
    procedure CMVisibleChanged(var Message: TMessage); message CM_VISIBLECHANGED;
    procedure WMRButtonDown(var Message: TWMRButtonDown); message WM_RBUTTONDOWN;
    procedure WMRButtonUp(var Message: TWMRButtonUp); message WM_RBUTTONUP;
  protected
    procedure AdjustBounds;
    function GetDefaultFontColor: TColor; virtual;
    function GetLabelCaption: string; virtual;
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure MouseUp(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
    procedure Paint; override;
    procedure MouseEnter; dynamic;
    procedure MouseLeave; dynamic;
    property Alignment: TAlignment read FAlignment write SetAlignment default taLeftJustify;
    property AutoSize: Boolean read FAutoSize write SetAutoSize default True;
    property FocusControl: TWinControl read FFocusControl write SetFocusControl;
    property Layout: TTextLayout read FLayout write SetLayout default tlTop;
    property LeftMargin: Integer read FLeftMargin write SetLeftMargin default 0;
    property RightMargin: Integer read FRightMargin write SetRightMargin default 0;
    property ShadowColor: TColor read FShadowColor write SetShadowColor default clBtnHighlight;
    property ShadowSize: Byte read FShadowSize write SetShadowSize default 1;
    property ShadowPos: TShadowPosition read FShadowPos write SetShadowPos default spLeftTop;
    property ShowAccelChar: Boolean read FShowAccelChar write SetShowAccelChar default True;
    property ShowFocus: Boolean read FShowFocus write SetShowFocus default False;
    property Transparent: Boolean read GetTransparent write SetTransparent default False;
    property WordWrap: Boolean read FWordWrap write SetWordWrap default False;
    property OnMouseEnter: TNotifyEvent read FOnMouseEnter write FOnMouseEnter;
    property OnMouseLeave: TNotifyEvent read FOnMouseLeave write FOnMouseLeave;
  public
    constructor Create(AOwner: TComponent); override;
    property Canvas;
    property MouseInControl: Boolean read FMouseInControl;
  end;

{ TMxLabel }

  TMxLabel = class(TMxCustomLabel)
  published
    property Align;
    property Alignment;
    property AutoSize;
    property Caption;
    property Color;
    property DragCursor;
    property DragMode;
    property Enabled;
    property FocusControl;
    property Font;
{$IFDEF RX_D4}
    property Anchors;
    property BiDiMode;
    property Constraints;
    property DragKind;
    property ParentBiDiMode;
{$ENDIF}
    property Layout;
    property ParentColor;
    property ParentFont;
    property ParentShowHint;
    property PopupMenu;
    property ShadowColor;
    property ShadowSize;
    property ShadowPos;
    property ShowAccelChar;
    property ShowFocus;
    property ShowHint;
    property Transparent;
    property Visible;
    property WordWrap;
    property OnClick;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
    property OnMouseEnter;
    property OnMouseLeave;
{$IFDEF WIN32}
    property OnStartDrag;
{$ENDIF}
{$IFDEF RX_D5}
    property OnContextPopup;
{$ENDIF}
{$IFDEF RX_D4}
    property OnEndDock;
    property OnStartDock;
{$ENDIF}
  end;

  TMxPanel = class(TCustomPanel)
  public
    property Canvas;
    property DockManager;
  protected
	FOnPaint: TNotifyEvent;
    procedure Paint; override;
  published
    property Align;
    property Alignment;
    property Anchors;
    property AutoSize;
    property BevelInner;
    property BevelOuter;
    property BevelWidth;
    property BiDiMode;
    property BorderWidth;
    property BorderStyle;
    property Caption;
    property Color;
    property Constraints;
    property Ctl3D;
    property UseDockManager default True;
    property DockSite;
    property DragCursor;
    property DragKind;
    property DragMode;
    property Enabled;
    property FullRepaint;
    property Font;
    property Locked;
    property ParentBiDiMode;
    property ParentColor;
    property ParentCtl3D;
    property ParentFont;
    property ParentShowHint;
    property PopupMenu;
    property ShowHint;
    property TabOrder;
    property TabStop;
    property Visible;
    property OnCanResize;
    property OnClick;
    property OnConstrainedResize;
    property OnContextPopup;
    property OnDockDrop;
    property OnDockOver;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDock;
    property OnEndDrag;
    property OnEnter;
    property OnExit;
    property OnGetSiteInfo;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
    property OnResize;
    property OnStartDock;
    property OnStartDrag;
    property OnUnDock;
	property OnPaint: TNotifyEvent read FOnPaint write FOnPaint;
  end;

  { TSecretPanel }

  TGlyphLayout = (glGlyphLeft, glGlyphRight, glGlyphTop, glGlyphBottom);
  TScrollDirection = (sdVertical, sdHorizontal);
  TPanelDrawEvent = procedure(Sender: TObject; Canvas: TCanvas;
    Rect: TRect) of object;

{ TMxSpeedButton }

  TMxNumGlyphs = 1..5;
  TMxDropDownMenuPos = (dmpBottom, dmpRight);
  TMxButtonState = (rbsUp, rbsDisabled, rbsDown, rbsExclusive, rbsInactive);

  TMxSpeedButton = class(TGraphicControl)
  private
    FGroupIndex: Integer;
    FStyle: TButtonStyle;
    FGlyph: Pointer;
    FDrawImage: TBitmap;
    FDown: Boolean;
    FDragging: Boolean;
    FFlat: Boolean;
    FMouseInControl: Boolean;
    FAllowAllUp: Boolean;
    FLayout: TButtonLayout;
    FSpacing: Integer;
    FMargin: Integer;
    FModalResult: TModalResult;
    FTransparent: Boolean;
    FMarkDropDown: Boolean;
    FDropDownMenu: TPopupMenu;
    FMenuPosition: TMxDropDownMenuPos;
    FInactiveGrayed: Boolean;
    FMenuTracking: Boolean;
    FRepeatTimer: TTimer;
    FAllowTimer: Boolean;
    FInitRepeatPause: Word;
    FRepeatPause: Word;
    FOnMouseEnter: TNotifyEvent;
    FOnMouseLeave: TNotifyEvent;
    procedure GlyphChanged(Sender: TObject);
    procedure UpdateExclusive;
    function GetGlyph: TBitmap;
    procedure SetGlyph(Value: TBitmap);
    function GetNumGlyphs: TMxNumGlyphs;
    procedure SetNumGlyphs(Value: TMxNumGlyphs);
    function GetWordWrap: Boolean;
    procedure SetWordWrap(Value: Boolean);
    function GetAlignment: TAlignment;
    procedure SetAlignment(Value: TAlignment);
    procedure SetDown(Value: Boolean);
    procedure SetAllowAllUp(Value: Boolean);
    procedure SetGroupIndex(Value: Integer);
    procedure SetLayout(Value: TButtonLayout);
    procedure SetSpacing(Value: Integer);
    procedure SetMargin(Value: Integer);
    procedure SetDropDownMenu(Value: TPopupMenu);
    procedure SetFlat(Value: Boolean);
    procedure SetStyle(Value: TButtonStyle);
    procedure SetInactiveGrayed(Value: Boolean);
    procedure SetTransparent(Value: Boolean);
    procedure SetMarkDropDown(Value: Boolean);
    procedure TimerExpired(Sender: TObject);
    procedure SetAllowTimer(Value: Boolean);
    function CheckMenuDropDown(const Pos: TSmallPoint;
      Manual: Boolean): Boolean;
    procedure DoMouseUp(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer);
    procedure CMButtonPressed(var Message: TMessage); message CM_RXBUTTONPRESSED;
    procedure CMDialogChar(var Message: TCMDialogChar); message CM_DIALOGCHAR;
    procedure CMEnabledChanged(var Message: TMessage); message CM_ENABLEDCHANGED;
    procedure CMFontChanged(var Message: TMessage); message CM_FONTCHANGED;
    procedure CMMouseEnter(var Message: TMessage); message CM_MOUSEENTER;
    procedure CMMouseLeave(var Message: TMessage); message CM_MOUSELEAVE;
    procedure CMSysColorChange(var Message: TMessage); message CM_SYSCOLORCHANGE;
    procedure CMTextChanged(var Message: TMessage); message CM_TEXTCHANGED;
    procedure CMVisibleChanged(var Message: TMessage); message CM_VISIBLECHANGED;
    procedure WMLButtonDblClk(var Message: TWMLButtonDown); message WM_LBUTTONDBLCLK;
    procedure WMMouseMove(var Message: TMessage); message WM_MOUSEMOVE;
    procedure WMRButtonDown(var Message: TWMRButtonDown); message WM_RBUTTONDOWN;
    procedure WMRButtonUp(var Message: TWMRButtonUp); message WM_RBUTTONUP;
  protected
    FState: TMxButtonState;
{$IFDEF RX_D4}
    procedure ActionChange(Sender: TObject; CheckDefaults: Boolean); override;
{$ENDIF}
    function GetDropDownMenuPos: TPoint;
    function GetPalette: HPALETTE; override;
    procedure Paint; override;
    procedure Loaded; override;
    procedure PaintGlyph(Canvas: TCanvas; ARect: TRect; AState: TMxButtonState;
      DrawMark: Boolean); virtual;
    procedure MouseEnter; dynamic;
    procedure MouseLeave; dynamic;
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure MouseMove(Shift: TShiftState; X, Y: Integer); override;
    procedure MouseUp(Button: TMouseButton; Shift: TShiftState;
      X, Y: Integer); override;
    procedure Notification(AComponent: TComponent;
      Operation: TOperation); override;
    property ButtonGlyph: Pointer read FGlyph;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure ButtonClick;
    function CheckBtnMenuDropDown: Boolean;
    procedure Click; override;
    procedure UpdateTracking;
  published
{$IFDEF RX_D4}
    property Action;
    property Anchors;
    property BiDiMode;
    property Constraints;
    property DragKind;
    property ParentBiDiMode;
{$ENDIF}
    property Alignment: TAlignment read GetAlignment write SetAlignment default taCenter;
    property AllowAllUp: Boolean read FAllowAllUp write SetAllowAllUp default False;
    property AllowTimer: Boolean read FAllowTimer write SetAllowTimer default False;
    property GroupIndex: Integer read FGroupIndex write SetGroupIndex default 0;
    { Ensure group index is declared before Down }
    property Down: Boolean read FDown write SetDown default False;
    property DropDownMenu: TPopupMenu read FDropDownMenu write SetDropDownMenu;
    property MenuPosition: TMxDropDownMenuPos read FMenuPosition write FMenuPosition
      default dmpBottom;
    property Caption;
    property DragCursor;
    property DragMode;
    property Enabled;
    property Flat: Boolean read FFlat write SetFlat default False;
    property Font;
    property Glyph: TBitmap read GetGlyph write SetGlyph;
    property GrayedInactive: Boolean read FInactiveGrayed write SetInactiveGrayed
      default True;
    property InitPause: Word read FInitRepeatPause write FInitRepeatPause default 500;
    property Layout: TButtonLayout read FLayout write SetLayout default blGlyphTop;
    property Margin: Integer read FMargin write SetMargin default -1;
    property MarkDropDown: Boolean read FMarkDropDown write SetMarkDropDown default True;
    property ModalResult: TModalResult read FModalResult write FModalResult default 0;
    property NumGlyphs: TMxNumGlyphs read GetNumGlyphs write SetNumGlyphs default 1;
    property ParentFont;
    property ParentShowHint default False;
    property RepeatInterval: Word read FRepeatPause write FRepeatPause default 100;
    property ShowHint default True;
    property Spacing: Integer read FSpacing write SetSpacing default 1;
    property Style: TButtonStyle read FStyle write SetStyle default bsAutoDetect;
    property Transparent: Boolean read FTransparent write SetTransparent default False;
    property WordWrap: Boolean read GetWordWrap write SetWordWrap default False;
    property Visible;
    property OnMouseEnter: TNotifyEvent read FOnMouseEnter write FOnMouseEnter;
    property OnMouseLeave: TNotifyEvent read FOnMouseLeave write FOnMouseLeave;
    property OnClick;
    property OnDblClick;
    property OnDragDrop;
    property OnDragOver;
    property OnEndDrag;
    property OnMouseDown;
    property OnMouseMove;
    property OnMouseUp;
{$IFDEF WIN32}
    property OnStartDrag;
{$ENDIF}
{$IFDEF RX_D4}
    property OnEndDock;
    property OnStartDock;
{$ENDIF}
  end;

{ TButtonImage }

  TButtonImage = class(TObject)
  private
    FGlyph: TObject;
    FButtonSize: TPoint;
    FCaption: TCaption;
    function GetNumGlyphs: TMxNumGlyphs;
    procedure SetNumGlyphs(Value: TMxNumGlyphs);
    function GetWordWrap: Boolean;
    procedure SetWordWrap(Value: Boolean);
    function GetAlignment: TAlignment;
    procedure SetAlignment(Value: TAlignment);
    function GetGlyph: TBitmap;
    procedure SetGlyph(Value: TBitmap);
  public
    constructor Create;
    destructor Destroy; override;
    procedure Invalidate;
{$IFDEF WIN32}
    procedure DrawEx(Canvas: TCanvas; X, Y, Margin, Spacing: Integer;
      Layout: TButtonLayout; AFont: TFont; Images: TImageList;
      ImageIndex: Integer; Flags: Word);
{$ENDIF}
    procedure Draw(Canvas: TCanvas; X, Y, Margin, Spacing: Integer;
      Layout: TButtonLayout; AFont: TFont; Flags: Word);
    property Alignment: TAlignment read GetAlignment write SetAlignment;
    property Caption: TCaption read FCaption write FCaption;
    property Glyph: TBitmap read GetGlyph write SetGlyph;
    property NumGlyphs: TMxNumGlyphs read GetNumGlyphs write SetNumGlyphs;
    property ButtonSize: TPoint read FButtonSize write FButtonSize;
    property WordWrap: Boolean read GetWordWrap write SetWordWrap;
  end;

{ TMxButtonGlyph }

  TMxButtonGlyph = class
  private
    FOriginal: TBitmap;
    FGlyphList: TImageList;
    FIndexs: array[TMxButtonState] of Integer;
    FTransparentColor: TColor;
    FNumGlyphs: TMxNumGlyphs;
    FWordWrap: Boolean;
    FAlignment: TAlignment;
    FOnChange: TNotifyEvent;
    procedure GlyphChanged(Sender: TObject);
    procedure SetGlyph(Value: TBitmap);
    procedure SetNumGlyphs(Value: TMxNumGlyphs);
    function MapColor(Color: TColor): TColor;
  protected
    procedure MinimizeCaption(Canvas: TCanvas; const Caption: string;
      Buffer: PChar; MaxLen, Width: Integer);
    function CreateButtonGlyph(State: TMxButtonState): Integer;
{$IFDEF WIN32}
    function CreateImageGlyph(State: TMxButtonState; Images: TImageList;
      Index: Integer): Integer;
{$ENDIF}
    procedure CalcButtonLayout(Canvas: TCanvas; const Client: TRect;
      var Caption: string; Layout: TButtonLayout; Margin, Spacing: Integer;
      PopupMark: Boolean; var GlyphPos: TPoint; var TextBounds: TRect;
      Flags: Word {$IFDEF WIN32}; Images: TImageList; ImageIndex: Integer
      {$ENDIF});
  public
    constructor Create;
    destructor Destroy; override;
    procedure Invalidate;
    function DrawButtonGlyph(Canvas: TCanvas; X, Y: Integer;
      State: TMxButtonState): TPoint;
{$IFDEF WIN32}
    function DrawButtonImage(Canvas: TCanvas; X, Y: Integer; Images: TImageList;
      ImageIndex: Integer; State: TMxButtonState): TPoint;
    function DrawEx(Canvas: TCanvas; const Client: TRect; const Caption: string;
      Layout: TButtonLayout; Margin, Spacing: Integer; PopupMark: Boolean;
      Images: TImageList; ImageIndex: Integer; State: TMxButtonState;
      Flags: Word): TRect;
{$ENDIF}
    procedure DrawButtonText(Canvas: TCanvas; const Caption: string;
      TextBounds: TRect; State: TMxButtonState; Flags: Word);
    procedure DrawPopupMark(Canvas: TCanvas; X, Y: Integer;
      State: TMxButtonState);
    function Draw(Canvas: TCanvas; const Client: TRect; const Caption: string;
      Layout: TButtonLayout; Margin, Spacing: Integer; PopupMark: Boolean;
      State: TMxButtonState; Flags: Word): TRect;
    property Alignment: TAlignment read FAlignment write FAlignment;
    property Glyph: TBitmap read FOriginal write SetGlyph;
    property NumGlyphs: TMxNumGlyphs read FNumGlyphs write SetNumGlyphs;
    property WordWrap: Boolean read FWordWrap write FWordWrap;
    property OnChange: TNotifyEvent read FOnChange write FOnChange;
  end;

function DrawShadowText(DC: HDC; Str: PChar; Count: Integer; var Rect: TRect;
  Format: Word; ShadowSize: Byte; ShadowColor: TColorRef;
  ShadowPos: TShadowPosition): Integer;

function CheckBitmap: TBitmap;

implementation

{$IFDEF WIN32}
 {$R *.R32}
{$ELSE}
 {$R *.R16}
{$ENDIF}

uses SysUtils, Dialogs, {$IFDEF WIN32} CommCtrl, {$ELSE} Str16, {$ENDIF}
  mxVCLUtils, mxMaxMin, Consts, RTLConsts, mxAppUtils {$IFDEF RX_D4}, ImgList,
  ActnList {$ENDIF};

const
  Alignments: array [TAlignment] of Word = (DT_LEFT, DT_RIGHT, DT_CENTER);
  WordWraps: array[Boolean] of Word = (0, DT_WORDBREAK);

{ TTextListBox }

procedure TTextListBox.SetHorizontalExtent;
begin
  SendMessage(Handle, LB_SETHORIZONTALEXTENT, FMaxWidth, 0);
end;

function TTextListBox.GetItemWidth(Index: Integer): Integer;
var
  ATabWidth: Longint;
  S: string;
begin
  S := Items[Index] + 'x';
  if TabWidth > 0 then begin
    ATabWidth := Round((TabWidth * Canvas.TextWidth('0')) * 0.25);
    Result := LoWord(GetTabbedTextExtent(Canvas.Handle, @S[1], Length(S),
      1, ATabWidth));
  end
  else Result := Canvas.TextWidth(S);
end;

procedure TTextListBox.ResetHorizontalExtent;
var
  I: Integer;
begin
  FMaxWidth := 0;
  for I := 0 to Items.Count - 1 do
    FMaxWidth := Max(FMaxWidth, GetItemWidth(I));
  SetHorizontalExtent;
end;

{$IFNDEF WIN32}

procedure TTextListBox.SetTabWidth(Value: Integer);
begin
  if Value < 0 then Value := 0;
  if FTabWidth <> Value then begin
    FTabWidth := Value;
    RecreateWnd;
  end;
end;

procedure TTextListBox.CreateParams(var Params: TCreateParams);
const
  TabStops: array[Boolean] of Longword = (0, LBS_USETABSTOPS);
begin
  inherited CreateParams(Params);
  Params.Style := Params.Style or TabStops[FTabWidth <> 0];
end;

procedure TTextListBox.CreateWnd;
begin
  inherited CreateWnd;
  if FTabWidth <> 0 then
    SendMessage(Handle, LB_SETTABSTOPS, 1, Longint(@FTabWidth));
end;

{$ENDIF}

procedure TTextListBox.WndProc(var Message: TMessage);
begin
  case Message.Msg of
    LB_ADDSTRING, LB_INSERTSTRING:
      begin
        inherited WndProc(Message);
        FMaxWidth := Max(FMaxWidth, GetItemWidth(Message.Result));
        SetHorizontalExtent;
      end;
    LB_DELETESTRING:
      begin
        if GetItemWidth(Message.wParam) >= FMaxWidth then begin
          Perform(WM_HSCROLL, SB_TOP, 0);
          inherited WndProc(Message);
          ResetHorizontalExtent;
        end
        else inherited WndProc(Message);
      end;
    LB_RESETCONTENT:
      begin
        FMaxWidth := 0;
        SetHorizontalExtent;
        Perform(WM_HSCROLL, SB_TOP, 0);
        inherited WndProc(Message);
      end;
    WM_SETFONT:
      begin
        inherited WndProc(Message);
        Canvas.Font.Assign(Self.Font);
        ResetHorizontalExtent;
        Exit;
      end;
    else inherited WndProc(Message);
  end;
end;

{ TMxCustomListBox implementation copied from STDCTRLS.PAS and modified }

{ TMxListBoxStrings }

type
  TMxListBoxStrings = class(TStrings)
  private
    ListBox: TMxCustomListBox;
  protected
{$IFNDEF RX_D3}
    procedure Error(Msg: Word; Data: Integer);
{$ENDIF}
    function Get(Index: Integer): string; override;
    function GetCount: Integer; override;
    function GetObject(Index: Integer): TObject; override;
    procedure PutObject(Index: Integer; AObject: TObject); override;
    procedure SetUpdateState(Updating: Boolean); override;
  public
    function Add(const S: string): Integer; override;
    procedure Clear; override;
    procedure Delete(Index: Integer); override;
    procedure Insert(Index: Integer; const S: string); override;
  end;

{$IFNDEF RX_D3}
procedure TMxListBoxStrings.Error(Msg: Word; Data: Integer);

{$IFDEF WIN32}
  function ReturnAddr: Pointer;
  asm
          MOV     EAX,[EBP+4]
  end;
{$ELSE}
  function ReturnAddr: Pointer; assembler;
  asm
          MOV     AX,[BP].Word[2]
          MOV     DX,[BP].Word[4]
  end;
{$ENDIF}

begin
  raise EStringListError.CreateFmt('%s: %d', [LoadStr(Msg),
    Data]) at ReturnAddr;
end;
{$ENDIF}

function TMxListBoxStrings.GetCount: Integer;
begin
  Result := SendMessage(ListBox.Handle, LB_GETCOUNT, 0, 0);
end;

function TMxListBoxStrings.Get(Index: Integer): string;
var
  Len: Integer;
{$IFDEF WIN32}
  Text: array[0..4095] of Char;
{$ENDIF}
begin
  Len := SendMessage(ListBox.Handle, LB_GETTEXT, Index,
    {$IFDEF WIN32} LongInt(@Text) {$ELSE} LongInt(@Result) {$ENDIF});
  if Len < 0 then Error(SListIndexError, Index);
{$IFDEF WIN32}
  SetString(Result, Text, Len);
{$ELSE}
  System.Move(Result[0], Result[1], Len);
  Result[0] := Char(Len);
{$ENDIF}
end;

function TMxListBoxStrings.GetObject(Index: Integer): TObject;
begin
  Result := TObject(ListBox.GetItemData(Index));
  if Longint(Result) = LB_ERR then Error(SListIndexError, Index);
end;

procedure TMxListBoxStrings.PutObject(Index: Integer; AObject: TObject);
begin
  ListBox.SetItemData(Index, LongInt(AObject));
end;

function TMxListBoxStrings.Add(const S: string): Integer;
{$IFNDEF WIN32}
var
  Text: array[0..255] of Char;
{$ENDIF}
begin
{$IFDEF WIN32}
  Result := SendMessage(ListBox.Handle, LB_ADDSTRING, 0, LongInt(PChar(S)));
{$ELSE}
  Result := SendMessage(ListBox.Handle, LB_ADDSTRING, 0, LongInt(StrPCopy(Text, S)));
{$ENDIF}
  if Result < 0 then raise EOutOfResources.Create(ResStr(SInsertLineError));
end;

procedure TMxListBoxStrings.Insert(Index: Integer; const S: string);
{$IFNDEF WIN32}
var
  Text: array[0..255] of Char;
{$ENDIF}
begin
  if SendMessage(ListBox.Handle, LB_INSERTSTRING, Index,
{$IFDEF WIN32}
    Longint(PChar(S))) < 0 then
{$ELSE}
    Longint(StrPCopy(Text, S))) < 0 then
{$ENDIF}
      raise EOutOfResources.Create(ResStr(SInsertLineError));
end;

procedure TMxListBoxStrings.Delete(Index: Integer);
begin
  ListBox.DeleteString(Index);
end;

procedure TMxListBoxStrings.Clear;
begin
  ListBox.ResetContent;
end;

procedure TMxListBoxStrings.SetUpdateState(Updating: Boolean);
begin
  SendMessage(ListBox.Handle, WM_SETREDRAW, Ord(not Updating), 0);
  if not Updating then ListBox.Refresh;
end;

{ TMxCustomListBox }

procedure ListIndexError(Index: Integer);

{$IFDEF WIN32}
  function ReturnAddr: Pointer;
  asm
          MOV     EAX,[EBP+4]
  end;
{$ELSE}
  function ReturnAddr: Pointer; assembler;
  asm
          MOV     AX,[BP].Word[2]
          MOV     DX,[BP].Word[4]
  end;
{$ENDIF}

begin
{$IFDEF RX_D3}
  raise EStringListError.CreateFmt(SListIndexError, [Index]) at ReturnAddr;
{$ELSE}
  raise EStringListError.CreateFmt('%s: %d', [LoadStr(SListIndexError),
    Index]) at ReturnAddr;
{$ENDIF}
end;

constructor TMxCustomListBox.Create(AOwner: TComponent);
const
  ListBoxStyle = [csSetCaption, csDoubleClicks];
begin
  inherited Create(AOwner);
{$IFDEF WIN32}
  if NewStyleControls then ControlStyle := ListBoxStyle
  else ControlStyle := ListBoxStyle + [csFramed];
{$ELSE}
  ControlStyle := ListBoxStyle + [csFramed];
{$ENDIF}
  Width := 121;
  Height := 97;
  TabStop := True;
  ParentColor := False;
  FItems := CreateItemList;
  TMxListBoxStrings(FItems).ListBox := Self;
  FCanvas := TControlCanvas.Create;
  TControlCanvas(FCanvas).Control := Self;
  FItemHeight := 16;
  FBorderStyle := bsSingle;
  FExtendedSelect := True;
end;

destructor TMxCustomListBox.Destroy;
begin
  inherited Destroy;
  FCanvas.Free;
  FItems.Free;
  FSaveItems.Free;
end;

function TMxCustomListBox.CreateItemList: TStrings;
begin
  Result := TMxListBoxStrings.Create;
end;

function TMxCustomListBox.GetItemData(Index: Integer): LongInt;
begin
  Result := SendMessage(Handle, LB_GETITEMDATA, Index, 0);
end;

procedure TMxCustomListBox.SetItemData(Index: Integer; AData: LongInt);
begin
  SendMessage(Handle, LB_SETITEMDATA, Index, AData);
end;

procedure TMxCustomListBox.DeleteString(Index: Integer);
begin
  SendMessage(Handle, LB_DELETESTRING, Index, 0);
end;

procedure TMxCustomListBox.SetHorizontalExtent;
begin
  SendMessage(Handle, LB_SETHORIZONTALEXTENT, FMaxItemWidth, 0);
end;

function TMxCustomListBox.GetItemWidth(Index: Integer): Integer;
var
  ATabWidth: Longint;
  S: string;
begin
  if (Style <> lbStandard) and Assigned(FOnGetItemWidth) and
    Assigned(FOnDrawItem) then
  begin
    Result := 0;
    FOnGetItemWidth(Self, Index, Result);
  end
  else begin
    S := Items[Index] + 'x';
    if TabWidth > 0 then begin
      {if (FTabChar > #0) then
        for I := 1 to Length(S) do
          if S[I] = FTabChar then S[I] := #9;}
      ATabWidth := Round((TabWidth * Canvas.TextWidth('0')) * 0.25);
      Result := LoWord(GetTabbedTextExtent(Canvas.Handle, @S[1], Length(S),
        1, ATabWidth));
    end
    else Result := Canvas.TextWidth(S);
  end;
end;

procedure TMxCustomListBox.ResetHorizontalExtent;
var
  I: Integer;
begin
  FMaxItemWidth := 0;
  for I := 0 to Items.Count - 1 do
    FMaxItemWidth := Max(FMaxItemWidth, GetItemWidth(I));
  SetHorizontalExtent;
end;

procedure TMxCustomListBox.ResetContent;
begin
  SendMessage(Handle, LB_RESETCONTENT, 0, 0);
end;

procedure TMxCustomListBox.Clear;
begin
  FItems.Clear;
end;

procedure TMxCustomListBox.SetColumnWidth;
begin
  if FColumns > 0 then
    SendMessage(Handle, LB_SETCOLUMNWIDTH, (Width + FColumns - 3) div
      FColumns, 0);
end;

procedure TMxCustomListBox.SetColumns(Value: Integer);
begin
  if FColumns <> Value then
    if (FColumns = 0) or (Value = 0) then begin
      FColumns := Value;
      RecreateWnd;
    end
    else begin
      FColumns := Value;
      if HandleAllocated then SetColumnWidth;
    end;
end;

function TMxCustomListBox.GetItemIndex: Integer;
begin
  Result := SendMessage(Handle, LB_GETCURSEL, 0, 0);
end;

function TMxCustomListBox.GetSelCount: Integer;
begin
  Result := SendMessage(Handle, LB_GETSELCOUNT, 0, 0);
end;

procedure TMxCustomListBox.SetItemIndex(Value: Integer);
begin
  if GetItemIndex <> Value then
    SendMessage(Handle, LB_SETCURSEL, Value, 0);
end;

procedure TMxCustomListBox.SetExtendedSelect(Value: Boolean);
begin
  if Value <> FExtendedSelect then begin
    FExtendedSelect := Value;
    RecreateWnd;
  end;
end;

procedure TMxCustomListBox.SetIntegralHeight(Value: Boolean);
begin
  if Value <> FIntegralHeight then begin
    FIntegralHeight := Value;
    RecreateWnd;
  end;
end;

function TMxCustomListBox.GetAutoScroll: Boolean;
begin
  Result := FAutoScroll and (Columns = 0);
end;

procedure TMxCustomListBox.SetOnDrawItem(Value: TDrawItemEvent);
begin
  if Assigned(FOnDrawItem) <> Assigned(Value) then begin
    FOnDrawItem := Value;
    Perform(WM_HSCROLL, SB_TOP, 0);
    if HandleAllocated then
      if AutoScroll then ResetHorizontalExtent
      else SendMessage(Handle, LB_SETHORIZONTALEXTENT, 0, 0);
  end
  else FOnDrawItem := Value;
end;

procedure TMxCustomListBox.SetOnGetItemWidth(Value: TGetItemWidthEvent);
begin
  if Assigned(FOnGetItemWidth) <> Assigned(Value) then begin
    FOnGetItemWidth := Value;
    Perform(WM_HSCROLL, SB_TOP, 0);
    if HandleAllocated then
      if AutoScroll then ResetHorizontalExtent
      else SendMessage(Handle, LB_SETHORIZONTALEXTENT, 0, 0);
  end
  else FOnGetItemWidth := Value;
end;

procedure TMxCustomListBox.SetAutoScroll(Value: Boolean);
begin
  if AutoScroll <> Value then begin
    FAutoScroll := Value;
    Perform(WM_HSCROLL, SB_TOP, 0);
    if HandleAllocated then begin
      if AutoScroll then ResetHorizontalExtent
      else SendMessage(Handle, LB_SETHORIZONTALEXTENT, 0, 0);
    end;
  end;
end;

function TMxCustomListBox.GetItemHeight: Integer;
var
  R: TRect;
begin
  Result := FItemHeight;
  if HandleAllocated and (FStyle = lbStandard) then begin
    Perform(LB_GETITEMRECT, 0, Longint(@R));
    Result := R.Bottom - R.Top;
  end;
end;

procedure TMxCustomListBox.SetItemHeight(Value: Integer);
begin
  if (FItemHeight <> Value) and (Value > 0) then begin
    FItemHeight := Value;
    RecreateWnd;
  end;
end;

procedure TMxCustomListBox.SetTabWidth(Value: Integer);
begin
  if Value < 0 then Value := 0;
  if FTabWidth <> Value then begin
    FTabWidth := Value;
    RecreateWnd;
  end;
end;

procedure TMxCustomListBox.SetMultiSelect(Value: Boolean);
begin
  if FMultiSelect <> Value then begin
    FMultiSelect := Value;
    RecreateWnd;
  end;
end;

function TMxCustomListBox.GetSelected(Index: Integer): Boolean;
var
  R: Longint;
begin
  R := SendMessage(Handle, LB_GETSEL, Index, 0);
  if R = LB_ERR then ListIndexError(Index);
  Result := LongBool(R);
end;

procedure TMxCustomListBox.SetSelected(Index: Integer; Value: Boolean);
begin
  if MultiSelect then begin
    if SendMessage(Handle, LB_SETSEL, Ord(Value), Index) = LB_ERR then
      ListIndexError(Index);
  end
  else begin
    if Value then SetItemIndex(Index)
    else if (ItemIndex = Index) then SetItemIndex(-1);
  end;
end;

procedure TMxCustomListBox.SetSorted(Value: Boolean);
begin
  if FSorted <> Value then begin
    FSorted := Value;
    RecreateWnd;
  end;
end;

procedure TMxCustomListBox.SetStyle(Value: TListBoxStyle);
begin
  if FStyle <> Value then begin
    FStyle := Value;
    RecreateWnd;
  end;
end;

function TMxCustomListBox.GetTopIndex: Integer;
begin
  Result := SendMessage(Handle, LB_GETTOPINDEX, 0, 0);
end;

procedure TMxCustomListBox.SetBorderStyle(Value: TBorderStyle);
begin
  if FBorderStyle <> Value then begin
    FBorderStyle := Value;
    RecreateWnd;
  end;
end;

procedure TMxCustomListBox.SetTopIndex(Value: Integer);
begin
  if GetTopIndex <> Value then SendMessage(Handle, LB_SETTOPINDEX, Value, 0);
end;

procedure TMxCustomListBox.SetGraySelection(Value: Boolean);
begin
  if FGraySelection <> Value then begin
    FGraySelection := Value;
    if not Focused then Invalidate;
  end;
end;

procedure TMxCustomListBox.SetItems(Value: TStrings);
begin
  Items.Assign(Value);
end;

function TMxCustomListBox.ItemAtPos(Pos: TPoint; Existing: Boolean): Integer;
var
  Count: Integer;
  ItemRect: TRect;
begin
  if PtInRect(ClientRect, Pos) then begin
    Result := TopIndex;
    Count := Items.Count;
    while Result < Count do begin
      Perform(LB_GETITEMRECT, Result, Longint(@ItemRect));
      if PtInRect(ItemRect, Pos) then Exit;
      Inc(Result);
    end;
    if not Existing then Exit;
  end;
  Result := -1;
end;

function TMxCustomListBox.ItemRect(Index: Integer): TRect;
var
  Count: Integer;
begin
  Count := Items.Count;
  if (Index = 0) or (Index < Count) then
    Perform(LB_GETITEMRECT, Index, Longint(@Result))
  else if Index = Count then begin
    Perform(LB_GETITEMRECT, Index - 1, Longint(@Result));
    OffsetRect(Result, 0, Result.Bottom - Result.Top);
  end
  else FillChar(Result, SizeOf(Result), 0);
end;

procedure TMxCustomListBox.CreateParams(var Params: TCreateParams);
type
  PSelects = ^TSelects;
  TSelects = array[Boolean] of Longword;
const
  BorderStyles: array[TBorderStyle] of Longword = (0, WS_BORDER);
  Styles: array[TListBoxStyle] of Longword =
    (0, LBS_OWNERDRAWFIXED, LBS_OWNERDRAWVARIABLE, 0, 0);
  Sorteds: array[Boolean] of Longword = (0, LBS_SORT);
  MultiSelects: array[Boolean] of Longword = (0, LBS_MULTIPLESEL);
  ExtendSelects: array[Boolean] of Longword = (0, LBS_EXTENDEDSEL);
  IntegralHeights: array[Boolean] of Longword = (LBS_NOINTEGRALHEIGHT, 0);
  MultiColumns: array[Boolean] of Longword = (0, LBS_MULTICOLUMN);
  TabStops: array[Boolean] of Longword = (0, LBS_USETABSTOPS);
var
  Selects: PSelects;
begin
  inherited CreateParams(Params);
  CreateSubClass(Params, 'LISTBOX');
  with Params do begin
{$IFNDEF WIN32}
    Inc(X); Inc(Y);
    Dec(Width, 2); Dec(Height, 2);
{$ENDIF}
    Selects := @MultiSelects;
    if FExtendedSelect then Selects := @ExtendSelects;
    Style := Style or (WS_HSCROLL or WS_VSCROLL or LBS_HASSTRINGS or
      LBS_NOTIFY) or Styles[FStyle] or Sorteds[FSorted] or
      Selects^[FMultiSelect] or IntegralHeights[FIntegralHeight] or
      MultiColumns[FColumns <> 0] or BorderStyles[FBorderStyle] or
      TabStops[FTabWidth <> 0];
{$IFDEF WIN32}
    if NewStyleControls and Ctl3D and (FBorderStyle = bsSingle) then begin
      Style := Style and not WS_BORDER;
      ExStyle := ExStyle or WS_EX_CLIENTEDGE;
    end;
{$ENDIF}
    WindowClass.Style := WindowClass.Style and not (CS_HREDRAW or CS_VREDRAW);
  end;
end;

procedure TMxCustomListBox.CreateWnd;
var
  W, H: Integer;
begin
  W := Width;
  H := Height;
  inherited CreateWnd;
  SetWindowPos(Handle, 0, Left, Top, W, H, SWP_NOZORDER or SWP_NOACTIVATE);
  if FTabWidth <> 0 then
    SendMessage(Handle, LB_SETTABSTOPS, 1, Longint(@FTabWidth));
  SetColumnWidth;
  if FSaveItems <> nil then begin
    FItems.Assign(FSaveItems);
    SetTopIndex(FSaveTopIndex);
    SetItemIndex(FSaveItemIndex);
    FSaveItems.Free;
    FSaveItems := nil;
  end;
end;

procedure TMxCustomListBox.DestroyWnd;
begin
  if FItems.Count > 0 then begin
    FSaveItems := TStringList.Create;
    FSaveItems.Assign(FItems);
    FSaveTopIndex := GetTopIndex;
    FSaveItemIndex := GetItemIndex;
  end;
  inherited DestroyWnd;
end;

procedure TMxCustomListBox.WndProc(var Message: TMessage);
begin
  if AutoScroll then begin
    case Message.Msg of
      LB_ADDSTRING, LB_INSERTSTRING:
        begin
          inherited WndProc(Message);
          FMaxItemWidth := Max(FMaxItemWidth, GetItemWidth(Message.Result));
          SetHorizontalExtent;
          Exit;
        end;
      LB_DELETESTRING:
        begin
          if GetItemWidth(Message.wParam) >= FMaxItemWidth then begin
            Perform(WM_HSCROLL, SB_TOP, 0);
            inherited WndProc(Message);
            ResetHorizontalExtent;
          end
          else inherited WndProc(Message);
          Exit;
        end;
      LB_RESETCONTENT:
        begin
          FMaxItemWidth := 0;
          SetHorizontalExtent;
          Perform(WM_HSCROLL, SB_TOP, 0);
          inherited WndProc(Message);
          Exit;
        end;
      WM_SETFONT:
        begin
          inherited WndProc(Message);
          Canvas.Font.Assign(Self.Font);
          ResetHorizontalExtent;
          Exit;
        end;
    end;
  end;
  {for auto drag mode, let listbox handle itself, instead of TControl}
  if not (csDesigning in ComponentState) and ((Message.Msg = WM_LBUTTONDOWN) or
    (Message.Msg = WM_LBUTTONDBLCLK)) and not Dragging then
  begin
    if DragMode = dmAutomatic then begin
      if IsControlMouseMsg(TWMMouse(Message)) then Exit;
      ControlState := ControlState + [csLButtonDown];
      Dispatch(Message);  {overrides TControl's BeginDrag}
      Exit;
    end;
  end;
  inherited WndProc(Message);
end;

procedure TMxCustomListBox.WMLButtonDown(var Message: TWMLButtonDown);
var
  ItemNo: Integer;
  ShiftState: TShiftState;
begin
  ShiftState := KeysToShiftState(Message.Keys);
  if (DragMode = dmAutomatic) and FMultiSelect then begin
    if not (ssShift in ShiftState) or (ssCtrl in ShiftState) then begin
      ItemNo := ItemAtPos(SmallPointToPoint(Message.Pos), True);
      if (ItemNo >= 0) and (Selected[ItemNo]) then begin
        BeginDrag(False);
        Exit;
      end;
    end;
  end;
  inherited;
  if (DragMode = dmAutomatic) and not (FMultiSelect and
    ((ssCtrl in ShiftState) or (ssShift in ShiftState))) then
    BeginDrag(False);
end;

procedure TMxCustomListBox.WMNCHitTest(var Msg: TWMNCHitTest);
begin
  if csDesigning in ComponentState then DefaultHandler(Msg)
  else inherited;
end;

procedure TMxCustomListBox.CNCommand(var Message: TWMCommand);
begin
  case Message.NotifyCode of
    LBN_SELCHANGE:
      begin
{$IFDEF RX_D3}
        inherited Changed;
{$ENDIF}
        Click;
      end;
    LBN_DBLCLK: DblClick;
  end;
end;

procedure TMxCustomListBox.WMPaint(var Message: TWMPaint);

  procedure PaintListBox;
  var
    DrawItemMsg: TWMDrawItem;
    MeasureItemMsg: TWMMeasureItem;
    DrawItemStruct: TDrawItemStruct;
    MeasureItemStruct: TMeasureItemStruct;
    R: TRect;
    Y, I, H, W: Integer;
  begin
    { Initialize drawing records }
    DrawItemMsg.Msg := CN_DRAWITEM;
    DrawItemMsg.DrawItemStruct := @DrawItemStruct;
    DrawItemMsg.Ctl := Handle;
    DrawItemStruct.CtlType := ODT_LISTBOX;
    DrawItemStruct.itemAction := ODA_DRAWENTIRE;
    DrawItemStruct.itemState := 0;
    DrawItemStruct.hDC := Message.DC;
    DrawItemStruct.CtlID := Handle;
    DrawItemStruct.hwndItem := Handle;
    { Intialize measure records }
    MeasureItemMsg.Msg := CN_MEASUREITEM;
    MeasureItemMsg.IDCtl := Handle;
    MeasureItemMsg.MeasureItemStruct := @MeasureItemStruct;
    MeasureItemStruct.CtlType := ODT_LISTBOX;
    MeasureItemStruct.CtlID := Handle;
    { Draw the listbox }
    Y := 0;
    I := TopIndex;
    GetClipBox(Message.DC, R);
    H := Height;
    W := Width;
    while Y < H do begin
      MeasureItemStruct.itemID := I;
      if I < Items.Count then
        MeasureItemStruct.itemData := Longint(Pointer(Items.Objects[I]));
      MeasureItemStruct.itemWidth := W;
      MeasureItemStruct.itemHeight := FItemHeight;
      DrawItemStruct.itemData := MeasureItemStruct.itemData;
      DrawItemStruct.itemID := I;
      Dispatch(MeasureItemMsg);
      DrawItemStruct.rcItem := Rect(0, Y, MeasureItemStruct.itemWidth,
        Y + Integer(MeasureItemStruct.itemHeight));
      Dispatch(DrawItemMsg);
      Inc(Y, MeasureItemStruct.itemHeight);
      Inc(I);
      if I >= Items.Count then break;
    end;
  end;

begin
  if Message.DC <> 0 then PaintListBox
  else inherited;
end;

procedure TMxCustomListBox.WMSize(var Message: TWMSize);
begin
  inherited;
  SetColumnWidth;
end;

procedure TMxCustomListBox.DragCanceled;
var
  M: TWMMouse;
{$IFDEF WIN32}
  MousePos: TPoint;
{$ENDIF}
begin
  with M do begin
    Msg := WM_LBUTTONDOWN;
{$IFDEF WIN32}
    GetCursorPos(MousePos);
    Pos := PointToSmallPoint(ScreenToClient(MousePos));
{$ELSE}
    GetCursorPos(Pos);
    Pos := ScreenToClient(Pos);
{$ENDIF}
    Keys := 0;
    Result := 0;
  end;
  DefaultHandler(M);
  M.Msg := WM_LBUTTONUP;
  DefaultHandler(M);
end;

procedure TMxCustomListBox.DefaultDrawText(X, Y: Integer; const S: string);
var
  ATabWidth: Longint;
begin
{$IFDEF RX_D4}
  TControlCanvas(FCanvas).UpdateTextFlags;
{$ENDIF}
  if FTabWidth = 0 then FCanvas.TextOut(X, Y, S)
  else begin
    ATabWidth := Round((TabWidth * Canvas.TextWidth('0')) * 0.25);
    TabbedTextOut(FCanvas.Handle, X, Y, @S[1], Length(S), 1, ATabWidth, X);
  end;
end;

procedure TMxCustomListBox.DrawItem(Index: Integer; Rect: TRect;
  State: TOwnerDrawState);
begin
  if Assigned(FOnDrawItem) then FOnDrawItem(Self, Index, Rect, State)
  else begin
    FCanvas.FillRect(Rect);
    if Index < Items.Count then begin
{$IFDEF RX_D4}
      if not UseRightToLeftAlignment then Inc(Rect.Left, 2)
      else Dec(Rect.Right, 2);
{$ELSE}
      Inc(Rect.Left, 2);
{$ENDIF}
      DefaultDrawText(Rect.Left, Max(Rect.Top, (Rect.Bottom +
        Rect.Top - Canvas.TextHeight('Wy')) div 2), Items[Index]);
    end;
  end;
end;

procedure TMxCustomListBox.MeasureItem(Index: Integer; var Height: Integer);
begin
  if Assigned(FOnMeasureItem) then FOnMeasureItem(Self, Index, Height)
end;

procedure TMxCustomListBox.CNDrawItem(var Message: TWMDrawItem);
var
  State: TOwnerDrawState;
begin
  with Message.DrawItemStruct^ do begin
{$IFDEF WIN32}
 {$IFDEF RX_D5}
    State := TOwnerDrawState(LongRec(itemState).Lo);
 {$ELSE}
    State := TOwnerDrawState(WordRec(LongRec(itemState).Lo).Lo);
 {$ENDIF}
{$ELSE}
    State := TOwnerDrawState(WordRec(itemState).Lo);
{$ENDIF}
    FCanvas.Handle := hDC;
    FCanvas.Font := Font;
    FCanvas.Brush := Brush;
    if (Integer(itemID) >= 0) and (odSelected in State) then begin
      with FCanvas do
        if not (csDesigning in ComponentState) and FGraySelection and
          not Focused then
        begin
          Brush.Color := clBtnFace;
          if ColorToRGB(Font.Color) = ColorToRGB(clBtnFace) then
            Font.Color := clBtnText;
        end
        else begin
          Brush.Color := clHighlight;
          Font.Color := clHighlightText
        end;
    end;
    if Integer(itemID) >= 0 then DrawItem(itemID, rcItem, State)
    else FCanvas.FillRect(rcItem);
    if odFocused in State then DrawFocusRect(hDC, rcItem);
    FCanvas.Handle := 0;
  end;
end;

procedure TMxCustomListBox.CNMeasureItem(var Message: TWMMeasureItem);
begin
  with Message.MeasureItemStruct^ do begin
    itemHeight := FItemHeight;
    if FStyle = lbOwnerDrawVariable then
      MeasureItem(itemID, Integer(itemHeight));
  end;
end;

procedure TMxCustomListBox.WMKillFocus(var Msg: TWMKillFocus);
begin
  inherited;
  if FGraySelection and MultiSelect and (SelCount > 1) then Invalidate;
end;

procedure TMxCustomListBox.WMSetFocus(var Msg: TWMSetFocus);
begin
  inherited;
  if FGraySelection and MultiSelect and (SelCount > 1) then Invalidate;
end;

{$IFDEF WIN32}
procedure TMxCustomListBox.CMCtl3DChanged(var Message: TMessage);
begin
  if NewStyleControls and (FBorderStyle = bsSingle) then RecreateWnd;
  inherited;
end;
{$ENDIF}

{ TCheckListBoxItem }

type
  TCheckListBoxItem = class
  private
    FData: LongInt;
    FState: TCheckBoxState;
    FEnabled: Boolean;
    function GetChecked: Boolean;
  public
    constructor Create;
    property Checked: Boolean read GetChecked;
    property Enabled: Boolean read FEnabled write FEnabled;
    property State: TCheckBoxState read FState write FState;
  end;

constructor TCheckListBoxItem.Create;
begin
  inherited Create;
  FState := clbDefaultState;
  FEnabled := clbDefaultEnabled;
end;

function TCheckListBoxItem.GetChecked: Boolean;
begin
  Result := FState = cbChecked;
end;

{ TCheckListBoxStrings }

type
  TCheckListBoxStrings = class(TMxListBoxStrings)
  public
    procedure Exchange(Index1, Index2: Integer); override;
    procedure Move(CurIndex, NewIndex: Integer); override;
  end;

procedure TCheckListBoxStrings.Exchange(Index1, Index2: Integer);
var
  TempEnabled1, TempEnabled2: Boolean;
  TempState1, TempState2: TCheckBoxState;
begin
  with TMxCheckListBox(ListBox) do begin
    TempState1 := State[Index1];
    TempEnabled1 := EnabledItem[Index1];
    TempState2 := State[Index2];
    TempEnabled2 := EnabledItem[Index2];
    inherited Exchange(Index1, Index2);
    State[Index1] := TempState2;
    EnabledItem[Index1] := TempEnabled2;
    State[Index2] := TempState1;
    EnabledItem[Index2] := TempEnabled1;
  end;
end;

procedure TCheckListBoxStrings.Move(CurIndex, NewIndex: Integer);
var
  TempEnabled: Boolean;
  TempState: TCheckBoxState;
begin
  with TMxCheckListBox(ListBox) do begin
    TempState := State[CurIndex];
    TempEnabled := EnabledItem[CurIndex];
    inherited Move(CurIndex, NewIndex);
    State[NewIndex] := TempState;
    EnabledItem[NewIndex] := TempEnabled;
  end;
end;

{ TMxCheckListBox }

const
  FCheckBitmap: TBitmap = nil;

function CheckBitmap: TBitmap;
begin
  if FCheckBitmap = nil then begin
    FCheckBitmap := TBitmap.Create;
    FCheckBitmap.Handle := LoadBitmap(hInstance, 'CHECK_IMAGES');
  end;
  Result := FCheckBitmap;
end;

procedure DestroyLocals; far;
begin
  if FCheckBitmap <> nil then begin
    FCheckBitmap.Free;
    FCheckBitmap := nil;
  end;
end;

const
  InternalVersion = 202; { for backward compatibility only }

constructor TMxCheckListBox.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FAutoScroll := True;
  with CheckBitmap do begin
    FCheckWidth := Width div 6;
    FCheckHeight := Height div 3;
  end;
  FDrawBitmap := TBitmap.Create;
  with FDrawBitmap do begin
    Width := FCheckWidth;
    Height := FCheckHeight;
  end;
  FIniLink := TIniLink.Create;
  FIniLink.OnSave := IniSave;
  FIniLink.OnLoad := IniLoad;
end;

destructor TMxCheckListBox.Destroy;
begin
  FSaveStates.Free;
  FSaveStates := nil;
  FDrawBitmap.Free;
  FDrawBitmap := nil;
  FIniLink.Free;
  inherited Destroy;
end;

procedure TMxCheckListBox.Loaded;
begin
  inherited Loaded;
  UpdateCheckStates;
end;

function TMxCheckListBox.CreateItemList: TStrings;
begin
  Result := TCheckListBoxStrings.Create;
end;

const
  sCount = 'Count';
  sItem = 'Item';

procedure TMxCheckListBox.InternalSaveStates(IniFile: TObject;
  const Section: string);
var
  I: Integer;
begin
  IniEraseSection(IniFile, Section);
  IniWriteInteger(IniFile, Section, sCount, Items.Count);
  for I := 0 to Items.Count - 1 do
    IniWriteInteger(IniFile, Section, sItem + IntToStr(I), Integer(State[I]));
end;

procedure TMxCheckListBox.InternalRestoreStates(IniFile: TObject;
  const Section: string);
var
  I: Integer;
  ACount: Integer;
begin
  ACount := Min(IniReadInteger(IniFile, Section, sCount, 0), Items.Count);
  for I := 0 to ACount - 1 do begin
    State[I] := TCheckBoxState(IniReadInteger(IniFile, Section,
      sItem + IntToStr(I), Integer(clbDefaultState)));
    if (State[I] = cbChecked) and (FCheckKind = ckRadioButtons) then Exit;
  end;
end;

{$IFDEF WIN32}
procedure TMxCheckListBox.SaveStatesReg(IniFile: TRegIniFile);
begin
  InternalSaveStates(IniFile, GetDefaultSection(Self));
end;

procedure TMxCheckListBox.RestoreStatesReg(IniFile: TRegIniFile);
begin
  InternalRestoreStates(IniFile, GetDefaultSection(Self));
end;
{$ENDIF WIN32}

procedure TMxCheckListBox.SaveStates(IniFile: TIniFile);
begin
  InternalSaveStates(IniFile, GetDefaultSection(Self));
end;

procedure TMxCheckListBox.RestoreStates(IniFile: TIniFile);
begin
  InternalRestoreStates(IniFile, GetDefaultSection(Self));
end;

function TMxCheckListBox.GetStorage: TFormPlacement;
begin
  Result := FIniLink.Storage;
end;

procedure TMxCheckListBox.SetStorage(Value: TFormPlacement);
begin
  FIniLink.Storage := Value;
end;

procedure TMxCheckListBox.IniSave(Sender: TObject);
begin
  if (Name <> '') and (FIniLink.IniObject <> nil) then
    InternalSaveStates(FIniLink.IniObject, FIniLink.RootSection +
      GetDefaultSection(Self));
end;

procedure TMxCheckListBox.IniLoad(Sender: TObject);
begin
  if (Name <> '') and (FIniLink.IniObject <> nil) then
    InternalRestoreStates(FIniLink.IniObject, FIniLink.RootSection +
      GetDefaultSection(Self));
end;

procedure TMxCheckListBox.ReadCheckData(Reader: TReader);
var
  I: Integer;
begin
  Items.BeginUpdate;
  try
    Reader.ReadListBegin;
    Clear;
    while not Reader.EndOfList do begin
      I := Items.Add(Reader.ReadString);
      if FReserved >= InternalVersion then begin
        State[I] := TCheckBoxState(Reader.ReadInteger);
        EnabledItem[I] := Reader.ReadBoolean;
      end
      else begin { for backward compatibility only }
        Checked[I] := Reader.ReadBoolean;
        EnabledItem[I] := Reader.ReadBoolean;
        if FReserved > 0 then
          State[I] := TCheckBoxState(Reader.ReadInteger);
      end;
    end;
    Reader.ReadListEnd;
    UpdateCheckStates;
  finally
    Items.EndUpdate;
  end;
end;

procedure TMxCheckListBox.WriteCheckData(Writer: TWriter);
var
  I: Integer;
begin
  with Writer do begin
    WriteListBegin;
    for I := 0 to Items.Count - 1 do begin
      WriteString(Items[I]);
      WriteInteger(Integer(Self.State[I]));
      WriteBoolean(EnabledItem[I]);
    end;
    WriteListEnd;
  end;
end;

procedure TMxCheckListBox.ReadVersion(Reader: TReader);
begin
  FReserved := Reader.ReadInteger;
end;

procedure TMxCheckListBox.WriteVersion(Writer: TWriter);
begin
  Writer.WriteInteger(InternalVersion);
end;

procedure TMxCheckListBox.DefineProperties(Filer: TFiler);

{$IFDEF WIN32}
  function DoWrite: Boolean;
  var
    I: Integer;
    Ancestor: TMxCheckListBox;
  begin
    Result := False;
    Ancestor := TMxCheckListBox(Filer.Ancestor);
    if (Ancestor <> nil) and (Ancestor.Items.Count = Items.Count) and
      (Ancestor.Items.Count > 0) then
      for I := 1 to Items.Count - 1 do begin
        Result := (CompareText(Items[I], Ancestor.Items[I]) <> 0) or
          (State[I] <> Ancestor.State[I]) or
          (EnabledItem[I] <> Ancestor.EnabledItem[I]);
        if Result then Break;
      end
    else Result := Items.Count > 0;
  end;
{$ENDIF}

begin
  inherited DefineProperties(Filer);
  Filer.DefineProperty('InternalVersion', ReadVersion, WriteVersion,
    {$IFDEF WIN32} Filer.Ancestor = nil {$ELSE} True {$ENDIF});
  Filer.DefineProperty('Strings', ReadCheckData, WriteCheckData,
    {$IFDEF WIN32} DoWrite {$ELSE} Items.Count > 0 {$ENDIF});
end;

procedure TMxCheckListBox.CreateWnd;
begin
  inherited CreateWnd;
  if FSaveStates <> nil then begin
    FSaveStates.Free;
    FSaveStates := nil;
  end;
  ResetItemHeight;
end;

procedure TMxCheckListBox.DestroyWnd;
begin
  inherited DestroyWnd;
end;

procedure TMxCheckListBox.WMDestroy(var Msg: TWMDestroy);
var
  I: Integer;
begin
  if Items.Count > 0 then begin
    if FSaveStates <> nil then FSaveStates.Clear
    else FSaveStates := TList.Create;
    for I := 0 to Items.Count - 1 do begin
      FSaveStates.Add(TObject(MakeLong(Ord(EnabledItem[I]), Word(State[I]))));
      FindCheckObject(I).Free;
    end;
  end;
  inherited;
end;

procedure TMxCheckListBox.CreateParams(var Params: TCreateParams);
begin
  inherited CreateParams(Params);
  with Params do
    if Style and (LBS_OWNERDRAWFIXED or LBS_OWNERDRAWVARIABLE) = 0 then
      Style := Style or LBS_OWNERDRAWFIXED;
end;

procedure TMxCheckListBox.SetItems(Value: TStrings);
var
  I: Integer;
begin
  Items.BeginUpdate;
  try
    inherited SetItems(Value);
    if (Value <> nil) and (Value is TMxListBoxStrings) and
      (TMxListBoxStrings(Value).ListBox <> nil) and
      (TMxListBoxStrings(Value).ListBox is TMxCheckListBox) then
    begin
      for I := 0 to Items.Count - 1 do
        if I < Value.Count then begin
          Self.State[I] := TMxCheckListBox(TMxListBoxStrings(Value).ListBox).State[I];
          EnabledItem[I] := TMxCheckListBox(TMxListBoxStrings(Value).ListBox).EnabledItem[I];
        end;
    end;
  finally
    Items.EndUpdate;
  end;
end;

function TMxCheckListBox.GetItemWidth(Index: Integer): Integer;
begin
  Result := inherited GetItemWidth(Index) + GetCheckWidth;
end;

function TMxCheckListBox.GetCheckWidth: Integer;
begin
  Result := FCheckWidth + 2;
end;

function TMxCheckListBox.GetAllowGrayed: Boolean;
begin
  Result := FAllowGrayed and (FCheckKind in [ckCheckBoxes, ckCheckMarks]);
end;

procedure TMxCheckListBox.CMFontChanged(var Message: TMessage);
begin
  inherited;
  ResetItemHeight;
end;

function TMxCheckListBox.GetItemHeight: Integer;
var
  R: TRect;
begin
  Result := FItemHeight;
  if HandleAllocated and ((FStyle = lbStandard) or
    ((FStyle = lbOwnerDrawFixed) and not Assigned(FOnDrawItem))) then
  begin
    Perform(LB_GETITEMRECT, 0, Longint(@R));
    Result := R.Bottom - R.Top;
  end;
end;

procedure TMxCheckListBox.ResetItemHeight;
var
  H: Integer;
begin
  if (Style = lbStandard) or ((Style = lbOwnerDrawFixed) and
    not Assigned(FOnDrawItem)) then
  begin
    Canvas.Font := Font;
    H := Max(Canvas.TextHeight('Wg'), FCheckHeight);
    if Style = lbOwnerDrawFixed then H := Max(H, FItemHeight);
    Perform(LB_SETITEMHEIGHT, 0, H);
    if (H * Items.Count) <= ClientHeight then
      SetScrollRange(Handle, SB_VERT, 0, 0, True);
  end;
end;

procedure TMxCheckListBox.DrawItem(Index: Integer; Rect: TRect;
  State: TOwnerDrawState);
var
  R: TRect;
  SaveEvent: TDrawItemEvent;
begin
  if Index < Items.Count then begin
    R := Rect;
{$IFDEF RX_D4}
    if not UseRightToLeftAlignment then begin
      R.Right := Rect.Left;
      R.Left := R.Right - GetCheckWidth;
    end
    else
    begin
      R.Left := Rect.Right;
      R.Right := R.Left + GetCheckWidth;
    end;
{$ELSE}
    R.Right := Rect.Left;
    R.Left := R.Right - GetCheckWidth;
{$ENDIF}
    DrawCheck(R, GetState(Index), EnabledItem[Index]);
    if not EnabledItem[Index] then
      if odSelected in State then Canvas.Font.Color := clInactiveCaptionText
      else Canvas.Font.Color := clGrayText;
  end;
  if (Style = lbStandard) and Assigned(FOnDrawItem) then begin
    SaveEvent := OnDrawItem;
    OnDrawItem := nil;
    try
      inherited DrawItem(Index, Rect, State);
    finally
      OnDrawItem := SaveEvent;
    end;
  end
  else inherited DrawItem(Index, Rect, State);
end;

procedure TMxCheckListBox.CNDrawItem(var Message: TWMDrawItem);
begin
  with Message.DrawItemStruct^ do
{$IFDEF RX_D4}
    if not UseRightToLeftAlignment then
      rcItem.Left := rcItem.Left + GetCheckWidth
    else
      rcItem.Right := rcItem.Right - GetCheckWidth;
{$ELSE}
    rcItem.Left := rcItem.Left + GetCheckWidth;
{$ENDIF}
  inherited;
end;

procedure TMxCheckListBox.DrawCheck(R: TRect; AState: TCheckBoxState;
  Enabled: Boolean);
const
  CheckImages: array[TCheckBoxState, TCheckKind, Boolean] of Integer =
    (((3, 0), (9,  6), (15, 12)),   { unchecked }
     ((4, 1), (10, 7), (16, 13)),   { checked   }
     ((5, 2), (11, 8), (17, 14)));  { grayed    }
var
  DrawRect: TRect;
  SaveColor: TColor;
begin
  DrawRect.Left := R.Left + (R.Right - R.Left - FCheckWidth) div 2;
  DrawRect.Top := R.Top + (R.Bottom - R.Top - FCheckHeight) div 2;
  DrawRect.Right := DrawRect.Left + FCheckWidth;
  DrawRect.Bottom := DrawRect.Top + FCheckHeight;
  SaveColor := Canvas.Brush.Color;
  AssignBitmapCell(CheckBitmap, FDrawBitmap, 6, 3,
    CheckImages[AState, FCheckKind, Enabled]);
  Canvas.Brush.Color := Self.Color;
  try
    Canvas.BrushCopy(DrawRect, FDrawBitmap, Bounds(0, 0, FCheckWidth,
      FCheckHeight), CheckBitmap.TransparentColor and not PaletteMask);
  finally
    Canvas.Brush.Color := SaveColor;
  end;
end;

procedure TMxCheckListBox.ApplyState(AState: TCheckBoxState;
  EnabledOnly: Boolean);
var
  I: Integer;
begin
  if FCheckKind in [ckCheckBoxes, ckCheckMarks] then
    for I := 0 to Items.Count - 1 do
      if not EnabledOnly or EnabledItem[I] then begin
        State[I] := AState;
      end;
end;

function TMxCheckListBox.GetCheckedIndex: Integer;
var
  I: Integer;
begin
  Result := -1;
  if FCheckKind = ckRadioButtons then
    for I := 0 to Items.Count - 1 do
      if State[I] = cbChecked then begin
        Result := I;
        Exit;
      end;
end;

procedure TMxCheckListBox.SetCheckedIndex(Value: Integer);
begin
  if (FCheckKind = ckRadioButtons) and (Items.Count > 0) then
    SetState(Max(Value, 0), cbChecked);
end;

procedure TMxCheckListBox.UpdateCheckStates;
begin
  if (FCheckKind = ckRadioButtons) and (Items.Count > 0) then begin
    FInUpdateStates := True;
    try
      SetState(Max(GetCheckedIndex, 0), cbChecked);
    finally
      FInUpdateStates := False;
    end;
  end;
end;

procedure TMxCheckListBox.SetCheckKind(Value: TCheckKind);
begin
  if FCheckKind <> Value then begin
    FCheckKind := Value;
    UpdateCheckStates;
    Invalidate;
  end;
end;

procedure TMxCheckListBox.SetChecked(Index: Integer; AChecked: Boolean);
const
  CheckStates: array[Boolean] of TCheckBoxState = (cbUnchecked, cbChecked);
begin
  SetState(Index, CheckStates[AChecked]);
end;

procedure TMxCheckListBox.SetState(Index: Integer; AState: TCheckBoxState);
var
  I: Integer;
begin
  if (AState <> GetState(Index)) or FInUpdateStates then begin
    if (FCheckKind = ckRadioButtons) and (AState = cbUnchecked) and
      (GetCheckedIndex = Index) then Exit;
    TCheckListBoxItem(GetCheckObject(Index)).State := AState;
    if (FCheckKind = ckRadioButtons) and (AState = cbChecked) then
      for I := Items.Count - 1 downto 0 do begin
        if (I <> Index) and (GetState(I) = cbChecked) then begin
          TCheckListBoxItem(GetCheckObject(I)).State := cbUnchecked;
          InvalidateCheck(I);
        end;
      end;
    InvalidateCheck(Index);
    if not (csReading in ComponentState) then ChangeItemState(Index);
  end;
end;

procedure TMxCheckListBox.SetItemEnabled(Index: Integer; Value: Boolean);
begin
  if Value <> GetItemEnabled(Index) then begin
    TCheckListBoxItem(GetCheckObject(Index)).Enabled := Value;
    InvalidateItem(Index);
  end;
end;

procedure TMxCheckListBox.InvalidateCheck(Index: Integer);
var
  R: TRect;
begin
  R := ItemRect(Index);
{$IFDEF RX_D4}
  if not UseRightToLeftAlignment then R.Right := R.Left + GetCheckWidth
  else R.Left := R.Right - GetCheckWidth;
{$ELSE}
  R.Right := R.Left + GetCheckWidth;
{$ENDIF}
  InvalidateRect(Handle, @R, not (csOpaque in ControlStyle));
  UpdateWindow(Handle);
end;

procedure TMxCheckListBox.InvalidateItem(Index: Integer);
var
  R: TRect;
begin
  R := ItemRect(Index);
  InvalidateRect(Handle, @R, not (csOpaque in ControlStyle));
  UpdateWindow(Handle);
end;

function TMxCheckListBox.GetChecked(Index: Integer): Boolean;
begin
  if IsCheckObject(Index) then
    Result := TCheckListBoxItem(GetCheckObject(Index)).GetChecked
  else Result := False;
end;

function TMxCheckListBox.GetState(Index: Integer): TCheckBoxState;
begin
  if IsCheckObject(Index) then
    Result := TCheckListBoxItem(GetCheckObject(Index)).State
  else Result := clbDefaultState;
  if (FCheckKind = ckRadioButtons) and (Result <> cbChecked) then
    Result := cbUnchecked;
end;

function TMxCheckListBox.GetItemEnabled(Index: Integer): Boolean;
begin
  if IsCheckObject(Index) then
    Result := TCheckListBoxItem(GetCheckObject(Index)).Enabled
  else Result := clbDefaultEnabled;
end;

procedure TMxCheckListBox.KeyPress(var Key: Char);
begin
  inherited KeyPress(Key);
  case Key of
    ' ': 
      begin
        ToggleClickCheck(ItemIndex);
        Key := #0;
      end;
    '+':
      begin
        ApplyState(cbChecked, True);
        ClickCheck;
      end;
    '-':
      begin
        ApplyState(cbUnchecked, True);
        ClickCheck;
      end;
  end;
end;

procedure TMxCheckListBox.MouseDown(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
var
  Index: Integer;
begin
  inherited MouseDown(Button, Shift, X, Y);
  if Button = mbLeft then begin
    Index := ItemAtPos(Point(X,Y), True);
    if (Index <> -1) then begin
{$IFDEF RX_D4}
      if not UseRightToLeftAlignment then begin
        if X - ItemRect(Index).Left < GetCheckWidth then
          ToggleClickCheck(Index);
      end
      else begin
        Dec(X, ItemRect(Index).Right - GetCheckWidth);
        if (X > 0) and (X < GetCheckWidth) then
          ToggleClickCheck(Index);
      end;
{$ELSE}
      if X - ItemRect(Index).Left < GetCheckWidth then
        ToggleClickCheck(Index);
{$ENDIF}
    end;
  end;
end;

procedure TMxCheckListBox.ToggleClickCheck(Index: Integer);
var
  State: TCheckBoxState;
begin
  if (Index >= 0) and (Index < Items.Count) and EnabledItem[Index] then begin
    State := Self.State[Index];
    case State of
      cbUnchecked:
        if AllowGrayed then State := cbGrayed else State := cbChecked;
      cbChecked: State := cbUnchecked;
      cbGrayed: State := cbChecked;
    end;
    Self.State[Index] := State;
    ClickCheck;
  end;
end;

procedure TMxCheckListBox.ChangeItemState(Index: Integer);
begin
  if Assigned(FOnStateChange) then FOnStateChange(Self, Index);
end;

procedure TMxCheckListBox.ClickCheck;
begin
  if Assigned(FOnClickCheck) then FOnClickCheck(Self);
end;

function TMxCheckListBox.GetItemData(Index: Integer): LongInt;
var
  Item: TCheckListBoxItem;
begin
  Result := 0;
  if IsCheckObject(Index) then begin
    Item := TCheckListBoxItem(GetCheckObject(Index));
    if Item <> nil then Result := Item.FData;
  end;
end;

function TMxCheckListBox.GetCheckObject(Index: Integer): TObject;
begin
  Result := FindCheckObject(Index);
  if Result = nil then Result := CreateCheckObject(Index);
end;

function TMxCheckListBox.FindCheckObject(Index: Integer): TObject;
var
  ItemData: Longint;
begin
  Result := nil;
  ItemData := inherited GetItemData(Index);
  if ItemData = LB_ERR then ListIndexError(Index)
  else begin
    Result := TCheckListBoxItem(ItemData);
    if not (Result is TCheckListBoxItem) then Result := nil;
  end;
end;

function TMxCheckListBox.CreateCheckObject(Index: Integer): TObject;
begin
  Result := TCheckListBoxItem.Create;
  inherited SetItemData(Index, LongInt(Result));
end;

function TMxCheckListBox.IsCheckObject(Index: Integer): Boolean;
begin
  Result := FindCheckObject(Index) <> nil;
end;

procedure TMxCheckListBox.SetItemData(Index: Integer; AData: LongInt);
var
  Item: TCheckListBoxItem;
  L: Longint;
begin
  Item := TCheckListBoxItem(GetCheckObject(Index));
  Item.FData := AData;
  if (FSaveStates <> nil) and (FSaveStates.Count > 0) then begin
    L := Longint(Pointer(FSaveStates[0]));
    Item.FState := TCheckBoxState(LongRec(L).Hi);
    Item.FEnabled := LongRec(L).Lo <> 0;
    FSaveStates.Delete(0);
  end;
end;

procedure TMxCheckListBox.ResetContent;
var
  I: Integer;
begin
  for I := Items.Count - 1 downto 0 do begin
    if IsCheckObject(I) then GetCheckObject(I).Free;
    inherited SetItemData(I, 0);
  end;
  inherited ResetContent;
end;

procedure TMxCheckListBox.DeleteString(Index: Integer);
begin
  if IsCheckObject(Index) then GetCheckObject(Index).Free;
  inherited SetItemData(Index, 0);
  inherited DeleteString(Index);
end;

{ TMxCustomLabel }

function DrawShadowText(DC: HDC; Str: PChar; Count: Integer; var Rect: TRect;
  Format: Word; ShadowSize: Byte; ShadowColor: TColorRef;
  ShadowPos: TShadowPosition): Integer;
var
  RText, RShadow: TRect;
  Color: TColorRef;
begin
  RText := Rect;
  RShadow := Rect;
  Color := SetTextColor(DC, ShadowColor);
  case ShadowPos of
    spLeftTop: OffsetRect(RShadow, -ShadowSize, -ShadowSize);
    spRightBottom: OffsetRect(RShadow, ShadowSize, ShadowSize);
    spLeftBottom:
      begin
        {OffsetRect(RText, ShadowSize, 0);}
        OffsetRect(RShadow, -ShadowSize, ShadowSize);
      end;
    spRightTop:
      begin
        {OffsetRect(RText, 0, ShadowSize);}
        OffsetRect(RShadow, ShadowSize, -ShadowSize);
      end;
  end; { case }
  Result := DrawText(DC, Str, Count, RShadow, Format);
  if Result > 0 then Inc(Result, ShadowSize);
  SetTextColor(DC, Color);
  DrawText(DC, Str, Count, RText, Format);
  UnionRect(Rect, RText, RShadow);
end;

constructor TMxCustomLabel.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  ControlStyle := ControlStyle + [csOpaque];
{$IFDEF WIN32}
  ControlStyle := ControlStyle + [csReplicatable];
{$ENDIF}
  Width := 65;
  Height := 17;
  FAutoSize := True;
  FShowAccelChar := True;
  FShadowColor := clBtnHighlight;
  FShadowSize := 1;
  FShadowPos := spLeftTop;
end;

function TMxCustomLabel.GetLabelCaption: string;
begin
  Result := Caption;
end;

function TMxCustomLabel.GetDefaultFontColor: TColor;
begin
  Result := Font.Color;
end;

procedure TMxCustomLabel.DoDrawText(var Rect: TRect; Flags: Word);
var
{$IFDEF WIN32}
  Text: string;
{$ELSE}
  Text: array[0..255] of Char;
{$ENDIF}
  PosShadow: TShadowPosition;
  SizeShadow: Byte;
  ColorShadow: TColor;
begin
{$IFDEF WIN32}
  Text := GetLabelCaption;
  if (Flags and DT_CALCRECT <> 0) and ((Text = '') or FShowAccelChar and
    (Text[1] = '&') and (Text[2] = #0)) then Text := Text + ' ';
{$ELSE}
  StrPLCopy(Text, GetLabelCaption, 255);
  if (Flags and DT_CALCRECT <> 0) and ((Text[0] = #0) or FShowAccelChar and
    (Text[0] = '&') and (Text[1] = #0)) then StrCopy(Text, ' ');
{$ENDIF}
  if not FShowAccelChar then Flags := Flags or DT_NOPREFIX;
{$IFDEF RX_D4}
  Flags := DrawTextBiDiModeFlags(Flags);
{$ENDIF}
  Canvas.Font := Font;
  Canvas.Font.Color := GetDefaultFontColor;
  PosShadow := FShadowPos;
  SizeShadow := FShadowSize;
  ColorShadow := FShadowColor;
  if not Enabled then begin
    if (FShadowSize = 0) and NewStyleControls then begin
      PosShadow := spRightBottom;
      SizeShadow := 1;
    end;
    Canvas.Font.Color := clGrayText;
    ColorShadow := clBtnHighlight;
  end;
{$IFDEF WIN32}
  DrawShadowText(Canvas.Handle, PChar(Text), Length(Text), Rect, Flags,
    SizeShadow, ColorToRGB(ColorShadow), PosShadow);
{$ELSE}
  DrawShadowText(Canvas.Handle, Text, StrLen(Text), Rect, Flags,
    SizeShadow, ColorToRGB(ColorShadow), PosShadow);
{$ENDIF}
end;

procedure TMxCustomLabel.Paint;
var
  Rect: TRect;
  DrawStyle: Integer;
begin
  if not Enabled and not (csDesigning in ComponentState) then
    FDragging := False;
  with Canvas do begin
    if not Transparent then begin
      Brush.Color := Self.Color;
      Brush.Style := bsSolid;
      FillRect(ClientRect);
    end;
    Brush.Style := bsClear;
    Rect := ClientRect;
    Inc(Rect.Left, FLeftMargin);
    Dec(Rect.Right, FRightMargin);
    InflateRect(Rect, -1, 0);
    DrawStyle := DT_EXPANDTABS or WordWraps[FWordWrap] or Alignments[FAlignment];
    { Calculate vertical layout }
    if FLayout <> tlTop then begin
      DoDrawText(Rect, DrawStyle or DT_CALCRECT);
      Rect.Left := ClientRect.Left + FLeftMargin;
      Rect.Right := ClientRect.Right - FRightMargin;
      if FLayout = tlBottom then OffsetRect(Rect, 0, Height - Rect.Bottom)
      else OffsetRect(Rect, 0, (Height - Rect.Bottom) div 2);
    end;
    DoDrawText(Rect, DrawStyle);
    if FShowFocus and Assigned(FFocusControl) and FFocused and
      not (csDesigning in ComponentState) then
    begin
      InflateRect(Rect, 1, 0);
{$IFDEF WIN32}
      Brush.Color := Self.Color;
{$ENDIF}
      DrawFocusRect(Rect);
    end;
  end;
end;

procedure TMxCustomLabel.AdjustBounds;
var
  DC: HDC;
  X: Integer;
  Rect: TRect;
  AAlignment: TAlignment;
begin
  if AutoSize then begin
    Rect := ClientRect;
    Inc(Rect.Left, FLeftMargin);
    Dec(Rect.Right, FRightMargin);
    InflateRect(Rect, -1, 0);
    DC := GetDC(0);
    Canvas.Handle := DC;
    DoDrawText(Rect, DT_EXPANDTABS or DT_CALCRECT or WordWraps[FWordWrap]);
    Dec(Rect.Left, FLeftMargin);
    Inc(Rect.Right, FRightMargin);
    Canvas.Handle := 0;
    ReleaseDC(0, DC);
    InflateRect(Rect, 1, 0);
    X := Left;
    AAlignment := FAlignment;
{$IFDEF RX_D4}
    if UseRightToLeftAlignment then ChangeBiDiModeAlignment(AAlignment);
{$ENDIF}
    if AAlignment = taRightJustify then Inc(X, Width - Rect.Right);
    SetBounds(X, Top, Rect.Right, Rect.Bottom);
  end;
end;

procedure TMxCustomLabel.SetAlignment(Value: TAlignment);
begin
  if FAlignment <> Value then begin
    FAlignment := Value;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetAutoSize(Value: Boolean);
begin
  if AutoSize <> Value then begin
    FAutoSize := Value;
    AdjustBounds;
  end;
end;

procedure TMxCustomLabel.SetLayout(Value: TTextLayout);
begin
  if FLayout <> Value then begin
    FLayout := Value;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetLeftMargin(Value: Integer);
begin
  if FLeftMargin <> Value then begin
    FLeftMargin := Max(Value, 0);
    AdjustBounds;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetRightMargin(Value: Integer);
begin
  if FRightMargin <> Value then begin
    FRightMargin := Max(Value, 0);
    AdjustBounds;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetShadowColor(Value: TColor);
begin
  if Value <> FShadowColor then begin
    FShadowColor := Value;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetShadowSize(Value: Byte);
begin
  if Value <> FShadowSize then begin
    FShadowSize := Value;
    AdjustBounds;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetShadowPos(Value: TShadowPosition);
begin
  if Value <> FShadowPos then begin
    FShadowPos := Value;
    Invalidate;
  end;
end;

function TMxCustomLabel.GetTransparent: Boolean;
begin
  Result := not (csOpaque in ControlStyle);
end;

procedure TMxCustomLabel.SetFocusControl(Value: TWinControl);
begin
  FFocusControl := Value;
{$IFDEF WIN32}
  if Value <> nil then Value.FreeNotification(Self);
{$ENDIF}
  if FShowFocus then Invalidate;
end;

procedure TMxCustomLabel.SetShowAccelChar(Value: Boolean);
begin
  if FShowAccelChar <> Value then begin
    FShowAccelChar := Value;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetTransparent(Value: Boolean);
begin
  if Transparent <> Value then begin
    if Value then ControlStyle := ControlStyle - [csOpaque]
    else ControlStyle := ControlStyle + [csOpaque];
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetShowFocus(Value: Boolean);
begin
  if FShowFocus <> Value then begin
    FShowFocus := Value;
    Invalidate;
  end;
end;

procedure TMxCustomLabel.SetWordWrap(Value: Boolean);
begin
  if FWordWrap <> Value then begin
    FWordWrap := Value;
    AdjustBounds;
  end;
end;

procedure TMxCustomLabel.Notification(AComponent: TComponent;
  Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (Operation = opRemove) and (AComponent = FFocusControl) then
    FocusControl := nil;
end;

procedure TMxCustomLabel.MouseDown(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  inherited MouseDown(Button, Shift, X, Y);
  if (Button = mbLeft) and Enabled then begin
    FDragging := True;
  end;
end;

procedure TMxCustomLabel.MouseUp(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  inherited MouseUp(Button, Shift, X, Y);
  if FDragging and (Button = mbLeft) then FDragging := False;
  UpdateTracking;
end;

procedure TMxCustomLabel.MouseEnter;
begin
  if Assigned(FOnMouseEnter) then FOnMouseEnter(Self);
end;

procedure TMxCustomLabel.MouseLeave;
begin
  if Assigned(FOnMouseLeave) then FOnMouseLeave(Self);
end;

procedure TMxCustomLabel.UpdateTracking;
var
  P: TPoint;
  OldValue: Boolean;
begin
  OldValue := FMouseInControl;
  GetCursorPos(P);
  FMouseInControl := Enabled and (FindDragTarget(P, True) = Self) and
    IsForegroundTask;
  if (FMouseInControl <> OldValue) then
    if FMouseInControl then MouseEnter else MouseLeave;
end;

procedure TMxCustomLabel.CMFocusChanged(var Message: TCMFocusChanged);
var
  Active: Boolean;
begin
  Active := Assigned(FFocusControl) and (Message.Sender = FFocusControl);
  if FFocused <> Active then begin
    FFocused := Active;
    if FShowFocus then Invalidate;
  end;
  inherited;
end;

procedure TMxCustomLabel.CMTextChanged(var Message: TMessage);
begin
  Invalidate;
  AdjustBounds;
end;

procedure TMxCustomLabel.CMFontChanged(var Message: TMessage);
begin
  inherited;
  AdjustBounds;
end;

procedure TMxCustomLabel.CMDialogChar(var Message: TCMDialogChar);
begin
  if (FFocusControl <> nil) and Enabled and ShowAccelChar and
    IsAccel(Message.CharCode, GetLabelCaption) then
    with FFocusControl do
      if CanFocus then begin
        SetFocus;
        Message.Result := 1;
      end;
end;

procedure TMxCustomLabel.WMRButtonDown(var Message: TWMRButtonDown);
begin
  inherited;
  UpdateTracking;
end;

procedure TMxCustomLabel.WMRButtonUp(var Message: TWMRButtonUp);
begin
  inherited;
  UpdateTracking;
end;

procedure TMxCustomLabel.CMEnabledChanged(var Message: TMessage);
begin
  inherited;
  UpdateTracking;
end;

procedure TMxCustomLabel.CMVisibleChanged(var Message: TMessage);
begin
  inherited;
  if Visible then UpdateTracking;
end;

procedure TMxCustomLabel.CMMouseEnter(var Message: TMessage);
begin
  inherited;
  if not FMouseInControl and Enabled and IsForegroundTask then begin
    FMouseInControl := True;
    MouseEnter;
  end;
end;

procedure TMxCustomLabel.CMMouseLeave(var Message: TMessage);
begin
  inherited;
  if FMouseInControl and Enabled and not FDragging then begin
    FMouseInControl := False;
    MouseLeave;
  end;
end;

{ TGlyphList }

type
  TGlyphList = class(TImageList)
  private
    FUsed: TBits;
    FCount: Integer;
    function AllocateIndex: Integer;
  public
    constructor CreateSize(AWidth, AHeight: Integer);
    destructor Destroy; override;
    function Add(Image, Mask: TBitmap): Integer;
    function AddMasked(Image: TBitmap; MaskColor: TColor): Integer;
{$IFDEF WIN32}
{$IFNDEF RX_D3} { Delphi 2.0 bug fix }
    procedure ReplaceMasked(Index: Integer; NewImage: TBitmap; MaskColor: TColor);
{$ENDIF}
{$ENDIF}
    procedure Delete(Index: Integer);
    property Count: Integer read FCount;
  end;

{ TGlyphCache }

  TGlyphCache = class
  private
    FGlyphLists: TList;
  public
    constructor Create;
    destructor Destroy; override;
    function GetList(AWidth, AHeight: Integer): TGlyphList;
    procedure ReturnList(List: TGlyphList);
    function Empty: Boolean;
  end;

{ TGlyphList }

constructor TGlyphList.CreateSize(AWidth, AHeight: Integer);
begin
{$IFDEF WIN32}
  inherited CreateSize(AWidth, AHeight);
{$ELSE}
  inherited Create(AWidth, AHeight);
{$ENDIF}
  FUsed := TBits.Create;
end;

destructor TGlyphList.Destroy;
begin
  FUsed.Free;
  inherited Destroy;
end;

function TGlyphList.AllocateIndex: Integer;
begin
  Result := FUsed.OpenBit;
  if Result >= FUsed.Size then begin
    Result := inherited Add(nil, nil);
    FUsed.Size := Result + 1;
  end;
  FUsed[Result] := True;
end;

{$IFDEF WIN32}
{$IFNDEF RX_D3} { Delphi 2.0 bug fix }
procedure TGlyphList.ReplaceMasked(Index: Integer; NewImage: TBitmap; MaskColor: TColor);
var
  TempIndex: Integer;
  Image, Mask: TBitmap;
begin
  if HandleAllocated then begin
    TempIndex := inherited AddMasked(NewImage, MaskColor);
    if TempIndex <> -1 then
    try
      Image := TBitmap.Create;
      Mask := TBitmap.Create;
      try
        with Image do begin
          Height := Self.Height;
          Width := Self.Width;
        end;
        with Mask do begin
          Monochrome := True; { fix }
          Height := Self.Height;
          Width := Self.Width;
        end;
        ImageList_Draw(Handle, TempIndex, Image.Canvas.Handle, 0, 0, ILD_NORMAL);
        ImageList_Draw(Handle, TempIndex, Mask.Canvas.Handle, 0, 0, ILD_MASK);
        if not ImageList_Replace(Handle, Index, Image.Handle, Mask.Handle) then
          raise EInvalidOperation.Create(LoadStr(SReplaceImage));
      finally
        Image.Free;
        Mask.Free;
      end;
    finally
      inherited Delete(TempIndex);
    end
    else raise EInvalidOperation.Create(LoadStr(SReplaceImage));
  end;
  Change;
end;
{$ENDIF}
{$ENDIF}

function TGlyphList.Add(Image, Mask: TBitmap): Integer;
begin
  Result := AllocateIndex;
  Replace(Result, Image, Mask);
  Inc(FCount);
end;

function TGlyphList.AddMasked(Image: TBitmap; MaskColor: TColor): Integer;
begin
  Result := AllocateIndex;
  ReplaceMasked(Result, Image, MaskColor);
  Inc(FCount);
end;

procedure TGlyphList.Delete(Index: Integer);
begin
  if FUsed[Index] then begin
    Dec(FCount);
    FUsed[Index] := False;
  end;
end;

{ TGlyphCache }

constructor TGlyphCache.Create;
begin
  inherited Create;
  FGlyphLists := TList.Create;
end;

destructor TGlyphCache.Destroy;
begin
  FGlyphLists.Free;
  inherited Destroy;
end;

function TGlyphCache.GetList(AWidth, AHeight: Integer): TGlyphList;
var
  I: Integer;
begin
  for I := FGlyphLists.Count - 1 downto 0 do begin
    Result := FGlyphLists[I];
    with Result do
      if (AWidth = Width) and (AHeight = Height) then Exit;
  end;
  Result := TGlyphList.CreateSize(AWidth, AHeight);
  FGlyphLists.Add(Result);
end;

procedure TGlyphCache.ReturnList(List: TGlyphList);
begin
  if List = nil then Exit;
  if List.Count = 0 then begin
    FGlyphLists.Remove(List);
    List.Free;
  end;
end;

function TGlyphCache.Empty: Boolean;
begin
  Result := FGlyphLists.Count = 0;
end;

const
  GlyphCache: TGlyphCache = nil;

{ TMxButtonGlyph }

constructor TMxButtonGlyph.Create;
var
  I: TMxButtonState;
begin
  inherited Create;
  FOriginal := TBitmap.Create;
  FOriginal.OnChange := GlyphChanged;
  FTransparentColor := clFuchsia;
  FAlignment := taCenter;
  FNumGlyphs := 1;
  for I := Low(I) to High(I) do FIndexs[I] := -1;
  if GlyphCache = nil then GlyphCache := TGlyphCache.Create;
end;

destructor TMxButtonGlyph.Destroy;
begin
  FOriginal.Free;
  Invalidate;
  if Assigned(GlyphCache) and GlyphCache.Empty then begin
    GlyphCache.Free;
    GlyphCache := nil;
  end;
  inherited Destroy;
end;

procedure TMxButtonGlyph.Invalidate;
var
  I: TMxButtonState;
begin
  for I := Low(I) to High(I) do begin
    if Assigned(FGlyphList) then
      if (FIndexs[I] <> -1) then TGlyphList(FGlyphList).Delete(FIndexs[I]);
    FIndexs[I] := -1;
  end;
  GlyphCache.ReturnList(TGlyphList(FGlyphList));
  FGlyphList := nil;
end;

procedure TMxButtonGlyph.GlyphChanged(Sender: TObject);
var
  Glyphs: Integer;
begin
  if Sender = FOriginal then begin
    Invalidate;
    if (FOriginal <> nil) and (FOriginal.Height > 0) then begin
      FTransparentColor := FOriginal.TransparentColor and not PaletteMask;
      if FOriginal.Width mod FOriginal.Height = 0 then begin
        Glyphs := FOriginal.Width div FOriginal.Height;
        if Glyphs > (Ord(High(TMxButtonState)) + 1) then Glyphs := 1;
        SetNumGlyphs(Glyphs);
      end;
    end;
    if Assigned(FOnChange) then FOnChange(Self);
  end;
end;

procedure TMxButtonGlyph.SetGlyph(Value: TBitmap);
begin
  Invalidate;
  FOriginal.Assign(Value);
end;

procedure TMxButtonGlyph.SetNumGlyphs(Value: TMxNumGlyphs);
begin
  if (Value <> FNumGlyphs) and (Value > 0) then begin
    Invalidate;
    FNumGlyphs := Value;
  end;
end;

function TMxButtonGlyph.MapColor(Color: TColor): TColor;
var
  Index: Byte;
begin
  if (Color = FTransparentColor) or (ColorToRGB(Color) =
    ColorToRGB(clBtnFace)) then Result := Color
  else begin
    Color := ColorToRGB(Color);
    Index := Byte(Longint(Word(GetRValue(Color)) * 77 +
      Word(GetGValue(Color)) * 150 + Word(GetBValue(Color)) * 29) shr 8);
    Result := RGB(Index, Index, Index);
  end;
end;

{$IFDEF WIN32}
function TMxButtonGlyph.CreateImageGlyph(State: TMxButtonState;
  Images: TImageList; Index: Integer): Integer;
var
  TmpImage, Mask: TBitmap;
  IWidth, IHeight, X, Y: Integer;
begin
  if (State = rbsDown) then State := rbsUp;
  Result := FIndexs[State];
  if (Result <> -1) or (Images.Width = 0) or (Images.Height = 0) or
    (Images.Count = 0) then Exit;
  IWidth := Images.Width;
  IHeight := Images.Height;
  if FGlyphList = nil then begin
    if GlyphCache = nil then GlyphCache := TGlyphCache.Create;
    FGlyphList := GlyphCache.GetList(IWidth, IHeight);
  end;
  TmpImage := TBitmap.Create;
  try
    TmpImage.Width := IWidth;
    TmpImage.Height := IHeight;
    case State of
      rbsUp, rbsDown, rbsExclusive:
        begin
          with TmpImage.Canvas do begin
            FillRect(Rect(0, 0, IWidth, IHeight));
            ImageList_Draw(Images.Handle, Index, Handle, 0, 0, ILD_NORMAL);
          end;
          Mask := TBitmap.Create;
          try
            with Mask do begin
              Monochrome := True;
              Height := IHeight;
              Width := IWidth;
            end;
            with Mask.Canvas do begin
              FillRect(Rect(0, 0, IWidth, IHeight));
              ImageList_Draw(Images.Handle, Index, Handle, 0, 0, ILD_MASK);
            end;
            FIndexs[State] := TGlyphList(FGlyphList).Add(TmpImage, Mask);
          finally
            Mask.Free;
          end;
        end;
      rbsDisabled:
        begin
          TmpImage.Canvas.Brush.Color := clBtnFace;
          TmpImage.Canvas.FillRect(Rect(0, 0, IWidth, IHeight));
          ImageListDrawDisabled(Images, TmpImage.Canvas, 0, 0, Index,
            clBtnHighlight, clBtnShadow, True);
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage,
            ColorToRGB(clBtnFace));
        end;
      rbsInactive:
        begin
          TmpImage.Canvas.Brush.Color := clBtnFace;
          TmpImage.Canvas.FillRect(Rect(0, 0, IWidth, IHeight));
          ImageList_Draw(Images.Handle, Index, TmpImage.Canvas.Handle, 0, 0,
            ILD_NORMAL);
          with TmpImage do begin
            for X := 0 to Width - 1 do
              for Y := 0 to Height - 1 do
                Canvas.Pixels[X, Y] := MapColor(Canvas.Pixels[X, Y]);
          end;
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage,
            ColorToRGB(clBtnFace));
        end;
    end;
  finally
    TmpImage.Free;
  end;
  Result := FIndexs[State];
end;
{$ENDIF}

function TMxButtonGlyph.CreateButtonGlyph(State: TMxButtonState): Integer;
var
  TmpImage, MonoBmp: TBitmap;
  IWidth, IHeight, X, Y: Integer;
  IRect, ORect: TRect;
  I: TMxButtonState;
begin
  if (State = rbsDown) and (NumGlyphs < 3) then State := rbsUp;
  Result := FIndexs[State];
  if (Result <> -1) or (FOriginal.Width = 0) or (FOriginal.Height = 0) or
    FOriginal.Empty then Exit;
  IWidth := FOriginal.Width div FNumGlyphs;
  IHeight := FOriginal.Height;
  if FGlyphList = nil then begin
    if GlyphCache = nil then GlyphCache := TGlyphCache.Create;
    FGlyphList := GlyphCache.GetList(IWidth, IHeight);
  end;
  TmpImage := TBitmap.Create;
  try
    TmpImage.Width := IWidth;
    TmpImage.Height := IHeight;
    IRect := Rect(0, 0, IWidth, IHeight);
    TmpImage.Canvas.Brush.Color := clBtnFace;
    I := State;
    if Ord(I) >= NumGlyphs then I := rbsUp;
    ORect := Rect(Ord(I) * IWidth, 0, (Ord(I) + 1) * IWidth, IHeight);
    case State of
      rbsUp, rbsDown, rbsExclusive:
        begin
          TmpImage.Canvas.CopyRect(IRect, FOriginal.Canvas, ORect);
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage, FTransparentColor);
        end;
      rbsDisabled:
        if NumGlyphs > 1 then begin
          TmpImage.Canvas.CopyRect(IRect, FOriginal.Canvas, ORect);
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage, FTransparentColor);
        end
        else begin
          MonoBmp := CreateDisabledBitmap(FOriginal, clBlack);
          try
            FIndexs[State] := TGlyphList(FGlyphList).AddMasked(MonoBmp,
              ColorToRGB(clBtnFace));
          finally
            MonoBmp.Free;
          end;
        end;
      rbsInactive:
        if NumGlyphs > 4 then begin
          TmpImage.Canvas.CopyRect(IRect, FOriginal.Canvas, ORect);
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage, FTransparentColor);
        end
        else begin
          with TmpImage do begin
            for X := 0 to Width - 1 do
              for Y := 0 to Height - 1 do
                Canvas.Pixels[X, Y] := MapColor(FOriginal.Canvas.Pixels[X, Y]);
          end;
          FIndexs[State] := TGlyphList(FGlyphList).AddMasked(TmpImage, FTransparentColor);
        end;
    end;
  finally
    TmpImage.Free;
  end;
  Result := FIndexs[State];
  FOriginal.Dormant;
end;

procedure TMxButtonGlyph.DrawPopupMark(Canvas: TCanvas; X, Y: Integer;
  State: TMxButtonState);
var
  AColor: TColor;

  procedure DrawMark;
  var
    I: Integer;
  begin
    with Canvas do begin
      for I := 0 to 6 do begin
        Pixels[X + I, Y - 1] := AColor;
        if (I > 0) and (I < 6) then begin
          Pixels[X + I, Y] := AColor;
          if (I > 1) and (I < 5) then Pixels[X + I, Y + 1] := AColor;
        end;
      end;
      Pixels[X + 3, Y + 2] := AColor;
    end;
  end;

begin
  if State = rbsDisabled then begin
    AColor := clBtnHighlight;
    Inc(X, 1); Inc(Y, 1);
    DrawMark;
    Dec(X, 1); Dec(Y, 1);
    AColor := clBtnShadow;
  end
  else AColor := clBtnText;
  DrawMark;
end;

function TMxButtonGlyph.DrawButtonGlyph(Canvas: TCanvas; X, Y: Integer;
  State: TMxButtonState): TPoint;
var
  Index: Integer;
begin
  Result := Point(0, 0);
  if (FOriginal = nil) or (FOriginal.Width = 0) or (FOriginal.Height = 0) or
    FOriginal.Empty then Exit;
  Index := CreateButtonGlyph(State);
  if Index >= 0 then begin
{$IFDEF WIN32}
    ImageList_Draw(FGlyphList.Handle, Index, Canvas.Handle, X, Y, ILD_NORMAL);
{$ELSE}
    FGlyphList.Draw(Canvas, X, Y, Index);
{$ENDIF}
    Result := Point(FGlyphList.Width, FGlyphList.Height);
  end;
end;

{$IFDEF WIN32}
function TMxButtonGlyph.DrawButtonImage(Canvas: TCanvas; X, Y: Integer;
  Images: TImageList; ImageIndex: Integer; State: TMxButtonState): TPoint;
var
  Index: Integer;
begin
  Result := Point(0, 0);
  if (Images = nil) or (ImageIndex < 0) or (ImageIndex >= Images.Count) then
    Exit;
  if State = rbsDisabled then begin
    ImageListDrawDisabled(Images, Canvas, X, Y, ImageIndex, clBtnHighlight,
      clBtnShadow, True);
  end
  else if State = rbsInactive then begin
    Index := CreateImageGlyph(State, Images, ImageIndex);
    if Index >= 0 then
      ImageList_Draw(FGlyphList.Handle, Index, Canvas.Handle, X, Y, ILD_NORMAL);
  end
  else
    ImageList_Draw(Images.Handle, ImageIndex, Canvas.Handle, X, Y, ILD_NORMAL);
  Result := Point(Images.Width, Images.Height);
end;
{$ENDIF}

procedure TMxButtonGlyph.MinimizeCaption(Canvas: TCanvas; const Caption: string;
  Buffer: PChar; MaxLen, Width: Integer);
var
  I: Integer;
{$IFNDEF WIN32}
  P: PChar;
{$ENDIF}
  Lines: TStrings;
begin
  StrPLCopy(Buffer, Caption, MaxLen);
  if FWordWrap then Exit;
  Lines := TStringList.Create;
  try
{$IFDEF WIN32}
    Lines.Text := Caption;
    for I := 0 to Lines.Count - 1 do
      Lines[I] := MinimizeText(Lines[I], Canvas, Width);
    StrPLCopy(Buffer, TrimRight(Lines.Text), MaxLen);
{$ELSE}
    Lines.SetText(Buffer);
    for I := 0 to Lines.Count - 1 do
      Lines[I] := MinimizeText(Lines[I], Canvas, Width);
    P := Lines.GetText;
    try
      StrPLCopy(Buffer, TrimRight(StrPas(P)), MaxLen);
    finally
      StrDispose(P);
    end;
{$ENDIF}
  finally
    Lines.Free;
  end;
end;

procedure TMxButtonGlyph.DrawButtonText(Canvas: TCanvas; const Caption: string;
  TextBounds: TRect; State: TMxButtonState; Flags: Word);
var
  CString: array[0..255] of Char;
begin
  Canvas.Brush.Style := bsClear;
  StrPLCopy(CString, Caption, SizeOf(CString) - 1);
  Flags := DT_VCENTER or WordWraps[FWordWrap] or Flags;
  if State = rbsDisabled then begin
    with Canvas do begin
      OffsetRect(TextBounds, 1, 1);
      Font.Color := clBtnHighlight;
      DrawText(Handle, CString, Length(Caption), TextBounds, Flags);
      OffsetRect(TextBounds, -1, -1);
      Font.Color := clBtnShadow;
      DrawText(Handle, CString, Length(Caption), TextBounds, Flags);
    end;
  end
  else DrawText(Canvas.Handle, CString, -1, TextBounds, Flags);
end;

procedure TMxButtonGlyph.CalcButtonLayout(Canvas: TCanvas; const Client: TRect;
  var Caption: string; Layout: TButtonLayout; Margin, Spacing: Integer;
  PopupMark: Boolean; var GlyphPos: TPoint; var TextBounds: TRect; Flags: Word
  {$IFDEF WIN32}; Images: TImageList; ImageIndex: Integer {$ENDIF});
var
  TextPos: TPoint;
  MaxSize, ClientSize, GlyphSize, TextSize: TPoint;
  TotalSize: TPoint;
  CString: array[0..255] of Char;
begin
  { calculate the item sizes }
  ClientSize := Point(Client.Right - Client.Left, Client.Bottom - Client.Top);
{$IFDEF WIN32}
  if Assigned(Images) and (Images.Width > 0) and (ImageIndex >= 0) and
    (ImageIndex < Images.Count) then
    GlyphSize := Point(Images.Width, Images.Height)
  else
{$ENDIF}
  if FOriginal <> nil then
    GlyphSize := Point(FOriginal.Width div FNumGlyphs, FOriginal.Height)
  else GlyphSize := Point(0, 0);
  if Layout in [blGlyphLeft, blGlyphRight] then begin
    MaxSize.X := ClientSize.X - GlyphSize.X;
    if Margin <> -1 then Dec(MaxSize.X, Margin);
    if Spacing <> -1 then Dec(MaxSize.X, Spacing);
    if PopupMark then Dec(MaxSize.X, 9);
    MaxSize.Y := ClientSize.Y;
  end
  else { blGlyphTop, blGlyphBottom } begin
    MaxSize.X := ClientSize.X;
    MaxSize.Y := ClientSize.Y - GlyphSize.Y;
    if Margin <> -1 then Dec(MaxSize.Y, Margin);
    if Spacing <> -1 then Dec(MaxSize.Y, Spacing);
  end;
  MaxSize.X := Max(0, MaxSize.X);
  MaxSize.Y := Max(0, MaxSize.Y);
  MinimizeCaption(Canvas, Caption, CString, SizeOf(CString) - 1, MaxSize.X);
  Caption := StrPas(CString);
  if Length(Caption) > 0 then begin
    TextBounds := Rect(0, 0, MaxSize.X, 0);
    DrawText(Canvas.Handle, CString, -1, TextBounds, DT_CALCRECT or DT_CENTER
      or DT_VCENTER or WordWraps[FWordWrap] or Flags);
  end
  else TextBounds := Rect(0, 0, 0, 0);
  TextBounds.Bottom := Max(TextBounds.Top, TextBounds.Top +
    Min(MaxSize.Y, HeightOf(TextBounds)));
  TextBounds.Right := Max(TextBounds.Left, TextBounds.Left +
    Min(MaxSize.X, WidthOf(TextBounds)));
  TextSize := Point(TextBounds.Right - TextBounds.Left, TextBounds.Bottom -
    TextBounds.Top);
  if PopupMark then
    if ((GlyphSize.X = 0) or (GlyphSize.Y = 0)) or (Layout = blGlyphLeft) then
      Inc(TextSize.X, 9)
    else if (GlyphSize.X > 0) then
      Inc(GlyphSize.X, 6);
  { If the layout has the glyph on the right or the left, then both the
    text and the glyph are centered vertically.  If the glyph is on the top
    or the bottom, then both the text and the glyph are centered horizontally.}
  if Layout in [blGlyphLeft, blGlyphRight] then begin
    GlyphPos.Y := (ClientSize.Y div 2) - (GlyphSize.Y div 2);
    TextPos.Y := (ClientSize.Y div 2) - (TextSize.Y div 2);
  end
  else begin
    GlyphPos.X := (ClientSize.X div 2) - (GlyphSize.X div 2);
    TextPos.X := (ClientSize.X div 2) - (TextSize.X div 2);
  end;
  { if there is no text or no bitmap, then Spacing is irrelevant }
  if (TextSize.X = 0) or (GlyphSize.X = 0) then Spacing := 0;
  { adjust Margin and Spacing }
  if Margin = -1 then begin
    if Spacing = -1 then begin
      TotalSize := Point(GlyphSize.X + TextSize.X, GlyphSize.Y + TextSize.Y);
      if Layout in [blGlyphLeft, blGlyphRight] then
        Margin := (ClientSize.X - TotalSize.X) div 3
      else Margin := (ClientSize.Y - TotalSize.Y) div 3;
      Spacing := Margin;
    end
    else begin
      TotalSize := Point(GlyphSize.X + Spacing + TextSize.X, GlyphSize.Y +
        Spacing + TextSize.Y);
      if Layout in [blGlyphLeft, blGlyphRight] then
        Margin := (ClientSize.X div 2) - (TotalSize.X div 2)
      else Margin := (ClientSize.Y div 2) - (TotalSize.Y div 2);
    end;
  end
  else begin
    if Spacing = -1 then begin
      TotalSize := Point(ClientSize.X - (Margin + GlyphSize.X), ClientSize.Y -
        (Margin + GlyphSize.Y));
      if Layout in [blGlyphLeft, blGlyphRight] then
        Spacing := (TotalSize.X div 2) - (TextSize.X div 2)
      else Spacing := (TotalSize.Y div 2) - (TextSize.Y div 2);
    end;
  end;
  case Layout of
    blGlyphLeft:
      begin
        GlyphPos.X := Margin;
        TextPos.X := GlyphPos.X + GlyphSize.X + Spacing;
      end;
    blGlyphRight:
      begin
        GlyphPos.X := ClientSize.X - Margin - GlyphSize.X;
        TextPos.X := GlyphPos.X - Spacing - TextSize.X;
      end;
    blGlyphTop:
      begin
        GlyphPos.Y := Margin;
        TextPos.Y := GlyphPos.Y + GlyphSize.Y + Spacing;
      end;
    blGlyphBottom:
      begin
        GlyphPos.Y := ClientSize.Y - Margin - GlyphSize.Y;
        TextPos.Y := GlyphPos.Y - Spacing - TextSize.Y;
      end;
  end;
  { fixup the result variables }
  Inc(GlyphPos.X, Client.Left);
  Inc(GlyphPos.Y, Client.Top);
  OffsetRect(TextBounds, TextPos.X + Client.Left, TextPos.Y + Client.Top);
end;

{$IFDEF WIN32}
function TMxButtonGlyph.Draw(Canvas: TCanvas; const Client: TRect;
  const Caption: string; Layout: TButtonLayout; Margin, Spacing: Integer;
  PopupMark: Boolean; State: TMxButtonState; Flags: Word): TRect;
begin
  Result := DrawEx(Canvas, Client, Caption, Layout, Margin, Spacing,
    PopupMark, nil, -1, State, Flags);
end;
{$ENDIF}

{$IFDEF WIN32}
function TMxButtonGlyph.DrawEx(Canvas: TCanvas; const Client: TRect;
  const Caption: string; Layout: TButtonLayout; Margin, Spacing: Integer;
  PopupMark: Boolean; Images: TImageList; ImageIndex: Integer;
  State: TMxButtonState; Flags: Word): TRect;
{$ELSE}
function TMxButtonGlyph.Draw(Canvas: TCanvas; const Client: TRect;
  const Caption: string; Layout: TButtonLayout; Margin, Spacing: Integer;
  PopupMark: Boolean; State: TMxButtonState; Flags: Word): TRect;
{$ENDIF}
var
{$IFDEF WIN32}
  UseImages: Boolean;
{$ENDIF}
  GlyphPos, PopupPos: TPoint;
  TextBounds: TRect;
  CaptionText: string;
begin
  CaptionText := Caption;
  CalcButtonLayout(Canvas, Client, CaptionText, Layout, Margin, Spacing,
    PopupMark, GlyphPos, TextBounds, Flags {$IFDEF WIN32}, Images,
    ImageIndex {$ENDIF});
{$IFDEF WIN32}
  UseImages := False;
  if Assigned(Images) and (ImageIndex >= 0) and (ImageIndex < Images.Count) and
    (Images.Width > 0) then
  begin
    UseImages := True;
    PopupPos := DrawButtonImage(Canvas, GlyphPos.X, GlyphPos.Y, Images,
      ImageIndex, State);
  end else
{$ENDIF}
  PopupPos := DrawButtonGlyph(Canvas, GlyphPos.X, GlyphPos.Y, State);
  DrawButtonText(Canvas, CaptionText, TextBounds, State, Flags);
  if PopupMark then
    if (Layout <> blGlyphLeft) and (((FOriginal <> nil) and
      (FOriginal.Width > 0)) {$IFDEF WIN32} or UseImages {$ENDIF}) then
    begin
      PopupPos.X := GlyphPos.X + PopupPos.X + 1;
      PopupPos.Y := GlyphPos.Y + PopupPos.Y div 2;
      DrawPopupMark(Canvas, PopupPos.X, PopupPos.Y, State);
    end
    else begin
      if CaptionText <> '' then
        PopupPos.X := TextBounds.Right + 3
      else
        PopupPos.X := (Client.Left + Client.Right - 7) div 2;
      PopupPos.Y := TextBounds.Top  + HeightOf(TextBounds) div 2;
      DrawPopupMark(Canvas, PopupPos.X, PopupPos.Y, State);
    end;
  Result := TextBounds;
end;

const
{$IFNDEF RX_D4}
  Pattern: TBitmap = nil;
{$ENDIF}
  ButtonCount: Integer = 0;

{ DrawButtonFrame - returns the remaining usable area inside the Client rect }

function DrawButtonFrame(Canvas: TCanvas; const Client: TRect;
  IsDown, IsFlat: Boolean; Style: TButtonStyle): TRect;
var
  NewStyle: Boolean;
begin
  Result := Client;
  NewStyle := (Style = bsNew) or (NewStyleControls and (Style = bsAutoDetect));
  if IsDown then begin
    if NewStyle then begin
      Frame3D(Canvas, Result, clWindowFrame, clBtnHighlight, 1);
      if not IsFlat then
        Frame3D(Canvas, Result, clBtnShadow, clBtnFace, 1);
    end
    else begin
      if IsFlat then
        Frame3D(Canvas, Result, clWindowFrame, clBtnHighlight, 1)
      else begin
        Frame3D(Canvas, Result, clWindowFrame, clWindowFrame, 1);
        Canvas.Pen.Color := clBtnShadow;
        Canvas.PolyLine([Point(Result.Left, Result.Bottom - 1),
          Point(Result.Left, Result.Top), Point(Result.Right, Result.Top)]);
      end;
    end;
  end
  else begin
    if NewStyle then begin
      if IsFlat then 
        Frame3D(Canvas, Result, clBtnHighlight, clBtnShadow, 1)
      else begin
        Frame3D(Canvas, Result, clBtnHighlight, clWindowFrame, 1);
        Frame3D(Canvas, Result, clBtnFace, clBtnShadow, 1);
      end; 
    end
    else begin
      if IsFlat then
        Frame3D(Canvas, Result, clBtnHighlight, clWindowFrame, 1)
      else begin
        Frame3D(Canvas, Result, clWindowFrame, clWindowFrame, 1);
        Frame3D(Canvas, Result, clBtnHighlight, clBtnShadow, 1);
      end;
    end;
  end;
  InflateRect(Result, -1, -1);
end;

{ TButtonImage }

constructor TButtonImage.Create;
begin
  FGlyph := TMxButtonGlyph.Create;
  NumGlyphs := 1;
  FButtonSize := Point(24, 23);
end;

destructor TButtonImage.Destroy;
begin
  FGlyph.Free;
  inherited Destroy;
end;

procedure TButtonImage.Invalidate;
begin
  TMxButtonGlyph(FGlyph).Invalidate;
end;

function TButtonImage.GetNumGlyphs: TMxNumGlyphs;
begin
  Result := TMxButtonGlyph(FGlyph).NumGlyphs;
end;

procedure TButtonImage.SetNumGlyphs(Value: TMxNumGlyphs);
begin
  TMxButtonGlyph(FGlyph).NumGlyphs := Value;
end;

function TButtonImage.GetWordWrap: Boolean;
begin
  Result := TMxButtonGlyph(FGlyph).WordWrap;
end;

procedure TButtonImage.SetWordWrap(Value: Boolean);
begin
  TMxButtonGlyph(FGlyph).WordWrap := Value;
end;

function TButtonImage.GetGlyph: TBitmap;
begin
  Result := TMxButtonGlyph(FGlyph).Glyph;
end;

procedure TButtonImage.SetGlyph(Value: TBitmap);
begin
  TMxButtonGlyph(FGlyph).Glyph := Value;
end;

function TButtonImage.GetAlignment: TAlignment;
begin
  Result := TMxButtonGlyph(FGlyph).Alignment;
end;

procedure TButtonImage.SetAlignment(Value: TAlignment);
begin
  TMxButtonGlyph(FGlyph).Alignment := Value;
end;

{$IFDEF WIN32}
procedure TButtonImage.Draw(Canvas: TCanvas; X, Y, Margin, Spacing: Integer;
  Layout: TButtonLayout; AFont: TFont; Flags: Word);
begin
  DrawEx(Canvas, X, Y, Margin, Spacing, Layout, AFont, nil, -1, Flags);
end;
{$ENDIF}

{$IFDEF WIN32}
procedure TButtonImage.DrawEx(Canvas: TCanvas; X, Y, Margin, Spacing: Integer;
  Layout: TButtonLayout; AFont: TFont; Images: TImageList; ImageIndex: Integer;
  Flags: Word);
{$ELSE}
procedure TButtonImage.Draw(Canvas: TCanvas; X, Y, Margin, Spacing: Integer;
  Layout: TButtonLayout; AFont: TFont; Flags: Word);
{$ENDIF}
var
  Target: TRect;
  SaveColor: Integer;
  SaveFont: TFont;
begin
  SaveColor := Canvas.Brush.Color;
  SaveFont := TFont.Create;
  SaveFont.Assign(Canvas.Font);
  try
    Target := Bounds(X, Y, FButtonSize.X, FButtonSize.Y);
    Canvas.Brush.Color := clBtnFace;
    Canvas.FillRect(Target);
    Frame3D(Canvas, Target, clBtnShadow, clWindowFrame, 1);
    Frame3D(Canvas, Target, clBtnHighlight, clBtnShadow, 1);
    if AFont <> nil then Canvas.Font := AFont;
{$IFDEF WIN32}
    TMxButtonGlyph(FGlyph).DrawEx(Canvas, Target, Caption, Layout, Margin,
      Spacing, False, Images, ImageIndex, rbsUp, Flags);
{$ELSE}
    TMxButtonGlyph(FGlyph).Draw(Canvas, Target, Caption, Layout, Margin,
      Spacing, False, rbsUp, Flags);
{$ENDIF}
  finally
    Canvas.Font.Assign(SaveFont);
    SaveFont.Free;
    Canvas.Brush.Color := SaveColor;
  end;
end;

{ TMxPanel }
procedure TMxPanel.Paint;
begin
  inherited Paint;
  if Assigned(FOnPaint) then FOnPaint(Self)
end;

{ TMxSpeedButton }
constructor TMxSpeedButton.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  SetBounds(0, 0, 25, 25);
  ControlStyle := [csCaptureMouse, csOpaque, csDoubleClicks];
{$IFDEF WIN32}
  ControlStyle := ControlStyle + [csReplicatable];
{$ENDIF}
  FInactiveGrayed := True;
  FDrawImage := TBitmap.Create;
  FGlyph := TMxButtonGlyph.Create;
  TMxButtonGlyph(FGlyph).OnChange := GlyphChanged;
  ParentFont := True;
  ParentShowHint := False;
  ShowHint := True;
  FSpacing := 1;
  FMargin := -1;
  FInitRepeatPause := 500;
  FRepeatPause := 100;
  FStyle := bsAutoDetect;
  FLayout := blGlyphTop;
  FMarkDropDown := True;
  Inc(ButtonCount);
end;

destructor TMxSpeedButton.Destroy;
begin
  TMxButtonGlyph(FGlyph).Free;
  Dec(ButtonCount);
{$IFNDEF RX_D4}
  if ButtonCount = 0 then begin
    Pattern.Free;
    Pattern := nil;
  end;
{$ENDIF}
  FDrawImage.Free;
  FDrawImage := nil;
  if FRepeatTimer <> nil then FRepeatTimer.Free;
  inherited Destroy;
end;

procedure TMxSpeedButton.Loaded;
var
  State: TMxButtonState;
begin
  inherited Loaded;
  if Enabled then begin
    if Flat then State := rbsInactive
    else State := rbsUp;
  end
  else State := rbsDisabled;
  TMxButtonGlyph(FGlyph).CreateButtonGlyph(State);
end;

procedure TMxSpeedButton.PaintGlyph(Canvas: TCanvas; ARect: TRect;
  AState: TMxButtonState; DrawMark: Boolean);
begin
  TMxButtonGlyph(FGlyph).Draw(Canvas, ARect, Caption, FLayout,
    FMargin, FSpacing, DrawMark, AState,
    {$IFDEF RX_D4} DrawTextBiDiModeFlags(Alignments[Alignment]) {$ELSE}
    Alignments[Alignment] {$ENDIF});
end;

procedure TMxSpeedButton.Paint;
var
  PaintRect: TRect;
  AState: TMxButtonState;
begin
  if not Enabled {and not (csDesigning in ComponentState)} then begin
    FState := rbsDisabled;
    FDragging := False;
  end
  else if FState = rbsDisabled then
    if FDown and (GroupIndex <> 0) then FState := rbsExclusive
    else FState := rbsUp;
  AState := FState;
  if FFlat and not FMouseInControl and not (csDesigning in ComponentState) then
    AState := rbsInactive;
  PaintRect := Rect(0, 0, Width, Height);
  FDrawImage.Width := Self.Width;
  FDrawImage.Height := Self.Height;
  with FDrawImage.Canvas do begin
    Font := Self.Font;
    Brush.Color := clBtnFace;
    Brush.Style := bsSolid;
    FillRect(PaintRect);
    if FTransparent then CopyParentImage(Self, FDrawImage.Canvas);
    if (AState <> rbsInactive) or (FState = rbsExclusive) then
      PaintRect := DrawButtonFrame(FDrawImage.Canvas, PaintRect,
        FState in [rbsDown, rbsExclusive], FFlat, FStyle)
    else if FFlat then
      InflateRect(PaintRect, -2, -2);
  end;
  if (FState = rbsExclusive) and not Transparent and
    (not FFlat or (AState = rbsInactive)) then
  begin
{$IFDEF RX_D4}
    FDrawImage.Canvas.Brush.Bitmap := AllocPatternBitmap(clBtnFace, clBtnHighlight);
{$ELSE}
    if Pattern = nil then
      Pattern := CreateTwoColorsBrushPattern(clBtnFace, clBtnHighlight);
    FDrawImage.Canvas.Brush.Bitmap := Pattern;
{$ENDIF}
    InflateRect(PaintRect, 1, 1);
    FDrawImage.Canvas.FillRect(PaintRect);
    InflateRect(PaintRect, -1, -1);
  end;
  if FState in [rbsDown, rbsExclusive] then OffsetRect(PaintRect, 1, 1);
  if (FState = rbsDisabled) or not FInactiveGrayed then AState := FState;
  PaintGlyph(FDrawImage.Canvas, PaintRect, AState, FMarkDropDown and
    Assigned(FDropDownMenu));
  Canvas.Draw(0, 0, FDrawImage);
end;

procedure TMxSpeedButton.Notification(AComponent: TComponent;
  Operation: TOperation);
begin
  inherited Notification(AComponent, Operation);
  if (AComponent = DropDownMenu) and (Operation = opRemove) then
    DropDownMenu := nil;
end;

function TMxSpeedButton.GetDropDownMenuPos: TPoint;
begin
  if Assigned(FDropDownMenu) then begin
    if MenuPosition = dmpBottom then begin
      case FDropDownMenu.Alignment of
        paLeft: Result := Point(-1, Height);
        paRight: Result := Point(Width + 1, Height);
        else {paCenter} Result := Point(Width div 2, Height);
      end;
    end
    else { dmpRight } begin
      case FDropDownMenu.Alignment of
        paLeft: Result := Point(Width, -1);
        paRight: Result := Point(-1, -1);
        else {paCenter} Result := Point(Width div 2, Height);
      end;
    end;
  end else Result := Point(0, 0);
end;

function TMxSpeedButton.CheckBtnMenuDropDown: Boolean;
begin
  Result := CheckMenuDropDown(
    {$IFDEF WIN32}PointToSmallPoint(GetDropDownMenuPos){$ELSE}
    GetDropDownMenuPos{$ENDIF}, True);
end;

function TMxSpeedButton.CheckMenuDropDown(const Pos: TSmallPoint;
  Manual: Boolean): Boolean;
var
  Form: TCustomForm;
begin
  Result := False;
  if csDesigning in ComponentState then Exit;
  if Assigned(FDropDownMenu) and (DropDownMenu.AutoPopup or Manual) then
  begin
    Form := GetParentForm(Self);
    if Form <> nil then Form.SendCancelMode(nil);
    DropDownMenu.PopupComponent := Self;
    with ClientToScreen(SmallPointToPoint(Pos)) do DropDownMenu.Popup(X, Y);
    Result := True;
  end;
end;

procedure TMxSpeedButton.MouseEnter;
begin
  if Assigned(FOnMouseEnter) then FOnMouseEnter(Self);
end;

procedure TMxSpeedButton.MouseLeave;
begin
  if Assigned(FOnMouseLeave) then FOnMouseLeave(Self);
end;

procedure TMxSpeedButton.MouseDown(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
var
  P: TPoint;
  Msg: TMsg;
begin
  if FMenuTracking then Exit;
  inherited MouseDown(Button, Shift, X, Y);
  if (not FMouseInControl) and Enabled then begin
    FMouseInControl := True;
    Repaint;
  end;
  if (Button = mbLeft) and Enabled {and not (ssDouble in Shift)} then begin
    if not FDown then begin
      FState := rbsDown;
      Repaint;
    end;
    FDragging := True;
    FMenuTracking := True;
    try
      P := GetDropDownMenuPos;
      if CheckMenuDropDown(PointToSmallPoint(P), False) then
        DoMouseUp(Button, Shift, X, Y);
      if PeekMessage(Msg, 0, 0, 0, PM_NOREMOVE) then begin
        if (Msg.Message = WM_LBUTTONDOWN) or (Msg.Message = WM_LBUTTONDBLCLK) then
        begin
          P := ScreenToClient(Msg.Pt);
          if (P.X >= 0) and (P.X < ClientWidth) and (P.Y >= 0)
            and (P.Y <= ClientHeight) then KillMessage(0, Msg.Message);
              {PeekMessage(Msg, 0, 0, 0, PM_REMOVE);}
        end;
      end;
    finally
      FMenuTracking := False;
    end;
    if FAllowTimer then begin
      if FRepeatTimer = nil then FRepeatTimer := TTimer.Create(Self);
      FRepeatTimer.Interval := InitPause;
      FRepeatTimer.OnTimer := TimerExpired;
      FRepeatTimer.Enabled  := True;
    end;
  end;
end;

procedure TMxSpeedButton.MouseMove(Shift: TShiftState; X, Y: Integer);
var
  NewState: TMxButtonState;
begin
  inherited MouseMove(Shift, X, Y);
  if FDragging then begin
    if not FDown then NewState := rbsUp
    else NewState := rbsExclusive;
    if (X >= 0) and (X < ClientWidth) and (Y >= 0) and (Y <= ClientHeight) then
      if FDown then NewState := rbsExclusive else NewState := rbsDown;
    if NewState <> FState then begin
      FState := NewState;
      Repaint;
    end;
  end;
end;

procedure TMxSpeedButton.MouseUp(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
begin
  inherited MouseUp(Button, Shift, X, Y);
  DoMouseUp(Button, Shift, X, Y);
  if FRepeatTimer <> nil then FRepeatTimer.Enabled  := False;
end;

procedure TMxSpeedButton.DoMouseUp(Button: TMouseButton; Shift: TShiftState;
  X, Y: Integer);
var
  DoClick: Boolean;
begin
  if FDragging and (Button = mbLeft) then begin
    FDragging := False;
    DoClick := (X >= 0) and (X < ClientWidth) and (Y >= 0) and (Y <= ClientHeight);
    if FGroupIndex = 0 then begin
      FState := rbsUp;
      FMouseInControl := False;
      if DoClick and not (FState in [rbsExclusive, rbsDown]) then
        Repaint
      else Invalidate;
    end
    else if DoClick then begin
      SetDown(not FDown);
      if FDown then Repaint;
    end
    else begin
      if FDown then FState := rbsExclusive;
      Repaint;
    end;
    if DoClick and not FMenuTracking then Click;
  end;
  UpdateTracking;
end;

procedure TMxSpeedButton.ButtonClick;
var
  FirstTickCount, Now: Longint;
begin
  if FMenuTracking or (not Enabled) or (Assigned(FDropDownMenu) and
    DropDownMenu.AutoPopup) then Exit;
  if not FDown then begin
    FState := rbsDown;
    Repaint;
  end;
  try
    FirstTickCount := GetTickCount;
    repeat
      Now := GetTickCount;
    until (Now - FirstTickCount >= 20) or (Now < FirstTickCount);
    if FGroupIndex = 0 then Click;
  finally
    FState := rbsUp;
    if FGroupIndex = 0 then Repaint
    else begin
      SetDown(not FDown);
      Click;
    end;
  end;
end;

procedure TMxSpeedButton.Click;
var
  Form: TCustomForm;
begin
  Form := GetParentForm(Self);
  if Form <> nil then Form.ModalResult := ModalResult;
  inherited Click;
end;

function TMxSpeedButton.GetPalette: HPALETTE;
begin
  Result := Glyph.Palette;
end;

function TMxSpeedButton.GetWordWrap: Boolean;
begin
  Result := TMxButtonGlyph(FGlyph).WordWrap;
end;

procedure TMxSpeedButton.SetWordWrap(Value: Boolean);
begin
  if Value <> WordWrap then begin
    TMxButtonGlyph(FGlyph).WordWrap := Value;
    Invalidate;
  end;
end;

function TMxSpeedButton.GetAlignment: TAlignment;
begin
  Result := TMxButtonGlyph(FGlyph).Alignment;
end;

procedure TMxSpeedButton.SetAlignment(Value: TAlignment);
begin
  if Alignment <> Value then begin
    TMxButtonGlyph(FGlyph).Alignment := Value;
    Invalidate;
  end;
end;

function TMxSpeedButton.GetGlyph: TBitmap;
begin
  Result := TMxButtonGlyph(FGlyph).Glyph;
end;

procedure TMxSpeedButton.SetGlyph(Value: TBitmap);
begin
  TMxButtonGlyph(FGlyph).Glyph := Value;
  Invalidate;
end;

function TMxSpeedButton.GetNumGlyphs: TMxNumGlyphs;
begin
  Result := TMxButtonGlyph(FGlyph).NumGlyphs;
end;

procedure TMxSpeedButton.SetNumGlyphs(Value: TMxNumGlyphs);
begin
  if Value < 0 then Value := 1
  else if Value > Ord(High(TMxButtonState)) + 1 then
    Value := Ord(High(TMxButtonState)) + 1;
  if Value <> TMxButtonGlyph(FGlyph).NumGlyphs then begin
    TMxButtonGlyph(FGlyph).NumGlyphs := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.GlyphChanged(Sender: TObject);
begin
  Invalidate;
end;

procedure TMxSpeedButton.UpdateExclusive;
var
  Msg: TMessage;
begin
  if (FGroupIndex <> 0) and (Parent <> nil) then begin
    Msg.Msg := CM_RXBUTTONPRESSED;
    Msg.WParam := FGroupIndex;
    Msg.LParam := Longint(Self);
    Msg.Result := 0;
    Parent.Broadcast(Msg);
  end;
end;

procedure TMxSpeedButton.SetDown(Value: Boolean);
begin
  if FGroupIndex = 0 then Value := False;
  if Value <> FDown then begin
    if FDown and (not FAllowAllUp) then Exit;
    FDown := Value;
    if Value then begin
      if FState = rbsUp then Invalidate;
      FState := rbsExclusive;
    end
    else begin
      FState := rbsUp;
    end;
    Repaint;
    if Value then UpdateExclusive;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetGroupIndex(Value: Integer);
begin
  if FGroupIndex <> Value then begin
    FGroupIndex := Value;
    UpdateExclusive;
  end;
end;

procedure TMxSpeedButton.SetLayout(Value: TButtonLayout);
begin
  if FLayout <> Value then begin
    FLayout := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetMargin(Value: Integer);
begin
  if (Value <> FMargin) and (Value >= -1) then begin
    FMargin := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetSpacing(Value: Integer);
begin
  if Value <> FSpacing then begin
    FSpacing := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetAllowAllUp(Value: Boolean);
begin
  if FAllowAllUp <> Value then begin
    FAllowAllUp := Value;
    UpdateExclusive;
  end;
end;

procedure TMxSpeedButton.SetAllowTimer(Value: Boolean);
begin
  FAllowTimer := Value;
  if not FAllowTimer and (FRepeatTimer <> nil) then begin
    FRepeatTimer.Enabled := False;
    FRepeatTimer.Free;
    FRepeatTimer := nil;
  end;
end;

procedure TMxSpeedButton.SetDropDownMenu(Value: TPopupMenu);
begin
  FDropDownMenu := Value;
{$IFDEF WIN32}
  if Value <> nil then Value.FreeNotification(Self);
{$ENDIF}
  if FMarkDropDown then Invalidate;
end;

procedure TMxSpeedButton.SetInactiveGrayed(Value: Boolean);
begin
  if Value <> FInactiveGrayed then begin
    FInactiveGrayed := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetFlat(Value: Boolean);
begin
  if Value <> FFlat then begin
    FFlat := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetStyle(Value: TButtonStyle);
begin
  if Style <> Value then begin
    FStyle := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetMarkDropDown(Value: Boolean);
begin
  if Value <> FMarkDropDown then begin
    FMarkDropDown := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.SetTransparent(Value: Boolean);
begin
  if Value <> FTransparent then begin
    FTransparent := Value;
    Invalidate;
  end;
end;

procedure TMxSpeedButton.WMRButtonDown(var Message: TWMRButtonDown);
begin
  inherited;
  UpdateTracking;
end;

procedure TMxSpeedButton.WMRButtonUp(var Message: TWMRButtonUp);
begin
  inherited;
  UpdateTracking;
end;

procedure TMxSpeedButton.WMLButtonDblClk(var Message: TWMLButtonDown);
begin
  if not FMenuTracking then begin
    inherited;
    if FDown then DblClick;
  end;
end;

procedure TMxSpeedButton.CMEnabledChanged(var Message: TMessage);
var
  State: TMxButtonState;
begin
  inherited;
  if Enabled then begin
    if Flat then State := rbsInactive
    else State := rbsUp;
  end else State := rbsDisabled;
  TMxButtonGlyph(FGlyph).CreateButtonGlyph(State);
  UpdateTracking;
  Repaint;
end;

procedure TMxSpeedButton.CMVisibleChanged(var Message: TMessage);
begin
  inherited;
  if Visible then UpdateTracking;
end;

procedure TMxSpeedButton.CMMouseEnter(var Message: TMessage);
begin
  inherited;
  if (not FMouseInControl) and Enabled and IsForegroundTask then begin
    FMouseInControl := True;
    if FFlat then Repaint;
    MouseEnter;
  end;
end;

procedure TMxSpeedButton.CMMouseLeave(var Message: TMessage);
begin
  inherited;
  if FMouseInControl and Enabled and not FDragging then begin
    FMouseInControl := False;
    if FFlat then Invalidate;
    MouseLeave;
  end;
end;

procedure TMxSpeedButton.WMMouseMove(var Message: TMessage);
begin
  inherited;
end;

procedure TMxSpeedButton.CMButtonPressed(var Message: TMessage);
var
  Sender: TControl;
begin
  if (Message.WParam = FGroupIndex) and Parent.HandleAllocated then begin
    Sender := TControl(Message.LParam);
    if (Sender <> nil) and (Sender is TMxSpeedButton) then
      if Sender <> Self then begin
        if TMxSpeedButton(Sender).Down and FDown then begin
          FDown := False;
          FState := rbsUp;
          Repaint;
        end;
        FAllowAllUp := TMxSpeedButton(Sender).AllowAllUp;
      end;
  end;
end;

procedure TMxSpeedButton.CMDialogChar(var Message: TCMDialogChar);
begin
  with Message do
    if IsAccel(CharCode, Caption) and Enabled then begin
      Click;
      Result := 1;
    end
    else inherited;
end;

procedure TMxSpeedButton.CMFontChanged(var Message: TMessage);
begin
  Invalidate;
end;

procedure TMxSpeedButton.CMTextChanged(var Message: TMessage);
begin
  Invalidate;
end;

procedure TMxSpeedButton.CMSysColorChange(var Message: TMessage);
begin
  TMxButtonGlyph(FGlyph).Invalidate;
  Invalidate;
end;

procedure TMxSpeedButton.UpdateTracking;
var
  P: TPoint;
  OldValue: Boolean;
begin
  OldValue := FMouseInControl;
  GetCursorPos(P);
  FMouseInControl := Enabled and (FindDragTarget(P, True) = Self) and
    IsForegroundTask;
  if (FMouseInControl <> OldValue) then
    if FMouseInControl then begin
      if Flat then Repaint;
      MouseEnter;
    end
    else begin
      if Flat then Invalidate;
      MouseLeave;
    end;
end;

procedure TMxSpeedButton.TimerExpired(Sender: TObject);
begin
  FRepeatTimer.Interval := RepeatInterval;
  if (FState = rbsDown) and MouseCapture then
    try
      Click;
    except
      FRepeatTimer.Enabled := False;
      raise;
    end;
end;

{$IFDEF RX_D4}
procedure TMxSpeedButton.ActionChange(Sender: TObject; CheckDefaults: Boolean);

  procedure CopyImage(ImageList: TCustomImageList; Index: Integer);
  begin
    with Glyph do begin
      Width := ImageList.Width;
      Height := ImageList.Height;
      Canvas.Brush.Color := clFuchsia;
      Canvas.FillRect(Rect(0, 0, Width, Height));
      ImageList.Draw(Canvas, 0, 0, Index);
      TransparentColor := clFuchsia;
    end;
  end;

begin
  inherited ActionChange(Sender, CheckDefaults);
  if Sender is TCustomAction then
    with TCustomAction(Sender) do begin
      if (not CheckDefaults or (Self.Down = False)) and (FGroupIndex <> 0) then
        Self.Down := Checked;
      if (Glyph.Empty) and (ActionList <> nil) and (ActionList.Images <> nil) and
        (ImageIndex >= 0) and (ImageIndex < ActionList.Images.Count) then
        CopyImage(ActionList.Images, ImageIndex);
    end;
end;
{$ENDIF RX_D4}

{$IFDEF WIN32}
initialization
  FCheckBitmap := nil;
finalization
  DestroyLocals;
{$ELSE}
initialization
  FCheckBitmap := nil;
  AddExitProc(DestroyLocals);
{$ENDIF}
end.