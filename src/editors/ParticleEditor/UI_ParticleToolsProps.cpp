//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "UI_ParticleTools.h"

void CParticleTool::OnDrawUI()
{
    if (m_LibPED)
        m_LibPED->OnDrawUI();
    if (m_CreatingParticle)
    {
        bool change;
        shared_str result;
        if (UIChooseForm::GetResult(change, result))
        {
            if (change)
            {
                if (result == "Effect")
                {
                    AppendPE(0, m_CreatingParticlePath.c_str());
                }
                else
                {
                    AppendPG(0, m_CreatingParticlePath.c_str());
                }
            }
            m_CreatingParticle = FALSE;
        }
        UIChooseForm::Update();
    }
}
void CParticleTool::FillChooseParticleType(ChooseItemVec &items, void *param)
{
    items.push_back(SChooseItem("Effect", "Effect Patricle"));
    items.push_back(SChooseItem("Group", "Group Patricle"));
}
void CParticleTool::OnParticleCreateItem(LPCSTR path)
{
    UIChooseForm::SelectItem(smCustom, 1, 0, TOnChooseFillItems(this, &CParticleTool::FillChooseParticleType), 0, 0, 0, 0);
    m_CreatingParticle = TRUE;
    m_CreatingParticlePath = path;
}
void CParticleTool::OnParticleCloneItem(LPCSTR parent_path, LPCSTR new_full_name)
{
    PS::CPEDef *PE = FindPE(parent_path);
    if (PE)
    {
        AppendPE(PE, new_full_name);
        Modified();
    }
    else
    {
        PS::CPGDef *PG = FindPG(parent_path);
        if (PG)
        {
            AppendPG(PG, new_full_name);
            Modified();
        }
    }
}

void CParticleTool::OnParticleItemRename(LPCSTR old_name, LPCSTR new_name, EItemType type)
{
    Rename(old_name, new_name);
    Modified();
}

void CParticleTool::OnParticleItemRemove(LPCSTR name, EItemType type)
{
    Remove(name);
    Modified();
}

void CParticleTool::OnControlClick(ButtonValue *sender, bool &bDataModified, bool &bSafe)
{
    m_Transform.identity();
    bDataModified = false;
}

void CParticleTool::OnParticleItemFocused(ListItem *items)
{
    PropItemVec props;
    m_EditMode = emEffect;

    ButtonValue *B;
    B = PHelper().CreateButton(props, "Transform\\Edit", "Reset", ButtonValue::flFirstOnly);
    B->OnBtnClickEvent = ButtonValue::TOnBtnClick(this, &CParticleTool::OnControlClick);
    PHelper().CreateFlag32(props, "Transform\\Type", &m_Flags, flSetXFORM, "Update", "Set");

    // reset to default
    ResetCurrent();

    if (items)
    {

        ListItem *item = items;
        if (item)
        {
            m_EditMode = EEditMode(item->Type());
            switch (m_EditMode)
            {
            case emEffect:
            {
                PS::CPEDef *def = ((PS::CPEDef *)item->m_Object);
                SetCurrentPE(def);
                def->FillProp(EFFECT_PREFIX, props, item);
            }
            break;
            case emGroup:
            {
                PS::CPGDef *def = ((PS::CPGDef *)item->m_Object);
                SetCurrentPG(def);
                def->FillProp(GROUP_PREFIX, props, item);
            }
            break;
            default:
                THROW;
            }
        }
    }
    m_ItemProps->AssignItems(props);
    UI->RedrawScene();
}

//------------------------------------------------------------------------------
extern xr_string _item_to_select_after_edit;

void CParticleTool::RealUpdateProperties()
{
    m_Flags.set(flRefreshProps, FALSE);

    ListItemsVec items;
    {
        PS::PEDIt Pe = ::Render->PSLibrary.FirstPED();
        PS::PEDIt Ee = ::Render->PSLibrary.LastPED();
        for (; Pe != Ee; Pe++)
        {
            ListItem *I = LHelper().CreateItem(items, *(*Pe)->m_Name, emEffect, 0, *Pe);
            I->SetIcon(1);
        }
    }
    {
        PS::PGDIt Pg = ::Render->PSLibrary.FirstPGD();
        PS::PGDIt Eg = ::Render->PSLibrary.LastPGD();
        for (; Pg != Eg; Pg++)
        {
            ListItem *I = LHelper().CreateItem(items, *(*Pg)->m_Name, emGroup, 0, *Pg);
            I->SetIcon(2);
        }
    }
    m_PList->AssignItems(items, false, true);
    if (_item_to_select_after_edit.size())
    {
        m_PList->SelectItem(_item_to_select_after_edit.c_str());
        _item_to_select_after_edit = "";
    }
    else
    {
        if (m_EditPG && m_EditPG->GetDefinition())
            m_PList->SelectItem(m_EditPG->Name().c_str());
        if (m_EditPE && m_EditPE->GetDefinition())
            m_PList->SelectItem(m_EditPE->Name().c_str());
    }
}
