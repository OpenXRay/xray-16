add_library(xrAICore SHARED)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI"
    FILES
    AISpaceBase.cpp
    AISpaceBase.hpp
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver"
    FILES
    Components/problem_solver.h
    Components/problem_solver_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver\\ConditionState"
    FILES
    Components/condition_state.h
    Components/condition_state_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver\\ConditionState\\ScriptWorldState"
    FILES
    Components/script_world_state.h
    Components/script_world_state_script.cpp
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver\\OperatorAbstract"
    FILES
    Components/operator_abstract.h
    Components/operator_abstract_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver\\OperatorCondition"
    FILES
    Components/operator_condition.h
    Components/operator_condition_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Component\\ProblemSolver\\OperatorCondition\\ScriptWorldProperty"
    FILES
    Components/script_world_property.h
    Components/script_world_property_inline.h
    Components/script_world_property_script.cpp
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\GameGraph"
    FILES
    Navigation/game_graph.h
    Navigation/game_graph_inline.h
    Navigation/game_graph_script.cpp
    Navigation/game_graph_space.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\GameLevelCrossTable"
    FILES
    Navigation/game_level_cross_table.h
    Navigation/game_level_cross_table_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\GraphAbstract"
    FILES
    Navigation/graph_abstract.h
    Navigation/graph_abstract_inline.h
    Navigation/graph_edge.h
    Navigation/graph_edge_inline.h
    Navigation/graph_vertex.h
    Navigation/graph_vertex_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\LevelGraph"
    FILES
    Navigation/level_graph.cpp
    Navigation/level_graph.h
    Navigation/level_graph_inline.h
    Navigation/level_graph_manager.h
    Navigation/level_graph_space.h
    Navigation/level_graph_vertex.cpp
    Navigation/level_graph_vertex_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\ObjectLocation"
    FILES
    Navigation/ai_object_location.h
    Navigation/ai_object_location_impl.h
    Navigation/ai_object_location_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\Algorithms\\AStar"
    FILES
    Navigation/a_star.h
    Navigation/a_star_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\Algorithms\\Dijkstra"
    FILES
    Navigation/dijkstra.h
    Navigation/dijkstra_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\DataStorages\\Constructors"
    FILES
    Navigation/data_storage_constructor.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\DataStorages\\PathBuilders"
    FILES
    Navigation/edge_path.h
    Navigation/edge_path_inline.h
    Navigation/vertex_path.h
    Navigation/vertex_path_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\DataStorages\\PriorityQueues"
    FILES
    Navigation/data_storage_binary_heap.h
    Navigation/data_storage_binary_heap_inline.h
    Navigation/data_storage_bucket_list.h
    Navigation/data_storage_bucket_list_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\DataStorages\\VertexAllocators"
    FILES
    Navigation/vertex_allocator_fixed.h
    Navigation/vertex_allocator_fixed_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\DataStorages\\VertexManagers"
    FILES
    Navigation/vertex_manager_fixed.h
    Navigation/vertex_manager_fixed_inline.h
    Navigation/vertex_manager_hash_fixed.h
    Navigation/vertex_manager_hash_fixed_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\GraphEngine"
    FILES
    Navigation/graph_engine.h
    Navigation/graph_engine_inline.h
    Navigation/graph_engine_space.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers"
    FILES
    Navigation/PathManagers/path_manager.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers\\Game"
    FILES
    Navigation/PathManagers/path_manager_game.h
    Navigation/PathManagers/path_manager_game_inline.h
    Navigation/PathManagers/path_manager_game_level.h
    Navigation/PathManagers/path_manager_game_level_inline.h
    Navigation/PathManagers/path_manager_game_vertex.h
    Navigation/PathManagers/path_manager_game_vertex_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers\\Generic"
    FILES
    Navigation/PathManagers/path_manager_generic.h
    Navigation/PathManagers/path_manager_generic_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers\\Level"
    FILES
    Navigation/PathManagers/path_manager_level.h
    Navigation/PathManagers/path_manager_level_inline.h
    Navigation/PathManagers/path_manager_level_flooder.h
    Navigation/PathManagers/path_manager_level_flooder_inline.h
    Navigation/PathManagers/path_manager_level_nearest_vertex.h
    Navigation/PathManagers/path_manager_level_nearest_vertex_inline.h
    Navigation/PathManagers/path_manager_level_straight_line.h
    Navigation/PathManagers/path_manager_level_straight_line_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers\\Params"
    FILES
    Navigation/PathManagers/path_manager_params.h
    Navigation/PathManagers/path_manager_params_flooder.h
    Navigation/PathManagers/path_manager_params_game_level.h
    Navigation/PathManagers/path_manager_params_game_vertex.h
    Navigation/PathManagers/path_manager_params_nearest_vertex.h
    Navigation/PathManagers/path_manager_params_straight_line.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\Pathfinding\\PathManagers\\Solver"
    FILES
    Navigation/PathManagers/path_manager_solver.h
    Navigation/PathManagers/path_manager_solver_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\PatrolPath"
    FILES
    Navigation/PatrolPath/patrol_path.cpp
    Navigation/PatrolPath/patrol_path.h
    Navigation/PatrolPath/patrol_path_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\PatrolPath\\Params"
    FILES
    Navigation/PatrolPath/patrol_path_params.cpp
    Navigation/PatrolPath/patrol_path_params.h
    Navigation/PatrolPath/patrol_path_params_inline.h
    Navigation/PatrolPath/patrol_path_params_script.cpp
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\PatrolPath\\PatrolPoint"
    FILES
    Navigation/PatrolPath/patrol_point.cpp
    Navigation/PatrolPath/patrol_point.h
    Navigation/PatrolPath/patrol_point_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "AI\\Navigation\\PatrolPath\\Storage"
    FILES
    Navigation/PatrolPath/patrol_path_storage.cpp
    Navigation/PatrolPath/patrol_path_storage.h
    Navigation/PatrolPath/patrol_path_storage_inline.h
)

target_sources_grouped(
    TARGET xrAICore
    NAME "Kernel"
    FILES
    pch.cpp
    pch.hpp
)


target_include_directories(xrAICore
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk/include"
)

target_link_libraries(xrAICore
    PRIVATE
    xrCore
    xrEngine
    xrMiscMath
    xrAPI
    xrScriptEngine
)

target_compile_definitions(xrAICore
    PRIVATE
    XRAICORE_EXPORTS
)

set_target_properties(xrAICore PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrAICore
    PRIVATE
    pch.hpp
)

install(TARGETS xrAICore LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
