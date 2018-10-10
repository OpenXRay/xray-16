// exxZERO Time Stamp AddIn. Document modified at : Thursday, March 07, 2002 14:12:50 , by user : Oles , from computer :
// OLES
#include "StdAfx.h"
#include "HitMarker.h"
#include "xrEngine/Render.h"
#include "xrEngine/LightAnimLibrary.h"
#include "xrUICore/Static/UIStaticItem.h"

#include "Grenade.h"

#include "Include/xrRender/UIRender.h"
#include "Include/xrRender/UIShader.h"

//--------------------------------------------------------------------
CHitMarker::CHitMarker()
{
    InitShader(pSettings->r_string("hud_hitmark", "hit_mark_texture"));
    InitShader_Grenade(pSettings->r_string("hud_hitmark", "grenade_mark_texture"));
}

void CHitMarker::InitShader(LPCSTR tex_name) { hShader2->create("hud" DELIMITER "default", tex_name); }
void CHitMarker::InitShader_Grenade(LPCSTR tex_name)
{
    hShader_Grenade->create("hud" DELIMITER "default", tex_name); // "hud" DELIMITER "default2"
}

//--------------------------------------------------------------------
CHitMarker::~CHitMarker()
{
    while (m_HitMarks.size())
    {
        xr_delete(m_HitMarks.front());
        m_HitMarks.pop_front();
    }

    while (m_GrenadeMarks.size())
    {
        xr_delete(m_GrenadeMarks.front());
        m_GrenadeMarks.pop_front();
    }
}
//--------------------------------------------------------------------

void CHitMarker::Render()
{
    float h1, p1;
    Device.vCameraDirection.getHP(h1, p1);

    while (m_HitMarks.size() && !m_HitMarks.front()->IsActive())
    {
        xr_delete(m_HitMarks.front());
        m_HitMarks.pop_front();
    }

    while (m_GrenadeMarks.size() && !m_GrenadeMarks.front()->IsActive())
    {
        xr_delete(m_GrenadeMarks.front());
        m_GrenadeMarks.pop_front();
    }

    HITMARKS::iterator it_b = m_HitMarks.begin();
    HITMARKS::iterator it_e = m_HitMarks.end();
    for (; it_b != it_e; ++it_b)
    {
        (*it_b)->Draw(-h1);
    }

    GRENADEMARKS::iterator itg_b = m_GrenadeMarks.begin();
    GRENADEMARKS::iterator itg_e = m_GrenadeMarks.end();
    for (; itg_b != itg_e; ++itg_b)
    {
        (*itg_b)->Draw(-h1);
    }
}
//--------------------------------------------------------------------

void CHitMarker::Hit(const Fvector& dir)
{
    Fvector hit_dir = dir;
    hit_dir.mul(-1.0f);
    m_HitMarks.push_back(new SHitMark(hShader2, hit_dir));
}

bool CHitMarker::AddGrenade_ForMark(CGrenade* grn)
{
    if (!grn)
        return false;
    u16 new_id = grn->ID();

    GRENADEMARKS::iterator it_b = m_GrenadeMarks.begin();
    GRENADEMARKS::iterator it_e = m_GrenadeMarks.end();

    for (; it_b != it_e; ++it_b)
    {
        if ((*it_b)->removed_grenade)
            continue;
        if ((*it_b)->p_grenade->ID() == new_id)
            return false;
    }

    m_GrenadeMarks.push_back(new SGrenadeMark(hShader_Grenade, grn));

    return true;
}

void CHitMarker::Update_GrenadeView(Fvector& pos_actor)
{
    GRENADEMARKS::iterator it_b = m_GrenadeMarks.begin();
    GRENADEMARKS::iterator it_e = m_GrenadeMarks.end();

    for (; it_b != it_e; ++it_b)
    {
        if ((*it_b)->removed_grenade)
            continue;

        CGrenade* grn = (*it_b)->p_grenade;
        if (grn->IsExploding())
        {
            (*it_b)->removed_grenade = true;
            continue;
        }

        Fvector pos_grn, dir;
        grn->Center(pos_grn);

        dir.sub(pos_grn, pos_actor);
        // dir.sub( pos_actor, pos_grn );
        dir.normalize();

        (*it_b)->Update(dir.getH());
    }
}

void CHitMarker::net_Relcase(IGameObject* obj)
{
    u16 remove_id = obj->ID();

    GRENADEMARKS::iterator it_b = m_GrenadeMarks.begin();
    GRENADEMARKS::iterator it_e = m_GrenadeMarks.end();

    for (; it_b != it_e; ++it_b)
    {
        if ((*it_b)->removed_grenade)
            continue;

        if ((*it_b)->p_grenade->ID() == remove_id)
        {
            (*it_b)->removed_grenade = true;
            // break;
        }
    } // for
}

//==========================================================================================

SHitMark::SHitMark(const ui_shader& sh, const Fvector& dir)
{
    m_StartTime = Device.fTimeGlobal;
    m_lanim = LALib.FindItem("hud_hit_mark");
    m_HitDirection = dir.getH();
    m_UIStaticItem = new CUIStaticItem();
    m_UIStaticItem->SetShader(sh);
    m_UIStaticItem->SetPos(256.0f, 128.0f);
    m_UIStaticItem->SetSize(Fvector2().set(512.0f, 512.0f));
}

SHitMark::~SHitMark() { xr_delete(m_UIStaticItem); }
bool SHitMark::IsActive() { return ((Device.fTimeGlobal - m_StartTime) < m_lanim->Length_sec()); }
void SHitMark::Draw(float cam_dir)
{
    int frame;
    u32 clr = m_lanim->CalculateRGB(Device.fTimeGlobal - m_StartTime, frame);
    m_UIStaticItem->SetTextureColor(subst_alpha(m_UIStaticItem->GetTextureColor(), color_get_A(clr)));

    m_UIStaticItem->Render(cam_dir + m_HitDirection);
}

// ######################################################################################

SGrenadeMark::SGrenadeMark(const ui_shader& sh, CGrenade* grn)
{
    p_grenade = grn;
    removed_grenade = false;
    m_LastTime = Device.fTimeGlobal;
    m_LightAnim = LALib.FindItem("hud_hit_mark");
    m_Angle = 0.0f;

    m_UIStaticItem = new CUIStaticItem();
    m_UIStaticItem->SetShader(sh);
    float xs = 640.0f;
    float ys = 640.0f;
    m_UIStaticItem->SetPos((UI_BASE_WIDTH - xs) * 0.5f, (UI_BASE_HEIGHT - ys) * 0.5f);
    m_UIStaticItem->SetSize(Fvector2().set(xs, ys));
}

SGrenadeMark::~SGrenadeMark() { xr_delete(m_UIStaticItem); }
void SGrenadeMark::Update(float angle)
{
    m_Angle = angle;
    m_LastTime = Device.fTimeGlobal;
}

bool SGrenadeMark::IsActive() const { return (2.0f * (Device.fTimeGlobal - m_LastTime) < m_LightAnim->Length_sec()); }
void SGrenadeMark::Draw(float cam_dir)
{
    int frame;
    u32 clr = m_LightAnim->CalculateRGB(2.0f * (Device.fTimeGlobal - m_LastTime), frame);
    m_UIStaticItem->SetTextureColor(subst_alpha(m_UIStaticItem->GetTextureColor(), color_get_A(clr)));

    m_UIStaticItem->Render(cam_dir + m_Angle);
}
