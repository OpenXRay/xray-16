object fraLeftBar: TfraLeftBar
  Left = 0
  Top = 0
  Width = 421
  Height = 386
  HorzScrollBar.Visible = False
  VertScrollBar.Increment = 34
  VertScrollBar.Size = 13
  VertScrollBar.Style = ssHotTrack
  VertScrollBar.Tracking = True
  Align = alClient
  Color = 10528425
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clBlack
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  ParentColor = False
  ParentFont = False
  TabOrder = 0
  object paLeftBar: TPanel
    Left = 0
    Top = 0
    Width = 400
    Height = 386
    Align = alLeft
    BevelInner = bvLowered
    BevelOuter = bvNone
    Color = 10528425
    Constraints.MaxWidth = 400
    Constraints.MinWidth = 400
    TabOrder = 0
    object Splitter1: TSplitter
      Left = 1
      Top = 242
      Width = 398
      Height = 2
      Cursor = crVSplit
      Align = alBottom
      Color = clBlack
      ParentColor = False
    end
    object paScene: TPanel
      Left = 1
      Top = 1
      Width = 398
      Height = 97
      Hint = 'Scene commands'
      Align = alTop
      Color = 10528425
      ParentShowHint = False
      ShowHint = True
      TabOrder = 0
      object APHeadLabel2: TLabel
        Left = 1
        Top = 1
        Width = 396
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Scene'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ebSceneMin: TExtBtn
        Left = 286
        Top = 2
        Width = 11
        Height = 11
        Align = alNone
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
        OnClick = PanelMimimizeClick
      end
      object ebSceneFile: TExtBtn
        Left = 3
        Top = 16
        Width = 295
        Height = 12
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        Caption = 'File'
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
        Margin = 3
        ParentFont = False
        Spacing = 3
        OnMouseDown = ebSceneFileMouseDown
      end
      object ebPreferences: TExtBtn
        Left = 2
        Top = 77
        Width = 295
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        Caption = 'Preferences'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Margin = 13
        ParentFont = False
        OnClick = ebEditorPreferencesClick
      end
      object ebPreviewObjectClick: TExtBtn
        Left = 2
        Top = 30
        Width = 295
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        Caption = 'Preview Object'
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
        Margin = 3
        ParentFont = False
        Spacing = 3
        OnMouseDown = ebPreviewObjectClickMouseDown
      end
      object ebSceneCommands1: TExtBtn
        Left = 2
        Top = 45
        Width = 295
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Caption = 'Images'
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
        Margin = 3
        ParentFont = False
        Spacing = 3
        OnMouseDown = ebSceneCommands1MouseDown
      end
      object ExtBtn3: TExtBtn
        Left = 2
        Top = 61
        Width = 295
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Caption = 'Sounds'
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
        Margin = 3
        ParentFont = False
        Spacing = 3
        OnMouseDown = ExtBtn3MouseDown
      end
    end
    object paModel: TPanel
      Left = 1
      Top = 98
      Width = 398
      Height = 49
      Hint = 'Scene commands'
      Align = alTop
      Color = 10528425
      ParentShowHint = False
      ShowHint = True
      TabOrder = 1
      object Label4: TLabel
        Left = 1
        Top = 1
        Width = 396
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Model'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ExtBtn2: TExtBtn
        Left = 286
        Top = 2
        Width = 11
        Height = 11
        Align = alNone
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
        OnClick = PanelMimimizeClick
      end
      object ebRenderEditorStyle: TExtBtn
        Left = 84
        Top = 31
        Width = 60
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        GroupIndex = 1
        Down = True
        Caption = 'Editor'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          6E040000424D6E04000000000000360000002800000028000000090000000100
          18000000000038040000120B0000120B00000000000000000000FF00FF000000
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF000000C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF000000C4C4C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FFFF00FFFF00FFFF00FF
          FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FF0000009999999999999999
          99999999999999999999C4C4C4FF00FFFF00FF00000099999999999999999999
          9999999999999999C4C4C4FF00FFFF00FF000000999999999999999999999999
          999999999999C4C4C4FF00FFC4C4C40000000000000000000000000000000000
          00000000FF00FFFF00FFFF00FF00000099999999999999999999999999999999
          9999C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999
          C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999C4C4
          C4FF00FFC4C4C4999999999999999999999999999999999999000000FF00FFFF
          00FFFF00FF000000999999999999999999999999999999999999C4C4C4FF00FF
          FF00FF000000999999999999999999999999999999999999C4C4C4FF00FFFF00
          FF000000999999999999999999999999999999999999C4C4C4FF00FFC4C4C499
          9999999999999999999999999999999999000000FF00FFFF00FFFF00FF000000
          999999999999999999999999999999999999C4C4C4FF00FFFF00FF0000009999
          99999999999999999999999999999999C4C4C4FF00FFFF00FF00000099999999
          9999999999999999999999999999C4C4C4FF00FFC4C4C4999999999999999999
          999999999999999999000000FF00FFFF00FFFF00FF0000009999999999999999
          99999999999999999999C4C4C4FF00FFFF00FF00000099999999999999999999
          9999999999999999C4C4C4FF00FFFF00FF000000999999999999999999999999
          999999999999C4C4C4FF00FFC4C4C49999999999999999999999999999999999
          99000000FF00FFFF00FFFF00FF00000099999999999999999999999999999999
          9999C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999
          C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999C4C4
          C4FF00FFC4C4C4999999999999999999999999999999999999000000FF00FFFF
          00FFFF00FF000000000000000000000000000000000000000000000000FF00FF
          FF00FF000000000000000000000000000000000000000000000000FF00FFFF00
          FF000000000000000000000000000000000000000000000000FF00FFC4C4C499
          9999999999999999999999999999999999000000FF00FFFF00FFFF00FFFF00FF
          FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00
          FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF
          00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFC4C4C4C4C4C4C4C4C4C4C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF}
        Margin = 3
        NumGlyphs = 4
        ParentFont = False
        Spacing = 3
        OnClick = ebRenderStyleClick
      end
      object ebRenderEngineStyle: TExtBtn
        Left = 144
        Top = 31
        Width = 60
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        GroupIndex = 1
        Caption = 'Engine'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          6E040000424D6E04000000000000360000002800000028000000090000000100
          18000000000038040000120B0000120B00000000000000000000FF00FF000000
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF000000C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF000000C4C4C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FFFF00FFFF00FFFF00FF
          FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FF0000009999999999999999
          99999999999999999999C4C4C4FF00FFFF00FF00000099999999999999999999
          9999999999999999C4C4C4FF00FFFF00FF000000999999999999999999999999
          999999999999C4C4C4FF00FFC4C4C40000000000000000000000000000000000
          00000000FF00FFFF00FFFF00FF00000099999999999999999999999999999999
          9999C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999
          C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999C4C4
          C4FF00FFC4C4C4999999999999999999999999999999999999000000FF00FFFF
          00FFFF00FF000000999999999999999999999999999999999999C4C4C4FF00FF
          FF00FF000000999999999999999999999999999999999999C4C4C4FF00FFFF00
          FF000000999999999999999999999999999999999999C4C4C4FF00FFC4C4C499
          9999999999999999999999999999999999000000FF00FFFF00FFFF00FF000000
          999999999999999999999999999999999999C4C4C4FF00FFFF00FF0000009999
          99999999999999999999999999999999C4C4C4FF00FFFF00FF00000099999999
          9999999999999999999999999999C4C4C4FF00FFC4C4C4999999999999999999
          999999999999999999000000FF00FFFF00FFFF00FF0000009999999999999999
          99999999999999999999C4C4C4FF00FFFF00FF00000099999999999999999999
          9999999999999999C4C4C4FF00FFFF00FF000000999999999999999999999999
          999999999999C4C4C4FF00FFC4C4C49999999999999999999999999999999999
          99000000FF00FFFF00FFFF00FF00000099999999999999999999999999999999
          9999C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999
          C4C4C4FF00FFFF00FF000000999999999999999999999999999999999999C4C4
          C4FF00FFC4C4C4999999999999999999999999999999999999000000FF00FFFF
          00FFFF00FF000000000000000000000000000000000000000000000000FF00FF
          FF00FF000000000000000000000000000000000000000000000000FF00FFFF00
          FF000000000000000000000000000000000000000000000000FF00FFC4C4C499
          9999999999999999999999999999999999000000FF00FFFF00FFFF00FFFF00FF
          FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00
          FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF
          00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFC4C4C4C4C4C4C4C4C4C4C4C4
          C4C4C4C4C4C4C4C4C4C4C4C4FF00FFFF00FF}
        Margin = 3
        NumGlyphs = 4
        ParentFont = False
        Spacing = 3
        OnClick = ebRenderStyleClick
      end
      object Label5: TLabel
        Left = 16
        Top = 32
        Width = 64
        Height = 13
        Caption = 'Render Style:'
      end
      object ebBonePart: TExtBtn
        Left = 2
        Top = 16
        Width = 87
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        Caption = 'Bone Parts'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Kind = knMinimize
        Margin = 13
        ParentFont = False
        OnClick = ebBonePartClick
      end
      object ExtBtn1: TExtBtn
        Left = 216
        Top = 31
        Width = 77
        Height = 15
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        Caption = 'Clip Maker'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Margin = 3
        NumGlyphs = 4
        ParentFont = False
        Spacing = 3
        OnClick = ExtBtn1Click
      end
    end
    object paObjectProperties: TPanel
      Left = 1
      Top = 147
      Width = 398
      Height = 95
      Align = alClient
      Color = 10528425
      ParentShowHint = False
      ShowHint = False
      TabOrder = 2
      object Label6: TLabel
        Left = 1
        Top = 1
        Width = 396
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Object Items'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object Bevel6: TBevel
        Left = 1
        Top = 14
        Width = 396
        Height = 1
        Align = alTop
        Shape = bsLeftLine
      end
      object paObjectProps: TPanel
        Left = 1
        Top = 15
        Width = 396
        Height = 79
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
      end
    end
    object paCurrentMotion: TPanel
      Left = 1
      Top = 244
      Width = 398
      Height = 141
      Align = alBottom
      Color = 10528425
      ParentShowHint = False
      ShowHint = True
      TabOrder = 3
      object Label1: TLabel
        Left = 1
        Top = 1
        Width = 396
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Item Properties'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ExtBtn10: TExtBtn
        Left = 286
        Top = 2
        Width = 11
        Height = 11
        Align = alNone
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
        OnClick = PanelMimimizeClick
      end
      object paPSList: TPanel
        Left = 1
        Top = 14
        Width = 396
        Height = 126
        Align = alClient
        BevelOuter = bvNone
        TabOrder = 0
        object Bevel1: TBevel
          Left = 0
          Top = 0
          Width = 396
          Height = 1
          Align = alTop
          Shape = bsLeftLine
        end
        object paItemProps: TPanel
          Left = 0
          Top = 1
          Width = 396
          Height = 125
          Align = alClient
          BevelOuter = bvNone
          Color = 10528425
          TabOrder = 0
        end
      end
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Left Bar'
    Options = []
    RegistryRoot = prLocalMachine
    Version = 14
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredProps.Strings = (
      'paCurrentMotion.Tag'
      'paCurrentMotion.Height'
      'paScene.Tag'
      'paScene.Height'
      'paModel.Tag'
      'paModel.Height')
    StoredValues = <>
    Left = 65529
    Top = 65526
  end
  object pmSceneFile: TMxPopupMenu
    Alignment = paCenter
    AutoHotkeys = maManual
    AutoPopup = False
    TrackButton = tbLeftButton
    MarginStartColor = 13158600
    MarginEndColor = 1644825
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = 10526880
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 181
    Top = 24
    object N7: TMenuItem
      Caption = '-'
    end
    object Clear1: TMenuItem
      Caption = 'Clear'
      OnClick = Clear1Click
    end
    object Load1: TMenuItem
      Caption = 'Load...'
      OnClick = Load1Click
    end
    object ebSave: TMenuItem
      Caption = 'Save'
      OnClick = Save2Click
    end
    object ebSaveAs: TMenuItem
      Caption = 'Save As...'
      OnClick = ebSaveAsClick
    end
    object N10: TMenuItem
      Caption = '-'
    end
    object ebMakeThumbnail: TMenuItem
      Caption = 'Make Thumbnail'
      OnClick = ebMakeThumbnailClick
    end
    object N5: TMenuItem
      Caption = '-'
    end
    object miRecentFiles: TMenuItem
      Caption = 'Open Recent'
      Enabled = False
    end
    object N2: TMenuItem
      Caption = '-'
    end
    object Import1: TMenuItem
      Caption = 'Import...'
      OnClick = Import1Click
    end
    object N11: TMenuItem
      Caption = '-'
    end
    object ebOptimizeMotions: TMenuItem
      Caption = 'Optimize Motions'
      OnClick = ebOptimizeMotionsClick
    end
    object N4: TMenuItem
      Caption = '-'
    end
    object ebExportBatch: TMenuItem
      Caption = 'Batch Convert...'
      OnClick = ebExportBatchClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object ebExport: TMenuItem
      Caption = 'Export'
      object miExportOGF: TMenuItem
        Caption = 'Export OGF...'
        OnClick = miExportOGFClick
      end
      object miExportOMF: TMenuItem
        Caption = 'Export OMF...'
        OnClick = miExportOMFClick
      end
      object ExportWaveFrontOBJ1: TMenuItem
        Caption = 'Export OBJ...'
        OnClick = ExportWaveFrontOBJ1Click
      end
      object ExportDM1: TMenuItem
        Caption = 'Export DM...'
        OnClick = ExportDM1Click
      end
      object ExportC1: TMenuItem
        Caption = 'Export C++...'
        OnClick = ExportC1Click
      end
    end
    object N12: TMenuItem
      Caption = '-'
    end
    object Quit1: TMenuItem
      Caption = 'Quit'
      OnClick = Quit1Click
    end
  end
  object pmPreviewObject: TMxPopupMenu
    Alignment = paCenter
    AutoHotkeys = maManual
    AutoPopup = False
    TrackButton = tbLeftButton
    MarginStartColor = 13158600
    MarginEndColor = 1644825
    BKColor = 10528425
    SelColor = clBlack
    SelFontColor = 10526880
    SepHColor = 1644825
    SepLColor = 13158600
    LeftMargin = 10
    Style = msOwnerDraw
    Left = 181
    Top = 47
    object N8: TMenuItem
      Caption = '-'
    end
    object Custom1: TMenuItem
      Caption = 'Custom...'
      OnClick = Custom1Click
    end
    object none1: TMenuItem
      Caption = 'Clear'
      OnClick = none1Click
    end
    object N3: TMenuItem
      Caption = '-'
    end
    object Preferences1: TMenuItem
      Caption = 'Preferences'
      OnClick = Preferences1Click
    end
  end
  object pmImages: TMxPopupMenu
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
    Left = 229
    Top = 58
    object N9: TMenuItem
      Caption = '-'
    end
    object ImageEditor1: TMenuItem
      Caption = 'Image Editor'
      OnClick = ImageEditor1Click
    end
    object N6: TMenuItem
      Caption = '-'
    end
    object Refresh1: TMenuItem
      Caption = 'Synchronize Textures'
      OnClick = Refresh1Click
    end
    object Checknewtextures1: TMenuItem
      Caption = 'Check New Textures'
      OnClick = Checknewtextures1Click
    end
  end
  object pmSounds: TMxPopupMenu
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
    Left = 261
    Top = 58
    object MenuItem1: TMenuItem
      Caption = '-'
    end
    object MenuItem2: TMenuItem
      Caption = 'Sound Editor'
      OnClick = MenuItem2Click
    end
    object MenuItem3: TMenuItem
      Caption = '-'
    end
    object MenuItem4: TMenuItem
      Caption = 'Synchronize Sounds'
      OnClick = MenuItem4Click
    end
  end
end
