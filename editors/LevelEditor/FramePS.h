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
#include <ImgList.hpp>
#include "ESceneCustomMTools.h"
// refs
class CEditObject;
//---------------------------------------------------------------------------
class TfraPS : public TForm
{
__published:	// IDE-managed Components
    TPanel *paSelectObject;
    TLabel *Label5;
	TExtBtn *ebSelectByRefs;
	TExtBtn *ebDeselectByRefs;
	TLabel *Label;
	TExtBtn *ExtBtn3;
	TPanel *Panel1;
	TLabel *APHeadLabel2;
	TExtBtn *ebCurrentPSPlay;
	TExtBtn *ebCurrentPSStop;
	TLabel *Label1;
	TPanel *paItems;
	TFormStorage *fsStorage;
	TImageList *ilModeIcons;
	TBevel *Bevel1;
    void __fastcall PaneMinClick(TObject *Sender);
    void __fastcall ebSelectByRefsClick(TObject *Sender);
    void __fastcall ebDeselectByRefsClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebCurrentPSPlayClick(TObject *Sender);
	void __fastcall ebCurrentPSStopClick(TObject *Sender);
	void __fastcall FormHide(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
private:	// User declarations
    void __fastcall SelByRef		(bool flag);
    TItemList* m_Items;
    void __stdcall  OnItemFocused	(ListItemsVec& items);
    LPCSTR m_Current;
public:		// User declarations
	__fastcall 		TfraPS	(TComponent* Owner);
    LPCSTR 			Current	(){return m_Current;}
};
//---------------------------------------------------------------------------
#endif
