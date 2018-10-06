#include "pch_script.h"
#include "UIAchievements.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "xrUICore/Hint/UIHint.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Cursor/UICursor.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "string_table.h"

CUIAchievements::CUIAchievements(CUIScrollView* parent) : m_parent(parent) {}
CUIAchievements::~CUIAchievements() { xr_delete(m_hint); }
void CUIAchievements::init_from_xml(CUIXml& xml)
{
    CUIXmlInit::InitWindow(xml, "achievements_itm", 0, this);

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE node = xml.NavigateToNode("achievements_itm", 0);
    xml.SetLocalRoot(node);

    m_name = UIHelper::CreateTextWnd(xml, "name", this);
    m_descr = UIHelper::CreateTextWnd(xml, "descr", this);
    m_icon = UIHelper::CreateStatic(xml, "icon", this);
    m_hint = UIHelper::CreateHint(xml, "hint_wnd");

    xml.SetLocalRoot(stored_root);
    Show(false);
}
void CUIAchievements::Update()
{
    if (ParentHasMe() && !m_repeat)
        return;

    luabind::functor<bool> f;
    R_ASSERT(GEnv.ScriptEngine->functor(m_functor_str, f));
    if (f())
    {
        if (!ParentHasMe())
        {
            m_parent->AddWindow(this, false);
            Show(true);
        }
    }
    else
    {
        if (ParentHasMe())
        {
            m_parent->RemoveWindow(this);
            Show(false);
        }
    }
}
bool CUIAchievements::ParentHasMe()
{
    WINDOW_LIST::const_iterator it = std::find(m_parent->Items().begin(), m_parent->Items().end(), this);
    return it != m_parent->Items().end();
}
void CUIAchievements::SetName(LPCSTR name) { m_name->SetTextST(name); }
void CUIAchievements::SetDescription(LPCSTR desc)
{
    m_descr->SetTextST(desc);
    m_descr->AdjustHeightToText();
    Fvector2 descr_size = m_descr->GetWndSize();
    descr_size.y += 30.0f;
    if (descr_size.y > GetWndSize().y)
        SetWndSize(Fvector2().set(GetWndSize().x, descr_size.y));
}

void CUIAchievements::SetHint(LPCSTR hint) { m_hint->set_text(StringTable().translate(hint).c_str()); }
void CUIAchievements::SetIcon(LPCSTR icon) { m_icon->InitTexture(icon); }
void CUIAchievements::SetFunctor(LPCSTR func)
{
    //	string128 str = "xr_statistic.";
    xr_sprintf(m_functor_str, sizeof(m_functor_str), "%s", func);
}

void CUIAchievements::SetRepeatable(bool repeat) { m_repeat = repeat; }
void CUIAchievements::DrawHint()
{
    Frect r;
    GetAbsoluteRect(r);
    Fvector2 pos = UI().GetUICursor().GetCursorPosition();
    if (r.in(pos))
        m_hint->Draw();
}

void CUIAchievements::Reset()
{
    if (ParentHasMe())
    {
        m_parent->RemoveWindow(this);
        Show(false);
    }
    inherited::Reset();
}
