#include "pch_script.h"

#include "animation_script_callback.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "GameObject.h"
#include "game_object_space.h"
#include "Include/xrRender/KinematicsAnimated.h"

CBlend* PlayMotionByParts(
    IKinematicsAnimated* sa, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam)
{
    CBlend* ret = 0;
    CMotionDef* md = sa->LL_GetMotionDef(motion_ID);

    if (md->bone_or_part != BI_NONE)
        return sa->LL_PlayCycle(md->bone_or_part, motion_ID, bMixIn, Callback, CallbackParam);

    for (u16 i = 0; i < MAX_PARTS; ++i)
    {
        CBlend* blend = sa->LL_PlayCycle(i, motion_ID, bMixIn, Callback, CallbackParam);
        if (blend && !ret)
            ret = blend;
        // m_anim_blend = PKinematicsAnimated->PlayCycle(*visual->startup_animation);
    }
    return ret;
}

CBlend* anim_script_callback::play_cycle(IKinematicsAnimated* sa, const shared_str& anim)
{
    MotionID m = sa->LL_MotionID(*anim);
    R_ASSERT(m.valid());
    if (sa->LL_GetMotionDef(m)->StopAtEnd())
    {
        on_end = false;
        on_begin = false;
        is_set = true;
        return PlayMotionByParts(sa, m, FALSE, anim_callback, this);
    }
    else
    {
        on_end = false;
        on_begin = false;
        is_set = false;
        return PlayMotionByParts(sa, m, FALSE, 0, 0);
    }
}

void anim_script_callback::anim_callback(CBlend* B)
{
    anim_script_callback* ths = ((anim_script_callback*)B->CallbackParam);
    VERIFY(ths);
    VERIFY(ths->is_set);

    ////////////////////BLEND UPDATE//////////////////////////////////////////////
    // float quant = dt*speed;
    // timeCurrent += quant; // stop@end - time is not going

    // bool	running_fwrd	=  ( quant > 0 );
    // float	const END_EPS	=	SAMPLE_SPF+EPS;
    // bool	at_end			=	running_fwrd && ( timeCurrent > ( timeTotal-END_EPS ) );
    // bool	at_begin		=	!running_fwrd && ( timeCurrent < 0.f );
    ////..............................
    ////..............................
    ////..............................
    // if( at_end )
    //{
    //	timeCurrent	= timeTotal-END_EPS;		// stop@end - time frozen at the end
    //	if( timeCurrent<0.f ) timeCurrent =0.f;
    //}
    // else
    //	timeCurrent	= 0.f;

    ////////////////////BLEND UPDATE//////////////////////////////////////////////

    if (B->timeTotal - B->timeCurrent - END_EPS < B->timeCurrent) // this cool expression sims to work for all cases!
    {
        VERIFY(B->speed > 0.f);
        ths->on_end = true;
    }
    else
    {
        VERIFY(B->speed < 0.f);
        ths->on_begin = true;
    }
}

void anim_script_callback::update(CGameObject& O)
{
    if (!is_set)
        return;
    if (!on_end && !on_begin)
        return;
    O.callback(GameObject::eScriptAnimation)(on_end);
    on_end = false;
    on_begin = false;
}
