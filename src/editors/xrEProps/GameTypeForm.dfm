object fmGameType: TfmGameType
  Left = 995
  Top = 394
  BorderIcons = [biSystemMenu]
  BorderStyle = bsToolWindow
  ClientHeight = 152
  ClientWidth = 163
  Color = 10528425
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  DesignSize = (
    163
    152)
  PixelsPerInch = 96
  TextHeight = 13
  object ebOk: TExtBtn
    Left = 105
    Top = 117
    Width = 56
    Height = 17
    Align = alNone
    Anchors = [akRight, akBottom]
    BevelShow = False
    BtnColor = 10528425
    Caption = 'Ok'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    OnClick = ebOkClick
  end
  object ebCancel: TExtBtn
    Left = 105
    Top = 135
    Width = 56
    Height = 17
    Align = alNone
    Anchors = [akRight, akBottom]
    BevelShow = False
    BtnColor = 10528425
    Caption = 'Cancel'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    OnClick = ebCancelClick
  end
  object cbSingle: TCheckBox
    Left = 0
    Top = 0
    Width = 145
    Height = 17
    Caption = 'Single'
    TabOrder = 0
  end
  object cbDeathMatch: TCheckBox
    Left = 0
    Top = 16
    Width = 145
    Height = 17
    Caption = 'DM'
    TabOrder = 1
  end
  object cbTeamDeathMatch: TCheckBox
    Left = 0
    Top = 32
    Width = 145
    Height = 17
    Caption = 'TDM'
    TabOrder = 2
  end
  object cbArtefactHunt: TCheckBox
    Left = 0
    Top = 48
    Width = 145
    Height = 17
    Caption = 'ArtefactHunt'
    TabOrder = 3
  end
  object cbCTA: TCheckBox
    Left = 0
    Top = 64
    Width = 145
    Height = 17
    Caption = 'CTA'
    TabOrder = 4
  end
end
