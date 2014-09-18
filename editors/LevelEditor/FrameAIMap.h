//---------------------------------------------------------------------------
#ifndef FramePSH
#define FramePSH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include "ElPanel.hpp"
#include "ElSplit.hpp"
#include "multi_edit.hpp"
#include "ESceneCustomMTools.h"
// refs
class CEditObject;
class ESceneAIMapTool;
//---------------------------------------------------------------------------
class TfraAIMap : public TForm
{
__published:	// IDE-managed Components
	TFormStorage *fsStorage;
	TPanel *paObjectList;
	TLabel *Label2;
	TExtBtn *ExtBtn2;
	TPanel *paLink;
	TLabel *Label3;
	TExtBtn *ExtBtn4;
	TExtBtn *ebGenerateMap;
	TExtBtn *ebModeAppend;
	TExtBtn *ebModeRemove;
	TExtBtn *ebUp;
	TExtBtn *ebDown;
	TExtBtn *ebLeft;
	TExtBtn *ebRight;
	TExtBtn *ebFull;
	TExtBtn *ebModeInvert;
	TExtBtn *ebSmoothNodes;
	TLabel *Label5;
	TExtBtn *ebIgnoreConstraints;
	TExtBtn *ebAutoLink;
	TBevel *Bevel2;
	TExtBtn *ExtBtn1;
	TExtBtn *ExtBtn5;
	TExtBtn *ExtBtn3;
	TExtBtn *ExtBtn6;
	TExtBtn *ebGenerateSelected;
	TExtBtn *ebResetSelected;
	TExtBtn *btnIgnoreMaterialClear;
	TListBox *lbIgnoreMaterialsList;
	TLabel *Label1;
	TExtBtn *btnAddIgnoredMaterial;
    void __fastcall PanelMinClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebGenerateMapClick(TObject *Sender);
	void __fastcall ebDrawSnapObjectsClick(TObject *Sender);
	void __fastcall ebInvertLinkClick(TObject *Sender);
	void __fastcall ebSideClick(TObject *Sender);
	void __fastcall ebSmoothNodesClick(TObject *Sender);
	void __fastcall ebSelLinkClick(TObject *Sender);
	void __fastcall ExtBtn6Click(TObject *Sender);
	void __fastcall ebGenerateSelectedClick(TObject *Sender);
	void __fastcall ebResetSelectedClick(TObject *Sender);
	void __fastcall btnAddIgnoredMaterialClick(TObject *Sender);
	void __fastcall btnIgnoreMaterialClearClick(TObject *Sender);
private:	// User declarations
	ESceneAIMapTool* tools;
public:		// User declarations
        __fastcall 	TfraAIMap(TComponent* Owner, ESceneAIMapTool* _tools);
};
//---------------------------------------------------------------------------
#endif
