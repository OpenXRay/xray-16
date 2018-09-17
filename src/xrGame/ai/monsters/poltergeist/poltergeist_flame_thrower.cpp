#include "StdAfx.h"
#include "poltergeist.h"
#include "xrServerEntities/xrMessages.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/level_graph.h"
#include "Level.h"
#include "ai_space.h"
#include "restricted_object.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "ai/monsters/ai_monster_effector.h"

CPolterFlame::CPolterFlame(CPoltergeist* polter) : inherited(polter) {}
CPolterFlame::~CPolterFlame() {}
void CPolterFlame::load(LPCSTR section)
{
    inherited::load(section);

    m_sound.create(pSettings->r_string(section, "flame_sound"), st_Effect, SOUND_TYPE_WORLD);

    m_particles_prepare = pSettings->r_string(section, "flame_particles_prepare");
    m_particles_fire = pSettings->r_string(section, "flame_particles_fire");
    m_particles_stop = pSettings->r_string(section, "flame_particles_stop");

    m_time_fire_delay = pSettings->r_u32(section, "flame_fire_time_delay");
    m_time_fire_play = pSettings->r_u32(section, "flame_fire_time_play");

    m_length = pSettings->r_float(section, "flame_length");
    m_hit_value = pSettings->r_float(section, "flame_hit_value");
    m_hit_delay = pSettings->r_u32(section, "flame_hit_delay");

    m_count = pSettings->r_u32(section, "flames_count");
    m_delay = pSettings->r_u32(section, "flames_delay");

    m_min_flame_dist = pSettings->r_float(section, "flame_min_dist");
    m_max_flame_dist = pSettings->r_float(section, "flame_max_dist");
    m_min_flame_height = pSettings->r_float(section, "flame_min_height");
    m_max_flame_height = pSettings->r_float(section, "flame_max_height");

    m_pmt_aura_radius = pSettings->r_float(section, "flame_aura_radius");

    //-----------------------------------------------------------------------------------------
    // Scanner
    m_scan_radius = pSettings->r_float(section, "flame_scan_radius");
    read_delay(section, "flame_scan_delay_min_max", m_scan_delay_min, m_scan_delay_max);

    // load scan effector
    LPCSTR ppi_section = pSettings->r_string(section, "flame_scan_effector_section");
    m_scan_effector_info.duality.h = pSettings->r_float(ppi_section, "duality_h");
    m_scan_effector_info.duality.v = pSettings->r_float(ppi_section, "duality_v");
    m_scan_effector_info.gray = pSettings->r_float(ppi_section, "gray");
    m_scan_effector_info.blur = pSettings->r_float(ppi_section, "blur");
    m_scan_effector_info.noise.intensity = pSettings->r_float(ppi_section, "noise_intensity");
    m_scan_effector_info.noise.grain = pSettings->r_float(ppi_section, "noise_grain");
    m_scan_effector_info.noise.fps = pSettings->r_float(ppi_section, "noise_fps");
    VERIFY(!fis_zero(m_scan_effector_info.noise.fps));

    sscanf(pSettings->r_string(ppi_section, "color_base"), "%f,%f,%f", &m_scan_effector_info.color_base.r,
        &m_scan_effector_info.color_base.g, &m_scan_effector_info.color_base.b);
    sscanf(pSettings->r_string(ppi_section, "color_gray"), "%f,%f,%f", &m_scan_effector_info.color_gray.r,
        &m_scan_effector_info.color_gray.g, &m_scan_effector_info.color_gray.b);
    sscanf(pSettings->r_string(ppi_section, "color_add"), "%f,%f,%f", &m_scan_effector_info.color_add.r,
        &m_scan_effector_info.color_add.g, &m_scan_effector_info.color_add.b);

    m_scan_effector_time = pSettings->r_float(ppi_section, "time");
    m_scan_effector_time_attack = pSettings->r_float(ppi_section, "time_attack");
    m_scan_effector_time_release = pSettings->r_float(ppi_section, "time_release");

    m_scan_sound.create(pSettings->r_string(section, "flame_scan_sound"), st_Effect, SOUND_TYPE_WORLD);
    //-----------------------------------------------------------------------------------------

    m_state_scanning = false;
    m_scan_next_time = 0;

    m_time_flame_started = 0;
}

void CPolterFlame::create_flame(const IGameObject* target_object)
{
    Fvector position;
    if (!get_valid_flame_position(target_object, position))
        return;

    SFlameElement* element = new SFlameElement();

    element->position = position;
    element->target_object = target_object;
    element->time_started = time();
    element->sound.clone(m_sound, st_Effect, SOUND_TYPE_WORLD);
    element->sound.play_at_pos(m_object, element->position);
    element->particles_object = 0;
    element->time_last_hit = 0;

    Fvector target_point = get_head_position(const_cast<IGameObject*>(target_object));
    element->target_dir.sub(target_point, element->position);
    element->target_dir.normalize();

    m_flames.push_back(element);
    select_state(element, ePrepare);

    m_time_flame_started = time();
}

void CPolterFlame::select_state(SFlameElement* elem, EFlameState state)
{
    elem->state = state;
    elem->time_started = time();

    switch (elem->state)
    {
    case ePrepare:
        // start prepare particles
        m_object->PlayParticles(m_particles_prepare, elem->position, elem->target_dir, TRUE);
        break;
    case eFire:
        // start fire particles
        elem->particles_object = m_object->PlayParticles(m_particles_fire, elem->position, elem->target_dir, FALSE);
        break;
    case eStop:
        // stop fire particles
        if (elem->particles_object)
            CParticlesObject::Destroy(elem->particles_object);

        // start finish particles
        m_object->PlayParticles(m_particles_stop, elem->position, elem->target_dir, TRUE);

        break;
    }
}

struct remove_predicate
{
    bool operator()(CPolterFlame::SFlameElement* element) { return (!element); }
};

void CPolterFlame::update_schedule()
{
    inherited::update_schedule();

    // check all flames
    for (auto it = m_flames.begin(); it != m_flames.end(); it++)
    {
        SFlameElement* elem = *it;

        // test switches to states
        switch (elem->state)
        {
        case ePrepare:
            // check if time_out
            if (elem->time_started + m_time_fire_delay < time())
                select_state(elem, eFire);
            break;
        case eFire:
            if (elem->time_started + m_time_fire_play < time())
                select_state(elem, eStop);
            else
            {
                // check if we need test hit to enemy
                if (elem->time_last_hit + m_hit_delay < time())
                {
                    // test hit
                    collide::rq_result rq;
                    if (Level().ObjectSpace.RayPick(
                            elem->position, elem->target_dir, m_length, collide::rqtBoth, rq, NULL))
                    {
                        if ((rq.O == elem->target_object) && (rq.range < m_length))
                        {
                            float hit_value;
                            hit_value = m_hit_value - m_hit_value * rq.range / m_length;

                            NET_Packet P;
                            SHit HS;
                            HS.GenHeader(GE_HIT,
                                elem->target_object->ID()); //					u_EventGen		(P,GE_HIT,
                                                            //element->target_object->ID());
                            HS.whoID = (m_object->ID()); //					P.w_u16			(ID());
                            HS.weaponID = (m_object->ID()); //					P.w_u16			(ID());
                            HS.dir = (elem->target_dir); //					P.w_dir			(element->target_dir);
                            HS.power = (hit_value); //					P.w_float		(m_flame_hit_value);
                            HS.boneID = (BI_NONE); //					P.w_s16			(BI_NONE);
                            HS.p_in_bone_space = (Fvector().set(
                                0.f, 0.f, 0.f)); //					P.w_vec3		(Fvector().set(0.f,0.f,0.f));
                            HS.impulse = (0.f); //					P.w_float		(0.f);
                            HS.hit_type =
                                (ALife::eHitTypeBurn); //					P.w_u16			(u16(ALife::eHitTypeBurn));

                            HS.Write_Packet(P);
                            m_object->u_EventSend(P);

                            elem->time_last_hit = time();
                        }
                    }
                }
            }
            break;
        case eStop: xr_delete(*it); break;
        }
    }

    // remove all flames in state stop

    // удалить все элементы, выполнение которых закончено
    m_flames.erase(std::remove_if(m_flames.begin(), m_flames.end(), remove_predicate()), m_flames.end());

    bool const detected = m_object->get_current_detection_level() >= m_object->get_detection_success_level();

    CEntityAlive const* enemy = Actor();
    // check if we can create another flame
    if (m_object->g_Alive() && enemy && m_flames.size() < m_count && !m_object->get_actor_ignore() && detected)
    {
        // check aura radius and accessibility
        float dist = enemy->Position().distance_to(m_object->Position());
        if ((dist < m_pmt_aura_radius) && m_object->control().path_builder().accessible(enemy->Position()))
        {
            // check timing
            if (m_time_flame_started + m_delay < time())
            {
                create_flame(enemy);
            }
        }
    }
}

void CPolterFlame::on_destroy()
{
    inherited::on_destroy();

    auto I = m_flames.begin();
    auto E = m_flames.end();

    // Пройти по всем объектам и проверить на хит врага
    for (; I != E; ++I)
    {
        if ((*I)->sound._feedback())
            (*I)->sound.stop();
        if ((*I)->particles_object)
            CParticlesObject::Destroy((*I)->particles_object);

        xr_delete((*I));
    }

    m_flames.clear();

    if (m_scan_sound._feedback())
        m_scan_sound.stop();
}

void CPolterFlame::on_die()
{
    inherited::on_die();
    if (m_scan_sound._feedback())
        m_scan_sound.stop();
}

#define FIND_POINT_ATTEMPT_COUNT 5

bool CPolterFlame::get_valid_flame_position(const IGameObject* target_object, Fvector& res_pos)
{
    const CGameObject* Obj = smart_cast<const CGameObject*>(target_object);
    if (!Obj)
        return (false);

    Fvector dir;
    float h, p;

    Fvector vertex_position;
    Fvector new_pos;

    for (u32 i = 0; i < FIND_POINT_ATTEMPT_COUNT; i++)
    {
        target_object->Direction().getHP(h, p);
        h = Random.randF(0, PI_MUL_2);
        dir.setHP(h, p);
        dir.normalize();

        vertex_position = ai().level_graph().vertex_position(Obj->ai_location().level_vertex_id());
        new_pos.mad(vertex_position, dir, Random.randF(m_min_flame_dist, m_max_flame_dist));

        u32 node = ai().level_graph().check_position_in_direction(
            Obj->ai_location().level_vertex_id(), vertex_position, new_pos);
        if (node != u32(-1))
        {
            res_pos = ai().level_graph().vertex_position(node);
            res_pos.y += Random.randF(m_min_flame_height, m_max_flame_height);
            return (true);
        }
    }

    float angle =
        ai().level_graph().vertex_high_cover_angle(Obj->ai_location().level_vertex_id(), PI_DIV_6, std::less<float>());

    dir.set(1.f, 0.f, 0.f);
    dir.setHP(angle + PI, 0.f);
    dir.normalize();

    vertex_position = ai().level_graph().vertex_position(Obj->ai_location().level_vertex_id());
    new_pos.mad(vertex_position, dir, Random.randF(m_min_flame_dist, m_max_flame_dist));

    u32 node =
        ai().level_graph().check_position_in_direction(Obj->ai_location().level_vertex_id(), vertex_position, new_pos);
    if (node != u32(-1))
    {
        res_pos = ai().level_graph().vertex_position(node);
        res_pos.y += Random.randF(m_min_flame_height, m_max_flame_height);
        return (true);
    }

    return (false);
}
