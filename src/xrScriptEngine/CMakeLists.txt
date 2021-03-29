project(xrScriptEngine)

set(SRC_FILES
    "BindingsDumper.cpp"
    "BindingsDumper.hpp"
    "DebugMacros.hpp"
    "Functor.hpp"
    "mslotutils.h"
    "pch.cpp"
    "pch.hpp"
    "script_callback_ex.h"
    "script_callStack.cpp"
    "script_callStack.hpp"
    "script_debugger.cpp"
    "script_debugger.hpp"
    "script_debugger_messages.hpp"
    "script_debugger_threads.cpp"
    "script_debugger_threads.hpp"
    "ScriptEngineConfig.hpp"
    "script_engine.cpp"
    "script_engine.hpp"
    "ScriptEngineScript.cpp"
    "ScriptEngineScript.hpp"
    "ScriptExporter.cpp"
    "ScriptExporter.hpp"
    "ScriptExportMacros.hpp"
    "script_lua_helper.cpp"
    "script_lua_helper.hpp"
    "script_process.cpp"
    "script_process.hpp"
    "script_space_forward.hpp"
    "script_stack_tracker.cpp"
    "script_stack_tracker.hpp"
    "script_thread.cpp"
    "script_thread.hpp"
    "xrScriptEngine.cpp"
    "xrScriptEngine.hpp"
    "LuaStudio/Config.hpp"
    "LuaStudio/Defines.hpp"
    "LuaStudio/LuaStudio.cpp"
    "LuaStudio/LuaStudio.hpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${SDL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrAPI
    xrCore
    xrLuabind
    ${LUA_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRSCRIPTENGINE_EXPORTS
    # Uncomment next string for debug script engine
    #-DCONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    #-DCONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "pch.hpp"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
