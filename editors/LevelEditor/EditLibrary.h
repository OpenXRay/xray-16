//---------------------------------------------------------------------------

#ifndef EditLibraryH
#define EditLibraryH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ImgList.hpp>

#include "../ECore/Editor/Library.h"
#include <Dialogs.hpp>
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include <Menus.hpp>
#include "PropertiesEObject.h"
#include "MxMenus.hpp"
#include "ElTreeAdvEdit.hpp"
#include "MXCtrls.hpp"
//---------------------------------------------------------------------------
// refs
class TObjectPreview;
class EObjectThumbnail;
//---------------------------------------------------------------------------

class TfrmEditLibrary : public TForm
{
__published:	// IDE-managed Components
	TPanel *paCommands;
	TExtBtn *ebMakeThm;
	TFormStorage *fsStorage;
	TExtBtn *ebProperties;
	TCheckBox *cbPreview;
	TExtBtn *ebMakeLOD_high;
	TPanel *Panel3;
	TLabel *lbFaces;
	TLabel *RxLabel2;
	TLabel *RxLabel3;
	TLabel *lbVertices;
	TBevel *Bevel4;
	TPanel *paItems;
	TPanel *paControl;
	TExtBtn *ebImport;
	TExtBtn *ebExportLWO;
	TExtBtn *ebSave;
	TExtBtn *ebCancel;
	TExtBtn *ebRenameObject;
	TExtBtn *ebRemoveObject;
	TMxPanel *paImage;
	TExtBtn *ebMakeLOD_low;
	TExtBtn *ebExportOBJ;
    void __fastcall ebSaveClick(TObject *Sender);
    void __fastcall ebCancelClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall ebPropertiesClick(TObject *Sender);
    void __fastcall tvItemsDblClick(TObject *Sender);
    void __fastcall cbPreviewClick(TObject *Sender);
	void __fastcall ebMakeThmClick(TObject *Sender);
	void __fastcall paImagePaint(TObject *Sender);
	void __fastcall ebImportClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall ebMakeLOD_highClick(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall ebExportLWOClick(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall ebRenameObjectClick(TObject *Sender);
	void __fastcall ebRemoveObjectClick(TObject *Sender);
	void __fastcall ebMakeLOD_lowClick(TObject *Sender);
	void __fastcall ebExportOBJClick(TObject *Sender);
private:	// User declarations
    void InitObjects();
    EObjectThumbnail* 		m_Thm;
	static TfrmEditLibrary*	form;
    xr_vector<CSceneObject*> m_pEditObjects;
    static FS_FileSet		modif_map;
    static bool 			bFinalExit;
    static bool 			bExitResult;
    TfrmPropertiesEObject*  m_Props;
    void					UpdateObjectProperties	();
    void					ChangeReference			(const RStringVec& items);

	void __fastcall 		OnObjectRename	(LPCSTR p0, LPCSTR p1, EItemType type);

    bool 					bReadOnly;
    TItemList*				m_Items;
	void __stdcall  		OnItemsFocused	(ListItemsVec& items);
	bool 					GenerateLOD		(ListItemsVec& props, bool bHighQuality);
    void 					MakeLOD			(bool bHighQuality);
    bool 					bFormLocked;
    void 					LockForm		()	{ bFormLocked = true;	paCommands->Enabled = false; 	paItems->Enabled = false; 	}
    void 					UnlockForm		()	{ bFormLocked = false;	paCommands->Enabled = true; 	paItems->Enabled = true;	}
    void __fastcall 		ResetSelected	();
    void __fastcall 		RefreshSelected	();
    bool					SelectionToReference(ListItemsVec* props);
    void					ExportOneOBJ	(CEditableObject*);
public:		// User declarations
    void __stdcall  		OnModified		();
    __fastcall 				TfrmEditLibrary	(TComponent* Owner);
    static bool 			FinalClose		();
    static void __fastcall 	OnRender		();
    static void __fastcall 	ZoomObject		();
	static CSceneObject* __fastcall RayPick	(const Fvector& start, const Fvector& direction, SRayPickInfo* pinf);
    static void __fastcall 	ShowEditor		();
};
//---------------------------------------------------------------------------
#endif
