{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997 Master-Bank                }
{                                                       }
{*******************************************************}

unit MXLConst;

{ RX Library constants }
{
  Reserved diapasone
  from MaxExtStrID + 10
  to   MaxExtStrID - 20
}

interface

const
{ The minimal VCL's used string ID is 61440. The custom IDs must be
  less that above. }
  MaxExtStrID = 61300;

const

{ Component pages }

  srRXControls         = MaxExtStrID;
  srRXDBAware          = MaxExtStrID - 1;
  srRXTools            = MaxExtStrID - 2;

{ TImageList component editor }

  srSaveImageList      = MaxExtStrID - 3;

{ TFormStorage component editor }

  srStorageDesigner    = MaxExtStrID - 4;

{ TPageManager component editor }

  srProxyEditor        = MaxExtStrID - 5;
  srPageProxies        = MaxExtStrID - 6;
  srProxyName          = MaxExtStrID - 7;
  srPageName           = MaxExtStrID - 8;

{ TSpeedbar component editor }

  srSBItemNotCreate    = MaxExtStrID - 9;
  srConfirmSBDelete    = MaxExtStrID - 10;
  srSpeedbarDesigner   = MaxExtStrID - 11;
  srNewSectionName     = MaxExtStrID - 12;

{ TRxTimerList component editor }

  srEventNotCreate     = MaxExtStrID - 13;
  srTimerDesigner      = MaxExtStrID - 14;
  srTimerEvents        = MaxExtStrID - 15;

{ TAnimatedImage component editor }

  srAniCurFilter       = MaxExtStrID - 16;
  srEditPicture        = MaxExtStrID - 17;
  srLoadAniCursor      = MaxExtStrID - 18;

{ TIconList property editor }

  srLoadIcon           = MaxExtStrID - 19;

{ TRxGradientCaption component editor }

  srCaptionDesigner    = MaxExtStrID - 20;
  srGradientCaptions   = MaxExtStrID + 10;

{ TMemoryTable & TRxMemoryData component editor }

  srBorrowStructure    = MaxExtStrID + 9;

implementation

{$IFDEF WIN32}
 {$R *.R32}
{$ELSE}
 {$R *.R16}
{$ENDIF}

end.