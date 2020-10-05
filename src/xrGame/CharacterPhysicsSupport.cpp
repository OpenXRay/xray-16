#include "StdAfx.h"

#include "CharacterPhysicsSupport.h"
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "PHMovementControl.h"
#include "CustomMonster.h"

#include "Include/xrRender/KinematicsAnimated.h"

#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/IActivationShape.h"

#include "xrPhysics/Geometry.h"

#include "xrPhysics/IPHCapture.h"

#include "xrPhysics/IPHWorld.h"

#include "IKLimbsController.h"
#include "Actor.h"
#include "ai/stalker/ai_stalker.h"
#include "imotion_position.h"
#include "imotion_velocity.h"
#include "animation_movement_controller.h"
#include "xrServer_Object_Base.h"
#include "interactive_animation.h"
#include "stalker_animation_manager.h"
#include "InventoryOwner.h"
#include "Inventory.h"
#include "ActivatingCharCollisionDelay.h"
#include "stalker_movement_manager_smart_cover.h"

// const float default_hinge_friction = 5.f;//gray_wolf comment
#ifdef DEBUG
#include "PHDebug.h"
extern BOOL death_anim_debug;
#endif // DEBUG

#include "xrEngine/device.h"

#define USE_SMART_HITS
#define USE_IK

// void  NodynamicsCollide( bool& do_colide, bool bo1, dContact& c, SGameMtl * /*material_1*/, SGameMtl * /*material_2*/
// )
//{
//	dBodyID body1=dGeomGetBody( c.geom.g1 );
//	dBodyID body2=dGeomGetBody( c.geom.g2 );
//	if( !body1 || !body2 || ( dGeomUserDataHasCallback( c.geom.g1,NodynamicsCollide )&& dGeomUserDataHasCallback(
// c.geom.g2, NodynamicsCollide ) ) )
//		return;
//	do_colide = false;
//}

IC bool is_imotion(interactive_motion* im) { return im && im->is_enabled(); }
CCharacterPhysicsSupport::~CCharacterPhysicsSupport()
{
    set_collision_hit_callback(0);
    if (m_flags.test(fl_skeleton_in_shell))
    {
        if (m_physics_skeleton)
            m_physics_skeleton->Deactivate();
        xr_delete(m_physics_skeleton); //! b_skeleton_in_shell
    }
    xr_delete(m_PhysicMovementControl);
    xr_delete(m_collision_activating_delay);
    VERIFY(!m_interactive_motion);
    xr_delete(m_collision_activating_delay);
    bone_fix_clear();
}

CCharacterPhysicsSupport::CCharacterPhysicsSupport(EType atype, CEntityAlive* aentity)
    : m_pPhysicsShell(aentity->PPhysicsShell()), m_EntityAlife(*aentity), mXFORM(aentity->XFORM()),
      m_ph_sound_player(aentity), m_interactive_motion(0), m_PhysicMovementControl(xr_new<CPHMovementControl>(aentity)),
      m_eType(atype), m_eState(esAlive), m_physics_skeleton(NULL), m_ik_controller(NULL), m_BonceDamageFactor(1.f),
      m_collision_hit_callback(NULL), m_interactive_animation(NULL), m_physics_shell_animated(NULL),
      m_physics_shell_animated_time_destroy(u32(-1)), m_weapon_attach_bone(0), m_active_item_obj(0),
      m_hit_valide_time(u32(-1)), m_collision_activating_delay(NULL)
{
    m_flags.assign(0);
    m_flags.set(fl_death_anim_on, FALSE);
    m_flags.set(fl_skeleton_in_shell, FALSE);
    m_flags.set(fl_use_hit_anims, TRUE);
    m_pPhysicsShell = NULL;
    switch (atype)
    {
    case etActor:
        m_PhysicMovementControl->AllocateCharacterObject(CPHMovementControl::actor);
        m_PhysicMovementControl->SetRestrictionType(rtActor);
        break;
    case etStalker:
        m_PhysicMovementControl->AllocateCharacterObject(CPHMovementControl::ai);
        m_PhysicMovementControl->SetRestrictionType(rtStalker);
        m_PhysicMovementControl->SetActorMovable(false);
        break;
    case etBitting:
        m_PhysicMovementControl->AllocateCharacterObject(CPHMovementControl::ai);

        m_PhysicMovementControl->SetRestrictionType(rtMonsterMedium);
        // m_PhysicMovementControl->SetActorMovable(false);
    }
};

void CCharacterPhysicsSupport::SetRemoved()
{
    m_eState = esRemoved;
    if (m_flags.test(fl_skeleton_in_shell))
    {
        if (!m_pPhysicsShell)
            return;

        if (m_pPhysicsShell->isEnabled())
            m_EntityAlife.processing_deactivate();

        m_pPhysicsShell->Deactivate();
        xr_delete(m_pPhysicsShell);
    }
    else
    {
        if (m_physics_skeleton)
            m_physics_skeleton->Deactivate();
        xr_delete(m_physics_skeleton);
        m_EntityAlife.processing_deactivate();
    }
}

void CCharacterPhysicsSupport::in_Load(LPCSTR section)
{
    m_character_shell_control.Load(section);
    m_flags.set(fl_specific_bonce_demager, TRUE);
    if (pSettings->line_exist(section, "bonce_damage_factor"))
        m_BonceDamageFactor = pSettings->r_float(section, "bonce_damage_factor_for_objects");
    else
        m_BonceDamageFactor = pSettings->r_float("collision_damage", "bonce_damage_factor_for_objects");
    CPHDestroyable::Load(section);
}

void CCharacterPhysicsSupport::run_interactive(CBlend* B)
{
    VERIFY(!m_interactive_animation);
    m_interactive_animation = xr_new<interactive_animation>(&m_EntityAlife, B);
}

void CCharacterPhysicsSupport::update_interactive_anims()
{
    if (Type() != etStalker)
        return;
    VERIFY(m_EntityAlife.cast_stalker());
    CAI_Stalker* stalker = m_EntityAlife.cast_stalker();
    CBlend* b = stalker->animation().global().blend();
    if (b && !m_interactive_animation && stalker->animation().global().callback_on_collision())
        run_interactive(b);
    if (m_interactive_animation && !m_interactive_animation->update(m_EntityAlife.XFORM()))
        xr_delete(m_interactive_animation);
}

void CCharacterPhysicsSupport::in_NetSpawn(CSE_Abstract* e)
{
    m_sv_hit = SHit();
    if (m_EntityAlife.use_simplified_visual())
    {
        m_flags.set(fl_death_anim_on, TRUE);
        IKinematics* ka = smart_cast<IKinematics*>(m_EntityAlife.Visual());
        VERIFY(ka);
        ka->CalculateBones_Invalidate();
        ka->CalculateBones(TRUE);
        CollisionCorrectObjPos(m_EntityAlife.Position());
        m_pPhysicsShell = P_build_Shell(&m_EntityAlife, false);
        ka->CalculateBones_Invalidate();
        ka->CalculateBones(TRUE);
        return;
    }
    CPHDestroyable::Init(); // this zerows colbacks !!;
    IRenderVisual* pVisual = m_EntityAlife.Visual();
    IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(pVisual);
    IKinematics* pK = smart_cast<IKinematics*>(pVisual);
    VERIFY(&e->spawn_ini());
    m_death_anims.setup(ka, *e->s_name, pSettings);
    if (!m_EntityAlife.g_Alive())
    {
        if (m_eType == etStalker)
        {
            // pK->LL_GetData( 0 ).shape.flags.set(SBoneShape::sfVisibilityIgnore,TRUE);
            // pK->LL_GetData( pK->LL_BoneID("bip01") ).shape.flags.set(SBoneShape::sfVisibilityIgnore,TRUE);
            ka->PlayCycle("waunded_1_idle_0");
        }
        else
            ka->PlayCycle("death_init");
    }
    else if (!m_EntityAlife.animation_movement_controlled())
        ka->PlayCycle("death_init"); ///непонятно зачем это вообще надо запускать
    ///этот хак нужен, потому что некоторым монстрам
    ///анимация после спона, может быть вообще не назначена
    pK->CalculateBones_Invalidate();
    pK->CalculateBones(TRUE);

    CPHSkeleton::Spawn(e);
    movement()->EnableCharacter();
    movement()->SetPosition(m_EntityAlife.Position());
    movement()->SetVelocity(0, 0, 0);
    if (m_eType != etActor)
    {
        m_flags.set(fl_specific_bonce_demager, TRUE);
        m_BonceDamageFactor = 1.f;
    }
    if (Type() == etStalker)
    {
        m_hit_animations.SetupHitMotions(*smart_cast<IKinematicsAnimated*>(m_EntityAlife.Visual()));
    }
    anim_mov_state.init();

    anim_mov_state.active = m_EntityAlife.animation_movement_controlled();
    CInifile* ini = m_EntityAlife.spawn_ini();
    if (ini && ini->section_exist("physics") && ini->line_exist("physics", "controller_can_be_moved_by_player"))
        m_PhysicMovementControl->SetActorMovable(!!ini->r_bool("physics", "controller_can_be_moved_by_player"));
}

bool CCharacterPhysicsSupport::CollisionCorrectObjPos()
{
    return CollisionCorrectObjPos(m_EntityAlife.Position(), true);
}

void CCharacterPhysicsSupport::CreateCharacterSafe()
{
    if (m_PhysicMovementControl->CharacterExist())
        return;
    CollisionCorrectObjPos(m_EntityAlife.Position(), true);
    CreateCharacter();
}

void CCharacterPhysicsSupport::CreateCharacter()
{
    m_PhysicMovementControl->CreateCharacter();
    m_PhysicMovementControl->SetPhysicsRefObject(&m_EntityAlife);
    m_PhysicMovementControl->SetPosition(m_EntityAlife.Position());
}

bool HACK_TERRIBLE_DONOT_COLLIDE_ON_SPAWN(CEntityAlive& ea)
{
    if (pSettings->line_exist(ea.cNameSect().c_str(), "hack_terrible_donot_collide_on_spawn") &&
        pSettings->r_bool(ea.cNameSect().c_str(), "hack_terrible_donot_collide_on_spawn"))
        return true;
    return false;
}

void CCharacterPhysicsSupport::SpawnInitPhysics(CSE_Abstract* e)
{
    if (m_EntityAlife.g_Alive())
    {
#ifdef DEBUG
        if (ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject) &&
            xr_stricmp(PH_DBG_ObjectTrackName(), *m_EntityAlife.cName()) == 0)
        {
            Msg("CCharacterPhysicsSupport::SpawnInitPhysics obj %s before collision correction %f,%f,%f",
                PH_DBG_ObjectTrackName(), m_EntityAlife.Position().x, m_EntityAlife.Position().y,
                m_EntityAlife.Position().z);
        }
#endif
#ifdef USE_IK
        if (etStalker == m_eType || etActor == m_eType ||
            (m_EntityAlife.Visual()->dcast_PKinematics()->LL_UserData() &&
                m_EntityAlife.Visual()->dcast_PKinematics()->LL_UserData()->section_exist("ik")))
            CreateIKController();
#endif
        VERIFY(pSettings);

        SpawnCharacterCreate();

#ifdef DEBUG
        if (ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject) &&
            xr_stricmp(PH_DBG_ObjectTrackName(), *m_EntityAlife.cName()) == 0)
        {
            Msg("CCharacterPhysicsSupport::SpawnInitPhysics obj %s after collision correction %f,%f,%f",
                PH_DBG_ObjectTrackName(), m_EntityAlife.Position().x, m_EntityAlife.Position().y,
                m_EntityAlife.Position().z);
        }
#endif
    }
    else
    {
        ActivateShell(NULL);
    }

    CSE_PHSkeleton* po = smart_cast<CSE_PHSkeleton*>(e);
    VERIFY(po);

    PHNETSTATE_VECTOR& saved_bones = po->saved_bones.bones;
    if (po->_flags.test(CSE_PHSkeleton::flSavedData) && saved_bones.size() != m_EntityAlife.PHGetSyncItemsNumber())
    {
#ifdef DEBUG
        Msg("! saved bones %d , current bones %d, object :%s", saved_bones.size(), m_EntityAlife.PHGetSyncItemsNumber(),
            m_EntityAlife.cName().c_str());
#endif
        po->_flags.set(CSE_PHSkeleton::flSavedData, FALSE);
        saved_bones.clear();
    }
}
void CCharacterPhysicsSupport::SpawnCharacterCreate()
{
    if (HACK_TERRIBLE_DONOT_COLLIDE_ON_SPAWN(m_EntityAlife)) //||  m_EntityAlife.animation_movement_controlled( )
        return;
    CreateCharacterSafe();
    // if( m_eType != etStalker )
    //	CreateCharacterSafe();
    // VERIFY( movement() );

    // if( movement()->CharacterExist() )
    //	return;
    // else
    //{
    //	VERIFY( !m_collision_activating_delay );
    //	m_collision_activating_delay = new activating_character_delay(this);
    //}
}
void CCharacterPhysicsSupport::destroy_imotion() { destroy(m_interactive_motion); }
void CCharacterPhysicsSupport::in_NetDestroy()
{
    destroy(m_interactive_motion);
    m_PhysicMovementControl->DestroyCharacter();

    if (m_physics_skeleton)
    {
        m_physics_skeleton->Deactivate();
        xr_delete(m_physics_skeleton);
    }
    if (m_pPhysicsShell)
    {
        m_pPhysicsShell->Deactivate();
        xr_delete(m_pPhysicsShell);
    }

    m_flags.set(fl_skeleton_in_shell, FALSE);
    CPHSkeleton::RespawnInit();
    CPHDestroyable::RespawnInit();
    m_eState = esAlive;

    xr_delete(m_interactive_animation);
    destroy_animation_collision();
    DestroyIKController();
    xr_delete(m_collision_activating_delay);
}

void CCharacterPhysicsSupport::in_NetSave(NET_Packet& P) { CPHSkeleton::SaveNetState(P); }
void CCharacterPhysicsSupport::in_Init() {}
void CCharacterPhysicsSupport::UpdateCollisionActivatingDellay()
{
    VERIFY(m_collision_activating_delay);
    VERIFY(movement());
    m_collision_activating_delay->update();
    if (!m_collision_activating_delay->active())
        xr_delete(m_collision_activating_delay);
}

void CCharacterPhysicsSupport::in_shedule_Update(u32 DT)
{
    /// VERIFY( 0 );

    // CPHSkeleton::Update(DT);
    if (m_collision_activating_delay)
        UpdateCollisionActivatingDellay();

    if (!m_EntityAlife.use_simplified_visual())
        CPHDestroyable::SheduleUpdate(DT);
    else if (m_pPhysicsShell && m_pPhysicsShell->isFullActive() && !m_pPhysicsShell->isEnabled())
        m_EntityAlife.deactivate_physics_shell();
    movement()->in_shedule_Update(DT);
#if 0
	if( anim_mov_state.active )
	{
		DBG_OpenCashedDraw( );
		DBG_DrawMatrix( mXFORM, 0.5f );
		DBG_ClosedCashedDraw( 5000 );
	}
#endif
}

#ifdef DEBUG
string64 sdbg_stalker_death_anim = "none";
pstr dbg_stalker_death_anim = sdbg_stalker_death_anim;
#endif
BOOL b_death_anim_velocity = TRUE;
const float cmp_angle = M_PI / 10.f;
const float cmp_ldisp = 0.1f;
IC bool cmp(const Fmatrix& f0, const Fmatrix& f1)
{
    Fmatrix if0;
    if0.invert(f0);
    Fmatrix cm;
    cm.mul_43(if0, f1);

    Fvector ax;
    float ang;
    Fquaternion q;
    q.set(cm);
    q.get_axis_angle(ax, ang);

    return ang < cmp_angle && cm.c.square_magnitude() < cmp_ldisp * cmp_ldisp;
}

bool is_similar(const Fmatrix& m0, const Fmatrix& m1, float param)
{
    Fmatrix tmp1;
    tmp1.invert(m0);
    Fmatrix tmp2;
    tmp2.mul(tmp1, m1);
    Fvector ax;
    float ang;
    Fquaternion q;
    q.set(tmp2);
    q.get_axis_angle(ax, ang);
    return _abs(ang) < M_PI / 2.f;
}

// static struct callback_tracks_disable: public IUpdateTracksCallback
//{
//	virtual	bool	operator () ( float dt, IKinematicsAnimated& k ){return false;}
//} tracks_disable_update;

void CCharacterPhysicsSupport::KillHit(SHit& H)
{
#ifdef DEBUG
    if (death_anim_debug)
        Msg("death anim: kill hit  ");
#endif
    VERIFY(m_EntityAlife.Visual());
    VERIFY(m_EntityAlife.Visual()->dcast_PKinematics());

    // IKinematicsAnimated * KA = m_EntityAlife.Visual( )->dcast_PKinematicsAnimated	();
    // VERIFY( KA );
    // KA->SetUpdateTracksCalback( &tracks_disable_update );

    m_character_shell_control.TestForWounded(m_EntityAlife.XFORM(), m_EntityAlife.Visual()->dcast_PKinematics());
    Fmatrix prev_pose;
    prev_pose.set(mXFORM);

    Fvector start;
    start.set(m_EntityAlife.Position());
    Fvector velocity;
    Fvector death_position;

    CreateShell(H.who, death_position, velocity);
    // ActivateShell( H.who );

    //	if(Type() == etStalker && xr_strcmp(dbg_stalker_death_anim, "none") != 0)
    float hit_angle = 0;
    MotionID m = m_death_anims.motion(m_EntityAlife, H, hit_angle);

    CAI_Stalker* const holder = m_EntityAlife.cast_stalker();
    if (holder && (holder->wounded() || holder->movement().current_params().cover()))
        m = MotionID();

    if (m.valid()) //&& cmp( prev_pose, mXFORM )
    {
        destroy(m_interactive_motion);
        if (false && b_death_anim_velocity)
            m_interactive_motion = xr_new<imotion_velocity>();
        else
            m_interactive_motion = xr_new<imotion_position>();
        m_interactive_motion->setup(m, m_pPhysicsShell, hit_angle);
    }
    else
        DestroyIKController();
    // KA->SetUpdateTracksCalback( 0 );

    if (is_imotion(m_interactive_motion))
        m_interactive_motion->play();

    m_character_shell_control.set_fatal_impulse(H);

    if (!is_imotion(m_interactive_motion))
    {
#ifdef DEBUG
        if (death_anim_debug)
        {
            Msg("death anim: kill hit use free ragdoll ");
            Msg("death anim: fatal impulse: %f, ", H.impulse);
        }
#endif

        EndActivateFreeShell(H.who, start, death_position, velocity);
        m_flags.set(fl_block_hit, TRUE);
    }
}
const u32 hit_valide_time = 1000;
void CCharacterPhysicsSupport::in_Hit(SHit& H, bool is_killing)
{
    m_sv_hit = H;
    m_hit_valide_time = Device.dwTimeGlobal + hit_valide_time;
    if (m_EntityAlife.use_simplified_visual() || esRemoved == m_eState)
        return;
    if (m_flags.test(fl_block_hit))
    {
        VERIFY2(!m_EntityAlife.g_Alive(),
            make_string("entity [%s][%d] is dead", m_EntityAlife.Name(), m_EntityAlife.ID()).c_str());
        if (Device.dwTimeGlobal - m_EntityAlife.GetLevelDeathTime() >= 2000)
            m_flags.set(fl_block_hit, FALSE);
        else
            return;
    }

    // is_killing = is_killing || ( m_eState==esAlive && !m_EntityAlife.g_Alive( ) );
    if (m_EntityAlife.g_Alive() && is_killing && H.type() == ALife::eHitTypeExplosion && H.damage() > 70.f)
        CPHDestroyable::Destroy();

    if ((!m_EntityAlife.g_Alive() || is_killing))
        m_character_shell_control.set_kill_hit(H);

    if (!m_pPhysicsShell && is_killing)
        KillHit(H);

    if (m_flags.test(fl_use_hit_anims) && Type() != etBitting &&
        !m_flags.test(fl_death_anim_on)) //&& Type() == etStalker
    {
        m_hit_animations.PlayHitMotion(H.direction(), H.bone_space_position(), H.bone(), m_EntityAlife);
    }

    if (!(m_pPhysicsShell && m_pPhysicsShell->isActive()))
    {
        if (!is_killing && m_EntityAlife.g_Alive())
            m_PhysicMovementControl->ApplyHit(H.direction(), H.phys_impulse(), H.type());
    }
    else
    {
#ifdef DEBUG
        if (is_killing && death_anim_debug && !is_imotion(m_interactive_motion))
        {
            Msg("death anim: applied fatal impulse dir: (%f,%f,%f), value: (%f) ", H.dir.x, H.dir.y, H.dir.z,
                H.impulse);
        }
#endif
        m_pPhysicsShell->applyHit(H.bone_space_position(), H.direction(), H.phys_impulse(), H.bone(), H.type());
    }
}

IC void CCharacterPhysicsSupport::UpdateDeathAnims()
{
    VERIFY(m_pPhysicsShell->isFullActive());

    if (!m_flags.test(fl_death_anim_on) &&
        !is_imotion(
            m_interactive_motion)) //! m_flags.test(fl_use_death_motion)//!b_death_anim_on&&m_pPhysicsShell->isFullActive()
    {
        DestroyIKController();
        smart_cast<IKinematicsAnimated*>(m_EntityAlife.Visual())->PlayCycle("death_init");
        m_flags.set(fl_death_anim_on, TRUE);
    }
}
#ifdef DEBUG
void DBG_PhysBones(IGameObject& O);
void DBG_DrawBones(IGameObject& O);
void DBG_DrawBind(IGameObject& O);
BOOL dbg_draw_character_bones = false;
BOOL dbg_draw_character_physics = false;
BOOL dbg_draw_character_binds = false;
BOOL dbg_draw_character_physics_pones = false;

void dbg_draw_geoms(xr_vector<CODEGeom*>& m_weapon_geoms)
{
    xr_vector<CODEGeom *>::iterator ii = m_weapon_geoms.begin(), ee = m_weapon_geoms.end();
    for (; ii != ee; ++ii)
    {
        CODEGeom* g = (*ii);

        g->dbg_draw(0.01f, color_xrgb(0, 255, 100), Flags32());
    }
}
#endif

void CCharacterPhysicsSupport::in_UpdateCL()
{
    if (m_eState == esRemoved)
    {
        return;
    }
#ifdef DEBUG
    if (dbg_draw_character_bones)
        dbg_draw_geoms(m_weapon_geoms);

    if (dbg_draw_character_bones)
        DBG_DrawBones(m_EntityAlife);

    if (dbg_draw_character_binds)
        DBG_DrawBind(m_EntityAlife);

    if (dbg_draw_character_physics_pones)
        DBG_PhysBones(m_EntityAlife);

    if (dbg_draw_character_physics && m_pPhysicsShell)
        m_pPhysicsShell->dbg_draw_geometry(0.2f, color_argb(100, 255, 0, 0));
#endif
    update_animation_collision();
    m_character_shell_control.CalculateTimeDelta();
    if (m_pPhysicsShell)
    {
        VERIFY(m_pPhysicsShell->isFullActive());
        m_pPhysicsShell->SetRagDoll(); //Теперь шела относиться к классу объектов cbClassRagDoll

        if (!is_imotion(m_interactive_motion)) //! m_flags.test(fl_use_death_motion)
            m_pPhysicsShell->InterpolateGlobalTransform(&mXFORM);
        else
            m_interactive_motion->update();

        UpdateDeathAnims();

        m_character_shell_control.UpdateFrictionAndJointResistanse(m_pPhysicsShell);
    }
    // else if ( !m_EntityAlife.g_Alive( ) && !m_EntityAlife.use_simplified_visual( ) )
    //{
    // ActivateShell( NULL );
    // m_PhysicMovementControl->DestroyCharacter( );
    //}
    else if (ik_controller())
    {
        update_interactive_anims();
        ik_controller()->Update();
    }

#ifdef DEBUG
    if (Type() == etStalker && ph_dbg_draw_mask1.test(phDbgHitAnims))
    {
        Fmatrix m;
        m_hit_animations.GetBaseMatrix(m, m_EntityAlife);
        DBG_DrawMatrix(m, 1.5f);
        /*
                IKinematicsAnimated	*K = smart_cast<IKinematicsAnimated*>(m_EntityAlife.Visual());
                u16 hb = K->LL_BoneID("bip01_head");
                u16 pb = K->LL_GetBoneRoot();
                u16 nb = K->LL_BoneID("bip01_neck");
                u16 eb = K->LL_BoneID("eye_right");
                Fmatrix &mh  = K->LL_GetTransform(hb);
                Fmatrix &mp  = K->LL_GetTransform(pb);
                Fmatrix &me	 = K->LL_GetTransform(eb);
                Fmatrix &mn	 = K->LL_GetTransform(nb);
                float d = DET(mh);
                if(Fvector().sub(mh.c,mp.c).magnitude() < 0.3f||d<0.7 )//|| Fvector().sub(me.c,mn.c) < 0.5
                {

                    K->CalculateBones_Invalidate();
                    K->CalculateBones();
                    ;
                }
        */
    }
#endif
}

void CCharacterPhysicsSupport::CreateSkeleton(CPhysicsShell*& pShell)
{
    R_ASSERT2(!pShell, "pShell already initialized!!");
    if (!m_EntityAlife.Visual())
        return;
#ifdef DEBUG
    CTimer t;
    t.Start();
#endif
    pShell = P_create_Shell();

    IKinematics* k = smart_cast<IKinematics*>(m_EntityAlife.Visual());

    phys_shell_verify_object_model(m_EntityAlife);
    pShell->preBuild_FromKinematics(k);

    pShell->mXFORM.set(mXFORM);

    pShell->SmoothElementsInertia(0.3f);
    pShell->set_PhysicsRefObject(&m_EntityAlife);
    SAllDDOParams disable_params;
    disable_params.Load(smart_cast<IKinematics*>(m_EntityAlife.Visual())->LL_UserData());
    pShell->set_DisableParams(disable_params);

    pShell->Build();

#ifdef DEBUG
    Msg("shell for %s[%d] created in %f ms", *m_EntityAlife.cName(), m_EntityAlife.ID(), t.GetElapsed_sec() * 1000.f);
#endif
}

bool CCharacterPhysicsSupport::DoCharacterShellCollide()
{
    if (m_eType == etStalker)
    {
        CAI_Stalker* OBJ = smart_cast<CAI_Stalker*>(&m_EntityAlife);
        VERIFY(OBJ);
        return !OBJ->wounded();
    }
    return true;
}

bool CCharacterPhysicsSupport::CollisionCorrectObjPos(const Fvector& start_from, bool character_create /*=false*/)
{
    // Fvector shift;shift.sub( start_from, m_EntityAlife.Position() );
    Fvector shift;
    shift.set(0, 0, 0);
    Fbox box;
    if (character_create)
        box.set(movement()->Box());
    else
    {
        if (m_pPhysicsShell)
        {
            VERIFY(m_pPhysicsShell->isFullActive());
            Fvector sz, c;
            get_box(m_pPhysicsShell, mXFORM, sz, c);
            box.setb(Fvector().sub(c, m_EntityAlife.Position()), Fvector(sz).mul(0.5f));
            m_pPhysicsShell->DisableCollision();
        }
        else
            box.set(m_EntityAlife.BoundingBox());
    }

    Fvector vbox;
    Fvector activation_pos;
    box.get_CD(activation_pos, vbox);
    shift.add(activation_pos);
    vbox.mul(2.f);
    activation_pos.add(shift, m_EntityAlife.Position());
    bool not_collide_characters = !DoCharacterShellCollide() && !character_create;
    bool set_rotation = !character_create;

    Fvector activation_res = Fvector().set(0, 0, 0);
    ////////////////

    bool ret = ActivateShapeCharacterPhysicsSupport(
        activation_res, vbox, activation_pos, mXFORM, not_collide_characters, set_rotation, &m_EntityAlife);
    //////////////////

    m_EntityAlife.Position().sub(activation_res, shift);

    if (m_pPhysicsShell)
        m_pPhysicsShell->EnableCollision();
    return ret;
}

void CCharacterPhysicsSupport::set_movement_position(const Fvector& pos)
{
    VERIFY(movement());

    CollisionCorrectObjPos(pos, true);

    movement()->SetPosition(m_EntityAlife.Position());
}
void CCharacterPhysicsSupport::ForceTransform(const Fmatrix& m)
{
    if (!m_EntityAlife.g_Alive())
        return;
    VERIFY(_valid(m));
    m_EntityAlife.XFORM().set(m);
    if (movement()->CharacterExist())
        movement()->EnableCharacter();
    set_movement_position(m.c);
    movement()->SetVelocity(0, 0, 0);
}
/*
void reset_root_bone_start_pose( CPhysicsShell& shell )
{
    VERIFY( &shell );
    CPhysicsElement * physics_root_element = shell.get_ElementByStoreOrder( 0 );
    VERIFY( physics_root_element );

    IKinematics * K = shell.PKinematics();
    VERIFY( K );

    u16	animation_root_bone_id = K->LL_GetBoneRoot();

    CODEGeom	*physics_root_bone_geom = physics_root_element->geometry( 0 );
    VERIFY( physics_root_bone_geom );

    u16 physics_root_bone_id = physics_root_bone_geom->bone_id();
    VERIFY( physics_root_bone_id != BI_NONE );

    if( animation_root_bone_id == physics_root_bone_id )
        return ;

    //u16 anim_bones_number = K->LL_BoneCount();

    //buffer_vector<u32>	anim_bones_bind_positions( xr_alloca(anim_bones_number*sizeof(u32)),
    //												anim_bones_number
    //											);
#pragma todo("LL_GetBindTransform shoud use buffer_vector")

    xr_vector<Fmatrix> anim_bones_bind_positions;
    K->LL_GetBindTransform( anim_bones_bind_positions );


    const Fmatrix physics_root_to_anim_root_bind_transformation
        = Fmatrix().mul_43( Fmatrix().invert( anim_bones_bind_positions[ physics_root_bone_id ] ),
                                              anim_bones_bind_positions[ animation_root_bone_id ] );

    const Fmatrix &physics_root_bone_anim_transform = K->LL_GetTransform( physics_root_bone_id );

    const Fmatrix physics_root_bone_corrected_pos = Fmatrix().mul_43( physics_root_bone_anim_transform ,
                                                                      physics_root_to_anim_root_bind_transformation
                                                                      );

    physics_root_element->SetTransform( Fmatrix().mul_43( shell.mXFORM, physics_root_bone_corrected_pos ) );

    //physics_root_element->TransformPosition( Fmatrix().mul_43( Fmatrix().invert( K->LL_GetTransform(
animation_root_bone_id ) ), physics_root_bone_corrected_pos ) );
}
*/
static const u32 physics_shell_animated_destroy_delay = 3000;
void CCharacterPhysicsSupport::destroy_animation_collision()
{
    xr_delete(m_physics_shell_animated);
    m_physics_shell_animated_time_destroy = u32(-1);
}
void CCharacterPhysicsSupport::create_animation_collision()
{
    m_physics_shell_animated_time_destroy = Device.dwTimeGlobal + physics_shell_animated_destroy_delay;
    if (m_physics_shell_animated)
        return;
    m_physics_shell_animated = xr_new<physics_shell_animated>(&m_EntityAlife, true);
}

void CCharacterPhysicsSupport::update_animation_collision()
{
    if (animation_collision())
    {
        animation_collision()->update(mXFORM);
        // animation_collision( )->shell()->set_LinearVel( movement()->GetVelocity() );
        if (Device.dwTimeGlobal > m_physics_shell_animated_time_destroy)
            destroy_animation_collision();
    }
}
#ifdef DEBUG
BOOL dbg_draw_ragdoll_spawn = FALSE;
#endif
void CCharacterPhysicsSupport::ActivateShell(IGameObject* who)
{
    R_ASSERT(_valid(m_EntityAlife.Position()));
    Fvector start;
    start.set(m_EntityAlife.Position());
    Fvector velocity;
    Fvector death_position;
    CreateShell(who, death_position, velocity);
    EndActivateFreeShell(who, start, death_position, velocity);
    VERIFY(m_pPhysicsShell);
    m_pPhysicsShell->Enable();
    m_pPhysicsShell->set_LinearVel(Fvector().set(0, -1, 0));
}
// void	CCharacterPhysicsSupport::	on_active_weapon_shell_activate()
//{
//	if( !m_weapon_geoms.empty() )
//		RemoveActiveWeaponCollision		();
//}
bool CCharacterPhysicsSupport::has_shell_collision_place(const CPhysicsShellHolder* obj) const
{
    return m_active_item_obj && obj == m_active_item_obj;
}
void CCharacterPhysicsSupport::on_child_shell_activate(CPhysicsShellHolder* obj)
{
    if (!has_shell_collision_place(obj))
        return;

    VERIFY(obj->PPhysicsShell());
#if 0
//	DBG_OpenCashedDraw();
	//m_pPhysicsShell->dbg_draw_geometry( 0.2f, color_xrgb( 255, 100, 0 ) );
	m_pPhysicsShell->dbg_draw_velocity( 0.01f, color_xrgb( 100, 255, 0 ) );
	m_pPhysicsShell->dbg_draw_force( 0.1f, color_xrgb( 100, 0, 255 ) );
	DBG_ClosedCashedDraw( 50000 );
#endif
    // DBG_OpenCashedDraw();
    // obj->PPhysicsShell()->dbg_draw_geometry( 0.2f, color_xrgb( 255, 100, 0 ) );

    RemoveActiveWeaponCollision();

    // DBG_ClosedCashedDraw( 50000 );
}

void CCharacterPhysicsSupport::RemoveActiveWeaponCollision()
{
    VERIFY(m_pPhysicsShell);
    VERIFY(m_weapon_attach_bone);
    VERIFY(!m_weapon_geoms.empty());
    xr_vector<CODEGeom *>::iterator ii = m_weapon_geoms.begin(), ee = m_weapon_geoms.end();
    Fmatrix m0;
    (*ii)->get_xform(m0);
    CPhysicsElement* root = m_active_item_obj->PPhysicsShell()->get_ElementByStoreOrder(0);
    CODEGeom* rg = root->geometry(0);
    VERIFY(rg);
    Fmatrix m1;
    rg->get_xform(m1);

    Fmatrix me;
    root->GetGlobalTransformDynamic(&me);

    Fmatrix m1_to_e = Fmatrix().mul_43(Fmatrix().invert(m1), me);

    Fmatrix m0e = Fmatrix().mul_43(m0, m1_to_e);
    root->SetTransform(m0e, mh_unspecified);

    for (; ii != ee; ++ii)
    {
        CODEGeom* g = (*ii);

        // g->dbg_draw( 0.01f, color_xrgb( 0, 0, 255 ), Flags32() );

        m_weapon_attach_bone->remove_geom(g);
        g->destroy();
        xr_delete(g);
    }

    // m_active_item_obj->PPhysicsShell()->dbg_draw_geometry( 0.2f, color_xrgb( 255, 0, 100 ) );

    Fvector a_vel, l_vel;
    const Fvector& mc = root->mass_Center();
    // dBodyGetPointVel( m_weapon_attach_bone->get_body(),mc.x, mc.y, mc.z, cast_fp(l_vel) );
    m_weapon_attach_bone->GetPointVel(l_vel, mc);
    m_weapon_attach_bone->get_AngularVel(a_vel);

    root->set_AngularVel(a_vel);
    root->set_LinearVel(l_vel);

    m_weapon_geoms.clear();
    m_weapon_attach_bone = 0;
    m_active_item_obj = 0;

    bone_fix_clear();
}
void CCharacterPhysicsSupport::bone_fix_clear()
{
    xr_vector<anim_bone_fix *>::iterator i = m_weapon_bone_fixes.begin(), e = m_weapon_bone_fixes.end();
    for (; i != e; ++i)
    {
        (*i)->deinit();
        xr_delete(*i);
    }
    m_weapon_bone_fixes.clear();
}

void CCharacterPhysicsSupport::bone_chain_disable(u16 bone, u16 r_bone, IKinematics& K)
{
    VERIFY(&K);
    u16 bid = bone;
    // K.LL_GetBoneInstance( bid ).set_callback( bctCustom, 0, 0, TRUE );

    while (bid != r_bone && bid != K.LL_GetBoneRoot())
    {
        CBoneData& bd = K.LL_GetData(bid);
        if (K.LL_GetBoneInstance(bid).callback() != anim_bone_fix::callback)
        {
            m_weapon_bone_fixes.push_back(xr_new<anim_bone_fix>());
            m_weapon_bone_fixes.back()->fix(bid, K);
        }
        bid = bd.GetParentID();

        // K.LL_GetBoneInstance( bid ).set_callback( bctCustom, 0, 0, TRUE );
    }
}

void CCharacterPhysicsSupport::AddActiveWeaponCollision()
{
    if (m_eType != etStalker)
        return;
    VERIFY(!m_weapon_attach_bone);
    VERIFY(!m_active_item_obj);
    VERIFY(m_weapon_geoms.empty());
    VERIFY(m_weapon_bone_fixes.empty());

    CInventoryOwner* inv_owner = smart_cast<CInventoryOwner*>(&m_EntityAlife);
    VERIFY(inv_owner);
    PIItem active_weapon_item = inv_owner->inventory().ActiveItem();
    if (!active_weapon_item)
        return;
    int bl = -1, br = -1, br2 = -1;
    m_EntityAlife.g_WeaponBones(bl, br, br2);
    if (br == -1)
        return;

    active_weapon_item->UpdateXForm();

    CPhysicsShell* weapon_shell = P_build_Shell(&active_weapon_item->object(), true, (BONE_P_MAP*)(0), true);

    VERIFY(m_pPhysicsShell);
    CPhysicsElement* weapon_attach_bone = m_pPhysicsShell->get_PhysicsParrentElement((u16)br);

    bone_chain_disable((u16)br, weapon_attach_bone->m_SelfID, *m_pPhysicsShell->PKinematics());
    if (bl != br && bl != -1)
    {
        CPhysicsElement* p = m_pPhysicsShell->get_PhysicsParrentElement((u16)bl);
        VERIFY(p);
        bone_chain_disable((u16)bl, p->m_SelfID, *m_pPhysicsShell->PKinematics());
    }
    if (br2 != bl && br2 != br && br2 != -1)
    {
        CPhysicsElement* p = m_pPhysicsShell->get_PhysicsParrentElement((u16)br2);
        VERIFY(p);
        bone_chain_disable((u16)br2, weapon_attach_bone->m_SelfID, *m_pPhysicsShell->PKinematics());
    }

    CPhysicsElement* weapon_element = weapon_shell->get_ElementByStoreOrder(0);

    u16 geom_num = weapon_element->numberOfGeoms();
    for (u16 i = 0; i < geom_num; ++i)
        m_weapon_geoms.push_back(weapon_element->geometry(i));
    xr_vector<CODEGeom *>::iterator ii = m_weapon_geoms.begin(), ee = m_weapon_geoms.end();

    // DBG_OpenCashedDraw();

    for (; ii != ee; ++ii)
    {
        CODEGeom* g = (*ii);
        // g->dbg_draw( 0.01f, color_xrgb( 255, 0, 0 ), Flags32() );
        weapon_element->remove_geom(g);
        g->set_bone_id(weapon_attach_bone->m_SelfID);
        weapon_attach_bone->add_geom(g);
        // g->dbg_draw( 0.01f, color_xrgb( 0, 255, 0 ), Flags32() );
    }
    m_weapon_attach_bone = weapon_attach_bone;
    m_active_item_obj = &(active_weapon_item->object());

    destroy_physics_shell(weapon_shell);

    // m_pPhysicsShell->dbg_draw_geometry( 1, color_xrgb( 0, 0, 255 ) );
    // DBG_ClosedCashedDraw( 50000 );
}

void CCharacterPhysicsSupport::CreateShell(IGameObject* who, Fvector& dp, Fvector& velocity)
{
    xr_delete(m_collision_activating_delay);
    xr_delete(m_interactive_animation);
    destroy_animation_collision();
    // DestroyIKController( );
    IKinematics* K = smart_cast<IKinematics*>(m_EntityAlife.Visual());
    // animation movement controller issues
    bool anim_mov_ctrl = m_EntityAlife.animation_movement_controlled();
    CBoneInstance& BR = K->LL_GetBoneInstance(K->LL_GetBoneRoot());
    Fmatrix start_xform;
    start_xform.identity();
    CBlend* anim_mov_blend = 0;
    if (anim_mov_ctrl)
    {
        m_EntityAlife.animation_movement()->ObjStartXform(start_xform);
        anim_mov_blend = m_EntityAlife.animation_movement()->ControlBlend();

        m_EntityAlife.destroy_anim_mov_ctrl();
        BR.set_callback_overwrite(TRUE);
    }
    //
    u16 anim_root = K->LL_GetBoneRoot();
    u16 physics_root = anim_root;

    if (m_eType != etBitting)
    {
        physics_root = K->LL_BoneID("bip01_pelvis");
        K->LL_SetBoneRoot(physics_root);
    }
    //
    if (!m_physics_skeleton)
        CreateSkeleton(m_physics_skeleton);

    if (m_eType == etActor)
    {
        CActor* A = smart_cast<CActor*>(&m_EntityAlife);
        R_ASSERT2(A, "not an actor has actor type");
        if (A->Holder())
            return;
        if (m_eState == esRemoved)
            return;
    }

    //////////////////////this needs to evaluate object box//////////////////////////////////////////////////////
    if (m_eType != etBitting)
        K->LL_SetBoneRoot(anim_root);

    for (u16 I = K->LL_BoneCount() - 1; I != u16(-1); --I)
        K->LL_GetBoneInstance(I).reset_callback();
    //
    if (anim_mov_ctrl) // we do not whant to move by long animation in root
        BR.set_callback_overwrite(TRUE);
    //
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
    if (m_eType != etBitting)
        K->LL_SetBoneRoot(physics_root);
    ////////////////////////////////////////////////////////////////////////////

    if (m_pPhysicsShell)
        return;

    m_PhysicMovementControl->GetCharacterVelocity(velocity);

    if (!m_PhysicMovementControl->CharacterExist())
        dp.set(m_EntityAlife.Position());
    else
        m_PhysicMovementControl->GetDeathPosition(dp);
    m_PhysicMovementControl->DestroyCharacter();

    // shell create
    R_ASSERT2(m_physics_skeleton, "No skeleton created!!");
    m_pPhysicsShell = m_physics_skeleton;
    m_physics_skeleton = NULL;
    m_pPhysicsShell->set_Kinematics(K);
    m_pPhysicsShell->RunSimulation();
    m_pPhysicsShell->mXFORM.set(mXFORM);
    m_pPhysicsShell->SetCallbacks();
    //

    if (anim_mov_ctrl) // we do not whant to move by long animation in root
        BR.set_callback_overwrite(TRUE);

    if (!DoCharacterShellCollide())
        m_pPhysicsShell->DisableCharacterCollision();

    if (m_eType != etBitting)
        K->LL_SetBoneRoot(anim_root);

    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);

    if (m_eType != etBitting)
        K->LL_SetBoneRoot(physics_root);
    // reset_root_bone_start_pose( *m_pPhysicsShell );

    m_flags.set(fl_death_anim_on, FALSE);
    m_eState = esDead;
    m_flags.set(fl_skeleton_in_shell, TRUE);

    if (IsGameTypeSingle())
    {
        m_pPhysicsShell->SetPrefereExactIntegration(); // use exact integration for ragdolls in single
#ifndef DEAD_BODY_COLLISION
        m_pPhysicsShell->SetRemoveCharacterCollLADisable();
#endif
    }
    else
        m_pPhysicsShell->SetIgnoreDynamic();
    m_pPhysicsShell->SetIgnoreSmall();
    AddActiveWeaponCollision();
}
void CCharacterPhysicsSupport::EndActivateFreeShell(
    IGameObject* who, const Fvector& inital_entity_position, const Fvector& dp, const Fvector& velocity)
{
    VERIFY(m_pPhysicsShell);
    VERIFY(m_eState == esDead);
#ifdef DEBUG
    if (dbg_draw_ragdoll_spawn)
    {
        DBG_OpenCashedDraw();
        m_pPhysicsShell->dbg_draw_geometry(0.2f, color_xrgb(255, 100, 0));
        DBG_ClosedCashedDraw(50000);
    }
#endif

    CollisionCorrectObjPos(dp);
    m_pPhysicsShell->SetGlTransformDynamic(mXFORM);

#ifdef DEBUG
    if (dbg_draw_ragdoll_spawn)
    {
        DBG_OpenCashedDraw();
        m_pPhysicsShell->dbg_draw_geometry(0.2f, color_xrgb(255, 0, 100));
        DBG_ClosedCashedDraw(50000);
    }
#endif
    // fly back after correction
    FlyTo(Fvector().sub(inital_entity_position, m_EntityAlife.Position()));

#ifdef DEBUG
    if (dbg_draw_ragdoll_spawn)
    {
        DBG_OpenCashedDraw();
        m_pPhysicsShell->dbg_draw_geometry(0.2f, color_xrgb(100, 255, 100));
        DBG_ClosedCashedDraw(50000);
    }
#endif
    Fvector v = velocity;
    m_character_shell_control.apply_start_velocity_factor(who, v);

#ifdef DEBUG
    if (death_anim_debug)
    {
        Msg("death anim: ragdoll velocity picked from char controller =(%f,%f,%f), velocity applied to ragdoll "
            "=(%f,%f,%f) "
            " ",
            velocity.x, velocity.y, velocity.z, v.x, v.y, v.z);
    }
#endif

    m_pPhysicsShell->set_LinearVel(v);
    // actualize
    m_pPhysicsShell->GetGlobalTransformDynamic(&mXFORM);
    m_pPhysicsShell->mXFORM.set(mXFORM);

    // if( false &&  anim_mov_ctrl && anim_mov_blend && anim_mov_blend->blend != CBlend::eFREE_SLOT &&
    // anim_mov_blend->timeCurrent + Device.fTimeDelta*anim_mov_blend->speed <
    // anim_mov_blend->timeTotal-SAMPLE_SPF-EPS)//.
    //{
    //	const Fmatrix sv_xform = mXFORM;
    //	mXFORM.set( start_xform );
    //	//anim_mov_blend->blendPower = 1;
    //	anim_mov_blend->timeCurrent  += Device.fTimeDelta * anim_mov_blend->speed;
    //	m_pPhysicsShell->AnimToVelocityState( Device.fTimeDelta, 2 * default_l_limit, 10.f * default_w_limit );
    //	mXFORM.set( sv_xform );
    //}
    IKinematics* K = smart_cast<IKinematics*>(m_EntityAlife.Visual());
    // u16 root =K->LL_GetBoneRoot();
    // if( root!=0 )
    //{
    //	K->LL_GetTransform( 0 ).set( Fidentity );
    //
    //	K->LL_SetBoneVisible( 0, FALSE, FALSE );
    //}

    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
}

void CCharacterPhysicsSupport::in_ChangeVisual()
{
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(m_EntityAlife.Visual());
    if (m_ik_controller)
    {
        DestroyIKController();
        if (KA)
            CreateIKController();
    }
    xr_delete(m_interactive_animation);
    destroy_animation_collision();
    destroy(m_interactive_motion);

    if (KA)
    {
        m_death_anims.setup(KA, m_EntityAlife.cNameSect().c_str(), pSettings);
        if (Type() != etBitting)
            m_hit_animations.SetupHitMotions(*KA);
    }

    if (!m_physics_skeleton && !m_pPhysicsShell)
        return;

    if (m_pPhysicsShell)
    {
        VERIFY(m_eType != etStalker);
        if (m_physics_skeleton)
        {
            m_EntityAlife.processing_deactivate();
            m_physics_skeleton->Deactivate();
            xr_delete(m_physics_skeleton);
        }
        if (m_EntityAlife.Visual())
            CreateSkeleton(m_physics_skeleton);
        if (m_pPhysicsShell)
            m_pPhysicsShell->Deactivate();
        xr_delete(m_pPhysicsShell);
        if (m_EntityAlife.Visual())
            ActivateShell(NULL);
    }
}

bool CCharacterPhysicsSupport::CanRemoveObject()
{
    if (m_eType == etActor)
    {
        return false;
    }
    else
    {
        return !m_EntityAlife.IsPlaying();
    }
}

void CCharacterPhysicsSupport::PHGetLinearVell(Fvector& velocity)
{
    if (m_pPhysicsShell && m_pPhysicsShell->isActive())
    {
        m_pPhysicsShell->get_LinearVel(velocity);
    }
    else
        movement()->GetCharacterVelocity(velocity);
}

void CCharacterPhysicsSupport::CreateIKController()
{
    VERIFY(!m_ik_controller);
    m_ik_controller = xr_new<CIKLimbsController>();
    m_ik_controller->Create(&m_EntityAlife);
}
void CCharacterPhysicsSupport::DestroyIKController()
{
    if (!m_ik_controller)
        return;
    m_ik_controller->Destroy(&m_EntityAlife);
    xr_delete(m_ik_controller);
}

void CCharacterPhysicsSupport::in_NetRelcase(IGameObject* O)
{
    m_PhysicMovementControl->NetRelcase(O);

    if (m_sv_hit.is_valide() && m_sv_hit.initiator() == O)
        m_sv_hit = SHit();
}

void CCharacterPhysicsSupport::set_collision_hit_callback(ICollisionHitCallback* cc)
{
    xr_delete(m_collision_hit_callback);
    m_collision_hit_callback = cc;
}
ICollisionHitCallback* CCharacterPhysicsSupport::get_collision_hit_callback() { return m_collision_hit_callback; }
void CCharacterPhysicsSupport::FlyTo(const Fvector& disp)
{
    R_ASSERT(m_pPhysicsShell);
    float ammount = disp.magnitude();
    if (fis_zero(ammount, EPS_L))
        return;
    physics_world()->Freeze();
    bool g = m_pPhysicsShell->get_ApplyByGravity();
    m_pPhysicsShell->set_ApplyByGravity(false);
    m_pPhysicsShell->add_ObjectContactCallback(StaticEnvironmentCB);
    void* cd = m_pPhysicsShell->get_CallbackData();
    m_pPhysicsShell->set_CallbackData(m_pPhysicsShell->PIsland());
    m_pPhysicsShell->UnFreeze();
    Fvector vel;
    vel.set(disp);
    const u16 steps_num = 10;
    const float fsteps_num = steps_num;
    vel.mul(1.f / fsteps_num / fixed_step);

    for (u16 i = 0; steps_num > i; ++i)
    {
        m_pPhysicsShell->set_LinearVel(vel);
#if 0
	DBG_OpenCashedDraw();
	//m_pPhysicsShell->dbg_draw_geometry( 0.2f, color_xrgb( 255, 100, 0 ) );
	m_pPhysicsShell->dbg_draw_velocity( 0.01f, color_xrgb( 0, 255, 0 ) );
	m_pPhysicsShell->dbg_draw_force( 0.1f, color_xrgb( 0, 0, 255 ) );
//	DBG_ClosedCashedDraw( 50000 );
#endif
        physics_world()->Step();
#if 0
//	DBG_OpenCashedDraw();
	//m_pPhysicsShell->dbg_draw_geometry( 0.2f, color_xrgb( 255, 100, 0 ) );
	m_pPhysicsShell->dbg_draw_velocity( 0.01f, color_xrgb( 100, 255, 0 ) );
	m_pPhysicsShell->dbg_draw_force( 0.1f, color_xrgb( 100, 0, 255 ) );
	DBG_ClosedCashedDraw( 50000 );
#endif
    }
    // u16 step_num=disp.magnitude()/fixed_step;
    m_pPhysicsShell->set_ApplyByGravity(g);
    m_pPhysicsShell->set_CallbackData(cd);
    m_pPhysicsShell->remove_ObjectContactCallback(StaticEnvironmentCB);
    physics_world()->UnFreeze();
}

void CCharacterPhysicsSupport::on_create_anim_mov_ctrl()
{
    VERIFY(!anim_mov_state.active);
    // anim_mov_state.character_exist = m_PhysicMovementControl->CharacterExist();
    // if(anim_mov_state.character_exist)
    // m_PhysicMovementControl->DestroyCharacter();
    m_PhysicMovementControl->SetNonInteractive(true);
    anim_mov_state.active = true;
}

void CCharacterPhysicsSupport::on_destroy_anim_mov_ctrl()
{
    VERIFY(anim_mov_state.active);
    // if( anim_mov_state.character_exist )
    // CreateCharacter();
    m_PhysicMovementControl->SetNonInteractive(false);
    anim_mov_state.active = false;
}

bool CCharacterPhysicsSupport::is_interactive_motion() { return is_imotion(m_interactive_motion); }
bool CCharacterPhysicsSupport::can_drop_active_weapon()
{
    return !is_interactive_motion() && m_flags.test(fl_death_anim_on);
};

void CCharacterPhysicsSupport::in_Die()
{
    if (m_hit_valide_time < Device.dwTimeGlobal || !m_sv_hit.is_valide())
    {
        if (m_EntityAlife.use_simplified_visual())
            return;
        ActivateShell(NULL);
        m_PhysicMovementControl->DestroyCharacter();
        return;
    }
    in_Hit(m_sv_hit, true);
}

u16 CCharacterPhysicsSupport::PHGetSyncItemsNumber()
{
    if (movement()->CharacterExist())
        return 1;
    else
        return m_EntityAlife.CPhysicsShellHolder::PHGetSyncItemsNumber();
}
CPHSynchronize* CCharacterPhysicsSupport::PHGetSyncItem(u16 item)
{
    if (movement()->CharacterExist())
        return movement()->GetSyncItem();
    else
        return m_EntityAlife.CPhysicsShellHolder::PHGetSyncItem(item);
}
