unit frmSeach;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms,
  Dialogs, ElTree, StdCtrls;

type
  TSearchForm = class(TForm)
    SearchBtn: TButton;
    CloseBtn: TButton;
    Label1: TLabel;
    FieldCombo: TComboBox;
    ActionCombo: TComboBox;
    DataEdit: TEdit;
    procedure CloseBtnClick(Sender: TObject);
  private
    { Private declarations }
  public
    Tree : TElTree;
  end;

var
  SearchForm: TSearchForm;

implementation

{$R *.DFM}

procedure TSearchForm.CloseBtnClick(Sender: TObject);
begin
  Close;
end;

initialization
  SearchForm := nil;

end.

