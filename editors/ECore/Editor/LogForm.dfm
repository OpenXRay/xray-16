object frmLog: TfrmLog
  Left = 560
  Top = 413
  Width = 400
  Height = 225
  BorderStyle = bsSizeToolWin
  Caption = 'Log'
  Color = clBtnFace
  Constraints.MinHeight = 80
  Constraints.MinWidth = 400
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Scaled = False
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 178
    Width = 392
    Height = 20
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 0
    object ebClear: TExtBtn
      Left = 184
      Top = 2
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Clear'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebClearClick
    end
    object ebClearSelected: TExtBtn
      Left = 266
      Top = 2
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Clear Selected'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebClearSelectedClick
    end
    object ebClose: TExtBtn
      Left = 0
      Top = 2
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Close'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebCloseClick
    end
    object ebFlush: TExtBtn
      Left = 82
      Top = 2
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Flush'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebFlushClick
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 392
    Height = 178
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 1
    object lbLog: TListBox
      Left = 0
      Top = 0
      Width = 392
      Height = 178
      Style = lbOwnerDrawFixed
      AutoComplete = False
      Align = alClient
      Color = 15263976
      ItemHeight = 13
      MultiSelect = True
      PopupMenu = MxPopupMenu1
      TabOrder = 0
      OnDrawItem = lbLogDrawItem
      OnKeyDown = lbLogKeyDown
      OnKeyPress = lbLogKeyPress
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Log Form'
    Version = 1
    StoredValues = <>
    Left = 8
    Top = 8
  end
  object MxPopupMenu1: TMxPopupMenu
    MarginEndColor = clBlack
    Left = 40
    Top = 8
    object imCopy: TMenuItem
      Caption = '&Copy'
      ShortCut = 16451
      OnClick = imCopyClick
    end
    object imSelectAll: TMenuItem
      Caption = 'Select &All'
      ShortCut = 16449
      OnClick = imSelectAllClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object ClearAll1: TMenuItem
      Caption = 'Clear All'
      OnClick = ebClearClick
    end
    object ClearSelected1: TMenuItem
      Caption = 'Clear Selected'
      OnClick = ebClearSelectedClick
    end
  end
end
