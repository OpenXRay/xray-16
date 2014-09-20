//---------------------------------------------------------------------------
#ifndef FrameShapeH
#define FrameShapeH
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
#include "mxPlacemnt.hpp"
#include "ESceneCustomMTools.h"
// refs
class CEditObject;
class ESceneShapeTool;
//---------------------------------------------------------------------------
class TfraShape : public TForm
{
__published:	// IDE-managed Components
	TPanel *paAppend;
	TLabel *APHeadLabel1;
	TExtBtn *ExtBtn2;
	TFormStorage *fsStorage;
	TExtBtn *ebTypeSphere;
	TExtBtn *ebTypeBox;
	TPanel *paEdit;
	TLabel *Label1;
	TExtBtn *ExtBtn3;
	TExtBtn *ebAttachShape;
	TExtBtn *ebDetachAllShapes;
	TPanel *Panel1;
	TExtBtn *ebEditLevelBoundMode;
	TExtBtn *ebRecalcLB;
    void __fastcall PaneMinClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebDetachAllShapesClick(TObject *Sender);
	void __fastcall ebAttachShapeClick(TObject *Sender);
	void __fastcall ebEditLevelBoundModeClick(TObject *Sender);
	void __fastcall ebRecalcLBClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TfraShape(TComponent* Owner);
    AnsiString GetCurrentEntity(BOOL bForceSelect=FALSE);
    ESceneShapeTool*	tool;
};
//---------------------------------------------------------------------------
#endif
