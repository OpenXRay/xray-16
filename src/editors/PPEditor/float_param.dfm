object frmTimeConstructor: TfrmTimeConstructor
  Left = 829
  Top = 128
  BorderStyle = bsNone
  ClientHeight = 29
  ClientWidth = 385
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 385
    Height = 29
    Align = alClient
    TabOrder = 0
    DesignSize = (
      385
      29)
    object AddButton: TSpeedButton
      Left = 334
      Top = 4
      Width = 23
      Height = 22
      Anchors = [akTop, akRight]
      Caption = '+'
    end
    object DeleteButton: TSpeedButton
      Left = 359
      Top = 4
      Width = 23
      Height = 22
      Anchors = [akTop, akRight]
      Caption = '-'
    end
    object RxLabel2: TMxLabel
      Left = 156
      Top = 8
      Width = 58
      Height = 13
      Caption = 'Length(sec)'
    end
    object MxLabel1: TMxLabel
      Left = 4
      Top = 8
      Width = 47
      Height = 13
      Caption = 'Start(sec)'
    end
    object WorkTime: TMultiObjSpinEdit
      Left = 220
      Top = 4
      Width = 101
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      MaxValue = 10000
      ValueType = vtFloat
      TabOrder = 0
    end
    object StartTime: TMultiObjSpinEdit
      Left = 52
      Top = 3
      Width = 101
      Height = 21
      LWSensitivity = 1
      Alignment = taRightJustify
      MaxValue = 10000
      ValueType = vtFloat
      Enabled = False
      TabOrder = 1
    end
  end
end
