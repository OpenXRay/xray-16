#include "StdAfx.h"
#include "Car.h"
//#if 0

#include "ParticlesObject.h"

#ifdef DEBUG
#include "xrEngine/StatGraph.h"
#include "PHDebug.h"
#endif // DEBUG

#include "PHDestroyable.h"

#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "math.h"
#include "script_entity_action.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "Include/xrRender/Kinematics.h"
#include "Level.h"
#include "ui/UIMainIngameWnd.h"
#include "CarWeapon.h"
#include "game_object_space.h"
#include "xrEngine/GameMtlLib.h"

#include "CharacterPhysicsSupport.h"
#include "car_memory.h"
#include "xrPhysics/IPHWorld.h"
BONE_P_MAP CCar::bone_map = BONE_P_MAP();

// extern CPHWorld*	ph_world;

CCar::CCar()
{
    m_memory = NULL;
    m_driver_anim_type = 0;
    m_bone_steer = BI_NONE;
    active_camera = 0;
    camera[ectFirst] = new CCameraFirstEye(this, CCameraBase::flRelativeLink | CCameraBase::flPositionRigid);
    camera[ectFirst]->tag = ectFirst;
    camera[ectFirst]->Load("car_firsteye_cam");

    camera[ectChase] = new CCameraLook(this, CCameraBase::flRelativeLink);
    camera[ectChase]->tag = ectChase;
    camera[ectChase]->Load("car_look_cam");

    camera[ectFree] = new CCameraLook(this);
    camera[ectFree]->tag = ectFree;
    camera[ectFree]->Load("car_free_cam");
    OnCameraChange(ectFirst);

    m_repairing = false;

    ///////////////////////////////
    //////////////////////////////
    /////////////////////////////
    b_wheels_limited = false;
    b_engine_on = false;
    e_state_steer = idle;
    e_state_drive = neutral;
    m_current_gear_ratio = phInfinity;
    rsp = false;
    lsp = false;
    fwp = false;
    bkp = false;
    brp = false;
    ///////////////////////////////
    //////////////////////////////
    /////////////////////////////
    m_exhaust_particles = "vehiclefx" DELIMITER "exhaust_1";
    m_car_sound = new SCarSound(this);

    //у машины слотов в инвентаре нет
    inventory = new CInventory();
    inventory->SetSlotsUseful(false);
    m_doors_torque_factor = 2.f;
    m_power_increment_factor = 0.5f;
    m_rpm_increment_factor = 0.5f;
    m_power_decrement_factor = 0.5f;
    m_rpm_decrement_factor = 0.5f;
    b_breaks = false;
    m_break_start = 0.f;
    m_break_time = 1.;
    m_breaks_to_back_rate = 1.f;

    b_exploded = false;
    m_car_weapon = NULL;
    m_power_neutral_factor = 0.25f;
    m_steer_angle = 0.f;
#ifdef DEBUG
    InitDebug();
#endif
}

CCar::~CCar(void)
{
    xr_delete(camera[0]);
    xr_delete(camera[1]);
    xr_delete(camera[2]);
    xr_delete(m_car_sound);
    ClearExhausts();
    xr_delete(inventory);
    xr_delete(m_car_weapon);
    xr_delete(m_memory);
    //	xr_delete			(l_tpEntityAction);
}

void CCar::reinit()
{
    CEntity::reinit();
    CScriptEntity::reinit();
    if (m_memory)
        m_memory->reinit();
}

void CCar::reload(LPCSTR section)
{
    CEntity::reload(section);
    if (m_memory)
        m_memory->reload(section);
}

void CCar::cb_Steer(CBoneInstance* B)
{
    VERIFY2(fsimilar(DET(B->mTransform), 1.f, DET_CHECK_EPS), "Bones receive returns 0 matrix");
    CCar* C = static_cast<CCar*>(B->callback_param());
    Fmatrix m;

    m.rotateZ(C->m_steer_angle);

    B->mTransform.mulB_43(m);
#ifdef DEBUG
    if (!fsimilar(DET(B->mTransform), 1.f, DET_CHECK_EPS))
    {
        Log("RotatingZ angle=", C->m_steer_angle);
        VERIFY2(0, "Bones callback returns BAD!!! matrix");
    }
#endif
}

// Core events
void CCar::Load(LPCSTR section)
{
    inherited::Load(section);
    // CPHSkeleton::Load(section);
    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
        self->GetSpatialData().type |= STYPE_VISIBLEFORAI;
}

BOOL CCar::net_Spawn(CSE_Abstract* DC)
{
#ifdef DEBUG
    InitDebug();
#endif
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeCar* co = smart_cast<CSE_ALifeCar*>(e);
    BOOL R = inherited::net_Spawn(DC);

    PKinematics(Visual())->CalculateBones_Invalidate();
    PKinematics(Visual())->CalculateBones(TRUE);

    CPHSkeleton::Spawn(e);
    setEnabled(TRUE);
    setVisible(TRUE);
    PKinematics(Visual())->CalculateBones_Invalidate();
    PKinematics(Visual())->CalculateBones(TRUE);
    m_fSaveMaxRPM = m_max_rpm;
    SetfHealth(co->health);

    if (!g_Alive())
        b_exploded = true;
    else
        b_exploded = false;

    CDamagableItem::RestoreEffect();

    CInifile* pUserData = PKinematics(Visual())->LL_UserData();
    if (pUserData->section_exist("destroyed"))
        CPHDestroyable::Load(pUserData, "destroyed");
    if (pUserData->section_exist("mounted_weapon_definition"))
        m_car_weapon = new CCarWeapon(this);

    if (pUserData->section_exist("visual_memory_definition"))
    {
        m_memory = new car_memory(this);
        m_memory->reload(pUserData->r_string("visual_memory_definition", "section"));
    }

    return (CScriptEntity::net_Spawn(DC) && R);
}

void CCar::ActorObstacleCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_colide)
    {
        if (material_1 && material_1->Flags.test(SGameMtl::flActorObstacle))
            do_colide = true;
        if (material_2 && material_2->Flags.test(SGameMtl::flActorObstacle))
            do_colide = true;
    }
}

void CCar::SpawnInitPhysics(CSE_Abstract* D)
{
    CSE_PHSkeleton* so = smart_cast<CSE_PHSkeleton*>(D);
    R_ASSERT(so);
    ParseDefinitions(); // parse ini filling in m_driving_wheels,m_steering_wheels,m_breaking_wheels
    CreateSkeleton(D); // creates m_pPhysicsShell & fill in bone_map
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    K->CalculateBones_Invalidate(); // this need to call callbacks
    K->CalculateBones(TRUE);
    Init(); // inits m_driving_wheels,m_steering_wheels,m_breaking_wheels values using recieved in ParceDefinitions &
    // from bone_map
    // PPhysicsShell()->add_ObjectContactCallback(ActorObstacleCallback);
    SetDefaultNetState(so);
    CPHUpdateObject::Activate();
}

void CCar::net_Destroy()
{
#ifdef DEBUG
    DBgClearPlots();
#endif
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    if (m_bone_steer != BI_NONE)
    {
        pKinematics->LL_GetBoneInstance(m_bone_steer).reset_callback();
    }
    CScriptEntity::net_Destroy();
    inherited::net_Destroy();
    CExplosive::net_Destroy();
    if (m_pPhysicsShell)
    {
        m_pPhysicsShell->Deactivate();
        m_pPhysicsShell->ZeroCallbacks();
        xr_delete(m_pPhysicsShell);
    }
    CHolderCustom::detach_Actor();
    ClearExhausts();
    m_wheels_map.clear();
    m_steering_wheels.clear();
    m_driving_wheels.clear();
    m_exhausts.clear();
    m_breaking_wheels.clear();
    m_doors.clear();
    m_gear_ratious.clear();
    m_car_sound->Destroy();
    CPHUpdateObject::Deactivate();
    CPHSkeleton::RespawnInit();
    m_damage_particles.Clear();
    CPHDestroyable::RespawnInit();
    CPHCollisionDamageReceiver::Clear();
    b_breaks = false;
}

void CCar::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    SaveNetState(P);
}

BOOL CCar::net_SaveRelevant()
{
    return TRUE;
    // return
    // !m_explosion_flags.test(CExplosive::flExploding)&&!CExplosive::IsExploded()&&!CPHDestroyable::Destroyed()&&!b_exploded;
}

void CCar::SaveNetState(NET_Packet& P)
{
    CPHSkeleton::SaveNetState(P);
    P.w_vec3(Position());
    Fvector Angle;
    XFORM().getXYZ(Angle);
    P.w_vec3(Angle);
    {
        xr_map<u16, SDoor>::iterator i, e;
        i = m_doors.begin();
        e = m_doors.end();
        P.w_u16(u16(m_doors.size()));
        for (; i != e; ++i)
            i->second.SaveNetState(P);
    }

    {
        xr_map<u16, SWheel>::iterator i, e;
        i = m_wheels_map.begin();
        e = m_wheels_map.end();
        P.w_u16(u16(m_wheels_map.size()));
        for (; i != e; ++i)
            i->second.SaveNetState(P);
    }
    P.w_float(GetfHealth());
}

void CCar::RestoreNetState(CSE_PHSkeleton* po)
{
    if (!po->_flags.test(CSE_PHSkeleton::flSavedData))
        return;
    CPHSkeleton::RestoreNetState(po);

    CSE_ALifeCar* co = smart_cast<CSE_ALifeCar*>(po);

    {
        xr_map<u16, SDoor>::iterator i, e;
        xr_vector<CSE_ALifeCar::SDoorState>::iterator ii = co->door_states.begin();
        i = m_doors.begin();
        e = m_doors.end();
        for (; i != e; ++i, ++ii)
        {
            i->second.RestoreNetState(*ii);
        }
    }
    {
        xr_map<u16, SWheel>::iterator i, e;
        xr_vector<CSE_ALifeCar::SWheelState>::iterator ii = co->wheel_states.begin();
        i = m_wheels_map.begin();
        e = m_wheels_map.end();
        for (; i != e; ++i, ++ii)
        {
            i->second.RestoreNetState(*ii);
        }
    }
    /*
        //as later may kill diable/enable state save it;
        bool enable = PPhysicsShell()->isEnabled();
    /////////////////////////////////////////////////////////////////////////
        Fmatrix restored_form;
        PPhysicsShell()->GetGlobalTransformDynamic(&restored_form);
    /////////////////////////////////////////////////////////////////////
        Fmatrix inv ,replace,sof;
        sof.setXYZ(co->o_Angle.x,co->o_Angle.y,co->o_Angle.z);
        sof.c.set(co->o_Position);
        inv.set(restored_form);
        inv.invert();
        replace.mul(sof,inv);
    ////////////////////////////////////////////////////////////////////
        {

            PKinematics(Visual())->CalculateBones_Invalidate();
            PKinematics(Visual())->CalculateBones();
            PPhysicsShell()->DisableCollision();
            CPHActivationShape activation_shape;//Fvector start_box;m_PhysicMovementControl.Box().getsize(start_box);

            Fvector center;Center(center);
            Fvector obj_size;BoundingBox().getsize(obj_size);
            get_box(PPhysicsShell(),restored_form,obj_size,center);
            replace.transform(center);
            activation_shape.Create(center,obj_size,this);
            activation_shape.set_rotation(sof);
            activation_shape.Activate(obj_size,1,1.f,M_PI/8.f);
            Fvector dd;
            dd.sub(activation_shape.Position(),center);
            activation_shape.Destroy();
            sof.c.add(dd);
            PPhysicsShell()->EnableCollision();
        }
    ////////////////////////////////////////////////////////////////////
        replace.mul(sof,inv);
        PPhysicsShell()->TransformPosition(replace);
        if(enable)PPhysicsShell()->Enable();
        else PPhysicsShell()->Disable();
        PPhysicsShell()->GetGlobalTransformDynamic(&XFORM());
        */
}
void CCar::SetDefaultNetState(CSE_PHSkeleton* po)
{
    if (po->_flags.test(CSE_PHSkeleton::flSavedData))
        return;
    xr_map<u16, SDoor>::iterator i, e;
    i = m_doors.begin();
    e = m_doors.end();
    for (; i != e; ++i)
    {
        i->second.SetDefaultNetState();
    }
}
void CCar::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
    if (CPHDestroyable::Destroyed())
        CPHDestroyable::SheduleUpdate(dt);
    else
        CPHSkeleton::Update(dt);

    if (CDelayedActionFuse::isActive() && CDelayedActionFuse::Update(GetfHealth()))
    {
        // CarExplode();
    }
    if (b_exploded && !m_explosion_flags.test(flExploding) && !getEnabled()) //! m_bExploding
        setEnabled(TRUE);
#ifdef DEBUG
    DbgSheduleUpdate();
#endif
}

void CCar::UpdateEx(float fov)
{
#ifdef DEBUG
    DbgUbdateCl();
#endif

    //	Log("UpdateCL",Device.dwFrame);
    // XFORM().set(m_pPhysicsShell->mXFORM);
    VisualUpdate(fov);
    if (OwnerActor() && OwnerActor()->IsMyCamera())
    {
        cam_Update(Device.fTimeDelta, fov);
        OwnerActor()->Cameras().UpdateFromCamera(Camera());
        OwnerActor()->Cameras().ApplyDevice(VIEWPORT_NEAR);
    }
}

BOOL CCar::AlwaysTheCrow() { return (m_car_weapon && m_car_weapon->IsActive()); }
void CCar::UpdateCL()
{
    inherited::UpdateCL();
    CExplosive::UpdateCL();
    if (m_car_weapon)
    {
        m_car_weapon->UpdateCL();
        if (m_memory)
            m_memory->set_camera(
                m_car_weapon->ViewCameraPos(), m_car_weapon->ViewCameraDir(), m_car_weapon->ViewCameraNorm());
    }
    ASCUpdate();
    if (Owner())
        return;
    //	UpdateEx			(g_fov);
    VisualUpdate(90);
    if (GetScriptControl())
        ProcessScripts();
}

void CCar::VisualUpdate(float fov)
{
    m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());

    Fvector lin_vel;
    m_pPhysicsShell->get_LinearVel(lin_vel);
    // Sound
    Fvector C, V;
    Center(C);
    V.set(lin_vel);
    
    m_car_sound->Update();
    if (Owner())
    {
        if (m_pPhysicsShell->isEnabled())
        {
            Owner()->XFORM().mul_43(XFORM(), m_sits_transforms[0]);
        }
        //
        // 		if(OwnerActor() && OwnerActor()->IsMyCamera())
        // 		{
        // 			cam_Update(Device.fTimeDelta, fov);
        // 			OwnerActor()->Cameras().Update(Camera());
        // 			OwnerActor()->Cameras().ApplyDevice();
        // 		}
        //
        /*		if(CurrentGameUI())//
                {
                    CurrentGameUI()->UIMainIngameWnd->CarPanel().Show(true);
                    CurrentGameUI()->UIMainIngameWnd->CarPanel().SetCarHealth(GetfHealth());
                    CurrentGameUI()->UIMainIngameWnd->CarPanel().SetSpeed(lin_vel.magnitude()/1000.f*3600.f/100.f);
                    CurrentGameUI()->UIMainIngameWnd->CarPanel().SetRPM(m_current_rpm/m_max_rpm/2.f);
                }
        */
    }

    UpdateExhausts();
    m_lights.Update();
}

void CCar::renderable_Render()
{
    inherited::renderable_Render();
    if (m_car_weapon)
        m_car_weapon->Render_internal();
}

void CCar::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    //	P.w_u32 (Level().timeServer());
    //	P.w_u16 (0);
}

void CCar::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
    //	u32 TimeStamp = 0;
    //	P.w_u32 (TimeStamp);
    //	u16 NumItems = 0;
    //	P.w_u32 (NumItems);
}

void CCar::OnHUDDraw(CCustomHUD* /**hud*/)
{
#ifdef DEBUG
    Fvector velocity;
    m_pPhysicsShell->get_LinearVel(velocity);
    UI().Font().pFontStat->SetColor(0xffffffff);
    UI().Font().pFontStat->OutSet(120, 530);
    UI().Font().pFontStat->OutNext("Position:      [%3.2f, %3.2f, %3.2f]", VPUSH(Position()));
    UI().Font().pFontStat->OutNext("Velocity:      [%3.2f]", velocity.magnitude());

#endif
}

// void CCar::Hit(float P,Fvector &dir,IGameObject * who,s16 element,Fvector p_in_object_space, float impulse,
// ALife::EHitType hit_type)
void CCar::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    // if(CDelayedActionFuse::isActive()||Initiator()==u16(-1)&&HDS.hit_type==ALife::eHitTypeStrike)
    //{
    //	HDS.power=0.f;
    //}

    // if(HDS.who->ID()!=ID())
    //{
    //	CExplosive::SetInitiator(HDS.who->ID());
    //}
    WheelHit(HDS.damage(), HDS.bone(), HDS.hit_type);
    DoorHit(HDS.damage(), HDS.bone(), HDS.hit_type);
    float hitScale = 1.f, woundScale = 1.f;
    if (HDS.hit_type != ALife::eHitTypeStrike)
        CDamageManager::HitScale(HDS.bone(), hitScale, woundScale);
    HDS.power *= GetHitImmunity(HDS.hit_type) * hitScale;

    inherited::Hit(&HDS);
    if (!CDelayedActionFuse::isActive())
    {
        CDelayedActionFuse::CheckCondition(GetfHealth());
    }
    CDamagableItem::HitEffect();
    //	if(Owner()&&Owner()->ID()==Level().CurrentEntity()->ID())
    //		CurrentGameUI()->UIMainIngameWnd->CarPanel().SetCarHealth(GetfHealth());
}

void CCar::ChangeCondition(float fDeltaCondition)
{
    CEntity::CalcCondition(-fDeltaCondition);
    CDamagableItem::HitEffect();
    if (Local() && !g_Alive() && !AlreadyDie())
        KillEntity(Initiator());
    //	if(Owner()&&Owner()->ID()==Level().CurrentEntity()->ID())
    //		CurrentGameUI()->UIMainIngameWnd->CarPanel().SetCarHealth(GetfHealth());
}

void CCar::PHHit(SHit& H)
{
    if (!m_pPhysicsShell)
        return;
    if (m_bone_steer == H.bone())
        return;
    if (CPHUpdateObject::IsActive())
    {
        Fvector vimpulse;
        vimpulse.set(H.direction());
        vimpulse.mul(H.phys_impulse());
        vimpulse.y *= GravityFactorImpulse();
        float mag = vimpulse.magnitude();
        if (!fis_zero(mag))
        {
            vimpulse.mul(1.f / mag);
            m_pPhysicsShell->applyHit(H.bone_space_position(), vimpulse, mag, H.bone(), H.type());
        }
    }
    else
    {
        m_pPhysicsShell->applyHit(H.bone_space_position(), H.direction(), H.phys_impulse(), H.bone(), H.type());
    }
}

void CCar::ApplyDamage(u16 level)
{
    CDamagableItem::ApplyDamage(level);
    switch (level)
    {
    case 1: m_damage_particles.Play1(this); break;
    case 2:
    {
        if (!CDelayedActionFuse::isActive())
        {
            CDelayedActionFuse::CheckCondition(GetfHealth());
        }
        m_damage_particles.Play2(this);
    }
    break;
    case 3: m_fuel = 0.f;
    }
}
void CCar::detach_Actor()
{
    if (!Owner())
        return;
    Owner()->setVisible(1);
    CHolderCustom::detach_Actor();
    PPhysicsShell()->remove_ObjectContactCallback(ActorObstacleCallback);
    NeutralDrive();
    Unclutch();
    ResetKeys();
    m_current_rpm = m_min_rpm;
    //	CurrentGameUI()->UIMainIngameWnd->CarPanel().Show(false);
    /// Break();
    // H_SetParent(NULL);
    HandBreak();
    processing_deactivate();
#ifdef DEBUG
    DBgClearPlots();
#endif
}

bool CCar::attach_Actor(CGameObject* actor)
{
    if (Owner() || CPHDestroyable::Destroyed())
        return false;
    CHolderCustom::attach_Actor(actor);

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* ini = K->LL_UserData();
    int id;
    if (ini->line_exist("car_definition", "driver_place"))
        id = K->LL_BoneID(ini->r_string("car_definition", "driver_place"));
    else
    {
        Owner()->setVisible(0);
        id = K->LL_GetBoneRoot();
    }
    CBoneInstance& instance = K->LL_GetBoneInstance(u16(id));
    m_sits_transforms.push_back(instance.mTransform);
    OnCameraChange(ectFirst);
    PPhysicsShell()->Enable();
    PPhysicsShell()->add_ObjectContactCallback(ActorObstacleCallback);
    //	VisualUpdate();
    processing_activate();
    ReleaseHandBreak();
    //	CurrentGameUI()->UIMainIngameWnd->CarPanel().Show(true);
    //	CurrentGameUI()->UIMainIngameWnd->CarPanel().SetCarHealth(fEntityHealth/100.f);
    // CurrentGameUI()->UIMainIngameWnd.ShowBattery(true);
    // CBoneData&	bone_data=K->LL_GetData(id);
    // Fmatrix driver_pos_tranform;
    // driver_pos_tranform.setHPB(bone_data.bind_hpb.x,bone_data.bind_hpb.y,bone_data.bind_hpb.z);
    // driver_pos_tranform.c.set(bone_data.bind_translate);
    // m_sits_transforms.push_back(driver_pos_tranform);
    // H_SetParent(actor);

    return true;
}

bool CCar::is_Door(u16 id, xr_map<u16, SDoor>::iterator& i)
{
    i = m_doors.find(id);
    if (i == m_doors.end())
    {
        return false;
    }
    else
    {
        if (i->second.joint) // temp for fake doors
            return true;
        else
            return false;
    }
}
bool CCar::is_Door(u16 id)
{
    xr_map<u16, SDoor>::iterator i;
    i = m_doors.find(id);
    if (i == m_doors.end())
    {
        return false;
    }
    return true;
}

bool CCar::Enter(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos)
{
    xr_map<u16, SDoor>::iterator i, e;

    i = m_doors.begin();
    e = m_doors.end();
    Fvector enter_pos;
    enter_pos.add(pos, foot_pos);
    enter_pos.mul(0.5f);
    for (; i != e; ++i)
    {
        if (i->second.CanEnter(pos, dir, enter_pos))
            return true;
    }
    return false;
}

bool CCar::Exit(const Fvector& pos, const Fvector& dir)
{
    xr_map<u16, SDoor>::iterator i, e;

    i = m_doors.begin();
    e = m_doors.end();
    for (; i != e; ++i)
    {
        if (i->second.CanExit(pos, dir))
        {
            i->second.GetExitPosition(m_exit_position);
            return true;
        }
    }
    return false;
}

void CCar::ParseDefinitions()
{
    bone_map.clear();

    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    bone_map.insert(std::make_pair(pKinematics->LL_GetBoneRoot(), physicsBone()));
    CInifile* ini = pKinematics->LL_UserData();
    R_ASSERT2(ini, "Car has no description !!! See ActorEditor Object - UserData");
    CExplosive::Load(ini, "explosion");
    // CExplosive::SetInitiator(ID());
    m_camera_position = ini->r_fvector3("car_definition", "camera_pos");
    ///////////////////////////car definition///////////////////////////////////////////////////
    fill_wheel_vector(ini->r_string("car_definition", "driving_wheels"), m_driving_wheels);
    fill_wheel_vector(ini->r_string("car_definition", "steering_wheels"), m_steering_wheels);
    fill_wheel_vector(ini->r_string("car_definition", "breaking_wheels"), m_breaking_wheels);
    fill_exhaust_vector(ini->r_string("car_definition", "exhausts"), m_exhausts);
    fill_doors_map(ini->r_string("car_definition", "doors"), m_doors);

    ///////////////////////////car properties///////////////////////////////

    m_max_power = ini->r_float("car_definition", "engine_power");
    m_max_power *= (0.8f * 1000.f);

    m_max_rpm = ini->r_float("car_definition", "max_engine_rpm");
    m_max_rpm *= (1.f / 60.f * 2.f * M_PI);

    m_min_rpm = ini->r_float("car_definition", "idling_engine_rpm");
    m_min_rpm *= (1.f / 60.f * 2.f * M_PI);

    m_power_rpm = ini->r_float("car_definition", "max_power_rpm");
    m_power_rpm *= (1.f / 60.f * 2.f * M_PI); //

    m_torque_rpm = ini->r_float("car_definition", "max_torque_rpm");
    m_torque_rpm *= (1.f / 60.f * 2.f * M_PI); //

    m_power_increment_factor =
        READ_IF_EXISTS(ini, r_float, "car_definition", "power_increment_factor", m_power_increment_factor);
    m_rpm_increment_factor =
        READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_increment_factor", m_rpm_increment_factor);
    m_power_decrement_factor =
        READ_IF_EXISTS(ini, r_float, "car_definition", "power_decrement_factor", m_power_increment_factor);
    m_rpm_decrement_factor =
        READ_IF_EXISTS(ini, r_float, "car_definition", "rpm_decrement_factor", m_rpm_increment_factor);
    m_power_neutral_factor =
        READ_IF_EXISTS(ini, r_float, "car_definition", "power_neutral_factor", m_power_neutral_factor);
    R_ASSERT2(m_power_neutral_factor > 0.1f && m_power_neutral_factor < 1.f, "power_neutral_factor must be 0 - 1 !!");
    if (ini->line_exist("car_definition", "exhaust_particles"))
    {
        m_exhaust_particles = ini->r_string("car_definition", "exhaust_particles");
    }

    b_auto_switch_transmission = !!ini->r_bool("car_definition", "auto_transmission");

    InitParabola();

    m_axle_friction = ini->r_float("car_definition", "axle_friction");
    m_steering_speed = ini->r_float("car_definition", "steering_speed");

    if (ini->line_exist("car_definition", "break_time"))
    {
        m_break_time = ini->r_float("car_definition", "break_time");
    }
    /////////////////////////transmission////////////////////////////////////////////////////////////////////////
    float main_gear_ratio = ini->r_float("car_definition", "main_gear_ratio");

    R_ASSERT2(ini->section_exist("transmission_gear_ratio"), "no section transmission_gear_ratio");
    m_gear_ratious.push_back(ini->r_fvector3("transmission_gear_ratio", "R"));
    m_gear_ratious[0][0] = -m_gear_ratious[0][0] * main_gear_ratio;
    string32 rat_num;
    for (int i = 1; true; ++i)
    {
        xr_sprintf(rat_num, "N%d", i);
        if (!ini->line_exist("transmission_gear_ratio", rat_num))
            break;
        Fvector gear_rat = ini->r_fvector3("transmission_gear_ratio", rat_num);
        gear_rat[0] *= main_gear_ratio;
        gear_rat[1] *= (1.f / 60.f * 2.f * M_PI);
        gear_rat[2] *= (1.f / 60.f * 2.f * M_PI);
        m_gear_ratious.push_back(gear_rat);
    }

    ///////////////////////////////sound///////////////////////////////////////////////////////
    m_car_sound->Init();
    ///////////////////////////////fuel///////////////////////////////////////////////////
    m_fuel_tank = ini->r_float("car_definition", "fuel_tank");
    m_fuel = m_fuel_tank;
    m_fuel_consumption = ini->r_float("car_definition", "fuel_consumption");
    m_fuel_consumption /= 100000.f;
    if (ini->line_exist("car_definition", "exhaust_particles"))
        m_exhaust_particles = ini->r_string("car_definition", "exhaust_particles");
    ///////////////////////////////lights///////////////////////////////////////////////////
    m_lights.Init(this);
    m_lights.ParseDefinitions();

    if (ini->section_exist("animations"))
    {
        m_driver_anim_type = ini->r_u16("animations", "driver_animation_type");
    }

    if (ini->section_exist("doors"))
    {
        m_doors_torque_factor = ini->r_u16("doors", "open_torque_factor");
    }

    m_damage_particles.Init(this);
}

void CCar::CreateSkeleton(CSE_Abstract* po)
{
    if (!Visual())
        return;
    IRenderVisual* pVis = Visual();
    IKinematics* pK = smart_cast<IKinematics*>(pVis);
    IKinematicsAnimated* pKA = smart_cast<IKinematicsAnimated*>(pVis);
    if (pKA)
    {
        pKA->PlayCycle("idle");
        pK->CalculateBones(TRUE);
    }
    phys_shell_verify_object_model(*this);
    /* Alundaio: p_build_shell
    #pragma todo(" replace below by P_build_Shell or call inherited")
    m_pPhysicsShell = P_create_Shell();
    m_pPhysicsShell->build_FromKinematics(pK, &bone_map);
    m_pPhysicsShell->set_PhysicsRefObject(this);
    m_pPhysicsShell->mXFORM.set(XFORM());
    m_pPhysicsShell->Activate(true);
    m_pPhysicsShell->SetAirResistance(0.f, 0.f);
    m_pPhysicsShell->SetPrefereExactIntegration();
    */
    m_pPhysicsShell = P_build_Shell(this, false, &bone_map);
    //-Alundaio

    ApplySpawnIniToPhysicShell(&po->spawn_ini(), m_pPhysicsShell, false);
    ApplySpawnIniToPhysicShell(pK->LL_UserData(), m_pPhysicsShell, false);
}

void CCar::Init()
{
    CPHCollisionDamageReceiver::Init();

    // get reference wheel radius
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    CInifile* ini = pKinematics->LL_UserData();
    R_ASSERT2(ini, "Car has no description !!! See ActorEditor Object - UserData");
    /// SWheel&
    /// ref_wheel=m_wheels_map.find(pKinematics->LL_BoneID(ini->r_string("car_definition","reference_wheel")))->second;

    if (ini->section_exist("air_resistance"))
    {
        PPhysicsShell()->SetAirResistance(default_k_l * ini->r_float("air_resistance", "linear_factor"),
            default_k_w * ini->r_float("air_resistance", "angular_factor"));
    }
    if (ini->line_exist("car_definition", "steer"))
    {
        m_bone_steer = pKinematics->LL_BoneID(ini->r_string("car_definition", "steer"));
        VERIFY2(fsimilar(DET(pKinematics->LL_GetTransform(m_bone_steer)), 1.f, EPS_L), "BBADD MTX");
        pKinematics->LL_GetBoneInstance(m_bone_steer).set_callback(bctPhysics, cb_Steer, this);
    }
    m_steer_angle = 0.f;
    // ref_wheel.Init();
    m_ref_radius = ini->r_float("car_definition", "reference_radius"); // ref_wheel.radius;
    b_exploded = false;
    b_engine_on = false;
    b_clutch = false;
    b_starting = false;
    b_stalling = false;
    b_transmission_switching = false;
    m_root_transform.set(bone_map.find(pKinematics->LL_GetBoneRoot())->second.element->mXFORM);
    m_current_transmission_num = 0;
    m_pPhysicsShell->set_DynamicScales(1.f, 1.f);
    CDamagableItem::Init(GetfHealth(), 3);
    float l_time_to_explosion = READ_IF_EXISTS(ini, r_float, "car_definition", "time_to_explosion", 120.f);
    CDelayedActionFuse::Initialize(l_time_to_explosion, CDamagableItem::DamageLevelToHealth(2));
    {
        xr_map<u16, SWheel>::iterator i, e;
        i = m_wheels_map.begin();
        e = m_wheels_map.end();
        for (; i != e; ++i)
        {
            i->second.Init();
            i->second.CDamagableHealthItem::Init(100.f, 2);
        }
    }

    {
        xr_vector<SWheelDrive>::iterator i, e;
        i = m_driving_wheels.begin();
        e = m_driving_wheels.end();
        for (; i != e; ++i)
            i->Init();
    }

    {
        xr_vector<SWheelBreak>::iterator i, e;
        i = m_breaking_wheels.begin();
        e = m_breaking_wheels.end();
        for (; i != e; ++i)
            i->Init();
    }

    {
        xr_vector<SWheelSteer>::iterator i, e;
        i = m_steering_wheels.begin();
        e = m_steering_wheels.end();
        for (; i != e; ++i)
            i->Init();
    }

    {
        xr_vector<SExhaust>::iterator i, e;
        i = m_exhausts.begin();
        e = m_exhausts.end();
        for (; i != e; ++i)
            i->Init();
    }

    {
        xr_map<u16, SDoor>::iterator i, e;
        i = m_doors.begin();
        e = m_doors.end();
        for (; i != e; ++i)
        {
            i->second.Init();
            i->second.CDamagableHealthItem::Init(100, 1);
        }
    }

    if (ini->section_exist("damage_items"))
    {
        CInifile::Sect& data = ini->r_section("damage_items");
        for (auto I = data.Data.cbegin(); I != data.Data.cend(); I++)
        {
            const CInifile::Item& item = *I;
            u16 index = pKinematics->LL_BoneID(*item.first);
            R_ASSERT3(index != BI_NONE, "Wrong bone name", *item.first);
            xr_map<u16, SWheel>::iterator i = m_wheels_map.find(index);

            if (i != m_wheels_map.end())
                i->second.CDamagableHealthItem::Init(float(atof(*item.second)), 2);
            else
            {
                xr_map<u16, SDoor>::iterator i = m_doors.find(index);
                R_ASSERT3(i != m_doors.end(), "only wheel and doors bones allowed for damage defs", *item.first);
                i->second.CDamagableHealthItem::Init(float(atof(*item.second)), 1);
            }
        }
    }

    if (ini->section_exist("immunities"))
    {
        LoadImmunities("immunities", ini);
    }

    CDamageManager::reload("car_definition", "damage", ini);

    HandBreak();
    Transmission(1);
}

void CCar::Revert() { m_pPhysicsShell->applyForce(0, 1.5f * EffectiveGravity() * m_pPhysicsShell->getMass(), 0); }
void CCar::NeutralDrive()
{
    xr_vector<SWheelDrive>::iterator i, e;
    i = m_driving_wheels.begin();
    e = m_driving_wheels.end();
    for (; i != e; ++i)
        i->Neutral();
    e_state_drive = neutral;
}
void CCar::ReleaseHandBreak()
{
    xr_vector<SWheelBreak>::iterator i, e;
    i = m_breaking_wheels.begin();
    e = m_breaking_wheels.end();
    for (; i != e; ++i)
        i->Neutral();
    if (e_state_drive == drive)
        Drive();
}
void CCar::Drive()
{
    if (!b_clutch || !b_engine_on)
        return;
    m_pPhysicsShell->Enable();
    m_current_rpm = EngineDriveSpeed();
    m_current_engine_power = EnginePower();
    xr_vector<SWheelDrive>::iterator i, e;
    i = m_driving_wheels.begin();
    e = m_driving_wheels.end();
    for (; i != e; ++i)
        i->Drive();
    e_state_drive = drive;
}

void CCar::StartEngine()
{
    if (m_fuel < EPS || b_engine_on)
        return;
    PlayExhausts();
    m_car_sound->Start();
    b_engine_on = true;
    m_current_rpm = 0.f;
    b_starting = true;
}
void CCar::StopEngine()
{
    if (!b_engine_on)
        return;
    // m_car_sound->Stop();
    // StopExhausts();
    AscCall(ascSndStall);
    AscCall(ascExhoustStop);
    NeutralDrive(); // set zero speed
    b_engine_on = false;
    UpdatePower(); // set engine friction;
    m_current_rpm = 0.f;
}

void CCar::Stall()
{
    // m_car_sound->Stall();
    // StopExhausts();
    AscCall(ascSndStall);
    AscCall(ascExhoustStop);
    NeutralDrive(); // set zero speed
    b_engine_on = false;
    UpdatePower(); // set engine friction;
    m_current_rpm = 0.f;
}
void CCar::ReleasePedals()
{
    Clutch();
    NeutralDrive(); // set zero speed
    UpdatePower(); // set engine friction;
}

void CCar::SwitchEngine()
{
    if (b_engine_on)
        StopEngine();
    else
        StartEngine();
}
void CCar::Clutch() { b_clutch = true; }
void CCar::Unclutch() { b_clutch = false; }
void CCar::Starter()
{
    b_starting = true;
    m_dwStartTime = Device.dwTimeGlobal;
}
void CCar::UpdatePower()
{
    m_current_rpm = EngineDriveSpeed();
    m_current_engine_power = EnginePower();
    if (b_auto_switch_transmission && !b_transmission_switching)
    {
        VERIFY2(CurrentTransmission() < m_gear_ratious.size(), "wrong transmission");
        if (m_current_rpm < m_gear_ratious[CurrentTransmission()][1])
            TransmissionDown();
        if (m_current_rpm > m_gear_ratious[CurrentTransmission()][2])
            TransmissionUp();
    }

    xr_vector<SWheelDrive>::iterator i, e;
    i = m_driving_wheels.begin();
    e = m_driving_wheels.end();
    for (; i != e; ++i)
        i->UpdatePower();
}

void CCar::SteerRight()
{
    b_wheels_limited = true; // no need to limit wheels when stiring
    m_pPhysicsShell->Enable();
    xr_vector<SWheelSteer>::iterator i, e;
    i = m_steering_wheels.begin();
    e = m_steering_wheels.end();
    for (; i != e; ++i)
        i->SteerRight();
    e_state_steer = right;
}
void CCar::SteerLeft()
{
    b_wheels_limited = true; // no need to limit wheels when stiring
    m_pPhysicsShell->Enable();
    xr_vector<SWheelSteer>::iterator i, e;
    i = m_steering_wheels.begin();
    e = m_steering_wheels.end();
    for (; i != e; ++i)
        i->SteerLeft();
    e_state_steer = left;
}

void CCar::SteerIdle()
{
    b_wheels_limited = false;
    m_pPhysicsShell->Enable();
    xr_vector<SWheelSteer>::iterator i, e;
    i = m_steering_wheels.begin();
    e = m_steering_wheels.end();
    for (; i != e; ++i)
        i->SteerIdle();
    e_state_steer = idle;
}

void CCar::LimitWheels()
{
    if (b_wheels_limited)
        return;
    b_wheels_limited = true;
    xr_vector<SWheelSteer>::iterator i, e;
    i = m_steering_wheels.begin();
    e = m_steering_wheels.end();
    for (; i != e; ++i)
        i->Limit();
}
void CCar::HandBreak()
{
    xr_vector<SWheelBreak>::iterator i, e;
    i = m_breaking_wheels.begin();
    e = m_breaking_wheels.end();
    for (; i != e; ++i)
        i->HandBreak();
}

void CCar::StartBreaking()
{
    if (!b_breaks)
    {
        b_breaks = true;
        m_break_start = Device.fTimeGlobal;
    }
}
void CCar::StopBreaking()
{
    xr_vector<SWheelBreak>::iterator i, e;
    i = m_breaking_wheels.begin();
    e = m_breaking_wheels.end();
    for (; i != e; ++i)
        i->Neutral();
    if (e_state_drive == drive)
        Drive();
    b_breaks = false;
}
void CCar::PressRight()
{
    if (lsp)
    {
        if (!fwp)
            SteerIdle();
    }
    else
        SteerRight();
    rsp = true;
}
void CCar::PressLeft()
{
    if (rsp)
    {
        if (!fwp)
            SteerIdle();
    }
    else
        SteerLeft();
    lsp = true;
}
void CCar::PressForward()
{
    if (bkp)
    {
        Unclutch();
        NeutralDrive();
    }
    else
    {
        DriveForward();
    }
    fwp = true;
}
void CCar::PressBack()
{
    if (fwp)
    {
        Unclutch();
        NeutralDrive();
    }
    else
    {
        // DriveBack();
        Unclutch();
        NeutralDrive();
        StartBreaking();
    }
    bkp = true;
}
void CCar::PressBreaks()
{
    HandBreak();
    brp = true;
}

void CCar::DriveBack()
{
    Clutch();
    Transmission(0);
    if (1 == CurrentTransmission() || 0 == CurrentTransmission())
        Starter();
    Drive();
}
void CCar::DriveForward()
{
    Clutch();
    if (0 == CurrentTransmission())
        Transmission(1);
    if (1 == CurrentTransmission() || 0 == CurrentTransmission())
        Starter();
    Drive();
}
void CCar::ReleaseRight()
{
    if (lsp)
        SteerLeft();
    else
        SteerIdle();
    rsp = false;
}
void CCar::ReleaseLeft()
{
    if (rsp)
        SteerRight();
    else
        SteerIdle();
    lsp = false;
}
void CCar::ReleaseForward()
{
    if (bkp)
    {
        Clutch();
        Transmission(0);
        if (1 == CurrentTransmission() || 0 == CurrentTransmission())
            Starter();
        Drive();
    }
    else
    {
        Unclutch();
        NeutralDrive();
    }

    fwp = false;
}
void CCar::ReleaseBack()
{
    if (b_breaks)
    {
        StopBreaking();
    }
    if (fwp)
    {
        Clutch();
        if (0 == CurrentTransmission())
            Transmission(1);
        if (1 == CurrentTransmission() || 0 == CurrentTransmission())
            Starter();
        Drive();
    }
    else
    {
        Unclutch();
        NeutralDrive();
    }
    bkp = false;
}

void CCar::ReleaseBreaks()
{
    ReleaseHandBreak();
    brp = false;
}

void CCar::Transmission(size_t num)
{
    if (num < m_gear_ratious.size())
    {
        if (CurrentTransmission() != num)
        {
            // m_car_sound					->TransmissionSwitch()		;
            AscCall(ascSndTransmission);
            m_current_transmission_num = num;
            m_current_gear_ratio = m_gear_ratious[num][0];
            b_transmission_switching = true;
            Drive();
        }
    }
#ifdef DEBUG
// Log("Transmission switch %d",(u32)num);
#endif
}
void CCar::CircleSwitchTransmission()
{
    if (0 == CurrentTransmission())
        return;
    size_t transmission = 1 + CurrentTransmission();
    transmission = transmission % m_gear_ratious.size();
    0 == transmission ? transmission++ : transmission;
    Transmission(transmission);
}

void CCar::TransmissionUp()
{
    if (0 == CurrentTransmission())
        return;
    size_t transmission = 1 + CurrentTransmission();
    size_t max_transmition_num = m_gear_ratious.size() - 1;
    transmission > max_transmition_num ? transmission = max_transmition_num : transmission;
    Transmission(transmission);
}

void CCar::TransmissionDown()
{
    if (0 == CurrentTransmission())
        return;
    size_t transmission = CurrentTransmission() - 1;
    transmission < 1 ? transmission = 1 : transmission;
    Transmission(transmission);
}

void CCar::PhTune(float step)
{
    for (u16 i = PPhysicsShell()->get_ElementsNumber(); i != 0; i--)
    {
        CPhysicsElement* e = PPhysicsShell()->get_ElementByStoreOrder(i - 1);
        if (e->isActive() && e->isEnabled())
            e->applyForce(0, e->getMass() * AntiGravityAccel(), 0);
        // dBodyAddForce(e->get_body(),0,e->getMass()*AntiGravityAccel(),0);
    }
}
float CCar::EffectiveGravity()
{
    float g = physics_world()->Gravity();
    if (CPHUpdateObject::IsActive())
        g *= 0.5f;
    return g;
}
float CCar::AntiGravityAccel() { return physics_world()->Gravity() - EffectiveGravity(); }
float CCar::GravityFactorImpulse() { return _sqrt(EffectiveGravity() / physics_world()->Gravity()); }
void CCar::UpdateBack()
{
    if (b_breaks)
    {
        float k = 1.f;
        float time = (Device.fTimeGlobal - m_break_start);
        if (time < m_break_time)
        {
            k *= (time / m_break_time);
        }
        xr_vector<SWheelBreak>::iterator i, e;
        i = m_breaking_wheels.begin();
        e = m_breaking_wheels.end();
        for (; i != e; ++i)
            i->Break(k);
        Fvector v;
        m_pPhysicsShell->get_LinearVel(v);
        // if(DriveWheelsMeanAngleRate()<m_breaks_to_back_rate)
        if (v.dotproduct(XFORM().k) < EPS)
        {
            StopBreaking();
            DriveBack();
        }
    }
    // else
    //{
    //	UpdatePower();
    //	if(b_engine_on&&!b_starting && m_current_rpm<m_min_rpm)Stall();
    //}
}

void CCar::PlayExhausts()
{
    xr_vector<SExhaust>::iterator i, e;
    i = m_exhausts.begin();
    e = m_exhausts.end();
    for (; i != e; ++i)
        i->Play();
}

void CCar::StopExhausts()
{
    xr_vector<SExhaust>::iterator i, e;
    i = m_exhausts.begin();
    e = m_exhausts.end();
    for (; i != e; ++i)
        i->Stop();
}

void CCar::UpdateExhausts()
{
    if (!b_engine_on)
        return;
    xr_vector<SExhaust>::iterator i, e;
    i = m_exhausts.begin();
    e = m_exhausts.end();
    for (; i != e; ++i)
        i->Update();
}

void CCar::ClearExhausts()
{
    xr_vector<SExhaust>::iterator i, e;
    i = m_exhausts.begin();
    e = m_exhausts.end();
    for (; i != e; ++i)
        i->Clear();
}

bool CCar::Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos)
{
    xr_map<u16, SDoor>::iterator i;

    if (!Owner())
    {
        if (Enter(pos, dir, foot_pos))
            return true;
    }

    RQR.r_clear();
    collide::ray_defs Q(pos, dir, 3.f, CDB::OPT_CULL, collide::rqtObject); // CDB::OPT_ONLYFIRST CDB::OPT_ONLYNEAREST
    VERIFY(!fis_zero(Q.dir.square_magnitude()));
    if (g_pGameLevel->ObjectSpace.RayQuery(RQR, CForm, Q))
    {
        collide::rq_results& R = RQR;
        int y = R.r_count();
        for (int k = 0; k < y; ++k)
        {
            collide::rq_result* I = R.r_begin() + k;
            if (is_Door((u16)I->element, i))
            {
                bool front = i->second.IsFront(pos, dir);
                if ((Owner() && !front) || (!Owner() && front))
                    i->second.Use();
                if (i->second.state == SDoor::broken)
                    break;
                return false;
            }
        }
    }

    if (Owner())
        return Exit(pos, dir);

    return false;
}
bool CCar::DoorUse(u16 id)
{
    xr_map<u16, SDoor>::iterator i;
    if (is_Door(id, i))
    {
        i->second.Use();
        return true;
    }
    else
    {
        return false;
    }
}

bool CCar::DoorSwitch(u16 id)
{
    xr_map<u16, SDoor>::iterator i;
    if (is_Door(id, i))
    {
        i->second.Switch();
        return true;
    }
    else
    {
        return false;
    }
}
bool CCar::DoorClose(u16 id)
{
    xr_map<u16, SDoor>::iterator i;
    if (is_Door(id, i))
    {
        i->second.Close();
        return true;
    }
    else
    {
        return false;
    }
}

bool CCar::DoorOpen(u16 id)
{
    xr_map<u16, SDoor>::iterator i;
    if (is_Door(id, i))
    {
        i->second.Open();
        return true;
    }
    else
    {
        return false;
    }
}
void CCar::InitParabola()
{
    // float t1=(m_power_rpm-m_torque_rpm);
    // float t2=m_max_power/m_power_rpm;
    // m_c = t2* (3.f*m_power_rpm - 4.f*m_torque_rpm)/t1/2.f;
    // t2/=m_power_rpm;
    // m_a = -t2/t1/2.f;
    // m_b = t2*m_torque_rpm/t1;

    // m_c = m_max_power* (3.f*m_power_rpm - 4.f*m_torque_rpm)/(m_power_rpm-m_torque_rpm)/2.f/m_power_rpm;
    // m_a = -m_max_power/(m_power_rpm-m_torque_rpm)/m_power_rpm/m_power_rpm/2.f;
    // m_b = m_max_power*m_torque_rpm/(m_power_rpm-m_torque_rpm)/m_power_rpm/m_power_rpm;

    m_a = expf((m_power_rpm - m_torque_rpm) / (2.f * m_power_rpm)) * m_max_power / m_power_rpm;
    m_b = m_torque_rpm;
    m_c = _sqrt(2.f * m_power_rpm * (m_power_rpm - m_torque_rpm));
}
float CCar::Parabola(float rpm)
{
    // float rpm_2=rpm*rpm;
    // float value=(m_a*rpm_2*rpm_2*rpm_2+m_b*rpm_2+m_c)*rpm_2;
    float ex = (rpm - m_b) / m_c;
    float value = m_a * expf(-ex * ex) * rpm;
    if (value < 0.f)
        return 0.f;
    if (e_state_drive == neutral)
        value *= m_power_neutral_factor;
    return value;
}

float CCar::EnginePower()
{
    float value;
    value = Parabola(m_current_rpm);
    if (b_starting)
    {
        if (m_current_rpm < m_min_rpm)
        {
            value = Parabola(m_min_rpm);
        }
        else if (Device.dwTimeGlobal - m_dwStartTime > 1000)
            b_starting = false;
    }
    if (value > m_current_engine_power)
        return value * m_power_increment_factor + m_current_engine_power * (1.f - m_power_increment_factor);
    else
        return value * m_power_decrement_factor + m_current_engine_power * (1.f - m_power_decrement_factor);
}
float CCar::DriveWheelsMeanAngleRate()
{
    xr_vector<SWheelDrive>::iterator i, e;
    i = m_driving_wheels.begin();
    e = m_driving_wheels.end();
    float drive_speed = 0.f;
    for (; i != e; ++i)
    {
        drive_speed += i->ASpeed();
        // if(wheel_speed<drive_speed)drive_speed=wheel_speed;
    }
    return drive_speed / m_driving_wheels.size();
}
float CCar::EngineDriveSpeed()
{
    // float wheel_speed,drive_speed=phInfinity;
    float calc_rpm = 0.f;
    if (b_transmission_switching)
    {
        calc_rpm = m_max_rpm;
        if (m_current_rpm > m_power_rpm)
        {
            b_transmission_switching = false;
        }
    }
    else
    {
        calc_rpm = EngineRpmFromWheels();

        if (!b_clutch && calc_rpm < m_min_rpm)
        {
            calc_rpm = m_min_rpm;
        }
        limit_above(calc_rpm, m_max_rpm);
    }
    if (calc_rpm > m_current_rpm)
        return (1.f - m_rpm_increment_factor) * m_current_rpm + m_rpm_increment_factor * calc_rpm;
    else
        return (1.f - m_rpm_decrement_factor) * m_current_rpm + m_rpm_decrement_factor * calc_rpm;

    // if(drive_speed<phInfinity) return dFabs(drive_speed*m_current_gear_ratio);
    // else					  return 0.f;
}

void CCar::UpdateFuel(float time_delta)
{
    if (!b_engine_on)
        return;
    if (m_current_rpm > m_min_rpm)
        m_fuel -= time_delta * (m_current_rpm - m_min_rpm) * m_fuel_consumption;
    else
        m_fuel -= time_delta * m_min_rpm * m_fuel_consumption;
    if (m_fuel < EPS)
        StopEngine();
}

float CCar::AddFuel(float ammount)
{
    float free_space = m_fuel_tank - m_fuel;
    if (ammount < free_space)
    {
        m_fuel += ammount;
        return ammount;
    }
    else
    {
        m_fuel = m_fuel_tank;
        return free_space;
    }
}

void CCar::ResetKeys()
{
    bkp = false;
    fwp = false;
    lsp = false;
    rsp = false;
}

#undef _USE_MATH_DEFINES

void CCar::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    CExplosive::OnEvent(P, type);

    //обработка сообщений, нужных для работы с багажником машины
    u16 id;
    switch (type)
    {
    case GE_OWNERSHIP_TAKE:
    {
        P.r_u16(id);
        IGameObject* O = Level().Objects.net_Find(id);
        if (GetInventory()->CanTakeItem(smart_cast<CInventoryItem*>(O)))
        {
            O->H_SetParent(this);
            GetInventory()->Take(smart_cast<CGameObject*>(O), false, false);
        }
        else
        {
            if (!O || !O->H_Parent() || (this != O->H_Parent()))
                return;
            NET_Packet P;
            u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
            P.w_u16(u16(O->ID()));
            u_EventSend(P);
        }
    }
    break;
    case GE_OWNERSHIP_REJECT:
    {
        P.r_u16(id);
        IGameObject* O = Level().Objects.net_Find(id);

        bool just_before_destroy = !P.r_eof() && P.r_u8();
        O->SetTmpPreDestroy(just_before_destroy);
        GetInventory()->DropItem(smart_cast<CGameObject*>(O), just_before_destroy, just_before_destroy);
        // if(GetInventory()->DropItem(smart_cast<CGameObject*>(O), just_before_destroy))
        //{
        //	O->H_SetParent(0, just_before_destroy);
        //}
        // moved to DropItem
    }
    break;
    }
}

void CCar::ResetScriptData(void* P) { CScriptEntity::ResetScriptData(P); }
void CCar::PhDataUpdate(float step)
{
    if (m_repairing)
        Revert();
    LimitWheels();
    UpdateFuel(step);

    // if(fwp)
    {
        UpdatePower();
        if (b_engine_on && !b_starting && m_current_rpm < m_min_rpm)
            Stall();
    }

    if (bkp)
    {
        UpdateBack();
    }

    if (brp)
        HandBreak();
    //////////////////////////////////////////////////////////
    for (int k = 0; k < (int)m_doors_update.size(); ++k)
    {
        SDoor* D = m_doors_update[k];
        if (!D->update)
        {
            m_doors_update.erase(m_doors_update.begin() + k);
            --k;
        }
        else
        {
            D->Update();
        }
    }

    m_steer_angle = m_steering_wheels.begin()->GetSteerAngle() * 0.1f + m_steer_angle * 0.9f;
    VERIFY(_valid(m_steer_angle));
}

BOOL CCar::UsedAI_Locations() { return (FALSE); }
u16 CCar::DriverAnimationType() { return m_driver_anim_type; }
void CCar::OnAfterExplosion() {}
void CCar::OnBeforeExplosion() { setEnabled(FALSE); }
void CCar::CarExplode()
{
    if (b_exploded)
        return;
    CPHSkeleton::SetNotNeedSave();
    if (m_car_weapon)
        m_car_weapon->Action(CCarWeapon::eWpnActivate, 0);
    m_lights.TurnOffHeadLights();
    b_exploded = true;
    CExplosive::GenExplodeEvent(Position(), Fvector().set(0.f, 1.f, 0.f));

    CActor* A = OwnerActor();
    if (A)
    {
        if (!m_doors.empty())
            m_doors.begin()->second.GetExitPosition(m_exit_position);
        else
            m_exit_position.set(Position());
        A->detach_Vehicle();
        if (A->g_Alive() <= 0.f)
            A->character_physics_support()->movement()->DestroyCharacter();
    }

    if (CPHDestroyable::CanDestroy())
        CPHDestroyable::Destroy(ID(), "physic_destroyable_object");
}
// void CCar::object_contactCallbackFun(bool& do_colide,dContact& c,SGameMtl * ,SGameMtl * )
//{
//
//	dxGeomUserData *l_pUD1 = NULL;
//	dxGeomUserData *l_pUD2 = NULL;
//	l_pUD1 = PHRetrieveGeomUserData(c.geom.g1);
//	l_pUD2 = PHRetrieveGeomUserData(c.geom.g2);
//
//	if(! l_pUD1) return;
//	if(!l_pUD2) return;
//
//	CEntityAlive* capturer=smart_cast<CEntityAlive*>(l_pUD1->ph_ref_object);
//	if(capturer)
//	{
//		CPHCapture* capture=capturer->m_PhysicMovementControl->PHCapture();
//		if(capture)
//		{
//			if(capture->m_taget_element->PhysicsRefObject()==l_pUD2->ph_ref_object)
//			{
//				do_colide = false;
//				capture->m_taget_element->Enable();
//				if(capture->e_state==CPHCapture::cstReleased) capture->ReleaseInCallBack();
//			}
//
//		}
//
//
//	}
//
//	capturer=smart_cast<CEntityAlive*>(l_pUD2->ph_ref_object);
//	if(capturer)
//	{
//		CPHCapture* capture=capturer->m_PhysicMovementControl->PHCapture();
//		if(capture)
//		{
//			if(capture->m_taget_element->PhysicsRefObject()==l_pUD1->ph_ref_object)
//			{
//				do_colide = false;
//				capture->m_taget_element->Enable();
//				if(capture->e_state==CPHCapture::cstReleased) capture->ReleaseInCallBack();
//			}
//
//		}
//
//	}
//}

template <class T>
IC void CCar::fill_wheel_vector(LPCSTR S, xr_vector<T>& type_wheels)
{
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    string64 S1;
    int count = _GetItemCount(S);
    for (int i = 0; i < count; ++i)
    {
        _GetItem(S, i, S1);

        u16 bone_id = pKinematics->LL_BoneID(S1);

        type_wheels.push_back(T());
        T& twheel = type_wheels.back();

        auto J = bone_map.find(bone_id);
        if (J == bone_map.end())
        {
            bone_map.insert(std::make_pair(bone_id, physicsBone()));

            SWheel& wheel = (m_wheels_map.insert(std::make_pair(bone_id, SWheel(this)))).first->second;
            wheel.bone_id = bone_id;
            twheel.pwheel = &wheel;
            wheel.Load(S1);
            twheel.Load(S1);
        }
        else
        {
            twheel.pwheel = &(m_wheels_map.find(bone_id))->second;
            twheel.Load(S1);
        }
    }
}

IC void CCar::fill_exhaust_vector(LPCSTR S, xr_vector<SExhaust>& exhausts)
{
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    string64 S1;
    int count = _GetItemCount(S);
    for (int i = 0; i < count; ++i)
    {
        _GetItem(S, i, S1);

        u16 bone_id = pKinematics->LL_BoneID(S1);

        exhausts.push_back(SExhaust(this));
        SExhaust& exhaust = exhausts.back();
        exhaust.bone_id = bone_id;

        auto J = bone_map.find(bone_id);
        if (J == bone_map.end())
            bone_map.insert(std::make_pair(bone_id, physicsBone()));
    }
}

IC void CCar::fill_doors_map(LPCSTR S, xr_map<u16, SDoor>& doors)
{
    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());
    string64 S1;
    int count = _GetItemCount(S);
    for (int i = 0; i < count; ++i)
    {
        _GetItem(S, i, S1);

        u16 bone_id = pKinematics->LL_BoneID(S1);
        SDoor door(this);
        door.bone_id = bone_id;
        doors.insert(std::make_pair(bone_id, door));
        auto J = bone_map.find(bone_id);
        if (J == bone_map.end())
            bone_map.insert(std::make_pair(bone_id, physicsBone()));
    }
}

IFactoryObject* CCar::_construct()
{
    inherited::_construct();
    CScriptEntity::_construct();
    return (this);
}

u16 CCar::Initiator()
{
    if (g_Alive() && Owner())
    {
        return Owner()->ID();
    }
    // else if(CExplosive::Initiator()!=u16(-1))return CExplosive::Initiator();
    else
        return ID();
}

float CCar::RefWheelMaxSpeed() const { return m_max_rpm / m_current_gear_ratio; }
float CCar::EngineCurTorque() const { return m_current_engine_power / m_current_rpm; }
float CCar::RefWheelCurTorque()
{
    if (b_transmission_switching)
        return 0.f;
    return EngineCurTorque() * ((m_current_gear_ratio < 0.f) ? -m_current_gear_ratio : m_current_gear_ratio);
}
void CCar::GetRayExplosionSourcePos(Fvector& pos) { random_point_in_object_box(pos, this); }
void CCar::net_Relcase(IGameObject* O)
{
    CExplosive::net_Relcase(O);
    inherited::net_Relcase(O);
    if (m_memory)
        m_memory->remove_links(O);
}

void CCar::ASCUpdate()
{
    for (u16 i = 0; i < cAsCallsnum; ++i)
    {
        EAsyncCalls c = EAsyncCalls(1 << i);
        if (async_calls.test(u16(c)))
            ASCUpdate(c);
    }
}

void CCar::ASCUpdate(EAsyncCalls c)
{
    async_calls.set(u16(c), FALSE);
    switch (c)
    {
    case ascSndTransmission: m_car_sound->TransmissionSwitch(); break;
    case ascSndStall: m_car_sound->Stop(); break;
    case ascExhoustStop: StopExhausts(); break;
    default: NODEFAULT;
    }
}

void CCar::AscCall(EAsyncCalls c) { async_calls.set(u16(c), TRUE); }
bool CCar::CanRemoveObject() { return CExplosive::IsExploded() && !CExplosive::IsSoundPlaying(); }
void CCar::SetExplodeTime(u32 et)
{
    CDelayedActionFuse::Initialize(float(et) / 1000.f, CDamagableItem::DamageLevelToHealth(2));
}
u32 CCar::ExplodeTime()
{
    if (CDelayedActionFuse::isInitialized())
        return u32(CDelayedActionFuse::Time()) * 1000;
    else
        return 0;
}

void CCar::Die(IGameObject* who)
{
    inherited::Die(who);
    CarExplode();
}

Fvector CCar::ExitVelocity()
{
    CPhysicsShell* P = PPhysicsShell();
    if (!P || !P->isActive())
        return Fvector().set(0, 0, 0);
    CPhysicsElement* E = P->get_ElementByStoreOrder(0);
    Fvector v = ExitPosition();
    E->GetPointVel(v, v);
    // dBodyGetPointVel(E->get_body(),v.x,v.y,v.z,cast_fp(v));
    return v;
}

/***** added by Ray Twitty (aka Shadows) START *****/
// получить и задать текущее количество топлива
float CCar::GetfFuel()
{
    return m_fuel;
}

void CCar::SetfFuel(float fuel)
{
    m_fuel = fuel;
}

// получить и задать размер топливного бака 
float CCar::GetfFuelTank()
{
    return m_fuel_tank;
}

void CCar::SetfFuelTank(float fuel_tank)
{
    m_fuel_tank = fuel_tank;
}

// получить и задать величину потребление топлива
float CCar::GetfFuelConsumption()
{
    return m_fuel_consumption;
}

void CCar::SetfFuelConsumption(float fuel_consumption)
{
    m_fuel_consumption = fuel_consumption;
}

// прибавить или убавить количество топлива
void CCar::ChangefFuel(float fuel)
{
    if(m_fuel + fuel < 0)
    {
        m_fuel = 0;
        return;
    }

    if(fuel < m_fuel_tank - m_fuel)
        m_fuel += fuel;
    else
        m_fuel = m_fuel_tank;
}

// прибавить или убавить жизней :)
void CCar::ChangefHealth(float health)
{
    float current_health = GetfHealth();
    if(current_health + health < 0)
    {
        SetfHealth(0);
        return;
    }

    if(health < 1 - current_health)
        SetfHealth(current_health + health);
	else
        SetfHealth(1);
}

// активен ли сейчас двигатель
bool CCar::isActiveEngine()
{
    return b_engine_on;
}
/***** added by Ray Twitty (aka Shadows) END *****/