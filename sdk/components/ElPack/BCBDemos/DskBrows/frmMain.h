//---------------------------------------------------------------------------

#ifndef frmMainH
#define frmMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ElTree.hpp"
#include <ImgList.hpp>
#include <ElStrUtils.hpp>
#include "ElXPThemedControl.h"
#include <string>
#include "ElHashList.hpp"
//---------------------------------------------------------------------------
using namespace std;

class TMainForm : public TForm
{
__published:	// IDE-managed Components
        TElTree *Tree;
        TButton *Button1;
        TButton *ExitBtn;
        TImageList *Images;
        TCheckBox *FullPathCB;
        void __fastcall ExitBtnClick(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall FullPathCBClick(TObject *Sender);
        void __fastcall TreeCompareItems(TObject *Sender,
          TElTreeItem *Item1, TElTreeItem *Item2, int &res);
        void __fastcall TreeDragDrop(TObject *Sender, TObject *Source,
          int X, int Y);
        void __fastcall TreeDragOver(TObject *Sender, TObject *Source,
          int X, int Y, TDragState State, bool &Accept);
        void __fastcall TreeHeaderColumnClick(TObject *Sender,
          int SectionIndex);
        void __fastcall TreeItemCollapse(TObject *Sender,
          TElTreeItem *Item);
        void __fastcall TreeItemExpand(TObject *Sender, TElTreeItem *Item);
        void __fastcall TreeItemExpanding(TObject *Sender,
          TElTreeItem *Item, bool &CanProcess);
        void __fastcall TreeKeyUp(TObject *Sender, WORD &Key,
          TShiftState Shift);
        void __fastcall TreeStartDrag(TObject *Sender,
          TDragObject *&DragObject);
        void __fastcall TreeValidateInplaceEdit(TObject *Sender,
          TElTreeItem *Item, TElHeaderSection *Section, AnsiString &Text,
          bool &Accept);
        void __fastcall TreeShowLineHint(TObject *Sender, TElTreeItem *Item,
          TElFString &Text, THintWindow *HintWindow, tagPOINT &MousePos,
          bool &DoShowHint);
private:	// User declarations
    TElTreeItem* LastSelected;
    TElTreeItem* ItemDragging;
    TElHashList* Hash;

public:		// User declarations
         __fastcall TMainForm(TComponent* Owner);
    void __fastcall FillRoots();
    void __fastcall FillTree(TElTreeItem* Item, AnsiString Path);

};

class TElDragObject : public TDragControlObject {
    protected:
      TCursor __fastcall GetDragCursor(bool Accepted, int X, int Y);
    public:
      __fastcall  TElDragObject(TControl* AControl) : TDragControlObject(AControl) { }

};

//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
