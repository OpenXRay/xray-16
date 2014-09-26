unit frmSelForm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls;

type
  TSelForm = class(TForm)
    SelLB: TListBox;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  SelForm: TSelForm;

implementation

{$R *.DFM}

end.

