#include "StdAfx.h"
#include "WeaponBinocularsVision.h"
#include "WeaponBinoculars.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "entity_alive.h"
#include "visual_memory_manager.h"
#include "Actor.h"
#include "actor_memory.h"
#include "relation_registry.h"
#include "Common/object_broker.h"

#include "game_base_space.h"
#include "Level.h"
#include "game_cl_base.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "xrEngine/IGame_Persistent.h"

#define RECT_SIZE 11

extern u32 C_ON_ENEMY;
extern u32 C_ON_NEUTRAL;
extern u32 C_ON_FRIEND;

struct FindVisObjByObject
{
    const IGameObject* O;
    FindVisObjByObject(const IGameObject* o) : O(o) {}
    bool operator()(const SBinocVisibleObj* vis) { return (O == vis->m_object); }
};

void SBinocVisibleObj::create_default(u32 color)
{
    Frect r = {0, 0, RECT_SIZE, RECT_SIZE};
    m_lt.InitTexture("ui" DELIMITER "ui_enemy_frame");
    m_lt.SetWndRect(r);
    m_lt.SetAlignment(waCenter);
    m_lb.InitTexture("ui" DELIMITER "ui_enemy_frame");
    m_lb.SetWndRect(r);
    m_lb.SetAlignment(waCenter);
    m_rt.InitTexture("ui" DELIMITER "ui_enemy_frame");
    m_rt.SetWndRect(r);
    m_rt.SetAlignment(waCenter);
    m_rb.InitTexture("ui" DELIMITER "ui_enemy_frame");
    m_rb.SetWndRect(r);
    m_rb.SetAlignment(waCenter);

    m_lt.SetTextureRect(Frect().set(0, 0, RECT_SIZE, RECT_SIZE));
    m_lb.SetTextureRect(Frect().set(0, 32 - RECT_SIZE, RECT_SIZE, 32));
    m_rt.SetTextureRect(Frect().set(32 - RECT_SIZE, 0, 32, RECT_SIZE));
    m_rb.SetTextureRect(Frect().set(32 - RECT_SIZE, 32 - RECT_SIZE, 32, 32));

    u32 clr = subst_alpha(color, 128);
    m_lt.SetTextureColor(clr);
    m_lb.SetTextureColor(clr);
    m_rt.SetTextureColor(clr);
    m_rb.SetTextureColor(clr);

    cur_rect.set(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

    m_flags.zero();
}

void SBinocVisibleObj::Draw()
{
    if (m_flags.test(flVisObjNotValid))
        return;

    m_lt.Draw();
    m_lb.Draw();
    m_rt.Draw();
    m_rb.Draw();
}

void SBinocVisibleObj::Update()
{
    m_flags.set(flVisObjNotValid, true);

    if (!m_object->Visual())
        return;

    Fbox b = m_object->Visual()->getVisData().box;

    Fmatrix xform;
    xform.mul(Device.mFullTransform, m_object->XFORM());
    Fvector2 mn = {flt_max, flt_max}, mx = {flt_min, flt_min};

    for (u32 k = 0; k < 8; ++k)
    {
        Fvector p;
        b.getpoint(k, p);
        xform.transform(p);
        mn.x = _min(mn.x, p.x);
        mn.y = _min(mn.y, p.y);
        mx.x = _max(mx.x, p.x);
        mx.y = _max(mx.y, p.y);
    }
    static Frect screen_rect = {-1.0f, -1.0f, 1.0f, 1.0f};

    Frect new_rect;
    new_rect.lt = mn;
    new_rect.rb = mx;

    if (FALSE == screen_rect.intersected(new_rect))
        return;
    if (new_rect.in(screen_rect.lt) && new_rect.in(screen_rect.rb))
        return;

    std::swap(mn.y, mx.y);
    mn.x = (1.f + mn.x) / 2.f * UI_BASE_WIDTH;
    mx.x = (1.f + mx.x) / 2.f * UI_BASE_WIDTH;
    mn.y = (1.f - mn.y) / 2.f * UI_BASE_HEIGHT;
    mx.y = (1.f - mx.y) / 2.f * UI_BASE_HEIGHT;

    if (mx.x - mn.x < RECT_SIZE)
        mx.x = mn.x + RECT_SIZE;

    if (mx.y - mn.y < RECT_SIZE)
        mx.y = mn.y + RECT_SIZE;

    if (m_flags.is(flTargetLocked))
    {
        cur_rect.lt.set(mn);
        cur_rect.rb.set(mx);
    }
    else
    {
        cur_rect.lt.x += (mn.x - cur_rect.lt.x) * m_upd_speed * Device.fTimeDelta;
        cur_rect.lt.y += (mn.y - cur_rect.lt.y) * m_upd_speed * Device.fTimeDelta;
        cur_rect.rb.x += (mx.x - cur_rect.rb.x) * m_upd_speed * Device.fTimeDelta;
        cur_rect.rb.y += (mx.y - cur_rect.rb.y) * m_upd_speed * Device.fTimeDelta;
        if (mn.similar(cur_rect.lt, 2.f) && mx.similar(cur_rect.rb, 2.f))
        {
            // target locked
            m_flags.set(flTargetLocked, true);
            u32 clr = subst_alpha(m_lt.GetTextureColor(), 255);

            //-----------------------------------------------------
            CActor* pActor = nullptr;
            if (IsGameTypeSingle())
                pActor = Actor();
            else
            {
                if (Level().CurrentViewEntity())
                {
                    pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
                }
            }
            if (pActor)
            {
                //-----------------------------------------------------

                CInventoryOwner* our_inv_owner = smart_cast<CInventoryOwner*>(pActor);
                CInventoryOwner* others_inv_owner = smart_cast<CInventoryOwner*>(m_object);
                CBaseMonster* monster = smart_cast<CBaseMonster*>(m_object);

                if (our_inv_owner && others_inv_owner && !monster)
                {
                    if (IsGameTypeSingle())
                    {
                        switch (RELATION_REGISTRY().GetRelationType(others_inv_owner, our_inv_owner))
                        {
                        case ALife::eRelationTypeEnemy: clr = C_ON_ENEMY; break;
                        case ALife::eRelationTypeNeutral: clr = C_ON_NEUTRAL; break;
                        case ALife::eRelationTypeFriend: clr = C_ON_FRIEND; break;
                        }
                    }
                    else
                    {
                        CEntityAlive* our_ealive = smart_cast<CEntityAlive*>(pActor);
                        CEntityAlive* others_ealive = smart_cast<CEntityAlive*>(m_object);
                        if (our_ealive && others_ealive)
                        {
                            if (Game().IsEnemy(our_ealive, others_ealive))
                                clr = C_ON_ENEMY;
                            else
                                clr = C_ON_FRIEND;
                        }
                    }
                }
            }

            m_lt.SetTextureColor(clr);
            m_lb.SetTextureColor(clr);
            m_rt.SetTextureColor(clr);
            m_rb.SetTextureColor(clr);
        }
    }

    m_lt.SetWndPos(Fvector2().set((cur_rect.lt.x), (cur_rect.lt.y)));
    m_lb.SetWndPos(Fvector2().set((cur_rect.lt.x), (cur_rect.rb.y)));
    m_rt.SetWndPos(Fvector2().set((cur_rect.rb.x), (cur_rect.lt.y)));
    m_rb.SetWndPos(Fvector2().set((cur_rect.rb.x), (cur_rect.rb.y)));

    m_flags.set(flVisObjNotValid, false);
}

CBinocularsVision::CBinocularsVision(const shared_str& sect) { Load(sect); }
CBinocularsVision::~CBinocularsVision() { delete_data(m_active_objects); }
void CBinocularsVision::Update()
{
    if (GEnv.isDedicatedServer)
        return;
    //-----------------------------------------------------
    const CActor* pActor = nullptr;
    if (IsGameTypeSingle())
        pActor = Actor();
    else
    {
        if (Level().CurrentViewEntity())
        {
            pActor = smart_cast<const CActor*>(Level().CurrentViewEntity());
        }
    }
    if (!pActor)
        return;
    //-----------------------------------------------------
    const CVisualMemoryManager::VISIBLES& vVisibles = pActor->memory().visual().objects();

    VIS_OBJECTS_IT it = m_active_objects.begin();
    for (; it != m_active_objects.end(); ++it)
        (*it)->m_flags.set(flVisObjNotValid, true);

    CVisualMemoryManager::VISIBLES::const_iterator v_it = vVisibles.begin();
    for (; v_it != vVisibles.end(); ++v_it)
    {
        const IGameObject* _object_ = (*v_it).m_object;
        if (!pActor->memory().visual().visible_right_now(smart_cast<const CGameObject*>(_object_)))
            continue;

        IGameObject* object_ = const_cast<IGameObject*>(_object_);

        CEntityAlive* EA = smart_cast<CEntityAlive*>(object_);
        if (!EA || !EA->g_Alive())
            continue;

        FindVisObjByObject f(object_);
        VIS_OBJECTS_IT found;
        found = std::find_if(m_active_objects.begin(), m_active_objects.end(), f);

        if (found != m_active_objects.end())
        {
            (*found)->m_flags.set(flVisObjNotValid, false);
        }
        else
        {
            m_active_objects.push_back(new SBinocVisibleObj());
            SBinocVisibleObj* new_vis_obj = m_active_objects.back();
            new_vis_obj->m_flags.set(flVisObjNotValid, false);
            new_vis_obj->m_object = object_;
            new_vis_obj->create_default(m_frame_color.get());
            new_vis_obj->m_upd_speed = m_rotating_speed;

            m_sounds.PlaySound("found_snd", Fvector().set(0, 0, 0), nullptr, true);
        }
    }
    std::sort(m_active_objects.begin(), m_active_objects.end());

    while (m_active_objects.size() && m_active_objects.back()->m_flags.test(flVisObjNotValid))
    {
        xr_delete(m_active_objects.back());
        m_active_objects.pop_back();
    }

    it = m_active_objects.begin();
    for (; it != m_active_objects.end(); ++it)
    {
        SBinocVisibleObj* visObj = (*it);

        const bool bLocked = visObj->m_flags.test(flTargetLocked);

        (*it)->Update();

        if (bLocked != visObj->m_flags.test(flTargetLocked))
            m_sounds.PlaySound("catch_snd", Fvector().set(0, 0, 0), nullptr, true);
    }
}

void CBinocularsVision::Draw()
{
    VIS_OBJECTS_IT it = m_active_objects.begin();
    for (; it != m_active_objects.end(); ++it)
        (*it)->Draw();
}

void CBinocularsVision::Load(const shared_str& section)
{
    m_rotating_speed = pSettings->r_float(section, "vis_frame_speed");
    m_frame_color = pSettings->r_fcolor(section, "vis_frame_color");
    m_sounds.LoadSound(section.c_str(), "found_snd", "found_snd", false, SOUND_TYPE_NO_SOUND);
    m_sounds.LoadSound(section.c_str(), "catch_snd", "catch_snd", false, SOUND_TYPE_NO_SOUND);
}

void CBinocularsVision::remove_links(IGameObject* object)
{
    VIS_OBJECTS::iterator I =
        std::find_if(m_active_objects.begin(), m_active_objects.end(), FindVisObjByObject(object));
    if (I == m_active_objects.end())
        return;

    m_active_objects.erase(I);
}
