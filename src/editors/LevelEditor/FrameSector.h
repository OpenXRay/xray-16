//---------------------------------------------------------------------------
#ifndef FrameSectorH
#define FrameSectorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

#include "multi_edit.hpp"
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "ESceneCustomMTools.h"
#include "mxPlacemnt.hpp"
//---------------------------------------------------------------------------
class TfraSector : public TForm
{
__published:	// IDE-managed Components
	TPanel *paSectorActions;
	TExtBtn *ebAddMesh;
	TExtBtn *ebDelMesh;
	TExtBtn *ebBoxPick;
	TLabel *RxLabel1;
	TPanel *paCommands;
	TExtBtn *ebValidate;
	TLabel *APHeadLabel1;
	TExtBtn *ExtBtn2;
	TLabel *APHeadLabel2;
	TExtBtn *ExtBtn1;
	TExtBtn *ebCreateNewSingle;
	TBevel *Bevel1;
	TExtBtn *ebCaptureInside;
	TExtBtn *ebCreateDefault;
	TExtBtn *ebRemoveDefault;
	TBevel *Bevel2;
	TFormStorage *fsStorage;
	TExtBtn *ebCreateNewMultiple;
	TExtBtn *ebDistributeObjects;
    void __fastcall PanelMinClick(TObject *Sender);
    void __fastcall TopClick(TObject *Sender);
	void __fastcall ebCaptureInsideVolumeClick(TObject *Sender);
	void __fastcall ebCreateDefaultClick(TObject *Sender);
	void __fastcall ebRemoveDefaultClick(TObject *Sender);
	void __fastcall ebValidateClick(TObject *Sender);
	void __fastcall ebBoxPickClick(TObject *Sender);
	void __fastcall ebDistributeObjectsClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfraSector(TComponent* Owner);
};
//---------------------------------------------------------------------------
#endif
