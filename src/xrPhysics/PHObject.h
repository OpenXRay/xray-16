#pragma once

#include "xrCDB/ISpatial.h"
#include "PHItemList.h"
#include "xrPhysicsCore/IPhysicsCore.h"

typedef u32 CLClassBits;
typedef u32 CLBits;
class SpatialBase;
using qResultVec = xr_vector<ISpatial*>;
class CPHObject;
class CPHUpdateObject;
class CPHMoveStorage;
class CPHSynchronize;
class CPhysicsGeom;

#ifdef DEBUG
class IPhysicsShellHolder;
#endif

class CPHObject : public SpatialBase
{
#ifdef DEBUG
    friend struct SPHObjDBGDraw;
#endif
    DECLARE_PHLIST_ITEM(CPHObject)

    Flags8 m_flags;

    enum
    {
        st_activated = (1 << 0),
        st_freezed = (1 << 1),
        st_dirty = (1 << 2),
        st_net_interpolation = (1 << 3),
        // fl_ray_motions удален, так как CCD теперь обрабатывается внутри Jolt
        st_recently_deactivated = (1 << 5)
    };

    CLBits m_collide_bits;
    u8 m_check_count;
    _flags<CLClassBits> m_collide_class_bits;

public:
    enum ECastType
    {
        tpNotDefinite,
        tpShell,
        tpCharacter,
        tpStaticShell
    };

protected:
    Fvector AABB;

protected:
    virtual void get_spatial_params() = 0;
    virtual void spatial_register();

    CPHObject* SelfPointer() { return this; }
public:
    virtual void FreezeContent();
    virtual void UnFreezeContent();
    virtual void EnableObject(CPHObject* obj);
    virtual bool DoCollideObj();
    virtual bool step_single(float step); // dReal -> float
    void reinit_single();
    void step_prediction(float time);
    void step(float time);

    virtual void PhDataUpdate(float step) = 0; // dReal -> float
    virtual void PhTune(float step) = 0; // dReal -> float
    virtual void spatial_move();

    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2) = 0;
    virtual void CutVelocity(float l_limit, float a_limit){};

    void Freeze();
    void UnFreeze();
    IC bool IsFreezed() { return !!(m_flags.test(st_freezed)); }
    void NetInterpolationON() { m_flags.set(st_net_interpolation, true); }
    void NetInterpolationOFF() { m_flags.set(st_net_interpolation, false); }
    bool NetInterpolation() { return !!(m_flags.test(st_net_interpolation)); }
    virtual u16 get_elements_number() = 0;
    virtual CPHSynchronize* get_element_sync(u16 element) = 0;

    CPHObject();
    void activate();
    IC bool is_active() const { return !!m_flags.test(st_activated); }
    void deactivate();
    void put_in_recently_deactivated();
    void remove_from_recently_deactivated();
    void check_recently_deactivated();
    void collision_disable();
    void collision_enable();
    virtual void ClearRecentlyDeactivated() { ; }
    virtual void Collide();
    virtual void near_callback(CPHObject* obj) { ; }
    virtual CPHMoveStorage* MoveStorage() { return NULL; }
    virtual ECastType CastType() { return tpNotDefinite; }
    virtual void vis_update_activate() {}
    virtual void vis_update_deactivate() {}
#ifdef DEBUG
    virtual IPhysicsShellHolder* ref_object() = 0;
#endif

    IC CLBits& collide_bits() { return m_collide_bits; }
    IC _flags<CLClassBits>& collide_class_bits() { return m_collide_class_bits; }
    IC const CLBits& collide_bits() const { return m_collide_bits; }
    IC const _flags<CLClassBits>& collide_class_bits() const { return m_collide_class_bits; }
    void CollideDynamics();
};

DEFINE_PHITEM_LIST(CPHObject, PH_OBJECT_STORAGE, PH_OBJECT_I)
