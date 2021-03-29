project(xrAICore)

set(SRC_FILES
    "AISpaceBase.hpp"
    "AISpaceBase.cpp"
    "pch.hpp"
    "pch.cpp"
    "Components/condition_state_inline.h"
    "Components/condition_state.h"
    "Components/operator_abstract_inline.h"
    "Components/operator_abstract.h"
    "Components/operator_condition_inline.h"
    "Components/operator_condition.h"
    "Components/problem_solver_inline.h"
    "Components/problem_solver.h"
    "Components/script_world_property_inline.h"
    "Components/script_world_property.h"
    "Components/script_world_property_script.cpp"
    "Components/script_world_state.h"
    "Components/script_world_state_script.cpp"
    "Navigation/ai_object_location.h"
    "Navigation/ai_object_location_impl.h"
    "Navigation/ai_object_location_inline.h"
    "Navigation/a_star.h"
    "Navigation/a_star_inline.h"
    "Navigation/data_storage_binary_heap.h"
    "Navigation/data_storage_binary_heap_inline.h"
    "Navigation/data_storage_bucket_list.h"
    "Navigation/data_storage_bucket_list_inline.h"
    "Navigation/data_storage_constructor.h"
    "Navigation/dijkstra.h"
    "Navigation/dijkstra_inline.h"
    "Navigation/edge_path.h"
    "Navigation/edge_path_inline.h"
    "Navigation/game_graph.h"
    "Navigation/game_graph_inline.h"
    "Navigation/game_graph_script.cpp"
    "Navigation/game_graph_space.h"
    "Navigation/game_level_cross_table.h"
    "Navigation/game_level_cross_table_inline.h"
    "Navigation/graph_abstract.h"
    "Navigation/graph_abstract_inline.h"
    "Navigation/graph_edge.h"
    "Navigation/graph_edge_inline.h"
    "Navigation/graph_engine.h"
    "Navigation/graph_engine_inline.h"
    "Navigation/graph_engine_space.h"
    "Navigation/graph_vertex.h"
    "Navigation/graph_vertex_inline.h"
    "Navigation/level_graph.cpp"
    "Navigation/level_graph.h"
    "Navigation/level_graph_inline.h"
    "Navigation/level_graph_manager.h"
    "Navigation/level_graph_space.h"
    "Navigation/level_graph_vertex.cpp"
    "Navigation/level_graph_vertex_inline.h"
    "Navigation/manager_builder_allocator_constructor.h"
    "Navigation/PathManagers"
    "Navigation/PatrolPath"
    "Navigation/vertex_allocator_fixed.h"
    "Navigation/vertex_allocator_fixed_inline.h"
    "Navigation/vertex_manager_fixed.h"
    "Navigation/vertex_manager_fixed_inline.h"
    "Navigation/vertex_manager_hash_fixed.h"
    "Navigation/vertex_manager_hash_fixed_inline.h"
    "Navigation/vertex_path.h"
    "Navigation/vertex_path_inline.h"
    "Navigation/PathManagers/path_manager_game.h"
    "Navigation/PathManagers/path_manager_game_inline.h"
    "Navigation/PathManagers/path_manager_game_level.h"
    "Navigation/PathManagers/path_manager_game_level_inline.h"
    "Navigation/PathManagers/path_manager_game_vertex.h"
    "Navigation/PathManagers/path_manager_game_vertex_inline.h"
    "Navigation/PathManagers/path_manager_generic.h"
    "Navigation/PathManagers/path_manager_generic_inline.h"
    "Navigation/PathManagers/path_manager.h"
    "Navigation/PathManagers/path_manager_level_flooder.h"
    "Navigation/PathManagers/path_manager_level_flooder_inline.h"
    "Navigation/PathManagers/path_manager_level.h"
    "Navigation/PathManagers/path_manager_level_inline.h"
    "Navigation/PathManagers/path_manager_level_nearest_vertex.h"
    "Navigation/PathManagers/path_manager_level_nearest_vertex_inline.h"
    "Navigation/PathManagers/path_manager_level_straight_line.h"
    "Navigation/PathManagers/path_manager_level_straight_line_inline.h"
    "Navigation/PathManagers/path_manager_params_flooder.h"
    "Navigation/PathManagers/path_manager_params_game_level.h"
    "Navigation/PathManagers/path_manager_params_game_vertex.h"
    "Navigation/PathManagers/path_manager_params.h"
    "Navigation/PathManagers/path_manager_params_nearest_vertex.h"
    "Navigation/PathManagers/path_manager_params_straight_line.h"
    "Navigation/PathManagers/path_manager_solver.h"
    "Navigation/PathManagers/path_manager_solver_inline.h"
    "Navigation/PatrolPath/patrol_path.h"
    "Navigation/PatrolPath/patrol_path.cpp"
    "Navigation/PatrolPath/patrol_path_inline.h"
    "Navigation/PatrolPath/patrol_path_params.h"
    "Navigation/PatrolPath/patrol_path_params.cpp"
    "Navigation/PatrolPath/patrol_path_params_inline.h"
    "Navigation/PatrolPath/patrol_path_params_script.cpp"
    "Navigation/PatrolPath/patrol_path_storage.h"
    "Navigation/PatrolPath/patrol_path_storage.cpp"
    "Navigation/PatrolPath/patrol_path_storage_inline.h"
    "Navigation/PatrolPath/patrol_point.h"
    "Navigation/PatrolPath/patrol_point.cpp"
    "Navigation/PatrolPath/patrol_point_inline.h"
)

#foreach(header ${SRC_FILES})
#    if (${header} MATCHES "_inline.h$")
#      list(REMOVE_ITEM SRC_FILES ${header})
#    endif()
#endforeach()

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${CMAKE_SOURCE_DIR}/sdk/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrEngine
    xrMiscMath
    xrAPI
    xrScriptEngine
    xrLuabind
    ${LUA_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRAICORE_EXPORTS
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
