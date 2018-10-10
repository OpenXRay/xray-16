#include "StdAfx.h"

#include "animation_utils.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "game_object_space.h"

anim_bone_fix::anim_bone_fix() : bone(NULL), parent(NULL), matrix(Fmatrix().identity()) {}
anim_bone_fix::~anim_bone_fix()
{
    VERIFY(!bone);
    VERIFY(!parent);
}
void anim_bone_fix::callback(CBoneInstance* BI)
{
    //	Fmatrix	m = BI->mTransform;

    anim_bone_fix* fix = (anim_bone_fix*)BI->callback_param();
    VERIFY(fix->bone);
    VERIFY(fix->parent);
    // VERIFY( fix->bone == BI );
    BI->mTransform.mul_43(fix->parent->mTransform, fix->matrix);

    // Fmatrix diff  = Fmatrix().mul_43( Fmatrix().invert( m ) , BI->mTransform );

    // if(diff.c.magnitude() > 0.5f)
    //{
    //	int i=0;i++;
    //}
    R_ASSERT2(_valid(BI->mTransform), "anim_bone_fix::	callback");
}

void anim_bone_fix::fix(u16 bone_id, IKinematics& K)
{
    // return;
    VERIFY(&K);
    VERIFY(K.LL_GetBoneRoot() != bone_id);

    CBoneInstance& bi = K.LL_GetBoneInstance(bone_id);

    VERIFY(!bi.callback());

    bone = &bi;
    CBoneData& bd = K.LL_GetData(bone_id);

    parent = &K.LL_GetBoneInstance(bd.GetParentID());
    matrix.mul_43(Fmatrix().invert(parent->mTransform), bi.mTransform);
    bi.set_callback(bctCustom, callback, this, TRUE);
}
void anim_bone_fix::refix()
{
    // return;
    bone->set_callback(bctCustom, callback, this, TRUE);
}
void anim_bone_fix::deinit()
{
    release();
    bone = NULL;
    parent = NULL;
}
void anim_bone_fix::release()
{
    // return;
    VERIFY(bone->callback() == callback);
    VERIFY(bone->callback_param() == this);
    bone->reset_callback();
}

bool find_in_parents(const u16 bone_to_find, const u16 from_bone, IKinematics& ca)
{
    const u16 root = ca.LL_GetBoneRoot();
    u16 bi = from_bone;
    for (; bi != root && bi != BI_NONE;)
    {
        const CBoneData& bd = ca.LL_GetData(bi);
        if (bi == bone_to_find)
            return true;
        bi = bd.GetParentID();
    }
    return false;
}
