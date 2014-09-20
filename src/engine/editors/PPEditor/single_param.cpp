#include "stdafx.h"
#pragma hdrstop

#include "single_param.h"
#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "MXCtrls"
#pragma link "multi_edit"
#pragma link "MXCtrls"
#pragma resource "*.dfm"
__fastcall TAddFloatForm::TAddFloatForm(TComponent* Owner, pp_params p)
    : TForm(Owner),m_pp_params(p)
{
    Value1->MinValue = -1;
    Value1->MaxValue = 1;
    Value1->Decimal = 2;
    Value2->Decimal = 2;
	Value1->Increment = 0.01f;    
	Value2->Increment = 0.01f;
    cmTextureName->Visible = false;
	if(m_pp_params==pp_dual_h)
    {
    	Label1->Caption = "Duality-H";
    	Label2->Caption = "Duality-V";
        Label2->Visible = true;
        Value2->Visible = true;
        Value1->MinValue = -1.0f;
        Value1->MaxValue = 1.0f;
        Value1->Decimal = 3;
        Value1->Increment = 0.001f;    

        Value2->MinValue = -1.0f;
        Value2->MaxValue = 1.0f;
        Value2->Decimal = 3;
        Value2->Increment = 0.001f;    
    }else
	if(m_pp_params==pp_noise_i)
    {
    	Label1->Caption = "Noise Intensity";
    	Label2->Caption = "Noise Grain";
    	Label3->Caption = "Noise FPS";

        Label2->Visible = true;
        Value2->Visible = true;
        Label3->Visible = true;
        Value3->Visible = true;

        Value1->MinValue = 0.0f;;
        Value1->MaxValue = 1.0f;;
        Value2->MinValue = 0.01f;
        Value2->MaxValue = 1000.0f;
        Value3->MinValue = 1.0f;;
        Value3->MaxValue = 1000.0f;;
    }else
	if(m_pp_params==pp_cm_influence)
    {
    	Label1->Caption = "Influence";

        Label2->Visible = false;
        Value2->Visible = false;
        Label3->Visible = false;
        Value3->Visible = false;

        Value1->MinValue = 0.0f;;
        Value1->MaxValue = 1.0f;;
        cmTextureName->Visible = true;
    };
}
//---------------------------------------------------------------------------
void    TAddFloatForm::Clear()
{
	Lock(true);
    TimeValue->Value 	= 0.0f;
    Value1->Value 		= 0.0f;
    Value2->Value 		= 0.0f;
    Value3->Value 		= 0.0f;
    cmTextureName->Text = "";
	Lock(false);
}

void TAddFloatForm::ShowCurrent(u32 keyIdx)
{
	if(keyIdx==-1)
    {
    	Clear();
        return;
    }

   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   float time 					= cparam->get_key_time(keyIdx);
   MainForm->SetMarkerPosition	(time);

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
   Value1->Value 				= val;

	if(m_pp_params==pp_dual_h)
    {
       cparam 						= MainForm->m_Animator->GetParam (pp_dual_v);

       cparam->get_value			(time, val, 0);
       Value2->Value 				= val;
    }else
	if(m_pp_params==pp_noise_i)
    {
       cparam 						= MainForm->m_Animator->GetParam (pp_noise_g);


       cparam->get_value			(time, val, 0);
       Value2->Value 				= val;

       cparam 						= MainForm->m_Animator->GetParam (pp_noise_f);

       cparam->get_value			(time, val, 0);
       Value3->Value 				= val;
    }else
	if(m_pp_params==pp_cm_influence)
    {
    	cmTextureName->Text = MainForm->m_Animator->PPinfo().cm_tex1.c_str();
    }
}

bool TAddFloatForm::DrawChannel(_pp_params p)
{
	switch(m_pp_params)
    {
    	case pp_dual_h:			return (p==pp_dual_h)||(p==pp_dual_v);
    	case pp_noise_i:     	return (p==pp_noise_i)||(p==pp_noise_g)||(p==pp_noise_f);
    	case pp_cm_influence:   return (p==pp_cm_influence);
        default:R_ASSERT(0);
    }
    return false;
}

void __fastcall TAddFloatForm::TimeValueExit(TObject *Sender)
{
    if (m_bLocked) 								return;
    if (MainForm->PointList->ItemIndex == -1) 	return;
	TMultiObjSpinEdit* spin 					= dynamic_cast<TMultiObjSpinEdit*>(Sender);
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
        cparam->add_value 					(time, Value1->Value, 0);

    
        if(m_pp_params==pp_dual_h)
        {
            cparam 							= MainForm->m_Animator->GetParam (pp_dual_v);
            cparam->delete_value			(time_prev);
            cparam->add_value 				(time, Value2->Value, 0);
        }else
        if(m_pp_params==pp_noise_i)
        {
            cparam 							= MainForm->m_Animator->GetParam (pp_noise_g);
            cparam->delete_value 			(time_prev);
            cparam->add_value 				(time, Value2->Value, 0);

            cparam 							= MainForm->m_Animator->GetParam (pp_noise_f);
            cparam->delete_value 			(time_prev);
            cparam->add_value 				(time, Value3->Value, 0);
        }

        MainForm->PointListSetTime			(MainForm->PointList->ItemIndex, time);
        MainForm->UpdateGraph();
        spin->Color 					= clWindow;
    }
}

void __fastcall TAddFloatForm::ChangeParam(TObject *Sender)
{
    if (m_bLocked) 								return;
    if (MainForm->PointList->ItemIndex == -1) 	return;
	TMultiObjSpinEdit* spin = dynamic_cast<TMultiObjSpinEdit*>(Sender);
    spin->Color = clLime;
}

void TAddFloatForm::AddNew(u32 keyIdx)
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   
   float time 					= (keyIdx!=-1)?cparam->get_key_time(keyIdx):0.0f;
   MainForm->SetMarkerPosition	(time);
   time							+= 0.01f;
   CreateKey					(time);

}

void TAddFloatForm::Remove(u32 keyIdx)
{
	if(keyIdx==-1)				return;
    
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   
   float time 					= cparam->get_key_time(keyIdx);

   cparam->delete_value 	 	(time);
   
   if(m_pp_params==pp_dual_h)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_dual_v);
   		cparam->delete_value 	(time);
   }else
   if(m_pp_params==pp_noise_i)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_noise_g);
   		cparam->delete_value 	(time);
        cparam 					= MainForm->m_Animator->GetParam (pp_noise_f);
   		cparam->delete_value 	(time);
   };
}

void  TAddFloatForm::RemoveAllKeys()
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());
   cparam->clear_all_keys		();

   if(m_pp_params==pp_dual_h)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_dual_h);
   		cparam->clear_all_keys		();
   }else
   if(m_pp_params==pp_noise_i)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_noise_g);
   		cparam->clear_all_keys		();

        cparam 					= MainForm->m_Animator->GetParam (pp_noise_f);
   		cparam->clear_all_keys		();
   }
}

void  TAddFloatForm::CreateKey(float time)
{
   CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (GetTimeChannel());

   cparam->add_value 			(time, 0.0f, 0);
	cparam->update_value	(time, 0.33f, 0);

   
   if(m_pp_params==pp_dual_h)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_dual_v);
   		cparam->add_value 		(time, 0.0f, 0);
		cparam->update_value	(time, 0.44f, 0);
   }
   if(m_pp_params==pp_noise_i)
   {
        cparam 					= MainForm->m_Animator->GetParam (pp_noise_g);
   		cparam->add_value 		(time, 0.0f, 0);
   		cparam->update_value	(time, 0.11f, 0);

        cparam 					= MainForm->m_Animator->GetParam (pp_noise_f);
   		cparam->add_value 		(time, 0.0f, 0);
   		cparam->update_value	(time, 0.22f, 0);
   }
}
//---------------------------------------------------------------------------

void __fastcall TAddFloatForm::TimeValueKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if(Key==VK_RETURN)
    	TimeValueExit(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TAddFloatForm::cmTextureNameChange(TObject *Sender)
{
    MainForm->m_Animator->PPinfo().cm_tex1 = cmTextureName->Text.c_str();
}
//---------------------------------------------------------------------------

