{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995, 1996 AO ROSNO             }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{*******************************************************}

unit mxConst;

interface

{$I mx.INC}

uses Controls;

const
  RX_VERSION = $0002004B;  { 2.75 }

const
{ Command message for Speedbar editor }
  CM_SPEEDBARCHANGED = CM_BASE + 80;
{ Command message for TRxSpeedButton }
  CM_RXBUTTONPRESSED = CM_BASE + 81;
{ Command messages for TRxWindowHook }
  CM_RECREATEWINDOW  = CM_BASE + 82;
  CM_DESTROYHOOK     = CM_BASE + 83;
{ Notify message for TRxTrayIcon }
  CM_TRAYICON        = CM_BASE + 84;

const
  crHand     = TCursor(14000);
  crDragHand = TCursor(14001);

const
{ TBitmap.GetTransparentColor from GRAPHICS.PAS uses this value }
  PaletteMask = $02000000;

{$IFDEF VER90}
const
  SDelphiKey = 'Software\Borland\Delphi\2.0';
{$ENDIF}

{$IFDEF VER93}
const
  SDelphiKey = 'Software\Borland\C++Builder\1.0';
{$ENDIF}

{$IFDEF VER100}
const
  SDelphiKey = 'Software\Borland\Delphi\3.0';
{$ENDIF}

{$IFDEF VER110}
const
  SDelphiKey = 'Software\Borland\C++Builder\3.0';
{$ENDIF}

{$IFDEF VER120}
const
  SDelphiKey = 'Software\Borland\Delphi\4.0';
{$ENDIF}

{$IFDEF VER125}
const
  SDelphiKey = 'Software\Borland\C++Builder\4.0';
{$ENDIF}

{$IFDEF VER130}
const
{$IFDEF BCB}
  SDelphiKey = 'Software\Borland\C++Builder\5.0';
{$ELSE}
  SDelphiKey = 'Software\Borland\Delphi\5.0';
{$ENDIF}
{$ENDIF}

implementation

uses {$IFDEF WIN32} Windows, {$ELSE} WinProcs, {$ENDIF} Forms;

{$IFDEF WIN32}
 {$R *.R32}
{$ELSE}
 {$R *.R16}
{$ENDIF}

initialization
  Screen.Cursors[crHand] := LoadCursor(hInstance, 'RX_HANDCUR');
  Screen.Cursors[crDragHand] := LoadCursor(hInstance, 'RX_DRAGCUR');
end.