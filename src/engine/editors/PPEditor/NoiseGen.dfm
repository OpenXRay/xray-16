object NGen: TNGen
  Left = 416
  Top = 454
  BorderStyle = bsDialog
  Caption = 'Noise generator'
  ClientHeight = 159
  ClientWidth = 335
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object RxLabel7: TMxLabel
    Left = 4
    Top = 112
    Width = 32
    Height = 13
    Caption = 'Period'
  end
  object RxLabel8: TMxLabel
    Left = 220
    Top = 112
    Width = 25
    Height = 13
    Caption = 'Time'
  end
  object Bevel1: TBevel
    Left = 0
    Top = 132
    Width = 333
    Height = 9
    Shape = bsTopLine
  end
  object GroupBox1: TGroupBox
    Left = 0
    Top = 0
    Width = 333
    Height = 49
    Caption = 'First color'
    TabOrder = 0
    object RxLabel1: TMxLabel
      Left = 48
      Top = 24
      Width = 10
      Height = 13
      Caption = 'R'
    end
    object RxLabel2: TMxLabel
      Left = 144
      Top = 24
      Width = 10
      Height = 13
      Caption = 'G'
    end
    object RxLabel3: TMxLabel
      Left = 240
      Top = 24
      Width = 9
      Height = 13
      Caption = 'B'
    end
    object FR: TMultiObjSpinEdit
      Left = 60
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 0
    end
    object FG: TMultiObjSpinEdit
      Left = 156
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 1
    end
    object FB: TMultiObjSpinEdit
      Left = 252
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 2
    end
    object FColor: TPanel
      Left = 8
      Top = 20
      Width = 21
      Height = 21
      TabOrder = 3
      OnClick = FColorClick
    end
  end
  object GroupBox2: TGroupBox
    Left = 0
    Top = 52
    Width = 333
    Height = 49
    Caption = 'Second color'
    TabOrder = 1
    object RxLabel4: TMxLabel
      Left = 48
      Top = 24
      Width = 10
      Height = 13
      Caption = 'R'
    end
    object RxLabel5: TMxLabel
      Left = 144
      Top = 24
      Width = 10
      Height = 13
      Caption = 'G'
    end
    object RxLabel6: TMxLabel
      Left = 240
      Top = 24
      Width = 9
      Height = 13
      Caption = 'B'
    end
    object SR: TMultiObjSpinEdit
      Left = 60
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 0
    end
    object SG: TMultiObjSpinEdit
      Left = 156
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 1
    end
    object SB: TMultiObjSpinEdit
      Left = 252
      Top = 20
      Width = 73
      Height = 21
      Alignment = taRightJustify
      ValueType = vtFloat
      TabOrder = 2
    end
    object SColor: TPanel
      Left = 8
      Top = 20
      Width = 21
      Height = 21
      TabOrder = 3
      OnClick = SColorClick
    end
  end
  object Period: TMultiObjSpinEdit
    Left = 44
    Top = 104
    Width = 85
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 2
  end
  object Time: TMultiObjSpinEdit
    Left = 248
    Top = 104
    Width = 85
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 3
  end
  object Button1: TButton
    Left = 0
    Top = 136
    Width = 75
    Height = 21
    Caption = 'Generate'
    Default = True
    TabOrder = 4
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 260
    Top = 136
    Width = 75
    Height = 21
    Cancel = True
    Caption = 'Cancel'
    TabOrder = 5
  end
  object ColorDialog: TColorDialog
    Ctl3D = True
    Options = [cdFullOpen, cdSolidColor]
    Left = 152
    Top = 104
  end
end
