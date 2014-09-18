object fraGroup: TfraGroup
  Left = 505
  Top = 307
  VertScrollBar.Visible = False
  Align = alClient
  BorderStyle = bsNone
  ClientHeight = 615
  ClientWidth = 213
  Color = 10528425
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clBlack
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = True
  PixelsPerInch = 96
  TextHeight = 13
  object paCommands: TPanel
    Left = 0
    Top = 0
    Width = 213
    Height = 18
    Align = alTop
    ParentColor = True
    TabOrder = 0
    DesignSize = (
      213
      18)
    object ExtBtn2: TExtBtn
      Left = 2
      Top = 0
      Width = 73
      Height = 17
      Align = alNone
      Anchors = [akTop, akRight]
      Caption = 'Commands'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Glyph.Data = {
        DE000000424DDE00000000000000360000002800000007000000070000000100
        180000000000A8000000120B0000120B00000000000000000000FFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFF00
        0000FFFFFFFFFFFF000000000000000000FFFFFFFFFFFF000000FFFFFF000000
        000000000000000000000000FFFFFF0000000000000000000000000000000000
        00000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
        0000}
      ParentFont = False
      OnClick = ExtBtn2Click
    end
    object ebUngroup: TExtBtn
      Left = 2
      Top = 32
      Width = 107
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Ungroup'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebUngroupClick
    end
    object ebGroup: TExtBtn
      Left = 2
      Top = 16
      Width = 107
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Group'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebMakeGroupClick
    end
    object ebSaveSelected: TExtBtn
      Left = 116
      Top = 33
      Width = 94
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Save As...'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebSaveSelectedClick
    end
    object ebMakeThumbnail: TExtBtn
      Left = 116
      Top = 18
      Width = 95
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Make Thumbnail'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebMakeThumbnailClick
    end
  end
  object paCurrentObject: TPanel
    Left = 0
    Top = 18
    Width = 213
    Height = 51
    Align = alTop
    Color = 10528425
    TabOrder = 1
    object APHeadLabel2: TLabel
      Left = 1
      Top = 1
      Width = 211
      Height = 13
      Align = alTop
      Alignment = taCenter
      Caption = 'Current Object'
      Color = clGray
      ParentColor = False
      OnClick = ExpandClick
    end
    object ebSelect: TExtBtn
      Left = 7
      Top = 16
      Width = 94
      Height = 15
      Align = alNone
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Select...'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebSelectClick
    end
    object MxLabel1: TMxLabel
      Left = 4
      Top = 32
      Width = 36
      Height = 13
      Caption = 'Curent:'
      ShadowColor = clGray
      ShadowSize = 0
      ShadowPos = spRightBottom
    end
    object lbCurrent: TMxLabel
      Left = 42
      Top = 32
      Width = 172
      Height = 13
      AutoSize = False
      Caption = '<empty>'
      ShadowColor = 15000804
      ShadowPos = spRightBottom
    end
    object ebReloadRefs: TExtBtn
      Left = 109
      Top = 16
      Width = 102
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Reload Refs'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebReloadRefsClick
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 115
    Width = 213
    Height = 51
    Align = alTop
    Color = 10528425
    TabOrder = 2
    object Label1: TLabel
      Left = 1
      Top = 1
      Width = 211
      Height = 13
      Align = alTop
      Alignment = taCenter
      Caption = 'Pivot Alignment'
      Color = clGray
      ParentColor = False
      OnClick = ExpandClick
    end
    object ebCenterToGroup: TExtBtn
      Left = 2
      Top = 16
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Center To Group'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebCenterToGroupClick
    end
    object ebAlignToObject: TExtBtn
      Left = 2
      Top = 32
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Align To Object...'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ebAlignToObjectClick
    end
  end
  object paSelectObject: TPanel
    Left = 0
    Top = 69
    Width = 213
    Height = 46
    Align = alTop
    Color = 10528425
    TabOrder = 3
    DesignSize = (
      213
      46)
    object Bevel3: TBevel
      Left = 154
      Top = 28
      Width = 45
      Height = 16
    end
    object Label2: TLabel
      Left = 5
      Top = 16
      Width = 84
      Height = 13
      Caption = 'Select by Current:'
      WordWrap = True
    end
    object ExtBtn1: TExtBtn
      Left = 97
      Top = 15
      Width = 27
      Height = 14
      Align = alNone
      BevelShow = False
      HotTrack = True
      Caption = '+'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ExtBtn1Click
    end
    object ExtBtn3: TExtBtn
      Left = 126
      Top = 15
      Width = 27
      Height = 14
      Align = alNone
      BevelShow = False
      HotTrack = True
      Caption = '-'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ExtBtn3Click
    end
    object APHeadLabel3: TLabel
      Left = 1
      Top = 1
      Width = 211
      Height = 13
      Align = alTop
      Alignment = taCenter
      Caption = 'Reference Select'
      Color = clGray
      ParentColor = False
      OnClick = ExpandClick
    end
    object ExtBtn5: TExtBtn
      Left = 200
      Top = 2
      Width = 11
      Height = 11
      Align = alNone
      Anchors = [akTop, akRight]
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Glyph.Data = {
        DE000000424DDE00000000000000360000002800000007000000070000000100
        180000000000A8000000120B0000120B00000000000000000000FFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFF00
        0000FFFFFFFFFFFF000000000000000000FFFFFFFFFFFF000000FFFFFF000000
        000000000000000000000000FFFFFF0000000000000000000000000000000000
        00000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
        0000}
      ParentFont = False
      OnClick = PaneMinClick
    end
    object ExtBtn6: TExtBtn
      Left = 97
      Top = 29
      Width = 27
      Height = 14
      Align = alNone
      BevelShow = False
      HotTrack = True
      Caption = '=%'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ExtBtn6Click
    end
    object Label4: TLabel
      Left = 201
      Top = 30
      Width = 8
      Height = 13
      Caption = '%'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object ExtBtn7: TExtBtn
      Left = 126
      Top = 29
      Width = 27
      Height = 14
      Align = alNone
      BevelShow = False
      HotTrack = True
      Caption = '+%'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      OnClick = ExtBtn7Click
    end
    object Label6: TLabel
      Left = 5
      Top = 31
      Width = 92
      Height = 13
      Caption = 'Select by Selected:'
      WordWrap = True
    end
    object seSelPercent: TMultiObjSpinEdit
      Left = 155
      Top = 30
      Width = 43
      Height = 13
      LWSensitivity = 1
      ButtonKind = bkLightWave
      MaxValue = 100
      MinValue = 1
      Value = 100
      AutoSize = False
      BorderStyle = bsNone
      Color = 12698049
      TabOrder = 0
      OnKeyPress = seSelPercentKeyPress
    end
  end
  object pmGroupCommands: TMxPopupMenu
    Alignment = paCenter
    AutoPopup = False
    TrackButton = tbLeftButton
    MarginStartColor = 10921638
    MarginEndColor = 2763306
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = 10526880
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 11
    Top = 199
    object N17: TMenuItem
      Caption = 'Group'
      OnClick = ebMakeGroupClick
    end
    object Ungroup1: TMenuItem
      Caption = 'Ungroup'
      OnClick = ebUngroupClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object MakeThumbnail1: TMenuItem
      Caption = 'Make Thumbnail'
      OnClick = ebMakeThumbnailClick
    end
    object N2: TMenuItem
      Caption = '-'
    end
    object SaveAs1: TMenuItem
      Caption = 'Save As...'
      OnClick = ebSaveSelectedClick
    end
  end
end
