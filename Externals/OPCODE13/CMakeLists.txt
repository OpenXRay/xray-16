add_library(xrOPCODE)
file(GLOB opcode_sources CONFIGURE_DEPENDS "*.cpp" "Ice/*.cpp")
target_sources(xrOPCODE PRIVATE ${opcode_sources})
target_include_directories(xrOPCODE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(xrOPCODE PUBLIC BAN_OPCODE_AUTOLINK
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:ICE_NO_DLL>
    PRIVATE OPCODE_EXPORTS _ALLOW_KEYWORD_MACROS)
set_target_properties(xrOPCODE PROPERTIES PREFIX "" OUTPUT_NAME "OPCODE")
if (BUILD_SHARED_LIBS)
    install(TARGETS xrOPCODE LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
