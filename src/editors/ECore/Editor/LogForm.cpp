#include "stdafx.h"
#pragma hdrstop

#include "LogForm.h"
#include "ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma link "MxMenus"
#pragma resource "*.dfm"
TfrmLog *TfrmLog::form=0;
//---------------------------------------------------------------------------
__fastcall TfrmLog::TfrmLog(TComponent* Owner)
    : TForm(Owner)
{
	DEFINE_INI(fsStorage);
}
//---------------------------------------------------------------------------
void __fastcall TfrmLog::CreateLog(){
	VERIFY(!form);
	form = xr_new<TfrmLog>((TComponent*)0);
}
void __fastcall TfrmLog::DestroyLog(){
	xr_delete(form);
}
void __fastcall TfrmLog::ShowLog(){
	VERIFY(form);
	form->Show();
}
void __fastcall TfrmLog::HideLog(){
	VERIFY(form);
	form->Close();    
}

void __fastcall TfrmLog::ebClearClick(TObject *Sender)
{
	lbLog->Items->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::AddDlgMessage(TMsgDlgType mt, const AnsiString& msg)
{        
    ExecCommand(COMMAND_RENDER_FOCUS);
    MessageDlg(msg, mt, TMsgDlgButtons() << mbOK, 0);
	AddMessage(mt,msg);
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::AddMessage(TMsgDlgType mt, const AnsiString& msg)
{
	if (!form) return;
    AnsiString M;
    for (int i=1; i<=msg.Length(); i++){
    	if (msg[i]=='\r') continue;
        if (msg[i]=='\n') M+=" ";
        else M+=msg[i];
    }
	form->lbLog->Items->AddObject(M,(TObject*)mt);
    form->lbLog->ItemIndex = form->lbLog->Items->Count-1;
    if ((mt==mtError)&&!form->Visible) form->Show();
}
//---------------------------------------------------------------------------

#define MSG_ERROR 	0x00C4C4FF
#define MSG_INFO  	0x00E6FFE7
#define MSG_CONF 	0x00FFE6E7
#define MSG_DEF  	0x00E8E8E8

void __fastcall TfrmLog::lbLogDrawItem(TWinControl *Control, int Index,
      TRect &Rect, TOwnerDrawState State)
{
	TListBox* lb = ((TListBox *)Control);
	TCanvas *pCanvas = lb->Canvas;
    if (!State.Contains(odSelected)){
		pCanvas->Brush->Color 	= TColor(MSG_DEF);
	    TMsgDlgType mt 			= (TMsgDlgType)lb->Items->Objects[Index];
	    switch(mt){
	    case mtError: 			pCanvas->Brush->Color=TColor(MSG_ERROR);break;
	    case mtInformation:     pCanvas->Brush->Color=TColor(MSG_INFO); break;
	    case mtConfirmation: 	pCanvas->Brush->Color=TColor(MSG_CONF);	break;
    	}
    }
	pCanvas->FillRect(Rect);
  	int     Offset = 2;   // default text offset width
	pCanvas->TextOut(Rect.Left + Offset, Rect.Top, lb->Items->Strings[Index]);
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (Key==VK_ESCAPE)	Close();
    else{
    	UI->ApplyGlobalShortCut(Key, Shift);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::ebClearSelectedClick(TObject *Sender)
{
	for (int i = 0; i < lbLog->Items->Count; i++){
    	if (lbLog->Selected[i]){
        	lbLog->Items->Delete(i);
            i--;
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::ebCloseClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::FormShow(TObject *Sender)
{
	// check window position
	UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::ebFlushClick(TObject *Sender)
{
	FlushLog();	
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::lbLogKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if (Shift.Contains(ssCtrl)&&(Key=='C')){
    	imCopyClick		(Sender);
    }else if (Shift.Contains(ssCtrl)&&(Key=='A')){
    	imSelectAllClick(Sender);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::imCopyClick(TObject *Sender)
{
    TClipboard* clp	= Clipboard();
    clp->Clear		();
    xr_string		tmp;
    for (int i = 0; i < lbLog->Items->Count; i++)
        if (lbLog->Selected[i]){
            tmp		= tmp+lbLog->Items->Strings[i].c_str()+"\r\n";
        }
    clp->SetTextBuf	((LPSTR)tmp.c_str());
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::imSelectAllClick(TObject *Sender)
{
	lbLog->SelectAll();
}
//---------------------------------------------------------------------------

void __fastcall TfrmLog::lbLogKeyPress(TObject *Sender, char &Key)
{
	Key = 0;	
}
//---------------------------------------------------------------------------


