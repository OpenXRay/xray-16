//---------------------------------------------------------------------------

#ifndef GameTypeFormH
#define GameTypeFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ExtBtn.hpp"
//---------------------------------------------------------------------------
class GameTypeChooser;

class TfmGameType : public TForm
{
__published:	// IDE-managed Components
	TExtBtn *ebOk;
	TExtBtn *ebCancel;
	TCheckBox *cbSingle;
	TCheckBox *cbDeathMatch;
	TCheckBox *cbTeamDeathMatch;
	TCheckBox *cbArtefactHunt;
	TCheckBox *cbCTA;
	void __fastcall ebOkClick(TObject *Sender);
	void __fastcall ebCancelClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	GameTypeChooser* 	m_data;
	__fastcall TfmGameType(TComponent* Owner);
    bool __fastcall Run				(const char* title, GameTypeChooser* data);
};

bool XR_EPROPS_API gameTypeRun(const char* title, GameTypeChooser* data);
#endif
