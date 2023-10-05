project(xrScriptEngine)

set(DEBUG_SRC
    "mslotutils.h"
    "script_callStack.cpp"
    "script_callStack.hpp"
    "script_debugger.cpp"
    "script_debugger.hpp"
    "script_debugger_messages.hpp"
    "script_debugger_threads.cpp"
    "script_debugger_threads.hpp"
    "script_lua_helper.cpp"
    "script_lua_helper.hpp"
)

set(ENGINE_SRC
    "BindingsDumper.cpp"
    "BindingsDumper.hpp"
    "Functor.hpp"
    "script_engine.cpp"
    "script_engine.hpp"
    "script_space_forward.hpp"
    "ScriptEngineScript.cpp"
    "ScriptEngineScript.hpp"
)

set(EXPORT_SRC
    "ScriptExporter.cpp"
    "ScriptExporter.hpp"
    "ScriptExportMacros.hpp"
)

set(KERNEL_SRC
    "DebugMacros.hpp"
    "pch.cpp"
    "pch.hpp"
    "ScriptEngineConfig.hpp"
    "xrScriptEngine.cpp"
    "xrScriptEngine.hpp"
)

set(LUA_STUDIO_SRC
    "LuaStudio/Config.hpp"
    "LuaStudio/Defines.hpp"
    "LuaStudio/LuaStudio.cpp"
    "LuaStudio/LuaStudio.hpp"
)

set(LUA_STUDIO_BACKEND_SRC
    "LuaStudio/Backend/Backend.hpp"
    "LuaStudio/Backend/Engine.hpp"
    "LuaStudio/Backend/Interfaces.hpp"
    "LuaStudio/Backend/LibraryLinkage.hpp"
    "LuaStudio/Backend/World.hpp"
)

set(PROCESS_SRC
    "script_process.cpp"
    "script_process.hpp"
)

set(SCRIPT_CALLBACK_EX_SRC
    "script_callback_ex.h"
)

set(THREAD_SRC
    "script_thread.cpp"
    "script_thread.hpp"
)

set(THREAD_STACK_TRACKER_SRC
    "script_stack_tracker.cpp"
    "script_stack_tracker.hpp"
)

source_group("Debug" FILES ${DEBUG_SRC})
source_group("Engine" FILES ${ENGINE_SRC})
source_group("Export" FILES ${EXPORT_SRC})
source_group("Kernel" FILES ${KERNEL_SRC})
source_group("LuaStudio" FILES ${LUA_STUDIO_SRC})
source_group("LuaStudio\\Backend" FILES ${LUA_STUDIO_BACKEND_SRC})
source_group("Process" FILES ${PROCESS_SRC})
source_group("ScriptCallbackEx" FILES ${SCRIPT_CALLBACK_EX_SRC})
source_group("Thread" FILES ${THREAD_SRC})
source_group("Thread\\StackTracker" FILES ${THREAD_STACK_TRACKER_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE}
    ${DEBUG_SRC}
    ${ENGINE_SRC}
    ${EXPORT_SRC}
    ${KERNEL_SRC}
    ${LUA_STUDIO_SRC}
    ${LUA_STUDIO_BACKEND_SRC}
    ${PROCESS_SRC}
    ${SCRIPT_CALLBACK_EX_SRC}
    ${THREAD_SRC}
    ${THREAD_STACK_TRACKER_SRC}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/xrLuaFix
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrAPI
    xrCore
    xrLuaFix
    xrLuabind
    $<$<BOOL:${STATIC_BUILD}>:xrGame>
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
