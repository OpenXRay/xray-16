////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_explosive.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger explosive class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameObject.h"
#include "Explosive.h"

class CExplosive;
class CGameObject;
class CAI_Stalker;

class CDangerExplosive
{
public:
    const CExplosive* m_grenade;
    const CGameObject* m_game_object;
    CAI_Stalker* m_reactor;
    u32 m_time;

public:
    IC CDangerExplosive(const CExplosive* grenade, const CGameObject* game_object, CAI_Stalker* reactor, u32 time);
    IC bool operator==(const CExplosive* grenade) const;
    bool operator==(const u16& id) const;
};

#include "danger_explosive_inline.h"
