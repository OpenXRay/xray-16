#include "StdAfx.h"
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "PhysicsShellHolder.h"
#include "xrMessages.h"
#include "object_factory.h"
#include "xrServer_Objects_ALife.h"
#include "Level.h"
#include "xrPhysics/PhysicsShell.h"
#include "Actor.h"
#include "CharacterPhysicsSupport.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "ai_space.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrNetServer/NET_Messages.h"

#include "xrPhysics/MathUtils.h"
#ifdef DEBUG
#include "xrPhysics/IPHWorld.h"
#endif

#include "Include/xrRender/Kinematics.h"
/*
[impulse_transition_to_parts]
random_min              =1       ; х массу объекта = величина случайно направленного импульса
; с случайн				о выбранной точкой приложения в пределах нового обекта
random_hit_imp         =0.1     ; х величена хит - импульса =............

;ref_bone                       ; кость из по которой определяется скорость для частей у который связь не задана по
умолчанию рут
imp_transition_factor  =0.1     ; фактор с которым прикладывается хит по исходному объекту ко всем частям
lv_transition_factor   =1       ; коэффициент передачи линейной скорости
av_transition_factor   =1       ; коэффициент передачи угловой скорости


[impulse_transition_from_source_bone]
source_bone            =0       ; ref_bone
imp_transition_factor  =1       ; коэффициент передачи импульса
lv_transition_factor   =1       ; коэффициент передачи линейной скорости
av_transition_factor   =1       ; коэффициент передачи угловой скорости

*/
CPHDestroyable::CPHDestroyable()
{
    m_flags.flags = 0;
    m_flags.set(fl_released, true);
    m_depended_objects = 0;
}
/////////spawn object representing destroyed
/// item//////////////////////////////////////////////////////////////////////////////////
void CPHDestroyable::GenSpawnReplace(u16 ref_id, LPCSTR section, shared_str visual_name)
{
    CSE_Abstract* D = F_entity_Create(section); //*cNameSect()
    VERIFY(D);
    CSE_Visual* V = smart_cast<CSE_Visual*>(D);
    V->set_visual(*visual_name);
    CSE_PHSkeleton* l_tpPHSkeleton = smart_cast<CSE_PHSkeleton*>(D);
    VERIFY(l_tpPHSkeleton);
    l_tpPHSkeleton->source_id = ref_id;
    // init

    // Send
    D->s_name = section; //*cNameSect()
    D->ID_Parent = u16(-1);
    InitServerObject(D);
    if (OnServer())
    {
        NET_Packet P;
        D->Spawn_Write(P, TRUE);
        Level().Send(P, net_flags(TRUE));
        // Destroy
        F_entity_Destroy(D);
        m_depended_objects++;
    };
};

void CPHDestroyable::InitServerObject(CSE_Abstract* D)
{
    CPhysicsShellHolder* obj = PPhysicsShellHolder();
    CSE_ALifeDynamicObjectVisual* l_tpALifeDynamicObject = smart_cast<CSE_ALifeDynamicObjectVisual*>(D);
    VERIFY(l_tpALifeDynamicObject);

    l_tpALifeDynamicObject->m_tGraphID = obj->ai_location().game_vertex_id();
    l_tpALifeDynamicObject->m_tNodeID = obj->ai_location().level_vertex_id();

    //	l_tpALifePhysicObject->startup_animation=m_startup_anim;

    D->set_name_replace("");
    //.	D->s_gameid			=	u8(GameID());
    D->s_RP = 0xff;
    D->ID = 0xffff;

    D->ID_Phantom = 0xffff;
    D->o_Position = obj->Position();
    if (ai().get_alife())
        l_tpALifeDynamicObject->m_tGraphID = ai().game_graph().current_level_vertex();
    else
        l_tpALifeDynamicObject->m_tGraphID = 0xffff;
    obj->XFORM().getXYZ(D->o_Angle);
    D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
    D->RespawnTime = 0;
}

void CPHDestroyable::PhysicallyRemoveSelf()
{
    CPhysicsShellHolder* obj = PPhysicsShellHolder();

    CActor* A = smart_cast<CActor*>(obj);
    if (A)
    {
        A->character_physics_support()->SetRemoved();
    }
    else
    {
        // obj->PPhysicsShell()->PureStep();
        obj->PPhysicsShell()->Disable();
        obj->PPhysicsShell()->DisableCollision();
    }

    obj->setVisible(FALSE);
    obj->setEnabled(FALSE);
}

void CPHDestroyable::PhysicallyRemovePart(CPHDestroyableNotificate* dn)
{
    CPhysicsShellHolder* sh = dn->PPhysicsShellHolder();
    CPhysicsShell* s = sh->PPhysicsShell();
    sh->setVisible(FALSE);
    sh->setEnabled(FALSE);
    s->Disable();
    s->DisableCollision();
}

void CPHDestroyable::Destroy(u16 source_id /*=u16(-1)*/, LPCSTR section /*="ph_skeleton_object"*/)
{
    if (!CanDestroy())
        return;
    m_notificate_objects.clear();
    CPhysicsShellHolder* obj = PPhysicsShellHolder();
    CPHSkeleton* phs = obj->PHSkeleton();
    if (phs)
        phs->SetNotNeedSave();
    if (obj->PPhysicsShell())
        obj->PPhysicsShell()->Enable();
    obj->processing_activate();
    if (source_id == obj->ID())
    {
        m_flags.set(fl_released, false);
    }
    xr_vector<shared_str>::iterator i = m_destroyed_obj_visual_names.begin(), e = m_destroyed_obj_visual_names.end();

    if (IsGameTypeSingle())
    {
        for (; e != i; i++)
            GenSpawnReplace(source_id, section, *i);
    };
    ///////////////////////////////////////////////////////////////////////////
    m_flags.set(fl_destroyed, true);
}

void CPHDestroyable::Load(CInifile* ini, LPCSTR section)
{
    m_flags.set(fl_destroyable, false);
    if (ini->line_exist(section, "destroyed_vis_name"))
    {
        m_flags.set(fl_destroyable, true);
        m_destroyed_obj_visual_names.push_back(ini->r_string(section, "destroyed_vis_name"));
    }
    else
    {
        CInifile::Sect& data = ini->r_section(section);
        if (data.Data.size() > 0)
            m_flags.set(fl_destroyable, true);
        for (auto I = data.Data.cbegin(); I != data.Data.cend(); I++)
            if (I->first.size())
                m_destroyed_obj_visual_names.push_back(I->first);
    }
}
void CPHDestroyable::Load(LPCSTR section)
{
    m_flags.set(fl_destroyable, false);

    if (pSettings->line_exist(section, "destroyed_vis_name"))
    {
        m_flags.set(fl_destroyable, true);
        m_destroyed_obj_visual_names.push_back(pSettings->r_string(section, "destroyed_vis_name"));
    }
}

void CPHDestroyable::Init() { m_depended_objects = 0; }
void CPHDestroyable::RespawnInit()
{
    m_flags.set(fl_destroyed, false);
    m_flags.set(fl_released, true);
    m_destroyed_obj_visual_names.clear();
    m_notificate_objects.clear();
    m_depended_objects = 0;
}
void CPHDestroyable::SheduleUpdate(u32 dt)
{
    if (!m_flags.test(fl_destroyed) || !m_flags.test(fl_released))
        return;
    CPhysicsShellHolder* obj = PPhysicsShellHolder();

    if (CanRemoveObject())
    {
        if (obj->Local())
            obj->DestroyObject();
    }
}

void CPHDestroyable::NotificatePart(CPHDestroyableNotificate* dn)
{
    CPhysicsShell* own_shell = PPhysicsShellHolder()->PPhysicsShell();
    CPhysicsShell* new_shell = dn->PPhysicsShellHolder()->PPhysicsShell();
    IKinematics* own_K = smart_cast<IKinematics*>(PPhysicsShellHolder()->Visual());
    IKinematics* new_K = smart_cast<IKinematics*>(dn->PPhysicsShellHolder()->Visual());
    VERIFY(own_K && new_K && own_shell && new_shell);
    CInifile* own_ini = own_K->LL_UserData();
    CInifile* new_ini = new_K->LL_UserData();
    //////////////////////////////////////////////////////////////////////////////////
    Fmatrix own_transform;
    own_shell->GetGlobalTransformDynamic(&own_transform);
    new_shell->SetGlTransformDynamic(own_transform);
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////
    float random_min = 1.f;
    float random_hit_imp = 1.f;
    ////////////////////////////////////////////////////////////////////////////////////
    u16 ref_bone = own_K->LL_GetBoneRoot();

    float imp_transition_factor = 1.f;
    float lv_transition_factor = 1.f;
    float av_transition_factor = 1.f;
    ////////////////////////////////////////////////////////////////////////////////////
    if (own_ini && own_ini->section_exist("impulse_transition_to_parts"))
    {
        random_min = own_ini->r_float("impulse_transition_to_parts", "random_min");
        random_hit_imp = own_ini->r_float("impulse_transition_to_parts", "random_hit_imp");
        ////////////////////////////////////////////////////////
        if (own_ini->line_exist("impulse_transition_to_parts", "ref_bone"))
            ref_bone = own_K->LL_BoneID(own_ini->r_string("impulse_transition_to_parts", "ref_bone"));
        imp_transition_factor = own_ini->r_float("impulse_transition_to_parts", "imp_transition_factor");
        lv_transition_factor = own_ini->r_float("impulse_transition_to_parts", "lv_transition_factor");
        av_transition_factor = own_ini->r_float("impulse_transition_to_parts", "av_transition_factor");

        if (own_ini->section_exist("collide_parts"))
        {
            if (own_ini->line_exist("collide_parts", "small_object"))
            {
                new_shell->SetSmall();
            }
            if (own_ini->line_exist("collide_parts", "ignore_small_objects"))
            {
                new_shell->SetIgnoreSmall();
            }
        }
    }

    if (new_ini && new_ini->section_exist("impulse_transition_from_source_bone"))
    {
        // random_min				=new_ini->r_float("impulse_transition_from_source_bone","random_min");
        // random_hit_imp			=new_ini->r_float("impulse_transition_from_source_bone","random_hit_imp");
        ////////////////////////////////////////////////////////
        if (new_ini->line_exist("impulse_transition_from_source_bone", "ref_bone"))
            ref_bone = own_K->LL_BoneID(new_ini->r_string("impulse_transition_from_source_bone", "ref_bone"));
        imp_transition_factor = new_ini->r_float("impulse_transition_from_source_bone", "imp_transition_factor");
        lv_transition_factor = new_ini->r_float("impulse_transition_from_source_bone", "lv_transition_factor");
        av_transition_factor = new_ini->r_float("impulse_transition_from_source_bone", "av_transition_factor");
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // dBodyID own_body=own_shell->get_Element(ref_bone)->get_body()			;
    CPhysicsElement* own_element = own_shell->get_Element(ref_bone);
    u16 new_el_number = new_shell->get_ElementsNumber();

    for (u16 i = 0; i < new_el_number; ++i)
    {
        CPhysicsElement* e = new_shell->get_ElementByStoreOrder(i);
        float random_hit = random_min * e->getMass();
        if (m_fatal_hit.is_valide() && m_fatal_hit.bone() != BI_NONE)
        {
            Fvector pos;
            Fmatrix m;
            m.set(own_K->LL_GetTransform(m_fatal_hit.bone()));
            m.mulA_43(PPhysicsShellHolder()->XFORM());
            m.transform_tiny(pos, m_fatal_hit.bone_space_position());
            e->applyImpulseVsGF(pos, m_fatal_hit.direction(), m_fatal_hit.phys_impulse() * imp_transition_factor);
            random_hit += random_hit_imp * m_fatal_hit.phys_impulse();
        }
        Fvector rnd_dir;
        rnd_dir.random_dir();
        e->applyImpulse(rnd_dir, random_hit);
        Fvector mc;
        mc.set(e->mass_Center());

        // dVector3 res_lvell;
        // dBodyGetPointVel(own_body,mc.x,mc.y,mc.z,res_lvell);
        Fvector res_lvell;
        own_element->GetPointVel(res_lvell, mc);

        res_lvell.mul(lv_transition_factor);
        e->set_LinearVel(res_lvell);

        // Fvector res_avell;res_avell.set(cast_fv(dBodyGetAngularVel(own_body)));
        Fvector res_avell;
        own_element->get_AngularVel(res_avell);
        res_avell.mul(av_transition_factor);
        e->set_AngularVel(res_avell);
    }

    new_shell->Enable();
    new_shell->EnableCollision();
    dn->PPhysicsShellHolder()->setVisible(TRUE);
    dn->PPhysicsShellHolder()->setEnabled(TRUE);

    if (own_shell->IsGroupObject())
        new_shell->RegisterToCLGroup(own_shell->GetCLGroup()); // CollideBits
    CPHSkeleton* ps = dn->PPhysicsShellHolder()->PHSkeleton();
    if (ps)
    {
        if (own_ini && own_ini->section_exist("autoremove_parts"))
        {
            ps->SetAutoRemove(
                1000 * (READ_IF_EXISTS(own_ini, r_u32, "autoremove_parts", "time", ps->DefaultExitenceTime())));
        }

        if (new_ini && new_ini->section_exist("autoremove"))
        {
            ps->SetAutoRemove(1000 * (READ_IF_EXISTS(new_ini, r_u32, "autoremove", "time", ps->DefaultExitenceTime())));
        }
    }
}

void CPHDestroyable::NotificateDestroy(CPHDestroyableNotificate* dn)
{
    VERIFY(m_depended_objects);
    VERIFY(!physics_world()->Processing());
    m_depended_objects--;
    PhysicallyRemovePart(dn);
    m_notificate_objects.push_back(dn);
    if (!m_depended_objects)
    {
        xr_vector<CPHDestroyableNotificate *>::iterator i = m_notificate_objects.begin(),
                                                        e = m_notificate_objects.end();
        for (; i < e; i++)
            NotificatePart(*i);
        PhysicallyRemoveSelf();
        m_notificate_objects.clear();
        m_flags.set(fl_released, true);
    }
}

void CPHDestroyable::SetFatalHit(const SHit& hit) { m_fatal_hit = hit; }
