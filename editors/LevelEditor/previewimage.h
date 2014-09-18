//---------------------------------------------------------------------------
#ifndef PreviewImageH
#define PreviewImageH
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class ETextureCore;
class TfrmPreviewImage : public TForm
{
__published:	// IDE-managed Components
	TPanel *paImage;
	TPaintBox *pbImage;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall pbImagePaint(TObject *Sender);
	void __fastcall pbImageMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
private:	// User declarations
	static TfrmPreviewImage* form;

    ETextureCore* tex;
    float mult;
public:		// User declarations
    __fastcall TfrmPreviewImage(TComponent* Owner);
    static int __fastcall Run();
};
//---------------------------------------------------------------------------
#endif
