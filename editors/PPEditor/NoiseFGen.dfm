object NFGen: TNFGen
  Left = 336
  Top = 587
  BorderStyle = bsDialog
  Caption = 'Noise generator'
  ClientHeight = 81
  ClientWidth = 289
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object RxLabel1: TMxLabel
    Left = 0
    Top = 16
    Width = 50
    Height = 13
    Caption = 'First value'
  end
  object RxLabel2: TMxLabel
    Left = 144
    Top = 16
    Width = 68
    Height = 13
    Caption = 'Second value'
  end
  object RxLabel3: TMxLabel
    Left = 0
    Top = 40
    Width = 32
    Height = 13
    Caption = 'Period'
  end
  object RxLabel4: TMxLabel
    Left = 144
    Top = 40
    Width = 25
    Height = 13
    Caption = 'Time'
  end
  object Bevel1: TBevel
    Left = 0
    Top = 56
    Width = 289
    Height = 9
    Shape = bsTopLine
  end
  object First: TMultiObjSpinEdit
    Left = 60
    Top = 8
    Width = 73
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 0
  end
  object Period: TMultiObjSpinEdit
    Left = 60
    Top = 32
    Width = 73
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 1
  end
  object Second: TMultiObjSpinEdit
    Left = 216
    Top = 8
    Width = 73
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 2
  end
  object Time: TMultiObjSpinEdit
    Left = 216
    Top = 32
    Width = 73
    Height = 21
    Alignment = taRightJustify
    ValueType = vtFloat
    TabOrder = 3
  end
  object Button1: TButton
    Left = 0
    Top = 60
    Width = 75
    Height = 21
    Caption = 'Generate'
    Default = True
    TabOrder = 4
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 212
    Top = 60
    Width = 75
    Height = 21
    Cancel = True
    Caption = 'Cancel'
    TabOrder = 5
  end
end
