object fraEmitter: TfraEmitter
  Left = 0
  Top = 0
  Width = 443
  Height = 277
  Align = alClient
  Color = 10528425
  ParentColor = False
  TabOrder = 0
  object RxLabel20: TLabel
    Left = 4
    Top = 102
    Width = 45
    Height = 13
    Caption = 'Birth rate:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object RxLabel22: TLabel
    Left = 4
    Top = 120
    Width = 58
    Height = 13
    Caption = 'Particle limit:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object ebBirthFunc: TExtBtn
    Left = 171
    Top = 99
    Width = 17
    Height = 17
    Align = alNone
    AllowAllUp = True
    BevelShow = False
    CloseButton = False
    GroupIndex = 1
    Caption = 'E'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = [fsBold]
    ParentFont = False
    FlatAlwaysEdge = True
  end
  object RxLabel1: TLabel
    Left = 4
    Top = 137
    Width = 27
    Height = 13
    Caption = 'Burst:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object RxLabel6: TLabel
    Left = 4
    Top = 152
    Width = 50
    Height = 13
    Caption = 'Play once:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object seBirthRate: TMultiObjSpinEdit
    Left = 84
    Top = 99
    Width = 88
    Height = 18
    LWSensitivity = 0.01
    ButtonKind = bkLightWave
    Increment = 0.01
    MaxValue = 10000
    ValueType = vtFloat
    AutoSize = False
    Color = 10526880
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    TabOrder = 0
  end
  object seParticleLimit: TMultiObjSpinEdit
    Left = 84
    Top = 117
    Width = 105
    Height = 18
    LWSensitivity = 1
    ButtonKind = bkLightWave
    MaxValue = 1000000
    AutoSize = False
    Color = 10526880
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    TabOrder = 1
  end
  object cbBurst: TMultiObjCheck
    Left = 84
    Top = 137
    Width = 13
    Height = 13
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    TabOrder = 2
  end
  object cbPlayOnce: TMultiObjCheck
    Left = 84
    Top = 152
    Width = 13
    Height = 13
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
    TabOrder = 3
  end
  object pcEmitterType: TElPageControl
    Left = 2
    Top = 2
    Width = 195
    Height = 96
    ActiveTabColor = 10528425
    BorderWidth = 0
    Color = 10528425
    DrawFocus = False
    Flat = True
    HotTrack = True
    InactiveTabColor = 10528425
    MinTabHeight = 20
    MinTabWidth = 20
    Multiline = False
    RaggedRight = False
    ScrollOpposite = False
    Style = etsButtons
    TabIndex = 3
    TabPosition = etpTop
    HotTrackFont.Charset = DEFAULT_CHARSET
    HotTrackFont.Color = 15790320
    HotTrackFont.Height = -11
    HotTrackFont.Name = 'MS Sans Serif'
    HotTrackFont.Style = []
    TabBkColor = 10528425
    ActivePage = tsBox
    FlatTabBorderColor = clBtnShadow
    ParentColor = False
    TabOrder = 4
    object tsPoint1: TElTabSheet
      PageControl = pcEmitterType
      ImageIndex = -1
      TabVisible = True
      Caption = 'Point'
      Visible = False
      object Panel3: TPanel
        Left = 0
        Top = 0
        Width = 195
        Height = 75
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
        object RxLabel42: TLabel
          Left = 60
          Top = 29
          Width = 63
          Height = 13
          Caption = '<No params>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
      end
    end
    object tsCone: TElTabSheet
      TabColor = 10528425
      PageControl = pcEmitterType
      ImageIndex = -1
      TabVisible = True
      Caption = 'Cone'
      Color = 10528425
      Visible = False
      object Panel1: TPanel
        Left = 0
        Top = 0
        Width = 195
        Height = 75
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
        object RxLabel4: TLabel
          Left = 2
          Top = 4
          Width = 43
          Height = 13
          Caption = 'Angle ('#176'):'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object RxLabel35: TLabel
          Left = 2
          Top = 22
          Width = 60
          Height = 13
          Caption = 'Heading (Y'#176')'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object RxLabel36: TLabel
          Left = 3
          Top = 40
          Width = 44
          Height = 13
          Caption = 'Pitch (X'#176')'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object RxLabel37: TLabel
          Left = 2
          Top = 59
          Width = 45
          Height = 13
          Caption = 'Bank (Z'#176')'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object seConeAngle: TMultiObjSpinEdit
          Left = 82
          Top = 1
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 180
          ValueType = vtFloat
          Value = 30
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 0
        end
        object seConeDirH: TMultiObjSpinEdit
          Left = 82
          Top = 19
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 360
          MinValue = -360
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 1
        end
        object seConeDirP: TMultiObjSpinEdit
          Left = 82
          Top = 37
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 360
          MinValue = -360
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 2
        end
        object seConeDirB: TMultiObjSpinEdit
          Left = 82
          Top = 55
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 360
          MinValue = -360
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 3
        end
      end
    end
    object tsSphere: TElTabSheet
      PageControl = pcEmitterType
      ImageIndex = -1
      TabVisible = True
      Caption = 'Sphere'
      Visible = False
      object Panel5: TPanel
        Left = 0
        Top = 0
        Width = 195
        Height = 75
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
        object RxLabel38: TLabel
          Left = 2
          Top = 4
          Width = 53
          Height = 13
          Caption = 'Radius (m):'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object seSphereRadius: TMultiObjSpinEdit
          Left = 82
          Top = 1
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 10000
          ValueType = vtFloat
          Value = 1
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 0
        end
      end
    end
    object tsBox: TElTabSheet
      PageControl = pcEmitterType
      ImageIndex = -1
      TabVisible = True
      Caption = 'Box'
      object Panel4: TPanel
        Left = 0
        Top = 0
        Width = 195
        Height = 75
        Align = alClient
        BevelOuter = bvNone
        Color = 10528425
        TabOrder = 0
        object RxLabel39: TLabel
          Left = 0
          Top = 4
          Width = 50
          Height = 13
          Caption = 'Size X (m):'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object RxLabel40: TLabel
          Left = 1
          Top = 22
          Width = 50
          Height = 13
          Caption = 'Size Y (m):'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object RxLabel41: TLabel
          Left = 0
          Top = 41
          Width = 50
          Height = 13
          Caption = 'Size Z (m):'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
        end
        object seBoxSizeX: TMultiObjSpinEdit
          Left = 82
          Top = 1
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 10000
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 0
        end
        object seBoxSizeY: TMultiObjSpinEdit
          Left = 82
          Top = 19
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 10000
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 1
        end
        object seBoxSizeZ: TMultiObjSpinEdit
          Left = 82
          Top = 37
          Width = 105
          Height = 18
          LWSensitivity = 0.1
          ButtonKind = bkLightWave
          Decimal = 1
          Increment = 0.1
          MaxValue = 10000
          ValueType = vtFloat
          AutoSize = False
          Color = 10526880
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          ParentFont = False
          TabOrder = 2
        end
      end
    end
  end
end
