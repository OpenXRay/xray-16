#include "stdafx.h"
#pragma hdrstop
#include "float_constructor.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmConstructor *frmConstructor;
//---------------------------------------------------------------------------
__fastcall TfrmConstructor::TfrmConstructor(TComponent* Owner)
    : TForm(Owner),b_locked(false)
{
    m_iTag 								= 0;
}
//---------------------------------------------------------------------------
TfrmTimeConstructor*    TfrmConstructor::AddEntryTemplate(int iInsertAfterIdx)
{
    TfrmTimeConstructor* form 	= new TfrmTimeConstructor (this);
    form->Parent 				= this;
    form->Left 					= 0;
    form->Tag 					= m_iTag++;
    form->Visible 				= true;
    form->AddButton->Tag 		= form->Tag;
    form->DeleteButton->Tag 	= form->Tag;
    form->AddButton->OnClick 	= OnAddButtonClick;
    form->DeleteButton->OnClick = OnDelButtonClick;
	form->WorkTime->OnChange	= OnTimeChangeClick;


    if(iInsertAfterIdx==-1)
       m_Entries.push_back 		(form);
     else
     {
    	vector<TfrmTimeConstructor*>::iterator it = m_Entries.begin();
        std::advance(it,iInsertAfterIdx+1);
        m_Entries.insert(it,form);
     }  

    return form;
}
//---------------------------------------------------------------------------
void    TfrmConstructor::UpdatePositions()
{
	if(b_locked)	return;
	float start_time = 0.0f;
    for (size_t a = 0; a < m_Entries.size (); ++a)
        {
        	m_Entries[a]->StartTime->Value 	= start_time;
        	m_Entries[a]->Left 				= 0;
        	m_Entries[a]->Top 				= (m_Entries[a]->Height + 4) * a;
            start_time						+= m_Entries[a]->WorkTime->Value;
        }
    ClientHeight = m_Entries.size()*(30+4) + Button1->Height;
    Button1->Top = ClientHeight-Button1->Height;
    Button2->Top = ClientHeight-Button2->Height;
}

void    __fastcall  TfrmConstructor::OnTimeChangeClick(TObject *Sender)
{
  UpdatePositions();
}

//---------------------------------------------------------------------------
void    __fastcall  TfrmConstructor::OnAddButtonClick   (TObject *Sender)
{
    int tag = dynamic_cast<TComponent*> (Sender)->Tag;
    for (size_t a = 0; a < m_Entries.size (); a++)
        if (m_Entries[a]->Tag == tag)
           {
           AddEntryTemplate (a);
    		UpdatePositions ();
           return;
           }
}
//---------------------------------------------------------------------------
void __fastcall  TfrmConstructor::OnDelButtonClick   (TObject *Sender)
{
    if (m_Entries.size () == 1) return;
    int tag = dynamic_cast<TControl*> (Sender)->Parent->Parent->Tag;
    vector<TfrmTimeConstructor*>::iterator s = m_Entries.begin(), e = m_Entries.end ();
    for (; s != e; ++s)
        if ((*s)->Tag == tag)
           {
           TfrmTimeConstructor *form = (*s);
           delete form;
           m_Entries.erase (s);
           UpdatePositions ();
           return;
           }
}
//---------------------------------------------------------------------------
void __fastcall  TfrmConstructor::Reset()
{
    vector<TfrmTimeConstructor*>::iterator it 	= m_Entries.begin();
    vector<TfrmTimeConstructor*>::iterator it_e = m_Entries.end ();
    for (; it!=it_e; ++it)
    {
        TfrmTimeConstructor* form = (*it);
        delete 				form;
    }
    m_Entries.clear				();

    AddEntryTemplate 			(-1);
    m_Entries[0]->WorkTime->Value = 0.0f;

}

//---------------------------------------------------------------------------
TfrmTimeConstructor* __fastcall  TfrmConstructor::GetEntry(u32 index)
{
    if (index >= m_Entries.size())
       throw Exception ("Float constructor: invalid index");
    return m_Entries[index];
}
//---------------------------------------------------------------------------

