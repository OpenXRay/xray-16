// XR_IOConsole.cpp: implementation of the CConsole class.
// modify 15.05.2008 sea

#include "stdafx.h"
#include "XR_IOConsole.h"
#include "line_editor.h"

#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "x_ray.h"
#include "xr_input.h"
#include "xr_ioc_cmd.h"
#include "GameFont.h"

#include "Include/xrRender/UIRender.h"

static float const UI_BASE_WIDTH = 1024.0f;
static float const UI_BASE_HEIGHT = 768.0f;
static float const LDIST = 0.05f;
static u32 const cmd_history_max = 64;

static u32 const prompt_font_color = color_rgba(228, 228, 255, 255);
static u32 const tips_font_color = color_rgba(230, 250, 230, 255);
static u32 const cmd_font_color = color_rgba(138, 138, 245, 255);
static u32 const cursor_font_color = color_rgba(255, 255, 255, 255);
static u32 const total_font_color = color_rgba(250, 250, 15, 180);
static u32 const default_font_color = color_rgba(250, 250, 250, 250);

static u32 const back_color = color_rgba(20, 20, 20, 200);
static u32 const tips_back_color = color_rgba(20, 20, 20, 200);
static u32 const tips_select_color = color_rgba(90, 90, 140, 230);
static u32 const tips_word_color = color_rgba(5, 100, 56, 200);
static u32 const tips_scroll_back_color = color_rgba(15, 15, 15, 230);
static u32 const tips_scroll_pos_color = color_rgba(70, 70, 70, 240);

ENGINE_API CConsole* Console = NULL;

extern char const* const ioc_prompt;
char const* const ioc_prompt = ">>> ";

extern char const* const ch_cursor;
char const* const ch_cursor = "_";

text_editor::line_edit_control& CConsole::ec() { return m_editor->control(); }
u32 CConsole::get_mark_color(Console_mark type)
{
    u32 color = default_font_color;
    switch (type)
    {
    case mark0: color = color_rgba(255, 255, 0, 255); break;
    case mark1: color = color_rgba(255, 0, 0, 255); break;
    case mark2: color = color_rgba(100, 100, 255, 255); break;
    case mark3: color = color_rgba(0, 222, 205, 155); break;
    case mark4: color = color_rgba(255, 0, 255, 255); break;
    case mark5: color = color_rgba(155, 55, 170, 155); break;
    case mark6: color = color_rgba(25, 200, 50, 255); break;
    case mark7: color = color_rgba(255, 255, 0, 255); break;
    case mark8: color = color_rgba(128, 128, 128, 255); break;
    case mark9: color = color_rgba(0, 255, 0, 255); break;
    case mark10: color = color_rgba(55, 155, 140, 255); break;
    case mark11: color = color_rgba(205, 205, 105, 255); break;
    case mark12: color = color_rgba(128, 128, 250, 255); break;
    case no_mark:
    default: break;
    }
    return color;
}

bool CConsole::is_mark(Console_mark type)
{
    switch (type)
    {
    case mark0:
    case mark1:
    case mark2:
    case mark3:
    case mark4:
    case mark5:
    case mark6:
    case mark7:
    case mark8:
    case mark9:
    case mark10:
    case mark11:
    case mark12: return true; break;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CConsole::CConsole() : m_hShader_back(NULL)
{
    m_editor = new text_editor::line_editor((u32)CONSOLE_BUF_SIZE);
    m_cmd_history_max = cmd_history_max;
    m_disable_tips = false;
    Register_callbacks();
}

void CConsole::Initialize()
{
    scroll_delta = 0;
    bVisible = false;
    pFont = NULL;
    pFont2 = NULL;

    m_mouse_pos.x = 0;
    m_mouse_pos.y = 0;
    m_last_cmd = NULL;

    m_cmd_history.reserve(m_cmd_history_max + 2);
    m_cmd_history.clear();
    reset_cmd_history_idx();

    m_tips.reserve(MAX_TIPS_COUNT + 1);
    m_tips.clear();
    m_temp_tips.reserve(MAX_TIPS_COUNT + 1);
    m_temp_tips.clear();

    m_tips_mode = 0;
    m_prev_length_str = 0;
    m_cur_cmd = NULL;
    reset_selected_tip();

    // Commands
    extern void CCC_Register();
    CCC_Register();
}

CConsole::~CConsole()
{
    xr_delete(m_hShader_back);
    xr_delete(m_editor);
    Destroy();
}

void CConsole::Destroy()
{
    xr_delete(pFont);
    xr_delete(pFont2);
    Commands.clear();
}

void CConsole::AddCommand(IConsole_Command* cc) { Commands[cc->Name()] = cc; }
void CConsole::RemoveCommand(IConsole_Command* cc)
{
    vecCMD_IT it = Commands.find(cc->Name());
    if (Commands.end() != it)
    {
        Commands.erase(it);
    }
}

void CConsole::OnFrame()
{
    m_editor->on_frame();

    if (Device.dwFrame % 10 == 0)
    {
        update_tips();
    }
}

void CConsole::OutFont(LPCSTR text, float& pos_y)
{
    float str_length = pFont->SizeOf_(text);
    float scr_width = 1.98f * Device.fWidth_2;
    if (str_length > scr_width) // 1024.0f
    {
        float f = 0.0f;
        int sz = 0;
        int ln = 0;
        PSTR one_line = (PSTR)_alloca((CONSOLE_BUF_SIZE + 1) * sizeof(char));

        while (text[sz] && (ln + sz < CONSOLE_BUF_SIZE - 5)) // перенос строк
        {
            one_line[ln + sz] = text[sz];
            one_line[ln + sz + 1] = 0;

            float t = pFont->SizeOf_(one_line + ln);
            if (t > scr_width)
            {
                OutFont(text + sz + 1, pos_y);
                pos_y -= LDIST;
                pFont->OutI(-1.0f, pos_y, "%s", one_line + ln);
                ln = sz + 1;
                f = 0.0f;
            }
            else
            {
                f = t;
            }

            ++sz;
        }
    }
    else
    {
        pFont->OutI(-1.0f, pos_y, "%s", text);
    }
}

void CConsole::OnUIReset()
{
    xr_delete(pFont);
    xr_delete(pFont2);
}

void CConsole::OnRender()
{
    if (!bVisible)
    {
        return;
    }

    if (!m_hShader_back)
    {
        m_hShader_back = new FactoryPtr<IUIShader>();
        (*m_hShader_back)->create("hud" DELIMITER "default", "ui" DELIMITER "ui_console"); // "ui/ui_empty"
    }

    if (!pFont)
    {
        pFont = new CGameFont("hud_font_di", CGameFont::fsDeviceIndependent);
        pFont->SetHeightI(0.025f);
    }
    if (!pFont2)
    {
        pFont2 = new CGameFont("hud_font_di2", CGameFont::fsDeviceIndependent);
        pFont2->SetHeightI(0.025f);
    }

    bool bGame = false;
    if ((g_pGameLevel && g_pGameLevel->bReady) ||
        (g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive()))
    {
        bGame = true;
    }
    if (GEnv.isDedicatedServer)
    {
        bGame = false;
    }

    DrawBackgrounds(bGame);

    float fMaxY;
    float dwMaxY = (float)Device.dwHeight;
    // float dwMaxX=float(Device.dwWidth/2);
    if (bGame)
    {
        fMaxY = 0.0f;
        dwMaxY /= 2;
    }
    else
    {
        fMaxY = 1.0f;
    }

    float ypos = fMaxY - LDIST * 1.1f;
    float scr_x = 1.0f / Device.fWidth_2;

    //---------------------------------------------------------------------------------
    float scr_width = 1.9f * Device.fWidth_2;
    float ioc_d = pFont->SizeOf_(ioc_prompt);
    float d1 = pFont->SizeOf_("_");

    LPCSTR s_cursor = ec().str_before_cursor();
    LPCSTR s_b_mark = ec().str_before_mark();
    LPCSTR s_mark = ec().str_mark();
    LPCSTR s_mark_a = ec().str_after_mark();

    // strncpy_s( buf1, cur_pos, editor, MAX_LEN );
    float str_length = ioc_d + pFont->SizeOf_(s_cursor);
    float out_pos = 0.0f;
    if (str_length > scr_width)
    {
        out_pos -= (str_length - scr_width);
        str_length = scr_width;
    }

    pFont->SetColor(prompt_font_color);
    pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", ioc_prompt);
    out_pos += ioc_d;

    if (!m_disable_tips && m_tips.size())
    {
        pFont->SetColor(tips_font_color);

        float shift_x = 0.0f;
        switch (m_tips_mode)
        {
        case 0: shift_x = scr_x * 1.0f; break;
        case 1: shift_x = scr_x * out_pos; break;
        case 2: shift_x = scr_x * (ioc_d + pFont->SizeOf_(m_cur_cmd.c_str()) + d1); break;
        case 3: shift_x = scr_x * str_length; break;
        }

        vecTipsEx::iterator itb = m_tips.begin() + m_start_tip;
        vecTipsEx::iterator ite = m_tips.end();
        for (u32 i = 0; itb != ite; ++itb, ++i) // tips
        {
            pFont->OutI(-1.0f + shift_x, fMaxY + i * LDIST, "%s", (*itb).text.c_str());
            if (i >= VIEW_TIPS_COUNT - 1)
            {
                break; // for
            }
        }
    }

    // ===== ==============================================
    pFont->SetColor(cmd_font_color);
    pFont2->SetColor(cmd_font_color);

    pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_b_mark);
    out_pos += pFont->SizeOf_(s_b_mark);
    pFont2->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_mark);
    out_pos += pFont2->SizeOf_(s_mark);
    pFont->OutI(-1.0f + out_pos * scr_x, ypos, "%s", s_mark_a);

    // pFont2->OutI( -1.0f + ioc_d * scr_x, ypos, "%s", editor=all );

    if (ec().cursor_view())
    {
        pFont->SetColor(cursor_font_color);
        pFont->OutI(-1.0f + str_length * scr_x, ypos, "%s", ch_cursor);
    }

    // ---------------------
    u32 log_line = LogFile.size() - 1;
    ypos -= LDIST;
    for (int i = log_line - scroll_delta; i >= 0; --i)
    {
        ypos -= LDIST;
        if (ypos < -1.0f)
        {
            break;
        }
        LPCSTR ls = LogFile[i].c_str();

        if (!ls)
        {
            continue;
        }
        Console_mark cm = (Console_mark)ls[0];
        pFont->SetColor(get_mark_color(cm));
        // u8 b = (is_mark( cm ))? 2 : 0;
        // OutFont( ls + b, ypos );
        OutFont(ls, ypos);
    }

    string16 q;
    xr_itoa(log_line, q, 10);
    u32 qn = xr_strlen(q);
    pFont->SetColor(total_font_color);
    pFont->OutI(0.95f - 0.03f * qn, fMaxY - 2.0f * LDIST, "[%d]", log_line);

    pFont->OnRender();
    pFont2->OnRender();
}

void CConsole::DrawBackgrounds(bool bGame)
{
    float ky = (bGame) ? 0.5f : 1.0f;

    Frect r;
    r.set(0.0f, 0.0f, float(Device.dwWidth), ky* float(Device.dwHeight));

    GEnv.UIRender->SetShader(**m_hShader_back);
    // 6 = back, 12 = tips, (VIEW_TIPS_COUNT+1)*6 = highlight_words, 12 = scroll
    GEnv.UIRender->StartPrimitive(6 + 12 + (VIEW_TIPS_COUNT + 1) * 6 + 12, IUIRender::ptTriList, IUIRender::pttTL);

    DrawRect(r, back_color);

    if (m_tips.size() == 0 || m_disable_tips)
    {
        GEnv.UIRender->FlushPrimitive();
        return;
    }

    pcstr max_str = "xxxxx";
    for (auto& it : m_tips)
        if (pFont->SizeOf_(it.text.c_str()) > pFont->SizeOf_(max_str))
            max_str = it.text.c_str();

    float w1 = pFont->SizeOf_("_");
    float ioc_w = pFont->SizeOf_(ioc_prompt) - w1;
    float cur_cmd_w = pFont->SizeOf_(m_cur_cmd.c_str());
    cur_cmd_w += (cur_cmd_w > 0.01f) ? w1 : 0.0f;

    float list_w = pFont->SizeOf_(max_str) + 2.0f * w1;

    float font_h = pFont->CurrentHeight_();
    float tips_h = std::min(m_tips.size(), (size_t)VIEW_TIPS_COUNT) * font_h;
    tips_h += (m_tips.size() > 0) ? 5.0f : 0.0f;

    Frect pr, sr;
    pr.x1 = ioc_w + cur_cmd_w;
    pr.x2 = pr.x1 + list_w;

    pr.y1 = UI_BASE_HEIGHT * 0.5f;
    pr.y1 *= float(Device.dwHeight) / UI_BASE_HEIGHT;

    pr.y2 = pr.y1 + tips_h;

    float select_y = 0.0f;
    float select_h = 0.0f;

    if (m_select_tip >= 0 && m_select_tip < (int)m_tips.size())
    {
        int sel_pos = m_select_tip - m_start_tip;

        select_y = sel_pos * font_h;
        select_h = font_h; // 1 string
    }

    sr.x1 = pr.x1;
    sr.y1 = pr.y1 + select_y;

    sr.x2 = pr.x2;
    sr.y2 = sr.y1 + select_h;

    DrawRect(pr, tips_back_color);
    DrawRect(sr, tips_select_color);

    // --------------------------- highlight words --------------------

    if (m_select_tip < (int)m_tips.size())
    {
        Frect r2;
        xr_string tmp;
        vecTipsEx::iterator itb2 = m_tips.begin() + m_start_tip;
        vecTipsEx::iterator ite2 = m_tips.end();
        for (u32 i = 0; itb2 != ite2; ++itb2, ++i) // tips
        {
            TipString const& ts = (*itb2);
            if ((ts.HL_start < 0) || (ts.HL_finish < 0) || (ts.HL_start > ts.HL_finish))
            {
                continue;
            }
            int str_size = (int)ts.text.size();
            if ((ts.HL_start >= str_size) || (ts.HL_finish > str_size))
            {
                continue;
            }

            r2.set_zero();
            tmp.assign(ts.text.c_str(), ts.HL_start);
            r2.x1 = pr.x1 + w1 + pFont->SizeOf_(tmp.c_str());
            r2.y1 = pr.y1 + i * font_h;

            tmp.assign(ts.text.c_str(), ts.HL_finish);
            r2.x2 = pr.x1 + w1 + pFont->SizeOf_(tmp.c_str());
            r2.y2 = r2.y1 + font_h;

            DrawRect(r2, tips_word_color);

            if (i >= VIEW_TIPS_COUNT - 1)
            {
                break; // for itb2
            }
        } // for itb
    } // if

    // --------------------------- scroll bar --------------------

    u32 tips_sz = m_tips.size();
    if (tips_sz > VIEW_TIPS_COUNT)
    {
        Frect rb, rs;

        rb.x1 = pr.x2;
        rb.y1 = pr.y1;
        rb.x2 = rb.x1 + 2 * w1;
        rb.y2 = pr.y2;
        DrawRect(rb, tips_scroll_back_color);

        VERIFY(rb.y2 - rb.y1 >= 1.0f);
        float back_height = rb.y2 - rb.y1;
        float u_height = (back_height * VIEW_TIPS_COUNT) / float(tips_sz);
        if (u_height < 0.5f * font_h)
        {
            u_height = 0.5f * font_h;
        }

        // float u_pos = (back_height - u_height) * float(m_start_tip) / float(tips_sz);
        float u_pos = back_height * float(m_start_tip) / float(tips_sz);

        // clamp( u_pos, 0.0f, back_height - u_height );

        rs = rb;
        rs.y1 = pr.y1 + u_pos;
        rs.y2 = rs.y1 + u_height;
        DrawRect(rs, tips_scroll_pos_color);
    }

    GEnv.UIRender->FlushPrimitive();
}

void CConsole::DrawRect(Frect const& r, u32 color)
{
    GEnv.UIRender->PushPoint(r.x1, r.y1, 0.0f, color, 0.0f, 0.0f);
    GEnv.UIRender->PushPoint(r.x2, r.y1, 0.0f, color, 1.0f, 0.0f);
    GEnv.UIRender->PushPoint(r.x2, r.y2, 0.0f, color, 1.0f, 1.0f);

    GEnv.UIRender->PushPoint(r.x1, r.y1, 0.0f, color, 0.0f, 0.0f);
    GEnv.UIRender->PushPoint(r.x2, r.y2, 0.0f, color, 1.0f, 1.0f);
    GEnv.UIRender->PushPoint(r.x1, r.y2, 0.0f, color, 0.0f, 1.0f);
}

void CConsole::ExecuteCommand(LPCSTR cmd_str, bool record_cmd)
{
    u32 str_size = xr_strlen(cmd_str);
    PSTR edt = (PSTR)_alloca((str_size + 1) * sizeof(char));
    PSTR first = (PSTR)_alloca((str_size + 1) * sizeof(char));
    PSTR last = (PSTR)_alloca((str_size + 1) * sizeof(char));

    xr_strcpy(edt, str_size + 1, cmd_str);
    edt[str_size] = 0;

    scroll_delta = 0;
    reset_cmd_history_idx();
    reset_selected_tip();

    text_editor::remove_spaces(edt);
    if (edt[0] == 0)
    {
        return;
    }
    if (record_cmd)
    {
        char c[2];
        c[0] = mark2;
        c[1] = 0;

        if (m_last_cmd.c_str() == 0 || xr_strcmp(m_last_cmd, edt) != 0)
        {
            Log(c, edt);
            add_cmd_history(edt);
            m_last_cmd = edt;
        }
    }
    text_editor::split_cmd(first, last, edt);

    // search
    vecCMD_IT it = Commands.find(first);
    if (it != Commands.end())
    {
        IConsole_Command* cc = it->second;
        if (cc && cc->bEnabled)
        {
            if (cc->bLowerCaseArgs)
            {
                xr_strlwr(last);
            }
            if (last[0] == 0)
            {
                if (cc->bEmptyArgsHandled)
                {
                    cc->Execute(last);
                }
                else
                {
                    IConsole_Command::TStatus stat;
                    cc->GetStatus(stat);
                    Msg("- %s %s", cc->Name(), stat);
                }
            }
            else
            {
                cc->Execute(last);
                if (record_cmd)
                {
                    cc->add_to_LRU((LPCSTR)last);
                }
            }
        }
        else
        {
            Log("! Command disabled.");
        }
    }
    else
    {
        Log("! Unknown command: ", first);
    }

    if (record_cmd)
    {
        ec().clear_states();
    }
}

void CConsole::Show()
{
    if (bVisible)
    {
        return;
    }
    bVisible = true;

    SDL_GetGlobalMouseState((int *) &m_mouse_pos.x, (int *) &m_mouse_pos.y); // Replace with SDL_GetMouseState in case retrieve window-relative coordinates

    ec().clear_states();
    scroll_delta = 0;
    reset_cmd_history_idx();
    reset_selected_tip();
    update_tips();

    m_editor->IR_Capture();
    Device.seqRender.Add(this, 1);
    Device.seqFrame.Add(this);
}

extern CInput* pInput;

void CConsole::Hide()
{
    if (!bVisible || g_pGamePersistent && GEnv.isDedicatedServer)
        return;

    // if ( g_pGameLevel ||
    // ( g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive() ))

    if (pInput->IsExclusiveMode())
    {
        SDL_WarpMouseGlobal(m_mouse_pos.x, m_mouse_pos.y); // Replace with SDL_WarpMouseInWindow in case set window-relative coordinates
    }

    bVisible = false;
    reset_selected_tip();
    update_tips();

    Device.seqFrame.Remove(this);
    Device.seqRender.Remove(this);
    m_editor->IR_Release();
}

void CConsole::SelectCommand()
{
    if (m_cmd_history.empty())
    {
        return;
    }
    VERIFY(0 <= m_cmd_history_idx && m_cmd_history_idx < (int)m_cmd_history.size());

    vecHistory::reverse_iterator it_rb = m_cmd_history.rbegin() + m_cmd_history_idx;
    ec().set_edit((*it_rb).c_str());
    reset_selected_tip();
}

void CConsole::Execute(LPCSTR cmd) { ExecuteCommand(cmd, false); }
void CConsole::ExecuteScript(LPCSTR str)
{
    u32 str_size = xr_strlen(str);
    PSTR buf = (PSTR)_alloca((str_size + 10) * sizeof(char));
    xr_strcpy(buf, str_size + 10, "cfg_load ");
    xr_strcat(buf, str_size + 10, str);
    Execute(buf);
}

// -------------------------------------------------------------------------------------------------

IConsole_Command* CConsole::find_next_cmd(LPCSTR in_str, shared_str& out_str)
{
    LPCSTR radmin_cmd_name = "ra ";
    bool b_ra = (in_str == strstr(in_str, radmin_cmd_name));
    u32 offset = (b_ra) ? xr_strlen(radmin_cmd_name) : 0;

    LPSTR t2;
    STRCONCAT(t2, in_str + offset, " ");

    vecCMD_IT it = Commands.lower_bound(t2);
    if (it != Commands.end())
    {
        IConsole_Command* cc = it->second;
        LPCSTR name_cmd = cc->Name();
        u32 name_cmd_size = xr_strlen(name_cmd);
        PSTR new_str = (PSTR)_alloca((offset + name_cmd_size + 2) * sizeof(char));

        xr_strcpy(new_str, offset + name_cmd_size + 2, (b_ra) ? radmin_cmd_name : "");
        xr_strcat(new_str, offset + name_cmd_size + 2, name_cmd);

        out_str._set((LPCSTR)new_str);
        return cc;
    }
    return NULL;
}

bool CConsole::add_next_cmds(LPCSTR in_str, vecTipsEx& out_v)
{
    u32 cur_count = out_v.size();
    if (cur_count >= MAX_TIPS_COUNT)
    {
        return false;
    }

    LPSTR t2;
    STRCONCAT(t2, in_str, " ");

    shared_str temp;
    IConsole_Command* cc = find_next_cmd(t2, temp);
    if (!cc || temp.size() == 0)
    {
        return false;
    }

    bool res = false;
    for (u32 i = cur_count; i < MAX_TIPS_COUNT * 2; ++i) // fake=protect
    {
        temp._set(cc->Name());
        bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
        if (!dup)
        {
            TipString ts(temp);
            out_v.push_back(ts);
            res = true;
        }
        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            break; // for
        }
        LPSTR t3;
        STRCONCAT(t3, out_v.back().text.c_str(), " ");
        cc = find_next_cmd(t3, temp);
        if (!cc)
        {
            break; // for
        }
    } // for
    return res;
}

bool CConsole::add_internal_cmds(LPCSTR in_str, vecTipsEx& out_v)
{
    u32 cur_count = out_v.size();
    if (cur_count >= MAX_TIPS_COUNT)
    {
        return false;
    }
    u32 in_sz = xr_strlen(in_str);

    bool res = false;
    // word in begin
    xr_string name2;
    vecCMD_IT itb = Commands.begin();
    vecCMD_IT ite = Commands.end();
    for (; itb != ite; ++itb)
    {
        LPCSTR name = itb->first;
        u32 name_sz = xr_strlen(name);
        if (name_sz >= in_sz)
        {
            name2.assign(name, in_sz);
            if (!xr_stricmp(name2.c_str(), in_str))
            {
                shared_str temp;
                temp._set(name);
                bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
                if (!dup)
                {
                    out_v.push_back(TipString(temp, 0, in_sz));
                    res = true;
                }
            }
        }

        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            return res;
        }
    } // for

    // word in internal
    itb = Commands.begin();
    ite = Commands.end();
    for (; itb != ite; ++itb)
    {
        LPCSTR name = itb->first;
        LPCSTR fd_str = strstr(name, in_str);
        if (fd_str)
        {
            shared_str temp;
            temp._set(name);
            bool dup = (std::find(out_v.begin(), out_v.end(), temp) != out_v.end());
            if (!dup)
            {
                u32 name_sz = xr_strlen(name);
                int fd_sz = name_sz - xr_strlen(fd_str);
                out_v.push_back(TipString(temp, fd_sz, fd_sz + in_sz));
                res = true;
            }
        }
        if (out_v.size() >= MAX_TIPS_COUNT)
        {
            return res;
        }
    } // for

    return res;
}

void CConsole::update_tips()
{
    m_temp_tips.clear();
    m_tips.clear();

    m_cur_cmd = NULL;
    if (!bVisible)
    {
        return;
    }

    LPCSTR cur = ec().str_edit();
    u32 cur_length = xr_strlen(cur);

    if (cur_length == 0)
    {
        m_prev_length_str = 0;
        return;
    }

    if (m_prev_length_str != cur_length)
    {
        reset_selected_tip();
    }
    m_prev_length_str = cur_length;

    PSTR first = (PSTR)_alloca((cur_length + 1) * sizeof(char));
    PSTR last = (PSTR)_alloca((cur_length + 1) * sizeof(char));
    text_editor::split_cmd(first, last, cur);

    u32 first_lenght = xr_strlen(first);

    if ((first_lenght > 2) && (first_lenght + 1 <= cur_length)) // param
    {
        if (cur[first_lenght] == ' ')
        {
            if (m_tips_mode != 2)
            {
                reset_selected_tip();
            }

            vecCMD_IT it = Commands.find(first);
            if (it != Commands.end())
            {
                IConsole_Command* cc = it->second;

                u32 mode = 0;
                if ((first_lenght + 2 <= cur_length) && (cur[first_lenght] == ' ') && (cur[first_lenght + 1] == ' '))
                {
                    mode = 1;
                    last += 1; // fake: next char
                }

                cc->fill_tips(m_temp_tips, mode);
                m_tips_mode = 2;
                m_cur_cmd._set(first);
                select_for_filter(last, m_temp_tips, m_tips);

                if (m_tips.size() == 0)
                {
                    m_tips.push_back(TipString("(empty)"));
                }
                if ((int)m_tips.size() <= m_select_tip)
                {
                    reset_selected_tip();
                }
                return;
            }
        }
    }

    // cmd name
    {
        add_internal_cmds(cur, m_tips);
        // add_next_cmds( cur, m_tips );
        m_tips_mode = 1;
    }

    if (m_tips.size() == 0)
    {
        m_tips_mode = 0;
        reset_selected_tip();
    }
    if ((int)m_tips.size() <= m_select_tip)
    {
        reset_selected_tip();
    }
}

void CConsole::select_for_filter(LPCSTR filter_str, vecTips& in_v, vecTipsEx& out_v)
{
    out_v.clear();
    u32 in_count = in_v.size();
    if (in_count == 0 || !filter_str)
    {
        return;
    }

    bool all = (xr_strlen(filter_str) == 0);

    //vecTips::iterator itb = in_v.begin();
    //vecTips::iterator ite = in_v.end();
    //for (; itb != ite; ++itb)
    for (auto& it : in_v)
    {
        shared_str const& str = it;
        if (all)
            out_v.push_back(TipString(str));
        else
        {
            pcstr fd_str = strstr(str.c_str(), filter_str);
            if (fd_str)
            {
                int fd_sz = str.size() - xr_strlen(fd_str);
                TipString ts(str, fd_sz, fd_sz + xr_strlen(filter_str));
                out_v.push_back(ts);
            }
        }
    } // for
}
