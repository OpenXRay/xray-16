//---------------------------------------------------------------------------
#ifndef FrameSpawnH
#define FrameSpawnH
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
//---------------------------------------------------------------------------
class TfraSpawn : public TForm
{
__published:	// IDE-managed Components
	TPanel *paCurrent;
	TLabel *APHeadLabel1;
	TFormStorage *fsStorage;
	TPanel *paCommands;
	TLabel *Label1;
	TExtBtn *ExtBtn1;
	TExtBtn *ebAttachObject;
	TExtBtn *evDetachObject;
	TPanel *paItems;
	TBevel *Bevel1;
	TPanel *paSelectObject;
	TLabel *Label5;
	TExtBtn *ebSelectByRefs;
	TExtBtn *ebDeselectByRefs;
	TLabel *APHeadLabel3;
	TExtBtn *ExtBtn3;
	TExtBtn *ebMultiSelectByRefMove;
	TBevel *Bevel2;
	TLabel *Label2;
	TExtBtn *ebMultiSelectByRefAppend;
	TMultiObjSpinEdit *seSelPercent;
	TLabel *Label3;
    void __fastcall PaneMinClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebAttachObjectClick(TObject *Sender);
	void __fastcall evDetachObjectClick(TObject *Sender);
	void __fastcall FormHide(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ebSelectByRefsClick(TObject *Sender);
	void __fastcall ebDeselectByRefsClick(TObject *Sender);
	void __fastcall ebMultiSelectByRefMoveClick(TObject *Sender);
	void __fastcall ebMultiSelectByRefAppendClick(TObject *Sender);
	void __fastcall seSelPercentKeyPress(TObject *Sender, char &Key);
private:	// User declarations
	TItemList* m_Items;
    void __stdcall  OnItemFocused	(ListItemsVec& items);
    LPCSTR m_Current;
	void 	SelByRefObject		( bool flag );
	void 	MultiSelByRefObject ( bool clear_prev );
	void __stdcall  OnDrawObjectThumbnail(LPCSTR name, HDC hdc, const Irect &R);
    xr_map<shared_str, shared_str>		m_caption_to_sect;
public:		// User declarations
	__fastcall TfraSpawn(TComponent* Owner);
    LPCSTR 			Current	(){return m_Current;}
};
//---------------------------------------------------------------------------
#endif
