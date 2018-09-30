#include "StdAfx.h"
#include "UISleepStatic.h"
#include "Actor_Flags.h"
#include "Level.h"
#include "date_time.h"
#include "xrUICore/XML/UITextureMaster.h"

CUISleepStatic::CUISleepStatic() : m_cur_time(0){};

void CUISleepStatic::Draw()
{
    //	inherited::Draw();
    m_UIStaticItem.Render();
    m_UIStaticItem2.Render();
}

void CUISleepStatic::Update()
{
    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
    split_time(Level().GetGameTime(), year, month, day, hours, mins, secs, milisecs);

    u32 start_pixel = 0, end_pixel = 0, start_pixel2 = 0, end_pixel2 = 0;
    hours += psActorSleepTime - 1;
    if (hours >= 24)
        hours -= 24;

    start_pixel = hours * 85;
    end_pixel = (hours + 7) * 85;
    if (end_pixel > 2048)
    {
        end_pixel2 = end_pixel - 2048;
        end_pixel = 2048;
    }

    Fvector2 parent_pos = GetParent()->GetWndPos();
    Fvector2 pos = GetWndPos();
    pos.x += parent_pos.x;
    pos.y += parent_pos.y;

    Frect r = Frect().set((float)start_pixel, 0.0f, (float)end_pixel, 128.0f);
    m_UIStaticItem.SetTextureRect(r);
    m_UIStaticItem.SetSize(Fvector2().set(iFloor((end_pixel - start_pixel) * UI().get_current_kx()), 128));
    m_UIStaticItem.SetPos(pos.x, pos.y);
    if (end_pixel2 > 0)
    {
        r.set((float)start_pixel2, 0.0f, (float)end_pixel2, 128.0f);
        m_UIStaticItem2.SetTextureRect(r);
        m_UIStaticItem2.SetSize(Fvector2().set(iFloor(end_pixel2 * UI().get_current_kx()), 128));
        m_UIStaticItem2.SetPos(m_UIStaticItem.GetPosX() + m_UIStaticItem.GetSize().x, m_UIStaticItem.GetPosY());
    }
    else
        m_UIStaticItem2.SetSize(Fvector2().set(1, 1));
}

void CUISleepStatic::InitTextureEx(LPCSTR tex_name, LPCSTR sh_name)
{
    inherited::InitTextureEx(tex_name, sh_name);

    LPCSTR res_shname = GEnv.UIRender->UpdateShaderName(tex_name, sh_name);
    CUITextureMaster::InitTexture(tex_name, &m_UIStaticItem2, res_shname);

    Fvector2 p = GetWndPos();
    m_UIStaticItem2.SetPos(p.x, p.y);
    p.set(1, 1);
    m_UIStaticItem2.SetSize(p);
}
