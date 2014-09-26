unit About;

interface 

uses Windows, Classes, Forms, Controls, StdCtrls, Buttons, ExtCtrls, ElURLLabel,
  Graphics, ElCLabel;

type
  TAboutBox = class(TForm)
    Panel1: TPanel;
    OKButton: TButton;
    ProductName: TLabel;
    Copyright: TLabel;
    Image1: TImage;
    HomeLabel: TElURLLabel;
    MailLabel: TElURLLabel;
    Label3: TLabel;
    ElPackLabel: TElURLLabel;
  end;

var
  AboutBox: TAboutBox;

implementation

{$R *.DFM}

end.

