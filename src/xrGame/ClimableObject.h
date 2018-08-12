#pragma once
#include "PhysicsShellHolder.h"
#include "xrPhysics/IClimableObject.h"
class IPHStaticGeomShell;
class CPHCharacter;
struct dContact;
struct SGameMtl;

class CClimableObject : public CPhysicsShellHolder,
                        public IClimableObject
#ifdef DEBUG
                        ,
                        public pureRender
#endif
{
    typedef CPhysicsShellHolder inherited;
    // CPHLeaderGeomShell* m_pStaticShell;
    IPHStaticGeomShell* m_pStaticShell;
    Fobb m_box;
    Fvector m_axis;
    Fvector m_side;
    Fvector m_norm;
    float m_radius;
    u16 m_material;

public:
    CClimableObject();
    ~CClimableObject();
    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void shedule_Update(u32 dt); // Called by sheduler
    virtual void UpdateCL(); // Called each frame, so no need for dt
    virtual void Center(Fvector& C) const;
    virtual float Radius() const;
    ////////////////////////////////////////////////////////////////////
    virtual IPhysicsShellHolder* cast_IPhysicsShellHolder() { return this; }
#ifdef DEBUG
    virtual void OnRender();
#endif
protected:
    virtual BOOL UsedAI_Locations();

public:
    const Fvector& Axis() const { return m_axis; }
    virtual float DDAxis(Fvector& dir) const;

    virtual const Fvector& Side() const { return m_side; }
    virtual float DDSide(Fvector& dir) const;

    virtual const Fvector& Norm() const { return m_norm; }
    virtual float DDNorm(Fvector& dir) const;
    virtual bool BeforeLadder(CPHCharacter* actor, float tolerance = 0.f) const;
    virtual float DDLowerP(CPHCharacter* actor, Fvector& out_dir) const; // returns distance and dir to lover point
    virtual float DDUpperP(CPHCharacter* actor, Fvector& out_dir) const; // returns distance and dir to upper point

    void DToAxis(CPHCharacter* actor, Fvector& dir) const;
    virtual float DDToAxis(CPHCharacter* actor, Fvector& out_dir) const; // returns distance and dir to ladder axis
    void POnAxis(CPHCharacter* actor, Fvector& P) const;

    virtual float AxDistToUpperP(CPHCharacter* actor) const;
    virtual float AxDistToLowerP(CPHCharacter* actor) const;

    void DSideToAxis(CPHCharacter* actor, Fvector& dir) const;
    float DDSideToAxis(CPHCharacter* actor, Fvector& dir) const;

    virtual void DToPlain(CPHCharacter* actor, Fvector& dist) const;
    virtual float DDToPlain(CPHCharacter* actor, Fvector& dir) const;
    bool InRange(CPHCharacter* actor) const;
    virtual bool InTouch(CPHCharacter* actor) const;
    virtual u16 Material() const { return m_material; }
    void LowerPoint(Fvector& P) const;
    void UpperPoint(Fvector& P) const;
    void DefineClimbState(CPHCharacter* actor) const;
    static void ObjectContactCallback(
        bool& /**do_colide/**/, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/);

public:
    virtual bool register_schedule() const { return false; }
};
