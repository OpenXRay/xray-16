#include "StdAfx.h"
#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Include/xrRender/Kinematics.h"
#include "xrPhysics/IPHWorld.h"

// extern CPHWorld*	ph_world;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCar::SExhaust::~SExhaust() { CParticlesObject::Destroy(p_pgobject); }
void CCar::SExhaust::Init()
{
    VERIFY(!physics_world()->Processing());
    pelement = (bone_map.find(bone_id))->second.element;
    IKinematics* K = smart_cast<IKinematics*>(pcar->Visual());
    CBoneData& bone_data = K->LL_GetData(u16(bone_id));
    transform.set(bone_data.bind_transform);
    /// transform.mulA(pcar->XFORM());
    // Fmatrix element_transform;
    // pelement->InterpolateGlobalTransform(&element_transform);
    // element_transform.invert();
    // transform.mulA(element_transform);
    p_pgobject = CParticlesObject::Create(*pcar->m_exhaust_particles, FALSE);
    Fvector zero_vector;
    zero_vector.set(0.f, 0.f, 0.f);
    p_pgobject->UpdateParent(pcar->XFORM(), zero_vector);
}

void CCar::SExhaust::Update()
{
    VERIFY(!physics_world()->Processing());
    Fmatrix global_transform;
    pelement->InterpolateGlobalTransform(&global_transform);
    global_transform.mulB_43(transform);

    // dVector3 res;
    // Fvector	 res_vel;
    // dBodyGetPointVel(pelement->get_body(),global_transform.c.x,global_transform.c.y,global_transform.c.z,res);
    // CopyMemory (&res_vel,res,sizeof(Fvector));
    Fvector res_vel;
    pelement->GetPointVel(res_vel, global_transform.c);
    // velocity.mul(0.95f);
    // res_vel.mul(0.05f);
    // velocity.add(res_vel);
    p_pgobject->UpdateParent(global_transform, res_vel);
}

void CCar::SExhaust::Clear() { CParticlesObject::Destroy(p_pgobject); }
void CCar::SExhaust::Play()
{
    VERIFY(!physics_world()->Processing());
    p_pgobject->Play(false);
    Update();
}

void CCar::SExhaust::Stop()
{
    VERIFY(!physics_world()->Processing());
    p_pgobject->Stop();
}
