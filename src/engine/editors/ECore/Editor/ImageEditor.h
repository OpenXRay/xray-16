//---------------------------------------------------------------------------

#ifndef ImageLibH
#define ImageLibH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <StdCtrls.hpp>
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include <ImgList.hpp>

#include "MXCtrls.hpp"

//---------------------------------------------------------------------------
// refs
class ETextureThumbnail;
class TProperties;
//---------------------------------------------------------------------------

class TfrmImageLib : public TForm
{
__published:	// IDE-managed Components
	TPanel *paRight;
	TFormStorage *fsStorage;
	TPanel *paCommand;
	TExtBtn *ebOk;
	TBevel *Bevel1;
	TPanel *paProperties;
	TPanel *paItems;
	TSplitter *Splitter1;
	TBevel *Bevel2;
	TImageList *ImageList;
	TExtBtn *ebCancel;
	TExtBtn *ebRemoveTexture;
	TBevel *Bevel5;
	TPanel *Panel1;
	TMxPanel *paImage;
	TPanel *paFilter;
	TExtBtn *ttImage;
	TExtBtn *ttBumpMap;
	TExtBtn *ttNormalMap;
	TExtBtn *ttCubeMap;
	TExtBtn *ttTerrain;
	TPanel *Panel2;
	TExtBtn *btFilter;
	TExtBtn *ExtBtn1;
    void __fastcall ebOkClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall ebCancelClick(TObject *Sender);
	void __fastcall ebRemoveTextureClick(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall paImagePaint(TObject *Sender);
	void __fastcall ttImageClick(TObject *Sender);
	void __fastcall btFilterClick(TObject *Sender);
	void __fastcall ExtBtn1Click(TObject *Sender);
private:
// list functions
    void __stdcall 		RemoveTexture		(LPCSTR fname, EItemType type, bool& res);

    void 				InitItemsList		();
	void __stdcall  	OnItemsFocused		(ListItemsVec& items);

    void __stdcall  	OnCubeMapBtnClick	(ButtonValue* value, bool& bModif, bool& bSafe);
    
	enum{
    	flUpdateProperties = (1<<0),
    };    
    static Flags32		m_Flags;
private:	// User declarations
	static TfrmImageLib* form;

    DEFINE_VECTOR		(ETextureThumbnail*,THMVec,THMIt);
    DEFINE_MAP			(shared_str,ETextureThumbnail*,THMMap,THMMapIt);
    THMMap				m_THM_Used;
    THMVec				m_THM_Current;
    TItemList*			m_ItemList;
    TProperties* 		m_ItemProps;

    ETextureThumbnail*	FindUsedTHM			(const shared_str& name);
    void				SaveUsedTHM			();
    void				DestroyUsedTHM		();

	void __fastcall 	RegisterModifiedTHM	();
    
    void 				OnModified			();
    static FS_FileSet	texture_map;
    static FS_FileSet	modif_map;
    bool 				bImportMode;
    bool 				bReadonlyMode;
    void __fastcall 	UpdateLib			();
    static bool 		bFormLocked;
    static void 		LockForm			(){ bFormLocked = true;	form->paProperties->Enabled = false; 	form->paItems->Enabled = false; }
    static void 		UnlockForm			(){ bFormLocked = false;form->paProperties->Enabled = true; 	form->paItems->Enabled = true; 	}

    void __stdcall		OnTypeChange		(PropValue* prop);
    void 				SortList			(ETextureThumbnail* thm, xr_vector<AnsiString>& sel_str_vec);
public:		// User declarations
    __fastcall 			TfrmImageLib		(TComponent* Owner);
// static function
    static void __fastcall ImportTextures	();
    static void __fastcall EditLib			(AnsiString& title, bool bImport=false);
    static bool __fastcall HideLib			();
    static bool __fastcall Visible			(){return !!form;}
    static void 		OnFrame				();
    static void			UpdateProperties	(){m_Flags.set(flUpdateProperties,TRUE);}
};
//---------------------------------------------------------------------------
#endif
