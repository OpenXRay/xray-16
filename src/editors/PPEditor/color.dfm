object AddColorForm: TAddColorForm
  Left = 596
  Top = 516
  BorderIcons = []
  BorderStyle = bsNone
  ClientHeight = 250
  ClientWidth = 195
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBox1: TGroupBox
    Left = 4
    Top = 4
    Width = 185
    Height = 213
    Caption = 'Color'
    TabOrder = 0
    object RxLabel2: TMxLabel
      Left = 12
      Top = 48
      Width = 10
      Height = 13
      Caption = 'R'
      Color = clBtnFace
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object RxLabel6: TMxLabel
      Left = 12
      Top = 72
      Width = 10
      Height = 13
      Caption = 'G'
      Color = clBtnFace
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clLime
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object RxLabel10: TMxLabel
      Left = 12
      Top = 96
      Width = 9
      Height = 13
      Caption = 'B'
      Color = clBtnFace
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlue
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object labelIntensity: TMxLabel
      Left = 13
      Top = 117
      Width = 35
      Height = 13
      Caption = 'Itensity'
      Visible = False
    end
    object MxLabel1: TMxLabel
      Left = 5
      Top = 17
      Width = 69
      Height = 13
      Caption = 'Key Time(sec)'
    end
    object RedValue: TMultiObjSpinEdit
      Left = 86
      Top = 43
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 1
      OnChange = CnahgeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
    object GreenValue: TMultiObjSpinEdit
      Left = 86
      Top = 67
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 2
      OnChange = CnahgeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
    object BlueValue: TMultiObjSpinEdit
      Left = 86
      Top = 89
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 3
      OnChange = CnahgeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
    object IntensityValue: TMultiObjSpinEdit
      Left = 86
      Top = 112
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 4
      Visible = False
      OnChange = CnahgeParam
      OnExit = TimeValueExit
    end
    object TimeValue: TMultiObjSpinEdit
      Left = 86
      Top = 11
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.1
      MaxValue = 99999
      ValueType = vtFloat
      TabOrder = 0
      OnChange = CnahgeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
  end
  object Color: TPanel
    Left = 12
    Top = 144
    Width = 37
    Height = 21
    TabOrder = 1
    OnClick = ColorClick
  end
  object ColorDialog: TColorDialog
    Ctl3D = True
    Options = [cdFullOpen, cdSolidColor]
    Left = 56
    Top = 140
  end
end
