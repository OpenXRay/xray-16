//---------------------------------------------------------------------------

#ifndef ItemDialogH
#define ItemDialogH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include "CSPIN.h"
#include "ExtBtn.hpp"
#include "multi_edit.hpp"
#include "MXCtrls.hpp"
#include "mxPlacemnt.hpp"

class XR_EPROPS_API TfrmItemDialog : public TForm
{
__published:	// IDE-managed Components
	TExtBtn *eb0;
	TExtBtn *eb3;
	TMxLabel *lbMsg;
	TExtBtn *eb1;
	TExtBtn *eb4;
	TExtBtn *eb2;
	TExtBtn *eb5;
	TPanel *Panel1;
	TExtBtn *eb6;
	TExtBtn *eb7;
	TExtBtn *eb8;
	TFormStorage *fsStorage;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall ebClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
private:	// User declarations
	static 	TfrmItemDialog* form;
    static 	int 			res;
public:		// User declarations
    __fastcall TfrmItemDialog(TComponent* Owner);
    static int __fastcall Run(LPCSTR caption, LPCSTR msg, LPCSTR buttons);
};
//---------------------------------------------------------------------------
#endif
