object AddFloatForm: TAddFloatForm
  Left = 1000
  Top = 375
  BorderIcons = []
  BorderStyle = bsNone
  ClientHeight = 233
  ClientWidth = 199
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object MxLabel2: TMxLabel
    Left = 16
    Top = 83
    Width = 29
    Height = 13
    Caption = 'Value'
  end
  object GroupBox1: TGroupBox
    Left = 4
    Top = 4
    Width = 185
    Height = 213
    Caption = 'Node data'
    TabOrder = 0
    object Label1: TMxLabel
      Left = 12
      Top = 48
      Width = 29
      Height = 13
      Caption = 'Value'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object Label2: TMxLabel
      Left = 14
      Top = 73
      Width = 29
      Height = 13
      Caption = 'Value'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = 4259584
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Visible = False
    end
    object Label3: TMxLabel
      Left = 15
      Top = 99
      Width = 29
      Height = 13
      Caption = 'Value'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlue
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Visible = False
    end
    object MxLabel1: TMxLabel
      Left = 12
      Top = 15
      Width = 69
      Height = 13
      Caption = 'Key Time(sec)'
    end
    object Value1: TMultiObjSpinEdit
      Left = 86
      Top = 43
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 1
      OnChange = ChangeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
    object Value2: TMultiObjSpinEdit
      Left = 86
      Top = 67
      Width = 93
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      Increment = 0.01
      ValueType = vtFloat
      TabOrder = 2
      Visible = False
      OnChange = ChangeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
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
      OnChange = ChangeParam
      OnExit = TimeValueExit
      OnKeyDown = TimeValueKeyDown
    end
    object cmTextureName: TEdit
      Left = 8
      Top = 184
      Width = 169
      Height = 21
      TabOrder = 3
      Text = 'cmTextureName'
      OnChange = cmTextureNameChange
    end
  end
  object Value3: TMultiObjSpinEdit
    Left = 90
    Top = 95
    Width = 93
    Height = 21
    LWSensitivity = 1
    Alignment = taRightJustify
    Increment = 0.01
    ValueType = vtFloat
    TabOrder = 1
    Visible = False
    OnChange = ChangeParam
    OnExit = TimeValueExit
    OnKeyDown = TimeValueKeyDown
  end
end
