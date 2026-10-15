#include "StdAfx.h"
#include "PHActivationShape.h"
#include "Physics.h"
#include "xrServerEntities/PHSynchronize.h"
#include "xrServerEntities/PHNetState.h"

void RestoreVelocityState(V_PH_WORLD_STATE& state)
{
    for (auto& entry : state) {
        SPHNetState current;
        entry.first->get_State(current);
        current.angular_vel = entry.second.angular_vel;
        current.linear_vel = entry.second.linear_vel;
        current.enabled = entry.second.enabled;
        entry.first->set_State(current);
    }
}

namespace {
PhysicsShapeHandle Shape(CPHActivationShape::EType type, const Fvector& size)
{
    auto* core = GetPhysicsCore();
    switch (type) {
    case CPHActivationShape::etSphere: return core->CreateSphereShape(size.x);
    case CPHActivationShape::etCylinder: return core->CreateCylinderShape(size.x, size.y * .5f);
    default: return core->CreateBoxShape(Fvector().set(size).mul(.5f));
    }
}
}

CPHActivationShape::CPHActivationShape()
{
    m_flags.zero();
    m_flags.set(flFixedRotation, true);
}
CPHActivationShape::~CPHActivationShape() { VERIFY(!m_geom); }

void CPHActivationShape::Create(const Fvector position, const Fvector size,
    IPhysicsShellHolder* holder, EType type, u16 flags)
{
    VERIFY(holder && !m_geom);
    m_transform.identity();
    m_transform.c = position;
    m_size = size;
    m_type = type;
    m_ref_object = holder;
    m_geom = Shape(type, size);
    m_flags.set(flags, true);
    // Activation is a query. It must not add a massive phantom body to the world.
}
void CPHActivationShape::Destroy()
{
    if (m_geom) GetPhysicsCore()->DestroyCDBModel(m_geom);
    m_geom = nullptr;
    m_ref_object = nullptr;
}
bool CPHActivationShape::Activate(const Fvector size, u16 steps, float max_displacement,
    float max_rotation, bool un_freeze_later)
{
    VERIFY(m_geom);
    auto* core = GetPhysicsCore();
    core->DestroyCDBModel(m_geom);
    m_geom = Shape(static_cast<EType>(m_type), size);
    m_size = size;
    if (!m_geom) return false;
    Fquaternion rotation;
    rotation.set(m_transform);
    Fvector position;
    const bool free = core->FindFreeShapePlacement(m_geom, m_transform.c, rotation, position,
        m_flags.test(flFixedPosition) ? 0.f : max_displacement,
        _max<int>(16, steps), m_check_characters, m_ref_object);
    if (free) m_transform.c = position;
    return free;
}
const Fvector& CPHActivationShape::Position() { return m_transform.c; }
void CPHActivationShape::Size(Fvector& size) { size = m_size; }
void CPHActivationShape::set_rotation(const Fmatrix& rotation)
{
    const auto position = m_transform.c;
    m_transform = rotation;
    m_transform.c = position;
}
void CPHActivationShape::PhDataUpdate(float) {}
void CPHActivationShape::PhTune(float) {}
void CPHActivationShape::CutVelocity(float, float) {}
PhysicsShapeHandle CPHActivationShape::dSpacedGeom() { return m_geom; }
void CPHActivationShape::get_spatial_params()
{
    spatial.sphere.P = m_transform.c;
    spatial.sphere.R = m_size.magnitude() * .5f;
    AABB = Fvector().set(m_size).mul(.5f);
}
void CPHActivationShape::InitContact(bool&, bool, float, CPhysicsGeom*, CPhysicsGeom*, u16, u16) {}
#ifdef DEBUG
IPhysicsShellHolder* CPHActivationShape::ref_object() { return m_ref_object; }
#endif
