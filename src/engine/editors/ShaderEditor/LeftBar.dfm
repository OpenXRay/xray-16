object fraLeftBar: TfraLeftBar
  Left = 0
  Top = 0
  Width = 443
  Height = 270
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
    Width = 260
    Height = 270
    Align = alLeft
    BevelInner = bvLowered
    BevelOuter = bvNone
    Color = 10528425
    Constraints.MaxWidth = 260
    Constraints.MinWidth = 260
    TabOrder = 0
    object Splitter1: TSplitter
      Left = 1
      Top = 31
      Width = 258
      Height = 2
      Cursor = crVSplit
      Align = alBottom
      Color = clBlack
      ParentColor = False
    end
    object Splitter2: TSplitter
      Left = 1
      Top = 139
      Width = 258
      Height = 2
      Cursor = crVSplit
      Align = alTop
      Color = clBlack
      ParentColor = False
    end
    object paScene: TPanel
      Left = 1
      Top = 1
      Width = 258
      Height = 66
      Hint = 'Scene commands'
      Align = alTop
      Color = 10528425
      ParentShowHint = False
      ShowHint = True
      TabOrder = 0
      object APHeadLabel2: TLabel
        Left = 1
        Top = 1
        Width = 256
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Scene'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ebSceneMin: TExtBtn
        Left = 244
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
      object ebPreferences: TExtBtn
        Left = 2
        Top = 49
        Width = 255
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
      object ebImageCommands: TExtBtn
        Left = 2
        Top = 16
        Width = 255
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
        OnMouseDown = ebImageCommandsMouseDown
      end
      object ExtBtn10: TExtBtn
        Left = 2
        Top = 32
        Width = 255
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
        OnMouseDown = ExtBtn10MouseDown
      end
    end
    object paEngineShaders: TPanel
      Left = 1
      Top = 141
      Width = 258
      Height = 588
      Align = alClient
      Color = 10528425
      ParentShowHint = False
      ShowHint = True
      TabOrder = 1
      object Label1: TLabel
        Left = 1
        Top = 1
        Width = 256
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Tools'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object pcShaders: TElPageControl
        Left = 1
        Top = 14
        Width = 256
        Height = 77
        ActiveTabColor = 10528425
        BorderWidth = 0
        Color = 10528425
        DrawFocus = False
        Flat = True
        HotTrack = True
        InactiveTabColor = 10528425
        Multiline = False
        OnChange = pcShadersChange
        RaggedRight = False
        ScrollOpposite = False
        Style = etsNetTabs
        TabIndex = 4
        TabPosition = etpTop
        HotTrackFont.Charset = DEFAULT_CHARSET
        HotTrackFont.Color = 15790320
        HotTrackFont.Height = -11
        HotTrackFont.Name = 'MS Sans Serif'
        HotTrackFont.Style = []
        TabBkColor = 10528425
        ActivePage = tsSoundEnv
        FlatTabBorderColor = clBtnShadow
        Align = alTop
        ParentColor = False
        TabOrder = 0
        object tsEngine: TElTabSheet
          TabColor = 10528425
          PageControl = pcShaders
          ImageIndex = -1
          TabVisible = True
          Caption = 'Engine'
          Color = 10528425
          Visible = False
          object Bevel1: TBevel
            Left = 0
            Top = 50
            Width = 252
            Height = 2
            Align = alTop
          end
          object Panel1: TPanel
            Left = 0
            Top = 0
            Width = 252
            Height = 50
            Align = alTop
            BevelOuter = bvNone
            Color = 10528425
            TabOrder = 0
            object ebEngineShaderRemove: TExtBtn
              Left = 1
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Remove'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebRemoveItemClick
            end
            object ebEngineShaderClone: TExtBtn
              Left = 71
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Clone'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebCloneItemClick
            end
            object ebEngineShaderFile: TExtBtn
              Left = 1
              Top = 2
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'File'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
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
              Kind = knMinimize
              Margin = 3
              ParentFont = False
              Spacing = 3
              OnMouseDown = ebCustomFileMouseDown
            end
            object ebEngineShaderCreate: TExtBtn
              Left = 1
              Top = 18
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Create'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Kind = knMinimize
              Margin = 13
              ParentFont = False
              Spacing = 3
              OnClick = ebCreateItemClick
            end
          end
        end
        object tsCompiler: TElTabSheet
          TabColor = 10528425
          PageControl = pcShaders
          ImageIndex = -1
          TabVisible = True
          Caption = 'Compiler'
          Color = 10528425
          Visible = False
          object Bevel3: TBevel
            Left = 0
            Top = 50
            Width = 252
            Height = 2
            Align = alTop
          end
          object Panel4: TPanel
            Left = 0
            Top = 0
            Width = 252
            Height = 50
            Align = alTop
            BevelOuter = bvNone
            Color = 10528425
            TabOrder = 0
            object ebCompilerShaderRemove: TExtBtn
              Left = 1
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Remove'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebRemoveItemClick
            end
            object ebCompilerShaderClone: TExtBtn
              Left = 71
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Clone'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebCloneItemClick
            end
            object ExtBtn4: TExtBtn
              Left = 1
              Top = 2
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'File'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
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
              Kind = knMinimize
              Margin = 3
              ParentFont = False
              Spacing = 3
              OnMouseDown = ebCustomFileMouseDown
            end
            object ebCShaderCreate: TExtBtn
              Left = 1
              Top = 18
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Create'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Kind = knMinimize
              Margin = 13
              ParentFont = False
              Spacing = 3
              OnClick = ebCreateItemClick
            end
          end
        end
        object tsMaterial: TElTabSheet
          TabColor = 10528425
          PageControl = pcShaders
          ImageIndex = -1
          TabVisible = True
          Caption = 'Material'
          Color = 10528425
          Visible = False
          object Bevel2: TBevel
            Left = 0
            Top = 50
            Width = 252
            Height = 2
            Align = alTop
          end
          object Panel2: TPanel
            Left = 0
            Top = 0
            Width = 252
            Height = 50
            Align = alTop
            BevelOuter = bvNone
            Color = 10528425
            TabOrder = 0
            object ebMaterialRemove: TExtBtn
              Left = 1
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Remove'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebRemoveItemClick
            end
            object ebMaterialClone: TExtBtn
              Left = 71
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Clone'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebCloneItemClick
            end
            object ExtBtn3: TExtBtn
              Left = 1
              Top = 2
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'File'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
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
              Kind = knMinimize
              Margin = 3
              ParentFont = False
              Spacing = 3
              OnMouseDown = ebCustomFileMouseDown
            end
            object ebMaterialCreate: TExtBtn
              Left = 1
              Top = 18
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Create'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Kind = knMinimize
              Margin = 13
              ParentFont = False
              Spacing = 3
              OnClick = ebCreateItemClick
            end
          end
        end
        object tsMaterialPair: TElTabSheet
          PageControl = pcShaders
          ImageIndex = -1
          TabVisible = True
          Caption = 'Material Pair'
          ParentColor = True
          Visible = False
          object Bevel4: TBevel
            Left = 0
            Top = 50
            Width = 252
            Height = 2
            Align = alTop
          end
          object Panel3: TPanel
            Left = 0
            Top = 0
            Width = 252
            Height = 50
            Align = alTop
            BevelOuter = bvNone
            Color = 10528425
            TabOrder = 0
            object ExtBtn6: TExtBtn
              Left = 1
              Top = 2
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'File'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
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
              Kind = knMinimize
              Margin = 3
              ParentFont = False
              Spacing = 3
              OnMouseDown = ebCustomFileMouseDown
            end
          end
        end
        object tsSoundEnv: TElTabSheet
          TabColor = 10528425
          PageControl = pcShaders
          ImageIndex = -1
          TabVisible = True
          Caption = 'Sound Env'
          Color = 10528425
          object Bevel5: TBevel
            Left = 0
            Top = 50
            Width = 250
            Height = 2
            Align = alTop
          end
          object Panel5: TPanel
            Left = 0
            Top = 0
            Width = 250
            Height = 50
            Align = alTop
            BevelOuter = bvNone
            Color = 10528425
            TabOrder = 0
            object ExtBtn2: TExtBtn
              Left = 1
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Remove'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clBlack
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebRemoveItemClick
            end
            object ExtBtn7: TExtBtn
              Left = 71
              Top = 34
              Width = 70
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Clone'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clBlack
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Margin = 13
              ParentFont = False
              OnClick = ebCloneItemClick
            end
            object ExtBtn8: TExtBtn
              Left = 1
              Top = 2
              Width = 140
              Height = 15
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
              Kind = knMinimize
              Margin = 3
              ParentFont = False
              Spacing = 3
              OnMouseDown = ebCustomFileMouseDown
            end
            object ebCreateSoundEnv: TExtBtn
              Left = 1
              Top = 18
              Width = 140
              Height = 15
              Align = alNone
              BevelShow = False
              HotTrack = True
              HotColor = 15790320
              Caption = 'Create'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clBlack
              Font.Height = -11
              Font.Name = 'MS Sans Serif'
              Font.Style = []
              Kind = knMinimize
              Margin = 13
              ParentFont = False
              Spacing = 3
              OnClick = ebCreateItemClick
            end
          end
        end
      end
      object paItemList: TPanel
        Left = 1
        Top = 91
        Width = 256
        Height = 496
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 1
      end
    end
    object paShaderProperties: TPanel
      Left = 1
      Top = 33
      Width = 258
      Height = 236
      Align = alBottom
      Color = 10528425
      ParentShowHint = False
      ShowHint = False
      TabOrder = 2
      object Label6: TLabel
        Left = 1
        Top = 1
        Width = 256
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Item Properties'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ExtBtn5: TExtBtn
        Left = 244
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
      object Bevel6: TBevel
        Left = 1
        Top = 14
        Width = 256
        Height = 1
        Align = alTop
        Shape = bsLeftLine
      end
      object paShaderProps: TPanel
        Left = 1
        Top = 15
        Width = 256
        Height = 220
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
      end
    end
    object paPreview: TPanel
      Left = 1
      Top = 67
      Width = 258
      Height = 72
      Align = alTop
      Color = 10528425
      ParentShowHint = False
      ShowHint = False
      TabOrder = 3
      object Label2: TLabel
        Left = 1
        Top = 1
        Width = 256
        Height = 13
        Align = alTop
        Alignment = taCenter
        Caption = 'Preview'
        Color = clGray
        ParentColor = False
        OnClick = PanelMaximizeClick
      end
      object ExtBtn9: TExtBtn
        Left = 244
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
      object paPreviewProps: TPanel
        Left = 1
        Top = 14
        Width = 256
        Height = 57
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
      end
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Left Bar'
    Options = []
    RegistryRoot = prLocalMachine
    Version = 5
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredProps.Strings = (
      'paScene.Tag'
      'paScene.Height'
      'paEngineShaders.Height'
      'pcShaders.ActivePage'
      'paShaderProperties.Tag'
      'paShaderProperties.Height'
      'paPreview.Tag'
      'paPreview.Height')
    StoredValues = <>
    Left = 65529
    Top = 65526
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
    Left = 69
    Top = 18
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
  object pmCustomFile: TMxPopupMenu
    Alignment = paCenter
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
    Left = 117
    Top = 192
    object MenuItem1: TMenuItem
      Caption = 'Save'
      OnClick = ebSaveClick
    end
    object MenuItem2: TMenuItem
      Caption = 'Reload'
      OnClick = ebReloadClick
    end
    object MenuItem3: TMenuItem
      Caption = '-'
    end
    object MenuItem14: TMenuItem
      Caption = 'Import'
      Enabled = False
    end
    object MenuItem15: TMenuItem
      Caption = 'Export'
      Enabled = False
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
    Left = 181
    Top = 26
    object MenuItem4: TMenuItem
      Caption = '-'
    end
    object MenuItem5: TMenuItem
      Caption = 'Sound Editor'
      OnClick = MenuItem5Click
    end
    object MenuItem6: TMenuItem
      Caption = '-'
    end
    object MenuItem7: TMenuItem
      Caption = 'Synchronize Sounds'
      OnClick = MenuItem7Click
    end
  end
end
