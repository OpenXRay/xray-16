#include "pch.hpp"
#include "UI3tButton.h"
#include "Hint/UIHint.h"

CUI3tButton::CUI3tButton()
{
    m_bTextureEnable = false;
    m_bUseTextColor[S_Disabled] = true;
    m_bUseTextColor[S_Highlighted] = false;
    m_bUseTextColor[S_Touched] = false;

    m_dwTextColor[S_Enabled] = 0xFFFFFFFF;
    m_dwTextColor[S_Disabled] = 0xFFAAAAAA;
    m_dwTextColor[S_Highlighted] = 0xFFFFFFFF;
    m_dwTextColor[S_Touched] = 0xFFFFFFFF;

    m_background = NULL;
    m_back_frameline = NULL;
    m_frameline_mode = false;
}

CUI3tButton::~CUI3tButton() {}
void CUI3tButton::OnClick()
{
    CUIButton::OnClick();
    PlaySoundT();
}

bool CUI3tButton::OnMouseDown(int mouse_btn) { return CUIButton::OnMouseDown(mouse_btn); }
void CUI3tButton::OnFocusLost() { inherited::OnFocusLost(); }
void CUI3tButton::OnFocusReceive()
{
    inherited::OnFocusReceive();
    PlaySoundH();
}

void CUI3tButton::InitSoundH(LPCSTR sound_file) { GEnv.Sound->create(m_sound_h, sound_file, st_Effect, sg_SourceType); }
void CUI3tButton::InitSoundT(LPCSTR sound_file) { GEnv.Sound->create(m_sound_t, sound_file, st_Effect, sg_SourceType); }
void CUI3tButton::PlaySoundT()
{
    if (m_sound_t._handle())
        m_sound_t.play(NULL, sm_2D);
}

void CUI3tButton::PlaySoundH()
{
    if (m_sound_h._handle())
        m_sound_h.play(NULL, sm_2D);
}
void CUI3tButton::InitButton(Fvector2 pos, Fvector2 size)
{
    if (m_frameline_mode)
    {
        if (!m_back_frameline)
        {
            m_back_frameline = new CUI_IB_FrameLineWnd();
            m_back_frameline->SetAutoDelete(true);
            AttachChild(m_back_frameline);
        }
        m_back_frameline->SetWndPos(Fvector2().set(0, 0));
        m_back_frameline->SetWndSize(size);
    }
    else
    {
        if (!m_background)
        {
            m_background = new CUI_IB_Static();
            m_background->SetAutoDelete(true);
            AttachChild(m_background);
        }
        m_background->SetWndPos(Fvector2().set(0, 0));
        m_background->SetWndSize(size);
    }
    CUIButton::SetWndPos(pos);
    CUIButton::SetWndSize(size);
}

void CUI3tButton::SetWidth(float width)
{
    CUIButton::SetWidth(width);
    if (m_background)
    {
        m_background->SetWidth(width);
    }
    else if (m_back_frameline)
    {
        m_back_frameline->SetWidth(width);
    }
}

void CUI3tButton::SetHeight(float height)
{
    CUIButton::SetHeight(height);
    if (m_background)
    {
        m_background->SetHeight(height);
    }
    else if (m_back_frameline)
    {
        m_back_frameline->SetHeight(height);
    }
}

void CUI3tButton::InitTexture(LPCSTR tex_name)
{
    string_path tex_enabled;
    string_path tex_disabled;
    string_path tex_touched;
    string_path tex_highlighted;

    // enabled state texture
    xr_strcpy(tex_enabled, tex_name);
    xr_strcat(tex_enabled, "_e");

    // pressed state texture
    xr_strcpy(tex_disabled, tex_name);
    xr_strcat(tex_disabled, "_d");

    // touched state texture
    xr_strcpy(tex_touched, tex_name);
    xr_strcat(tex_touched, "_t");

    // touched state texture
    xr_strcpy(tex_highlighted, tex_name);
    xr_strcat(tex_highlighted, "_h");

    this->InitTexture(tex_enabled, tex_disabled, tex_touched, tex_highlighted);
}

void CUI3tButton::InitTexture(LPCSTR tex_enabled, LPCSTR tex_disabled, LPCSTR tex_touched, LPCSTR tex_highlighted)
{
    if (m_background)
    {
        m_background->InitState(S_Enabled, tex_enabled);
        m_background->InitState(S_Disabled, tex_disabled);
        m_background->InitState(S_Touched, tex_touched);
        m_background->InitState(S_Highlighted, tex_highlighted);
    }
    else if (m_back_frameline)
    {
        m_back_frameline->InitState(S_Enabled, tex_enabled);
        m_back_frameline->InitState(S_Disabled, tex_disabled);
        m_back_frameline->InitState(S_Touched, tex_touched);
        m_back_frameline->InitState(S_Highlighted, tex_highlighted);
    }

    this->m_bTextureEnable = true;
}

void CUI3tButton::SetTextureOffset(float x, float y)
{
    if (m_background)
    {
        this->m_background->SetTextureOffset(x, y);
    }
}

void CUI3tButton::Draw() { inherited::Draw(); }
void CUI3tButton::DrawTexture()
{
    if (m_bTextureEnable)
    {
        if (m_background)
        {
            m_background->SetStretchTexture(true);
            m_background->Draw();
        }
        else if (m_back_frameline)
        {
            m_back_frameline->Draw();
        }
    }
}

void CUI3tButton::Update()
{
    inherited::Update();

    if (m_bTextureEnable)
    {
        if (!m_bIsEnabled)
        {
            if (m_background)
            {
                m_background->SetCurrentState(S_Disabled);
            }
            else if (m_back_frameline)
            {
                m_back_frameline->SetCurrentState(S_Disabled);
            }
        }
        else if (CUIButton::BUTTON_PUSHED == GetButtonState())
        {
            if (m_background)
            {
                m_background->SetCurrentState(S_Touched);
            }
            else if (m_back_frameline)
            {
                m_back_frameline->SetCurrentState(S_Touched);
            }
        }
        else if (m_bCursorOverWindow)
        {
            if (m_background)
            {
                m_background->SetCurrentState(S_Highlighted);
            }
            else if (m_back_frameline)
            {
                m_back_frameline->SetCurrentState(S_Highlighted);
            }
        }
        else
        {
            if (m_background)
            {
                m_background->SetCurrentState(S_Enabled);
            }
            else if (m_back_frameline)
            {
                m_back_frameline->SetCurrentState(S_Enabled);
            }
        }
    }

    u32 textColor;

    if (!m_bIsEnabled)
    {
        textColor = m_bUseTextColor[S_Disabled] ? m_dwTextColor[S_Disabled] : m_dwTextColor[S_Enabled];
    }
    else if (CUIButton::BUTTON_PUSHED == GetButtonState())
    {
        textColor = m_bUseTextColor[S_Touched] ? m_dwTextColor[S_Touched] : m_dwTextColor[S_Enabled];
    }
    else if (m_bCursorOverWindow)
    {
        textColor = m_bUseTextColor[S_Highlighted] ? m_dwTextColor[S_Highlighted] : m_dwTextColor[S_Enabled];
    }
    else
    {
        textColor = m_dwTextColor[S_Enabled];
    }

    TextItemControl()->SetTextColor(textColor);
}
