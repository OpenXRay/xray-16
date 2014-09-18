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
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"

#include "ElTreeAdvEdit.hpp"
#include "ElPgCtl.hpp"
#include "MXCtrls.hpp"
#include "../xrEProps/folderlib.h"
#include "ElBtnCtl.hpp"
#include "ElCheckCtl.hpp"

//---------------------------------------------------------------------------
class TfraLeftBar : public TFrame
{
__published:	// IDE-managed Components
    TPanel *paLeftBar;
	TFormStorage *fsStorage;
	TMxPopupMenu *pmSceneFile;
	TMenuItem *Load1;
	TMenuItem *ebSave;
	TMenuItem *miExportOGF;
	TMenuItem *ebSaveAs;
	TMenuItem *N5;
	TMenuItem *Import1;
	TMenuItem *N2;
	TMenuItem *miRecentFiles;
	TMxPopupMenu *pmPreviewObject;
	TMenuItem *Custom1;
	TMenuItem *N3;
	TMenuItem *none1;
	TMenuItem *Preferences1;
	TMenuItem *Clear1;
	TMenuItem *N4;
	TMxPopupMenu *pmImages;
	TMenuItem *Refresh1;
	TMenuItem *Checknewtextures1;
	TMenuItem *ImageEditor1;
	TMenuItem *N6;
	TMenuItem *N7;
	TMenuItem *N8;
	TMenuItem *N9;
	TMenuItem *ExportDM1;
	TPanel *paScene;
	TLabel *APHeadLabel2;
	TExtBtn *ebSceneMin;
	TExtBtn *ebSceneFile;
	TExtBtn *ebPreferences;
	TExtBtn *ebPreviewObjectClick;
	TExtBtn *ebSceneCommands1;
	TPanel *paModel;
	TLabel *Label4;
	TExtBtn *ExtBtn2;
	TExtBtn *ebRenderEditorStyle;
	TExtBtn *ebRenderEngineStyle;
	TLabel *Label5;
	TExtBtn *ebBonePart;
	TPanel *paObjectProperties;
	TLabel *Label6;
	TBevel *Bevel6;
	TPanel *paObjectProps;
	TPanel *paCurrentMotion;
	TLabel *Label1;
	TExtBtn *ExtBtn10;
	TPanel *paPSList;
	TBevel *Bevel1;
	TPanel *paItemProps;
	TSplitter *Splitter1;
	TExtBtn *ExtBtn1;
	TExtBtn *ExtBtn3;
	TMxPopupMenu *pmSounds;
	TMenuItem *MenuItem1;
	TMenuItem *MenuItem2;
	TMenuItem *MenuItem3;
	TMenuItem *MenuItem4;
	TMenuItem *miExportOMF;
	TMenuItem *N1;
	TMenuItem *ebOptimizeMotions;
	TMenuItem *ExportWaveFrontOBJ1;
	TMenuItem *N10;
	TMenuItem *ebMakeThumbnail;
	TMenuItem *N11;
	TMenuItem *ebExportBatch;
	TMenuItem *ebExport;
	TMenuItem *ExportC1;
	TMenuItem *N12;
	TMenuItem *Quit1;
    void __fastcall ebSaveClick(TObject *Sender);
    void __fastcall PanelMimimizeClick(TObject *Sender);
    void __fastcall PanelMaximizeClick(TObject *Sender);
    void __fastcall ebEditorPreferencesClick(TObject *Sender);
	void __fastcall ebSceneFileMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall tvMotionsStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall tvMotionsDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall tvMotionsDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall Import1Click(TObject *Sender);
	void __fastcall Load1Click(TObject *Sender);
	void __fastcall Save2Click(TObject *Sender);
	void __fastcall ebSaveAsClick(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall miRecentFilesClick(TObject *Sender);
	void __fastcall ebBonePartClick(TObject *Sender);
	void __fastcall miExportOGFClick(TObject *Sender);
	void __fastcall ebMakePreviewClick(TObject *Sender);
	void __fastcall ebRenderStyleClick(TObject *Sender);
	void __fastcall Custom1Click(TObject *Sender);
	void __fastcall none1Click(TObject *Sender);
	void __fastcall Preferences1Click(TObject *Sender);
	void __fastcall ebPreviewObjectClickMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall Clear1Click(TObject *Sender);
	void __fastcall Refresh1Click(TObject *Sender);
	void __fastcall Checknewtextures1Click(TObject *Sender);
	void __fastcall ebSceneCommands1MouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ImageEditor1Click(TObject *Sender);
	void __fastcall ExportDM1Click(TObject *Sender);
	void __fastcall ExtBtn1Click(TObject *Sender);
	void __fastcall ExtBtn3MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall MenuItem2Click(TObject *Sender);
	void __fastcall MenuItem4Click(TObject *Sender);
	void __fastcall miExportOMFClick(TObject *Sender);
	void __fastcall ebOptimizeMotionsClick(TObject *Sender);
	void __fastcall ExportWaveFrontOBJ1Click(TObject *Sender);
	void __fastcall ebMakeThumbnailClick(TObject *Sender);
	void __fastcall ebExportBatchClick(TObject *Sender);
	void __fastcall ExportC1Click(TObject *Sender);
	void __fastcall Quit1Click(TObject *Sender);
private:	// User declarations
	void __fastcall ShowPPMenu		(TMxPopupMenu* M, TObject* btn);
	void 			RenameItem		(LPCSTR p0, LPCSTR p1, EItemType tp);
public:		// User declarations
        __fastcall TfraLeftBar		(TComponent* Owner);
    void 			UpdateBar		();
    void 			OnTimer			();
    void			SetRenderStyle	(bool bEngineStyle);
    void 			MinimizeAllFrames();
    void 			MaximizeAllFrames();
    void 			RefreshBar		();
    void			SetReadOnly		(BOOL val);
};
//---------------------------------------------------------------------------
extern PACKAGE TfraLeftBar *fraLeftBar;
//---------------------------------------------------------------------------
#endif
