object NewEffectDialog: TNewEffectDialog
  Left = 292
  Top = 382
  BorderStyle = bsDialog
  Caption = 'NewEffectDialog'
  ClientHeight = 66
  ClientWidth = 158
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
    Left = 4
    Top = 16
    Width = 52
    Height = 13
    Caption = 'Effect time'
  end
  object Time: TMultiObjSpinEdit
    Left = 68
    Top = 8
    Width = 89
    Height = 21
    Alignment = taRightJustify
    MaxValue = 1000
    ValueType = vtFloat
    TabOrder = 0
    OnChange = TimeChange
  end
  object Button1: TButton
    Left = 0
    Top = 48
    Width = 75
    Height = 17
    Caption = 'OK'
    Default = True
    Enabled = False
    ModalResult = 1
    TabOrder = 1
  end
  object Button2: TButton
    Left = 84
    Top = 48
    Width = 75
    Height = 17
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
end
