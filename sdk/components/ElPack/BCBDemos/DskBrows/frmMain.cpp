//---------------------------------------------------------------------------

#include <vcl.h>
#include <typeinfo.h>
#include <ShellApi.h>
#pragma hdrstop

#include "frmMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElTree"
#pragma link "ElXPThemedControl"
#pragma resource "*.dfm"
TMainForm *MainForm;
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
TCursor __fastcall TElDragObject::GetDragCursor(bool Accepted, int X, int Y) {

  if (typeid(Control) == typeid(TElTree)) {
    if (((dynamic_cast< TElTree* >(Control))->GetItemAtY(Y) != NULL) ||
      (Accepted)) {
       return (dynamic_cast< TElTree* >(Control))->DragCursor;
    }
    else {
      return crNoDrop;
    }
  }
  else {
    return TDragControlObject::GetDragCursor(Accepted,X,Y);
  }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ExitBtnClick(TObject *Sender)
{
  MainForm->Close();
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormCreate(TObject *Sender)
{
  LastSelected = NULL;
  Hash = new TElHashList();
  Tree->IsUpdating = true;
  FillRoots();
  Tree->IsUpdating = false;
  Tree->DragImageMode = dimNever;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FullPathCBClick(TObject *Sender)
{
  Tree->HeaderSections->Item[0]->Visible = FullPathCB->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeCompareItems(TObject *Sender,
      TElTreeItem *Item1, TElTreeItem *Item2, int &res)
{
  AnsiString S1, S2;

  S1 = "";
  S2 = "";
  try {
    if (Item1->ColumnText->Count > 0) { S1 = Item1->ColumnText->Strings[0]; }
  }
  catch (Exception &exception) { }

  try {
    if (Item2->ColumnText->Count > 0) { S2 = Item2->ColumnText->Strings[0]; }
  }
  catch (Exception &exception) { }

  if (Item1->Bold) {
    if (Item2->Bold) {
      res = AnsiCompareText(S1, S2);
    }
    else {
      res = -1;
    }
  }
  else {
    if (Item2->Bold) {
      res = 1;
    }
    else {
      res = AnsiCompareText(S1, S2);
    }
  }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeDragDrop(TObject *Sender, TObject *Source,
      int X, int Y)
{
  MessageBox(0, "Sorry, but moving a file is not implemented", "ElPack Demo", 0);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeDragOver(TObject *Sender, TObject *Source,
      int X, int Y, TDragState State, bool &Accept)
{
  TElTreeItem* TSI;
  Accept = false;
  if (typeid(Source) != typeid(TElDragObject)) { return; }

  TSI = dynamic_cast< TElTree* >(dynamic_cast<TElDragObject *>(Source)->Control)->GetItemAtY(Y);
  if ((TSI != NULL) && (!(TSI->IsUnder(ItemDragging))) ) {
    Accept = true;
  }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeHeaderColumnClick(TObject *Sender,
      int SectionIndex)
{

  Tree->SortSection = SectionIndex;
  switch ( SectionIndex ) {
    case 0 : Tree->SortType = Eltree::stText; break;
    case 1 : Tree->SortType = Eltree::stCustom; break;
    case 2 : Tree->SortType = Eltree::stNumber; break;
    case 3 : Tree->SortType = Eltree::stDate; break;
    case 4 : Tree->SortType = Eltree::stTime; break;
  }
  if ((Tree->HeaderSections->Item[SectionIndex])->SortMode == hsmAscend) {
    (Tree->HeaderSections->Item[SectionIndex])->SortMode = hsmDescend;
  }
  else {
    (Tree->HeaderSections->Item[SectionIndex])->SortMode = hsmAscend;
  }
  if (LastSelected != NULL) {
    LastSelected->Sort(false);
  }
  else {
    Tree->Sort(false);
  }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeItemCollapse(TObject *Sender,
      TElTreeItem *Item)
{
    Tree->IsUpdating = true;
    Item->Clear();
    LastSelected = Item->Parent;
    Tree->IsUpdating = false;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeItemExpand(TObject *Sender,
      TElTreeItem *Item)
{

    Tree->IsUpdating = true;
    FillTree(Item, Item->ColumnText->Strings[0]);
    LastSelected = Item;
    Tree->IsUpdating = false;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeItemExpanding(TObject *Sender,
      TElTreeItem *Item, bool &CanProcess)
{

TSearchRec SRec;
    AnsiString s;

  s = Item->ColumnText->Strings[0];
//  if (s[s.Length()] != '\\') { s = s + '\\'; }
  if (AnsiLastChar(s) != "\\") { s = s + "\\"; }
  s = s + "*.*";
//  FillChar(SRec, sizeof(SRec), #0);
  CanProcess = (FindFirst(s, faAnyFile, SRec) == 0);
  FindClose(SRec);

}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeKeyUp(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if (Key == VK_DELETE) {
    MessageBox(0, "Sorry, but deleting a file is not implemented",
                "ElPack Demo", 0);
  }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
  ItemDragging = Tree->ItemFocused;
  DragObject = new TElDragObject(Tree);

}
//---------------------------------------------------------------------------

void __fastcall TMainForm::TreeValidateInplaceEdit(TObject *Sender,
      TElTreeItem *Item, TElHeaderSection *Section, AnsiString &Text,
      bool &Accept)
{

  MessageBox(0, "Sorry, renaming a file is not implemented","ElPack Demo", 0);
  Accept = false;

}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FillTree(TElTreeItem* Item, AnsiString Path) {

  TSearchRec    SRec;
  TElTreeItem*  TSI;
  bool          b;
  AnsiString    s, FName;
  int           hn;
  HICON         IconHandle;
  char*         p;
  TSHFileInfo   SHFI;
  TIcon*        Icon ;


  s = Path;

  if (*(s.AnsiLastChar()) != '\\') { s = s + "\\"; }
  b = (FindFirst(s+"*.*", faAnyFile, SRec) == 0);
  while (b) {
    if ( (strcmp(SRec.FindData.cFileName,".") != 0) &&
         (strcmp(SRec.FindData.cFileName,"..") != 0) ) {
      TSI = Tree->Items->AddItem(Item);
      TSI->Text = SRec.FindData.cFileName;
      FName = s + SRec.FindData.cFileName;
      TSI->ColumnText->Add(FName);
      TSI->ColumnText->Add(IntToStr(SRec.Size));
      TSI->ColumnText->Add(DateToStr(FileDateToDateTime(SRec.Time)));
      TSI->ColumnText->Add(TimeToStr(FileDateToDateTime(SRec.Time)));
      if ((faDirectory & SRec.Attr)>0) {
        TSI->ParentStyle = false;
        TSI->Bold = true;
        TSI->ForceButtons = true;
      }
      if ((faHidden & SRec.Attr)>0) {
        TSI->ParentStyle = false;
        TSI->Italic = true;
        TSI->ParentColors = false;
        TSI->Color = clGray;
        TSI->BkColor = Tree->BkColor;
        TSI->UseBkColor = false;
      }
      if ((FILE_ATTRIBUTE_COMPRESSED & SRec.FindData.dwFileAttributes) > 0) {
        TSI->ParentColors = false;
        TSI->Color = clBlue;
        TSI->BkColor = Tree->BkColor;
        TSI->UseBkColor = false;
      }
//      GetMem(p, 260);
      p = (char *) malloc(260);
      StrPCopy(p, FName);
      SHGetFileInfo(p, 0, &SHFI, sizeof(SHFI), 0x400 | 0x200 | 0x100 | 4 | 1);
      IconHandle = SHFI.hIcon;
      if (IconHandle != 0) {
        hn = Hash->GetIndex(SHFI.szTypeName);
        if ((hn == -1) ||
           (strcmp(SHFI.szTypeName,"Application")==0) ||
           (strcmp(SHFI.szTypeName,"Icon")==0)) {
          Icon = new TIcon();
          Icon->Handle = IconHandle;
          TSI->ImageIndex = Images->AddIcon(Icon);
          Hash->AddItem(SHFI.szTypeName, &(TSI->ImageIndex));
        }
        else {
          TSI->ImageIndex = *((int *) Hash->GetByIndex(hn));
        }
        TSI->StateImageIndex = TSI->ImageIndex;
      }
      free(p);
    }
    b = (FindNext(SRec) == 0);
  }
  FindClose(SRec);
}


void __fastcall  TMainForm::FillRoots()
{
    TElTreeItem* TSI;
    DWORD DrivesMask;
    AnsiString s;
    TIcon* Icon;
    HICON IconHandle;
    char* p;
    TSHFileInfo SHFI;

  DrivesMask = GetLogicalDrives();
  for (int i=0; i < 25; i++ ) {
    if (((DrivesMask >> i) % 2) == 1) {

      s = char(i+65);
      s = s+":";
      TSI = Tree->Items->AddItem(NULL);
      TSI->ParentStyle = false;
      TSI->Bold = true;
      TSI->ColumnText->Add(s+"\\");
      TSI->ForceButtons = true;
//      GetMem(p, 260);
      p = (char *) malloc(260);
      StrPCopy(p, s+"\\");
      SHGetFileInfo(p, 0, &SHFI, sizeof(SHFI), 0x400 | 0x200 | 0x100 | 4 | 1);
      IconHandle = SHFI.hIcon;
      if ( IconHandle != 0 ) {
        Icon = new TIcon();
        Icon->Handle = IconHandle;
        TSI->ImageIndex = Images->AddIcon(Icon);
        TSI->StateImageIndex = TSI->ImageIndex;
//        Icon->Free;
      }
      free(p);
      if (strlen(SHFI.szDisplayName)>0) {
        s = StrPas(SHFI.szDisplayName);
      }
      TSI->Text = s;
    }
  }
}


void __fastcall TMainForm::TreeShowLineHint(TObject *Sender,
      TElTreeItem *Item, TElFString &Text, THintWindow *HintWindow,
      tagPOINT &MousePos, bool &DoShowHint)
{
  Text = Item->ColumnText->Strings[0];
}
//---------------------------------------------------------------------------

