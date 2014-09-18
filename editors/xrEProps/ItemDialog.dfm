object frmItemDialog: TfrmItemDialog
  Left = 606
  Top = 409
  BorderIcons = [biSystemMenu]
  BorderStyle = bsToolWindow
  Caption = 'Item'
  ClientHeight = 105
  ClientWidth = 274
  Color = 10528425
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Position = poDefault
  Scaled = False
  OnClose = FormClose
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object eb0: TExtBtn
    Left = 1
    Top = 51
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb3: TExtBtn
    Tag = 3
    Left = 1
    Top = 69
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object lbMsg: TMxLabel
    Left = 0
    Top = 7
    Width = 274
    Height = 42
    Align = alTop
    Alignment = taCenter
    AutoSize = False
    Caption = 'Overwrite:'
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    ShadowColor = clBlack
    ShadowSize = 0
    ShadowPos = spRightBottom
    WordWrap = True
  end
  object eb1: TExtBtn
    Tag = 1
    Left = 92
    Top = 51
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb4: TExtBtn
    Tag = 4
    Left = 92
    Top = 69
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb2: TExtBtn
    Tag = 2
    Left = 183
    Top = 51
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb5: TExtBtn
    Tag = 5
    Left = 183
    Top = 69
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb6: TExtBtn
    Tag = 6
    Left = 1
    Top = 87
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb7: TExtBtn
    Tag = 7
    Left = 92
    Top = 87
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object eb8: TExtBtn
    Tag = 8
    Left = 183
    Top = 87
    Width = 90
    Height = 17
    Align = alNone
    BevelShow = False
    BtnColor = 10528425
    Caption = '-'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    Transparent = False
    FlatAlwaysEdge = True
    Visible = False
    OnClick = ebClick
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 274
    Height = 7
    Align = alTop
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 0
  end
  object fsStorage: TFormStorage
    IniSection = 'Item Dialog'
    RegistryRoot = prLocalMachine
    Version = 8
    StoredValues = <>
    Left = 65529
    Top = 65526
  end
end
