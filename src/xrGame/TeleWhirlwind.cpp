#include "StdAfx.h"
#include "TeleWhirlwind.h"
#include "xrPhysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "Level.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "xrMessages.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"

CTeleWhirlwind::CTeleWhirlwind()
{
    m_owner_object = NULL;
    m_center.set(0.f, 0.f, 0.f);
    m_keep_radius = 1.f;
    m_throw_power = 100.f;
}

CTelekineticObject* CTeleWhirlwind::activate(
    CPhysicsShellHolder* obj, float strength, float height, u32 max_time_keep, bool rot)
{
    if (inherited::activate(obj, strength, height, max_time_keep, rot))
    {
        CTeleWhirlwindObject* o = smart_cast<CTeleWhirlwindObject*>(objects.back());
        VERIFY(o);
        o->set_throw_power(m_throw_power);
        return o;
    }
    else
        return 0;
}
void CTeleWhirlwind::clear_impacts() { m_saved_impacts.clear(); }
void CTeleWhirlwind::clear() { inherited::clear(); }
void CTeleWhirlwind::add_impact(const Fvector& dir, float val)
{
    Fvector force, point;
    force.set(dir);
    force.mul(val);
    point.set(0.f, 0.f, 0.f);
    m_saved_impacts.push_back(SPHImpact(force, point, 0));
}
void CTeleWhirlwind::set_throw_power(float throw_pow) { m_throw_power = throw_pow; }
void CTeleWhirlwind::draw_out_impact(Fvector& dir, float& val)
{
    VERIFY2(m_saved_impacts.size(), "NO IMPACTS ADDED!");

    if (m_saved_impacts.empty())
        return;

    dir.set(m_saved_impacts[0].force);
    val = dir.magnitude();

    // Swartz
    //if (!fis_zero(val))
    //    dir.mul(1.f / val);
    m_saved_impacts.erase(m_saved_impacts.begin());
}

static bool RemovePred(CTelekineticObject* tele_object)
{
    return (!tele_object->get_object() || tele_object->get_object()->getDestroy());
}

void CTeleWhirlwind::clear_notrelevant()
{
    //убрать все объеты со старыми параметрами
    objects.erase(std::remove_if(objects.begin(), objects.end(), &RemovePred), objects.end());
}

void CTeleWhirlwind::play_destroy(CTeleWhirlwindObject* obj) {}
CTeleWhirlwindObject::CTeleWhirlwindObject()
{
    m_telekinesis = 0;
    throw_power = 0.f;
}

bool CTeleWhirlwindObject::init(CTelekinesis* tele, CPhysicsShellHolder* obj, float s, float h, u32 ttk, bool rot)
{
    bool result = inherited::init(tele, obj, s, h, ttk, rot);
    m_telekinesis = static_cast<CTeleWhirlwind*>(tele);

    throw_power = strength;
    if (m_telekinesis->is_active_object(obj))
    {
        return false;
    }
    if (obj->PPhysicsShell())
    {
        obj->PPhysicsShell()->SetAirResistance(0.f, 0.f);
        obj->m_pPhysicsShell->set_ApplyByGravity(TRUE);
    }

    if (object->ph_destroyable() && object->ph_destroyable()->CanDestroy())
        b_destroyable = true;
    else
        b_destroyable = false;

    return result;
}
void CTeleWhirlwindObject::raise_update()
{
    // u32 time=Device.dwTimeGlobal;
    //	if (time_raise_started + 100000 < time) release();
}

void CTeleWhirlwindObject::release()
{
    if (!object || object->getDestroy() || !object->m_pPhysicsShell || !object->m_pPhysicsShell->isActive())
        return;

    Fvector dir_inv;
    dir_inv.sub(object->Position(), m_telekinesis->Center());
    float magnitude = dir_inv.magnitude();

    // включить гравитацию
    // Fvector zer;zer.set(0,0,0);
    // object->m_pPhysicsShell->set_LinearVel(zer);
    object->m_pPhysicsShell->set_ApplyByGravity(TRUE);
    /////////////////////////////////////
    float impulse = 0.f;
    if (magnitude > 0.2f)
    {
        dir_inv.mul(1.f / magnitude);
        impulse = throw_power / magnitude / magnitude;
    }
    else
    {
        dir_inv.random_dir();
        impulse = throw_power * 100.f;
    }
    /////////////////////////////////////////////////
    bool b_destroyed = false;
    if (magnitude < 2.f * object->Radius())
    {
        b_destroyed = destroy_object(dir_inv, throw_power * 100.f);
    }

    if (!b_destroyed)
        object->m_pPhysicsShell->applyImpulse(dir_inv, impulse);
    switch_state(TS_None);
}

bool CTeleWhirlwindObject::destroy_object(const Fvector dir, float val)
{
    CPHDestroyable* D = object->ph_destroyable();
    if (D)
    {
        D->PhysicallyRemoveSelf();
        D->Destroy(m_telekinesis->OwnerObject()->ID());

        //.		m_telekinesis->add_impact(dir,val*10.f);

        xr_vector<shared_str>::iterator i = D->m_destroyed_obj_visual_names.begin();
        xr_vector<shared_str>::iterator e = D->m_destroyed_obj_visual_names.end();
        if (IsGameTypeSingle())
        {
            for (; e != i; i++)
                m_telekinesis->add_impact(dir, val * 10.f);
        };

        CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(object);
        if (PP)
        {
            u16 root = (smart_cast<IKinematics*>(object->Visual()))->LL_GetBoneRoot();
            PP->StartParticles(
                m_telekinesis->destroing_particles(), root, Fvector().set(0, 1, 0), m_telekinesis->OwnerObject()->ID());
        }
        return true;
    }
    return false;
}

void CTeleWhirlwindObject::raise(float step)
{
    CPhysicsShell* p = get_object()->PPhysicsShell();

    if (!p || !p->isActive())
        return;
    else
    {
        p->SetAirResistance(0.f, 0.f);
        p->set_ApplyByGravity(TRUE);
    }
    u16 element_number = p->get_ElementsNumber();
    Fvector center = m_telekinesis->Center();
    CPhysicsElement* maxE = p->get_ElementByStoreOrder(0);
    for (u16 element = 0; element < element_number; ++element)
    {
        float k = strength; // 600.f;
        float predict_v_eps = 0.1f;
        float mag_eps = .01f;

        CPhysicsElement* E = p->get_ElementByStoreOrder(element);
        if (maxE->getMass() < E->getMass())
            maxE = E;
        if (!E->isActive())
            continue;
        Fvector pos = E->mass_Center();

        Fvector diff;
        diff.sub(center, pos);
        float mag = _sqrt(diff.x * diff.x + diff.z * diff.z);
        Fvector lc;
        lc.set(center);
        if (mag > 1.f)
        {
            lc.y /= mag;
        }
        diff.sub(lc, pos);
        mag = diff.magnitude();
        float accel = k / mag / mag / mag; //*E->getMass()
        Fvector dir;
        if (mag < mag_eps)
        {
            accel = 0.f;
            // Fvector zer;zer.set(0,0,0);
            // E->set_LinearVel(zer);
            dir.random_dir();
        }
        else
        {
            dir.set(diff);
            dir.mul(1.f / mag);
        }
        Fvector vel;
        E->get_LinearVel(vel);
        float delta_v = accel * fixed_step;
        Fvector delta_vel;
        delta_vel.set(dir);
        delta_vel.mul(delta_v);
        Fvector predict_vel;
        predict_vel.add(vel, delta_vel);
        Fvector delta_pos;
        delta_pos.set(predict_vel);
        delta_pos.mul(fixed_step);
        Fvector predict_pos;
        predict_pos.add(pos, delta_pos);

        Fvector predict_diff;
        predict_diff.sub(lc, predict_pos);
        float predict_mag = predict_diff.magnitude();
        float predict_v = predict_vel.magnitude();

        Fvector force;
        force.set(dir);
        if (predict_mag > mag && predict_vel.dotproduct(dir) > 0.f && predict_v > predict_v_eps)
        {
            Fvector motion_dir;
            motion_dir.set(predict_vel);
            motion_dir.mul(1.f / predict_v);
            float needed_d = diff.dotproduct(motion_dir);
            Fvector needed_diff;
            needed_diff.set(motion_dir);
            needed_diff.mul(needed_d);
            Fvector nearest_p;
            nearest_p.add(pos, needed_diff); //
            Fvector needed_vel;
            needed_vel.set(needed_diff);
            needed_vel.mul(1.f / fixed_step);
            force.sub(needed_vel, vel);
            force.mul(E->getMass() / fixed_step);
        }
        else
        {
            force.mul(accel * E->getMass());
        }

        E->applyForce(force.x, force.y + get_object()->EffectiveGravity() * E->getMass(), force.z);
    }
    Fvector dist;
    dist.sub(center, maxE->mass_Center());
    if (dist.magnitude() < m_telekinesis->keep_radius() && b_destroyable)
    {
        p->setTorque(Fvector().set(0, 0, 0));
        p->setForce(Fvector().set(0, 0, 0));
        p->set_LinearVel(Fvector().set(0, 0, 0));
        p->set_AngularVel(Fvector().set(0, 0, 0));
        switch_state(TS_Keep);
    }
}

void CTeleWhirlwindObject::keep()
{
    CPhysicsShell* p = get_object()->PPhysicsShell();
    if (!p || !p->isActive())
        return;
    else
    {
        p->SetAirResistance(0.f, 0.f);
        p->set_ApplyByGravity(FALSE);
    }

    u16 element_number = p->get_ElementsNumber();
    Fvector center = m_telekinesis->Center();

    CPhysicsElement* maxE = p->get_ElementByStoreOrder(0);
    for (u16 element = 0; element < element_number; ++element)
    {
        CPhysicsElement* E = p->get_ElementByStoreOrder(element);
        if (maxE->getMass() < E->getMass())
            maxE = E;
        Fvector dir;
        dir.sub(center, E->mass_Center());
        dir.normalize_safe();
        Fvector vel;
        E->get_LinearVel(vel);
        float force = dir.dotproduct(vel) * E->getMass() / 2.f;
        if (force < 0.f)
        {
            dir.mul(force);
        }
    }

    maxE->setTorque(Fvector().set(0, 500.f, 0));

    Fvector dist;
    dist.sub(center, maxE->mass_Center());
    if (dist.magnitude() > m_telekinesis->keep_radius() * 1.5f)
    {
        p->setTorque(Fvector().set(0, 0, 0));
        p->setForce(Fvector().set(0, 0, 0));
        p->set_LinearVel(Fvector().set(0, 0, 0));
        p->set_AngularVel(Fvector().set(0, 0, 0));
        p->set_ApplyByGravity(TRUE);
        switch_state(TS_Raise);
    }
}
void CTeleWhirlwindObject::fire(const Fvector& target)
{
    // inherited::fire(target);
}
void CTeleWhirlwindObject::fire(const Fvector& target, float power)
{
    // inherited:: fire(target,power);
}

void CTeleWhirlwindObject::set_throw_power(float throw_pow) { throw_power = throw_pow; }
void CTeleWhirlwindObject::switch_state(ETelekineticState new_state) { inherited::switch_state(new_state); }
bool CTeleWhirlwindObject::can_activate(CPhysicsShellHolder* obj) { return (obj != NULL); }
