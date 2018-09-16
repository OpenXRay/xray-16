#include "stdafx.h"
#include "UICDkey.h"
#include "UILines.h"
#include "xrEngine/line_edit_control.h"
#include "MainMenu.h"
#include "xrEngine/xr_IOConsole.h"
#include "RegistryFuncs.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "player_name_modifyer.h"
#include "xrGameSpy/GameSpy_GP.h"
#include "xrCore/os_clipboard.h"

string64 gsCDKey = "";
LPCSTR AddHyphens(LPCSTR c);
LPCSTR DelHyphens(LPCSTR c);

CUICDkey::CUICDkey()
{
    m_view_access = false;
    CreateCDKeyEntry();
    assign_callbacks();
}

void CUICDkey::assign_callbacks()
{
    m_editor_control->assign_callback(
        SDL_SCANCODE_V, text_editor::ks_Ctrl, Callback(this, &CUICDkey::paste_from_clipboard));
    m_editor_control->assign_callback(
        SDL_SCANCODE_INSERT, text_editor::ks_Shift, Callback(this, &CUICDkey::paste_from_clipboard));
}

struct inappropriate_characters
{
    inline bool operator()(char const character) const
    {
        if ((character >= 'a') && (character <= 'z'))
            return false;

        if ((character >= 'A') && (character <= 'Z'))
            return false;

        if ((character >= '0') && (character <= '9'))
            return false;

        return true;
    }
}; // struct inappropriate_characters

void CUICDkey::paste_from_clipboard()
{
    string32 temp;
    os_clipboard::paste_from_clipboard(&temp[0], sizeof(temp));
    LPSTR const new_end = std::remove_if(&temp[0], &temp[0] + xr_strlen(temp), inappropriate_characters());
    *new_end = 0;
    temp[16] = 0;

    m_editor_control->set_edit(temp);
}

void CUICDkey::Show(bool status)
{
    inherited::Show(status);
    SetCurrentOptValue();
}

void CUICDkey::OnFocusLost()
{
    inherited::OnFocusLost();
    if (m_bInputFocus)
    {
        m_bInputFocus = false;
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    }
    SaveOptValue();
}

void CUICDkey::Draw()
{
    LPCSTR edt_str = ec().str_edit();
    u32 edt_size = xr_strlen(edt_str);

    if (edt_size == 0)
    {
        m_view_access = true;
    }

    // inherited::Draw();
    CUIStatic::Draw();
    Frect rect;
    GetAbsoluteRect(rect);
    Fvector2 out;

    out.y = (m_wndSize.y - TextItemControl()->m_pFont->CurrentHeight_()) / 2.0f;
    out.x = TextItemControl()->m_TextOffset.x + TextItemControl()->GetIndentByAlign();
    TextItemControl()->m_pFont->SetColor(TextItemControl()->GetTextColor());
    TextItemControl()->m_pFont->SetAligment(TextItemControl()->GetTextAlignment());

    Fvector2 pos;
    pos.set(rect.left + out.x, rect.top + out.y);
    UI().ClientToScreenScaled(pos);

    string64 xx_str = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    edt_size = xr_strlen(edt_str);
    if (edt_size > 63)
    {
        edt_size = 63;
    }
    xx_str[edt_size] = 0;

    string64 xx_str1 = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    LPCSTR edt_str1 = ec().str_before_cursor();
    u32 edt_size1 = xr_strlen(edt_str1);
    if (edt_size1 > 63)
    {
        edt_size1 = 63;
    }
    xx_str1[edt_size1] = 0;

    if (m_bInputFocus)
    {
        LPCSTR res = (m_view_access) ? edt_str : xx_str;
        LPCSTR res1 = (m_view_access) ? edt_str1 : xx_str1;

        TextItemControl()->m_pFont->Out(pos.x, pos.y, "%s", AddHyphens(res));

        float _h = TextItemControl()->m_pFont->CurrentHeight_();
        UI().ClientToScreenScaledHeight(_h);

        out.y = rect.top + (m_wndSize.y - _h) / 2.0f;

        float w_tmp = 0.0f;
        int i = (int)xr_strlen(res1);
        w_tmp = TextItemControl()->m_pFont->SizeOf_(res1);
        UI().ClientToScreenScaledWidth(w_tmp);
        out.x = rect.left + w_tmp;

        w_tmp = TextItemControl()->m_pFont->SizeOf_("-");
        UI().ClientToScreenScaledWidth(w_tmp);

        if (i > 3)
            out.x += w_tmp;
        if (i > 7)
            out.x += w_tmp;
        if (i > 11)
            out.x += w_tmp;

        UI().ClientToScreenScaled(out);
        TextItemControl()->m_pFont->Out(out.x, out.y, "_");
    }
    else
    {
        TextItemControl()->m_pFont->Out(pos.x, pos.y, "%s", AddHyphens(xx_str));
    }
    TextItemControl()->m_pFont->OnRender();
}

LPCSTR CUICDkey::GetText() { return AddHyphens(inherited::GetText()); }
void CUICDkey::SetCurrentOptValue()
{
    CUIOptionsItem::SetCurrentOptValue();
    string512 CDKeyStr;
    CDKeyStr[0] = 0;
    GetCDKey_FromRegistry(CDKeyStr);
    inherited::SetText(DelHyphens(CDKeyStr));
}

void CUICDkey::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();

    xr_strcpy(gsCDKey, sizeof(gsCDKey), AddHyphens(inherited::GetText()));
    WriteCDKey_ToRegistry(gsCDKey);

    if (MainMenu()->IsCDKeyIsValid())
        m_view_access = false;
}

void CUICDkey::SaveBackUpOptValue() // current->backup
{
    xr_strcpy(m_opt_backup_value, inherited::GetText());
}

void CUICDkey::UndoOptValue() // backup->current
{
    inherited::SetText(m_opt_backup_value);
}

bool CUICDkey::IsChangedOptValue() const { return 0 != xr_strcmp(m_opt_backup_value, inherited::GetText()); }
void CUICDkey::CreateCDKeyEntry() {}
//=================================================================

void CUIMPPlayerName::OnFocusLost()
{
    inherited::OnFocusLost();
    if (m_bInputFocus)
    {
        m_bInputFocus = false;
        GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, NULL);
    }
    string64 name;
    xr_strcpy(name, GetText());
    WritePlayerName_ToRegistry(name);
}

// -------------------------------------------------------------------------------------------------

void GetCDKey_FromRegistry(char* cdkey)
{
    ReadRegistry_StrValue(REGISTRY_VALUE_GSCDKEY, cdkey);
    if (xr_strlen(cdkey) > 64)
    {
        cdkey[64] = 0;
    }
}

void WriteCDKey_ToRegistry(LPSTR cdkey)
{
    if (xr_strlen(cdkey) > 64)
    {
        cdkey[64] = 0;
    }
    WriteRegistry_StrValue(REGISTRY_VALUE_GSCDKEY, cdkey);
}

void GetPlayerName_FromRegistry(char* name, u32 const name_size)
{
    string256 new_name;
    if (!ReadRegistry_StrValue(REGISTRY_VALUE_USERNAME, name))
    {
        name[0] = 0;
        Msg("! Player name registry key (%s) not found !", REGISTRY_VALUE_USERNAME);
        return;
    }
    u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
    if (xr_strlen(name) > max_name_length)
    {
        name[max_name_length] = 0;
    }
    if (xr_strlen(name) == 0)
    {
        Msg("! Player name in registry is empty! (%s)", REGISTRY_VALUE_USERNAME);
    }
    modify_player_name(name, new_name);
    strncpy_s(name, name_size, new_name, max_name_length);
}

void WritePlayerName_ToRegistry(LPSTR name)
{
    u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
    if (xr_strlen(name) > max_name_length)
    {
        name[max_name_length] = 0;
    }
    WriteRegistry_StrValue(REGISTRY_VALUE_USERNAME, name);
}
