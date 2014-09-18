//---------------------------------------------------------------------------
#ifndef LeftBarH
#define LeftBarH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>
#include "ExtBtn.hpp"
#include <Menus.hpp>
#include "ExtBtn.hpp"
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include "ElXPThemedControl.hpp"
#include "ElTree.hpp"
#include "ElTreeAdvEdit.hpp"
#include "ElPgCtl.hpp"
#include "MXCtrls.hpp"

#include <ImgList.hpp>

//---------------------------------------------------------------------------
class TfraLeftBar : public TFrame
{
__published:	// IDE-managed Components
    TPanel *paLeftBar;
    TPanel *paScene;
	TFormStorage *fsStorage;
	TLabel *APHeadLabel2;
	TExtBtn *ebSceneMin;
	TExtBtn *ebSceneCommands;
	TExtBtn *ebPreferences;
	TMxPopupMenu *pmEngineShadersFile;
	TMenuItem *Save1;
	TMenuItem *Reload1;
	TMxPopupMenu *pmPreviewObject;
	TMenuItem *Custom1;
	TMenuItem *N2;
	TExtBtn *ebImageCommands;
	TMxPopupMenu *pmImages;
	TMenuItem *ImageEditor1;
	TMenuItem *N6;
	TMenuItem *Refresh1;
	TMenuItem *Checknewtextures1;
	TMxPopupMenu *pmCreateMenu;
	TMenuItem *MenuItem2;
	TMenuItem *N3;
	TMenuItem *N4;
	TMenuItem *N5;
	TMenuItem *N7;
	TImageList *ilModeIcons;
	TMenuItem *ParticleGroup1;
	TMxPopupMenu *pmSounds;
	TMenuItem *MenuItem1;
	TMenuItem *MenuItem3;
	TMenuItem *MenuItem4;
	TMenuItem *MenuItem5;
	TExtBtn *ebSoundCommands;
	TMenuItem *N1;
	TMenuItem *Preferneces1;
	TMenuItem *Validate1;
	TPanel *paParticles;
	TLabel *Label1;
	TPanel *paPSList;
	TBevel *Bevel1;
	TPanel *Panel1;
	TExtBtn *ebEngineRemove;
	TExtBtn *ebEngineClone;
	TExtBtn *ebFile;
	TExtBtn *ebCreate;
	TPanel *paItemList;
	TPanel *Panel2;
	TSplitter *Splitter1;
	TLabel *refName;
	TListBox *refLB;
	TMenuItem *Savexr1;
	TMenuItem *Loadxr1;
	TMenuItem *N8;
	TMenuItem *Compact1;
	TMenuItem *N9;
	TMenuItem *Groupfromcurrenteffect1;
    void __fastcall ebSaveClick(TObject *Sender);
    void __fastcall ebReloadClick(TObject *Sender);
    void __fastcall PanelMimimizeClick(TObject *Sender);
    void __fastcall PanelMaximizeClick(TObject *Sender);
    void __fastcall ebEditorPreferencesClick(TObject *Sender);
    void __fastcall ebRefreshTexturesClick(TObject *Sender);
	void __fastcall ebSceneCommandsMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall PreviewClick(TObject *Sender);
	void __fastcall ebParticleCloneClick(TObject *Sender);
	void __fastcall ebImageCommandsMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ImageEditor1Click(TObject *Sender);
	void __fastcall Refresh1Click(TObject *Sender);
	void __fastcall Checknewtextures1Click(TObject *Sender);
	void __fastcall ebPECreateClick(TObject *Sender);
	void __fastcall ebFileMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ebCreateMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall ebPGCreateClick(TObject *Sender);
	void __fastcall ebEngineRemoveClick(TObject *Sender);
	void __fastcall ebSoundCommandsMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall MenuItem3Click(TObject *Sender);
	void __fastcall MenuItem5Click(TObject *Sender);
	void __fastcall Preferneces1Click(TObject *Sender);
	void __fastcall Validate1Click(TObject *Sender);
	void __fastcall refLBClick(TObject *Sender);
	void __fastcall Savexr1Click(TObject *Sender);
	void __fastcall Loadxr1Click(TObject *Sender);
	void __fastcall Compact1Click(TObject *Sender);
	void __fastcall Groupfromcurrenteffect1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfraLeftBar		(TComponent* Owner);
    void 			UpdateBar		();
    void			OnTimer			();
	void 			ClearParticleList();
    void 			MinimizeAllFrames();
    void 			MaximizeAllFrames();
    void			RefreshBar		(){;}
};
//---------------------------------------------------------------------------
extern PACKAGE TfraLeftBar *fraLeftBar;
//---------------------------------------------------------------------------
#endif
