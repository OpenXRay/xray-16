////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity_space.h
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace ScriptEntity
{
enum EActionType : u32
{
    eActionTypeMovement = u32(0),
    eActionTypeWatch,
    eActionTypeAnimation,
    eActionTypeSound,
    eActionTypeParticle,
    eActionTypeObject,
    eActionTypeCount,
    eActionTypeRemoved
};
};
