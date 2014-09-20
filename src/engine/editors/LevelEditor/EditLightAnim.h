//---------------------------------------------------------------------------

#ifndef EditLightAnimH
#define EditLightAnimH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ImgList.hpp>
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include "multi_edit.hpp"
#include "Gradient.hpp"
#include "ElTrackBar.hpp"
#include "ElTreeAdvEdit.hpp"
#include "MxMenus.hpp"
#include <Menus.hpp>
#include "MXCtrls.hpp"
#include "RenderWindow.hpp"
//---------------------------------------------------------------------------
// refs
class CLAItem;
class TProperties;
class ListItem;
class PropValue;
//---------------------------------------------------------------------------

class TfrmEditLightAnim : public TForm
{
__published:	// IDE-managed Components
	TPanel *paItemProps;
	TFormStorage *fsStorage;
	TPanel *paListAndButtons;
	TPanel *Panel2;
	TExtBtn *ebAddAnim;
	TExtBtn *ebDeleteAnim;
	TExtBtn *ebSave;
	TExtBtn *ebReload;
	TPanel *paItems;
	TPanel *paColorAndControls;
	TPanel *paPropsGroup;
	TPanel *paProps;
	TPanel *paColor;
	TMxLabel *lbCurFrame;
	TPanel *Panel1;
	TExtBtn *ebPrevKey;
	TExtBtn *ebFirstKey;
	TExtBtn *ebNextKey;
	TExtBtn *ebLastKey;
	TExtBtn *ebMoveKeyLeft;
	TExtBtn *ebMoveKeyRight;
	TExtBtn *ebFirstFrame;
	TExtBtn *ebLastFrame;
	TMultiObjSpinEdit *sePointer;
	TStaticText *stStartFrame;
	TStaticText *stEndFrame;
	TExtBtn *ebDeleteKey;
	TExtBtn *ebCreateKey;
	TD3DWindow *wnShape;
	TPaintBox *pbG;
	TMxLabel *lbAlpha;
	TExtBtn *ebClone;
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall ebAddAnimClick(TObject *Sender);
	void __fastcall ebDeleteAnimClick(TObject *Sender);
	void __fastcall ebSaveClick(TObject *Sender);
	void __fastcall ebReloadClick(TObject *Sender);
	void __fastcall pbGPaint(TObject *Sender);
	void __fastcall pbGMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall sePointerKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall sePointerExit(TObject *Sender);
	void __fastcall ebDeleteKeyClick(TObject *Sender);
	void __fastcall ebCreateKeyClick(TObject *Sender);
	void __fastcall pbGMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
	void __fastcall pbGMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall sePointerChange(TObject *Sender);
	void __fastcall ebFirstKeyClick(TObject *Sender);
	void __fastcall ebPrevKeyClick(TObject *Sender);
	void __fastcall ebNextKeyClick(TObject *Sender);
	void __fastcall ebLastKeyClick(TObject *Sender);
	void __fastcall ebMoveKeyLeftClick(TObject *Sender);
	void __fastcall ebMoveKeyRightClick(TObject *Sender);
	void __fastcall ebLastFrameClick(TObject *Sender);
	void __fastcall ebFirstFrameClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall wnShapeKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall ebCloneClick(TObject *Sender);
private:	// User declarations
    void 	InitItems();
    bool 	bFinalClose;
    bool 	IsClose();
	static 	TfrmEditLightAnim *form;
    CLAItem* 				m_CurrentItem;
    ListItem* 				m_CurrentOwner;
    void	SetCurrentItem	(CLAItem* item, ListItem* owner);
    void	UpdateView		();
    int		iMoveKey;
    int 	iTgtMoveKey;
    bool __stdcall 			OnFrameCountAfterEdit	(PropValue* v, s32& val);
	void __stdcall  		OnItemFocused			(TElTreeItem* item);

    TItemList*				m_Items;
    TProperties*			m_Props;
    void __stdcall 			OnModified				(void);
    void					UpdateProperties		();
	void __stdcall 			FindItemByName			(LPCSTR name, bool& res);
public:		// User declarations
    __fastcall 				TfrmEditLightAnim		(TComponent* Owner);
    static bool 			FinalClose				();
    static void __fastcall 	ShowEditor				();
    static void __fastcall	OnIdle					();
};
//---------------------------------------------------------------------------
#endif
