#pragma once

#include "xrCore/_vector3d.h"
#include "xrCommon/math_funcs_inline.h"

#ifdef XRAY_STATIC_BUILD
#   define PARTICLES_API
#else
#   ifdef XR_PARTICLES_EXPORTS
#      define PARTICLES_API XR_EXPORT
#   else
#      define PARTICLES_API XR_IMPORT
#   endif
#endif

// Actually this must be < sqrt(MAXFLOAT) since we store this value squared.
#define P_MAXFLOAT 1.0e16f

#ifdef MAXINT
#define P_MAXINT MAXINT
#else
#define P_MAXINT 0x7fffffff
#endif

#define drand48() ::Random.randF()
//#define drand48() (((float) rand())/((float) RAND_MAX))

namespace PAPI
{
class pVector : public Fvector
{
public:
    pVector(float ax, float ay, float az) { set(ax, ay, az); }
    pVector() {}
    float length() const { return _sqrt(x * x + y * y + z * z); }
    float length2() const { return (x * x + y * y + z * z); }
    float operator*(const pVector& a) const { return x * a.x + y * a.y + z * a.z; }
    pVector operator*(const float s) const { return pVector(x * s, y * s, z * s); }

    pVector operator/(const float s) const
    {
        float invs = 1.0f / s;
        return pVector(x * invs, y * invs, z * invs);
    }

    pVector operator+(const pVector& a) const { return pVector(x + a.x, y + a.y, z + a.z); }
    pVector operator-(const pVector& a) const { return pVector(x - a.x, y - a.y, z - a.z); }

    pVector operator-()
    {
        x = -x;
        y = -y;
        z = -z;
        return *this;
    }

    pVector& operator+=(const pVector& a)
    {
        x += a.x;
        y += a.y;
        z += a.z;
        return *this;
    }

    pVector& operator-=(const pVector& a)
    {
        x -= a.x;
        y -= a.y;
        z -= a.z;
        return *this;
    }

    pVector& operator*=(const float a)
    {
        x *= a;
        y *= a;
        z *= a;
        return *this;
    }

    pVector& operator/=(const float a)
    {
        float b = 1.0f / a;
        x *= b;
        y *= b;
        z *= b;
        return *this;
    }

    pVector& operator=(const pVector& a)
    {
        x = a.x;
        y = a.y;
        z = a.z;
        return *this;
    }

    pVector operator^(const pVector& b) const
    {
        return pVector(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
    }
};

struct Rotation
{
    float x;
};

// A single particle
struct Particle
{
    enum
    {
        ANIMATE_CCW = 1 << 0,
    };

    Rotation rot; // 4
    pVector pos; // 12
    pVector posB; // 12
    pVector vel; // 12
    pVector size; // 12
    //pVector rot; // 12 60 // From xrEngine/psystem.h
    u32 color; // 4
    float age; // 4
    u16 frame; // 2
    Flags16 flags; // 2
}; // = 64 here
// = 72 was in xrEngine/psystem.h

using OnBirthParticleCB = void(*)(void* owner, u32 param, PAPI::Particle& P, u32 idx);
using OnDeadParticleCB = void(*)(void* owner, u32 param, PAPI::Particle& P, u32 idx);

//////////////////////////////////////////////////////////////////////
// Type codes for domains
enum PDomainEnum : u32
{
    PDPoint = 0, // Single point
    PDLine = 1, // Line segment
    PDTriangle = 2, // Triangle
    PDPlane = 3, // Arbitrarily-oriented plane
    PDBox = 4, // Axis-aligned box
    PDSphere = 5, // Sphere
    PDCylinder = 6, // Cylinder
    PDCone = 7, // Cone
    PDBlob = 8, // Gaussian blob
    PDDisc = 9, // Arbitrarily-oriented disc
    PDRectangle = 10, // Rhombus-shaped planar region
};

//////////////////////////////////////////////////////////////////////
// Type codes for all actions
enum PActionEnum : u32
{
    PAAvoidID, // Avoid entering the domain of space.
    PABounceID, // Bounce particles off a domain of space.
    PACallActionListID_obsolette, //
    PACopyVertexBID, // Set the secondary position from current position.
    PADampingID, // Dampen particle velocities.
    PAExplosionID, // An Explosion.
    PAFollowID, // Accelerate toward the previous particle in the effect.
    PAGravitateID, // Accelerate each particle toward each other particle.
    PAGravityID, // Acceleration in the given direction.
    PAJetID, //
    PAKillOldID, //
    PAMatchVelocityID, //
    PAMoveID, //
    PAOrbitLineID, //
    PAOrbitPointID, //
    PARandomAccelID, //
    PARandomDisplaceID, //
    PARandomVelocityID, //
    PARestoreID, //
    PASinkID, //
    PASinkVelocityID, //
    PASourceID, //
    PASpeedLimitID, //
    PATargetColorID, //
    PATargetSizeID, //
    PATargetRotateID, //
    PATargetRotateDID, //
    PATargetVelocityID, //
    PATargetVelocityDID, //
    PAVortexID, //
    PATurbulenceID, //
    PAScatterID, //
};

struct ParticleAction;

class IParticleManager
{
public:
    IParticleManager() {}
    virtual ~IParticleManager() {}
    // create&destroy
    virtual int CreateEffect(u32 max_particles) = 0;
    virtual void DestroyEffect(int effect_id) = 0;
    virtual int CreateActionList() = 0;
    virtual void DestroyActionList(int alist_id) = 0;

    // control
    virtual void PlayEffect(int effect_id, int alist_id) = 0;
    virtual void StopEffect(int effect_id, int alist_id, bool deffered = true) = 0;

    // update&render
    virtual void Update(int effect_id, int alist_id, float dt) = 0;
    virtual void Render(int effect_id) = 0;
    virtual void Transform(int alist_id, const Fmatrix& m, const Fvector& velocity) = 0;

    // effect
    virtual void RemoveParticle(int effect_id, u32 p_id) = 0;
    virtual void SetMaxParticles(int effect_id, u32 max_particles) = 0;
    virtual void SetCallback(int effect_id, OnBirthParticleCB b, OnDeadParticleCB d, void* owner, u32 param) = 0;
    virtual void GetParticles(int effect_id, Particle*& particles, u32& cnt) = 0;
    virtual u32 GetParticlesCount(int effect_id) = 0;

    // action
    virtual ParticleAction* CreateAction(PActionEnum type) = 0;
    virtual size_t LoadActions(int alist_id, IReader& R) = 0;
    virtual void SaveActions(int alist_id, IWriter& W) = 0;
};

PARTICLES_API IParticleManager* ParticleManager();
}
