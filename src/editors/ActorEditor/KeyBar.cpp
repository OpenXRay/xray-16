#include "stdafx.h"
#pragma hdrstop

#include "KeyBar.h"
#include "UI_ActorTools.h"
#include "UI_ActorMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "Gradient"
#pragma link "ElTrackBar"
#pragma link "ElXPThemedControl"
#pragma link "MXCtrls"
#pragma link "multi_edit"
#pragma link "ExtBtn"
#pragma link "ElBtnCtl"
#pragma link "ElCheckCtl"
#pragma link "ElPopBtn"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TfrmKeyBar::TfrmKeyBar(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
TfrmKeyBar* TfrmKeyBar::CreateKeyBar(TWinControl* parent)
{
	TfrmKeyBar* B = xr_new<TfrmKeyBar>(parent);
    B->Parent= parent;
    B->Align = alBottom;
    B->Show	();
    return B;
}

void __fastcall TfrmKeyBar::seLODLWChange(TObject *Sender, int Val)
{
	ATools->m_RenderObject.m_fLOD = seLOD->Value;
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::seLODKeyPress(TObject *Sender, char &Key)
{
	if (Key==VK_RETURN)	ATools->m_RenderObject.m_fLOD = seLOD->Value;
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::seLODExit(TObject *Sender)
{
	ATools->m_RenderObject.m_fLOD = seLOD->Value;
}
//---------------------------------------------------------------------------

void TfrmKeyBar::UpdateBar()
{
	seLOD->Value 	        = ATools->m_RenderObject.m_fLOD;
    float a,b,c;
    ATools->GetStatTime     (a,b,c);
    stStartTime->Caption   	= AnsiString().sprintf("%3.2f",a);
    stEndTime->Caption   	= AnsiString().sprintf("%3.2f",b);
    lbCurrentTime->Caption	= AnsiString().sprintf("%3.2f",c);
    anm_track->Min          = iFloor(a*1000);
    anm_track->Max          = iFloor(b*1000);

	if(auto_ch->Checked)
	    anm_track->Position     = iFloor(c*1000);

	if(m_currentEditMotion	!= ATools->GetCurrentMotion())
    {
        PanelCh1->Repaint();
        PanelCh2->Repaint();
        PanelCh3->Repaint();
        PanelCh4->Repaint();
    }

   	m_currentEditMotion	= ATools->GetCurrentMotion();

    bool bMarksPresent12 = (m_currentEditMotion && m_currentEditMotion->marks.size()>=2);
    bool bMarksPresent34 = (m_currentEditMotion && m_currentEditMotion->marks.size()==4);

    PanelCh1->Visible		= bMarksPresent12 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar12 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar34;
    PanelCh2->Visible		= bMarksPresent12 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar12 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar34;
    PanelCh3->Visible		= bMarksPresent34 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar34;
    PanelCh4->Visible		= bMarksPresent34 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar34;

    PanelCh1->Enabled		= bMarksPresent12;
    PanelCh2->Enabled		= bMarksPresent12;
    PanelCh3->Enabled		= bMarksPresent34;
    PanelCh4->Enabled		= bMarksPresent34;

    int h = 24;
    if(bMarksPresent34 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar34)
    	h+=60 ; //(4x15)
    else
    if(bMarksPresent12 || ((CAEPreferences*)EPrefs)->bAlwaysShowKeyBar12)
    	h+=30 ; //(2x15)



	if(ClientHeight != h)
	    ClientHeight = h;

}

void TfrmKeyBar::draw_marks (int i)
{
	if(m_currentEditMotion	!= ATools->GetCurrentMotion())
    {
        PanelCh1->Repaint();
        PanelCh2->Repaint();
        PanelCh3->Repaint();
        PanelCh4->Repaint();
    }

   	m_currentEditMotion	= ATools->GetCurrentMotion();

    if(!m_currentEditMotion)
    	return;

     if(m_currentEditMotion->marks.size()==0)
		return;

     if(m_currentEditMotion->marks.size() < (u32)(i+1) )
		return;

	 motion_marks& M 		= m_currentEditMotion->marks[i];

    TMxPanel* P 			= NULL;
    if(i==0)
    	P = PanelCh1;
    else
    if(i==1)
    	P = PanelCh2;
    else
    if(i==2)
    	P = PanelCh3;
    else
    if(i==3)
    	P = PanelCh4;
	R_ASSERT				(P);

    TCanvas* C 				= P->Canvas;

    Ivector2 				start_offset;
    TPoint					_shift_track;
    _shift_track			= anm_track->ClientToScreen(TPoint(0,0));

    TPoint					_shift_panel;
    _shift_panel			= P->ClientToScreen(TPoint(0,0));


    start_offset.set		(_shift_track.x + anm_track->OffsetLeft - _shift_panel.x, 1);
    Ivector2 border_size;
    border_size.set			(anm_track->Width - anm_track->OffsetLeft - anm_track->OffsetRight, 13);
    C->Pen->Style			= psSolid;
    C->Pen->Color			= Graphics::clGray;
    C->Rectangle			(	start_offset.x,
    							start_offset.y,
                                start_offset.x+border_size.x,
                                start_offset.y+border_size.y);


    float a,b,c;
    ATools->GetStatTime     (a,b,c);
	float motion_length 	= b-a;
    float k_len				= border_size.x/motion_length;

	motion_marks::C_ITERATOR it 	= M.intervals.begin();
	motion_marks::C_ITERATOR it_e 	= M.intervals.end();

    for( ;it!=it_e; ++it)
    {
    	const motion_marks::interval& iv	= *it;
    	Ivector2	posLT,posRB;
		posLT.set		(iFloor(iv.first*k_len), 2);
		posRB.set		(iFloor(iv.second*k_len), 11);

        posLT.add				(start_offset);
        posRB.add				(start_offset);

        C->Pen->Color			= Graphics::clRed;
        C->Brush->Color			= Graphics::clGreen;
        C->Rectangle			(	posLT.x,
                                    posLT.y,
                                    posRB.x,
                                    posRB.y);
    }
}

bool interval_comparer(const motion_marks::interval& i1, const motion_marks::interval& i2)
{
	return (i1.first<i2.first);
}

void TfrmKeyBar::set_mark	(int id, int action)
{
   	m_currentEditMotion	= ATools->GetCurrentMotion();

    if(!m_currentEditMotion)
    	return;

     if(m_currentEditMotion->marks.size()==0)
		return;

	motion_marks& M 		= m_currentEditMotion->marks[id];
    float a,b,c;
    ATools->GetStatTime     (a,b,c);
    float cur_time			= c-a;

    motion_marks::ITERATOR it 		= M.intervals.begin();
    motion_marks::ITERATOR it_e 	= M.intervals.end();

    if(action==3)
    { //del current

        for( ;it!=it_e; ++it)
        {
    		motion_marks::interval& iv	= *it;
            if(iv.first<cur_time && iv.second>cur_time)
            {
                M.intervals.erase(it);
                break;
            }
        }
    }else
    if(action==2)
    {//up
        for( ;it!=it_e; ++it)
        {
    		motion_marks::interval& iv	= *it;
            if(iv.first<cur_time && iv.second>cur_time)
            {
                iv.second	= cur_time;
                break;
            }
        }
    }else
    if(action==1)
    {//down
        for( ;it!=it_e; ++it)
        {
    		motion_marks::interval& iv	= *it;
            if(iv.first<cur_time && iv.second>cur_time)
            {
                iv.first	= cur_time;
                break;
            }
        }
        if(it==it_e)
        {//insert new
           M.intervals.push_back(motion_marks::interval(cur_time, b-a));
        }
    }

	std::sort				(M.intervals.begin(), M.intervals.end(), interval_comparer);
	PanelCh1->Repaint		();
    PanelCh2->Repaint		();
    PanelCh3->Repaint		();
    PanelCh4->Repaint		();
}

void __fastcall TfrmKeyBar::BtnUpCh1Click(TObject *Sender)
{
	  set_mark(0,2);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnUpCh2Click(TObject *Sender)
{
	  set_mark(1,2);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnUpCh3Click(TObject *Sender)
{
	  set_mark(2,2);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnUpCh4Click(TObject *Sender)
{
	  set_mark(3,2);
}

//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDownCh1Click(TObject *Sender)
{
	  set_mark(0,1);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDownCh2Click(TObject *Sender)
{
	  set_mark(1,1);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDownCh3Click(TObject *Sender)
{
	  set_mark(2,1);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDownCh4Click(TObject *Sender)
{
	  set_mark(3,1);
}

//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDelCh1Click(TObject *Sender)
{
	  set_mark(0,3);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDelCh2Click(TObject *Sender)
{
	  set_mark(1,3);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDelCh3Click(TObject *Sender)
{
	  set_mark(2,3);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::BtnDelCh4Click(TObject *Sender)
{
	  set_mark(3,3);
}

//---------------------------------------------------------------------------
void __fastcall TfrmKeyBar::PanelCh1Paint(TObject *Sender)
{
		draw_marks(0);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::PanelCh2Paint(TObject *Sender)
{
   		draw_marks(1);
}


void __fastcall TfrmKeyBar::PanelCh3Paint(TObject *Sender)
{
   		draw_marks(2);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::PanelCh4Paint(TObject *Sender)
{
   		draw_marks(3);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::spinTimeFactorExit(TObject *Sender)
{
	 EDevice.time_factor(spinTimeFactor->Value);
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::spinTimeFactorKeyPress(TObject *Sender,
      char &Key)
{
	if (Key==VK_RETURN)
    {
	 EDevice.time_factor(spinTimeFactor->Value);
     }
}
//---------------------------------------------------------------------------

void __fastcall TfrmKeyBar::spinTimeFactorLWChange(TObject *Sender,
      int Val)
{
	 EDevice.time_factor(spinTimeFactor->Value);
}
//---------------------------------------------------------------------------

