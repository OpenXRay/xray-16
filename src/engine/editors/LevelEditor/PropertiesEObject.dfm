object frmPropertiesEObject: TfrmPropertiesEObject
  Left = 388
  Top = 383
  Width = 467
  Height = 363
  BiDiMode = bdRightToLeft
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'Object Properties'
  Color = 10528425
  Constraints.MinHeight = 363
  Constraints.MinWidth = 400
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  ParentBiDiMode = False
  Scaled = False
  OnDestroy = FormDestroy
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object ElPageControl1: TElPageControl
    Left = 0
    Top = 0
    Width = 459
    Height = 329
    ActiveTabColor = 10528425
    BorderWidth = 0
    Color = 10528425
    DrawFocus = False
    Flat = True
    HotTrack = False
    InactiveTabColor = 10528425
    Multiline = False
    RaggedRight = False
    ScrollOpposite = False
    Style = etsAngledTabs
    TabHeight = 15
    TabIndex = 1
    TabPosition = etpBottom
    HotTrackFont.Charset = DEFAULT_CHARSET
    HotTrackFont.Color = clBlue
    HotTrackFont.Height = -11
    HotTrackFont.Name = 'MS Sans Serif'
    HotTrackFont.Style = []
    TabBkColor = 10528425
    ActivePage = tsSurfaces
    FlatTabBorderColor = clBtnShadow
    Align = alClient
    ParentColor = False
    TabOrder = 0
    UseXPThemes = False
    object tsBasic: TElTabSheet
      PageControl = ElPageControl1
      ImageIndex = -1
      TabVisible = True
      Caption = 'Main Options'
      Color = 10528425
      Visible = False
      object paBasic: TPanel
        Left = 0
        Top = 0
        Width = 455
        Height = 317
        Align = alClient
        BevelOuter = bvLowered
        Color = 10528425
        TabOrder = 0
      end
    end
    object tsSurfaces: TElTabSheet
      PageControl = ElPageControl1
      ImageIndex = -1
      TabVisible = True
      Caption = 'Surfaces'
      Color = 10528425
      object paSurfaces: TPanel
        Left = 0
        Top = 19
        Width = 319
        Height = 291
        Align = alClient
        BevelOuter = bvLowered
        Color = 10528425
        TabOrder = 0
      end
      object Panel2: TPanel
        Left = 319
        Top = 19
        Width = 136
        Height = 291
        Align = alRight
        BevelOuter = bvNone
        ParentColor = True
        TabOrder = 1
        object gbTexture: TGroupBox
          Left = 0
          Top = 0
          Width = 136
          Height = 291
          Align = alClient
          Caption = ' Texture Details '
          TabOrder = 0
          object RxLabel7: TLabel
            Left = 4
            Top = 15
            Width = 31
            Height = 13
            Caption = 'Width:'
          end
          object RxLabel8: TLabel
            Left = 4
            Top = 30
            Width = 34
            Height = 13
            Caption = 'Height:'
          end
          object RxLabel9: TLabel
            Left = 4
            Top = 45
            Width = 30
            Height = 13
            Caption = 'Alpha:'
          end
          object lbWidth: TLabel
            Left = 72
            Top = 15
            Width = 13
            Height = 13
            Caption = '...'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'MS Sans Serif'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object lbHeight: TLabel
            Left = 72
            Top = 30
            Width = 13
            Height = 13
            Caption = '...'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'MS Sans Serif'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object lbAlpha: TLabel
            Left = 72
            Top = 45
            Width = 13
            Height = 13
            Caption = '...'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'MS Sans Serif'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object paImage: TMxPanel
            Left = 2
            Top = 60
            Width = 132
            Height = 132
            BevelOuter = bvLowered
            Caption = '<no image>'
            ParentColor = True
            TabOrder = 0
            OnPaint = paImagePaint
          end
        end
      end
      object Panel1: TPanel
        Left = 0
        Top = 0
        Width = 455
        Height = 19
        Align = alTop
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 2
        object ebSortByImage: TExtBtn
          Left = 164
          Top = 1
          Width = 45
          Height = 17
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          GroupIndex = 1
          Caption = 'Texture'
          Enabled = False
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          FlatAlwaysEdge = True
        end
        object ebSortByName: TExtBtn
          Left = 119
          Top = 1
          Width = 45
          Height = 17
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          GroupIndex = 1
          Down = True
          Caption = 'Name'
          Enabled = False
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          FlatAlwaysEdge = True
        end
        object ebDropper: TExtBtn
          Left = 2
          Top = 1
          Width = 63
          Height = 17
          Align = alNone
          AllowAllUp = True
          BevelShow = False
          BtnColor = 10528425
          GroupIndex = 2
          Caption = 'Dropper'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Glyph.Data = {
            3E020000424D3E0200000000000036000000280000000D0000000D0000000100
            18000000000008020000120B0000120B00000000000000000000A0A6A9A0A6A9
            A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6
            A900A0A6A9A0A6A9000000A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9
            A0A6A9A0A6A9A0A6A900A0A6A9000000F7F7F7000000000000A0A6A9A0A6A9A0
            A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A900A0A6A9A0A6A9000000F7F7F7F7F7
            F7000000A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A900A0A6A9A0A6A9
            000000F7F7F7F7F7F7F7F7F7000000A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6
            A900A0A6A9A0A6A9A0A6A9000000F7F7F7F7F7F7F7F7F7000000A0A6A9000000
            A0A6A9A0A6A9A0A6A900A0A6A9A0A6A9A0A6A9A0A6A9000000F7F7F7F7F7F7F7
            F7F7000000000000A0A6A9A0A6A9A0A6A900A0A6A9A0A6A9A0A6A9A0A6A9A0A6
            A9000000F7F7F7000000000000000000000000A0A6A9A0A6A900A0A6A9A0A6A9
            A0A6A9A0A6A9A0A6A9A0A6A9000000000000000000000000000000A0A6A9A0A6
            A900A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9000000000000000000000000000000
            000000000000A0A6A900A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A900
            0000000000000000000000A0A6A9A0A6A900A0A6A9A0A6A9A0A6A9A0A6A9A0A6
            A9A0A6A9A0A6A9A0A6A9A0A6A9000000A0A6A9A0A6A9A0A6A900A0A6A9A0A6A9
            A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6A9A0A6
            A900}
          Margin = 2
          ParentFont = False
          FlatAlwaysEdge = True
        end
        object ebSortByShader: TExtBtn
          Left = 209
          Top = 1
          Width = 45
          Height = 17
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          GroupIndex = 1
          Caption = 'Shader'
          Enabled = False
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          FlatAlwaysEdge = True
        end
        object Label1: TLabel
          Left = 79
          Top = 3
          Width = 37
          Height = 13
          Caption = 'Sort By:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
      end
    end
  end
  object fsStorage: TFormStorage
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredProps.Strings = (
      'ElPageControl1.ActivePage')
    StoredValues = <>
    Left = 8
    Top = 24
  end
end
