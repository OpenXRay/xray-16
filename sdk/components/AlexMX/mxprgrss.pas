{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1997 Master-Bank                }
{                                                       }
{*******************************************************}

unit MxPrgrss;

interface

{$I MX.INC}

uses SysUtils, Classes, Controls;

procedure RegisterProgressControl(AClass: TControlClass; const MaxPropName,
  MinPropName, ProgressPropName: string);
procedure UnRegisterProgressControl(AClass: TControlClass);
function SupportsProgressControl(Control: TControl): Boolean;

procedure SetProgressMax(Control: TControl; MaxValue: Longint);
procedure SetProgressMin(Control: TControl; MinValue: Longint);
procedure SetProgressValue(Control: TControl; ProgressValue: Longint);

implementation

{$DEFINE USE_GAUGE}
{$IFDEF WIN32}
  {$IFDEF USE_PROGRESSBAR}
    {$UNDEF USE_GAUGE}
  {$ENDIF}
{$ENDIF}

uses TypInfo, {$IFDEF WIN32} {$IFDEF USE_GAUGE} Gauges, {$ENDIF} ComCtrls;
  {$ELSE} Gauges; {$ENDIF}

{ TProgressList }

type
  TProgressProp = (ppMax, ppMin, ppProgress);

  PProgressData = ^TProgressData;
  TProgressData = record
    ControlClass: TControlClass;
    MaxProperty: string[63];
    MinProperty: string[63];
    ProgressProperty: string[63];
  end;

  TProgressList = class(TList)
  public
    constructor Create;
    destructor Destroy; override;
    procedure Add(AClass: TControlClass; const MaxPropName, MinPropName,
      ProgressPropName: string);
    function FindClass(AClass: TControlClass): Integer;
    procedure Remove(AClass: TControlClass);
    function SetControlProperty(Control: TControl; Prop: TProgressProp;
      Value: Longint): Boolean;
  end;

constructor TProgressList.Create;
begin
  inherited Create;
{$IFDEF WIN32}
  Add(TProgressBar, 'Max', 'Min', 'Position');
{$ENDIF}
{$IFDEF USE_GAUGE}
  Add(TGauge, 'MaxValue', 'MinValue', 'Progress');
{$ENDIF}
end;

destructor TProgressList.Destroy;
var
  I: Integer;
begin
  for I := 0 to Count - 1 do Dispose(PProgressData(Items[I]));
  inherited Destroy;
end;

procedure TProgressList.Add(AClass: TControlClass; const MaxPropName,
  MinPropName, ProgressPropName: string);
var
  NewRec: PProgressData;
begin
  New(NewRec);
  with NewRec^ do begin
    ControlClass := AClass;
    MaxProperty := MaxPropName;
    MinProperty := MinPropName;
    ProgressProperty := ProgressPropName;
  end;
  inherited Add(NewRec);
end;

function TProgressList.FindClass(AClass: TControlClass): Integer;
var
  P: PProgressData;
begin
  for Result := Count - 1 downto 0 do begin
    P := PProgressData(Items[Result]);
    if AClass.InheritsFrom(P^.ControlClass) then Exit;
  end;
  Result := -1;
end;

procedure TProgressList.Remove(AClass: TControlClass);
var
  I: Integer;
  P: PProgressData;
begin
  for I := Count - 1 downto 0 do begin
    P := PProgressData(Items[I]);
    if P^.ControlClass.InheritsFrom(AClass) then begin
      Dispose(P);
      Delete(I);
    end;
  end;
end;

function TProgressList.SetControlProperty(Control: TControl;
  Prop: TProgressProp; Value: Longint): Boolean;
var
  PropInfo: PPropInfo;
  I: Integer;
  PropName: string;
begin
  Result := False;
  if (Control <> nil) then begin
    I := FindClass(TControlClass(Control.ClassType));
    if I >= 0 then begin
      case Prop of
        ppMax: PropName := PProgressData(Items[I])^.MaxProperty;
        ppMin: PropName := PProgressData(Items[I])^.MinProperty;
        else {ppProgress}
          PropName := PProgressData(Items[I])^.ProgressProperty;
      end;
      PropInfo := GetPropInfo(Control.ClassInfo, PropName);
      if (PropInfo <> nil) and (PropInfo^.PropType^.Kind in
        [tkInteger, tkFloat {$IFDEF WIN32}, tkVariant {$ENDIF}]) then
      begin
        SetOrdProp(Control, PropInfo, Value);
        Result := True;
      end;
    end;
  end;
end;

const
  ProgressList: TProgressList = nil;

function GetProgressList: TProgressList;
begin
  if ProgressList = nil then ProgressList := TProgressList.Create;
  Result := ProgressList;
end;

function SupportsProgressControl(Control: TControl): Boolean;
begin
  if Control <> nil then
    Result := GetProgressList.FindClass(TControlClass(Control.ClassType)) >= 0
  else Result := False;
end;

procedure RegisterProgressControl(AClass: TControlClass; const MaxPropName,
  MinPropName, ProgressPropName: string);
begin
  GetProgressList.Add(AClass, MaxPropName, MinPropName, ProgressPropName);
end;

procedure UnRegisterProgressControl(AClass: TControlClass);
begin
  if ProgressList <> nil then ProgressList.Remove(AClass);
end;

procedure SetProgressMax(Control: TControl; MaxValue: Longint);
begin
  GetProgressList.SetControlProperty(Control, ppMax, MaxValue);
end;

procedure SetProgressMin(Control: TControl; MinValue: Longint);
begin
  GetProgressList.SetControlProperty(Control, ppMin, MinValue);
end;

procedure SetProgressValue(Control: TControl; ProgressValue: Longint);
begin
  GetProgressList.SetControlProperty(Control, ppProgress, ProgressValue);
end;

{$IFNDEF WIN32}
procedure Finalize; far;
begin
  ProgressList.Free;
end;
{$ENDIF}

initialization
{$IFDEF WIN32}
finalization
  ProgressList.Free;
{$ELSE}
  AddExitProc(Finalize);
{$ENDIF}
end.