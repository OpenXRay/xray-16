#include "pch_script.h"
#include "PhysicObject.h"
#include "xrPhysics/PhysicsShell.h"

#include "xrServer_Objects_ALife.h"
#include "Level.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "xrEngine/xr_collide_form.h"
#include "xrEngine/cf_dynamic_mesh.h"
#include "PHSynchronize.h"
#include "game_object_space.h"

#include "moving_bones_snd_player.h"
#include "xrPhysics/ExtendedGeom.h"
#ifdef DEBUG
#include "PHDebug.h"
#include "xrEngine/ObjectDump.h"
#endif
BOOL dbg_draw_doors = false;
CPhysicObject::CPhysicObject(void)
    : m_type(epotBox), m_mass(10.f), m_collision_hit_callback(nullptr), m_anim_blend(nullptr),
      bones_snd_player(nullptr), m_net_updateData(nullptr), m_just_after_spawn(false), m_activated(false) {}

CPhysicObject::~CPhysicObject(void) { xr_delete(m_net_updateData); }
BOOL CPhysicObject::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeObjectPhysic* po = smart_cast<CSE_ALifeObjectPhysic*>(e);
    R_ASSERT(po);
    m_type = EPOType(po->type);
    m_mass = po->mass;
    m_collision_hit_callback = NULL;
    m_anim_blend = 0;
    inherited::net_Spawn(DC);

    create_collision_model();

    CPHSkeleton::Spawn(e);
    setVisible(TRUE);
    setEnabled(TRUE);

    if (!PPhysicsShell()->isBreakable() && !scriptBinder.object() && !CPHSkeleton::IsRemoving())
        SheduleUnregister();

    // if (PPhysicsShell()->Animated())
    //{
    //	processing_activate();
    //}
    bones_snd_player = create_moving_bones_snd_player(*this);
    if (bones_snd_player)
        play_bones_sound();

    m_just_after_spawn = true;
    m_activated = false;

    if (DC->s_flags.is(M_SPAWN_UPDATE))
    {
        NET_Packet temp;
        temp.B.count = 0;
        DC->UPDATE_Write(temp);
        if (temp.B.count > 0)
        {
            temp.r_seek(0);
            net_Import(temp);
        }
    }
// processing_activate();
#ifdef DEBUG
    if (dbg_draw_doors)
    {
        DBG_OpenCashedDraw();
        Fvector closed, open;
        get_door_vectors(closed, open);
        DBG_ClosedCashedDraw(50000000);
    }
#endif
    return TRUE;
}
void CPhysicObject::create_collision_model()
{
    xr_delete(CForm);

    VERIFY(Visual());
    IKinematics* K = Visual()->dcast_PKinematics();
    VERIFY(K);

    CInifile* ini = K->LL_UserData();
    if (ini && ini->section_exist("collide") && ini->line_exist("collide", "mesh") && ini->r_bool("collide", "mesh"))
    {
        CForm = new CCF_DynamicMesh(this);
        return;
    }

    CForm = new CCF_Skeleton(this);

    /*
    switch(m_type) {
        case epotBox:
        case epotFixedChain:
        case epotFreeChain :
        case epotSkeleton  :	collidable.model = new CCF_Skeleton(this);	break;

        default: NODEFAULT;

    }
    */
}
void CPhysicObject::play_bones_sound()
{
    if (!bones_snd_player)
    {
        Msg("! no sound loaded for obj: %s, model :%s - can not play", cName().c_str(), cNameVisual().c_str());
        return;
    }
    if (is_active(bones_snd_player))
        return;
    // processing_activate();
    bones_snd_player->play(*this);
}

void CPhysicObject::stop_bones_sound()
{
    if (!is_active(bones_snd_player))
        return;
    // processing_deactivate();
    bones_snd_player->stop();
}

static CPhysicsShellHolder* retrive_collide_object(bool bo1, dContact& c)
{
    CPhysicsShellHolder* collide_obj = 0;

    dxGeomUserData* ud = 0;
    if (bo1)
        ud = PHRetrieveGeomUserData(c.geom.g2);
    else
        ud = PHRetrieveGeomUserData(c.geom.g1);

    if (ud)
        collide_obj = static_cast<CPhysicsShellHolder*>(ud->ph_ref_object);
    else
        collide_obj = 0;
    return collide_obj;
}
static void door_ignore(bool& do_collide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    CPhysicsShellHolder* collide_obj = retrive_collide_object(bo1, c);
    if (!collide_obj || collide_obj->cast_actor())
        return;

    CPhysicsShell* ph_shell = collide_obj->PPhysicsShell();
    if (!ph_shell)
    {
        do_collide = false; //? must be AI
        return;
    }
    VERIFY(ph_shell);

    if (ph_shell->HasTracedGeoms())
        return;

    do_collide = false;
}

void CPhysicObject::set_door_ignore_dynamics()
{
    R_ASSERT(PPhysicsShell());
    PPhysicsShell()->remove_ObjectContactCallback(door_ignore);
    PPhysicsShell()->add_ObjectContactCallback(door_ignore);
    // PPhysicsShell()->
}
void CPhysicObject::unset_door_ignore_dynamics()
{
    R_ASSERT(PPhysicsShell());
    PPhysicsShell()->remove_ObjectContactCallback(door_ignore);
}

void CPhysicObject::SpawnInitPhysics(CSE_Abstract* D)
{
    CreatePhysicsShell(D);
    RunStartupAnim(D);
}

void CPhysicObject::RunStartupAnim(CSE_Abstract* D)
{
    if (Visual() && smart_cast<IKinematics*>(Visual()))
    {
        //		CSE_PHSkeleton	*po	= smart_cast<CSE_PHSkeleton*>(D);
        IKinematicsAnimated* PKinematicsAnimated = NULL;
        R_ASSERT(Visual() && smart_cast<IKinematics*>(Visual()));
        PKinematicsAnimated = smart_cast<IKinematicsAnimated*>(Visual());
        if (PKinematicsAnimated)
        {
            CSE_Visual* visual = smart_cast<CSE_Visual*>(D);
            R_ASSERT(visual);
            R_ASSERT2(*visual->startup_animation, "no startup animation");

            VERIFY2((!!PKinematicsAnimated->LL_MotionID(visual->startup_animation.c_str()).valid()),
                (make_string(" animation %s not faund ", visual->startup_animation.c_str()) +
                    dbg_object_base_dump_string(this))
                    .c_str());
            m_anim_blend = m_anim_script_callback.play_cycle(PKinematicsAnimated, visual->startup_animation);
        }
        smart_cast<IKinematics*>(Visual())->CalculateBones_Invalidate();
        smart_cast<IKinematics*>(Visual())->CalculateBones(TRUE);
    }
}
IC bool check_blend(CBlend* b, LPCSTR name, LPCSTR sect, LPCSTR visual)
{
#ifdef DEBUG
    if (!b)
        Msg(" ! can not control anim - model is not animated name[%s] sect[%s] visual[%s]", name, sect, visual);
#endif
    return !!b;
}
void CPhysicObject::run_anim_forward()
{
    if (!check_blend(m_anim_blend, cName().c_str(), cNameSect().c_str(), cNameVisual().c_str()))
        return;
    m_anim_blend->playing = TRUE;
    m_anim_blend->stop_at_end_callback = TRUE;
    if (m_anim_blend->speed < 0.f)
        m_anim_blend->speed = -m_anim_blend->speed;
}
void CPhysicObject::run_anim_back()
{
    if (!check_blend(m_anim_blend, cName().c_str(), cNameSect().c_str(), cNameVisual().c_str()))
        return;
    m_anim_blend->playing = TRUE;
    m_anim_blend->stop_at_end_callback = TRUE;
    if (m_anim_blend->speed > 0.f)
        m_anim_blend->speed = -m_anim_blend->speed;
}
void CPhysicObject::stop_anim()
{
    if (!check_blend(m_anim_blend, cName().c_str(), cNameSect().c_str(), cNameVisual().c_str()))
        return;
    m_anim_blend->playing = FALSE;
}

float CPhysicObject::anim_time_get()
{
    if (!check_blend(m_anim_blend, cName().c_str(), cNameSect().c_str(), cNameVisual().c_str()))
        return 0.f;
    return m_anim_blend->timeCurrent;
}
void CPhysicObject::anim_time_set(float time)
{
    if (!check_blend(m_anim_blend, cName().c_str(), cNameSect().c_str(), cNameVisual().c_str()))
        return;
    if (time < 0.f || time > m_anim_blend->timeTotal)
    {
#ifdef DEBUG
        Msg(" ! can not set blend time %f - it must be in range 0 - %f(timeTotal) obj: %s, model: %s, anim: %s", time,
            m_anim_blend->timeTotal, cName().c_str(), cNameVisual().c_str(),
            smart_cast<IKinematicsAnimated*>(PPhysicsShell()->PKinematics())
                ->LL_MotionDefName_dbg(m_anim_blend->motionID)
                .first);
#endif
        return;
    }
    m_anim_blend->timeCurrent = time;
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    VERIFY(K);
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
}

void CPhysicObject::net_Destroy()
{
    // if (PPhysicsShell()->Animated())
    //{
    //	processing_deactivate();
    //}

    inherited::net_Destroy();
    CPHSkeleton::RespawnInit();
    xr_delete(bones_snd_player);
}

void CPhysicObject::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    CPHSkeleton::SaveNetState(P);
}
void CPhysicObject::CreatePhysicsShell(CSE_Abstract* e)
{
    CSE_ALifeObjectPhysic* po = smart_cast<CSE_ALifeObjectPhysic*>(e);
    CreateBody(po);
}

void CPhysicObject::CreateSkeleton(CSE_ALifeObjectPhysic* po)
{
    if (m_pPhysicsShell)
        return;
    if (!Visual())
        return;
    LPCSTR fixed_bones = *po->fixed_bones;
    m_pPhysicsShell = P_build_Shell(this, !po->_flags.test(CSE_PHSkeleton::flActive), fixed_bones);
    ApplySpawnIniToPhysicShell(&po->spawn_ini(), m_pPhysicsShell, fixed_bones[0] != '\0');
    ApplySpawnIniToPhysicShell(
        smart_cast<IKinematics*>(Visual())->LL_UserData(), m_pPhysicsShell, fixed_bones[0] != '\0');
}

void CPhysicObject::Load(LPCSTR section)
{
    inherited::Load(section);
    CPHSkeleton::Load(section);
}

void CPhysicObject::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
    CPHSkeleton::Update(dt);
#ifdef DEBUG
    if (dbg_draw_doors)
    {
        Fvector c, o;
        get_door_vectors(c, o);
    }
#endif
}

void CPhysicObject::UpdateCL()
{
    inherited::UpdateCL();

    //Если наш физический объект анимированный, то
    //двигаем объект за анимацией
    if (m_pPhysicsShell->PPhysicsShellAnimator())
    {
        m_pPhysicsShell->AnimatorOnFrame();
    }

    if (!IsGameTypeSingle())
    {
        Interpolate();
    }

    m_anim_script_callback.update(*this);
    PHObjectPositionUpdate();

#ifdef DEBUG
    if (dbg_draw_doors)
    {
        Fvector c, o;
        get_door_vectors(c, o);
    }
#endif

    if (!is_active(bones_snd_player))
        return;
    bones_snd_player->update(Device.fTimeDelta, *this);
}
void CPhysicObject::PHObjectPositionUpdate()
{
    if (m_pPhysicsShell)
    {
        if (m_type == epotBox)
        {
            m_pPhysicsShell->Update();
            XFORM().set(m_pPhysicsShell->mXFORM);
        }
        else if (m_pPhysicsShell->PPhysicsShellAnimator())
        {
            Fmatrix m;
            m_pPhysicsShell->InterpolateGlobalTransform(&m);
            XFORM().set(m);
        }
        else
            m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
    }
}

void CPhysicObject::AddElement(CPhysicsElement* root_e, int id)
{
    IKinematics* K = smart_cast<IKinematics*>(Visual());

    CPhysicsElement* E = P_create_Element();
    CBoneInstance& B = K->LL_GetBoneInstance(u16(id));
    E->mXFORM.set(K->LL_GetTransform(u16(id)));
    Fobb bb = K->LL_GetBox(u16(id));

    if (bb.m_halfsize.magnitude() < 0.05f)
    {
        bb.m_halfsize.add(0.05f);
    }
    E->add_Box(bb);
    E->setMass(10.f);
    E->set_ParentElement(root_e);
    B.set_callback(bctPhysics, m_pPhysicsShell->GetBonesCallback(), E);
    m_pPhysicsShell->add_Element(E);
    if (!(m_type == epotFreeChain && root_e == 0))
    {
        CPhysicsJoint* J = P_create_Joint(CPhysicsJoint::full_control, root_e, E);
        J->SetAnchorVsSecondElement(0, 0, 0);
        J->SetAxisDirVsSecondElement(1, 0, 0, 0);
        J->SetAxisDirVsSecondElement(0, 1, 0, 2);
        J->SetLimits(-M_PI / 2, M_PI / 2, 0);
        J->SetLimits(-M_PI / 2, M_PI / 2, 1);
        J->SetLimits(-M_PI / 2, M_PI / 2, 2);
        m_pPhysicsShell->add_Joint(J);
    }

    CBoneData& BD = K->LL_GetData(u16(id));
    for (vecBonesIt it = BD.children.begin(); BD.children.end() != it; ++it)
    {
        AddElement(E, (*it)->GetSelfID());
    }
}

void CPhysicObject::CreateBody(CSE_ALifeObjectPhysic* po)
{
    if (m_pPhysicsShell)
        return;
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    switch (m_type)
    {
    case epotBox:
    {
        m_pPhysicsShell = P_build_SimpleShell(this, m_mass, !po->_flags.test(CSE_ALifeObjectPhysic::flActive));
    }
    break;
    case epotFixedChain:
    case epotFreeChain:
    {
        m_pPhysicsShell = P_create_Shell();
        m_pPhysicsShell->set_Kinematics(pKinematics);
        AddElement(0, pKinematics->LL_GetBoneRoot());
        m_pPhysicsShell->setMass1(m_mass);
    }
    break;

    case epotSkeleton:
    {
        // pKinematics->LL_SetBoneRoot(0);
        CreateSkeleton(po);
    }
    break;

    default: {
    }
    break;
    }

    m_pPhysicsShell->mXFORM.set(XFORM());
    m_pPhysicsShell->SetAirResistance(0.001f, 0.02f);
    if (pKinematics)
    {
        SAllDDOParams disable_params;
        disable_params.Load(pKinematics->LL_UserData());
        m_pPhysicsShell->set_DisableParams(disable_params);
    }
    // m_pPhysicsShell->SetAirResistance(0.002f, 0.3f);
}

BOOL CPhysicObject::net_SaveRelevant()
{
    return TRUE; //! m_flags.test(CSE_ALifeObjectPhysic::flSpawnCopy);
}

BOOL CPhysicObject::UsedAI_Locations() { return (FALSE); }
void CPhysicObject::InitServerObject(CSE_Abstract* D)
{
    CPHSkeleton::InitServerObject(D);
    CSE_ALifeObjectPhysic* l_tpALifePhysicObject = smart_cast<CSE_ALifeObjectPhysic*>(D);
    if (!l_tpALifePhysicObject)
        return;
    l_tpALifePhysicObject->type = u32(m_type);
}
ICollisionHitCallback* CPhysicObject::get_collision_hit_callback() { return m_collision_hit_callback; }
void CPhysicObject::set_collision_hit_callback(ICollisionHitCallback* cc)
{
    xr_delete(m_collision_hit_callback);
    m_collision_hit_callback = cc;
}

//////////////////////////////////////////////////////////////////////////
/*
DEFINE_MAP_PRED	(LPCSTR,	CPhysicsJoint*,	JOINT_P_MAP,	JOINT_P_PAIR_IT,	pred_str);

JOINT_P_MAP			*l_tpJointMap = new JOINT_P_MAP();

l_tpJointMap->insert(std::make_pair(bone_name,joint*));
JOINT_P_PAIR_IT		I = l_tpJointMap->find(bone_name);
if (l_tpJointMap->end()!=I){
//bone_name is found and is an pair_iterator
(*I).second
}

JOINT_P_PAIR_IT		I = l_tpJointMap->begin();
JOINT_P_PAIR_IT		E = l_tpJointMap->end();
for ( ; I != E; ++I) {
(*I).second->joint_method();
Msg("%s",(*I).first);
}

*/

//////////////////////////////////////////////////////////////////////////
bool CPhysicObject::is_ai_obstacle() const
{
    return !!(READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "is_ai_obstacle", true));
}

// network synchronization ----------------------------

net_updatePhData* CPhysicObject::NetSync()
{
    if (!m_net_updateData)
        m_net_updateData = new net_updatePhData();
    return m_net_updateData;
}

void CPhysicObject::net_Export(NET_Packet& P)
{
    if (this->H_Parent() || IsGameTypeSingle())
    {
        P.w_u8(0);
        return;
    }

    CPHSynchronize* pSyncObj = NULL;
    SPHNetState State;
    pSyncObj = this->PHGetSyncItem(0);

    if (pSyncObj && !this->H_Parent())
        pSyncObj->get_State(State);
    else
        State.position.set(this->Position());

    mask_num_items num_items;
    num_items.mask = 0;
    u16 temp = this->PHGetSyncItemsNumber();
    R_ASSERT(temp < (u16(1) << 5));
    num_items.num_items = u8(temp);

    if (State.enabled)
        num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_state_enabled;
    if (fis_zero(State.angular_vel.square_magnitude()))
        num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_angular_null;
    if (fis_zero(State.linear_vel.square_magnitude()))
        num_items.mask |= CSE_ALifeObjectPhysic::inventory_item_linear_null;
    // if (m_pPhysicsShell->PPhysicsShellAnimator())		{num_items.mask |= CSE_ALifeObjectPhysic::animated;}

    P.w_u8(num_items.common);

    /*if (num_items.mask&CSE_ALifeObjectPhysic::animated)
    {
        net_Export_Anim_Params(P);
    }*/
    net_Export_PH_Params(P, State, num_items);

    if (PPhysicsShell()->isEnabled())
    {
        P.w_u8(1); // not freezed
    }
    else
    {
        P.w_u8(0); // freezed
    }
};

void CPhysicObject::net_Export_PH_Params(NET_Packet& P, SPHNetState& State, mask_num_items& num_items)
{
    // UI().Font().pFontStat->OutSet(100.0f,100.0f);
    P.w_vec3(State.force);
    // Msg("Export State.force.y:%4.6f",State.force.y);
    P.w_vec3(State.torque);
    // UI().Font().pFontStat->OutNext("Export State.torque:%4.6f",State.torque.magnitude());
    P.w_vec3(State.position);
    // Msg("Export State.position.y:%4.6f",State.position.y);
    // Msg("Export State.enabled:%i",int(State.enabled));

    float magnitude = _sqrt(State.quaternion.magnitude());
    if (fis_zero(magnitude))
    {
        magnitude = 1;
        State.quaternion.x = 0.f;
        State.quaternion.y = 0.f;
        State.quaternion.z = 1.f;
        State.quaternion.w = 0.f;
    }
    else
    {
        /*		float				invert_magnitude = 1.f/magnitude;

        State.quaternion.x	*= invert_magnitude;
        State.quaternion.y	*= invert_magnitude;
        State.quaternion.z	*= invert_magnitude;
        State.quaternion.w	*= invert_magnitude;

        clamp				(State.quaternion.x,-1.f,1.f);
        clamp				(State.quaternion.y,-1.f,1.f);
        clamp				(State.quaternion.z,-1.f,1.f);
        clamp				(State.quaternion.w,-1.f,1.f);*/
    }

    P.w_float(State.quaternion.x);
    P.w_float(State.quaternion.y);
    P.w_float(State.quaternion.z);
    P.w_float(State.quaternion.w);

    if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_angular_null))
    {
        /*	clamp				(State.angular_vel.x,-10.f*PI_MUL_2,10.f*PI_MUL_2);
        clamp				(State.angular_vel.y,-10.f*PI_MUL_2,10.f*PI_MUL_2);
        clamp				(State.angular_vel.z,-10.f*PI_MUL_2,10.f*PI_MUL_2);*/

        P.w_float(State.angular_vel.x);
        P.w_float(State.angular_vel.y);
        P.w_float(State.angular_vel.z);
    }

    if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_linear_null))
    {
        /*clamp				(State.linear_vel.x,-32.f,32.f);
        clamp				(State.linear_vel.y,-32.f,32.f);
        clamp				(State.linear_vel.z,-32.f,32.f);*/

        P.w_float(State.linear_vel.x);
        P.w_float(State.linear_vel.y);
        P.w_float(State.linear_vel.z);
        // Msg("Export State.linear_vel.y:%4.6f",State.linear_vel.y);
    }
    else
    {
        // Msg("Export State.linear_vel.y:%4.6f",0.0f);
    }
}

void CPhysicObject::net_Import(NET_Packet& P)
{
    u8 NumItems = 0;
    NumItems = P.r_u8();
    if (!NumItems)
        return;

    CSE_ALifeObjectPhysic::mask_num_items num_items;
    num_items.common = NumItems;
    NumItems = num_items.num_items;

    /*if (num_items.mask & CSE_ALifeObjectPhysic::animated)
    {
        net_Import_Anim_Params(P);
    }*/

    net_update_PItem N;
    N.dwTimeStamp = Device.dwTimeGlobal;

    net_Import_PH_Params(P, N, num_items);
    ////////////////////////////////////////////
    P.r_u8(); // freezed or not..

    if (this->cast_game_object()->Local())
    {
        return;
    }

    net_updatePhData* p = NetSync();

    //	if (!p->NET_IItem.empty() && (p->NET_IItem.back().dwTimeStamp>=N.dwTimeStamp))
    //		return;

    // if (!p->NET_IItem.empty())
    // m_flags.set							(FInInterpolate, TRUE);

    Level().AddObject_To_Objects4CrPr(this);
    // this->CrPr_SetActivated				(false);
    // this->CrPr_SetActivationStep			(0);

    p->NET_IItem.push_back(N);

    while (p->NET_IItem.size() > 2)
    {
        p->NET_IItem.pop_front();
    }
    if (!m_activated)
    {
#ifdef DEBUG
        Msg("Activating object [%d] before interpolation starts", ID());
#endif // #ifdef DEBUG
        processing_activate();
        m_activated = true;
    }
};

void CPhysicObject::net_Import_PH_Params(NET_Packet& P, net_update_PItem& N, mask_num_items& num_items)
{
    // N.State.force.set			(0.f,0.f,0.f);
    // N.State.torque.set			(0.f,0.f,0.f);
    // UI().Font().pFontStat->OutSet(100.0f,100.0f);
    P.r_vec3(N.State.force);
    // Msg("Import N.State.force.y:%4.6f",N.State.force.y);
    P.r_vec3(N.State.torque);

    P.r_vec3(N.State.position);
    // Msg("Import N.State.position.y:%4.6f",N.State.position.y);

    P.r_float(N.State.quaternion.x);
    P.r_float(N.State.quaternion.y);
    P.r_float(N.State.quaternion.z);
    P.r_float(N.State.quaternion.w);

    N.State.enabled = num_items.mask & CSE_ALifeObjectPhysic::inventory_item_state_enabled;
    // UI().Font().pFontStat->OutNext("Import N.State.enabled:%i",int(N.State.enabled));
    if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_angular_null))
    {
        N.State.angular_vel.x = P.r_float();
        N.State.angular_vel.y = P.r_float();
        N.State.angular_vel.z = P.r_float();
    }
    else
        N.State.angular_vel.set(0.f, 0.f, 0.f);

    if (!(num_items.mask & CSE_ALifeObjectPhysic::inventory_item_linear_null))
    {
        N.State.linear_vel.x = P.r_float();
        N.State.linear_vel.y = P.r_float();
        N.State.linear_vel.z = P.r_float();
    }
    else
        N.State.linear_vel.set(0.f, 0.f, 0.f);
    // Msg("Import N.State.linear_vel.y:%4.6f",N.State.linear_vel.y);

    N.State.previous_position = N.State.position;
    N.State.previous_quaternion = N.State.quaternion;
}

//-----------

void CPhysicObject::PH_B_CrPr(){};
void CPhysicObject::PH_I_CrPr() // actions & operations between two phisic prediction steps
    {};
void CPhysicObject::PH_A_CrPr()
{
    if (m_just_after_spawn)
    {
        VERIFY(Visual());
        IKinematics* K = Visual()->dcast_PKinematics();
        VERIFY(K);
        if (!PPhysicsShell())
        {
            return;
        }
        if (!PPhysicsShell()->isFullActive())
        {
            K->CalculateBones_Invalidate();
            K->CalculateBones(TRUE);
        }
        PPhysicsShell()->GetGlobalTransformDynamic(&XFORM());
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
#if 0
        Fbox bb= BoundingBox	();
        DBG_OpenCashedDraw		();
        Fvector c,r,p;
        bb.get_CD(c,r );
        XFORM().transform_tiny(p,c);
        DBG_DrawAABB( p, r,color_xrgb(255, 0, 0));
        //PPhysicsShell()->XFORM().transform_tiny(c);
        Fmatrix mm;
        PPhysicsShell()->GetGlobalTransformDynamic(&mm);
        mm.transform_tiny(p,c);
        DBG_DrawAABB( p, r,color_xrgb(0, 255, 0));
        DBG_ClosedCashedDraw	(50000);
#endif
        spatial_move();
        m_just_after_spawn = false;

        VERIFY(!OnServer());

        PPhysicsShell()->get_ElementByStoreOrder(0)->Fix();
        PPhysicsShell()->SetIgnoreStatic();
        // PPhysicsShell()->SetIgnoreDynamic	();
        // PPhysicsShell()->DisableCollision();
    }
    // CalculateInterpolationParams()
};

void CPhysicObject::CalculateInterpolationParams()
{
    if (this->m_pPhysicsShell)
        this->m_pPhysicsShell->NetInterpolationModeON();
};

void CPhysicObject::Interpolate()
{
    net_updatePhData* p = NetSync();
    CPHSynchronize* pSyncObj = this->PHGetSyncItem(0);

    // simple linear interpolation...
    if (!this->H_Parent() && this->getVisible() && this->m_pPhysicsShell && !OnServer() && p->NET_IItem.size())
    {
        SPHNetState newState = p->NET_IItem.front().State;

        if (p->NET_IItem.size() >= 2)
        {
            float ret_interpolate = interpolate_states(p->NET_IItem.front(), p->NET_IItem.back(), newState);
            // Msg("Interpolation factor is %0.4f", ret_interpolate);
            // Msg("Current position is: x = %3.3f, y = %3.3f, z = %3.3f", newState.position.x, newState.position.y,
            // newState.position.z);
            if (ret_interpolate >= 1.f)
            {
                p->NET_IItem.pop_front();
                if (m_activated)
                {
                    Msg("Deactivating object [%d] after interpolation finish", ID());
                    processing_deactivate();
                    m_activated = false;
                }
            }
        }
        pSyncObj->set_State(newState);
    }
}

float CPhysicObject::interpolate_states(
    net_update_PItem const& first, net_update_PItem const& last, SPHNetState& current)
{
    float ret_val = 0.f;
    u32 CurTime = Device.dwTimeGlobal;

    if (CurTime == last.dwTimeStamp)
        return 0.f;

    float factor = float(CurTime - last.dwTimeStamp) / float(last.dwTimeStamp - first.dwTimeStamp);

    ret_val = factor;
    if (factor > 1.f)
    {
        factor = 1.f;
    }
    else if (factor < 0.f)
    {
        factor = 0.f;
    }

    current.position.x = first.State.position.x + (factor * (last.State.position.x - first.State.position.x));
    current.position.y = first.State.position.y + (factor * (last.State.position.y - first.State.position.y));
    current.position.z = first.State.position.z + (factor * (last.State.position.z - first.State.position.z));
    current.previous_position = current.position;

    current.quaternion.slerp(first.State.quaternion, last.State.quaternion, factor);
    current.previous_quaternion = current.quaternion;
    return ret_val;
}

bool CPhysicObject::get_door_vectors(Fvector& closed, Fvector& open) const
{
    VERIFY(Visual());
    IKinematics* K = Visual()->dcast_PKinematics();
    VERIFY(K);
    u16 door_bone = K->LL_BoneID("door");
    if (door_bone == BI_NONE)
        return false;
    const CBoneData& bd = K->LL_GetData(door_bone);
    const SBoneShape& shape = bd.shape;
    if (shape.type != SBoneShape::stBox)
        return false;

    if (shape.flags.test(SBoneShape::sfNoPhysics))
        return false;

    Fmatrix start_bone_pos;
    K->Bone_GetAnimPos(start_bone_pos, door_bone, u8(-1), true);

    Fmatrix start_pos = Fmatrix().mul_43(XFORM(), start_bone_pos);

    const Fobb& box = shape.box;

    Fvector center_pos;
    start_pos.transform_tiny(center_pos, box.m_translate);

    Fvector door_dir;
    start_pos.transform_dir(door_dir, box.m_rotate.i);
    Fvector door_dir_local = box.m_rotate.i;
    // Fvector door_dir_bone; start_bone_pos.transform_dir(door_dir_bone, box.m_rotate.i );

    const Fvector det_vector = Fvector().sub(center_pos, start_pos.c);

    if (door_dir.dotproduct(det_vector) < 0.f)
    {
        door_dir.invert();
        door_dir_local.invert();
        // door_dir_bone.invert();
    }

    const SJointIKData& joint = bd.IK_data;

    // Xottab_DUTY: commented this to allow sliding type doors
    // https://github.com/OpenXRay/xray-16/issues/73
    /*if (joint.type != jtJoint)
        return false;*/

    const Fvector2& limits = joint.limits[1].limit;

    // if( limits.y < EPS ) //limits.y - limits.x < EPS
    //	return false;

    if (M_PI - limits.y < EPS && M_PI + limits.x < EPS)
        return false;

    Fmatrix to_hi = Fmatrix().rotateY(-limits.x);
    to_hi.transform_dir(open, door_dir_local);

    Fmatrix to_lo = Fmatrix().rotateY(-limits.y);
    to_lo.transform_dir(closed, door_dir_local);

    start_pos.transform_dir(open);
    start_pos.transform_dir(closed);

// DBG_OpenCashedDraw( );

#ifdef DEBUG
    if (dbg_draw_doors)
    {
        DBG_DrawMatrix(Fidentity, 10.0f);

        DBG_DrawMatrix(XFORM(), .5f, 100);

        DBG_DrawMatrix(start_pos, 0.2f, 100);

        const Fvector pos = start_pos.c.add(Fvector().set(0, 0.2f, 0));
        const Fvector pos1 = start_pos.c.add(Fvector().set(0, 0.3f, 0));

        DBG_DrawLine(pos, Fvector().add(pos, open), color_xrgb(0, 255, 0));
        DBG_DrawLine(pos, Fvector().add(pos, closed), color_xrgb(255, 0, 0));

        DBG_DrawLine(pos1, Fvector().add(pos1, det_vector), color_xrgb(255, 255, 0));
    }
#endif
    // DBG_ClosedCashedDraw( 50000000 );

    return true;
}
