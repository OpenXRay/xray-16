object frmText: TfrmText
  Left = 397
  Top = 371
  Width = 512
  Height = 304
  Caption = 'Text'
  Color = 10528425
  Constraints.MinHeight = 205
  Constraints.MinWidth = 302
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  Scaled = False
  OnActivate = FormActivate
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnDeactivate = FormDeactivate
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object paBottomBar: TPanel
    Left = 0
    Top = 0
    Width = 504
    Height = 20
    Align = alTop
    BevelOuter = bvNone
    Color = 10528425
    TabOrder = 0
    object ebOk: TExtBtn
      Left = 2
      Top = 1
      Width = 50
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Ok'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebOkClick
    end
    object ebCancel: TExtBtn
      Left = 52
      Top = 1
      Width = 50
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = 'Cancel'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebCancelClick
    end
    object ebApply: TExtBtn
      Left = 107
      Top = 1
      Width = 50
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = '&Apply'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebApplyClick
    end
    object ebLoad: TExtBtn
      Left = 214
      Top = 1
      Width = 37
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = '&Load'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebLoadClick
    end
    object ebSave: TExtBtn
      Left = 251
      Top = 1
      Width = 37
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = '&Save'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebSaveClick
    end
    object ebClear: TExtBtn
      Left = 288
      Top = 1
      Width = 37
      Height = 18
      Align = alNone
      BevelShow = False
      Caption = '&Clear'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ebClearClick
    end
  end
  object sbStatusPanel: TElStatusBar
    Left = 0
    Top = 251
    Width = 504
    Height = 19
    Panels = <
      item
        Alignment = taLeftJustify
        Width = 55
        IsHTML = False
      end
      item
        Alignment = taLeftJustify
        IsHTML = False
      end>
    SimplePanel = False
    SimpleTextIsHTML = False
    SizeGrip = False
    ResizablePanels = False
    Bevel = epbNone
    UseXPThemes = False
    Align = alBottom
    Color = clBtnFace
    ParentColor = False
    ParentShowHint = False
    ShowHint = True
    DockOrientation = doNoOrient
    DoubleBuffered = False
  end
  object mmText: TMemo
    Left = 0
    Top = 20
    Width = 504
    Height = 231
    Align = alClient
    Color = 10526880
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Lucida Console'
    Font.Style = []
    ParentFont = False
    ScrollBars = ssBoth
    TabOrder = 2
    OnChange = mmTextChange
    OnKeyUp = mmTextKeyUp
  end
  object fsStorage: TFormStorage
    IniSection = 'Text Form'
    RegistryRoot = prLocalMachine
    Version = 6
    StoredValues = <>
    Left = 45
    Top = 30
  end
  object pmTextMenu: TMxPopupMenu
    Alignment = paCenter
    AutoHotkeys = maManual
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
    Left = 8
    Top = 34
  end
  object tmIdle: TTimer
    Interval = 250
    OnTimer = tmIdleTimer
    Left = 8
    Top = 64
  end
end
