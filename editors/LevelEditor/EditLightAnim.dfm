object frmEditLightAnim: TfrmEditLightAnim
  Left = -903
  Top = 271
  Width = 650
  Height = 258
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'Light Animation Library'
  Color = 10528425
  Constraints.MinHeight = 250
  Constraints.MinWidth = 650
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Scaled = False
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object paItemProps: TPanel
    Left = 191
    Top = 0
    Width = 443
    Height = 222
    Align = alClient
    BevelOuter = bvNone
    Color = 10528425
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    TabOrder = 0
    Visible = False
    object paColorAndControls: TPanel
      Left = 241
      Top = 0
      Width = 202
      Height = 222
      Align = alRight
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 0
      object paColor: TPanel
        Left = 0
        Top = 0
        Width = 202
        Height = 188
        Align = alClient
        BevelInner = bvLowered
        Color = 10526880
        TabOrder = 0
        object lbCurFrame: TMxLabel
          Left = 3
          Top = 3
          Width = 8
          Height = 13
          Caption = '7'
          Color = clWhite
          Font.Charset = DEFAULT_CHARSET
          Font.Color = 15790320
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentColor = False
          ParentFont = False
          ShadowColor = 5460819
          ShadowPos = spRightBottom
          Transparent = True
        end
        object lbAlpha: TMxLabel
          Left = 189
          Top = 3
          Width = 8
          Height = 13
          Alignment = taRightJustify
          Caption = '7'
          Color = clWhite
          Font.Charset = DEFAULT_CHARSET
          Font.Color = 15790320
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentColor = False
          ParentFont = False
          ShadowColor = 5460819
          ShadowPos = spRightBottom
          Transparent = True
        end
      end
      object Panel1: TPanel
        Left = 0
        Top = 188
        Width = 202
        Height = 34
        Align = alBottom
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 1
        object ebPrevKey: TExtBtn
          Left = 44
          Top = 18
          Width = 21
          Height = 15
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebPrevKeyClick
        end
        object ebFirstKey: TExtBtn
          Left = 23
          Top = 18
          Width = 21
          Height = 15
          Hint = 'First Key'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '|<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebFirstKeyClick
        end
        object ebNextKey: TExtBtn
          Left = 137
          Top = 18
          Width = 21
          Height = 15
          Hint = 'Next Key'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebNextKeyClick
        end
        object ebLastKey: TExtBtn
          Left = 158
          Top = 18
          Width = 21
          Height = 15
          Hint = 'Last Key'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '>>|'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebLastKeyClick
        end
        object ebMoveKeyLeft: TExtBtn
          Left = 1
          Top = 18
          Width = 21
          Height = 15
          Hint = 'Move Key Left'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '<-'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebMoveKeyLeftClick
        end
        object ebMoveKeyRight: TExtBtn
          Left = 180
          Top = 18
          Width = 21
          Height = 15
          Hint = 'Move Key Right'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '->'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebMoveKeyRightClick
        end
        object ebFirstFrame: TExtBtn
          Left = 1
          Top = 1
          Width = 21
          Height = 15
          Hint = 'First Frame'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '|<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebFirstFrameClick
        end
        object ebLastFrame: TExtBtn
          Left = 180
          Top = 1
          Width = 21
          Height = 15
          Hint = 'Last Frame'
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '>>|'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          ParentShowHint = False
          ShowHint = True
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebLastFrameClick
        end
        object ebDeleteKey: TExtBtn
          Left = 101
          Top = 18
          Width = 35
          Height = 15
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
          OnClick = ebDeleteKeyClick
        end
        object ebCreateKey: TExtBtn
          Left = 66
          Top = 18
          Width = 35
          Height = 15
          Align = alNone
          BevelShow = False
          BtnColor = 10528425
          Caption = '+'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebCreateKeyClick
        end
        object sePointer: TMultiObjSpinEdit
          Left = 66
          Top = 0
          Width = 70
          Height = 17
          Hint = 'Current Frame'
          LWSensitivity = 0.01
          BtnColor = 10526880
          ButtonKind = bkDiagonal
          ButtonWidth = 16
          MaxValue = 100000
          AutoSize = False
          Color = 12582911
          Ctl3D = False
          ParentCtl3D = False
          ParentShowHint = False
          ShowHint = True
          TabOrder = 0
          OnBottomClick = sePointerExit
          OnTopClick = sePointerExit
          OnChange = sePointerChange
          OnExit = sePointerExit
          OnKeyDown = sePointerKeyDown
        end
        object stStartFrame: TStaticText
          Left = 23
          Top = 1
          Width = 42
          Height = 15
          AutoSize = False
          BevelInner = bvNone
          BevelOuter = bvNone
          BorderStyle = sbsSingle
          Caption = '0'
          Color = 10526880
          ParentColor = False
          TabOrder = 1
        end
        object stEndFrame: TStaticText
          Left = 137
          Top = 1
          Width = 42
          Height = 15
          AutoSize = False
          BorderStyle = sbsSingle
          Caption = '0'
          Color = 10526880
          ParentColor = False
          TabOrder = 2
        end
      end
    end
    object paPropsGroup: TPanel
      Left = 0
      Top = 0
      Width = 241
      Height = 222
      Align = alClient
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 1
      object paProps: TPanel
        Left = 0
        Top = 0
        Width = 241
        Height = 187
        Align = alClient
        BevelOuter = bvLowered
        Color = 10528425
        TabOrder = 0
      end
      object wnShape: TD3DWindow
        Left = 0
        Top = 187
        Width = 241
        Height = 35
        FocusedColor = 15000804
        Align = alBottom
        BorderStyle = bsNone
        Color = 5592405
        TabOrder = 1
        OnKeyDown = wnShapeKeyDown
        object pbG: TPaintBox
          Left = 1
          Top = 1
          Width = 239
          Height = 33
          Align = alClient
          Color = clBlack
          ParentColor = False
          OnMouseDown = pbGMouseDown
          OnMouseMove = pbGMouseMove
          OnMouseUp = pbGMouseUp
          OnPaint = pbGPaint
        end
      end
    end
  end
  object paListAndButtons: TPanel
    Left = 0
    Top = 0
    Width = 191
    Height = 222
    Align = alLeft
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 1
    object Panel2: TPanel
      Left = 0
      Top = 203
      Width = 191
      Height = 19
      Align = alBottom
      BevelOuter = bvNone
      Color = 10528425
      TabOrder = 0
      object ebAddAnim: TExtBtn
        Left = 1
        Top = 1
        Width = 38
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 10528425
        Caption = 'Add'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebAddAnimClick
      end
      object ebDeleteAnim: TExtBtn
        Left = 77
        Top = 1
        Width = 38
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 10528425
        Caption = 'Delete'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebDeleteAnimClick
      end
      object ebSave: TExtBtn
        Left = 115
        Top = 1
        Width = 38
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 10528425
        Caption = 'Save'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebSaveClick
      end
      object ebReload: TExtBtn
        Left = 153
        Top = 1
        Width = 38
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 10528425
        Caption = 'Reload'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebReloadClick
      end
      object ebClone: TExtBtn
        Left = 39
        Top = 1
        Width = 38
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 10528425
        Caption = 'Clone'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebCloneClick
      end
    end
    object paItems: TPanel
      Left = 0
      Top = 0
      Width = 191
      Height = 203
      Align = alClient
      BevelOuter = bvLowered
      ParentColor = True
      TabOrder = 1
    end
  end
  object fsStorage: TFormStorage
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredValues = <
      item
        Name = 'EmitterDirX'
        Value = 0
      end
      item
        Name = 'EmitterDirY'
        Value = 0
      end
      item
        Name = 'EmitterDirZ'
        Value = 0
      end>
  end
end
