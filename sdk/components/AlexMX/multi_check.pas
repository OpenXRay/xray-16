
{*******************************************************}
{                                                       }
{       Borland Delphi Visual Component Library         }
{                                                       }
{       Copyright (c) 1995,99 Inprise Corporation       }
{                                                       }
{*******************************************************}

unit multi_check;

{$S-,W-,R-,H+,X+}
{$C PRELOAD}

interface

uses Messages, Windows, SysUtils, Classes, Controls, Forms, Menus, Graphics,
  StdCtrls, ExtCtrls, ComCtrls, CommCtrl;

type
  TMultiObjCheck = class(TCheckBox)
  published
    procedure ObjFirstInit	( chk: TCheckBoxState );
    procedure ObjNextInit	( chk: TCheckBoxState );
    procedure ObjApply		( var _to: integer );
  end;

implementation

//------------------------------------------------------
procedure TMultiObjCheck.ObjFirstInit( chk: TCheckBoxState );
begin
  state := chk;
  if (state=cbGrayed) then state := cbUnchecked;
end;

procedure TMultiObjCheck.ObjNextInit( chk: TCheckBoxState );
begin
  if( (state=cbChecked) and (chk=cbUnchecked) ) then state := cbGrayed;
  if( (state=cbUnchecked) and (chk=cbChecked) ) then state := cbGrayed;
end;

procedure TMultiObjCheck.ObjApply( var _to: integer );
begin
  if( state=cbChecked ) then _to := 1;
  if( state=cbUnchecked ) then _to := 0;
end;

//------------------------------------------------------------------------------
end.

