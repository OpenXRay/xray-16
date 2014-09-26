{*******************************************************}
{                                                       }
{         RX Library include file                       }
{         Copyright (c) 1996 AO ROSNO                   }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{*******************************************************}

{ RX.INC is included in all RX Library units so you can specify
  global compiler options here. RX.INC is included before each units'
  required compiler options and the options specified here can be
  overridden by hardcode options in the source files }

{ Conditional defines that affect compilation }

{$DEFINE DCS}
{ Client/Server or Enterprise version. Deactivate this define if you
  have Delphi Desktop, Standard or Professional Edition }

{$DEFINE RX_MIDAS}
{ RX MIDAS components } 

{.$DEFINE RX_ADO}
{ RX ADO components } 

{$DEFINE USE_RX_GIF}
{ This define enables GIF image support. Deactivate this define
  if you are using another GIF image support library. See README.TXT
  for the legal aspects }

{$IFDEF WIN32}
{.$DEFINE USE_PROGRESSBAR}
{ Activate this define if you do not want to use TGauge Borland sample
  component in TDBProgress component and FileUtil routines. In this case
  TProgressBar component will be used }
{$ENDIF}

{$DEFINE USE_FOUR_DIGIT_YEAR}
{ This define controls whether FourDigitYear variable is used to control
  date format in TDateEdit, TDBDateEdit components. When this define is not
  active the ShortDateFormat variable is used instead }

{.$DEFINE USE_OLD_DBLISTS}
{ Activate this define to install old BDE lists components for backward
  compatibility with oldest RX versions }

{$DEFINE DEFAULT_POPUP_CALENDAR}
{ This define controls whether a popup calendar is used as default
  instead of a modal dialog in TDateEdit, TDBDateEdit components }

{ ------------ !! DO NOT CHANGE DEFINES BELOW THIS LINE !! ------------ }

{$B-} {- Complete Boolean Evaluation }
{$R-} {- Range-Checking }
{$V-} {- Var-String Checking }
{$T-} {- Typed @ operator }
{$X+} {- Extended syntax }
{$P+} {- Open string params }
{$IFDEF WIN32}
{$J+} {- Writeable structured consts }
{$H+} {- Use long strings by default }
{$ENDIF}

{$IFDEF BCB}
  {$DEFINE CBUILDER}
{$ENDIF}

{$IFDEF VER93}  { Borland C++Builder 1.x }
  {$DEFINE CBUILDER}
{$ENDIF}

{$IFDEF VER100} { Borland Delphi 3.x }
  {$DEFINE RX_D3}
{$ENDIF}

{$IFDEF VER110} { Borland C++Builder 3.x }
  {$DEFINE CBUILDER}
  {$ObjExportAll On}
  {$UNDEF DCS}
  {$DEFINE RX_D3}
{$ENDIF}

{$IFDEF VER120} { Borland Delphi 4.x }
  {$DEFINE RX_D3}
  {$DEFINE RX_D4}
{$ENDIF}

{$IFDEF VER125} { Borland C++Builder 4.x }
  {$DEFINE CBUILDER}
  {$ObjExportAll On}
  {$UNDEF DCS}
  {$DEFINE RX_D3}
  {$DEFINE RX_D4}
{$ENDIF}

{$IFDEF VER130} { Borland Delphi 5.x }
  {$DEFINE RX_D3}
  {$DEFINE RX_D4}
  {$DEFINE RX_D5}
{$ENDIF}

{$IFDEF VER130} { Borland C++Builder 5.x }
  {$IFDEF BCB}
    {$DEFINE CBUILDER}
    {$ObjExportAll On}
    {$UNDEF DCS}
  {$ENDIF}
{$ENDIF}

{$IFNDEF VER80}                  { Delphi 1.x     }
 {$IFNDEF VER90}                 { Delphi 2.x     }
  {$IFNDEF VER93}                { C++Builder 1.x }
    {$DEFINE RX_D3}              { Delphi 3.x or higher }
    {$IFNDEF VER100}
      {$DEFINE RX_V110}          { C++Builder 3.x or higher }
      {$IFNDEF VER110}
        {$DEFINE RX_D4}          { Delphi 4.x or higher }
        {$IFNDEF VER120}
          {$DEFINE RX_V125}      { C++Builder 4.x or higher }
          {$IFNDEF VER125}
            {$DEFINE RX_D5}      { Delphi 5.x or higher }
            {$IFDEF BCB}
              {$DEFINE RX_V130C} { C++Builder 5.x or higher }
            {$ENDIF}
          {$ENDIF}
        {$ENDIF}
      {$ENDIF}
    {$ENDIF}
  {$ENDIF}
 {$ENDIF}
{$ENDIF}

{$IFDEF CBUILDER}
  {$DEFINE USE_PROGRESSBAR}
  {$UNDEF USE_OLD_DBLISTS}
  {$IFNDEF VER93}
    {$DEFINE RX_V110}      { C++Builder 3.x or higher }
    {$IFNDEF VER110}
      {$DEFINE RX_V125}    { C++Builder 4.x or higher }
      {$IFNDEF VER125} 
        {$DEFINE RX_V130C} { C++Builder 5.x or higher }
      {$ENDIF} 
    {$ENDIF}
  {$ENDIF}
{$ENDIF}

{$IFNDEF RX_D3}
  {$DEFINE DCS}
  {$UNDEF RX_MIDAS}
{$ENDIF}

{$IFNDEF RX_D5}
  {$UNDEF RX_ADO}
{$ENDIF}

{$IFNDEF DCS}
  {$UNDEF RX_MIDAS}
  {$UNDEF RX_ADO}
{$ENDIF}
