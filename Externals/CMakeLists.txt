add_subdirectory(LuaJIT-proj)

add_subdirectory(luabind)
set_target_properties(xrLuabind PROPERTIES
	INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:xrLuabind,INTERFACE_INCLUDE_DIRECTORIES>
)

if (USE_CXIMAGE)
    add_subdirectory(cximage)
endif()
add_subdirectory(GameSpy)
add_subdirectory(OPCODE)
add_subdirectory(ode)
#add_subdirectory(NVTT)
add_subdirectory(imgui-proj)
