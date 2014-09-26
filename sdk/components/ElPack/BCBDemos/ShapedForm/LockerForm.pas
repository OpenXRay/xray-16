unit LockerForm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ElImgFrm, ExtCtrls, StdCtrls, ElPopBtn, ElBtnCtl, ElVCLUtils, ElACtrls,
  Menus, ElXPThemedControl;

type
  TfrmLocker = class(TForm)
    LockedImage: TImage;
    UnlockedImage: TImage;
    ImageForm: TElImageForm;
    CaptionLabel: TLabel;
    LockButton: TElPopupButton;
    UnlockBkgnd: TImage;
    LockBkgnd: TImage;
    SampleEdit: TElAdvancedEdit;
    PopupMenu1: TPopupMenu;
    Close1: TMenuItem;
    procedure LockButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure Close1Click(Sender: TObject);
  private
    Unlocked : boolean;
  public
    { Public declarations }
  end;

var
  frmLocker: TfrmLocker;

implementation

{$R *.DFM}

procedure TfrmLocker.LockButtonClick(Sender: TObject);
begin
  Unlocked := not Unlocked;
  Hide;
  if Unlocked then
  begin                      
    ClientHeight := UnlockedImage.Picture.Height;
    CaptionLabel.Height := 91;
    SampleEdit.Top := 108;
    ImageForm.FormImage := UnlockedImage;
    ImageForm.Background := UnlockBkgnd.Picture.Bitmap;
    LockButton.Caption := 'Lock';
    LockButton.Top := 180;
  end else
  begin
    ClientHeight := LockedImage.Picture.Height;
    CaptionLabel.Height := 71;
    SampleEdit.Top := 88;
    ImageForm.FormImage := LockedImage;
    ImageForm.Background := LockBkgnd.Picture.Bitmap;
    LockButton.Caption := 'Unlock';
    LockButton.Top := 160;
  end;
  Show;
end;

procedure TfrmLocker.FormCreate(Sender: TObject);
begin
  ImageForm.Background := LockBkgnd.Picture.Bitmap;
  ImageForm.BackgroundType := bgtTileBitmap;
end;

procedure TfrmLocker.Close1Click(Sender: TObject);
begin
  Close;
end;

end.

