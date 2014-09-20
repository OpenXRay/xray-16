//---------------------------------------------------------------------------
#ifndef FramePortalH
#define FramePortalH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "ESceneCustomMTools.h"
#include "mxPlacemnt.hpp"
//refs
class CSector;
class EScenePortalTool;
//---------------------------------------------------------------------------
class TfraPortal : public TForm
{
__published:	// IDE-managed Components
	TPanel *paCommands;
	TLabel *APHeadLabel1;
	TExtBtn *ExtBtn2;
	TExtBtn *ebInvertOrient;
	TExtBtn *ebComputeAllPortals;
	TExtBtn *ebComputeSelPortals;
	TFormStorage *fsStorage;
	TExtBtn *ExtBtn1;
    void __fastcall PanelMinClick(TObject *Sender);
    void __fastcall TopClick(TObject *Sender);
	void __fastcall ebComputeClick(TObject *Sender);
	void __fastcall ebComputeAllPortalsClick(TObject *Sender);
	void __fastcall ebInvertOrientClick(TObject *Sender);
	void __fastcall ExtBtn1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfraPortal(TComponent* Owner);
        EScenePortalTool*	tool;
};
//---------------------------------------------------------------------------
#endif
