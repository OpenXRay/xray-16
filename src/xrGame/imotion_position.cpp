#include "StdAfx.h"
#include "imotion_position.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/MathUtils.h"
#include "xrPhysics/ExtendedGeom.h"
#include "Include/xrRender/Kinematics.h"
#include "Common/Noncopyable.hpp"
#include "PhysicsShellHolder.h"
#include "game_object_space.h"
#include "animation_utils.h"
#include "xrCore/buffer_vector.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

#ifdef DEBUG
BOOL dbg_imotion_draw_skeleton = FALSE;
BOOL dbg_imotion_draw_velocity = FALSE;
BOOL dbg_imotion_collide_debug = FALSE;
float dbg_imotion_draw_velocity_scale = 0.01;

#endif

static const float max_collide_timedelta = 0.02f; // 0.005f;
static const float end_delta = 0.5f * max_collide_timedelta;
static const float collide_adwance_delta = 2.f * max_collide_timedelta;
static const float depth_resolve = 0.01f;

imotion_position::imotion_position()
    : interactive_motion(), time_to_end(0.f), saved_visual_callback(0), blend(0), shell_motion_has_history(false){

                                                                                  };

static void interactive_motion_diag(LPCSTR message, const CBlend& b, CPhysicsShell* s, float time_left)
{
#ifdef DEBUG
    if (!death_anim_debug)
        return;
    const MotionID& m = b.motionID;
    VERIFY(m.valid());
    VERIFY(s);
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(s->PKinematics());
    VERIFY(KA);
    CPhysicsShellHolder* O = smart_cast<CPhysicsShellHolder*>(s->get_ElementByStoreOrder(0)->PhysicsRefObject());
    VERIFY(O);
    LPCSTR motion_name = KA->LL_MotionDefName_dbg(m).first;
    Msg("death anims - interactive_motion:- %s, motion: %s, blend time %f , total blend time %f , time left: %f , obj: "
        "%s, "
        "model:  %s ",
        message, motion_name, b.timeCurrent, b.timeTotal, time_left, O->cName().c_str(), O->cNameVisual().c_str());
#endif
}

void imotion_position::interactive_motion_diagnostic(LPCSTR message)
{
#ifdef DEBUG
    VERIFY(blend);
    interactive_motion_diag(message, *blend, shell, time_to_end);
#endif
}
#ifdef DEBUG
CPhysicsShellHolder* collide_obj = 0;
#endif

static float depth = 0;
static void get_depth(bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    save_max(depth, c.geom.depth);
#ifdef DEBUG
    if (depth != c.geom.depth)
        return;
    dxGeomUserData* ud = 0;
    if (bo1)
        ud = PHRetrieveGeomUserData(c.geom.g2);
    else
        ud = PHRetrieveGeomUserData(c.geom.g1);
    if (ud)
        collide_obj = static_cast<CPhysicsShellHolder*>(ud->ph_ref_object);
    else
        collide_obj = 0;
#endif
}
static std::string collide_diag()
{
#ifdef DEBUG
    if (collide_obj)
        return make_string("collide obj: %s", collide_obj->cName().c_str());
    else
        return make_string("collide static");
#else
    return std::string("");
#endif
}

void disable_bone_calculation(IKinematics& K, bool v)
{
    u16 bn = K.LL_BoneCount();
    for (u16 i = 1; i < bn; ++i) // ommit real root
    {
        CBoneInstance& bi = K.LL_GetBoneInstance(i);
        if (bi.callback_param() != 0)
            continue;
#ifdef DEBUG
        if (v && bi.callback_overwrite() == BOOL(v))
            Msg("! bone callback_overwrite may have different states");
#endif
        bi.set_callback_overwrite(v);
    }
}
void imotion_position::state_start()
{
    VERIFY(shell);
    inherited::state_start();

    IKinematics* K = shell->PKinematics();
    saved_visual_callback = K->GetUpdateCallback();
    K->SetUpdateCallback(0);
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(shell->PKinematics());
    VERIFY(KA);
    KA->SetUpdateTracksCalback(&update_callback);
    update_callback.motion = this;
    struct get_controled_blend : public IterateBlendsCallback, private Noncopyable

    {
        CBlend* blend;
        const PlayCallback cb;
        get_controled_blend(const PlayCallback _cb) : blend(0), cb(_cb) {}
        virtual void operator()(CBlend& B)
        {
            if (cb == B.Callback && B.bone_or_part == 0)
                blend = &B;
        }
    } get_blend(anim_callback);

    KA->LL_IterateBlends(get_blend);
#ifdef DEBUG
    if (!get_blend.blend)
    {
        Msg("bad animation params : %p", anim_callback);
        KA->LL_DumpBlends_dbg();
        NODEFAULT;
    }
#endif
    VERIFY(get_blend.blend);
    const CBlend& B = *get_blend.blend;

    blend = get_blend.blend;

    VERIFY2(B.stop_at_end,
        make_string("can not use cyclic anim in death animth motion: %s", KA->LL_MotionDefName_dbg(B.motionID).first));
    time_to_end = B.timeTotal - (SAMPLE_SPF + EPS) - B.timeCurrent;
    time_to_end /= B.speed;
    shell->add_ObjectContactCallback(get_depth);
    /*
    collide();
    if( flags.test( fl_switch_dm_toragdoll ) )
    {
        flags.assign( 0 );
        shell->remove_ObjectContactCallback( get_depth );
        return;
    }
*/

    if (!is_enabled())
        return;
    CPhysicsShellHolder* obj = static_cast<CPhysicsShellHolder*>(shell->get_ElementByStoreOrder(0)->PhysicsRefObject());
    VERIFY(obj);
    obj->processing_activate();
    shell->Disable();
    // K->LL_SetBoneRoot( 0 );
    shell->EnabledCallbacks(FALSE);
    init_bones();

    shell->mXFORM.set(obj->XFORM());
    disable_update(true);
    disable_bone_calculation(*K, true);
    // K->CalculateBones_Invalidate();
    collide_not_move(*KA);
    if (flags.test(fl_switch_dm_toragdoll))
    {
        interactive_motion_diagnostic("stoped immediately");
        switch_to_free();
        flags.set(fl_not_played, TRUE);
        return;
    }
    move(float(Device.dwTimeDelta) / 1000, *KA);
    if (flags.test(fl_switch_dm_toragdoll))
        switch_to_free();
    // K->CalculateBones_Invalidate();
}

#ifdef DEBUG
static void dbg_draw_state_end(CPhysicsShell* shell)
{
    VERIFY(shell);
    if (dbg_imotion_draw_velocity)
    {
        DBG_OpenCashedDraw();
        shell->dbg_draw_velocity(dbg_imotion_draw_velocity_scale, color_argb(100, 255, 0, 0));
        // shell->dbg_draw_force( 0.01, color_xrgb( 0, 0, 255 ) );
        DBG_ClosedCashedDraw(50000);
    }
    if (dbg_imotion_collide_debug)
    {
#ifdef DEBUG
        DBG_OpenCashedDraw();
        shell->dbg_draw_geometry(0.02, color_argb(255, 255, 255, 255));
        DBG_ClosedCashedDraw(50000);
#endif
    }
}
#endif

static xr_vector<anim_bone_fix*> saved_fixes;
static void save_fixes(IKinematics* K)
{
    VERIFY(K);
    saved_fixes.clear();
    u16 nbb = K->LL_BoneCount();
    for (u16 i = 0; i < nbb; ++i)
    {
        CBoneInstance& bi = K->LL_GetBoneInstance(i);
        if (bi.callback() == anim_bone_fix::callback)
        {
            VERIFY(bi.callback_param());
            anim_bone_fix* fix = (anim_bone_fix*)bi.callback_param();
            VERIFY(fix->bone == &bi);
            saved_fixes.push_back(fix);
        }
    }
}
static void restore_fixes()
{
    struct refix
    {
        void operator()(anim_bone_fix* fix) { fix->refix(); }
    } rf;

    std::for_each(saved_fixes.begin(), saved_fixes.end(), rf);
    saved_fixes.clear();
}
void imotion_position::state_end()
{
    VERIFY(shell);
    inherited::state_end();

    CPhysicsShellHolder* obj = static_cast<CPhysicsShellHolder*>(shell->get_ElementByStoreOrder(0)->PhysicsRefObject());
    VERIFY(obj);
    obj->processing_deactivate();
    shell->Enable();
    shell->setForce(Fvector().set(0.f, 0.f, 0.f));
    shell->setTorque(Fvector().set(0.f, 0.f, 0.f));

    shell->AnimToVelocityState(end_delta, default_l_limit * 10, default_w_limit * 10);
#ifdef DEBUG
    dbg_draw_state_end(shell);
#endif
    shell->remove_ObjectContactCallback(get_depth);
    IKinematics* K = shell->PKinematics();
    disable_update(false);
    disable_bone_calculation(*K, false);
    K->SetUpdateCallback(saved_visual_callback);
    deinit_bones();

    save_fixes(K);

    shell->EnabledCallbacks(TRUE);

    restore_fixes();

    VERIFY(K);

    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(shell->PKinematics());
    VERIFY(KA);
    update_callback.motion = 0;
    KA->SetUpdateTracksCalback(0);

#if 0

            DBG_OpenCashedDraw();
            shell->dbg_draw_geometry( 0.02, color_argb( 255, 0, 255, 0 )  );
            DBG_DrawBones( *shell->get_ElementByStoreOrder( 0 )->PhysicsRefObject() );
            DBG_ClosedCashedDraw( 50000 );

#endif

    u16 root = K->LL_GetBoneRoot();
    if (root != 0)
    {
        K->LL_GetTransform(0).set(Fidentity);
        K->LL_SetBoneVisible(0, FALSE, FALSE);
        u16 bip01 = K->LL_BoneID("bip01");
        if (bip01 != BI_NONE && bip01 != root)
        {
            K->LL_GetTransform(bip01).set(Fidentity);
            K->LL_SetBoneVisible(bip01, FALSE, FALSE);
        }
    }

    K->CalculateBones_Invalidate();
    K->CalculateBones(true);

#if 0

            DBG_OpenCashedDraw();
            shell->dbg_draw_geometry( 0.02, color_argb( 255, 0, 0, 255 )  );
            DBG_DrawBones( *shell->get_ElementByStoreOrder( 0 )->PhysicsRefObject() );
            DBG_ClosedCashedDraw( 50000 );

#endif
}

void imotion_position::disable_update(bool v)
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
    // K->SetUpdateCallback( v ? 0 : saved_visual_callback );
    update_callback.update = !v;
    // disable_bone_calculation( *K, v );
}

void imotion_position::move_update()
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);

    disable_update(false);
    K->CalculateBones_Invalidate();
    K->CalculateBones();
    disable_update(true);
    VERIFY(shell);
}
void imotion_position::force_calculate_bones(IKinematicsAnimated& KA)
{
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
    VERIFY(K == smart_cast<IKinematics*>(&KA));
    disable_bone_calculation(*K, false);

    K->Bone_Calculate(&K->LL_GetData(0), &Fidentity);

    if (saved_visual_callback)
    {
        u16 sv_root = K->LL_GetBoneRoot();
        K->LL_SetBoneRoot(0);
        saved_visual_callback(K);
        K->LL_SetBoneRoot(sv_root);
    }
    disable_bone_calculation(*K, true);
}
float imotion_position::advance_animation(float dt, IKinematicsAnimated& KA)
{
    time_to_end -= dt;
    KA.LL_UpdateTracks(dt, true, true);

    force_calculate_bones(KA);

    shell->Disable();
    return dt;
}

#ifdef DEBUG
void DBG_DrawBones(IGameObject& O);
void DBG_PhysBones(IGameObject& O);
void collide_anim_dbg_draw(CPhysicsShell* shell, float dt)
{
    VERIFY(shell);
    if (dbg_imotion_draw_velocity)
    {
        shell->AnimToVelocityState(dt, default_l_limit * 10, default_w_limit * 10);
        DBG_OpenCashedDraw();
        shell->dbg_draw_velocity(dbg_imotion_draw_velocity_scale, color_xrgb(0, 255, 0));
        DBG_ClosedCashedDraw(50000);
    }
    if (dbg_imotion_draw_skeleton)
    {
        DBG_OpenCashedDraw();
        CPhysicsShellHolder* sh =
            static_cast<CPhysicsShellHolder*>(shell->get_ElementByStoreOrder(0)->PhysicsRefObject());
        DBG_PhysBones(*sh);
        DBG_ClosedCashedDraw(50000);
    }
}
#endif

float imotion_position::collide_animation(float dt, IKinematicsAnimated& k)
{
    advance_animation(dt, k);
#ifdef DEBUG
    collide_anim_dbg_draw(shell, dt);
#endif
    shell->ToAnimBonesPositions(shell_motion_has_history ? mh_not_clear : mh_unspecified);
    depth = 0;
#ifdef DEBUG
    if (dbg_imotion_collide_debug)
        DBG_OpenCashedDraw();
#endif
    shell->CollideAll();
#ifdef DEBUG
    if (dbg_imotion_collide_debug)
        DBG_ClosedCashedDraw(50000);
#endif
#ifdef DEBUG
    if (dbg_imotion_collide_debug)
        interactive_motion_diagnostic(make_string(" collide_animation: deppth= %f", depth).c_str());
#endif
    return dt;
}

static u32 blends_num(IKinematicsAnimated& KA)
{
    // u32 res = 0;
    // for( u32 i = 0; i < MAX_PARTS; ++i  )
    //{
    //	res += KA.LL_PartBlendsCount( i );
    //}
    // return res;
    struct scbl : public IterateBlendsCallback
    {
        u32 count;
        scbl() : count() {}
        virtual void operator()(CBlend& B) { ++count; }
    } cbl;

    KA.LL_IterateBlends(cbl);
    return cbl.count;
}

class sblend_save
{
    CBlend sv;
    CBlend* b;

public:
    sblend_save() : b(0){};
    void save(CBlend* B)
    {
        b = B;
        sv = *B;
    }
    void restore()
    {
        VERIFY(b);
        *b = sv;
        b = 0;
    }
};

static void save_blends(buffer_vector<sblend_save>& buffer, IKinematicsAnimated& KA)
{
    buffer.clear();
    struct scbl : public IterateBlendsCallback, private Noncopyable
    {
        buffer_vector<sblend_save>& _buffer;
        scbl(buffer_vector<sblend_save>& bf) : _buffer(bf) {}
        virtual void operator()(CBlend& B)
        {
            sblend_save s;
            s.save(&B);
            _buffer.push_back(s);
        }
    } cbl(buffer);
    KA.LL_IterateBlends(cbl);
}

static void restore_blends(buffer_vector<sblend_save>& buffer)
{
    buffer_vector<sblend_save>::iterator i = buffer.begin(), e = buffer.end();
    for (; i != e; ++i)
        (*i).restore();
    buffer.clear();
}

void imotion_position::collide_not_move(IKinematicsAnimated& KA)
{
    u32 sv_blends_num = blends_num(KA);
    buffer_vector<sblend_save> saved_blends(_alloca(sv_blends_num * sizeof(sblend_save)), sv_blends_num);
    save_blends(saved_blends, KA);
    motion_collide(0.5f * max_collide_timedelta, KA);
    restore_blends(saved_blends);
}
float imotion_position::move(float dt, IKinematicsAnimated& KA)
{
    VERIFY(shell);
    // float ret = 0;
    float advance_time = 0.f;
    float collide_dt = dt;
    u32 iterations = 1;

    if (dt > max_collide_timedelta)
    {
        float f_iterations = ceil(dt / max_collide_timedelta);
        VERIFY(f_iterations > 0.f);
        collide_dt = dt / f_iterations;
        iterations = u32(f_iterations);
    }

    for (u32 i = 0; i < iterations; ++i)
    {
        float ad = 0.f;
        u32 sv_blends_num = blends_num(KA);
        buffer_vector<sblend_save> saved_blends(_alloca(sv_blends_num * sizeof(sblend_save)), sv_blends_num);

        if (!flags.test(fl_switch_dm_toragdoll))
        {
            save_blends(saved_blends, KA);
            ad = motion_collide(collide_dt, KA);
        }
        advance_time += ad;
        if (!!flags.test(fl_switch_dm_toragdoll))
        {
            restore_blends(saved_blends);
            time_to_end -= ad;
            force_calculate_bones(KA);
            // advance_time += advance_animation( -( end_delta ), KA );//+ ad
            shell->ToAnimBonesPositions(shell_motion_has_history ? mh_not_clear : mh_unspecified);
            shell_motion_has_history = true;

#ifdef DEBUG
            if (dbg_imotion_collide_debug)
            {
                depth = 0;
                shell->CollideAll();
                interactive_motion_diagnostic(make_string(" move (to ragdoll): deppth= %f", depth).c_str());
                DBG_OpenCashedDraw();
                shell->dbg_draw_geometry(0.02, color_argb(255, 255, 0, 255));
                DBG_ClosedCashedDraw(50000);
            }
#endif
            advance_time += advance_animation(end_delta, KA);
            break;
        }
    }
    return advance_time;
}

float imotion_position::motion_collide(float dt, IKinematicsAnimated& KA)
{
    VERIFY(shell);

    float advance_time = collide_animation(dt, KA);

    if (time_to_end < (max_collide_timedelta + end_delta))
    {
        interactive_motion_diagnostic(make_string("motion_collide 0: stoped: time out, time delta %f", dt).c_str());
        flags.set(fl_switch_dm_toragdoll, TRUE);
        return advance_time;
    }
    if (depth > depth_resolve)
    {
#ifdef DEBUG
        if (dbg_imotion_collide_debug)
        {
            // interactive_motion_diagnostic( make_string( " motion_collide collided0: deppth= %f", depth ).c_str() );
            interactive_motion_diagnostic(
                make_string("motion_collide 1: stoped: colide: %s, depth %f", collide_diag().c_str(), depth).c_str());
            DBG_OpenCashedDraw();
            shell->dbg_draw_geometry(0.02, color_argb(255, 0, 255, 0));
            DBG_ClosedCashedDraw(50000);
        }
#endif
        u32 sv_blends_num = blends_num(KA);
        buffer_vector<sblend_save> saved_blends(_alloca(sv_blends_num * sizeof(sblend_save)), sv_blends_num);
        save_blends(saved_blends, KA); //		sv1
        float depth0 = depth;
        advance_time += collide_animation(collide_adwance_delta, KA);

#ifdef DEBUG
        if (dbg_imotion_collide_debug)
        {
            interactive_motion_diagnostic(make_string(" motion_collide collided1: deppth= %f", depth).c_str());
            DBG_OpenCashedDraw();
            shell->dbg_draw_geometry(0.02, color_argb(255, 0, 255, 255));
            DBG_ClosedCashedDraw(50000);
        }
#endif

        if (depth > depth0)
        {
            interactive_motion_diagnostic(
                make_string("motion_collide 1: stoped: colide: %s, depth %f", collide_diag().c_str(), depth).c_str());
            flags.set(fl_switch_dm_toragdoll, TRUE);
        }
        else
        {
            depth0 = depth;
            advance_time += collide_animation(collide_adwance_delta, KA);

#ifdef DEBUG
            if (dbg_imotion_collide_debug)
            {
                interactive_motion_diagnostic(make_string(" motion_collide collided2: deppth= %f", depth).c_str());
                DBG_OpenCashedDraw();
                shell->dbg_draw_geometry(0.02, color_argb(255, 0, 255, 0));
                DBG_ClosedCashedDraw(50000);
            }
#endif
            if (depth > depth_resolve)
            {
                interactive_motion_diagnostic(make_string("motion_collide 2: stoped: colide: %s, depth %f",
                    collide_diag().c_str(), depth).c_str());
                flags.set(fl_switch_dm_toragdoll, TRUE);
            }
        }
        restore_blends(saved_blends); // rs1
        // advance_time += advance_animation( dt-advance_time, KA );
        time_to_end += (dt - advance_time);
        advance_time += (dt - advance_time);
        force_calculate_bones(KA);
        shell->ToAnimBonesPositions(shell_motion_has_history ? mh_clear : mh_unspecified);

#ifdef DEBUG
        if (dbg_imotion_collide_debug)
        {
            depth = 0;
            shell->CollideAll();
            interactive_motion_diagnostic(make_string(" motion_collide restore: %f ", depth).c_str());
            DBG_OpenCashedDraw();
            shell->dbg_draw_geometry(0.02, color_argb(255, 255, 0, 0));
            DBG_ClosedCashedDraw(50000);
        }
#endif
    }
    return advance_time;
}

////////////////////////////////////////////////////////////////////////////////////
bool imotion_position::tracks_update::operator()(float dt, IKinematicsAnimated& k)
{
    if (!update)
        return false;
    VERIFY(motion && &(motion->update_callback) == this);
    motion->move(dt, k);
    return true;
}

void imotion_position::init_bones()
{
    set_root_callback();
    /*
        IKinematics &K  = *shell->PKinematics( );
        u16 bn = K.LL_BoneCount();
        for(u16 i = 1; i< bn; ++i )//ommit real root
        {
            CBoneInstance &bi = K.LL_GetBoneInstance( i );
            VERIFY(!bi.callback());
            VERIFY(!bi.callback_param());
            if(bi.callback_overwrite())
                bi.set_callback( bctCustom, 0, (void*)1, TRUE );
        }
    */
}

void imotion_position::deinit_bones()
{
    /*
        IKinematics &K  = *shell->PKinematics( );
        u16 bn = K.LL_BoneCount();
        for(u16 i = 1; i< bn; ++i )//ommit real root
        {
            CBoneInstance &bi = K.LL_GetBoneInstance( i );
            VERIFY( !bi.callback() );
            VERIFY( !bi.callback_param() || bi.callback_overwrite() );
            bi.reset_callback();
        }
    */
    remove_root_callback();
}

void imotion_position::set_root_callback()
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
    CBoneInstance& bi = K->LL_GetBoneInstance(0);
    VERIFY(!bi.callback());
    bi.set_callback(bctCustom, rootbone_callback, this, true); // root may be not "0" !
}

void imotion_position::remove_root_callback()
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
    CBoneInstance& bi = K->LL_GetBoneInstance(0);
    VERIFY(bi.callback() == rootbone_callback);
    VERIFY(bi.callback_param() == (void*)this);
    bi.reset_callback();
}

void imotion_position::rootbone_callback(CBoneInstance* BI)
{
    imotion_position* im = (imotion_position*)BI->callback_param();
    VERIFY(im);
    if (!im->update_callback.update)
        return;
    VERIFY(im->shell);
    IKinematics* K = im->shell->PKinematics();
    VERIFY(K);
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(K);
    VERIFY(KA);
    SKeyTable keys;
    KA->LL_BuldBoneMatrixDequatize(&K->LL_GetData(0), u8(-1), keys);

    CKey* key = 0;
    for (int i = 0; i < keys.chanel_blend_conts[0]; ++i)
    {
        if (keys.blends[0][i] == im->blend)
            key = &keys.keys[0][i];
    }
    if (key)
    {
        key->Q.rotation(Fvector().set(0, 1, 0), im->angle);
    }
    KA->LL_BoneMatrixBuild(*BI, &Fidentity, keys);

    R_ASSERT2(_valid(BI->mTransform), "imotion_position::rootbone_callback");
}
