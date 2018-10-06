#include "pch.hpp"
#include "Windows/UIWindow.h"
#include "UIStatic.h"

#include "XML/UITextureMaster.h"
#include "xrEngine/LightAnimLibrary.h"
#include "Lines/UILines.h"
#include "Include/xrRender/UIRender.h"
#include "Buttons/UIBtnHint.h"
#include "Cursor/UICursor.h"

bool is_in2(const Frect& b1, const Frect& b2);

void lanim_cont::set_defaults()
{
    m_lanim = NULL;
    m_lanim_start_time = -1.0f;
    m_lanim_delay_time = 0.0f;
    m_lanimFlags.zero();
}
void lanim_cont_xf::set_defaults()
{
    lanim_cont::set_defaults();
    m_origSize.set(0, 0);
}

CUIStatic::CUIStatic()
    : m_bTextureEnable(true), m_bStretchTexture(false), m_bHeading(false), m_bConstHeading(false), m_fHeading(0.0f),
      m_pTextControl(NULL)
{
    m_TextureOffset.set(0.0f, 0.0f);
    m_lanim_xform.set_defaults();
}

CUIStatic::~CUIStatic() { xr_delete(m_pTextControl); }
void CUIStatic::SetXformLightAnim(LPCSTR lanim, bool bCyclic)
{
    if (lanim && lanim[0] != 0)
        m_lanim_xform.m_lanim = LALib.FindItem(lanim);
    else
        m_lanim_xform.m_lanim = NULL;

    m_lanim_xform.m_lanimFlags.zero();

    m_lanim_xform.m_lanimFlags.set(LA_CYCLIC, bCyclic);
    m_lanim_xform.m_origSize = GetWndSize();
}

void CUIStatic::InitTexture(LPCSTR texture) { InitTextureEx(texture); }
void CUIStatic::CreateShader(const char* tex, const char* sh) { m_UIStaticItem.CreateShader(tex, sh); }
void CUIStatic::InitTextureEx(LPCSTR tex_name, LPCSTR sh_name)
{
    LPCSTR res_shname = GEnv.UIRender->UpdateShaderName(tex_name, sh_name);
    CUITextureMaster::InitTexture(tex_name, &m_UIStaticItem, res_shname);

    Fvector2 p = GetWndPos();
    m_UIStaticItem.SetPos(p.x, p.y);
}

void CUIStatic::Draw()
{
    DrawTexture();
    inherited::Draw();
    DrawText();
}

void CUIStatic::DrawText()
{
    if (m_pTextControl)
    {
        if (!fsimilar(m_pTextControl->m_wndSize.x, m_wndSize.x) || !fsimilar(m_pTextControl->m_wndSize.y, m_wndSize.y))
        {
            m_pTextControl->m_wndSize = m_wndSize;
            m_pTextControl->ParseText(true);
        }

        Fvector2 p;
        GetAbsolutePos(p);
        m_pTextControl->Draw(p.x, p.y);
    }
    if (g_statHint->Owner() == this)
        g_statHint->Draw_();
}

#include "Include/xrRender/UIShader.h"

void CUIStatic::DrawTexture()
{
    if (m_bTextureEnable && GetShader() && GetShader()->inited())
    {
        Frect rect;
        GetAbsoluteRect(rect);
        m_UIStaticItem.SetPos(rect.left + m_TextureOffset.x, rect.top + m_TextureOffset.y);

        if (m_bStretchTexture)
        {
            if (Heading())
            {
                if (m_UIStaticItem.GetFixedLTWhileHeading())
                {
                    float t1, t2;
                    t1 = rect.width();
                    t2 = rect.height();
                    rect.y2 = rect.y1 + t1;
                    rect.x2 = rect.x1 + t2;
                }
            }
            m_UIStaticItem.SetSize(Fvector2().set(rect.width(), rect.height()));
        }
        else
        {
            Frect r = {0.0f, 0.0f, m_UIStaticItem.GetTextureRect().width(), m_UIStaticItem.GetTextureRect().height()};

            {
                if (Heading())
                {
                    float t1, t2;
                    t1 = rect.width();
                    t2 = rect.height();
                    rect.y2 = rect.y1 + t1;
                    rect.x2 = rect.x1 + t2;
                }

                m_UIStaticItem.SetSize(Fvector2().set(r.width(), r.height()));
            }
        }

        if (Heading())
        {
            m_UIStaticItem.Render(GetHeading());
        }
        else
            m_UIStaticItem.Render();
    }
}

void CUIStatic::Update()
{
    inherited::Update();
    // update light animation if defined
    UpdateColorAnimation();

    if (m_lanim_xform.m_lanim)
    {
        if (m_lanim_xform.m_lanim_start_time < 0.0f)
            ResetXformAnimation();

        float t = Device.dwTimeGlobal / 1000.0f;

        if (m_lanim_xform.m_lanimFlags.test(LA_CYCLIC) ||
            t - m_lanim_xform.m_lanim_start_time < m_lanim_xform.m_lanim->Length_sec())
        {
            int frame;
            u32 clr = m_lanim_xform.m_lanim->CalculateRGB(t - m_lanim_xform.m_lanim_start_time, frame);

            EnableHeading_int(true);
            float heading = (PI_MUL_2 / 255.0f) * color_get_A(clr);
            SetHeading(heading);

            float _value = (float)color_get_R(clr);

            float f_scale = _value / 64.0f;
            Fvector2 _sz;
            _sz.set(m_lanim_xform.m_origSize.x * f_scale, m_lanim_xform.m_origSize.y * f_scale);
            SetWndSize(_sz);
        }
        else
        {
            EnableHeading_int(m_bHeading);
            SetWndSize(m_lanim_xform.m_origSize);
        }
    }

    if (CursorOverWindow() && m_stat_hint_text.size() && !g_statHint->Owner() &&
        Device.dwTimeGlobal > m_dwFocusReceiveTime + 700)
    {
        g_statHint->SetHintText(this, m_stat_hint_text.c_str());

        Fvector2 c_pos = GetUICursor().GetCursorPosition();
        Frect vis_rect;
        vis_rect.set(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

        // select appropriate position
        Frect r;
        r.set(0.0f, 0.0f, g_statHint->GetWidth(), g_statHint->GetHeight());
        r.add(c_pos.x, c_pos.y);

        r.sub(0.0f, r.height());
        if (false == is_in2(vis_rect, r))
            r.sub(r.width(), 0.0f);
        if (false == is_in2(vis_rect, r))
            r.add(0.0f, r.height());

        if (false == is_in2(vis_rect, r))
            r.add(r.width(), 45.0f);

        g_statHint->SetWndPos(r.lt);
    }
}

void CUIStatic::ResetXformAnimation() { m_lanim_xform.m_lanim_start_time = Device.dwTimeGlobal / 1000.0f; }
void CUIStatic::SetShader(const ui_shader& sh) { m_UIStaticItem.SetShader(sh); }
CUILines* CUIStatic::TextItemControl()
{
    if (!m_pTextControl)
    {
        m_pTextControl = new CUILines();
        m_pTextControl->SetTextAlignment(CGameFont::alLeft);
    }
    return m_pTextControl;
}

void CUIStatic::AdjustHeightToText()
{
    if (!fsimilar(TextItemControl()->m_wndSize.x, GetWidth()))
    {
        TextItemControl()->m_wndSize.x = GetWidth();
        TextItemControl()->ParseText(true);
    }
    SetHeight(TextItemControl()->GetVisibleHeight());
}

void CUIStatic::AdjustWidthToText()
{
    if (!m_pTextControl)
        return;
    float _len = m_pTextControl->GetFont()->SizeOf_(m_pTextControl->GetText());
    UI().ClientToScreenScaledWidth(_len);
    SetWidth(_len);
}

void CUIStatic::ColorAnimationSetTextureColor(u32 color, bool only_alpha)
{
    SetTextureColor((only_alpha) ? subst_alpha(GetTextureColor(), color) : color);
}

void CUIStatic::ColorAnimationSetTextColor(u32 color, bool only_alpha)
{
    TextItemControl()->SetTextColor((only_alpha) ? subst_alpha(TextItemControl()->GetTextColor(), color) : color);
}

void CUIStatic::OnFocusLost()
{
    inherited::OnFocusLost();

    if (g_statHint->Owner() == this)
        g_statHint->Discard();
}

//-------------------------------------
CUITextWnd::CUITextWnd() {}
void CUITextWnd::AdjustHeightToText()
{
    if (!fsimilar(TextItemControl().m_wndSize.x, GetWidth()))
    {
        TextItemControl().m_wndSize.x = GetWidth();
        TextItemControl().ParseText(true);
    }
    SetHeight(TextItemControl().GetVisibleHeight());
}

void CUITextWnd::AdjustWidthToText()
{
    float _len = TextItemControl().GetFont()->SizeOf_(TextItemControl().GetText());
    UI().ClientToScreenScaledWidth(_len);
    SetWidth(_len);
}

void CUITextWnd::Draw()
{
    if (!fsimilar(TextItemControl().m_wndSize.x, m_wndSize.x) || !fsimilar(TextItemControl().m_wndSize.y, m_wndSize.y))
    {
        TextItemControl().m_wndSize = m_wndSize;
        TextItemControl().ParseText(true);
    }

    Fvector2 p;
    GetAbsolutePos(p);
    TextItemControl().Draw(p.x, p.y);
}

void CUITextWnd::Update()
{
    R_ASSERT(GetChildWndList().size() == 0);
    UpdateColorAnimation();
    inherited::Update();
}

void CUITextWnd::ColorAnimationSetTextColor(u32 color, bool only_alpha)
{
    SetTextColor((only_alpha) ? subst_alpha(GetTextColor(), color) : color);
}
