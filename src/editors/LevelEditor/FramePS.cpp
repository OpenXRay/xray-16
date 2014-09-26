#include "stdafx.h"
#pragma hdrstop

#include "../ECore/Editor/ui_main.h"
#include "FramePS.h"
#include "..\..\Layers\xrRender\PSLibrary.h"
#include "Scene.h"
#include "EParticlesObject.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraPS::TfraPS(TComponent* Owner)
        : TForm(Owner)
{
    DEFINE_INI(fsStorage);
    m_Current = 0;
}
//---------------------------------------------------------------------------
void TfraPS::OnItemFocused(ListItemsVec& items)
{
	VERIFY(items.size()<=1);
    m_Current 			= 0;
    for (ListItemsIt it=items.begin(); it!=items.end(); it++)
        m_Current 		= (*it)->Key();
    ExecCommand			(COMMAND_RENDER_FOCUS);
}
//------------------------------------------------------------------------------
void __fastcall TfraPS::PaneMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::ExpandClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Selecting
//---------------------------------------------------------------------------
void __fastcall TfraPS::ebSelectByRefsClick(TObject *Sender)
{
	SelByRef( true );
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::ebDeselectByRefsClick(TObject *Sender)
{
	SelByRef( false );
}
                                                                
void __fastcall TfraPS::SelByRef( bool flag )
{
	if(m_Current){
		ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
        ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
		for(;_F!=_E;_F++){
			if( (*_F)->Visible() ){
				EParticlesObject *_O = (EParticlesObject *)(*_F);
				if(_O->RefCompare(m_Current)) _O->Select( flag );
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::ebCurrentPSPlayClick(TObject *Sender)
{
    ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
    ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
    for(;_F!=_E;_F++){
        if( (*_F)->Visible() && (*_F)->Selected())
            ((EParticlesObject *)(*_F))->Play();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::ebCurrentPSStopClick(TObject *Sender)
{
    ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
    ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
    for(;_F!=_E;_F++){
        if( (*_F)->Visible() && (*_F)->Selected())
            ((EParticlesObject *)(*_F))->Stop();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::FormHide(TObject *Sender)
{
    m_Items->SaveSelection	(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::FormShow(TObject *Sender)
{
    m_Items->LoadSelection	(fsStorage);
    ListItemsVec items;
    for (PS::PEDIt E=::Render->PSLibrary.FirstPED(); E!=::Render->PSLibrary.LastPED(); E++){
    	ListItem* I=LHelper().CreateItem(items,*(*E)->m_Name,0,0,*E);
        I->SetIcon(1);
    }
    for (PS::PGDIt G=::Render->PSLibrary.FirstPGD(); G!=::Render->PSLibrary.LastPGD(); G++){
    	ListItem* I=LHelper().CreateItem(items,*(*G)->m_Name,0,0,*G);
        I->SetIcon(2);
    }
    m_Items->AssignItems	(items,false,true);
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::FormCreate(TObject *Sender)
{
    m_Items 				= TItemList::CreateForm("Particles",paItems, alClient, 0);
    m_Items->SetImages		(ilModeIcons);
    m_Items->SetOnItemsFocusedEvent(TOnILItemsFocused(this,&TfraPS::OnItemFocused));
}
//---------------------------------------------------------------------------

void __fastcall TfraPS::FormDestroy(TObject *Sender)
{
	TItemList::DestroyForm	(m_Items);
}
//---------------------------------------------------------------------------

