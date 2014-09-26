object fraDetailObject: TfraDetailObject
  Left = 723
  Top = 289
  VertScrollBar.Visible = False
  Align = alClient
  BorderStyle = bsNone
  ClientHeight = 250
  ClientWidth = 217
  Color = 10528425
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = True
  PixelsPerInch = 96
  TextHeight = 13
  object paCommand: TPanel
    Left = 0
    Top = 0
    Width = 217
    Height = 154
    Align = alTop
    ParentColor = True
    TabOrder = 0
    object Label1: TLabel
      Left = 1
      Top = 1
      Width = 215
      Height = 13
      Align = alTop
      Alignment = taCenter
      Caption = 'DO Commands'
      Color = clGray
      ParentColor = False
      OnClick = ExpandClick
    end
    object ExtBtn3: TExtBtn
      Left = 205
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
      OnClick = PaneMinClick
    end
    object ExtBtn1: TExtBtn
      Left = 2
      Top = 16
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'First Initialize'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ExtBtn1Click
    end
    object ExtBtn6: TExtBtn
      Left = 2
      Top = 31
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Reinitialize All'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ExtBtn6Click
    end
    object ExtBtn4: TExtBtn
      Left = 2
      Top = 46
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Reinitialize Objects Only'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ebUpdateObjectsClick
    end
    object ebResetSlots: TExtBtn
      Left = 2
      Top = 102
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Clear Slots'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ebResetSlotsClick
    end
    object ExtBtn7: TExtBtn
      Left = 2
      Top = 82
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Update Renderer'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ebInvalidateCacheClick
    end
    object ebClearDetails: TExtBtn
      Left = 2
      Top = 117
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Clear Details'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ebClearDetailsClick
    end
    object Bevel3: TBevel
      Left = 1
      Top = 98
      Width = 215
      Height = 2
    end
    object Bevel4: TBevel
      Left = 1
      Top = 78
      Width = 215
      Height = 2
    end
    object ExtBtn5: TExtBtn
      Left = 2
      Top = 137
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Object List'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ExtBtn5Click
    end
    object Bevel1: TBevel
      Left = 1
      Top = 133
      Width = 215
      Height = 2
    end
    object ebReinitializeSlotObjects: TExtBtn
      Left = 2
      Top = 61
      Width = 215
      Height = 15
      Align = alNone
      AllowAllUp = True
      BevelShow = False
      HotTrack = True
      HotColor = 15790320
      Caption = 'Reinitialize Selected Slot Objects'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Margin = 13
      ParentFont = False
      OnClick = ebReinitializeSlotObjectsClick
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'FrameDO'
    Options = []
    Version = 15
    StoredProps.Strings = (
      'paCommand.Height'
      'paCommand.Tag')
    StoredValues = <>
    Top = 65528
  end
end
