#include "pch_script.h"
#include "entity_alive.h"
#include "inventoryowner.h"
#include "inventory.h"
#include "xrPhysics/physicsshell.h"
#include "xrEngine/GameMtlLib.h"
#include "phmovementcontrol.h"
#include "wound.h"
#include "xrmessages.h"
#include "Level.h"
#include "Include/xrRender/Kinematics.h"
#include "relation_registry.h"
#include "monster_community.h"
#include "entitycondition.h"
#include "script_game_object.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"
#include "script_callback_ex.h"
#include "game_object_space.h"
#include "material_manager.h"
#include "game_base_space.h"

#define SMALL_ENTITY_RADIUS 0.6f
#define BLOOD_MARKS_SECT "bloody_marks"

//отметки крови на стенах
FactoryPtr<IWallMarkArray>* CEntityAlive::m_pBloodMarksVector = NULL;
float CEntityAlive::m_fBloodMarkSizeMin = 0.f;
float CEntityAlive::m_fBloodMarkSizeMax = 0.f;
float CEntityAlive::m_fBloodMarkDistance = 0.f;
float CEntityAlive::m_fNominalHit = 0.f;

//капание крови
FactoryPtr<IWallMarkArray>* CEntityAlive::m_pBloodDropsVector = NULL;
float CEntityAlive::m_fStartBloodWoundSize = 0.3f;
float CEntityAlive::m_fStopBloodWoundSize = 0.1f;
float CEntityAlive::m_fBloodDropSize = 0.03f;

//минимальный размер ожега, после которого горят партиклы
//минимальное время горения
u32 CEntityAlive::m_dwMinBurnTime = 10000;
//размер раны, чтоб запустить партиклы
float CEntityAlive::m_fStartBurnWoundSize = 0.3f;
//размер раны, чтоб остановить партиклы
float CEntityAlive::m_fStopBurnWoundSize = 0.1f;

STR_VECTOR* CEntityAlive::m_pFireParticlesVector = nullptr;

/////////////////////////////////////////////
// CEntityAlive
/////////////////////////////////////////////
CEntityAlive::CEntityAlive()
    : m_bMobility(false), m_fAccuracy(0), m_fIntelligence(0),
      m_entity_condition(nullptr), m_ef_creature_type(0),
      m_hit_bone_surface_areas_actual(false)
{
    monster_community = new MONSTER_COMMUNITY();

    m_ef_weapon_type = u32(-1);
    m_ef_detector_type = u32(-1);
    b_eating = false;
    m_is_agresive = false;
    m_is_start_attack = false;
    m_use_timeout = 5000;
    m_used_time = Device.dwTimeGlobal;
    m_squad_index = u8(-1);

    m_material_manager = 0;
}

CEntityAlive::~CEntityAlive()
{
    xr_delete(monster_community);
    xr_delete(m_material_manager);
}

void CEntityAlive::Load(LPCSTR section)
{
    CEntity::Load(section);
    conditions().LoadCondition(section);
    conditions().LoadImmunities(pSettings->r_string(section, "immunities_sect"), pSettings);

    m_fFood = 100 * pSettings->r_float(section, "ph_mass");

    // bloody wallmarks
    if (0 == m_pBloodMarksVector)
        LoadBloodyWallmarks(BLOOD_MARKS_SECT);

    if (0 == m_pFireParticlesVector)
        LoadFireParticles("entity_fire_particles");

    //биолог. вид к торому принадлежит монстр или персонаж
    monster_community->set(pSettings->r_string(section, "species"));
}

void CEntityAlive::LoadBloodyWallmarks(LPCSTR section)
{
    VERIFY(0 == m_pBloodMarksVector);
    VERIFY(0 == m_pBloodDropsVector);
    m_pBloodMarksVector = new FactoryPtr<IWallMarkArray>();
    m_pBloodDropsVector = new FactoryPtr<IWallMarkArray>();

    //кровавые отметки на стенах
    string256 tmp;
    LPCSTR wallmarks_name = pSettings->r_string(section, "wallmarks");

    int cnt = _GetItemCount(wallmarks_name);

    for (int k = 0; k < cnt; ++k)
        (*m_pBloodMarksVector)->AppendMark(_GetItem(wallmarks_name, k, tmp));

    m_fBloodMarkSizeMin = pSettings->r_float(section, "min_size");
    m_fBloodMarkSizeMax = pSettings->r_float(section, "max_size");
    m_fBloodMarkDistance = pSettings->r_float(section, "dist");
    m_fNominalHit = pSettings->r_float(section, "nominal_hit");

    //капли крови с открытых ран
    wallmarks_name = pSettings->r_string(section, "blood_drops");
    cnt = _GetItemCount(wallmarks_name);

    for (int k = 0; k < cnt; ++k)
        (*m_pBloodDropsVector)->AppendMark(_GetItem(wallmarks_name, k, tmp));

    /*
    for (int k=0; k<cnt; ++k)
    {
        s.create ("effects\\wallmark",_GetItem(wallmarks_name,k,tmp));
        m_pBloodDropsVector->push_back	(s);
    }
    */

    m_fStartBloodWoundSize = pSettings->r_float(section, "start_blood_size");
    m_fStopBloodWoundSize = pSettings->r_float(section, "stop_blood_size");
    m_fBloodDropSize = pSettings->r_float(section, "blood_drop_size");
}

void CEntityAlive::UnloadBloodyWallmarks()
{
    if (m_pBloodMarksVector)
    {
        //		m_pBloodMarksVector->clear	();
        xr_delete(m_pBloodMarksVector);
    }
    if (m_pBloodDropsVector)
    {
        //		m_pBloodDropsVector->clear	();
        xr_delete(m_pBloodDropsVector);
    }
}

void CEntityAlive::LoadFireParticles(LPCSTR section)
{
    m_pFireParticlesVector = new STR_VECTOR();

    string256 tmp;
    LPCSTR particles_name = pSettings->r_string(section, "fire_particles");

    int cnt = _GetItemCount(particles_name);

    shared_str s;
    for (int k = 0; k < cnt; ++k)
    {
        s = _GetItem(particles_name, k, tmp);
        m_pFireParticlesVector->push_back(s);
    }

    m_fStartBurnWoundSize = pSettings->r_float(section, "start_burn_size");
    m_fStopBurnWoundSize = pSettings->r_float(section, "stop_burn_size");

    m_dwMinBurnTime = pSettings->r_u32(section, "min_burn_time");
}

void CEntityAlive::UnloadFireParticles()
{
    if (m_pFireParticlesVector)
    {
        m_pFireParticlesVector->clear();
        xr_delete(m_pFireParticlesVector);
    }
}

void CEntityAlive::reinit()
{
    CEntity::reinit();

    m_fAccuracy = 25.f;
    m_fIntelligence = 25.f;
}

void CEntityAlive::reload(LPCSTR section)
{
    CEntity::reload(section);
    //	CEntityCondition::reload(section);

    m_ef_creature_type = pSettings->r_u32(section, "ef_creature_type");
    m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
    m_ef_detector_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_detector_type", u32(-1));

    m_fFood = 100 * pSettings->r_float(section, "ph_mass");
}

void CEntityAlive::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    // condition update with the game time pass
    conditions().UpdateConditionTime();
    conditions().UpdateCondition();
    //Обновление партиклов огня
    UpdateFireParticles();
    //капли крови
    UpdateBloodDrops();
    //обновить раны
    conditions().UpdateWounds();

    //убить сущность
    if (Local() && !g_Alive() && !AlreadyDie())
    {
        if (conditions().GetWhoHitLastTime())
        {
            //			Msg			("%6d : KillEntity from CEntityAlive (using who hit last time) for object
            //%s",Device.dwTimeGlobal,*cName());
            KillEntity(conditions().GetWhoHitLastTimeID());
        }
        else
        {
            //			Msg			("%6d : KillEntity from CEntityAlive for object %s",Device.dwTimeGlobal,*cName());
            KillEntity(ID());
        }
    }
}

BOOL CEntityAlive::net_Spawn(CSE_Abstract* DC)
{
    //установить команду в соответствии с community
    /*	if(monster_community->team() != 255)
            id_Team = monster_community->team();*/

    conditions().reinit();
    inherited::net_Spawn(DC);

    m_BloodWounds.clear();
    m_ParticleWounds.clear();

    //добавить кровь и огонь на партиклы, если нужно
    for (WOUND_VECTOR::const_iterator it = conditions().wounds().begin(); conditions().wounds().end() != it; ++it)
    {
        CWound* pWound = *it;
        StartFireParticles(pWound);
        StartBloodDrops(pWound);
    }

    return (TRUE);
}

void CEntityAlive::net_Destroy() { inherited::net_Destroy(); }
void CEntityAlive::HitImpulse(float /**amount/**/, Fvector& /**vWorldDir/**/, Fvector& /**vLocalDir/**/)
{
    //	float Q					= 2*float(amount)/m_PhysicMovementControl->GetMass();
    //	m_PhysicMovementControl->vExternalImpulse.mad	(vWorldDir,Q);
}

void CEntityAlive::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    //-------------------------------------------------------------------
    if (HDS.hit_type == ALife::eHitTypeWound_2)
        HDS.hit_type = ALife::eHitTypeWound;
    //-------------------------------------------------------------------
    CDamageManager::HitScale(
        HDS.boneID, conditions().hit_bone_scale(), conditions().wound_bone_scale(), pHDS->aim_bullet);

    //изменить состояние, перед тем как родительский класс обработает хит
    CWound* pWound = conditions().ConditionHit(&HDS);

    if (pWound)
    {
        if (ALife::eHitTypeBurn == HDS.hit_type || ALife::eHitTypeLightBurn == HDS.hit_type)
            StartFireParticles(pWound);
        else if (ALife::eHitTypeWound == HDS.hit_type || ALife::eHitTypeFireWound == HDS.hit_type)
            StartBloodDrops(pWound);
    }

    if (HDS.hit_type != ALife::eHitTypeTelepatic)
    {
        //добавить кровь на стены
        if (!use_simplified_visual())
            BloodyWallmarks(HDS.damage(), HDS.dir, HDS.bone(), HDS.p_in_bone_space);
    }

    //-------------------------------------------
    conditions().SetConditionDeltaTime(0);
    //-------------------------------------------
    inherited::Hit(&HDS);

    if (g_Alive() && IsGameTypeSingle())
    {
        CEntityAlive* EA = smart_cast<CEntityAlive*>(HDS.who);
        if (EA && EA->g_Alive() && EA->ID() != ID())
        {
            RELATION_REGISTRY().FightRegister(EA->ID(), ID(), this->tfGetRelationType(EA), HDS.damage());
            RELATION_REGISTRY().Action(EA, this, RELATION_REGISTRY::ATTACK);
        }
    }
}

void CEntityAlive::Die(IGameObject* who)
{
    if (IsGameTypeSingle())
        RELATION_REGISTRY().Action(smart_cast<CEntityAlive*>(who), this, RELATION_REGISTRY::KILL);
    inherited::Die(who);

    const CGameObject* who_object = smart_cast<const CGameObject*>(who);
    callback(GameObject::eDeath)(lua_game_object(), who_object ? who_object->lua_game_object() : 0);

    if (!getDestroy() && (GameID() == eGameIDSingle))
    {
        NET_Packet P;
        u_EventGen(P, GE_ASSIGN_KILLER, ID());
        P.w_u16(u16(who->ID()));
        u_EventSend(P);
    }

    // disable react to sound
    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
        self->GetSpatialData().type &= ~STYPE_REACTTOSOUND;
    if (character_physics_support())
        character_physics_support()->in_Die();
}

//вывзывает при подсчете хита
float CEntityAlive::CalcCondition(float /**hit/**/)
{
    conditions().UpdateCondition();

    // dont call inherited::CalcCondition it will be meaningless
    return conditions().GetHealthLost(); //*100.f;
}

///////////////////////////////////////////////////////////////////////
u16 CEntityAlive::PHGetSyncItemsNumber() { return character_physics_support()->PHGetSyncItemsNumber(); }
CPHSynchronize* CEntityAlive::PHGetSyncItem(u16 item) { return character_physics_support()->PHGetSyncItem(item); }
void CEntityAlive::PHUnFreeze()
{
    if (character_physics_support()->movement()->CharacterExist())
        character_physics_support()->movement()->UnFreeze();
    else if (m_pPhysicsShell)
        m_pPhysicsShell->UnFreeze();
}
void CEntityAlive::PHFreeze()
{
    if (character_physics_support()->movement()->CharacterExist())
        character_physics_support()->movement()->Freeze();
    else if (m_pPhysicsShell)
        m_pPhysicsShell->Freeze();
}
//////////////////////////////////////////////////////////////////////

//добавление кровавых отметок на стенах, после получения хита
void CEntityAlive::BloodyWallmarks(float P, const Fvector& dir, s16 element, const Fvector& position_in_object_space)
{
    if (BI_NONE == (u16)element)
        return;

    //вычислить координаты попадания
    IKinematics* V = smart_cast<IKinematics*>(Visual());

    Fvector start_pos = position_in_object_space;
    if (V)
    {
        Fmatrix& m_bone = (V->LL_GetBoneInstance(u16(element))).mTransform;
        m_bone.transform_tiny(start_pos);
    }
    XFORM().transform_tiny(start_pos);

    float small_entity = 1.f;
    if (Radius() < SMALL_ENTITY_RADIUS)
        small_entity = 0.5;

    float wallmark_size = m_fBloodMarkSizeMax;
    wallmark_size *= (P / m_fNominalHit);
    wallmark_size *= small_entity;
    clamp(wallmark_size, m_fBloodMarkSizeMin, m_fBloodMarkSizeMax);

    VERIFY(m_pBloodMarksVector);
    PlaceBloodWallmark(dir, start_pos, m_fBloodMarkDistance, wallmark_size, &**m_pBloodMarksVector);
}

void CEntityAlive::PlaceBloodWallmark(const Fvector& dir, const Fvector& start_pos, float trace_dist,
    float wallmark_size, IWallMarkArray* pwallmarks_vector)
{
    collide::rq_result result;
    BOOL reach_wall =
        Level().ObjectSpace.RayPick(start_pos, dir, trace_dist, collide::rqtBoth, result, this) && !result.O;

    //если кровь долетела до статического объекта
    if (reach_wall)
    {
        CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + result.element;
        SGameMtl* pMaterial = GMLib.GetMaterialByIdx(pTri->material);

        if (pMaterial->Flags.is(SGameMtl::flBloodmark))
        {
            //вычислить нормаль к пораженной поверхности
            Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();

            //вычислить точку попадания
            Fvector end_point;
            end_point.set(0, 0, 0);
            end_point.mad(start_pos, dir, result.range);

            // ref_shader wallmarkShader = wallmarks_vector[::Random.randI(wallmarks_vector.size())];
            VERIFY(!pwallmarks_vector->empty());
            {
                //добавить отметку на материале
                // GlobalEnv.Render->add_StaticWallmark(wallmarkShader, end_point, wallmark_size, pTri, pVerts);
                GEnv.Render->add_StaticWallmark(pwallmarks_vector, end_point, wallmark_size, pTri, pVerts);
            }
        }
    }
}

void CEntityAlive::StartFireParticles(CWound* pWound)
{
    if (pWound->TypeSize(ALife::eHitTypeBurn) > m_fStartBurnWoundSize)
    {
        if (std::find(m_ParticleWounds.begin(), m_ParticleWounds.end(), pWound) == m_ParticleWounds.end())
        {
            m_ParticleWounds.push_back(pWound);
        }

        IKinematics* V = smart_cast<IKinematics*>(Visual());

        u16 particle_bone = CParticlesPlayer::GetNearestBone(V, pWound->GetBoneNum());
        VERIFY(particle_bone < 64 || BI_NONE == particle_bone);

        pWound->SetParticleBoneNum(particle_bone);
        pWound->SetParticleName((*m_pFireParticlesVector)[::Random.randI(0, m_pFireParticlesVector->size())]);

        if (BI_NONE != particle_bone)
        {
            CParticlesPlayer::StartParticles(pWound->GetParticleName(), pWound->GetParticleBoneNum(),
                Fvector().set(0, 1, 0), ID(), u32(float(m_dwMinBurnTime) * ::Random.randF(0.5f, 1.5f)), false);
        }
        else
        {
            CParticlesPlayer::StartParticles(pWound->GetParticleName(), Fvector().set(0, 1, 0), ID(),
                u32(float(m_dwMinBurnTime) * ::Random.randF(0.5f, 1.5f)), false);
        }
    }
}

void CEntityAlive::UpdateFireParticles()
{
    if (m_ParticleWounds.empty())
        return;

    //	WOUND_VECTOR_IT last_it;

    for (auto it = m_ParticleWounds.begin(); it != m_ParticleWounds.end();)
    {
        CWound* pWound = *it;
        float burn_size = pWound->TypeSize(ALife::eHitTypeBurn);

        if (pWound->GetDestroy() || (burn_size > 0 && (burn_size < m_fStopBurnWoundSize || !g_Alive())))
        {
            CParticlesPlayer::AutoStopParticles(pWound->GetParticleName(), pWound->GetParticleBoneNum(),
                u32(float(m_dwMinBurnTime) * ::Random.randF(0.5f, 1.5f)));
            it = m_ParticleWounds.erase(it);
            continue;
        }
        it++;
    }
}

ALife::ERelationType CEntityAlive::tfGetRelationType(const CEntityAlive* tpEntityAlive) const
{
    int relation =
        MONSTER_COMMUNITY::relation(this->monster_community->index(), tpEntityAlive->monster_community->index());

    switch (relation)
    {
    case 1: return (ALife::eRelationTypeFriend); break;
    case 0: return (ALife::eRelationTypeNeutral); break;
    case -1: return (ALife::eRelationTypeEnemy); break;
    case -2: return (ALife::eRelationTypeWorstEnemy); break;

    default: return (ALife::eRelationTypeDummy); break;
    }
};

bool CEntityAlive::is_relation_enemy(const CEntityAlive* tpEntityAlive) const
{
    return ((tfGetRelationType(tpEntityAlive) == ALife::eRelationTypeEnemy) ||
        (tfGetRelationType(tpEntityAlive) == ALife::eRelationTypeWorstEnemy));
}

void CEntityAlive::StartBloodDrops(CWound* pWound)
{
    if (pWound->BloodSize() > m_fStartBloodWoundSize)
    {
        if (std::find(m_BloodWounds.begin(), m_BloodWounds.end(), pWound) == m_BloodWounds.end())
        {
            m_BloodWounds.push_back(pWound);
            pWound->m_fDropTime = 0.f;
        }
    }
}

void CEntityAlive::UpdateBloodDrops()
{
    static float m_fBloodDropTimeMax = pSettings->r_float(BLOOD_MARKS_SECT, "blood_drop_time_max");
    static float m_fBloodDropTimeMin = pSettings->r_float(BLOOD_MARKS_SECT, "blood_drop_time_min");

    if (m_BloodWounds.empty())
        return;

    if (!g_Alive())
    {
        m_BloodWounds.clear();
        return;
    }

    //	WOUND_VECTOR_IT last_it;

    for (auto it = m_BloodWounds.begin(); it != m_BloodWounds.end();)
    {
        CWound* pWound = *it;
        float blood_size = pWound->BloodSize();

        if (pWound->GetDestroy() || blood_size < m_fStopBloodWoundSize)
        {
            it = m_BloodWounds.erase(it);
            continue;
        }

        if (pWound->m_fDropTime < Device.fTimeGlobal)
        {
            float size_k = blood_size - m_fStopBloodWoundSize;
            size_k = size_k < 1.f ? size_k : 1.f;
            pWound->m_fDropTime = Device.fTimeGlobal +
                (m_fBloodDropTimeMax - (m_fBloodDropTimeMax - m_fBloodDropTimeMin) * size_k) * Random.randF(0.8f, 1.2f);
            VERIFY(m_pBloodDropsVector);
            if (pWound->GetBoneNum() != BI_NONE)
            {
                Fvector pos;
                Fvector pos_distort;
                pos_distort.random_dir();
                pos_distort.mul(0.15f);
                CParticlesPlayer::GetBonePos(this, pWound->GetBoneNum(), Fvector().set(0, 0, 0), pos);
                pos.add(pos_distort);
                PlaceBloodWallmark(
                    Fvector().set(0.f, -1.f, 0.f), pos, m_fBloodMarkDistance, m_fBloodDropSize, &**m_pBloodDropsVector);
            }
        }
        it++;
    }
}

void CEntityAlive::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    conditions().save(output_packet);
}

void CEntityAlive::load(IReader& input_packet)
{
    inherited::load(input_packet);
    conditions().load(input_packet);
}

BOOL CEntityAlive::net_SaveRelevant() { return (TRUE); }
CEntityConditionSimple* CEntityAlive::create_entity_condition(CEntityConditionSimple* ec)
{
    if (!ec)
        m_entity_condition = new CEntityCondition(this);
    else
        m_entity_condition = smart_cast<CEntityCondition*>(ec);

    return (inherited::create_entity_condition(m_entity_condition));
}

/*
float CEntityAlive::GetfHealth	() const
{
    return conditions().health()*100.f;
}

float CEntityAlive::SetfHealth	(float value)
{
    conditions().health() = value/100.f;
    return value;
}
*/
float CEntityAlive::SetfRadiation(float value)
{
    conditions().radiation() = value / 100.f;
    return value;
}
/*
float CEntityAlive::g_Health	() const
{
    return conditions().GetHealth()*100.f;
}
float CEntityAlive::g_MaxHealth	() const
{
    return conditions().GetMaxHealth()*100.f;
}
*/
float CEntityAlive::g_Radiation() const { return conditions().GetRadiation() * 100.f; }
IFactoryObject* CEntityAlive::_construct()
{
    inherited::_construct();
    if (character_physics_support())
        m_material_manager = new CMaterialManager(this, character_physics_support()->movement());
    return (this);
}

u32 CEntityAlive::ef_creature_type() const { return (m_ef_creature_type); }
u32 CEntityAlive::ef_weapon_type() const
{
    VERIFY(m_ef_weapon_type != u32(-1));
    return (m_ef_weapon_type);
}

u32 CEntityAlive::ef_detector_type() const
{
    VERIFY(m_ef_detector_type != u32(-1));
    return (m_ef_detector_type);
}
void CEntityAlive::PHGetLinearVell(Fvector& velocity)
{
    if (character_physics_support())
    {
        character_physics_support()->PHGetLinearVell(velocity);
    }
    else
        inherited::PHGetLinearVell(velocity);
}

void CEntityAlive::set_lock_corpse(bool b_l_corpse)
{
    if (b_eating && !b_l_corpse)
    {
        m_used_time = Device.dwTimeGlobal;
    }
    b_eating = b_l_corpse;
}

bool CEntityAlive::is_locked_corpse()
{
    if (!b_eating)
    {
        if (m_used_time + m_use_timeout > Device.dwTimeGlobal)
        {
            return true;
        }
    }
    return b_eating;
}

CIKLimbsController* CEntityAlive::character_ik_controller()
{
    if (character_physics_support())
    {
        return character_physics_support()->ik_controller();
    }
    else
    {
        return NULL;
    }
}
CPHSoundPlayer* CEntityAlive::ph_sound_player()
{
    if (character_physics_support())
    {
        return character_physics_support()->ph_sound_player();
    }
    else
    {
        return NULL;
    }
}

ICollisionHitCallback* CEntityAlive::get_collision_hit_callback()
{
    CCharacterPhysicsSupport* cs = character_physics_support();
    if (cs)
        return cs->get_collision_hit_callback();
    else
        return false;
}

void CEntityAlive::set_collision_hit_callback(ICollisionHitCallback* cc)
{
    CCharacterPhysicsSupport* cs = character_physics_support();
    if (cs)
        cs->set_collision_hit_callback(cc);
}

void CEntityAlive::net_Relcase(IGameObject* object)
{
    inherited::net_Relcase(object);
    conditions().remove_links(object);
}

Fvector CEntityAlive::predict_position(const float& time_to_check) const { return (Position()); }
Fvector CEntityAlive::target_position() const { return (Position()); }
void CEntityAlive::create_anim_mov_ctrl(CBlend* b, Fmatrix* start_pose, bool local_animation)
{
    bool b_animation_movement_controlled = animation_movement_controlled();
    inherited::create_anim_mov_ctrl(b, start_pose, local_animation);
    CCharacterPhysicsSupport* cs = character_physics_support();
    if (!b_animation_movement_controlled && cs)
        cs->on_create_anim_mov_ctrl();
}

void CEntityAlive::destroy_anim_mov_ctrl()
{
    inherited::destroy_anim_mov_ctrl();
    CCharacterPhysicsSupport* cs = character_physics_support();
    if (cs)
        cs->on_destroy_anim_mov_ctrl();
}

#include "xrEngine/xr_collide_form.h"

struct element_predicate
{
    inline bool operator()(CCF_Skeleton::SElement const& element, u16 element_id) const
    {
        return element.elem_id < element_id;
    }
}; // struct element_predicate

struct sort_surface_area_predicate
{
    inline bool operator()(std::pair<u16, float> const& left, std::pair<u16, float> const& right) const
    {
        return left.second > right.second;
    }
}; // struct sort_surface_area_predicate

void CEntityAlive::OnChangeVisual()
{
    inherited::OnChangeVisual();

    m_hit_bone_surface_areas_actual = false;
}

void CEntityAlive::fill_hit_bone_surface_areas() const
{
    VERIFY(!m_hit_bone_surface_areas_actual);
    m_hit_bone_surface_areas_actual = true;

    IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
    VERIFY(kinematics);
    VERIFY(kinematics->LL_BoneCount());

    m_hit_bone_surface_areas.clear();

    for (u16 i = 0, n = kinematics->LL_BoneCount(); i < n; ++i)
    {
        SBoneShape const& shape = kinematics->LL_GetData(i).shape;
        if (SBoneShape::stNone == shape.type)
            continue;

        if (shape.flags.is(SBoneShape::sfNoPickable))
            continue;

        float surface_area = flt_max;
        switch (shape.type)
        {
        case SBoneShape::stBox:
        {
            Fvector const& half_size = shape.box.m_halfsize;
            surface_area = 2.f * (half_size.x * (half_size.y + half_size.z) + half_size.y * half_size.z);
            break;
        }
        case SBoneShape::stSphere:
        {
            surface_area = 4.f * PI * _sqr(shape.sphere.R);
            break;
        }
        case SBoneShape::stCylinder:
        {
            surface_area = 2.f * PI * shape.cylinder.m_radius * (shape.cylinder.m_radius + shape.cylinder.m_height);
            break;
        }
        default: NODEFAULT;
        }

        m_hit_bone_surface_areas.push_back(std::make_pair(i, surface_area));
    }

    std::sort(m_hit_bone_surface_areas.begin(), m_hit_bone_surface_areas.end(), sort_surface_area_predicate());
}

BOOL g_ai_use_old_vision = 0;

Fvector CEntityAlive::get_new_local_point_on_mesh(u16& bone_id) const
{
    if (g_ai_use_old_vision)
        return inherited::get_new_local_point_on_mesh(bone_id);

    IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
    if (!kinematics)
        return inherited::get_new_local_point_on_mesh(bone_id);

    if (!kinematics->LL_BoneCount())
        return inherited::get_new_local_point_on_mesh(bone_id);

    if (!m_hit_bone_surface_areas_actual)
        fill_hit_bone_surface_areas();

    if (m_hit_bone_surface_areas.empty())
        return inherited::get_new_local_point_on_mesh(bone_id);

    float hit_bones_surface_area = 0.f;
    hit_bone_surface_areas_type::const_iterator i = m_hit_bone_surface_areas.begin();
    hit_bone_surface_areas_type::const_iterator const e = m_hit_bone_surface_areas.end();
    for (; i != e; ++i)
    {
        if (!kinematics->LL_GetBoneVisible((*i).first))
            continue;

        SBoneShape const& shape = kinematics->LL_GetData((*i).first).shape;
        VERIFY(shape.type != SBoneShape::stNone);
        VERIFY(!shape.flags.is(SBoneShape::sfNoPickable));

        hit_bones_surface_area += (*i).second;
    }

    VERIFY2(hit_bones_surface_area > 0.f, make_string("m_hit_bone_surface_areas[%d]", m_hit_bone_surface_areas.size()));
    float const selected_area = m_hit_bones_random.randF(hit_bones_surface_area);

    i = m_hit_bone_surface_areas.begin();
    for (float accumulator = 0.f; i != e; ++i)
    {
        if (!kinematics->LL_GetBoneVisible((*i).first))
            continue;

        SBoneShape const& shape = kinematics->LL_GetData((*i).first).shape;
        VERIFY(shape.type != SBoneShape::stNone);
        VERIFY(!shape.flags.is(SBoneShape::sfNoPickable));

        accumulator += (*i).second;
        if (accumulator >= selected_area)
            break;
    }

    VERIFY2(i != e, make_string("m_hit_bone_surface_areas[%d]", m_hit_bone_surface_areas.size()));
    SBoneShape const& shape = kinematics->LL_GetData((*i).first).shape;
    bone_id = (*i).first;
    Fvector result = Fvector().set(flt_max, flt_max, flt_max);
    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        Fmatrix transform;
        shape.box.xform_full(transform);

        Fvector direction;
        u32 random_value = ::Random.randI(6);
        Fvector random = {(random_value & 1) ? -1.f : 1.f, ::Random.randF(2.f) - 1.f, ::Random.randF(2.f) - 1.f};
        random.normalize();
        if (random_value < 2)
            direction = Fvector().set(random.x, random.y, random.z);
        else if (random_value < 4)
            direction = Fvector().set(random.z, random.x, random.y);
        else
            direction = Fvector().set(random.y, random.z, random.x);

        transform.transform_tiny(result, direction);
        break;
    }
    case SBoneShape::stSphere:
    {
        result.random_dir().mul(shape.sphere.R).add(shape.sphere.P);
        break;
    }
    case SBoneShape::stCylinder:
    {
        float const total_square = (shape.cylinder.m_height + shape.cylinder.m_radius); // *2*PI*c_cylinder.m_radius
        float const random_value = ::Random.randF(total_square);
        float const angle = ::Random.randF(2.f * PI);

        float const x = shape.cylinder.m_direction.x;
        float const y = shape.cylinder.m_direction.y;
        float const z = shape.cylinder.m_direction.z;
        Fvector normal = Fvector().set(y - z, z - x, x - y);

        Fmatrix rotation = Fmatrix().rotation(shape.cylinder.m_direction, normal);
        Fmatrix rotation_y = Fmatrix().rotateY(angle);
        Fmatrix const transform = Fmatrix().mul_43(rotation, rotation_y);
        transform.transform_dir(normal, Fvector().set(0.f, 0.f, 1.f));

        float height, radius;
        if (random_value < shape.cylinder.m_height)
        {
            height = random_value - shape.cylinder.m_height / 2.f;
            radius = shape.cylinder.m_radius;
        }
        else
        {
            float const normalized_value = random_value - shape.cylinder.m_height;
            height = shape.cylinder.m_height / 2.f * ((normalized_value < shape.cylinder.m_radius / 2.f) ? -1.f : 1.f);
            radius = height > 0.f ? normalized_value - shape.cylinder.m_radius / 2.f : normalized_value;
        }

        normal.mul(radius);
        result.mul(shape.cylinder.m_direction, height);
        result.add(normal);
        result.add(shape.cylinder.m_center);
        break;
    }
    default: NODEFAULT;
    }

    return result;
}

Fvector CEntityAlive::get_last_local_point_on_mesh(Fvector const& last_point, u16 bone_id) const
{
    if (bone_id == u16(-1))
        return inherited::get_last_local_point_on_mesh(last_point, bone_id);

    IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
    VERIFY(kinematics);

    Fmatrix transform;
    kinematics->Bone_GetAnimPos(transform, bone_id, u8(-1), false);

    Fvector result;
    transform.transform_tiny(result, last_point);

    XFORM().transform_tiny(result, Fvector(result));
    return result;
}
