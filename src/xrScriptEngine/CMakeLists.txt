add_library(xrScriptEngine SHARED)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Debug"
    FILES
    mslotutils.h
    script_callStack.cpp
    script_callStack.hpp
    script_debugger.cpp
    script_debugger.hpp
    script_debugger_messages.hpp
    script_debugger_threads.cpp
    script_debugger_threads.hpp
    script_lua_helper.cpp
    script_lua_helper.hpp
    script_profiler.cpp
    script_profiler.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Engine"
    FILES
    BindingsDumper.cpp
    BindingsDumper.hpp
    Functor.hpp
    script_engine.cpp
    script_engine.hpp
    script_space_forward.hpp
    ScriptEngineScript.cpp
    ScriptEngineScript.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Export"
    FILES
    ScriptExporter.cpp
    ScriptExporter.hpp
    ScriptExportMacros.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Kernel"
    FILES
    DebugMacros.hpp
    pch.cpp
    pch.hpp
    ScriptEngineConfig.hpp
    xrScriptEngine.cpp
    xrScriptEngine.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "LuaStudio"
    FILES
    LuaStudio/Config.hpp
    LuaStudio/Defines.hpp
    LuaStudio/LuaStudio.cpp
    LuaStudio/LuaStudio.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "LuaStudio\\Backend"
    FILES
    LuaStudio/Backend/Backend.hpp
    LuaStudio/Backend/Engine.hpp
    LuaStudio/Backend/Interfaces.hpp
    LuaStudio/Backend/LibraryLinkage.hpp
    LuaStudio/Backend/World.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Process"
    FILES
    script_process.cpp
    script_process.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "ScriptCallbackEx"
    FILES
    script_callback_ex.h
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Thread"
    FILES
    script_thread.cpp
    script_thread.hpp
)

target_sources_grouped(
    TARGET xrScriptEngine
    NAME "Thread\\StackTracker"
    FILES
    script_stack_tracker.cpp
    script_stack_tracker.hpp
)

target_include_directories(xrScriptEngine
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals"
)

target_link_libraries(xrScriptEngine
    PUBLIC
    xrLuabind

    PRIVATE
    xrAPI
    xrCore
    xrLuaFix
)

target_compile_definitions(xrScriptEngine
    PRIVATE
    XRSCRIPTENGINE_EXPORTS
    # Uncomment next string for debug script engine
    #CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    #CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS
)

set_target_properties(xrScriptEngine PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrScriptEngine
    PRIVATE
    pch.hpp
)

install(TARGETS xrScriptEngine LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
