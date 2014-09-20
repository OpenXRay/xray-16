//---------------------------------------------------------------------------

#ifndef BonePartH
#define BonePartH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ElTree.hpp"
#include "ExtBtn.hpp"
#include "ElXPThemedControl.hpp"
#include "mxPlacemnt.hpp"
//---------------------------------------------------------------------------
class TfrmBonePart : public TForm
{
__published:	// IDE-managed Components
	TElTree *tvPart1;
	TLabel *RxLabel29;
	TEdit *edPart1Name;
	TElTree *tvPart2;
	TLabel *RxLabel1;
	TEdit *edPart2Name;
	TElTree *tvPart3;
	TLabel *RxLabel2;
	TEdit *edPart3Name;
	TElTree *tvPart4;
	TLabel *RxLabel3;
	TEdit *edPart4Name;
	TExtBtn *ebSave;
	TExtBtn *ebCancel;
	TFormStorage *fsStorage;
	TExtBtn *ExtBtn1;
	TLabel *lbPart1;
	TLabel *lbPart3;
	TLabel *lbPart4;
	TLabel *lbPart2;
	TLabel *Label1;
	TLabel *lbTotalBones;
	TExtBtn *ExtBtn2;
	TExtBtn *ExtBtn3;
	TExtBtn *ExtBtn4;
	TExtBtn *ExtBtn5;
	TExtBtn *ebSaveTo;
	TExtBtn *ebLoadFrom;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall tvPartDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall tvPartDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall tvPartStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall ebSaveClick(TObject *Sender);
	void __fastcall ebCancelClick(TObject *Sender);
	void __fastcall ExtBtn1Click(TObject *Sender);
	void __fastcall ebClearClick(TObject *Sender);
	void __fastcall ebSaveToClick(TObject *Sender);
	void __fastcall ebLoadFromClick(TObject *Sender);
private:	// User declarations
	xr_vector<TElTreeItem*> FDragItems;
    CEditableObject* m_EditObject;
	BPVec* m_BoneParts;
	void __fastcall FillBoneParts();
	void __fastcall UpdateCount();
public:		// User declarations
	__fastcall TfrmBonePart(TComponent* Owner);
    bool Run(CEditableObject* object);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmBonePart *frmBonePart;
//---------------------------------------------------------------------------
#endif
