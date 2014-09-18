object fraBottomBar: TfraBottomBar
  Left = 0
  Top = 0
  Width = 443
  Height = 270
  VertScrollBar.Visible = False
  Align = alClient
  Color = 10528425
  ParentColor = False
  TabOrder = 0
  object paBottomBar: TPanel
    Left = 0
    Top = 0
    Width = 443
    Height = 17
    Align = alTop
    BevelOuter = bvLowered
    ParentColor = True
    TabOrder = 0
    object paInfo: TPanel
      Left = 95
      Top = 1
      Width = 347
      Height = 15
      Align = alRight
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 0
      object paSel: TPanel
        Left = 0
        Top = 0
        Width = 82
        Height = 15
        Align = alLeft
        Alignment = taLeftJustify
        BevelInner = bvLowered
        Caption = 'Sel: 0'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clMaroon
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = True
        ParentFont = False
        TabOrder = 0
      end
      object paGridSquareSize: TPanel
        Left = 289
        Top = 0
        Width = 58
        Height = 15
        Align = alRight
        Alignment = taLeftJustify
        BevelInner = bvLowered
        Caption = 'Grid: 0.1 m'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clMaroon
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = True
        ParentFont = False
        TabOrder = 1
      end
      object paUICursor: TPanel
        Left = 205
        Top = 0
        Width = 84
        Height = 15
        Align = alRight
        Alignment = taLeftJustify
        BevelInner = bvLowered
        Caption = 'Cur: 2048, 2048'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clMaroon
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = True
        ParentFont = False
        TabOrder = 2
      end
      object paCamera: TPanel
        Left = 82
        Top = 0
        Width = 123
        Height = 15
        Align = alClient
        Alignment = taLeftJustify
        BevelInner = bvLowered
        Caption = 'Cam: '
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clMaroon
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = True
        ParentFont = False
        TabOrder = 3
      end
    end
    object paStatusBar: TPanel
      Left = 219
      Top = 1
      Width = 141
      Height = 15
      Align = alClient
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 1
      object paStatusLabel: TPanel
        Left = 0
        Top = 0
        Width = 48
        Height = 15
        Align = alLeft
        BevelInner = bvRaised
        BevelOuter = bvNone
        Caption = 'Status:'
        ParentColor = True
        TabOrder = 0
      end
      object paStatus: TPanel
        Left = 48
        Top = 0
        Width = 93
        Height = 15
        Align = alClient
        Alignment = taLeftJustify
        BevelInner = bvLowered
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clMaroon
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = True
        ParentFont = False
        TabOrder = 1
        object cgProgress: TCGauge
          Left = -36
          Top = 2
          Width = 127
          Height = 11
          Align = alRight
          Font.Charset = RUSSIAN_CHARSET
          Font.Color = clMaroon
          Font.Height = -11
          Font.Name = 'MS Serif'
          Font.Style = []
          ForeColor = 5592405
          ParentFont = False
          Progress = 75
          Visible = False
        end
      end
    end
    object paTools: TPanel
      Left = 1
      Top = 1
      Width = 218
      Height = 15
      Align = alLeft
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 2
      object ebOptions: TExtBtn
        Left = 0
        Top = 0
        Width = 58
        Height = 15
        Align = alNone
        BevelShow = False
        ExtButton = True
        ExtTransparent = True
        Caption = 'Options'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        ParentFont = False
        FlatAlwaysEdge = True
        OnMouseDown = ebOptionsMouseDown
      end
      object ebLog: TExtBtn
        Left = 116
        Top = 0
        Width = 34
        Height = 15
        Align = alNone
        BevelShow = False
        Caption = 'Log'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        ParentFont = False
        Spacing = 0
        FlatAlwaysEdge = True
        OnClick = ebLogClick
      end
      object ebStat: TExtBtn
        Left = 150
        Top = 0
        Width = 34
        Height = 15
        Align = alNone
        BevelShow = False
        Caption = 'Stat'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        ParentFont = False
        Spacing = 0
        FlatAlwaysEdge = True
        OnClick = ebStatClick
      end
      object ebStop: TExtBtn
        Left = 184
        Top = 0
        Width = 34
        Height = 15
        Align = alNone
        BevelShow = False
        Caption = 'Break'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        ParentFont = False
        FlatAlwaysEdge = True
        OnClick = ebStopClick
      end
      object ebMacro: TExtBtn
        Left = 58
        Top = 0
        Width = 58
        Height = 15
        Align = alNone
        BevelShow = False
        ExtButton = True
        ExtTransparent = True
        Caption = 'Macro'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        ParentFont = False
        FlatAlwaysEdge = True
        OnMouseDown = ebMacroMouseDown
      end
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Bottom Bar'
    Options = []
    RegistryRoot = prLocalMachine
    OnRestorePlacement = fsStorageRestorePlacement
    StoredProps.Strings = (
      'N100.Checked'
      'N125.Checked'
      'N150.Checked'
      'N200.Checked'
      'N25.Checked'
      'N50.Checked'
      'N75.Checked'
      'miRenderFillPoint.Checked'
      'miRenderFillSolid.Checked'
      'miRenderFillWireframe.Checked'
      'miRenderHWTransform.Checked'
      'miRenderShadeFlat.Checked'
      'miRenderShadeGouraud.Checked'
      'miRenderWithTextures.Checked')
    StoredValues = <>
    Left = 41
    Top = 25
  end
  object pmOptions: TMxPopupMenu
    Alignment = paCenter
    AutoHotkeys = maManual
    AutoPopup = False
    TrackButton = tbLeftButton
    OnPopup = pmOptionsPopup
    MarginStartColor = 10921638
    MarginEndColor = 2763306
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = 10526880
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 5
    Top = 24
    object Render1: TMenuItem
      Caption = 'Render'
      object Quality1: TMenuItem
        Caption = 'Quality'
        object N25: TMenuItem
          Tag = 25
          Caption = '25%'
          RadioItem = True
          OnClick = QualityClick
        end
        object N50: TMenuItem
          Tag = 50
          Caption = '50%'
          RadioItem = True
          OnClick = QualityClick
        end
        object N75: TMenuItem
          Tag = 75
          Caption = '75%'
          RadioItem = True
          OnClick = QualityClick
        end
        object N100: TMenuItem
          Tag = 100
          Caption = '100%'
          Checked = True
          RadioItem = True
          OnClick = QualityClick
        end
        object N125: TMenuItem
          Tag = 125
          Caption = '125%'
          RadioItem = True
          OnClick = QualityClick
        end
        object N150: TMenuItem
          Tag = 150
          Caption = '150%'
          RadioItem = True
          OnClick = QualityClick
        end
        object N200: TMenuItem
          Tag = 200
          Caption = '200%'
          RadioItem = True
          OnClick = QualityClick
        end
      end
      object FillMode1: TMenuItem
        Caption = 'Fill Mode'
        object miRenderFillPoint: TMenuItem
          Caption = 'Point'
          RadioItem = True
          OnClick = ClickOptionsMenuItem
        end
        object miRenderFillWireframe: TMenuItem
          Caption = 'Wireframe'
          RadioItem = True
          OnClick = ClickOptionsMenuItem
        end
        object miRenderFillSolid: TMenuItem
          Caption = 'Solid'
          Checked = True
          RadioItem = True
          OnClick = ClickOptionsMenuItem
        end
      end
      object ShadeMode1: TMenuItem
        Caption = 'Shade Mode'
        object miRenderShadeFlat: TMenuItem
          Caption = 'Flat'
          Checked = True
          RadioItem = True
          OnClick = ClickOptionsMenuItem
        end
        object miRenderShadeGouraud: TMenuItem
          Caption = 'Gouraud'
          RadioItem = True
          OnClick = ClickOptionsMenuItem
        end
      end
      object miRenderEdgedFaces: TMenuItem
        Caption = 'Edged Faces'
        GroupIndex = 4
        OnClick = ClickOptionsMenuItem
      end
      object N7: TMenuItem
        Caption = '-'
        GroupIndex = 4
      end
      object miRenderHWTransform: TMenuItem
        Caption = 'HW Render'
        Checked = True
        GroupIndex = 4
        OnClick = ClickOptionsMenuItem
      end
      object N8: TMenuItem
        Caption = '-'
        GroupIndex = 4
      end
      object miRenderLinearFilter: TMenuItem
        Caption = 'Linear Filter'
        Checked = True
        GroupIndex = 4
        OnClick = ClickOptionsMenuItem
      end
      object miRenderWithTextures: TMenuItem
        Caption = 'Textures'
        Checked = True
        GroupIndex = 4
        OnClick = ClickOptionsMenuItem
      end
    end
    object N2: TMenuItem
      Caption = '-'
      GroupIndex = 4
    end
    object miDrawSafeRect: TMenuItem
      Caption = 'Draw Safe Rect'
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object miDrawGrid: TMenuItem
      Caption = 'Draw Grid'
      Checked = True
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object N1: TMenuItem
      Caption = '-'
      GroupIndex = 4
    end
    object miWeather: TMenuItem
      Caption = 'Weather'
      GroupIndex = 4
      object miWeatherNone: TMenuItem
        Tag = -1
        Caption = 'none'
        OnClick = miWeatherClick
      end
      object N6: TMenuItem
        Caption = '-'
      end
    end
    object miFog: TMenuItem
      Caption = 'Fog'
      Checked = True
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object N5: TMenuItem
      Caption = '-'
      GroupIndex = 4
    end
    object miMuteSounds: TMenuItem
      Caption = 'Mute Sounds'
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object miLightScene: TMenuItem
      Caption = 'Light Scene'
      Checked = True
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object miRealTime: TMenuItem
      Caption = 'Real Time'
      GroupIndex = 4
      OnClick = ClickOptionsMenuItem
    end
    object Device1: TMenuItem
      Caption = 'Device'
      GroupIndex = 4
      object TimeFactor1: TMenuItem
        Caption = 'Time Factor'
        object N0051: TMenuItem
          Tag = 5
          Caption = '0.05'
          RadioItem = True
          OnClick = N501Click
        end
        object N011: TMenuItem
          Tag = 10
          Caption = '0.1'
          RadioItem = True
          OnClick = N501Click
        end
        object N021: TMenuItem
          Tag = 20
          Caption = '0.2'
          RadioItem = True
          OnClick = N501Click
        end
        object N031: TMenuItem
          Tag = 30
          Caption = '0.3'
          RadioItem = True
          OnClick = N501Click
        end
        object N051: TMenuItem
          Tag = 50
          Caption = '0.5'
          RadioItem = True
          OnClick = N501Click
        end
        object N101: TMenuItem
          Tag = 100
          Caption = '1.0'
          Checked = True
          RadioItem = True
          OnClick = N501Click
        end
        object N301: TMenuItem
          Tag = 300
          Caption = '3.0'
          RadioItem = True
          OnClick = N501Click
        end
        object N501: TMenuItem
          Tag = 500
          Caption = '5.0'
          RadioItem = True
          OnClick = N501Click
        end
        object N1001: TMenuItem
          Tag = 1000
          Caption = '10.0'
          OnClick = N501Click
        end
      end
    end
  end
  object pmMacro: TMxPopupMenu
    Alignment = paCenter
    AutoHotkeys = maManual
    AutoPopup = False
    TrackButton = tbLeftButton
    OnPopup = pmOptionsPopup
    MarginStartColor = 10921638
    MarginEndColor = 2763306
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = 10526880
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 69
    Top = 24
  end
end
