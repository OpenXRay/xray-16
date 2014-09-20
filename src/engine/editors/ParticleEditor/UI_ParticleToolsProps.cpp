//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "UI_ParticleTools.h"
#include "UI_ParticleMain.h"
#include "../xrEProps/TextForm.h"
#include "../xrEProps/ItemList.h"

void CParticleTool::OnParticleItemRename(LPCSTR old_name, LPCSTR new_name, EItemType type)
{
	Rename			(old_name,new_name);
	Modified		();
}

void CParticleTool::OnParticleItemRemove(LPCSTR name, EItemType type, bool& res)
{
	Remove			(name);
    Modified		();
    res				= true;
}

void  CParticleTool::OnControlClick(ButtonValue* sender, bool& bDataModified, bool& bSafe)
{
	m_Transform.identity();
    bDataModified	= false;
}

void CParticleTool::OnParticleItemFocused(ListItemsVec& items)
{
	PropItemVec props;
	m_EditMode	= emEffect;

    ButtonValue* B;
	B=PHelper().CreateButton	(props,"Transform\\Edit",	"Reset",	ButtonValue::flFirstOnly);
    B->OnBtnClickEvent.bind		(this,&CParticleTool::OnControlClick);
    PHelper().CreateFlag32		(props,"Transform\\Type",	&m_Flags,	flSetXFORM,"Update","Set");

    // reset to default
    ResetCurrent	();

	if (!items.empty()){
	    for (ListItemsIt it=items.begin(); it!=items.end(); it++){
            ListItem* item = *it;
            if (item){
                m_EditMode 			= EEditMode(item->Type());
                switch(m_EditMode){
                case emEffect:{
                    PS::CPEDef* def	= ((PS::CPEDef*)item->m_Object);
                	SetCurrentPE	(def);
                    def->FillProp	(EFFECT_PREFIX,props,item);
                }break;
                case emGroup:{
                    PS::CPGDef* def	= ((PS::CPGDef*)item->m_Object);
                	SetCurrentPG	(def);
                    def->FillProp	(GROUP_PREFIX,props,item);
                }break;
                default: THROW;
                }
            }
        }
    }
	m_ItemProps->AssignItems(props);
    UI->RedrawScene();
}
//------------------------------------------------------------------------------
extern AnsiString _item_to_select_after_edit;

void CParticleTool::RealUpdateProperties()
{
	m_Flags.set(flRefreshProps,FALSE);

	ListItemsVec items;
    {
        PS::PEDIt Pe = ::Render->PSLibrary.FirstPED();
        PS::PEDIt Ee = ::Render->PSLibrary.LastPED();
        for (; Pe!=Ee; Pe++){
            ListItem* I=LHelper().CreateItem(items,*(*Pe)->m_Name,emEffect,0,*Pe);
            I->SetIcon(1);
        }
	}
    {
        PS::PGDIt Pg = ::Render->PSLibrary.FirstPGD();
        PS::PGDIt Eg = ::Render->PSLibrary.LastPGD();
        for (; Pg!=Eg; Pg++){
            ListItem* I=LHelper().CreateItem(items,*(*Pg)->m_Name,emGroup,0,*Pg);
            I->SetIcon(2);
        }
	}
	m_PList->AssignItems(items,false,true);
    if(_item_to_select_after_edit.Length())
    {
    	m_PList->SelectItem(_item_to_select_after_edit.c_str(),true,false,true);
        _item_to_select_after_edit = "";
    }
    
}

