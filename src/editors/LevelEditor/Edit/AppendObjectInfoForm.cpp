#include "stdafx.h"
#pragma hdrstop

#include "AppendObjectInfoForm.h"
#include "CustomObject.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TfrmAppendObjectInfo* 		g_frmConflictLoadObject = NULL;
//---------------------------------------------------------------------------
__fastcall TfrmAppendObjectInfo::TfrmAppendObjectInfo(TComponent* Owner)
	: TForm(Owner)
{
	m_result 					= 4;//auto-rename all
}

void TfrmAppendObjectInfo::Prepare()
{
    StaticText2->Caption 		= m_existing_object->Name;
}    

void __fastcall TfrmAppendObjectInfo::btOverwriteClick(TObject *Sender)
{
	TButton* bt = (TButton*)Sender;
	m_result = bt->Tag;
    if(CheckBox1->Checked)
		m_result	+= 1;
            
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------

