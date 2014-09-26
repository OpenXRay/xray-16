#include "stdafx.h"
#pragma hdrstop
#include "Color.h"
#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "MXCtrls"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TAddColorForm::TAddColorForm(TComponent* Owner, _pp_params p)
    : TForm(Owner),m_pp_params(p),m_bLocked(false)
{
	if(m_pp_params==pp_base_color)
    {
    	RedValue->MinValue 		= -1.0f;
        GreenValue->MinValue 	= -1.0f; 
        BlueValue->MinValue 	= -1.0f;
        
    	RedValue->MaxValue 		= 1.0f;
        GreenValue->MaxValue 	= 1.0f;
        BlueValue->MaxValue 	= 1.0f;
    }else
	if(m_pp_params==pp_add_color)
    {
    	RedValue->MinValue 		= -1.0f;
        GreenValue->MinValue 	= -1.0f;
        BlueValue->MinValue 	= -1.0f;
        
    	RedValue->MaxValue 		= 1.0f;
        GreenValue->MaxValue 	= 1.0f;
        BlueValue->MaxValue 	= 1.0f;
    }else
	if(m_pp_params==pp_gray_color)
    {
        labelIntensity->Visible = true;
        IntensityValue->Visible = true;
        
    	RedValue->MinValue 		= -1.0f;
        GreenValue->MinValue 	= -1.0f;
        BlueValue->MinValue 	= -1.0f;
        
    	RedValue->MaxValue 		= 1.0f;
        GreenValue->MaxValue 	= 1.0f;
        BlueValue->MaxValue 	= 1.0f;

        IntensityValue->MinValue	= 0.0f;
        IntensityValue->MaxValue    = 1.0f;
    }
}
//---------------------------------------------------------------------------
/*
{
    if (m_bLocked) 								return;
    if (MainForm->PointList->ItemIndex == -1) 	return;
    
    CPostProcessParam* cparam 			= MainForm->m_Animator->GetParam (GetTimeChannel());

    float time_prev						= cparam->get_key_time(MainForm->PointList->ItemIndex);
    cparam->delete_value				(time_prev);
    
    float time 						= TimeValue->Value;

    cparam->add_value 				(time, RedValue->Value, 0);
    cparam->add_value 				(time, GreenValue->Value, 1);
    cparam->add_value 				(time, BlueValue->Value, 2);

	if(m_pp_params==pp_gray_color)
    {
       	cparam 						= MainForm->m_Animator->GetParam (pp_gray_value);
    	cparam->delete_value		(time_prev);
       	cparam->add_value 			(time, IntensityValue->Value, 0);
    }

    MainForm->PointListSetTime		(MainForm->PointList->ItemIndex, time);
	MainForm->UpdateGraph			();
}
*/
void __fastcall TAddColorForm::TimeValueExit(TObject *Sender)
{
    if (m_bLocked) 								return;
    if (MainForm->PointList->ItemIndex == -1) 	return;
	TMultiObjSpinEdit* spin = dynamic_cast<TMultiObjSpinEdit*>(Sender);
    if(spin->Color == clLime)
    {
    	if(TimeValue->Value<TimeValue->MinValue)
        	TimeValue->Value=TimeValue->MinValue;
    	if(TimeValue->Value>TimeValue->MaxValue)
        	TimeValue->Value=TimeValue->MaxValue;

        CPostProcessParam* cparam 			= MainForm->m_Animator->GetParam (GetTimeChannel());

        float time_prev						= cparam->get_key_time(MainForm->PointList->ItemIndex);
        cparam->delete_value				(time_prev);
    
        float time 							= TimeValue->Value;

        cparam->add_value 				(time, RedValue->Value, 0);
        cparam->add_value 				(time, GreenValue->Value, 1);
        cparam->add_value 				(time, BlueValue->Value, 2);

        if(m_pp_params==pp_gray_color)
        {
            cparam 						= MainForm->m_Animator->GetParam (pp_gray_value);
            cparam->delete_value		(time_prev);
            cparam->add_value 			(time, IntensityValue->Value, 0);
        }

        MainForm->PointListSetTime		(MainForm->PointList->ItemIndex, time);
        MainForm->UpdateGraph			();
        TimeValue->Color 				= clWindow;
        RedValue->Color 				= clWindow;
        GreenValue->Color 				= clWindow;
        BlueValue->Color 				= clWindow;
        IntensityValue->Color 			= clWindow;
    }
}

void __fastcall TAddColorForm::CnahgeParam(TObject *Sender)
{
    if (m_bLocked) 								return;
    if (MainForm->PointList->ItemIndex == -1) 	return;
	TMultiObjSpinEdit* spin = dynamic_cast<TMultiObjSpinEdit*>(Sender);
    spin->Color = clLime;
}

//---------------------------------------------------------------------------
void    TAddColorForm::Clear()
{
    Lock(true);
    TimeValue->Value 		= 0.0f;
    RedValue->Value 		= 0.0f;
    GreenValue->Value 		= 0.0f;
    BlueValue->Value 		= 0.0f;
    IntensityValue->Value	= 0.0f;
    Lock(false);
}


void TAddColorForm::ShowCurrent(u32 keyIdx)
{
	if(keyIdx==-1)
    {
    	Clear();
        return;
    }
        
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   float time 					= cparam->get_key_time(keyIdx);
   MainForm->SetMarkerPosition(time);

   if(keyIdx>0)
   	TimeValue->MinValue			= cparam->get_key_time(keyIdx-1)+0.02f;
   else 
   	TimeValue->MinValue			= 0.0f;


   if(cparam->get_keys_count()>keyIdx+1)
   	TimeValue->MaxValue			= cparam->get_key_time(keyIdx+1)-0.02f;
   else
   	TimeValue->MaxValue			= 1000.0f;

   TimeValue->Value 			= time;
   
   float 						val;
   cparam->get_value			(time, val, 0);
   RedValue->Value 				= val;
   cparam->get_value			(time, val, 1);
   GreenValue->Value 			= val;
   cparam->get_value			(time, val, 2);
   BlueValue->Value 			= val;
   if(m_pp_params==pp_gray_color)
   {
       CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (pp_gray_value);

       cparam->get_value			(time, val, 0);
       IntensityValue->Value 		= val;
   }
}

bool TAddColorForm::DrawChannel(_pp_params p)		
{
	switch(m_pp_params)
    {
    	case pp_add_color:		return (p==pp_add_color);
    	case pp_base_color:     return (p==pp_base_color);
    	case pp_gray_color:     return (p==pp_gray_color)||(p==pp_gray_value);
        default:R_ASSERT(0);
    }
    return false;
};

void TAddColorForm::AddNew(u32 keyIdx)
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());

   float time 					= (keyIdx!=-1)?cparam->get_key_time(keyIdx):0.0f;
   
   MainForm->SetMarkerPosition	(time);
   time							+= 0.01f;
   CreateKey					(time);

}

void TAddColorForm::Remove(u32 keyIdx)
{
	if(keyIdx==-1)				return;
    
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   
   float time 					= cparam->get_key_time(keyIdx);

   cparam->delete_value 	 	(time);
   
   if(m_pp_params==pp_gray_color)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_gray_value);
   		cparam->delete_value 	(time);
   }
}

void TAddColorForm::RemoveAllKeys()
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   cparam->clear_all_keys		();

   if(m_pp_params==pp_gray_color)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_gray_value);
   cparam->clear_all_keys		();
   }
}
void  TAddColorForm::CreateKey(float time)
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   
   cparam->add_value 			(time, 0.0f, 0);
   cparam->add_value 			(time, 0.0f, 1);
   cparam->add_value 			(time, 0.0f, 2);

   
   if(m_pp_params==pp_gray_color)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_gray_value);
   		cparam->add_value 		(time, 0.0f, 0);
   }
}

void __fastcall TAddColorForm::ColorClick(TObject *Sender)
{
      float l 					= _abs((float)RedValue->MaxValue - (float)RedValue->MinValue);

      float kl					= l/255.0f;

	  ColorDialog->Color 		= color_rgba(	(BlueValue->Value 	- BlueValue->MinValue)/kl , 
      											(GreenValue->Value 	- GreenValue->MinValue)/kl, 
                                                (RedValue->Value 	- RedValue->MinValue)/kl, 
                                                0.0f);
      if (ColorDialog->Execute() )
      {
      	TColor clr 				= ColorDialog->Color;

        
		RedValue->Value 		= RedValue->MinValue 	+ color_get_B(clr)*kl;
		GreenValue->Value 		= GreenValue->MinValue 	+ color_get_G(clr)*kl;
		BlueValue->Value 		= BlueValue->MinValue 	+ color_get_R(clr)*kl; // /255

        TimeValueExit			(RedValue);
      }
}

//---------------------------------------------------------------------------

void __fastcall TAddColorForm::TimeValueKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if(Key==VK_RETURN)
    	TimeValueExit(Sender);
}
//---------------------------------------------------------------------------

