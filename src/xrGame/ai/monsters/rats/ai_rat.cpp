////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat.cpp
//	Created 	: 23.04.2002
//  Modified 	: 07.11.2002
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai/monsters/rats/ai_rat.h"
#include "ai/ai_monsters_misc.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "ai/monsters/rats/ai_rat_space.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "movement_manager.h"
#include "location_manager.h"
#include "xrServerEntities/ai_sounds.h"
#include "sound_player.h"
#include "ai/monsters/rats/ai_rat_impl.h"
#include "ai_space.h"
#include "rat_state_manager.h"
#include "rat_states.h"
#include "Common/object_broker.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "ai/monsters/ai_monster_squad.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "Actor.h"

using namespace RatSpace;

CAI_Rat::CAI_Rat() : m_behaviour_manager(0)
{
    init();
    //	m_behaviour_manager				= new steering_behaviour::manager(this);
    //	m_behaviour_manager->add		(new steering_behaviour::cohesion(this),	.5f);
    //	m_behaviour_manager->add		(new steering_behaviour::separation(this),	.5f);
    //	m_behaviour_manager->add		(new steering_behaviour::alignment(this),	.5f);
}

CAI_Rat::~CAI_Rat() { delete_data(m_state_manager); }
void CAI_Rat::init()
{
    m_tAction = eRatActionNone;
    m_hit_direction.set(0, 0, 1);
    m_hit_time = 0;
    m_tpCurrentGlobalAnimation.invalidate();
    m_tpCurrentGlobalBlend = 0;
    m_bActionStarted = false;
    m_bFiring = false;
    m_previous_query_time = 0;
    m_tGoalDir.set(10.0f * (Random.randF() - Random.randF()), 10.0f * (Random.randF() - Random.randF()),
        10.0f * (Random.randF() - Random.randF()));
    m_tCurrentDir = m_tGoalDir;
    m_tHPB.set(0, 0, 0);
    m_fDHeading = 0;
    m_fGoalChangeTime = 0.f;
    m_tLastSound.tpEntity = 0;
    m_tLastSound.dwTime = 0;
    m_tLastSound.eSoundType = SOUND_TYPE_NO_SOUND;
    m_bNoWay = false;
    m_dwMoraleLastUpdateTime = 0;
    m_bStanding = false;
    m_bActive = false;
    m_dwStartAttackTime = 0;
    m_saved_impulse = 0.f;
    m_bMoving = false;
    m_bCanAdjustSpeed = false;
    m_bStraightForward = false;
    m_turning = false;
    time_to_next_attack = 2000;
    time_old_attack = 0;
    m_squad_count = u32(-1);
    m_current_way_point = u32(-1);
}

void CAI_Rat::init_state_manager()
{
    m_state_manager = new rat_state_manager();
    m_state_manager->construct(this);
    fire(false);

    m_state_manager->add_state(aiRatDeath, new rat_state_death());
    m_state_manager->add_state(aiRatFreeActive, new rat_state_free_active());
    m_state_manager->add_state(aiRatFreePassive, new rat_state_free_passive());
    m_state_manager->add_state(aiRatAttackRange, new rat_state_attack_range());
    m_state_manager->add_state(aiRatAttackMelee, new rat_state_attack_melee());
    m_state_manager->add_state(aiRatUnderFire, new rat_state_under_fire());
    m_state_manager->add_state(aiRatRetreat, new rat_state_retreat());
    m_state_manager->add_state(aiRatPursuit, new rat_state_pursuit());
    m_state_manager->add_state(aiRatFreeRecoil, new rat_state_free_recoil());
    m_state_manager->add_state(aiRatReturnHome, new rat_state_return_home());
    m_state_manager->add_state(aiRatEatCorpse, new rat_state_eat_corpse());
    m_state_manager->add_state(aiRatNoWay, new rat_state_no_way());

    m_state_manager->push_state(aiRatFreeActive);
}

void CAI_Rat::reinit()
{
    inherited::reinit();
    CEatableItem::reinit();
    init();
    init_state_manager();
}

void CAI_Rat::reload(LPCSTR section)
{
    inherited::reload(section);
    CEatableItem::reload(section);
    LPCSTR head_bone_name = pSettings->r_string(section, "bone_head");
    sound().add(pSettings->r_string(section, "sound_death"), 100, SOUND_TYPE_MONSTER_DYING, 0, u32(eRatSoundMaskDie),
        eRatSoundDie, head_bone_name);
    sound().add(pSettings->r_string(section, "sound_hit"), 100, SOUND_TYPE_MONSTER_INJURING, 1,
        u32(eRatSoundMaskInjuring), eRatSoundInjuring, head_bone_name);
    sound().add(pSettings->r_string(section, "sound_attack"), 100, SOUND_TYPE_MONSTER_ATTACKING, 2,
        u32(eRatSoundMaskAttack), eRatSoundAttack, head_bone_name);
    sound().add(pSettings->r_string(section, "sound_voice"), 100, SOUND_TYPE_MONSTER_TALKING, 4,
        u32(eRatSoundMaskVoice), eRatSoundVoice, head_bone_name);
    sound().add(pSettings->r_string(section, "sound_eat"), 100, SOUND_TYPE_MONSTER_EATING, 3, u32(eRatSoundMaskEat),
        eRatSoundEat, head_bone_name);
}

void CAI_Rat::Die(IGameObject* who)
{
    inherited::Die(who);

    m_eCurrentState = aiRatDeath;

    m_flags.set(FCanTake, TRUE);

    SelectAnimation(XFORM().k, movement().detail().direction(), movement().speed());

    sound().play(eRatSoundDie);

    update_morale_broadcast(m_fMoraleDeathQuant, m_fMoraleDeathDistance);

    CGroupHierarchyHolder& Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
    vfRemoveActiveMember();
    vfRemoveStandingMember();
    --(Group.m_dwAliveCount);
    m_eCurrentState = aiRatDeath;
}

void CAI_Rat::Load(LPCSTR section)
{
    init();

    inherited::Load(section);
    CEatableItem::Load(section);

    // initialize start position
    Fvector P = Position();
    P.x += ::Random.randF();
    P.z += ::Random.randF();

    // active\passive
    m_fChangeActiveStateProbability = pSettings->r_float(section, "ChangeActiveStateProbability");
    m_dwPassiveScheduleMin = pSettings->r_s32(section, "PassiveScheduleMin");
    m_dwPassiveScheduleMax = pSettings->r_s32(section, "PassiveScheduleMax");
    m_dwActiveCountPercent = pSettings->r_s32(section, "ActiveCountPercent");
    m_dwStandingCountPercent = pSettings->r_s32(section, "StandingCountPercent");

    // eye shift
    m_tEyeShift.y = pSettings->r_float(section, "EyeYShift");

    // former constants
    m_dwLostMemoryTime = pSettings->r_s32(section, "LostMemoryTime");
    m_dwLostRecoilTime = pSettings->r_s32(section, "LostRecoilTime");
    m_fUnderFireDistance = pSettings->r_float(section, "UnderFireDistance");
    m_dwRetreatTime = pSettings->r_s32(section, "RetreatTime");
    m_fRetreatDistance = pSettings->r_float(section, "RetreatDistance");
    m_fAttackStraightDistance = pSettings->r_float(section, "AttackStraightDistance");
    m_fStableDistance = pSettings->r_float(section, "StableDistance");
    m_fWallMinTurnValue = pSettings->r_float(section, "WallMinTurnValue") / 180.f * PI;
    m_fWallMaxTurnValue = pSettings->r_float(section, "WallMaxTurnValue") / 180.f * PI;

    m_fAngleSpeed = pSettings->r_float(section, "AngleSpeed");
    m_fSafeGoalChangeDelta = pSettings->r_float(section, "GoalChangeDelta");
    m_tGoalVariation = pSettings->r_fvector3(section, "GoalVariation");

    m_fMoraleDeathDistance = pSettings->r_float(section, "MoraleDeathDistance");
    m_dwActionRefreshRate = pSettings->r_s32(section, "ActionRefreshRate");

    SetMaxHealth(pSettings->r_float(section, "MaxHealthValue"));
    m_fSoundThreshold = pSettings->r_float(section, "SoundThreshold");

    m_bEatMemberCorpses = pSettings->r_bool(section, "EatMemberCorpses");
    m_bCannibalism = pSettings->r_bool(section, "Cannibalism");
    m_dwEatCorpseInterval = pSettings->r_s32(section, "EatCorpseInterval");

    m_fNullASpeed = pSettings->r_float(section, "AngularStandSpeed") / 180.f * PI; // PI_MUL_2
    m_fMinASpeed = pSettings->r_float(section, "AngularMinSpeed") / 180.f * PI; // PI_MUL_2
    m_fMaxASpeed = pSettings->r_float(section, "AngularMaxSpeed") / 180.f * PI; //.2f
    m_fAttackASpeed = pSettings->r_float(section, "AngularAttackSpeed") / 180.f * PI; //.15f;

    m_phMass = pSettings->r_float(section, "corp_mass");
    m_dwActiveScheduleMin = shedule.t_min;
    m_dwActiveScheduleMax = shedule.t_max;
}

BOOL CAI_Rat::net_Spawn(CSE_Abstract* DC)
{
    //////////////////////////////////////////////////////////////////////////
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeMonsterRat* tpSE_Rat = smart_cast<CSE_ALifeMonsterRat*>(e);

    // model
    if (!inherited::net_Spawn(DC))
        return (FALSE);
    // model
    if (!CEatableItem::net_Spawn(DC))
        return (FALSE);

    monster_squad().register_member((u8)g_Team(), (u8)g_Squad(), (u8)g_Group(), this);

    // personal characteristics
    movement().m_body.current.yaw = movement().m_body.target.yaw = -tpSE_Rat->o_torso.yaw;
    movement().m_body.current.pitch = movement().m_body.target.pitch = 0;
    movement().m_body.speed = PI_MUL_2;

    eye_fov = tpSE_Rat->fEyeFov;
    eye_range = tpSE_Rat->fEyeRange;
    SetfHealth(tpSE_Rat->get_health());
    m_fMinSpeed = tpSE_Rat->fMinSpeed;
    m_fMaxSpeed = tpSE_Rat->fMaxSpeed;
    m_fAttackSpeed = tpSE_Rat->fAttackSpeed;
    m_fMaxPursuitRadius = tpSE_Rat->fMaxPursuitRadius;
    m_fMaxHomeRadius = tpSE_Rat->fMaxHomeRadius;
    // morale
    m_fMoraleSuccessAttackQuant = tpSE_Rat->fMoraleSuccessAttackQuant;
    m_fMoraleDeathQuant = tpSE_Rat->fMoraleDeathQuant;
    m_fMoraleFearQuant = tpSE_Rat->fMoraleFearQuant;
    m_fMoraleRestoreQuant = tpSE_Rat->fMoraleRestoreQuant;
    m_dwMoraleRestoreTimeInterval = tpSE_Rat->u16MoraleRestoreTimeInterval;
    m_fMoraleMinValue = tpSE_Rat->fMoraleMinValue;
    m_fMoraleMaxValue = tpSE_Rat->fMoraleMaxValue;
    m_fMoraleNormalValue = tpSE_Rat->fMoraleNormalValue;
    // attack
    m_fHitPower = tpSE_Rat->fHitPower;
    m_dwHitInterval = tpSE_Rat->u16HitInterval;
    m_fAttackDistance = tpSE_Rat->fAttackDistance;
    m_fAttackAngle = tpSE_Rat->fAttackAngle / 180.f * PI;
    m_fAttackSuccessProbability = tpSE_Rat->fAttackSuccessProbability;

    //	m_tCurGP						= tpSE_Rat->m_tGraphID;
    //	m_tNextGP						= tpSE_Rat->m_tNextGraphID;
    m_current_graph_point = m_next_graph_point = ai_location().game_vertex_id();

    int iPointCount = (int)movement().locations().vertex_types().size();
    for (int j = 0; j < iPointCount; ++j)
        if (ai().game_graph().mask(movement().locations().vertex_types()[j].tMask,
                ai().game_graph().vertex(ai_location().game_vertex_id())->vertex_type()))
        {
            m_time_to_change_graph_point = Device.dwTimeGlobal + ::Random32.random(60000) + 60000;
            break;
        }

    //////////////////////////////////////////////////////////////////////////

    m_fSpeed = m_fCurSpeed = m_fMaxSpeed;

    if (g_Alive())
        m_tSpawnPosition.set(Level().seniority_holder().team(g_Team()).squad(g_Squad()).leader()->Position());
    else
        m_tSpawnPosition.set(Position());

    m_home_position.set(m_tSpawnPosition);
    m_tStateStack.push(m_eCurrentState = aiRatFreeActive);
    if (g_Alive())
        add_active_member(true);

    m_bStateChanged = true;
    ai_location().game_vertex(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());

    m_tHPB.x = -movement().m_body.current.yaw;
    m_tHPB.y = -movement().m_body.current.pitch;
    m_tHPB.z = 0;

    movement().enable_movement(false);

    load_animations();

    if (g_Alive())
        Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group()).m_dwLastActionTime = 0;

    m_flags.set(FCanTake, FALSE);

    CInifile* m_spawn_ini = spawn_ini();

    if (!m_spawn_ini || !m_spawn_ini->section_exist("patrol") || !m_spawn_ini->line_exist("patrol", "way"))
    {
        m_walk_on_way = false;
    }
    else
    {
        m_walk_on_way = true;
    }

    if (m_walk_on_way)
    {
        m_current_way_point = 0;
        m_path = ai().patrol_paths().path(m_spawn_ini->r_string("patrol", "way"));
    }

    return (TRUE);
}

void CAI_Rat::net_Destroy()
{
    inherited::net_Destroy();
    CEatableItem::net_Destroy();
    monster_squad().remove_member((u8)g_Team(), (u8)g_Squad(), (u8)g_Group(), this);
}

void CAI_Rat::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());

    // export last known packet
    R_ASSERT(!NET.empty());
    net_update& N = NET.back();
    P.w_float(GetfHealth());
    P.w_u32(N.dwTimeStamp);
    P.w_u8(0);
    P.w_vec3(N.p_pos);
    P.w_float(N.o_model);
    P.w_float(N.o_torso.yaw);
    P.w_float(N.o_torso.pitch);
    P.w_float(N.o_torso.roll);
    P.w_u8(u8(g_Team()));
    P.w_u8(u8(g_Squad()));
    P.w_u8(u8(g_Group()));

    GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    float f1 = 0;
    if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
    {
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
    }
    else
    {
        P.w(&f1, sizeof(f1));
        P.w(&f1, sizeof(f1));
    }

    CEatableItem::net_Export(P);
}

void CAI_Rat::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());
    net_update N;

    u8 flags;

    float health;
    P.r_float(health);
    SetfHealth(health);

    P.r_u32(N.dwTimeStamp);
    P.r_u8(flags);
    P.r_vec3(N.p_pos);
    P.r_angle8(N.o_model);
    P.r_angle8(N.o_torso.yaw);
    P.r_angle8(N.o_torso.pitch);
    P.r_angle8(N.o_torso.roll);
    id_Team = P.r_u8();
    id_Squad = P.r_u8();
    id_Group = P.r_u8();

    GameGraph::_GRAPH_ID t;
    P.r(&t, sizeof(t));
    P.r(&t, sizeof(t));
    ai_location().game_vertex(t);

    if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp))
    {
        NET.push_back(N);
        NET_WasInterpolating = TRUE;
    }

    setVisible(TRUE);
    setEnabled(TRUE);

    CEatableItem::net_Import(P);
}

void CAI_Rat::CreateSkeleton()
{
    if (!Visual())
        return;
    CPhysicsElement* element = P_create_Element();
    Fobb box;
    box.m_rotate.identity();
    box.m_translate.set(0, 0.1f, -0.15f);
    box.m_halfsize.set(0.10f, 0.085f, 0.25f);
    element->add_Box(box);
    // Fsphere sphere;
    // sphere.P.set(0,0,0);
    // sphere.R=0.25;
    // element->add_Sphere(sphere);
    element->setDensity(m_phMass);
    element->SetMaterial(smart_cast<IKinematics*>(
        Visual())->LL_GetData(smart_cast<IKinematics*>(Visual())->LL_GetBoneRoot())
                             .game_mtl_idx);
    m_pPhysicsShell = P_create_Shell();
    m_pPhysicsShell->add_Element(element);
    m_pPhysicsShell->Activate(XFORM(), 0, XFORM());
    m_pPhysicsShell->set_PhysicsRefObject(this);
    if (!fsimilar(0.f, m_saved_impulse))
    {
        m_pPhysicsShell->applyHit(m_saved_hit_position, m_saved_hit_dir, m_saved_impulse, 0, m_saved_hit_type);
    }
    /*
    IKinematics* M		= smart_cast<IKinematics*>(Visual());			VERIFY(M);
    m_pPhysicsShell		= P_create_Shell();

    //get bone instance
    int id=M->LL_BoneID("bip01_pelvis");
    CBoneInstance& instance=M->LL_GetBoneInstance				(id);

    //create root element
    CPhysicsElement* element=P_create_Element				();
    element->mXFORM.identity();
    instance.set_callback(m_pPhysicsShell->GetBonesCallback(),element);
    Fobb box;
    box.m_rotate.identity();
    box.m_translate.set(0,0,0);
    box.m_halfsize.set(0.10f,0.085f,0.25f);
    element->add_Box(box);

    element->setDensity(200.f);
    m_pPhysicsShell->add_Element(element);
    element->SetMaterial("materials/skel1");

    //set shell start position
    Fmatrix m;
    m.set(mRotate);
    m.c.set(Position());
    m_pPhysicsShell->mXFORM.set(m);
    */
}

void CAI_Rat::shedule_Update(u32 dt)
{
    if (!monster_squad().get_squad(this)->GetLeader() || !monster_squad().get_squad(this)->GetLeader()->g_Alive())
    {
        monster_squad().get_squad(this)->SetLeader(this);
    }
    inherited::shedule_Update(dt);
}

void CAI_Rat::UpdateCL()
{
///////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
    if (monster_squad().get_squad(this)->GetLeader() == this)
    {
        if (m_walk_on_way && m_path)
            draw_way();
    }
#endif
    ///////////////////////////////////////////////////////////////////////////////////////

    if (!m_pPhysicsShell && !g_Alive())
        CreateSkeleton();

    if (!Useful())
    {
        inherited::UpdateCL();
        Exec_Look(Device.fTimeDelta);

        CMonsterSquad* squad = monster_squad().get_squad(this);

        if (squad &&
            ((squad->GetLeader() != this && !squad->GetLeader()->g_Alive()) || squad->get_index(this) == u8(-1)))
            squad->SetLeader(this);

        if (squad && squad->SquadActive() && squad->GetLeader() == this && m_squad_count != squad->squad_alife_count())
        {
            squad->set_rat_squad_index(squad->GetLeader());
            m_squad_count = squad->squad_alife_count();
        }
    }
    else
    {
        if (!H_Parent() && m_pPhysicsShell && m_pPhysicsShell->isActive())
            m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());

        CPhysicsShellHolder::UpdateCL();
        CEatableItem::UpdateCL();
    }
}

void CAI_Rat::UpdatePositionAnimation()
{
    Fmatrix l_tSavedTransform = XFORM();
    m_fTimeUpdateDelta = Device.fTimeDelta;
    move(m_bCanAdjustSpeed, m_bStraightForward);
    float y, p, b;
    XFORM().getHPB(y, p, b);
    NET_Last.p_pos = Position();
    NET_Last.o_model = -y;
    NET_Last.o_torso.yaw = -y;
    NET_Last.o_torso.pitch = -p;
    XFORM() = l_tSavedTransform;

    if (!bfScriptAnimation())
        SelectAnimation(XFORM().k, Fvector().set(1, 0, 0), m_fSpeed);
}

// void CAI_Rat::Hit(float P,Fvector &dir,IGameObject*who,s16 element,Fvector p_in_object_space,float impulse,
// ALife::EHitType hit_type /*= ALife::eHitTypeWound*/)
void CAI_Rat::Hit(SHit* pHDS)
{
    //	inherited::Hit				(P,dir,who,element,p_in_object_space,impulse, hit_type);
    inherited::Hit(pHDS);
    if (!m_pPhysicsShell)
    {
        m_saved_impulse = pHDS->impulse;
        m_saved_hit_dir.set(pHDS->dir);
        m_saved_hit_type = pHDS->hit_type;
        m_saved_hit_position.set(pHDS->p_in_bone_space);
    }
    else
    {
        //		CEatableItem::Hit		(P,dir,who,element,p_in_object_space,impulse, hit_type);
        CEatableItem::Hit(pHDS);
    }
}

void CAI_Rat::feel_touch_new(IGameObject* O) {}
/////////////////////////////////////
// Rat as eatable item
/////////////////////////////////////
void CAI_Rat::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    CEatableItem::OnH_A_Chield();
}

void CAI_Rat::OnH_B_Chield()
{
    setVisible(FALSE);
    setEnabled(FALSE);

    if (m_pPhysicsShell)
        m_pPhysicsShell->Deactivate();

    CEatableItem::OnH_B_Chield();
}

void CAI_Rat::OnH_B_Independent()
{
    CEatableItem::OnH_B_Independent(TRUE);
    inherited::OnH_B_Independent(TRUE);

    if (!Useful())
        return;

    setVisible(TRUE);
    setEnabled(TRUE);

    if (m_pPhysicsShell)
        activate_physic_shell();
}

void CAI_Rat::OnH_A_Independent()
{
    CEatableItem::OnH_A_Independent();
    inherited::OnH_A_Independent();
}

bool CAI_Rat::Useful() const
{
    if (!g_Alive())
    {
        return CEatableItem::Useful();
    }

    return false;
}

#ifdef DEBUG
void CAI_Rat::OnRender()
{
    //	inherited::OnRender();
    //	CEatableItem::OnRender();
}
#endif

BOOL CAI_Rat::UsedAI_Locations() { return (TRUE); }
void CAI_Rat::make_Interpolation()
{
    inherited::make_Interpolation();
    CEatableItem::make_Interpolation();
}

void CAI_Rat::PH_B_CrPr()
{
    inherited::PH_B_CrPr();
    CEatableItem::PH_B_CrPr();
}

void CAI_Rat::PH_I_CrPr()
{
    inherited::PH_I_CrPr();
    CEatableItem::PH_I_CrPr();
}

#ifdef DEBUG
void CAI_Rat::PH_Ch_CrPr()
{
    inherited::PH_Ch_CrPr();
    CEatableItem::PH_Ch_CrPr();
}
#endif

void CAI_Rat::PH_A_CrPr()
{
    inherited::PH_A_CrPr();
    CEatableItem::PH_A_CrPr();
}

void CAI_Rat::create_physic_shell()
{
    // do not delete!!!
}

void CAI_Rat::setup_physic_shell()
{
    // do not delete!!!
}

void CAI_Rat::activate_physic_shell() { CEatableItem::activate_physic_shell(); }
void CAI_Rat::on_activate_physic_shell()
{
    IGameObject* object = smart_cast<IGameObject*>(H_Parent());
    R_ASSERT(object);
    XFORM().set(object->XFORM());
    inherited::activate_physic_shell();
}

float CAI_Rat::get_custom_pitch_speed(float def_speed)
{
    if (fsimilar(m_fSpeed, 0.f))
        return (PI_DIV_6);
    else if (fsimilar(m_fSpeed, m_fMinSpeed))
        return (PI_DIV_4);
    else if (fsimilar(m_fSpeed, m_fMaxSpeed))
        return (PI_DIV_3);
    else if (fsimilar(m_fSpeed, m_fAttackSpeed))
        return (PI_DIV_2);

    xrDebug::Fatal(DEBUG_INFO, "Impossible RAT speed!");
    return (PI_DIV_2);
}

BOOL CAI_Rat::renderable_ShadowReceive() { return TRUE; }
BOOL CAI_Rat::renderable_ShadowGenerate() { return FALSE; }
IFactoryObject* CAI_Rat::_construct()
{
    CCustomMonster::_construct();
    CEatableItem::_construct();
    return (this);
}
