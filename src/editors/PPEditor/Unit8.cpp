//---------------------------------------------------------------------------

#include <vcl.h>
#include <stdio.h>
#pragma hdrstop
#include "Unit8.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm8 *Form8;
//---------------------------------------------------------------------------
__fastcall TForm8::TForm8(TComponent* Owner)
    : TForm(Owner)
{
    m_iTag = 0;
    AddEntryTemplate (-1);
}
//---------------------------------------------------------------------------
void    TForm8::AddEntryTemplate    (int iInsertAfter)
{
    TForm9 *form = new TForm9 (this);
    form->Parent = this;
    form->Left = 0;
    form->Tag = m_iTag++;
    form->Visible = true;
    form->AddButton->Tag = form->Tag;
    form->DeleteButton->Tag = form->Tag;
    form->AddButton->OnClick = OnAddButtonClick;
    form->DeleteButton->OnClick = OnDelButtonClick;
    vector<TForm9*> temp;
    iInsertAfter++;
    if (!m_Entries.size () || (int)m_Entries.size () == iInsertAfter)
       {
       m_Entries.push_back (form);
       }
    else
       {
       for (int a = 0; a < (int)m_Entries.size (); a++)
           {
           if (a == iInsertAfter)
              temp.push_back (form);
           temp.push_back (m_Entries[a]);
           }
       m_Entries.assign (temp.begin (), temp.end ());
       }
    ResetPositions ();
}
//---------------------------------------------------------------------------
void    TForm8::ResetPositions      ()
{
    int top = 0;
    for (size_t a = 0; a < m_Entries.size (); a++)
        {
        m_Entries[a]->Left = 0;
        m_Entries[a]->Top = (m_Entries[a]->Height + 4) * a + Panel2->Height;
        top = m_Entries[a]->Top;
        }
    Button1->Top = top + m_Entries[0]->Height + 4;
    Button2->Top = Button1->Top;
    if (m_Entries.size () <= 6)
       ClientHeight = Button1->Top + Button1->Height;
}
//---------------------------------------------------------------------------
void    __fastcall  TForm8::OnAddButtonClick   (TObject *Sender)
{
    int tag = dynamic_cast<TComponent*> (Sender)->Tag;
    for (size_t a = 0; a < m_Entries.size (); a++)
        if (m_Entries[a]->Tag == tag)
           {
           AddEntryTemplate (a);
           return;
           }
}
//---------------------------------------------------------------------------
void    __fastcall  TForm8::OnDelButtonClick   (TObject *Sender)
{
    if (m_Entries.size () == 1) return;
    int tag = dynamic_cast<TControl*> (Sender)->Parent->Parent->Tag;
    vector<TForm9*>::iterator s = m_Entries.begin(), e = m_Entries.end ();
    for (; s != e; ++s)
        if ((*s)->Tag == tag)
           {
           TForm9 *form = (*s);
           delete form;
           m_Entries.erase (s);
           RecalcSize ();
           return;
           }
}
//---------------------------------------------------------------------------
void                TForm8::RecalcSize          ()
{
    int top;
    for (int a = 0; a < (int)m_Entries.size (); a++)
        {
        m_Entries[a]->Top = a * m_Entries[a]->Height + 2;
        top = m_Entries[a]->Top;
        }
    Button1->Top = top + m_Entries[0]->Height + 4;
    Button2->Top = Button1->Top;
    if (m_Entries.size () <= 6)
       ClientHeight = Button1->Top + Button1->Height;
}
//---------------------------------------------------------------------------

