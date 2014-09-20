object frmShaderFunction: TfrmShaderFunction
  Left = 465
  Top = 371
  BorderStyle = bsDialog
  Caption = 'Wave Form'
  ClientHeight = 157
  ClientWidth = 415
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  Scaled = False
  OnClose = FormClose
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 415
    Height = 157
    Align = alClient
    BevelOuter = bvNone
    Color = 10528425
    TabOrder = 0
    object RxLabel1: TLabel
      Left = 5
      Top = 7
      Width = 44
      Height = 13
      Caption = 'Function:'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label4: TLabel
      Left = 5
      Top = 29
      Width = 61
      Height = 13
      Caption = 'Offset (arg1):'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label1: TLabel
      Left = 5
      Top = 53
      Width = 79
      Height = 13
      Caption = 'Amplitude (arg2):'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label2: TLabel
      Left = 5
      Top = 77
      Width = 63
      Height = 13
      Caption = 'Phase (arg3):'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label3: TLabel
      Left = 5
      Top = 101
      Width = 56
      Height = 13
      Caption = 'Rate (arg4):'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Bevel2: TBevel
      Left = 223
      Top = 24
      Width = 188
      Height = 112
    end
    object imDraw: TImage
      Left = 224
      Top = 25
      Width = 186
      Height = 110
    end
    object Label5: TLabel
      Left = 225
      Top = 5
      Width = 185
      Height = 13
      Caption = 'y = arg1 + arg2*func((time + arg3)*arg4)'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object lbMax: TLabel
      Left = 200
      Top = 23
      Width = 20
      Height = 13
      Alignment = taRightJustify
      Caption = 'Max'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object lbCenter: TLabel
      Left = 189
      Top = 74
      Width = 31
      Height = 13
      Alignment = taRightJustify
      Caption = 'Center'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object lbMin: TLabel
      Left = 203
      Top = 122
      Width = 17
      Height = 13
      Alignment = taRightJustify
      Caption = 'Min'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object lbStart: TLabel
      Left = 221
      Top = 139
      Width = 22
      Height = 13
      Alignment = taRightJustify
      Caption = '0 ms'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object lbEnd: TLabel
      Left = 370
      Top = 139
      Width = 40
      Height = 13
      Alignment = taRightJustify
      Caption = '1000 ms'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object ebOk: TExtBtn
      Left = 4
      Top = 136
      Width = 91
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Ok'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebOkClick
    end
    object ebCancel: TExtBtn
      Left = 96
      Top = 136
      Width = 91
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Cancel'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebCancelClick
    end
    object Label6: TLabel
      Left = 256
      Top = 139
      Width = 30
      Height = 13
      Alignment = taRightJustify
      Caption = 'Scale:'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object stFunction: TStaticText
      Left = 89
      Top = 6
      Width = 94
      Height = 16
      Alignment = taCenter
      AutoSize = False
      BorderStyle = sbsSunken
      Caption = '...'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clNavy
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      OnMouseDown = stFunctionMouseDown
    end
    object seArg1: TMultiObjSpinEdit
      Left = 89
      Top = 27
      Width = 95
      Height = 21
      LWSensitivity = 0.1
      Alignment = taRightJustify
      ButtonKind = bkLightWave
      Decimal = 5
      Increment = 1E-5
      MaxValue = 100000
      MinValue = -100000
      ValueType = vtFloat
      AutoSize = False
      Color = 13158600
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 1
      OnLWChange = seArgLWChange
      OnExit = seArgExit
      OnKeyDown = seArgKeyDown
    end
    object seArg2: TMultiObjSpinEdit
      Left = 89
      Top = 51
      Width = 95
      Height = 21
      LWSensitivity = 0.1
      Alignment = taRightJustify
      ButtonKind = bkLightWave
      Decimal = 5
      Increment = 1E-5
      MaxValue = 100000
      MinValue = -100000
      ValueType = vtFloat
      AutoSize = False
      Color = 13158600
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 2
      OnLWChange = seArgLWChange
      OnExit = seArgExit
      OnKeyDown = seArgKeyDown
    end
    object seArg3: TMultiObjSpinEdit
      Left = 89
      Top = 75
      Width = 95
      Height = 21
      LWSensitivity = 0.1
      Alignment = taRightJustify
      ButtonKind = bkLightWave
      Decimal = 5
      Increment = 1E-5
      MaxValue = 100000
      MinValue = -100000
      ValueType = vtFloat
      AutoSize = False
      Color = 13158600
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 3
      OnLWChange = seArgLWChange
      OnExit = seArgExit
      OnKeyDown = seArgKeyDown
    end
    object seArg4: TMultiObjSpinEdit
      Left = 89
      Top = 99
      Width = 95
      Height = 21
      LWSensitivity = 0.1
      Alignment = taRightJustify
      ButtonKind = bkLightWave
      Decimal = 5
      Increment = 1E-5
      MaxValue = 100000
      MinValue = -100000
      ValueType = vtFloat
      AutoSize = False
      Color = 13158600
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 4
      OnLWChange = seArgLWChange
      OnExit = seArgExit
      OnKeyDown = seArgKeyDown
    end
    object seScale: TMultiObjSpinEdit
      Left = 289
      Top = 137
      Width = 58
      Height = 21
      LWSensitivity = 0.1
      Alignment = taRightJustify
      ButtonKind = bkLightWave
      Increment = 0.01
      MaxValue = 100
      MinValue = 0.01
      ValueType = vtFloat
      Value = 1
      AutoSize = False
      Color = 13158600
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 5
      OnLWChange = seArgLWChange
      OnExit = seArgExit
      OnKeyDown = seArgKeyDown
    end
  end
  object pmFunction: TMxPopupMenu
    Alignment = paCenter
    AutoPopup = False
    TrackButton = tbLeftButton
    MarginStartColor = 13158600
    MarginEndColor = 1644825
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = clWhite
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 61
    Top = 65529
  end
  object fsStorage: TFormStorage
    IniSection = 'Shader Function'
    Options = [fpPosition]
    UseRegistry = True
    StoredValues = <>
    Top = 65520
  end
end
