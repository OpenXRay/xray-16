
{*******************************************************}
{                                                       }
{       Borland Delphi Visual Component Library         }
{                                                       }
{       Copyright (c) 1995,99 Inprise Corporation       }
{                                                       }
{*******************************************************}

unit multi_color;

{$S-,W-,R-,H+,X+}
{$C PRELOAD}

interface

uses Messages, Windows, SysUtils, Classes, Controls, Forms, Menus, Graphics,
  StdCtrls, ExtCtrls, ComCtrls, CommCtrl;

type
  TMultiObjColor = class(TShape)
  private
	m_BeforeDialog: integer;
	m_AfterDialog: integer;
	m_Diffs: boolean;
	m_Changed: boolean;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
	procedure ObjFirstInit( value: integer );
	procedure ObjNextInit( value: integer );
	function ObjApply( var _to:integer ): boolean;
	function Get(): integer;
	procedure _Set( value: integer );
	function diffs(): boolean;
  published
  end;

implementation

{ TMultiObjColor }
constructor TMultiObjColor.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  ControlStyle := ControlStyle + [csReplicatable];
  Width := 65;
  Height := 30;
  m_BeforeDialog:=0;
  m_AfterDialog:=0;
  m_Diffs:=false;
  m_Changed:=false;
end;

destructor TMultiObjColor.Destroy;
begin
  inherited Destroy;
end;

procedure TMultiObjColor.ObjFirstInit( value: integer );
begin
  m_Changed := false;
  m_Diffs := false;
  m_BeforeDialog := value;
  m_AfterDialog := value;
  Brush.Color := value;
  Brush.Style := bsSolid;
  Pen.Color := clBlack;
end;

procedure TMultiObjColor.ObjNextInit( value: integer );
begin
  if( not m_Diffs ) then
      if( m_BeforeDialog <> value) then
      begin
        m_Diffs := true;
        Brush.Color := clGray;
        Pen.Color   := clWhite;
        Brush.Style := bsClear;
      end;
end;

function TMultiObjColor.ObjApply( var _to: integer ): boolean;
begin
  result := false;
  if( m_Changed ) then
  begin
    _to := m_AfterDialog;
    result := true;
  end;
end;

function TMultiObjColor.Get(): integer;
begin
  result := m_AfterDialog;
end;

procedure TMultiObjColor._Set( value: integer );
begin
  m_AfterDialog := value;
  m_Changed := true;
  m_Diffs := false;
  Brush.Color := value;
  Pen.Color := clBlack;
end;

function TMultiObjColor.diffs(): boolean;
begin
  result := m_Diffs;
end;

end.

