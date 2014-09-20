//---------------------------------------------------------------------------

#ifndef PropertiesEObjectH
#define PropertiesEObjectH
//---------------------------------------------------------------------------

#include <Classes.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include "mxPlacemnt.hpp"
#include "ExtBtn.hpp"
#include <ComCtrls.hpp>
#include <StdCtrls.hpp>
#include "ElPgCtl.hpp"
#include "ElXPThemedControl.hpp"
#include "MXCtrls.hpp"
//refs
class CSceneObject;
class ETextureThumbnail;
struct SRayPickInfo;

class TfrmPropertiesEObject : public TForm
{
__published:	// IDE-managed Components
	TFormStorage *fsStorage;
	TElPageControl *ElPageControl1;
	TElTabSheet *tsBasic;
	TElTabSheet *tsSurfaces;
	TPanel *paBasic;
	TPanel *paSurfaces;
	TPanel *Panel2;
	TGroupBox *gbTexture;
	TLabel *RxLabel7;
	TLabel *RxLabel8;
	TLabel *RxLabel9;
	TLabel *lbWidth;
	TLabel *lbHeight;
	TLabel *lbAlpha;
	TPanel *Panel1;
	TExtBtn *ebSortByImage;
	TExtBtn *ebSortByName;
	TExtBtn *ebDropper;
	TExtBtn *ebSortByShader;
	TLabel *Label1;
	TMxPanel *paImage;
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall paImagePaint(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
private:	// User declarations
    TProperties* 	m_BasicProp;
    TProperties* 	m_SurfProp;
    bool			bModified;
    TOnModifiedEvent OnModifiedEvent;
    void __stdcall 	OnSurfaceFocused	   	(TElTreeItem* item);
    ETextureThumbnail* m_Thumbnail;
//.    CSceneObject* 	m_pEditObject;
    xr_vector<CSceneObject*>      m_pEditObjects;
    void			FillBasicProps			();
    void			FillSurfProps			();
public:		// User declarations
    __fastcall 		TfrmPropertiesEObject	(TComponent* Owner);
	static TfrmPropertiesEObject* CreateProperties	(TWinControl* parent=0, TAlign align=alNone, TOnModifiedEvent modif=0);
	static void 	DestroyProperties		(TfrmPropertiesEObject*& props);
	void 			UpdateProperties		(xr_vector<CSceneObject*> Ss, bool bReadOnly);
    void			ShowProperties			(){Show();}
    void			HideProperties			(){Hide();}
    bool __fastcall IsModified				(){return (m_BasicProp->IsModified()||m_SurfProp->IsModified());}
	void __fastcall OnPick					(const SRayPickInfo& pinf);
};
//---------------------------------------------------------------------------
#endif
