#include "StdAfx.h"
#include "ParticlesObject.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "Level.h"
#include "GamePersistent.h"
#include "xrPhysics/ExtendedGeom.h"
#include "PhysicsGamePars.h"

#include "xrPhysics/PhysicsExternalCommon.h"
#include "xrPhysics/NativeContactEffects.h"
#include "PHSoundPlayer.h"
#include "PhysicsShellHolder.h"
#include "xrPhysics/PHCommander.h"
#include "xrPhysics/MathUtils.h"
#include "xrPhysics/IPHWorld.h"

#include "xrPhysics/PHReqComparer.h"

#include "Include/xrRender/FactoryPtr.h"
#include "Include/xrRender/WallMarkArray.h"
//#ifdef	DEBUG

//#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
static const float PARTICLE_EFFECT_DIST = 70.f;
static const float SOUND_EFFECT_DIST = 70.f;
const float mass_limit =
    10000.f; // some conventional value used as evaluative param (there is no code restriction on mass)
//////////////////////////////////////////////////////////////////////////////////
static const float SQUARE_PARTICLE_EFFECT_DIST = PARTICLE_EFFECT_DIST * PARTICLE_EFFECT_DIST;
static const float SQUARE_SOUND_EFFECT_DIST = SOUND_EFFECT_DIST * SOUND_EFFECT_DIST;
class CPHParticlesPlayCall : public CPHAction
{
    LPCSTR ps_name;

protected:
    Fvector c_pos;
    Fvector c_normal;

public:
    CPHParticlesPlayCall(const Fvector& contact_pos, const Fvector& contact_normal, bool invert_n, LPCSTR psn)
    {
        ps_name = psn;
        c_pos = contact_pos;
        c_normal = contact_normal;
        if (invert_n)
        {
            c_normal.invert();
        }
    }
    virtual void run()
    {
        CParticlesObject* ps = CParticlesObject::Create(ps_name, TRUE);

        Fmatrix pos;
        Fvector zero_vel = {0.f, 0.f, 0.f};
        pos.k.set(c_normal);
        Fvector::generate_orthonormal_basis(pos.k, pos.j, pos.i);
        pos.c.set(c_pos);

        ps->UpdateParent(pos, zero_vel);
        GamePersistent().ps_needtoplay.push_back(ps);
    };
    virtual bool obsolete() const { return false; }
};
static const float minimal_plane_distance_between_liquid_particles = 0.2f;
class CPHLiquidParticlesPlayCall : public CPHParticlesPlayCall, public CPHReqComparerV
{
    u32 remove_time;
    bool b_called;

public:
    CPHLiquidParticlesPlayCall(const Fvector& contact_pos, const Fvector& contact_normal, bool invert_n, LPCSTR psn)
        : CPHParticlesPlayCall(contact_pos, contact_normal, invert_n, psn), b_called(false)
    {
        static const u32 time_to_call_remove = 3000;
        remove_time = Device.dwTimeGlobal + time_to_call_remove;
    }
    const Fvector& position() const { return c_pos; }
private:
    virtual bool compare(const CPHReqComparerV* v) const { return v->compare(this); }
    virtual void run()
    {
        if (b_called)
            return;
        b_called = true;
        CPHParticlesPlayCall::run();
    }
    virtual bool obsolete() const { return Device.dwTimeGlobal > remove_time; }
};

class CPHLiquidParticlesCondition : public CPHCondition, public CPHReqComparerV
{
private:
    virtual bool compare(const CPHReqComparerV* v) const { return v->compare(this); }
    virtual bool is_true() { return true; }
    virtual bool obsolete() const { return false; }
};
class CPHFindLiquidParticlesComparer : public CPHReqComparerV
{
    Fvector m_position;

public:
    CPHFindLiquidParticlesComparer(const Fvector& position) : m_position(position) {}
private:
    virtual bool compare(const CPHReqComparerV* v) const { return v->compare(this); }
    virtual bool compare(const CPHLiquidParticlesCondition* v) const { return true; }
    virtual bool compare(const CPHLiquidParticlesPlayCall* v) const
    {
        VERIFY(v);

        Fvector disp = Fvector().sub(m_position, v->position());
        return disp.x * disp.x + disp.z * disp.z <
            (minimal_plane_distance_between_liquid_particles * minimal_plane_distance_between_liquid_particles);
    }
};

// class CPHLiquidParticlesComparer :
//	public CPHReqComparerV
//{
//	virtual bool			compare							(const	CPHReqComparerV* v)					const	{return
// v->compare(this);}
//	virtual bool			compare							(const	CPHOnesConditionSelfCmpTrue* v)		const	{return
// true;}
//
//};

class CPHWallMarksCall : public CPHAction
{
    // ref_shader pWallmarkShader;
    wm_shader pWallmarkShader;
    Fvector pos;
    CDB::TRI* T;

public:
    // CPHWallMarksCall(const Fvector &p,CDB::TRI* Tri,ref_shader s)
    CPHWallMarksCall(const Fvector& p, CDB::TRI* Tri, const wm_shader& s)
    {
        pWallmarkShader = s;
        pos.set(p);
        T = Tri;
    }
    virtual void run()
    {
        //добавить отметку на материале
        GEnv.Render->add_StaticWallmark(pWallmarkShader, pos, 0.09f, T, Level().ObjectSpace.GetStaticVerts());
    };
    virtual bool obsolete() const { return false; }
};

static void play_object(IPhysicsShellHolder* holder, SGameMtlPair* mtl_pair, const Fvector& contact_pos)
{
    if (!holder) return;

    auto* phShell = static_cast<CPhysicsShellHolder*>(holder);
    if (!phShell->m_pPhysicsShell && !phShell->character_physics_support() || phShell->getDestroy())
        return;

    CPHSoundPlayer* sp = phShell->ph_sound_player();
    if (sp)
        sp->Play(mtl_pair, contact_pos);
}

template <class Pars>
IC bool play_liquid_particle_criteria(IPhysicsShellHolder* holder, float vel_cret)
{
    if (vel_cret > Pars::vel_cret_particles)
        return true;

    CPHObject* ph_obj = holder ? smart_cast<CPHObject*>(holder) : nullptr;
    bool controller = !!ph_obj && ph_obj->CastType() == CPHObject::tpCharacter;

    return !controller && vel_cret > Pars::vel_cret_particles / 4.f;
}

template <class Pars>
void play_particles(float vel_cret, IPhysicsShellHolder* holder, const Fvector& contact_pos, const Fvector& contact_normal, bool b_invert_normal,
    const SGameMtl* static_mtl, LPCSTR ps_name)
{
    VERIFY(static_mtl);
    bool liquid = !!static_mtl->Flags.test(SGameMtl::flLiquid);

    bool play_liquid = liquid && play_liquid_particle_criteria<Pars>(holder, vel_cret);
    bool play_not_liquid = !liquid && vel_cret > Pars::vel_cret_particles;

    if (play_not_liquid)
        Level().ph_commander().add_call(xr_new<CPHOnesCondition>(), xr_new<CPHParticlesPlayCall>(contact_pos, contact_normal, b_invert_normal, ps_name));
    else if (play_liquid)
    {
        CPHFindLiquidParticlesComparer find(contact_pos);
        if (!Level().ph_commander().has_call(&find, &find))
            Level().ph_commander().add_call(
                xr_new<CPHLiquidParticlesCondition>(), xr_new<CPHLiquidParticlesPlayCall>(contact_pos, contact_normal, b_invert_normal, ps_name));
    }
}

template <class Pars>
void TContactShotMark(
    bool& do_colide, bool bo1,
    CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2)
{
    IPhysicsShellHolder* holder_1 = my_geom ? (IPhysicsShellHolder*)my_geom->get_callback_data() : nullptr;
    IPhysicsShellHolder* holder_2 = oposite_geom ? (IPhysicsShellHolder*)oposite_geom->get_callback_data() : nullptr;

    IPhysicsShellHolder* dyn_holder = bo1 ? holder_1 : holder_2;
    SGameMtl* static_mat = bo1 ? material_2 : material_1;
    SGameMtl* dyn_mat = bo1 ? material_1 : material_2;

    if (!static_mat || !dyn_mat) return;

    float vel_cret = 0;
    bool b_invert_normal = false;
    CPhysicsGeom* impact_geometry = nullptr;
    if (!ContactShotMarkGetEffectPars(contact_pos, contact_normal, my_geom, oposite_geom,
        impact_geometry, vel_cret, b_invert_normal)) return;
    dyn_holder = impact_geometry->ph_ref_object;
    static_mat = b_invert_normal ? material_1 : material_2;
    dyn_mat = b_invert_normal ? material_2 : material_1;

    Fvector to_camera;
    to_camera.sub(contact_pos, Device.vCameraPosition);
    float square_cam_dist = to_camera.square_magnitude();

    const auto* staticGeometry = b_invert_normal ? my_geom : oposite_geom;
    if (!NativeContactHasEffects<Pars>(vel_cret, square_cam_dist,
        static_mat->Flags.test(SGameMtl::flPassable), staticGeometry && staticGeometry->contact_triangle != u32(-1),
        SQUARE_SOUND_EFFECT_DIST, SQUARE_PARTICLE_EFFECT_DIST)) return;
    const auto* response = my_geom ? my_geom->contact_response : nullptr;
    if (!response && oposite_geom) response = oposite_geom->contact_response;
    u16 static_mat_id = NativeContactMaterialIndex(static_mat, response);
    u16 dyn_mat_id = NativeContactMaterialIndex(dyn_mat, response);

    SGameMtlPair* mtl_pair = GMLib.GetMaterialPairByIndices(static_mat_id, dyn_mat_id);
    if (mtl_pair)
    {
        if (staticGeometry && staticGeometry->contact_triangle != u32(-1) &&
            vel_cret > Pars::vel_cret_wallmark && !mtl_pair->CollideMarks->empty()) {
            auto* triangle = Level().ObjectSpace.GetStaticTris() + staticGeometry->contact_triangle;
            Level().ph_commander().add_call(xr_new<CPHOnesCondition>(),
                xr_new<CPHWallMarksCall>(contact_pos, triangle, mtl_pair->CollideMarks->GenerateWallmark()));
        }
        if (square_cam_dist < SQUARE_SOUND_EFFECT_DIST)
        {
            if (!static_mat->Flags.test(SGameMtl::flPassable))
            {
                if (vel_cret > Pars::vel_cret_sound)
                {
                    if (!mtl_pair->CollideSounds.empty())
                    {
                        float volume = collide_volume_min +
                            vel_cret * (collide_volume_max - collide_volume_min) /
                                (_sqrt(mass_limit) * default_l_limit - Pars::vel_cret_sound);
                        volume = std::clamp(volume, collide_volume_min, 0.9f);
                        ref_sound& randSound =
                            mtl_pair->CollideSounds[Random.randI(mtl_pair->CollideSounds.size())];
                        Fvector pos = contact_pos;
                        randSound.play_no_feedback(0, 0, 0, &pos, &volume);
                    }
                }
            }
            else
            {
                if (!mtl_pair->CollideSounds.empty())
                {
                    play_object(dyn_holder, mtl_pair, contact_pos);
                }
            }
        }
        if (square_cam_dist < SQUARE_PARTICLE_EFFECT_DIST && !mtl_pair->CollideParticles.empty() && vel_cret > Pars::vel_cret_particles)
        {
            LPCSTR ps_name = mtl_pair->CollideParticles[::Random.randI(0, mtl_pair->CollideParticles.size())].c_str();
            play_particles<Pars>(vel_cret, dyn_holder, contact_pos, contact_normal, b_invert_normal, static_mat, ps_name);
        }
    }
}

ObjectContactCallbackFun* ContactShotMark = &TContactShotMark<EffectPars>;
ObjectContactCallbackFun* CharacterContactShotMark = &TContactShotMark<CharacterEffectPars>;
