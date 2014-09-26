{*******************************************************}
{                                                       }
{         Delphi VCL Extensions (RX)                    }
{                                                       }
{         Copyright (c) 1995 AO ROSNO                   }
{         Copyright (c) 1997, 1998 Master-Bank          }
{                                                       }
{*******************************************************}

unit mxBoxProcs;

{$I MX.INC}

interface

uses Classes, Controls, StdCtrls, MxCtrls;

procedure BoxMoveSelectedItems(SrcList, DstList: TWinControl);
procedure BoxMoveAllItems(SrcList, DstList: TWinControl);
procedure BoxDragOver(List: TWinControl; Source: TObject;
  X, Y: Integer; State: TDragState; var Accept: Boolean; Sorted: Boolean);
procedure BoxMoveFocusedItem(List: TWinControl; DstIndex: Integer);

procedure BoxMoveSelected(List: TWinControl; Items: TStrings);
procedure BoxSetItem(List: TWinControl; Index: Integer);
function BoxGetFirstSelection(List: TWinControl): Integer;
function BoxCanDropItem(List: TWinControl; X, Y: Integer;
  var DragIndex: Integer): Boolean;

implementation

uses {$IFDEF WIN32} Windows {$ELSE} WinTypes, WinProcs {$ENDIF}, Graphics;

function BoxItems(List: TWinControl): TStrings;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).Items
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).Items
  else Result := nil;
end;

function BoxGetSelected(List: TWinControl; Index: Integer): Boolean;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).Selected[Index]
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).Selected[Index]
  else Result := False;
end;

procedure BoxSetSelected(List: TWinControl; Index: Integer; Value: Boolean);
begin
  if List is TCustomListBox then
    TCustomListBox(List).Selected[Index] := Value
  else if List is TMxCustomListBox then
    TMxCustomListBox(List).Selected[Index] := Value;
end;

function BoxGetItemIndex(List: TWinControl): Integer;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).ItemIndex
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).ItemIndex
  else Result := LB_ERR;
end;

{$IFNDEF WIN32}
function BoxGetCanvas(List: TWinControl): TCanvas;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).Canvas
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).Canvas
  else Result := nil;
end;
{$ENDIF}

procedure BoxSetItemIndex(List: TWinControl; Index: Integer);
begin
  if List is TCustomListBox then
    TCustomListBox(List).ItemIndex := Index
  else if List is TMxCustomListBox then
    TMxCustomListBox(List).ItemIndex := Index;
end;

function BoxMultiSelect(List: TWinControl): Boolean;
begin
  if List is TCustomListBox then
    Result := TListBox(List).MultiSelect
  else if List is TMxCustomListBox then
    Result := TMxCheckListBox(List).MultiSelect
  else Result := False;
end;

function BoxSelCount(List: TWinControl): Integer;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).SelCount
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).SelCount
  else Result := 0;
end;

function BoxItemAtPos(List: TWinControl; Pos: TPoint;
  Existing: Boolean): Integer;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).ItemAtPos(Pos, Existing)
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).ItemAtPos(Pos, Existing)
  else Result := LB_ERR;
end;

function BoxItemRect(List: TWinControl; Index: Integer): TRect;
begin
  if List is TCustomListBox then
    Result := TCustomListBox(List).ItemRect(Index)
  else if List is TMxCustomListBox then
    Result := TMxCustomListBox(List).ItemRect(Index)
  else FillChar(Result, SizeOf(Result), 0);
end;

procedure BoxMoveSelected(List: TWinControl; Items: TStrings);
var
  I: Integer;
begin
  if BoxItems(List) = nil then Exit;
  I := 0;
  while I < BoxItems(List).Count do begin
    if BoxGetSelected(List, I) then begin
      Items.AddObject(BoxItems(List).Strings[I], BoxItems(List).Objects[I]);
      BoxItems(List).Delete(I);
    end
    else Inc(I);
  end;
end;

function BoxGetFirstSelection(List: TWinControl): Integer;
var
  I: Integer;
begin
  Result := LB_ERR;
  if BoxItems(List) = nil then Exit;
  for I := 0 to BoxItems(List).Count - 1 do begin
    if BoxGetSelected(List, I) then begin
      Result := I;
      Exit;
    end;
  end;
  Result := LB_ERR;
end;

procedure BoxSetItem(List: TWinControl; Index: Integer);
var
  MaxIndex: Integer;
begin
  if BoxItems(List) = nil then Exit;
  with List do begin
    if CanFocus then SetFocus;
    MaxIndex := BoxItems(List).Count - 1;
    if Index = LB_ERR then Index := 0
    else if Index > MaxIndex then Index := MaxIndex;
    if Index >= 0 then begin
      if BoxMultiSelect(List) then BoxSetSelected(List, Index, True)
      else BoxSetItemIndex(List, Index);
    end;
  end;
end;

procedure BoxMoveSelectedItems(SrcList, DstList: TWinControl);
var
  Index, I, NewIndex: Integer;
begin
  Index := BoxGetFirstSelection(SrcList);
  if Index <> LB_ERR then begin
    BoxItems(SrcList).BeginUpdate;
    BoxItems(DstList).BeginUpdate;
    try
      I := 0;
      while I < BoxItems(SrcList).Count do begin
        if BoxGetSelected(SrcList, I) then begin
          NewIndex := BoxItems(DstList).AddObject(BoxItems(SrcList).Strings[I],
            BoxItems(SrcList).Objects[I]);
          if (SrcList is TMxCheckListBox) and (DstList is TMxCheckListBox) then
          begin
            TMxCheckListBox(DstList).State[NewIndex] :=
              TMxCheckListBox(SrcList).State[I];
          end;
          BoxItems(SrcList).Delete(I);
        end
        else Inc(I);
      end;
      BoxSetItem(SrcList, Index);
    finally
      BoxItems(SrcList).EndUpdate;
      BoxItems(DstList).EndUpdate;
    end;
  end;
end;

procedure BoxMoveAllItems(SrcList, DstList: TWinControl);
var
  I, NewIndex: Integer;
begin
  for I := 0 to BoxItems(SrcList).Count - 1 do begin
    NewIndex := BoxItems(DstList).AddObject(BoxItems(SrcList)[I],
      BoxItems(SrcList).Objects[I]);
    if (SrcList is TMxCheckListBox) and (DstList is TMxCheckListBox) then
    begin
      TMxCheckListBox(DstList).State[NewIndex] :=
        TMxCheckListBox(SrcList).State[I];
    end;
  end;
  BoxItems(SrcList).Clear;
  BoxSetItem(SrcList, 0);
end;

function BoxCanDropItem(List: TWinControl; X, Y: Integer;
  var DragIndex: Integer): Boolean;
var
  Focused: Integer;
begin
  Result := False;
  if (BoxSelCount(List) = 1) or (not BoxMultiSelect(List)) then begin
    Focused := BoxGetItemIndex(List);
    if Focused <> LB_ERR then begin
      DragIndex := BoxItemAtPos(List, Point(X, Y), True);
      if (DragIndex >= 0) and (DragIndex <> Focused) then begin
        Result := True;
      end;
    end;
  end;
end;

procedure BoxDragOver(List: TWinControl; Source: TObject;
  X, Y: Integer; State: TDragState; var Accept: Boolean; Sorted: Boolean);
var
  DragIndex: Integer;
  R: TRect;

  procedure DrawItemFocusRect(Idx: Integer);
{$IFDEF WIN32}
  var
    P: TPoint;
    DC: HDC;
{$ENDIF}
  begin
    R := BoxItemRect(List, Idx);
{$IFDEF WIN32}
    P := List.ClientToScreen(R.TopLeft);
    R := Bounds(P.X, P.Y, R.Right - R.Left, R.Bottom - R.Top);
    DC := GetDC(0);
    DrawFocusRect(DC, R);
    ReleaseDC(0, DC);
{$ELSE}
    BoxGetCanvas(List).DrawFocusRect(R);
{$ENDIF}
  end;

begin
  if Source <> List then
    Accept := (Source is TWinControl) or (Source is TMxCustomListBox)
  else begin
    if Sorted then Accept := False
    else begin
      Accept := BoxCanDropItem(List, X, Y, DragIndex);
      if ((List.Tag - 1) = DragIndex) and (DragIndex >= 0) then begin
        if State = dsDragLeave then begin
          DrawItemFocusRect(List.Tag - 1);
          List.Tag := 0;
        end;
      end
      else begin
        if List.Tag > 0 then DrawItemFocusRect(List.Tag - 1);
        if DragIndex >= 0 then DrawItemFocusRect(DragIndex);
        List.Tag := DragIndex + 1;
      end;
    end;
  end;
end;

procedure BoxMoveFocusedItem(List: TWinControl; DstIndex: Integer);
begin
  if (DstIndex >= 0) and (DstIndex < BoxItems(List).Count) then
    if (DstIndex <> BoxGetItemIndex(List)) then begin
      BoxItems(List).Move(BoxGetItemIndex(List), DstIndex);
      BoxSetItem(List, DstIndex);
    end;
end;

end.
