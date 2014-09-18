//---------------------------------------------------------------------------
#ifndef FrameLightH
#define FrameLightH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

#include "ESceneCustomMTools.h"
#include "ExtBtn.hpp"
// refs
class CEditObject;
//---------------------------------------------------------------------------
class TfraLight : public TForm
{
__published:	// IDE-managed Components
    TPanel *paCommands;
	TExtBtn *ebUseSelInD3D;
	TLabel *APHeadLabel1;
	TExtBtn *ExtBtn2;
	TExtBtn *ebUnuseSelInD3D;
	TExtBtn *ebUseAllInD3D;
	TExtBtn *ebUnuseAllInD3D;
    void __fastcall PaneMinClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebUseSelInD3DClick(TObject *Sender);
	void __fastcall ebUnuseSelInD3DClick(TObject *Sender);
	void __fastcall ebUseAllInD3DClick(TObject *Sender);
	void __fastcall ebUnuseAllInD3DClick(TObject *Sender);
private:	// User declarations
    void UseInD3D(bool bAll, bool bFlag);
public:		// User declarations
	__fastcall TfraLight(TComponent* Owner);
};
//---------------------------------------------------------------------------
#endif
