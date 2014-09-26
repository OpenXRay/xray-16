unit frmPswGen;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ElIni, ElBtnCtl, ElCheckCtl, ElACtrls, ElPopBtn,
  ElSpin, ElFrmPers, ElXPThemedControl, ElGroupBox, ExtCtrls, ElPanel;

type
  TPswGenForm = class(TForm)
    GroupBox1: TElGroupBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;                 
    FormPers: TElFormPersist;
    CapRB: TElCheckBox;
    LetRB: TElCheckBox;
    DigRB: TElCheckBox;
    AllRB: TElCheckBox;
    CustomRB: TElCheckBox;
    OKBtn: TElPopupButton;
    CancelBtn: TElPopupButton;
    PswGenBtn: TElPopupButton;
    CustomEdit: TElAdvancedEdit;
    PswEdit: TElAdvancedEdit;
    LengthSpin: TElSpinEdit;
    procedure CustomRBClick(Sender: TObject);
    procedure PswGenBtnClick(Sender: TObject);
    procedure OKBtnClick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  private
    { Private declarations }
  public
    DoAccept : boolean;
    Pssw : string;
  end;

var
  PswGenForm: TPswGenForm;

implementation

uses LogoMain;

{$R *.DFM}

procedure TPswGenForm.CustomRBClick(Sender: TObject);
begin
  CustomEdit.Enabled := CustomRB.Checked;
  if CustomEdit.Enabled
     then CustomEdit.Color := clWindow
     else CustomEdit.ParentColor := true;
end;

type TSymArray = array [0 .. 255] of char;

procedure TPswGenForm.PswGenBtnClick(Sender: TObject);
var i, j : integer;
    arrlen : integer;
    SA : String;
begin
  if CapRB.Checked then
    for i := ord('A') to ord('Z') do SA := sa + char(i);
  if LetRB.Checked then
    for i := ord('a') to ord('z') do SA := sa + char(i);
  if DigRB.Checked then
    for i := ord('0') to ord('9') do SA := sa + char(i);
  if AllRB.Checked then
  begin
    SA := sa + '!';
    SA := sa + '@';
    SA := sa + '#';
    SA := sa + '$';
    SA := sa + '%';
    SA := sa + '^';
    SA := sa + '&';
    SA := sa + '*';
  end;
  if CustomRB.Checked then
  begin
    for i := 1 to Length(CustomEdit.Text) do
      if Pos(CustomEdit.Text[i], SA) = 0 then SA := SA + CustomEdit.Text[i];
  end;
  arrlen := Length(SA);
  if arrlen > 0 then
  begin
    Randomize;
    SetLength(Pssw, LengthSpin.Value);
    for i := 1 to LengthSpin.Value do
    begin
      j := Round(Random(arrlen))+1;
      Pssw[i] := SA[j];
    end;
    PswEdit.Text := Pssw;
  end;
end;

procedure TPswGenForm.OKBtnClick(Sender: TObject);
begin
  DoAccept := true;
end;

procedure TPswGenForm.FormCloseQuery(Sender: TObject;
  var CanClose: Boolean);
begin
  if DoAccept then
  begin
    DoAccept := false;
    CanClose := MessageDlg('Do you want to accept the generated password?', mtConfirmation, [mbOk, mbCancel], 0) = mrOk;
  end;
end;

end.

