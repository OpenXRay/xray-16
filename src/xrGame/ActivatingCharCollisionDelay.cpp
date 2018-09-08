#include "StdAfx.h"

#include "ActivatingCharCollisionDelay.h"
#include "CharacterPhysicsSupport.h"
#include "PHMovementControl.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
activating_character_delay::activating_character_delay(CCharacterPhysicsSupport* char_support_)
    : char_support(*char_support_), activate_time(Device.dwTimeGlobal + delay)
{
    VERIFY(char_support_);
    VERIFY(char_support.movement());
    VERIFY(!char_support.movement()->CharacterExist());
}
bool activating_character_delay::active()
{
    VERIFY(char_support.movement());
    return !char_support.movement()->CharacterExist();
}
void activating_character_delay::update()
{
    if (!active())
        return;

    if (Device.dwTimeGlobal < activate_time)
        return;

    if (do_position_correct())
        char_support.CreateCharacter();

    activate_time = Device.dwTimeGlobal + delay;
}

bool activating_character_delay::do_position_correct()
{
    CPHMovementControl* m = char_support.movement();
    VERIFY(m);

    IGameObject* obj = m->ParentObject();
#ifdef DEBUG
    CEntityAlive* e_alife = smart_cast<CEntityAlive*>(obj);
    VERIFY(e_alife);
    VERIFY(!e_alife->PPhysicsShell());
    VERIFY(e_alife->g_Alive());
#endif
    VERIFY(obj);
    Fvector sv_pos = obj->Position();
    bool ret = char_support.CollisionCorrectObjPos();
    if (!ret)
        obj->Position().set(sv_pos);
#if 0
	else
	{
		DBG_OpenCashedDraw();
		DBG_DrawMatrix( obj->XFORM(), 1.f );
		Fmatrix m = obj->XFORM();
		m.c = sv_pos;
		DBG_DrawMatrix( m, 0.5f );
		DBG_DrawLine( obj->Position(), m.c, color_xrgb( 255, 255, 255 ) );
		DBG_ClosedCashedDraw(50000);
	}
#endif
    return ret;
}
