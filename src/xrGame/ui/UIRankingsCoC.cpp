#include "StdAfx.h"

#include "UIRankingsCoC.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "../ai_space.h"

CUIRankingsCoC::CUIRankingsCoC(CUIScrollView* parent)
    : CUIWindow("CUIRankingsCoC"), m_parent(parent) {}

CUIRankingsCoC::~CUIRankingsCoC()
{
    xr_delete(m_hint);
}

void CUIRankingsCoC::init_from_xml(CUIXml& xml, u8 index, bool bUnique)
{
    string128 tmp;
    xr_sprintf(tmp, sizeof(tmp), "%s", bUnique ? "coc_ranking_itm_actor" : "coc_ranking_itm");

    CUIXmlInit::InitWindow(xml, tmp, 0, this);

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE node = xml.NavigateToNode(tmp, 0);

    xml.SetLocalRoot(node);

    m_index = index;
    m_name  = UIHelper::CreateStatic(xml, "name", this);
    m_descr = UIHelper::CreateStatic(xml, "descr", this);
    m_icon  = UIHelper::CreateStatic(xml, "icon", this);
    m_hint  = UIHelper::CreateHint(xml, "hint_wnd");
    //m_border = UIHelper::CreateStatic(xml, "border", this);

    xml.SetLocalRoot(stored_root);
    Show(false);
}
void CUIRankingsCoC::Update()
{
    //if (ParentHasMe())
    //    return;

    luabind::functor<bool> functorCanShow;
    if (GEnv.ScriptEngine->functor("pda.coc_rankings_can_show", functorCanShow))
    {
        if (functorCanShow(m_index))
        {
            if (!ParentHasMe())
            {
                luabind::functor<pcstr> functorSetName;
                if (GEnv.ScriptEngine->functor("pda.coc_rankings_set_name", functorSetName))
                    SetName(functorSetName(m_index));

                luabind::functor<pcstr> functorSetDescription;
                if (GEnv.ScriptEngine->functor("pda.coc_rankings_set_description", functorSetDescription))
                    SetDescription(functorSetDescription(m_index));
                luabind::functor<pcstr> functorSetHint;
                if (GEnv.ScriptEngine->functor("pda.coc_rankings_set_hint", functorSetHint))
                    SetHint(functorSetHint(m_index));

                luabind::functor<pcstr> functorSetIcon;
                if (GEnv.ScriptEngine->functor("pda.coc_rankings_set_icon", functorSetIcon))
                    SetIcon(functorSetIcon(m_index));

                /*
                luabind::functor<pcstr> functorShowBorder;
                if (GEnv.ScriptEngine->functor("pda.coc_rankings_show_border", functorShowBorder))
                {
                    if (functorShowBorder(m_index))
                    {
                        if (!m_border->IsShown())
                            m_border->Show(true);
                    }
                    else
                    {
                        if (m_border->IsShown())
                            m_border->Show(false);
                    }
                }
                */

                m_parent->AddWindow(this, false);
                if (!IsShown())
                    Show(true);
            }
        }
        else
        {
            if (ParentHasMe())
            {
                m_parent->RemoveWindow(this);
                if (IsShown())
                    Show(false);
            }
        }
    }
}
bool CUIRankingsCoC::ParentHasMe() const
{
    const auto& items = m_parent->Items();

    const auto it = std::find(items.begin(), items.end(), this);
    return it != items.end();
}

void CUIRankingsCoC::SetName(pcstr name) const
{
    m_name->TextItemControl()->SetColoringMode(true);
    m_name->TextItemControl()->SetUseNewLineMode(true);
    m_name->SetText(name);
}

void CUIRankingsCoC::SetDescription(pcstr desc)
{
    m_descr->TextItemControl()->SetColoringMode(true);
    m_descr->TextItemControl()->SetUseNewLineMode(true);
    m_descr->SetText(desc);
    m_descr->AdjustHeightToText();
    Fvector2 descr_size = m_descr->GetWndSize();
    descr_size.y += 30.0f;
    if(descr_size.y>GetWndSize().y)
        SetWndSize(Fvector2().set(GetWndSize().x, descr_size.y));
}

void CUIRankingsCoC::SetHint(pcstr hint) const
{
    m_hint->set_text(CStringTable().translate(hint).c_str());
}

void CUIRankingsCoC::SetIcon(pcstr icon) const
{
    if (!xr_strcmp(icon, ""))
        return;

    m_icon->InitTexture(icon);
}

void CUIRankingsCoC::DrawHint()
{
    Frect r;
    GetAbsoluteRect(r);
    Fvector2 pos = UI().GetUICursor().GetCursorPosition();
    if (r.in(pos))
        m_hint->Draw();
}

void CUIRankingsCoC::Reset()
{
    if (ParentHasMe())
    {
        m_parent->RemoveWindow(this);
        Show(false);
    }
    inherited::Reset();
}
