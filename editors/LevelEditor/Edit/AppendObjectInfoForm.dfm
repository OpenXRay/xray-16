object frmAppendObjectInfo: TfrmAppendObjectInfo
  Left = 741
  Top = 135
  Anchors = [akLeft, akBottom]
  BorderStyle = bsDialog
  Caption = 'Loading problem'
  ClientHeight = 114
  ClientWidth = 258
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  DesignSize = (
    258
    114)
  PixelsPerInch = 96
  TextHeight = 13
  object StaticText1: TStaticText
    Left = 8
    Top = 7
    Width = 139
    Height = 17
    Caption = 'Object already exist in scene'
    TabOrder = 4
  end
  object btOverwrite: TButton
    Tag = 1
    Left = 8
    Top = 88
    Width = 80
    Height = 20
    Anchors = [akLeft, akBottom]
    Caption = 'Overwrite'
    TabOrder = 1
    OnClick = btOverwriteClick
  end
  object btSkip: TButton
    Tag = 5
    Left = 169
    Top = 88
    Width = 80
    Height = 20
    Anchors = [akLeft, akBottom]
    Caption = 'Skip'
    TabOrder = 3
    OnClick = btOverwriteClick
  end
  object CheckBox1: TCheckBox
    Left = 8
    Top = 66
    Width = 121
    Height = 17
    Anchors = [akLeft, akBottom]
    Caption = 'Dont ask me again'
    TabOrder = 0
  end
  object Button1: TButton
    Tag = 3
    Left = 88
    Top = 88
    Width = 80
    Height = 20
    Anchors = [akLeft, akBottom]
    Caption = 'Auto rename'
    TabOrder = 2
    OnClick = btOverwriteClick
  end
  object StaticText2: TStaticText
    Left = 8
    Top = 27
    Width = 102
    Height = 17
    Caption = 'object name here'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = [fsBold]
    ParentFont = False
    TabOrder = 5
  end
end
