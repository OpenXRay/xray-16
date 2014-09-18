object DB_packer: TDB_packer
  Left = 467
  Top = 124
  Width = 687
  Height = 604
  Caption = 'DB_packer'
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
    Width = 679
    Height = 470
    Align = alClient
    BevelOuter = bvNone
    Color = 10528425
    TabOrder = 0
    object Panel5: TPanel
      Left = 382
      Top = 0
      Width = 297
      Height = 470
      Align = alRight
      BevelInner = bvLowered
      BevelOuter = bvSpace
      Color = 10528425
      TabOrder = 0
      object shellTree: TElShellTree
        Left = 2
        Top = 2
        Width = 293
        Height = 466
        Cursor = crDefault
        LeftPosition = 0
        RootFolder = sfoDesktop
        UseSystemMenus = False
        ClearOnCollapse = True
        CheckForChildren = False
        ShowFiles = True
        HighlightCompressed = True
        FileSystemOnly = False
        SortModifiers = [ssmFoldersFirst, ssmExecutablesFirst]
        SizeFormat = ssfAsIs
        DefaultColumns = False
        Align = alClient
        DefaultSectionWidth = 120
        BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
        DragCursor = crDrag
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        GradientSteps = 64
        HeaderHeight = 19
        HeaderHotTrack = False
        HeaderInvertSortArrows = True
        HeaderSections.Data = {F4FFFFFF00000000}
        HeaderFont.Charset = DEFAULT_CHARSET
        HeaderFont.Color = clWindowText
        HeaderFont.Height = -11
        HeaderFont.Name = 'MS Sans Serif'
        HeaderFont.Style = []
        HorzScrollBarStyles.ShowTrackHint = False
        HorzScrollBarStyles.Width = 17
        HorzScrollBarStyles.ButtonSize = 17
        IncrementalSearch = False
        LineBorderActiveColor = clBlack
        LineBorderInactiveColor = clBlack
        LineHeight = 17
        LinesColor = clBtnShadow
        MouseFrameSelect = True
        MultiSelect = False
        OwnerDrawMask = '~~@~~'
        ScrollbarOpposite = False
        ShowLeafButton = False
        SortMode = smAddClick
        StoragePath = '\Tree'
        VertScrollBarStyles.ShowTrackHint = True
        VertScrollBarStyles.Width = 17
        VertScrollBarStyles.ButtonSize = 8
        TextColor = clBtnText
        BkColor = clBtnFace
        DockOrientation = doNoOrient
        TabOrder = 0
        TabStop = True
      end
    end
    object Panel6: TPanel
      Left = 0
      Top = 0
      Width = 382
      Height = 470
      Align = alClient
      BevelInner = bvLowered
      BevelOuter = bvSpace
      Color = 10528425
      TabOrder = 1
      object Splitter1: TSplitter
        Left = 2
        Top = 233
        Width = 378
        Height = 2
        Cursor = crVSplit
        Align = alTop
      end
      object Panel3: TPanel
        Left = 2
        Top = 235
        Width = 378
        Height = 233
        Align = alClient
        BevelInner = bvLowered
        BevelOuter = bvSpace
        Color = 10528425
        TabOrder = 0
        DesignSize = (
          378
          233)
        object MxLabel2: TMxLabel
          Left = 8
          Top = 4
          Width = 58
          Height = 13
          Caption = 'Include files'
        end
        object ExtBtn3: TExtBtn
          Left = 336
          Top = 26
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn3Click
        end
        object ExtBtn4: TExtBtn
          Left = 336
          Top = 42
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '>>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn4Click
        end
        object lbIncludeFiles: TElListBox
          Left = 8
          Top = 24
          Width = 321
          Height = 201
          AllowGrayed = False
          ItemHeight = 13
          MultiSelect = True
          ItemIndex = 0
          TopIndex = 0
          BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
          HorizontalScroll = False
          LineBorderActiveColor = clBlack
          LineBorderInactiveColor = clBlack
          SelectedFont.Charset = DEFAULT_CHARSET
          SelectedFont.Color = clHighlightText
          SelectedFont.Height = -11
          SelectedFont.Name = 'MS Sans Serif'
          SelectedFont.Style = []
          Anchors = [akLeft, akTop, akRight, akBottom]
          TabOrder = 0
        end
      end
      object Panel4: TPanel
        Left = 2
        Top = 2
        Width = 378
        Height = 231
        Align = alTop
        BevelInner = bvLowered
        BevelOuter = bvSpace
        Color = 10528425
        TabOrder = 1
        DesignSize = (
          378
          231)
        object MxLabel1: TMxLabel
          Left = 8
          Top = 4
          Width = 71
          Height = 13
          Caption = 'Include folders'
        end
        object ExtBtn1: TExtBtn
          Left = 336
          Top = 42
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn1Click
        end
        object ExtBtn2: TExtBtn
          Left = 336
          Top = 58
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '>>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn2Click
        end
        object lbIncludeFolders: TElListBox
          Left = 8
          Top = 24
          Width = 321
          Height = 199
          AllowGrayed = False
          ItemHeight = 13
          MultiSelect = True
          ItemIndex = 0
          TopIndex = 0
          BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
          HorizontalScroll = False
          LineBorderActiveColor = clBlack
          LineBorderInactiveColor = clBlack
          SelectedFont.Charset = DEFAULT_CHARSET
          SelectedFont.Color = clHighlightText
          SelectedFont.Height = -11
          SelectedFont.Name = 'MS Sans Serif'
          SelectedFont.Style = []
          Anchors = [akLeft, akTop, akRight, akBottom]
          TabOrder = 0
        end
      end
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 470
    Width = 679
    Height = 100
    Align = alBottom
    BevelInner = bvLowered
    BevelOuter = bvSpace
    Color = 10528425
    TabOrder = 1
    object btnLoad: TExtBtn
      Left = 0
      Top = 2
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Load'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = btnLoadClick
    end
    object btnSave: TExtBtn
      Left = 0
      Top = 18
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Save'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = btnSaveClick
    end
    object ExtBtn5: TExtBtn
      Left = 0
      Top = 42
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Start'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ExtBtn5Click
    end
  end
end
