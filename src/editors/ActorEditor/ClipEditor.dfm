object ClipMaker: TClipMaker
  Left = 381
  Top = 254
  Width = 766
  Height = 534
  HorzScrollBar.Style = ssFlat
  HorzScrollBar.ThumbSize = 12
  BiDiMode = bdRightToLeft
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'Clip Maker'
  Color = 6908265
  Constraints.MinHeight = 256
  Constraints.MinWidth = 500
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
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object paB: TPanel
    Left = 129
    Top = 0
    Width = 629
    Height = 500
    Align = alClient
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 0
    object paBase: TPanel
      Left = 0
      Top = 0
      Width = 629
      Height = 137
      Align = alTop
      BevelOuter = bvNone
      Color = 6908265
      TabOrder = 0
      object sbBase: TScrollBox
        Left = 0
        Top = 0
        Width = 629
        Height = 137
        HorzScrollBar.Style = ssFlat
        HorzScrollBar.Tracking = True
        VertScrollBar.Visible = False
        Align = alClient
        BorderStyle = bsNone
        TabOrder = 0
        object paFrame: TMxPanel
          Left = 0
          Top = 0
          Width = 137
          Height = 137
          Align = alLeft
          BevelOuter = bvNone
          ParentColor = True
          TabOrder = 0
          object Bevel6: TBevel
            Left = 0
            Top = 82
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel7: TBevel
            Left = 0
            Top = 50
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel8: TBevel
            Left = 0
            Top = 66
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel1: TBevel
            Left = 0
            Top = 18
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel3: TBevel
            Left = 0
            Top = 34
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel18: TBevel
            Left = 0
            Top = 118
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object Bevel21: TBevel
            Left = 0
            Top = 98
            Width = 137
            Height = 1
            Align = alTop
            Shape = bsBottomLine
            Style = bsRaised
          end
          object paClips: TMxPanel
            Tag = -1
            Left = 0
            Top = 0
            Width = 137
            Height = 18
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 0
            OnDragDrop = ClipDragDrop
            OnDragOver = ClipDragOver
            OnEndDrag = ClipEndDrag
            OnMouseDown = ClipMouseDown
            OnMouseMove = ClipMouseMove
            OnMouseUp = ClipMouseUp
            OnStartDrag = ClipStartDrag
            OnPaint = ClipPaint
            object Bevel9: TBevel
              Left = 0
              Top = 0
              Width = 137
              Height = 1
              Align = alTop
              Shape = bsBottomLine
              Style = bsRaised
            end
          end
          object gtClip: TMxPanel
            Left = 0
            Top = 99
            Width = 137
            Height = 19
            Align = alTop
            BevelOuter = bvNone
            Caption = ' '
            Color = 5460819
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'MS Sans Serif'
            Font.Style = [fsBold]
            ParentFont = False
            ParentShowHint = False
            ShowHint = False
            TabOrder = 1
            OnPaint = gtClipPaint
          end
          object paBP3: TMxPanel
            Tag = 3
            Left = 0
            Top = 67
            Width = 137
            Height = 15
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 2
            OnDragDrop = BPDragDrop
            OnDragOver = BPDragOver
            OnEndDrag = BPEndDrag
            OnMouseDown = BPMouseDown
            OnMouseUp = BPMouseUp
            OnStartDrag = BPStartDrag
            OnPaint = BPOnPaint
          end
          object paBP2: TMxPanel
            Tag = 2
            Left = 0
            Top = 51
            Width = 137
            Height = 15
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 3
            OnDragDrop = BPDragDrop
            OnDragOver = BPDragOver
            OnEndDrag = BPEndDrag
            OnMouseDown = BPMouseDown
            OnMouseUp = BPMouseUp
            OnStartDrag = BPStartDrag
            OnPaint = BPOnPaint
          end
          object paBP1: TMxPanel
            Tag = 1
            Left = 0
            Top = 35
            Width = 137
            Height = 15
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 4
            OnDragDrop = BPDragDrop
            OnDragOver = BPDragOver
            OnEndDrag = BPEndDrag
            OnMouseDown = BPMouseDown
            OnMouseUp = BPMouseUp
            OnStartDrag = BPStartDrag
            OnPaint = BPOnPaint
          end
          object paBP0: TMxPanel
            Left = 0
            Top = 19
            Width = 137
            Height = 15
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 5
            OnDragDrop = BPDragDrop
            OnDragOver = BPDragOver
            OnEndDrag = BPEndDrag
            OnMouseDown = BPMouseDown
            OnMouseUp = BPMouseUp
            OnStartDrag = BPStartDrag
            OnPaint = BPOnPaint
          end
          object paFXs: TMxPanel
            Tag = -2
            Left = 0
            Top = 83
            Width = 137
            Height = 15
            Align = alTop
            BevelOuter = bvNone
            Color = 6316128
            TabOrder = 6
            OnDragDrop = BPDragDrop
            OnDragOver = BPDragOver
            OnEndDrag = BPEndDrag
            OnMouseDown = BPMouseDown
            OnMouseUp = BPMouseUp
            OnStartDrag = BPStartDrag
            DesignSize = (
              137
              15)
            object timeTrackBar: TElTrackBar
              Left = 0
              Top = 8
              Width = 137
              Height = 7
              Page = 0
              OffsetLeft = 5
              OffsetRight = 5
              TickWidth = 0
              SelEnd = 0
              TrackWidth = 3
              SelectionMarkSize = 2
              Anchors = [akLeft, akTop, akRight, akBottom]
              Color = 6908265
              ParentColor = False
              TabOrder = 0
              ActiveBorderType = fbtFlat
              InactiveBorderType = fbtFlat
            end
          end
        end
      end
    end
    object paClipProps: TPanel
      Left = 0
      Top = 137
      Width = 629
      Height = 363
      Align = alClient
      BevelOuter = bvNone
      Color = 6908265
      TabOrder = 1
      object Bevel20: TBevel
        Left = 628
        Top = 1
        Width = 1
        Height = 362
        Align = alRight
        Shape = bsRightLine
        Style = bsRaised
      end
      object Bevel15: TBevel
        Left = 0
        Top = 0
        Width = 629
        Height = 1
        Align = alTop
        Shape = bsBottomLine
        Style = bsRaised
      end
    end
  end
  object paA: TPanel
    Left = 0
    Top = 0
    Width = 129
    Height = 500
    Align = alLeft
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 1
    object Bevel16: TBevel
      Left = 0
      Top = 118
      Width = 129
      Height = 1
      Align = alTop
      Shape = bsBottomLine
      Style = bsRaised
    end
    object Panel1: TPanel
      Left = 0
      Top = 0
      Width = 129
      Height = 118
      Align = alTop
      BevelOuter = bvNone
      Color = 6316128
      TabOrder = 0
      object MxLabel1: TMxLabel
        Left = 30
        Top = 2
        Width = 50
        Height = 16
        Caption = 'Clips:'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -13
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object MxLabel2: TMxLabel
        Left = 3
        Top = 18
        Width = 13
        Height = 64
        AutoSize = False
        Caption = 'B o n e'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -13
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
        WordWrap = True
      end
      object MxLabel3: TMxLabel
        Left = 14
        Top = 18
        Width = 11
        Height = 64
        AutoSize = False
        Caption = 'p a r t '
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -13
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
        WordWrap = True
      end
      object Bevel2: TBevel
        Left = 28
        Top = 0
        Width = 1
        Height = 122
        Shape = bsRightLine
        Style = bsRaised
      end
      object Bevel4: TBevel
        Left = 0
        Top = 18
        Width = 128
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object Bevel5: TBevel
        Left = 0
        Top = 82
        Width = 128
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object Bevel10: TBevel
        Left = 30
        Top = 34
        Width = 98
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object Bevel11: TBevel
        Left = 30
        Top = 50
        Width = 98
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object Bevel12: TBevel
        Left = 30
        Top = 66
        Width = 98
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object MxLabel4: TMxLabel
        Left = 30
        Top = 100
        Width = 42
        Height = 16
        Caption = 'Time:'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -13
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object lbBPName0: TMxLabel
        Left = 38
        Top = 19
        Width = 64
        Height = 15
        AutoSize = False
        Caption = '-'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object Bevel13: TBevel
        Left = 128
        Top = 0
        Width = 1
        Height = 118
        Align = alRight
        Shape = bsRightLine
        Style = bsRaised
      end
      object lbBPName1: TMxLabel
        Left = 38
        Top = 35
        Width = 64
        Height = 15
        AutoSize = False
        Caption = '-'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object lbBPName2: TMxLabel
        Left = 38
        Top = 51
        Width = 64
        Height = 15
        AutoSize = False
        Caption = '-'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object lbBPName3: TMxLabel
        Left = 38
        Top = 67
        Width = 64
        Height = 15
        AutoSize = False
        Caption = '-'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
      object ebTrash: TExtBtn
        Tag = -1
        Left = 1
        Top = 99
        Width = 26
        Height = 18
        Hint = 'Trash'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          76010000424D7601000000000000760000002800000020000000100000000100
          04000000000000010000120B0000120B00001000000000000000000000000000
          800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
          FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333000000000
          3333333777777777F3333330F777777033333337F3F3F3F7F3333330F0808070
          33333337F7F7F7F7F3333330F080707033333337F7F7F7F7F3333330F0808070
          33333337F7F7F7F7F3333330F080707033333337F7F7F7F7F3333330F0808070
          333333F7F7F7F7F7F3F33030F080707030333737F7F7F7F7F7333300F0808070
          03333377F7F7F7F773333330F080707033333337F7F7F7F7F333333070707070
          33333337F7F7F7F7FF3333000000000003333377777777777F33330F88877777
          0333337FFFFFFFFF7F3333000000000003333377777777777333333330777033
          3333333337FFF7F3333333333000003333333333377777333333}
        NumGlyphs = 2
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        OnClick = ebTrashClick
        OnDragDrop = ebTrashDragDrop
        OnDragOver = ebTrashDragOver
      end
      object Bevel14: TBevel
        Left = 0
        Top = 98
        Width = 128
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object MxLabel5: TMxLabel
        Left = 30
        Top = 83
        Width = 64
        Height = 15
        AutoSize = False
        Caption = 'FX'#39's'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = 12698049
        Font.Height = -11
        Font.Name = 'Courier New'
        Font.Style = []
        ParentFont = False
        ShadowColor = clBlack
        ShadowPos = spRightBottom
      end
    end
    object Panel2: TPanel
      Left = 0
      Top = 119
      Width = 129
      Height = 19
      Align = alTop
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 1
      object ebPrevClip: TExtBtn
        Left = 0
        Top = 2
        Width = 24
        Height = 15
        Hint = 'Prev Clip'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          16020000424D16020000000000003600000028000000100000000A0000000100
          180000000000E0010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBA
          BABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABAFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFBABABA222222BABABAFFFFFFFFFFFFFFFFFFBABA
          BA292929BABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABA3434340000000B
          0B0BBABABAFFFFFFBABABA505050000000000000BABABAFFFFFFFFFFFFFFFFFF
          777777454545000000000000000000060606BABABA5656560000000000000000
          00000000BABABAFFFFFFFFFFFFFFFFFF77777745454500000000000000000006
          0606BABABA565656000000000000000000000000BABABAFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFBABABA3434340000000B0B0BBABABAFFFFFFBABABA5050500000
          00000000BABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABA22
          2222BABABAFFFFFFFFFFFFFFFFFFBABABA292929BABABAFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFBABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF}
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebPrevClipClick
      end
      object ebPlay: TExtBtn
        Left = 24
        Top = 2
        Width = 26
        Height = 15
        Hint = 'Play'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          C2010000424DC20100000000000036000000280000000C0000000B0000000100
          1800000000008C010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFA7A7A7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFBABABA0000005050508E8E8EFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFBABABA0000000000000000004F4F4F7B7B7BFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFBABABA00000000000000000000000008080828
          2828898989FFFFFFFFFFFFFFFFFFFFFFFFBABABA000000000000000000000000
          0000000000001212121F1F1FC6C6C6FFFFFFFFFFFFBABABA0000000000000000
          00000000080808282828898989FFFFFFFFFFFFFFFFFFFFFFFFBABABA00000000
          00000000004F4F4F7B7B7BFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABA
          0000005050508E8E8EFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFA7A7A7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFF}
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebPlayClick
      end
      object ebPlayCycle: TExtBtn
        Left = 50
        Top = 2
        Width = 26
        Height = 15
        Hint = 'Play Looped'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          9E020000424D9E020000000000003600000028000000120000000B0000000100
          18000000000068020000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000FFFFFFA7A7A7FFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
          0000FFFFFFFFFFFF0000BABABA0000005050508E8E8EFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF
          0000BABABA0000000000000000004F4F4F7B7B7BFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFF000000FFFFFF000000000000000000000000FFFFFF0000BABABA000000
          000000000000000000080808282828898989FFFFFFFFFFFF000000FFFFFFFFFF
          FFFFFFFF000000000000FFFFFF0000000000BABABA0000000000000000000000
          000000000000001212121F1F1FC6C6C6000000FFFFFF000000FFFFFFFFFFFF00
          0000FFFFFF0000000000BABABA00000000000000000000000008080828282889
          8989FFFFFFFFFFFF000000FFFFFF000000000000FFFFFFFFFFFFFFFFFF000000
          0000BABABA0000000000000000004F4F4F7B7B7BFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFF000000000000000000000000FFFFFF000000FFFFFF0000BABABA000000
          5050508E8E8EFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000
          00000000FFFFFFFFFFFFFFFFFFFFFFFF0000FFFFFFA7A7A7FFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          0000}
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebPlayCycleClick
      end
      object ebStop: TExtBtn
        Left = 76
        Top = 2
        Width = 26
        Height = 15
        Hint = 'Stop'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          9E010000424D9E0100000000000036000000280000000C0000000A0000000100
          18000000000068010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFA5A5A5ADADADADADADADADADADADADA5A5A5FFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFF969696000000000000000000000000000000000000969696
          FFFFFFFFFFFFFFFFFFFFFFFF9696960000000000000000000000000000000000
          00969696FFFFFFFFFFFFFFFFFFFFFFFF96969600000000000000000000000000
          0000000000969696FFFFFFFFFFFFFFFFFFFFFFFF969696000000000000000000
          000000000000000000969696FFFFFFFFFFFFFFFFFFFFFFFF9696960000000000
          00000000000000000000000000969696FFFFFFFFFFFFFFFFFFFFFFFF96969600
          0000000000000000000000000000000000969696FFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFA5A5A5ADADADADADADADADADADADADA5A5A5FFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFF}
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebStopClick
      end
      object ebNextClip: TExtBtn
        Left = 102
        Top = 2
        Width = 26
        Height = 15
        Hint = 'Next Clip'
        Align = alNone
        BevelShow = False
        HotTrack = True
        HotColor = 15790320
        BtnColor = 10528425
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          16020000424D16020000000000003600000028000000100000000A0000000100
          180000000000E0010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABAFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFBABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABA
          292929BABABAFFFFFFFFFFFFFFFFFFBABABA222222BABABAFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFBABABA000000000000505050BABABAFFFFFFBA
          BABA0B0B0B000000343434BABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABA
          000000000000000000000000565656BABABA0606060000000000000000004545
          45BABABAFFFFFFFFFFFFFFFFFFBABABA000000000000000000000000565656BA
          BABA060606000000000000000000454545BABABAFFFFFFFFFFFFFFFFFFBABABA
          000000000000505050BABABAFFFFFFBABABA0B0B0B000000343434BABABAFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFBABABA292929BABABAFFFFFFFFFFFFFFFFFFBA
          BABA222222BABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          BABABAFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBABABAFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF}
        ParentFont = False
        ParentShowHint = False
        ShowHint = True
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebNextClipClick
      end
      object Bevel17: TBevel
        Left = 0
        Top = 18
        Width = 130
        Height = 1
        Shape = bsBottomLine
        Style = bsRaised
      end
      object Bevel19: TBevel
        Left = 128
        Top = 0
        Width = 1
        Height = 19
        Align = alRight
        Shape = bsRightLine
        Style = bsRaised
      end
    end
    object paClipList: TPanel
      Left = 0
      Top = 138
      Width = 128
      Height = 362
      Align = alClient
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 2
      object Panel4: TPanel
        Left = 0
        Top = 332
        Width = 128
        Height = 30
        Align = alBottom
        BevelOuter = bvNone
        ParentColor = True
        TabOrder = 0
        object ebInsertClip: TExtBtn
          Left = 1
          Top = 0
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
          BtnColor = 10528425
          Caption = 'Insert'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebInsertClipClick
        end
        object ebAppendClip: TExtBtn
          Left = 43
          Top = 0
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
          BtnColor = 10528425
          Caption = 'Append'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebAppendClipClick
        end
        object ebLoadClips: TExtBtn
          Left = 1
          Top = 15
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
          BtnColor = 10528425
          Caption = 'Load'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebLoadClipsClick
        end
        object ebSaveClips: TExtBtn
          Left = 43
          Top = 15
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
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
          OnClick = ebSaveClipsClick
        end
        object ebSync: TExtBtn
          Left = 85
          Top = 0
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
          BtnColor = 10528425
          Caption = 'Sync'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          Transparent = False
          FlatAlwaysEdge = True
          OnClick = ebSyncClick
        end
        object ebClear: TExtBtn
          Left = 85
          Top = 15
          Width = 42
          Height = 15
          Align = alNone
          BevelShow = False
          HotTrack = True
          HotColor = 15790320
          BtnColor = 10528425
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
      end
    end
    object Panel3: TPanel
      Left = 128
      Top = 138
      Width = 1
      Height = 362
      Align = alRight
      BevelOuter = bvNone
      Color = 5460819
      TabOrder = 3
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'ClipMaker'
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredValues = <>
    Left = 33
    Top = 65528
  end
end
