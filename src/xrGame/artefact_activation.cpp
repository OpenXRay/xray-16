////////////////////////////////////////////////////////////////////////////////
//	Module		:	artefact_activation.cpp
//	Created		:	19.12.2007
//	Modified	:	19.12.2007
//	Autor		:	Alexander Maniluk
//	Description	:	artefact activation class
////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "artefact_activation.h"

#include "xrPhysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "game_cl_base.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"

#include "Inventory.h"
#include "Level.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrPhysics/IPHWorld.h"
#include "restriction_space.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrNetServer/NET_Messages.h"

SArtefactActivation::SArtefactActivation(CArtefact* af, u32 owner_id)
{
    m_af = af;
    Load();
    m_light = GEnv.Render->light_create();
    m_light->set_shadow(true);
    m_owner_id = owner_id;
    m_in_process = false;
}
SArtefactActivation::~SArtefactActivation() { m_light.destroy(); }
void SArtefactActivation::Load()
{
    for (int i = 0; i < (int)eMax; ++i)
        m_activation_states.push_back(SStateDef());

    LPCSTR activation_seq = pSettings->r_string(*m_af->cNameSect(), "artefact_activation_seq");

    m_activation_states[(int)eStarting].Load(activation_seq, "starting");
    m_activation_states[(int)eFlying].Load(activation_seq, "flying");
    m_activation_states[(int)eBeforeSpawn].Load(activation_seq, "idle_before_spawning");
    m_activation_states[(int)eSpawnZone].Load(activation_seq, "spawning");
}

void SArtefactActivation::Start()
{
    VERIFY(!physics_world()->Processing());
    m_af->StopLights();
    m_cur_activation_state = eStarting;
    m_cur_state_time = 0.0f;

    m_af->processing_activate();

    NET_Packet P;
    CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, m_af->H_Parent()->ID());
    P.w_u16(m_af->ID());
    if (OnServer())
        CGameObject::u_EventSend(P);
    m_light->set_active(true);
    ChangeEffects();
    m_in_process = true;
}

void SArtefactActivation::Stop()
{
    m_in_process = false;
    m_cur_state_time = eNone;
    m_af->CPHUpdateObject::Deactivate();
    m_af->StartLights();
    ChangeEffects();
}

void SArtefactActivation::UpdateActivation()
{
    if (!m_in_process)
        return;

    VERIFY(!physics_world()->Processing());
    m_cur_state_time += Device.fTimeDelta;
    if (m_cur_state_time >= m_activation_states[int(m_cur_activation_state)].m_time)
    {
        m_cur_activation_state = (EActivationStates)(int)(m_cur_activation_state + 1);

        if (m_cur_activation_state == eMax)
        {
            m_cur_activation_state = eNone;

            m_af->processing_deactivate();
            m_af->CPHUpdateObject::Deactivate();
            m_af->DestroyObject();
        }

        m_cur_state_time = 0.0f;
        ChangeEffects();

        if (m_cur_activation_state == eSpawnZone && OnServer())
            SpawnAnomaly();
    }
    UpdateEffects();
}

void SArtefactActivation::PhDataUpdate(float step)
{
    R_ASSERT(m_af);

    if (!m_af->m_pPhysicsShell)
        return;

    if (m_cur_activation_state == eFlying)
    {
        Fvector dir = {0, -1.f, 0};
        if (Level().ObjectSpace.RayTest(m_af->Position(), dir, 1.0f, collide::rqtBoth, NULL, m_af))
        {
            dir.y = physics_world()->Gravity() * 1.1f;
            m_af->m_pPhysicsShell->applyGravityAccel(dir);
        }
    }
}
void SArtefactActivation::ChangeEffects()
{
    VERIFY(!physics_world()->Processing());
    SStateDef& state_def = m_activation_states[(int)m_cur_activation_state];

    if (m_snd._feedback())
        m_snd.stop();

    if (state_def.m_snd.size())
    {
        m_snd.create(*state_def.m_snd, st_Effect, sg_SourceType);
        m_snd.play_at_pos(m_af, m_af->Position());
    };

    m_light->set_range(state_def.m_light_range);
    m_light->set_color(state_def.m_light_color.r, state_def.m_light_color.g, state_def.m_light_color.b);

    if (state_def.m_particle.size())
    {
        Fvector dir;
        dir.set(0, 1, 0);

        m_af->CParticlesPlayer::StartParticles(state_def.m_particle, dir, m_af->ID(), iFloor(state_def.m_time * 1000));
    };
    if (state_def.m_animation.size())
    {
        IKinematicsAnimated* K = smart_cast<IKinematicsAnimated*>(m_af->Visual());
        if (K)
            K->PlayCycle(*state_def.m_animation);
    }
}

void SArtefactActivation::UpdateEffects()
{
    VERIFY(!physics_world()->Processing());
    if (m_snd._feedback())
        m_snd.set_position(m_af->Position());

    m_light->set_position(m_af->Position());
}

void SArtefactActivation::SpawnAnomaly()
{
    VERIFY(!physics_world()->Processing());
    string128 tmp;
    LPCSTR str = pSettings->r_string("artefact_spawn_zones", *m_af->cNameSect());
    VERIFY3(3 == _GetItemCount(str), "Bad record format in artefact_spawn_zones", str);
    float zone_radius = (float)atof(_GetItem(str, 1, tmp));
    LPCSTR zone_sect = _GetItem(str, 0, tmp); // must be last call of _GetItem... (LPCSTR !!!)

    Fvector pos;
    m_af->Center(pos);
    CSE_Abstract* object = Level().spawn_item(
        zone_sect, pos, (GEnv.isDedicatedServer) ? u32(-1) : m_af->ai_location().level_vertex_id(), 0xffff, true);
    CSE_ALifeAnomalousZone* AlifeZone = smart_cast<CSE_ALifeAnomalousZone*>(object);
    VERIFY(AlifeZone);
    CShapeData::shape_def _shape;
    _shape.data.sphere.P.set(0.0f, 0.0f, 0.0f);
    _shape.data.sphere.R = zone_radius;
    _shape.type = CShapeData::cfSphere;
    AlifeZone->assign_shapes(&_shape, 1);
    //		AlifeZone->m_maxPower		= zone_power;
    AlifeZone->m_owner_id = m_owner_id;
    AlifeZone->m_space_restrictor_type = RestrictionSpace::eRestrictorTypeNone;

    NET_Packet P;
    object->Spawn_Write(P, TRUE);
    Level().Send(P, net_flags(TRUE));
    F_entity_Destroy(object);
    //. #ifdef DEBUG
    Msg("artefact [%s] spawned a zone [%s] at [%f]", *m_af->cName(), zone_sect, Device.fTimeGlobal);
    //. #endif
}
shared_str clear_brackets(LPCSTR src)
{
    if (0 == src)
        return shared_str(0);

    if (NULL == strchr(src, '"'))
        return shared_str(src);

    string512 _original;
    xr_strcpy(_original, src);
    u32 _len = xr_strlen(_original);
    if (0 == _len)
        return shared_str("");
    if ('"' == _original[_len - 1])
        _original[_len - 1] = 0; // skip end
    if ('"' == _original[0])
        return shared_str(&_original[0] + 1); // skip begin
    return shared_str(_original);
}

void SArtefactActivation::SStateDef::Load(LPCSTR section, LPCSTR name)
{
    LPCSTR str = pSettings->r_string(section, name);
    VERIFY(_GetItemCount(str) == 8);

    string128 tmp;

    m_time = (float)atof(_GetItem(str, 0, tmp));

    m_snd = clear_brackets(_GetItem(str, 1, tmp));

    m_light_color.r = (float)atof(_GetItem(str, 2, tmp));
    m_light_color.g = (float)atof(_GetItem(str, 3, tmp));
    m_light_color.b = (float)atof(_GetItem(str, 4, tmp));

    m_light_range = (float)atof(_GetItem(str, 5, tmp));

    m_particle = clear_brackets(_GetItem(str, 6, tmp));
    m_animation = clear_brackets(_GetItem(str, 7, tmp));
}

bool SArtefactActivation::IsInProgress() { return m_in_process; }
