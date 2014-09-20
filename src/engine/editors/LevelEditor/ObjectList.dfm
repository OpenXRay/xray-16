object frmObjectList: TfrmObjectList
  Left = 348
  Top = 415
  Width = 411
  Height = 504
  Caption = 'Object List'
  Color = clGray
  Constraints.MinHeight = 360
  Constraints.MinWidth = 297
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
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 306
    Top = 0
    Width = 97
    Height = 470
    Align = alRight
    BevelInner = bvRaised
    BevelOuter = bvNone
    Color = 10528425
    TabOrder = 0
    object ebShowSel: TExtBtn
      Left = 1
      Top = 84
      Width = 95
      Height = 19
      Align = alNone
      BtnColor = 10528425
      Caption = 'Show Selected'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebShowSelClick
    end
    object ebHideSel: TExtBtn
      Left = 1
      Top = 104
      Width = 95
      Height = 19
      Align = alNone
      BtnColor = 10528425
      Caption = 'Hide Selected'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebHideSelClick
    end
    object ebShowProperties: TExtBtn
      Left = 2
      Top = 125
      Width = 95
      Height = 19
      Align = alNone
      BtnColor = 10528425
      Caption = 'Show Properties'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebShowPropertiesClick
    end
    object rgSO: TRadioGroup
      Left = 1
      Top = 1
      Width = 95
      Height = 81
      Align = alTop
      Caption = ' Show options '
      ItemIndex = 1
      Items.Strings = (
        'All'
        'Visible Only'
        'Invisible Only')
      TabOrder = 0
      OnClick = rgSOClick
    end
    object Panel2: TPanel
      Left = 1
      Top = 429
      Width = 95
      Height = 40
      Align = alBottom
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 1
      object sbRefreshList: TExtBtn
        Left = 1
        Top = 0
        Width = 95
        Height = 19
        Align = alNone
        BtnColor = 10528425
        Caption = 'Refresh List'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = sbRefreshListClick
      end
      object sbClose: TExtBtn
        Left = 1
        Top = 20
        Width = 95
        Height = 19
        Align = alNone
        BtnColor = 10528425
        Caption = 'Close'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = sbCloseClick
      end
    end
  end
  object Panel3: TPanel
    Left = 0
    Top = 0
    Width = 306
    Height = 470
    Align = alClient
    Color = clGray
    TabOrder = 1
    object tvItems: TElTree
      Left = 1
      Top = 1
      Width = 304
      Height = 444
      Cursor = crDefault
      LeftPosition = 0
      DragCursor = crDrag
      Align = alClient
      AutoCollapse = False
      AutoLineHeight = False
      DockOrientation = doNoOrient
      DefaultSectionWidth = 120
      AdjustMultilineHeight = False
      BorderStyle = bsNone
      BorderSides = []
      CheckBoxGlyph.Data = {
        D6050000424DD60500000000000036000000280000003C000000080000000100
        180000000000A005000000000000000000000000000000000000FFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
        0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF535353
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF535353FFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000FFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFF535353535353535353FFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFF000000000000000000FFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFF535353535353535353FFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FF000000000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF53
        5353535353535353535353535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000
        000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5353535353
        53535353535353535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFF000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000FFFFFFFFFF
        FF000000000000FFFFFFFFFFFFFFFFFF535353535353535353FFFFFFFFFFFF53
        5353535353FFFFFFFFFFFFFFFFFF000000000000000000FFFFFFFFFFFF000000
        000000FFFFFFFFFFFFFFFFFF535353535353535353FFFFFFFFFFFF5353535353
        53FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000FFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF535353535353535353FFFFFFFFFFFF
        FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFF
        FFFFFFFF535353535353FFFFFFFFFFFFFFFFFFFFFFFF535353535353FFFFFFFF
        FFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF
        535353535353FFFFFFFFFFFFFFFFFFFFFFFF535353535353FFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFF535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFF535353535353FFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFF535353535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFF535353535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        535353535353FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF535353FFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFF
        FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF535353}
      Color = clWhite
      CustomCheckboxes = True
      CustomPlusMinus = True
      DrawFocusRect = False
      ExpandOnDblClick = False
      ExplorerEditMode = True
      FilteredVisibility = True
      FocusedSelectColor = 10526880
      FocusedSelectTextColor = clBlack
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      FullRowSelect = False
      GradientSteps = 64
      HeaderColor = 7368816
      HeaderHeight = 17
      HeaderHotTrack = False
      HeaderSections.Data = {F4FFFFFF00000000}
      HeaderFlat = True
      HeaderFont.Charset = DEFAULT_CHARSET
      HeaderFont.Color = clWindowText
      HeaderFont.Height = -11
      HeaderFont.Name = 'MS Sans Serif'
      HeaderFont.Style = []
      HorizontalLines = True
      HorzDivLinesColor = 7368816
      HorzScrollBarStyles.ShowTrackHint = False
      HorzScrollBarStyles.Width = 17
      HorzScrollBarStyles.ButtonSize = 16
      HorzScrollBarStyles.UseSystemMetrics = False
      HorzScrollBarStyles.UseXPThemes = False
      IgnoreEnabled = False
      IncrementalSearch = False
      ItemIndent = 14
      KeepSelectionWithinLevel = False
      LineBorderActiveColor = clBlack
      LineBorderInactiveColor = clBlack
      LineHeight = 17
      LinesColor = clBtnShadow
      LinesStyle = psSolid
      MinusPicture.Data = {
        F6000000424DF600000000000000360000002800000008000000080000000100
        180000000000C000000000000000000000000000000000000000808080808080
        808080808080808080808080808080808080808080808080808080E0E0E08080
        80808080808080808080808080808080808080E0E0E080808080808080808080
        8080808080808080E0E0E0E0E0E0E0E0E0808080808080808080808080808080
        E0E0E0E0E0E0E0E0E0808080808080808080808080E0E0E0E0E0E0E0E0E0E0E0
        E0E0E0E0808080808080808080E0E0E0E0E0E0E0E0E0E0E0E0E0E0E080808080
        8080808080808080808080808080808080808080808080808080}
      MouseFrameSelect = True
      OwnerDrawMask = '~~@~~'
      ParentFont = False
      ParentShowHint = False
      PlusMinusTransparent = True
      PlusPicture.Data = {
        F6000000424DF600000000000000360000002800000008000000080000000100
        180000000000C0000000120B0000120B00000000000000000000808080808080
        8080808080808080808080808080808080808080808080808080808080808080
        80808080808080808080808080E0E0E0E0E0E080808080808080808080808080
        8080808080E0E0E0E0E0E0E0E0E0E0E0E0808080808080808080808080E0E0E0
        E0E0E0E0E0E0E0E0E0E0E0E0E0E0E0808080808080E0E0E0E0E0E0E0E0E0E0E0
        E0808080808080808080808080E0E0E0E0E0E080808080808080808080808080
        8080808080808080808080808080808080808080808080808080}
      QuickEditMode = True
      ScrollbarOpposite = False
      ShowHint = False
      ShowImages = False
      ShowLeafButton = False
      ShowLines = False
      SortSection = -1
      StoragePath = '\Tree'
      SortUseCase = False
      StripedOddColor = clGray
      StripedEvenColor = 8158332
      StripedItems = True
      TabOrder = 0
      TabStop = True
      Tracking = False
      TrackColor = 10526880
      VertDivLinesColor = 7368816
      VerticalLines = True
      VertScrollBarStyles.ShowTrackHint = True
      VertScrollBarStyles.Width = 17
      VertScrollBarStyles.ButtonSize = 16
      VertScrollBarStyles.UseSystemMetrics = False
      VertScrollBarStyles.UseXPThemes = False
      VirtualityLevel = vlNone
      UseXPThemes = False
      TextColor = clBtnText
      BkColor = clGray
      OnAfterSelectionChange = tvItemsAfterSelectionChange
      OnItemFocused = tvItemsItemFocused
      OnMouseDown = tvItemsMouseDown
      OnDblClick = tvItemsDblClick
      OnKeyPress = tvItemsKeyPress
    end
    object Panel4: TPanel
      Left = 1
      Top = 445
      Width = 304
      Height = 24
      Align = alBottom
      Color = clGray
      TabOrder = 1
      object ElEdit1: TElEdit
        Left = 1
        Top = 1
        Width = 302
        Height = 21
        Cursor = crIBeam
        VertScrollBarStyles.ShowTrackHint = False
        VertScrollBarStyles.Width = 17
        VertScrollBarStyles.ButtonSize = 17
        HorzScrollBarStyles.ShowTrackHint = False
        HorzScrollBarStyles.Width = 17
        HorzScrollBarStyles.ButtonSize = 17
        UseCustomScrollBars = True
        Alignment = taLeftJustify
        BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
        RTLContent = False
        Transparent = False
        TopMargin = 2
        BorderStyle = bsSingle
        LineBorderActiveColor = clBlack
        LineBorderInactiveColor = clBlack
        WordWrap = False
        OnChange = ElEdit1Change
        Align = alClient
        Color = clWhite
        Ctl3D = True
        ParentColor = False
        ParentCtl3D = False
        TabOrder = 0
        OnEnter = ElEdit1Change
        OnExit = ElEdit1Exit
        OnKeyDown = ElEdit1KeyDown
      end
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Object List'
    StoredValues = <>
    Left = 336
    Top = 160
  end
end
