////////////////////////////////////////////////////////////////////////////
//	Module 		: script_storage_space.h
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Storage space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace ScriptStorage {
	enum ELuaMessageType {
		eLuaMessageTypeInfo = u32(0),
		eLuaMessageTypeError,
		eLuaMessageTypeMessage,
		eLuaMessageTypeHookCall,
		eLuaMessageTypeHookReturn,
		eLuaMessageTypeHookLine,
		eLuaMessageTypeHookCount,
		eLuaMessageTypeHookTailReturn = u32(-1),
	};
}
