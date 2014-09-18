object Form8: TForm8
  Left = 350
  Top = 648
  Width = 313
  Height = 160
  BorderIcons = [biSystemMenu]
  Caption = 'Single constructor'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Button1: TButton
    Left = 4
    Top = 108
    Width = 75
    Height = 25
    Caption = 'OK'
    ModalResult = 1
    TabOrder = 0
  end
  object Button2: TButton
    Left = 208
    Top = 108
    Width = 75
    Height = 25
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 245
    Height = 29
    TabOrder = 2
    object Label1: TLabel
      Left = 4
      Top = 8
      Width = 53
      Height = 13
      Caption = 'Initial value'
    end
    object InitValue: TMultiObjSpinEdit
      Left = 120
      Top = 4
      Width = 121
      Height = 21
      Alignment = taRightJustify
      Increment = 0.05
      ValueType = vtFloat
      TabOrder = 0
    end
  end
  object OpenDialog: TOpenDialog
    DefaultExt = 'scon'
    Filter = 'Single constructor files (*.scon)|*.scon|All files (*.*)|*.*'
    FilterIndex = 0
    Options = [ofHideReadOnly, ofNoChangeDir, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 52
    Top = 28
  end
  object SaveDialog: TSaveDialog
    DefaultExt = 'scon'
    Filter = 'Single constructor files (*.scon)|*.scon|All files (*.*)|*.*'
    Options = [ofHideReadOnly, ofNoChangeDir, ofPathMustExist, ofEnableSizing]
    Left = 116
    Top = 28
  end
end
