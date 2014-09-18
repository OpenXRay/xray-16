#pragma once

namespace Lua {
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

	int __cdecl LuaOut(ELuaMessageType tLuaMessageType, LPCSTR caFormat, ...);
}

