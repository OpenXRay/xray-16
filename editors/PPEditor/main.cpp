#include "stdafx.h"
#pragma hdrstop
#include "main.h"
#include "color.h"
#include "single_param.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "MXCtrls"
#pragma resource "*.dfm"

TMainForm *MainForm;


//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TForm(Owner)
{
    frmConstructor 							= new TfrmConstructor(this);
    Image->Picture->Bitmap->Width 			= Image->Width;
    Image->Picture->Bitmap->Height 			= Image->Height;

    m_Animator 								= new CPostprocessAnimator (0, false);
    m_ActiveShowForm 						= NULL;

    m_props[tAddColor]						= new TAddColorForm(this, pp_add_color);
    m_props[tAddColor]->GetForm()->Parent 	= WorkArea;
    m_props[tAddColor]->GetForm()->Visible 	= false;
    
    m_props[tBaseColor]						= new TAddColorForm(this,pp_base_color);
    m_props[tBaseColor]->GetForm()->Parent 	= WorkArea;
    m_props[tBaseColor]->GetForm()->Visible = false;

    m_props[tGray]							= new TAddColorForm(this,pp_gray_color);
    m_props[tGray]->GetForm()->Parent 		= WorkArea;
    m_props[tGray]->GetForm()->Visible 		= false;

    m_props[tDuality]						= new TAddFloatForm(this,pp_dual_h);
    m_props[tDuality]->GetForm()->Parent 	= WorkArea;
    m_props[tDuality]->GetForm()->Visible 	= false;

    m_props[tNoise]							= new TAddFloatForm(this,pp_noise_i);
    m_props[tNoise]->GetForm()->Parent 		= WorkArea;
    m_props[tNoise]->GetForm()->Visible 	= false;

    m_props[tBlur]							= new TAddColorForm(this,pp_gray_color);
    m_props[tBlur]->GetForm()->Parent 		= WorkArea;
    m_props[tBlur]->GetForm()->Visible 		= false;

    m_props[tColorMap]						= new TAddFloatForm(this,pp_cm_influence);
    m_props[tColorMap]->GetForm()->Parent 	= WorkArea;
    m_props[tColorMap]->GetForm()->Visible 	= false;

    m_Marker 								= 0.0f;

    TabControl->TabIndex 					= 0;
    TabControlChange 						(TabControl);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormDestroy(TObject *Sender)
{
    delete m_Animator;
    m_Animator = NULL;
    delete frmConstructor;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormResize(TObject *Sender)
{
    Image->Picture->Bitmap->Width = 0;
    Image->Picture->Bitmap->Height = 0;
    Image->Picture->Bitmap->Width = Image->Width;
    Image->Picture->Bitmap->Height = Image->Height;
    UpdateGraph ();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::NewEffectButtonClick(TObject *Sender)
{
    if (Application->MessageBox ("Do you wish to create a new effect ?", "Warning", MB_YESNO | MB_ICONSTOP) == IDNO)
       return;
       
    m_Animator->Create ();   
    UpdateGraph ();

     m_props[tAddColor]->Clear();
     m_props[tBaseColor]->Clear();
     m_props[tGray]->Clear();
     m_props[tDuality]->Clear();
     m_props[tNoise]->Clear();
     m_props[tBlur]->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormPaint(TObject *Sender)
{
    UpdateGraph ();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ImageMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
//    m_pEffect->add_point (X, Y);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::GrayColorClick(TObject *Sender)
{
//    m_pEffect->get_add_color()->show_constructor ();
}
//---------------------------------------------------------------------------
void TMainForm::PointListSetTime(int idx, float time)
{
    string256 str;
    sprintf(str,"%2.2f",time);
    PointList->Items->Strings[idx] = str;
}

void TMainForm::FillPointList()
{
    PointList->Clear();
   	CPostProcessParam* cparam 	= MainForm->m_Animator->GetParam (m_ActiveShowForm->GetTimeChannel());
    u32 cnt = cparam->get_keys_count();
    for(u32 i=0; i<cnt;++i)
    {
        PointList->AddItem("", NULL);
    	float t = cparam->get_key_time(i);
        PointListSetTime(PointList->Count-1, t);
    }
}

void __fastcall TMainForm::TabControlChange(TObject *Sender)
{
    if (m_ActiveShowForm)
       {
       m_ActiveShowForm->GetForm()->Visible = false;
       m_ActiveShowForm 					= NULL;
       }
    m_ActiveShowForm = m_props[TabControl->TabIndex];
    
    m_ActiveShowForm->GetForm()->Parent 	= WorkArea;
    m_ActiveShowForm->GetForm()->Left 	= 0;
    m_ActiveShowForm->GetForm()->Top 		= 0;
    m_ActiveShowForm->GetForm()->Width 	= WorkArea->Width;
    m_ActiveShowForm->GetForm()->Height 	= WorkArea->Height;
    m_ActiveShowForm->GetForm()->Visible 	= true;

    FillPointList					();
    
    if(PointList->Count)
		PointList->ItemIndex 		= 0;    
        
	PointListClick					(NULL);
    UpdateGraph 					();
}
//---------------------------------------------------------------------------
void            TMainForm::SetMarkerPosition   (float time)
{
    m_Marker = time;
    UpdateGraph ();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::LoadButtonClick(TObject *Sender)
{
/*
    if (m_Animator->GetLength() != 0.0f)
       if (Application->MessageBox ("Do you wish to save current effect ?", "Warning", MB_YESNO | MB_ICONSTOP) == IDYES)
          SaveButtonClick (Sender);
*/          
    if(OpenDialog->Execute ())
    {
        m_Marker = 0.0f;
        m_Animator->Load 	(OpenDialog->FileName.c_str ());
        Caption 			= OpenDialog->FileName.c_str ();

        TabControl->TabIndex 				= 0;
        TabControlChange 					(TabControl);
    }
}

void __fastcall TMainForm::SaveButtonClick(TObject *Sender)
{
    if (m_Animator->GetLength() == 0.0f) return;
    if (SaveDialog->Execute())
    {
    	m_Animator->Save (SaveDialog->FileName.c_str ());
        Caption = SaveDialog->FileName.c_str ();
    }
}

void TMainForm::UpdateGraph()
{
	if(!m_Animator)				return;
    TCanvas *canvas 			 = Image->Picture->Bitmap->Canvas;
    canvas->Brush->Color 		 = clWhite;
    canvas->FillRect 			 (TRect (0, 0, Image->Picture->Bitmap->Width, Image->Picture->Bitmap->Height));
    canvas->Pen->Color 			 = clBlack;
    canvas->MoveTo 				 (0, Image->Picture->Bitmap->Height / 2);
    canvas->LineTo               (Image->Picture->Bitmap->Width, Image->Picture->Bitmap->Height / 2);
    canvas->MoveTo               (0, 0);
    canvas->LineTo               (Image->Picture->Bitmap->Width, 0);
    canvas->MoveTo               (0, Image->Picture->Bitmap->Height - 1);
    canvas->LineTo               (Image->Picture->Bitmap->Width, Image->Picture->Bitmap->Height - 1);
    canvas->TextOutA             (0, Image->Picture->Bitmap->Height / 2 - 14, "0.0");
    canvas->TextOutA             (0, Image->Picture->Bitmap->Height - 14, "-1.0");
    canvas->TextOutA             (0, 2, "1.0");


    float alltime 				= m_Animator->GetLength ();
    string128 					buf;
    sprintf 					(buf, "Effect time : %.3f", alltime);
    StatusBar->Panels->Items[0]->Text = buf;
    if (alltime == 0.0f) 		return;
    
    float width 				= (float)Image->Picture->Bitmap->Width, height = (float)Image->Picture->Bitmap->Height * 0.5f;
    //draw marker
    int left 					= (int)(width / alltime * m_Marker);
    canvas->MoveTo 				(left, 0);
    canvas->Pen->Style 			= psDot;
    canvas->LineTo 				(left, Image->Picture->Bitmap->Height);
    canvas->Pen->Style 			= psSolid;

    SPPInfo						m_EffectorParams;
    ZeroMemory 					(&m_EffectorParams, sizeof (SPPInfo));

    float increment 			= alltime / (float)Image->Width;

    for (float t = 0.0f; t < alltime; t += increment)
    {
        m_Animator->Process 	(t, m_EffectorParams);
        int x 					= (int)(width / alltime * t);
        
        for(u32 idx =pp_base_color;idx<pp_last;++idx)
        {
            if(!m_ActiveShowForm->DrawChannel((_pp_params)idx))
                continue;
            
            switch (idx)
                   {
                   case pp_base_color:
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_base.r * height + height)] = clRed;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_base.g * height + height)] = clGreen;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_base.b * height + height)] = clBlue;
                        break;
                   case pp_add_color:
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_add.r * height + height)] = clRed;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_add.g * height + height)] = clGreen;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_add.b * height + height)] = clBlue;
                        break;
                   case pp_gray_color:
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_gray.r * height + height)] = clRed;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_gray.g * height + height)] = clGreen;
                        canvas->Pixels[x][(int)(-m_EffectorParams.color_gray.b * height + height)] = clBlue;
                        break;
                   case pp_gray_value:
                        canvas->Pixels[x][(int)(-m_EffectorParams.blur * height + height)] = clBlack;
                        break;
                   case pp_blur:
                        canvas->Pixels[x][(int)(-m_EffectorParams.gray * height + height)] = clBlack;
                        break;
                   case pp_dual_h:
                        canvas->Pixels[x][(int)(-m_EffectorParams.duality.h * height + height)] = clRed;
                        break;
                   case pp_dual_v:
                        canvas->Pixels[x][(int)(-m_EffectorParams.duality.v * height + height)] = clGreen;
                        break;
                   case pp_noise_i:
                        canvas->Pixels[x][(int)(-m_EffectorParams.noise.intensity * height + height)] = clRed;
                        break;
                   case pp_noise_g:
                        canvas->Pixels[x][(int)(-m_EffectorParams.noise.grain * height + height)] = clGreen;
                        break;
                   case pp_noise_f:
                        canvas->Pixels[x][(int)(-m_EffectorParams.noise.fps * height + height)] = clBlue;
                        break;
                   }//switch
           	}//channel
    }//time
}

//---------------------------------------------------------------------------

void __fastcall TMainForm::PointListClick(TObject *Sender)
{
	m_ActiveShowForm->Lock(true);
	m_ActiveShowForm->ShowCurrent(PointList->ItemIndex);
	m_ActiveShowForm->Lock(false);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::btnAddKeyClick(TObject *Sender)
{
	int idx 						= PointList->ItemIndex;
    m_ActiveShowForm->Lock			(true);
	m_ActiveShowForm->AddNew		(PointList->ItemIndex);
	m_ActiveShowForm->Lock			(false);

    TabControlChange 				(TabControl);
	PointList->ItemIndex 			= idx+1;
    PointListClick					(NULL);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::btnRemoveKeyClick(TObject *Sender)
{
	int idx 						= PointList->ItemIndex;
	m_ActiveShowForm->Lock			(true);
	m_ActiveShowForm->Remove		(PointList->ItemIndex);
	m_ActiveShowForm->Lock			(false);

    TabControlChange 				(TabControl);
    if(PointList->Count>idx)
		PointList->ItemIndex 			= idx;
    else
		PointList->ItemIndex 			= idx-1;
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::ClearAllClick(TObject *Sender)
{
	m_ActiveShowForm->Lock			(true);
	m_ActiveShowForm->RemoveAllKeys	();
	m_ActiveShowForm->Lock			(false);

    TabControlChange 				(TabControl);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::copyFromClick(TObject *Sender)
{
	TPoint P;	
	P = Mouse->CursorPos;
	PopupCopyFrom->Popup(P.x, P.y);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::PopupCopyFromChange(TObject *Sender,
      TMenuItem *Source, bool Rebuild)
{
//	
}
//---------------------------------------------------------------------------


void __fastcall TMainForm::PopupClick(TObject *Sender)
{
  //
  TMenuItem* itm = dynamic_cast<TMenuItem*>(Sender);

    m_ActiveShowForm->Lock			(true);
    m_ActiveShowForm->RemoveAllKeys	();

   	CPostProcessParam* cparam 		= MainForm->m_Animator->GetParam (m_props[itm->Tag]->GetTimeChannel());
    u32 cnt 						= cparam->get_keys_count();
    string256 str;
    for(u32 i=0; i<cnt;++i)
    {
    	float t = cparam->get_key_time(i);
        m_ActiveShowForm->CreateKey	(t);
    }
    m_ActiveShowForm->Lock			(false);

	TabControlChange				(NULL);
}
//---------------------------------------------------------------------------





