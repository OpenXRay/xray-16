#include "StdAfx.h"

#include "UIEditKeyBind.h"
#include "xr_level_controller.h"
#include "Common/object_broker.h"
#include "xrEngine/XR_IOConsole.h"

CUIEditKeyBind::CUIEditKeyBind(bool bPrim)
{
    m_bPrimary = bPrim;
    m_bIsEditMode = false;
    TextItemControl()->SetTextComplexMode(false);
    m_keyboard = NULL;
    m_opt_backup_value = NULL;
    m_action = NULL;
}
CUIEditKeyBind::~CUIEditKeyBind() {}
u32 cut_string_by_length(CGameFont* pFont, LPCSTR src, LPSTR dst, u32 dst_size, float length)
{
    if (pFont->IsMultibyte())
    {
        u16 nPos = pFont->GetCutLengthPos(length, src);
        VERIFY(nPos < dst_size);
        strncpy_s(dst, dst_size, src, nPos);
        dst[nPos] = '\0';
        return nPos;
    }
    else
    {
        float text_len = pFont->SizeOf_(src);
        UI().ClientToScreenScaledWidth(text_len);
        VERIFY(xr_strlen(src) <= dst_size);
        xr_strcpy(dst, dst_size, src);

        while (text_len > length)
        {
            dst[xr_strlen(dst) - 1] = 0;
            VERIFY(xr_strlen(dst));
            text_len = pFont->SizeOf_(dst);
            UI().ClientToScreenScaledWidth(text_len);
        }

        return xr_strlen(dst);
    }
}

void CUIEditKeyBind::SetText(const char* text)
{
    if (!text || 0 == xr_strlen(text))
        TextItemControl()->SetText("---");
    else
    {
        string256 buff;

        cut_string_by_length(TextItemControl()->GetFont(), text, buff, sizeof(buff), GetWidth());

        TextItemControl()->SetText(buff);
    }
}

void CUIEditKeyBind::InitKeyBind(Fvector2 pos, Fvector2 size)
{
    CUIStatic::SetWndPos(pos);
    CUIStatic::SetWndSize(size);
    InitTexture("ui_listline2");
    TextItemControl()->SetFont(UI().Font().pFontLetterica16Russian);
    SetStretchTexture(true);
    SetEditMode(false);
}

void CUIEditKeyBind::OnFocusLost()
{
    CUIStatic::OnFocusLost();
    SetEditMode(false);
    TextItemControl()->SetTextColor((subst_alpha(TextItemControl()->GetTextColor(), color_get_A(0xffffffff))));
}

bool CUIEditKeyBind::OnMouseDown(int mouse_btn)
{
    if (m_bIsEditMode)
    {
        string64 message;

        m_keyboard = dik_to_ptr(mouse_btn, true);
        if (!m_keyboard)
            return true;
        SetValue();
        OnFocusLost();

        xr_strcpy(message, m_action->action_name);
        xr_strcat(message, "=");
        xr_strcat(message, m_keyboard->key_name);
        SendMessage2Group("key_binding", message);

        return true;
    }

    if (mouse_btn == MOUSE_1)
        SetEditMode(m_bCursorOverWindow);

    return CUIStatic::OnMouseDown(mouse_btn);
}

bool CUIEditKeyBind::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (dik == MOUSE_1 || dik == MOUSE_2 || dik == MOUSE_3)
        return false;

    if (CUIStatic::OnKeyboardAction(dik, keyboard_action))
        return true;

    string64 message;
    if (m_bIsEditMode)
    {
        m_keyboard = dik_to_ptr(dik, true);
        if (!m_keyboard)
            return true;

        SetValue();

        xr_strcpy(message, m_action->action_name);
        xr_strcat(message, "=");
        xr_strcat(message, m_keyboard->key_name);
        OnFocusLost();
        SendMessage2Group("key_binding", message);
        return true;
    }
    return false;
}

void CUIEditKeyBind::Update() { CUIStatic::Update(); }
void CUIEditKeyBind::SetEditMode(bool b)
{
    m_bIsEditMode = b;

    if (b)
    {
        SetColorAnimation("ui_map_area_anim", LA_CYCLIC | LA_ONLYALPHA | LA_TEXTCOLOR);
        TextureOn();
    }
    else
    {
        SetColorAnimation(NULL, 0);
        TextureOff();
    }
}

void CUIEditKeyBind::AssignProps(const shared_str& entry, const shared_str& group)
{
    CUIOptionsItem::AssignProps(entry, group);
    m_action = action_name_to_ptr(entry.c_str());
}

void CUIEditKeyBind::SetValue()
{
    if (m_keyboard)
        SetText(m_keyboard->key_local_name.c_str());
    else
        SetText(NULL);
}

void CUIEditKeyBind::SetCurrentOptValue()
{
    string64 buff;
    ZeroMemory(buff, sizeof(buff));

    _binding* pbinding = &g_key_bindings[m_action->id];

    int idx = (m_bPrimary) ? 0 : 1;
    m_keyboard = pbinding->m_keyboard[idx];

    SetValue();
}

void CUIEditKeyBind::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();
    BindAction2Key();
}

void CUIEditKeyBind::SaveBackUpOptValue()
{
    CUIOptionsItem::SaveBackUpOptValue();
    m_opt_backup_value = m_keyboard;
}

void CUIEditKeyBind::UndoOptValue()
{
    m_keyboard = m_opt_backup_value;
    CUIOptionsItem::UndoOptValue();
}

bool CUIEditKeyBind::IsChangedOptValue() const { return m_keyboard != m_opt_backup_value; }
void CUIEditKeyBind::BindAction2Key()
{
    xr_string comm_unbind = (m_bPrimary) ? "unbind " : "unbind_sec ";
    comm_unbind += m_action->action_name;
    Console->Execute(comm_unbind.c_str());

    if (m_keyboard)
    {
        xr_string comm_bind = (m_bPrimary) ? "bind " : "bind_sec ";
        comm_bind += m_action->action_name;
        comm_bind += " ";
        comm_bind += m_keyboard->key_name;
        Console->Execute(comm_bind.c_str());
    }
}

void CUIEditKeyBind::OnMessage(LPCSTR message)
{
    // message = "command=key"
    int eq = (int)strcspn(message, "=");

    if (!m_keyboard)
        return;

    if (0 != xr_strcmp(m_keyboard->key_name, message + eq + 1))
        return;

    string64 command;
    xr_strcpy(command, message);
    command[eq] = 0;

    if (0 == xr_strcmp(m_action->action_name, command))
        return; // fuck

    _action* other_action = action_name_to_ptr(command);
    if (is_group_not_conflicted(m_action->key_group, other_action->key_group))
        return;

    SetText("---");
    m_keyboard = NULL;
}
