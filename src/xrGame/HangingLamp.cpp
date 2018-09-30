#include "pch_script.h"
#include "HangingLamp.h"
#include "xrEngine/LightAnimLibrary.h"
#include "xrEngine/xr_collide_form.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/MathUtils.h"
#include "xrServer_Objects_ALife.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHangingLamp::CHangingLamp() { Init(); }
CHangingLamp::~CHangingLamp() {}
void CHangingLamp::Init()
{
    fHealth = 100.f;
    light_bone = BI_NONE;
    ambient_bone = BI_NONE;
    lanim = 0;
    ambient_power = 0.f;
    light_render = 0;
    light_ambient = 0;
    glow_render = 0;
    m_bState = 1;
}

void CHangingLamp::RespawnInit()
{
    Init();
    if (Visual())
    {
        IKinematics* K = smart_cast<IKinematics*>(Visual());
        K->LL_SetBonesVisible(u64(-1));
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
    }
}

void CHangingLamp::Center(Fvector& C) const
{
    if (renderable.visual)
    {
        renderable.xform.transform_tiny(C, renderable.visual->getVisData().sphere.P);
    }
    else
    {
        C.set(XFORM().c);
    }
}

float CHangingLamp::Radius() const { return (renderable.visual) ? renderable.visual->getVisData().sphere.R : EPS; }
void CHangingLamp::Load(LPCSTR section) { inherited::Load(section); }
void CHangingLamp::net_Destroy()
{
    light_render.destroy();
    light_ambient.destroy();
    glow_render.destroy();
    RespawnInit();
    if (Visual())
        CPHSkeleton::RespawnInit();
    inherited::net_Destroy();
}

BOOL CHangingLamp::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeObjectHangingLamp* lamp = smart_cast<CSE_ALifeObjectHangingLamp*>(e);
    R_ASSERT(lamp);
    inherited::net_Spawn(DC);
    Fcolor clr;

    // set bone id
    //	CInifile* pUserData		= K->LL_UserData();
    //	R_ASSERT3				(pUserData,"Empty HangingLamp user data!",lamp->get_visual());
    xr_delete(CForm);
    if (Visual())
    {
        IKinematics* K = smart_cast<IKinematics*>(Visual());
        R_ASSERT(Visual() && smart_cast<IKinematics*>(Visual()));
        light_bone = K->LL_BoneID(*lamp->light_main_bone);
        VERIFY(light_bone != BI_NONE);
        ambient_bone = K->LL_BoneID(*lamp->light_ambient_bone);
        VERIFY(ambient_bone != BI_NONE);
        CForm = new CCF_Skeleton(this);
    }
    fBrightness = lamp->brightness;
    clr.set(lamp->color);
    clr.a = 1.f;
    clr.mul_rgb(fBrightness);

    light_render = GEnv.Render->light_create();
    light_render->set_shadow(!!lamp->flags.is(CSE_ALifeObjectHangingLamp::flCastShadow));
    light_render->set_volumetric(!!lamp->flags.is(CSE_ALifeObjectHangingLamp::flVolumetric));
    light_render->set_type(
        lamp->flags.is(CSE_ALifeObjectHangingLamp::flTypeSpot) ? IRender_Light::SPOT : IRender_Light::POINT);
    light_render->set_range(lamp->range);
    light_render->set_virtual_size(lamp->m_virtual_size);
    light_render->set_color(clr);
    light_render->set_cone(lamp->spot_cone_angle);
    light_render->set_texture(*lamp->light_texture);

    light_render->set_volumetric_quality(lamp->m_volumetric_quality);
    light_render->set_volumetric_intensity(lamp->m_volumetric_intensity);
    light_render->set_volumetric_distance(lamp->m_volumetric_distance);

    if (lamp->glow_texture.size())
    {
        glow_render = GEnv.Render->glow_create();
        glow_render->set_texture(*lamp->glow_texture);
        glow_render->set_color(clr);
        glow_render->set_radius(lamp->glow_radius);
    }

    if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPointAmbient))
    {
        ambient_power = lamp->m_ambient_power;
        light_ambient = GEnv.Render->light_create();
        light_ambient->set_type(IRender_Light::POINT);
        light_ambient->set_shadow(false);
        clr.mul_rgb(ambient_power);
        light_ambient->set_range(lamp->m_ambient_radius);
        light_ambient->set_color(clr);
        light_ambient->set_texture(*lamp->m_ambient_texture);
    }

    fHealth = lamp->m_health;

    lanim = LALib.FindItem(*lamp->color_animator);

    CPHSkeleton::Spawn(e);
    if (smart_cast<IKinematicsAnimated*>(Visual()))
        smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle("idle");
    if (smart_cast<IKinematics*>(Visual()))
    {
        smart_cast<IKinematics*>(Visual())->CalculateBones_Invalidate();
        smart_cast<IKinematics*>(Visual())->CalculateBones(TRUE);
        //.intepolate_pos
    }
    if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic) && !Visual())
        Msg("! WARNING: lamp, obj name [%s],flag physics set, but has no visual", *cName());
    //.	if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic)&&Visual()&&!guid_physic_bone)	fHealth=0.f;
    if (Alive() && m_bState)
        TurnOn();
    else
    {
        processing_activate(); // temporal enable
        TurnOff(); // -> and here is disable :)
    }

    setVisible((BOOL) !!Visual());
    setEnabled((BOOL) !!CForm);

    return (TRUE);
}

void CHangingLamp::SpawnInitPhysics(CSE_Abstract* D)
{
    CSE_ALifeObjectHangingLamp* lamp = smart_cast<CSE_ALifeObjectHangingLamp*>(D);
    if (lamp->flags.is(CSE_ALifeObjectHangingLamp::flPhysic))
        CreateBody(lamp);
    if (smart_cast<IKinematics*>(Visual()))
    {
        smart_cast<IKinematics*>(Visual())->CalculateBones_Invalidate();
        smart_cast<IKinematics*>(Visual())->CalculateBones(TRUE);
        //.intepolate_pos
    }
}

void CHangingLamp::CopySpawnInit()
{
    CPHSkeleton::CopySpawnInit();
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    if (!K->LL_GetBoneVisible(light_bone))
        TurnOff();
}
void CHangingLamp::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    CPHSkeleton::SaveNetState(P);
}

BOOL CHangingLamp::net_SaveRelevant() { return (TRUE); }
void CHangingLamp::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    output_packet.w_u8((u8)m_bState);
}
void CHangingLamp::load(IReader& input_packet)
{
    inherited::load(input_packet);
    m_bState = (u8)input_packet.r_u8();
}
void CHangingLamp::shedule_Update(u32 dt)
{
    CPHSkeleton::Update(dt);

    inherited::shedule_Update(dt);
}

void CHangingLamp::UpdateCL()
{
    inherited::UpdateCL();

    if (m_pPhysicsShell)
        m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());

    if (Alive() && light_render->get_active())
    {
        if (Visual())
            PKinematics(Visual())->CalculateBones();

        // update T&R from light (main) bone
        Fmatrix xf;
        if (light_bone != BI_NONE)
        {
            Fmatrix& M = smart_cast<IKinematics*>(Visual())->LL_GetTransform(light_bone);
            xf.mul(XFORM(), M);
            VERIFY(!fis_zero(DET(xf)));
        }
        else
        {
            xf.set(XFORM());
        }
        light_render->set_rotation(xf.k, xf.i);
        light_render->set_position(xf.c);
        if (glow_render)
            glow_render->set_position(xf.c);

        // update T&R from ambient bone
        if (light_ambient)
        {
            if (ambient_bone != light_bone)
            {
                if (ambient_bone != BI_NONE)
                {
                    Fmatrix& M = smart_cast<IKinematics*>(Visual())->LL_GetTransform(ambient_bone);
                    xf.mul(XFORM(), M);
                    VERIFY(!fis_zero(DET(xf)));
                }
                else
                {
                    xf.set(XFORM());
                }
            }
            light_ambient->set_rotation(xf.k, xf.i);
            light_ambient->set_position(xf.c);
        }

        if (lanim)
        {
            int frame;
            u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame); // возвращает в формате BGR
            Fcolor fclr;
            fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
            fclr.mul_rgb(fBrightness / 255.f);
            light_render->set_color(fclr);
            if (glow_render)
                glow_render->set_color(fclr);
            if (light_ambient)
            {
                fclr.mul_rgb(ambient_power);
                light_ambient->set_color(fclr);
            }
        }
    }
}

void CHangingLamp::TurnOn()
{
    if (!Alive())
        return;

    Fvector p = XFORM().c;
    light_render->set_position(p);
    light_render->set_active(true);
    if (glow_render)
    {
        glow_render->set_position(p);
        glow_render->set_active(true);
    }
    if (light_ambient)
    {
        light_ambient->set_position(p);
        light_ambient->set_active(true);
    }
    if (Visual())
    {
        IKinematics* K = smart_cast<IKinematics*>(Visual());
        K->LL_SetBoneVisible(light_bone, TRUE, TRUE);
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
        K->LL_SetBoneVisible(light_bone, TRUE, TRUE); // hack
    }
    processing_activate();
    m_bState = 1;
}

void CHangingLamp::TurnOff()
{
    if (!m_bState)
        return;

    light_render->set_active(false);
    if (glow_render)
        glow_render->set_active(false);
    if (light_ambient)
        light_ambient->set_active(false);
    if (Visual())
    {
        IKinematics* K = smart_cast<IKinematics*>(Visual());
        VERIFY(K);
        K->LL_SetBoneVisible(light_bone, FALSE, TRUE);
        VERIFY2(K->LL_GetBonesVisible() != 0,
            make_string("can not Turn Off lamp: %s, visual %s - because all bones become invisible",
                cNameVisual().c_str(), cName().c_str()));
    }
    processing_deactivate();
    m_bState = 0;
}

// void CHangingLamp::Hit(float P,Fvector &dir, IGameObject* who,s16 element,
//					   Fvector p_in_object_space, float impulse, ALife::EHitType hit_type)
void CHangingLamp::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    callback(GameObject::eHit)(
        lua_game_object(), HDS.power, HDS.dir, smart_cast<const CGameObject*>(HDS.who)->lua_game_object(), HDS.bone());
    BOOL bWasAlive = Alive();

    if (m_pPhysicsShell)
        m_pPhysicsShell->applyHit(pHDS->p_in_bone_space, pHDS->dir, pHDS->impulse, pHDS->boneID, pHDS->hit_type);

    if (pHDS->boneID == light_bone)
        fHealth = 0.f;
    else
        fHealth -= pHDS->damage() * 100.f;

    if (bWasAlive && (!Alive()))
        TurnOff();
}

static BONE_P_MAP bone_map = BONE_P_MAP();
void CHangingLamp::CreateBody(CSE_ALifeObjectHangingLamp* lamp)
{
    if (!Visual())
        return;
    if (m_pPhysicsShell)
        return;

    IKinematics* pKinematics = smart_cast<IKinematics*>(Visual());

    m_pPhysicsShell = P_create_Shell();

    bone_map.clear();
    LPCSTR fixed_bones = *lamp->fixed_bones;
    if (fixed_bones)
    {
        int count = _GetItemCount(fixed_bones);
        for (int i = 0; i < count; ++i)
        {
            string64 fixed_bone;
            _GetItem(fixed_bones, i, fixed_bone);
            u16 fixed_bone_id = pKinematics->LL_BoneID(fixed_bone);
            R_ASSERT2(BI_NONE != fixed_bone_id, "wrong fixed bone");
            bone_map.insert(std::make_pair(fixed_bone_id, physicsBone()));
        }
    }
    else
    {
        bone_map.insert(std::make_pair(pKinematics->LL_GetBoneRoot(), physicsBone()));
    }

    phys_shell_verify_object_model(*this);

    m_pPhysicsShell->build_FromKinematics(pKinematics, &bone_map);
    m_pPhysicsShell->set_PhysicsRefObject(this);
    m_pPhysicsShell->mXFORM.set(XFORM());
    m_pPhysicsShell->Activate(true); //,
    // m_pPhysicsShell->SmoothElementsInertia(0.3f);
    m_pPhysicsShell->SetAirResistance(); // 0.0014f,1.5f

    /////////////////////////////////////////////////////////////////////////////
    auto i = bone_map.begin(), e = bone_map.end();
    for (; i != e; i++)
    {
        CPhysicsElement* fixed_element = i->second.element;
        /// R_ASSERT2(fixed_element,"fixed bone has no physics");
        if (fixed_element)
            fixed_element->Fix();
    }

    m_pPhysicsShell->mXFORM.set(XFORM());
    m_pPhysicsShell->SetAirResistance(0.001f, 0.02f);
    SAllDDOParams disable_params;
    disable_params.Load(smart_cast<IKinematics*>(Visual())->LL_UserData());
    m_pPhysicsShell->set_DisableParams(disable_params);
    ApplySpawnIniToPhysicShell(&lamp->spawn_ini(), m_pPhysicsShell, fixed_bones[0] != '\0');
}

void CHangingLamp::net_Export(NET_Packet& P) { VERIFY(Local()); }
void CHangingLamp::net_Import(NET_Packet& P) { VERIFY(Remote()); }
BOOL CHangingLamp::UsedAI_Locations() { return (FALSE); }
SCRIPT_EXPORT(CHangingLamp, (CGameObject), {
    luabind::module(luaState)[luabind::class_<CHangingLamp, CGameObject>("hanging_lamp")
                                  .def(luabind::constructor<>())
                                  .def("turn_on", &CHangingLamp::TurnOn)
                                  .def("turn_off", &CHangingLamp::TurnOff)];
});
