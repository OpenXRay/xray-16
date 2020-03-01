#include "StdAfx.h"

#include "interactive_motion.h"

#include "xrPhysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"

#include "Include/xrRender/Kinematics.h"

#include "game_object_space.h"

void interactive_motion_diagnostic(LPCSTR message, const MotionID& m, CPhysicsShell* s)
{
#ifdef DEBUG
    if (!death_anim_debug)
        return;
    VERIFY(m.valid());
    VERIFY(s);
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(s->PKinematics());
    VERIFY(KA);
    CPhysicsShellHolder* O = smart_cast<CPhysicsShellHolder*>(s->get_ElementByStoreOrder(0)->PhysicsRefObject());
    VERIFY(O);
    LPCSTR motion_name = KA->LL_MotionDefName_dbg(m).first;
    Msg("death anims - interactive_motion:- %s, motion: %s, obj: %s, model:  %s ", message, motion_name,
        O->cName().c_str(), O->cNameVisual().c_str());
#endif
}

interactive_motion::interactive_motion() { init(); }
interactive_motion::~interactive_motion() { VERIFY(flags.get() == 0); }
void interactive_motion::init()
{
    flags.assign(0);

    shell = 0;
    angle = 0;
}
void interactive_motion::destroy()
{
    if (flags.test(fl_started))
        state_end();

    flags.assign(0);
}
void interactive_motion::setup(LPCSTR m, CPhysicsShell* s, float angle)
{
    VERIFY(m);
    VERIFY(s);
    IKinematicsAnimated* K = smart_cast<IKinematicsAnimated*>(s->PKinematics());
    VERIFY(K);
    setup(K->LL_MotionID(m), s, angle);
}

void interactive_motion::setup(const MotionID& m, CPhysicsShell* s, float _angle)
{
    VERIFY(s);
    VERIFY(m.valid());
#ifdef DEBUG
    IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(s->PKinematics());
    CMotionDef* MD = KA->LL_GetMotionDef(m);
    VERIFY2(MD->StopAtEnd(),
        make_string("can not use cyclic anim in death animth motion: %s", KA->LL_MotionDefName_dbg(m).first));

#endif
    motion = m;
    angle = _angle;
    interactive_motion_diagnostic("started", m, s);

    shell = s;
    flags.set(fl_use_death_motion, TRUE);
}

void interactive_motion::shell_setup()
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
}

void interactive_motion::anim_callback(CBlend* B)
{
    VERIFY(B->CallbackParam);
    ((interactive_motion*)(B->CallbackParam))->flags.set(fl_switch_dm_toragdoll, TRUE);
}

void interactive_motion::play()
{
    VERIFY(shell);
    VERIFY(motion.valid());
    smart_cast<IKinematicsAnimated*>(shell->PKinematics())->PlayCycle(motion, TRUE, anim_callback, this);
    state_start();
}

void interactive_motion::state_start()
{
    VERIFY(shell);
    shell_setup();
    flags.set(fl_started, TRUE);
}

void interactive_motion::state_end()
{
    VERIFY(shell);
    flags.set(fl_switch_dm_toragdoll, FALSE);
    flags.set(fl_use_death_motion, FALSE);

    flags.set(fl_started, FALSE);
}

void interactive_motion::update()
{
    VERIFY(shell);
    IKinematics* K = shell->PKinematics();
    VERIFY(K);

    collide();
    if (!flags.test(fl_switch_dm_toragdoll))
    {
        move_update();
        if (flags.test(fl_switch_dm_toragdoll))
            switch_to_free();
    }
}

void interactive_motion::switch_to_free()
{
    // set to normal state
    VERIFY(shell);
    state_end();
    /// set all matrises valide
    CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(shell->get_ElementByStoreOrder(0)->PhysicsRefObject());
    VERIFY(obj);
    shell->InterpolateGlobalTransform(&obj->XFORM());
    IKinematics* K = shell->PKinematics();
    VERIFY(K);
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
}
