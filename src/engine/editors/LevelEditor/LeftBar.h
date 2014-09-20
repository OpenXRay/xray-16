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
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include <Menus.hpp>

#include "ESceneClassList.h"
//---------------------------------------------------------------------------
class TfraLeftBar : public TFrame
{
__published:	// IDE-managed Components
    TPanel *paLeftBar;
    TPanel *paScene;
    TPanel *paEdit;
    TPanel *paTarget;
	TExtBtn *ebTargetObject;
	TExtBtn *ebTargetLight;
	TExtBtn *ebTargetSoundSrc;
	TExtBtn *ebTargetSoundEnv;
    TPanel *paFrames;
	TFormStorage *fsStorage;
    TExtBtn *ebTargetGlow;
	TExtBtn *ebTargetSpawnPoint;
	TExtBtn *ebTargetWay;
	TExtBtn *ebTargetSector;
	TExtBtn *ebTargetPortal;
	TLabel *APHeadLabel2;
	TExtBtn *sbSceneMin;
	TLabel *lbTools;
	TLabel *lbEditMode;
	TExtBtn *sbEditMin;
	TExtBtn *sbTargetMin;
	TExtBtn *ebTargetGroup;
	TExtBtn *ebSceneFile;
	TExtBtn *ebScene;
	TExtBtn *ebObjectList;
	TExtBtn *ebPreferences;
	TExtBtn *ExtBtn7;
	TExtBtn *ExtBtn8;
	TExtBtn *ExtBtn10;
	TMxPopupMenu *pmSceneFile;
	TMenuItem *Clear1;
	TMenuItem *miOpen;
	TMenuItem *Save1;
	TMenuItem *SaveAs1;
	TMxPopupMenu *pmScene;
	TMenuItem *Options1;
	TMenuItem *Validate1;
	TMxPopupMenu *pmToolsEdit;
	TMxPopupMenu *pmToolsSelection;
	TMxPopupMenu *pmToolsVisibility;
	TMenuItem *Cut1;
	TMenuItem *Copy1;
	TMenuItem *miPaste;
	TMenuItem *Undo1;
	TMenuItem *Redo1;
	TMenuItem *Delete1;
	TMenuItem *Invert1;
	TMenuItem *SelectAll1;
	TMenuItem *UnselectAll1;
	TMenuItem *HideAll1;
	TMenuItem *HideSelected1;
	TMenuItem *N1;
	TMenuItem *HideUnselected1;
	TMenuItem *UnhideAll1;
	TExtBtn *ebTargetPS;
	TPanel *paSnapList;
	TLabel *Label1;
	TExtBtn *ExtBtn16;
	TExtBtn *ebUseSnapList;
	TExtBtn *ExtBtn1;
	TMxPopupMenu *pmSnapListCommand;
	TMenuItem *MenuItem3;
	TMenuItem *MenuItem4;
	TListBox *lbSnapList;
	TBevel *Bevel1;
	TMenuItem *miAddSelectedToList;
	TExtBtn *ebTargetShape;
	TMxPopupMenu *pmObjectContext;
	TMenuItem *miVisibility;
	TMenuItem *HideSelected2;
	TMenuItem *HideUnselected2;
	TMenuItem *HideAll2;
	TMenuItem *N5;
	TMenuItem *UnhideAll2;
	TMenuItem *Locking1;
	TMenuItem *LockSelected1;
	TMenuItem *MenuItem1;
	TMenuItem *MenuItem2;
	TMenuItem *N6;
	TMenuItem *MenuItem5;
	TMenuItem *MenuItem6;
	TMenuItem *MenuItem7;
	TMenuItem *Edit1;
	TMenuItem *miCopy;
	TMenuItem *miPaste2;
	TMenuItem *miCut;
	TMenuItem *N4;
	TMenuItem *miProperties;
	TMenuItem *N3;
	TMenuItem *miRecentFiles;
	TMenuItem *Quit1;
	TExtBtn *ebProperties;
	TExtBtn *ebLightAnimationEditor;
	TExtBtn *ebImages;
	TMenuItem *N8;
	TMenuItem *N12;
	TMenuItem *N13;
	TMenuItem *N15;
	TMenuItem *N17;
	TMenuItem *N18;
	TMenuItem *N19;
	TMenuItem *N20;
	TExtBtn *ebTargetDO;
	TMenuItem *N22;
	TMenuItem *miSceneSummary;
	TExtBtn *ebSounds;
	TExtBtn *ExtBtn2;
	TExtBtn *ebTargetAIMap;
	TExtBtn *ebCompile;
	TMxPopupMenu *pmCompile;
	TMenuItem *MenuItem19;
	TMenuItem *MenuItem23;
	TMenuItem *MenuItem24;
	TMenuItem *MenuItem25;
	TMenuItem *MenuItem26;
	TMenuItem *MenuItem27;
	TMenuItem *MenuItem29;
	TMenuItem *MenuItem32;
	TMenuItem *MenuItem33;
	TMenuItem *MenuItem34;
	TMxPopupMenu *pmObjects;
	TMenuItem *MenuItem13;
	TMenuItem *MenuItem16;
	TMenuItem *MenuItem17;
	TMenuItem *ReloadObjects1;
	TMenuItem *CleanLibrary;
	TMxPopupMenu *pmImages;
	TMenuItem *N11;
	TMenuItem *ImageEditor1;
	TMenuItem *MenuItem8;
	TMenuItem *UpdateSceneTextures1;
	TMenuItem *N10;
	TMenuItem *MenuItem9;
	TMenuItem *MenuItem10;
	TMxPopupMenu *pmSounds;
	TMenuItem *MenuItem11;
	TMenuItem *MenuItem12;
	TMenuItem *MenuItem15;
	TMenuItem *MenuItem14;
	TMenuItem *UpdateEnvironmentGeometry1;
	TMenuItem *RemoveSelectedFromList1;
	TMenuItem *SelectObjectFromList1;
	TMenuItem *N9;
	TMenuItem *N14;
	TExtBtn *ebSnapListMode;
	TExtBtn *ebModeInvert;
	TBevel *Bevel2;
	TMenuItem *N21;
	TMenuItem *ebOpenSel;
	TMenuItem *ebSaveSel;
	TMenuItem *N23;
	TMenuItem *SynchronizeSounds1;
	TExtBtn *ebTargetWallmarks;
	TMenuItem *ExportErrorList1;
	TMenuItem *miHightlightTexture;
	TMenuItem *MakeSoundOccluder1;
	TMenuItem *N24;
	TMenuItem *N25;
	TMenuItem *ClearDebugDraw1;
	TExtBtn *ebMultiRename;
    TMenuItem *N7;
    TMenuItem *Editminimap1;
        TMenuItem *SyncTHM1;
        TMenuItem *N26;
        TMenuItem *Makepack1;
	TMenuItem *N27;
	TMenuItem *ExportObj;
	TMenuItem *e1;
	TExtBtn *ebTargetFogVolumes;
	TExtBtn *btEnableObject;
	TExtBtn *btEnableLight;
	TExtBtn *btEnableSoundSrc;
	TExtBtn *ExtBtn11;
	TExtBtn *ExtBtn12;
	TExtBtn *ExtBtn13;
	TExtBtn *ExtBtn14;
	TExtBtn *ExtBtn15;
	TExtBtn *btEnableSoundEnv;
	TExtBtn *btEnableGlow;
	TExtBtn *btEnableShape;
	TExtBtn *btEnableSpawnPoint;
	TExtBtn *btEnableWay;
	TExtBtn *btEnableSector;
	TExtBtn *btEnablePortal;
	TExtBtn *btEnableGroup;
	TExtBtn *btEnablePS;
	TExtBtn *btEnableDO;
	TExtBtn *btEnableAIMap;
	TExtBtn *btEnableWallmarks;
	TExtBtn *btEnableFogVolumes;
	TBevel *Bevel3;
	TMenuItem *ClipEditor1;
    void __fastcall ebClearClick(TObject *Sender);
    void __fastcall ebLoadClick(TObject *Sender);
    void __fastcall ebSaveClick(TObject *Sender);
    void __fastcall ebSaveAsClick(TObject *Sender);
    void __fastcall ebBuildClick(TObject *Sender);
    void __fastcall ebOptionsClick(TObject *Sender);
    void __fastcall ebCutClick(TObject *Sender);
    void __fastcall ebCopyClick(TObject *Sender);
    void __fastcall ebPasteClick(TObject *Sender);
    void __fastcall ebUndoClick(TObject *Sender);
    void __fastcall ebRedoClick(TObject *Sender);
    void __fastcall ebValidateSceneClick(TObject *Sender);
    void __fastcall ebObjectListClick(TObject *Sender);
    void __fastcall ebEditLibClick(TObject *Sender);
    void __fastcall TargetClick(TObject *Sender);
    void __fastcall PanelMimimizeClickClick(TObject *Sender);
    void __fastcall PanelMaximizeClick(TObject *Sender);
    void __fastcall ebEditorPreferencesClick(TObject *Sender);
    void __fastcall ebRefreshEditorClick(TObject *Sender);
	void __fastcall ebInvertClick(TObject *Sender);
	void __fastcall ebSelectAllClick(TObject *Sender);
	void __fastcall ebUnselectAllClick(TObject *Sender);
	void __fastcall ebDeleteClick(TObject *Sender);
	void __fastcall ebHideAllClick(TObject *Sender);
	void __fastcall ebHideSelectedClick(TObject *Sender);
	void __fastcall ebUnhideAllClick(TObject *Sender);
	void __fastcall ebHideUnselectedClick(TObject *Sender);
	void __fastcall ebMakeGameClick(TObject *Sender);
	void __fastcall ebSceneFileMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ebSceneMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ExtBtn7MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ExtBtn8MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ExtBtn10MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ebClearSnapClick(TObject *Sender);
	void __fastcall ebSetSnapClick(TObject *Sender);
	void __fastcall ExtBtn1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall miAddSelectedToListClick(TObject *Sender);
	void __fastcall ebUseSnapListClick(TObject *Sender);
	void __fastcall RefreshObjects1Click(TObject *Sender);
	void __fastcall CheckNewTexturesClick(TObject *Sender);
	void __fastcall MakeDetailsClick(TObject *Sender);
	void __fastcall miPropertiesClick(TObject *Sender);
	void __fastcall CleanLibraryClick(TObject *Sender);
	void __fastcall Quit1Click(TObject *Sender);
	void __fastcall ebPropertiesClick(TObject *Sender);
	void __fastcall ebLightAnimationEditorClick(TObject *Sender);
	void __fastcall ebImagesMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ImageEditor1Click(TObject *Sender);
	void __fastcall MenuItem9Click(TObject *Sender);
	void __fastcall UpdateSceneTextures1Click(TObject *Sender);
	void __fastcall ImportCompilerErrorsClick(TObject *Sender);
	void __fastcall ebIgnoreModeClick(TObject *Sender);
	void __fastcall miClearErrorListClick(TObject *Sender);
	void __fastcall miSceneSummaryClick(TObject *Sender);
	void __fastcall MakeHOM1Click(TObject *Sender);
	void __fastcall UpdateEnvironmentGeometry1Click(TObject *Sender);
	void __fastcall MenuItem14Click(TObject *Sender);
	void __fastcall ebSoundsMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ExtBtn2MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall MakeAIMap1Click(TObject *Sender);
	void __fastcall ebCompileMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall SelectObjectFromList1Click(TObject *Sender);
	void __fastcall RemoveSelectedFromList1Click(TObject *Sender);
	void __fastcall ebSnapListModeClick(TObject *Sender);
	void __fastcall ebOpenSelClick(TObject *Sender);
	void __fastcall ebSaveSelClick(TObject *Sender);
	void __fastcall MenuItem12Click(TObject *Sender);
	void __fastcall SynchronizeSounds1Click(TObject *Sender);
	void __fastcall ExportErrorList1Click(TObject *Sender);
	void __fastcall miHightlightTextureClick(TObject *Sender);
	void __fastcall MakeSoundOccluder1Click(TObject *Sender);
	void __fastcall ClearDebugDraw1Click(TObject *Sender);
	void __fastcall ebMultiRenameClick(TObject *Sender);
    void __fastcall Editminimap1Click(TObject *Sender);
        void __fastcall SyncTHM1Click(TObject *Sender);
        void __fastcall Makepack1Click(TObject *Sender);
	void __fastcall ExportObjClick(TObject *Sender);
	void __fastcall e1Click(TObject *Sender);
	void __fastcall btEnableObjectClick(TObject *Sender);
	void __fastcall ClipEditor1Click(TObject *Sender);
private:	// User declarations
    void RedrawBar();
	void __fastcall miRecentFilesClick(TObject *Sender);
    
    xr_vector<std::pair<TExtBtn*,TExtBtn*> >	m_TargetButtons;
public:		// User declarations
        __fastcall TfraLeftBar(TComponent* Owner);
	void ChangeTarget(ObjClassID tgt);
    void UpdateSnapList();
    void MinimizeAllFrames();
    void MaximizeAllFrames();
    void UpdateBar();
    void OnTimer();
    void RefreshBar();
};
//---------------------------------------------------------------------------
extern PACKAGE TfraLeftBar *fraLeftBar;
//---------------------------------------------------------------------------
#endif
