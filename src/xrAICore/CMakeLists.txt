project(xrAICore)

set(AI_SRC
    "AISpaceBase.cpp"
    "AISpaceBase.hpp"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_SRC
    "Components/problem_solver.h"
    "Components/problem_solver_inline.h"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SRC
    "Components/condition_state.h"
    "Components/condition_state_inline.h"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SCRIPT_WORLD_STATE_SRC
    "Components/script_world_state.h"
    "Components/script_world_state_script.cpp"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_ABSTRACT_SRC
    "Components/operator_abstract.h"
    "Components/operator_abstract_inline.h"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SRC
    "Components/operator_condition.h"
    "Components/operator_condition_inline.h"
)

set(AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SCRIPT_WORLD_PROPERTY_SRC
    "Components/script_world_property.h"
    "Components/script_world_property_inline.h"
    "Components/script_world_property_script.cpp"
)

set(AI_NAVIGATION_GAME_GRAPH_SRC
    "Navigation/game_graph.h"
    "Navigation/game_graph_inline.h"
    "Navigation/game_graph_script.cpp"
    "Navigation/game_graph_space.h"
)

set(AI_NAVIGATION_GAME_LEVEL_CROSS_TABLE_SRC
    "Navigation/game_level_cross_table.h"
    "Navigation/game_level_cross_table_inline.h"
)

set(AI_NAVIGATION_GRAPH_ABSTRACT_SRC
    "Navigation/graph_abstract.h"
    "Navigation/graph_abstract_inline.h"
    "Navigation/graph_edge.h"
    "Navigation/graph_edge_inline.h"
    "Navigation/graph_vertex.h"
    "Navigation/graph_vertex_inline.h"
)

set(AI_NAVIGATION_LEVEL_GRAPH
    "Navigation/level_graph.cpp"
    "Navigation/level_graph.h"
    "Navigation/level_graph_inline.h"
    "Navigation/level_graph_manager.h"
    "Navigation/level_graph_space.h"
    "Navigation/level_graph_vertex.cpp"
    "Navigation/level_graph_vertex_inline.h"
)

set(AI_NAVIGATION_OBJECT_LOCATION
    "Navigation/ai_object_location.h"
    "Navigation/ai_object_location_impl.h"
    "Navigation/ai_object_location_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_ALGORITHMS_ASTAR
    "Navigation/a_star.h"
    "Navigation/a_star_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_ALGORITHMS_DIJKSTRA
    "Navigation/dijkstra.h"
    "Navigation/dijkstra_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_DATASTORAGES_CONSTRUCTORS
    "Navigation/data_storage_constructor.h"
)

set(AI_NAVIGATION_PATHFINDING_DATASTORAGES_PATH_BUILDERS
    "Navigation/edge_path.h"
    "Navigation/edge_path_inline.h"
    "Navigation/vertex_path.h"
    "Navigation/vertex_path_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_DATASTORAGES_PRIORITY_QUEUES
    "Navigation/data_storage_binary_heap.h"
    "Navigation/data_storage_binary_heap_inline.h"
    "Navigation/data_storage_bucket_list.h"
    "Navigation/data_storage_bucket_list_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_ALLOCATORS
    "Navigation/vertex_allocator_fixed.h"
    "Navigation/vertex_allocator_fixed_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_MANAGERS
    "Navigation/vertex_manager_fixed.h"
    "Navigation/vertex_manager_fixed_inline.h"
    "Navigation/vertex_manager_hash_fixed.h"
    "Navigation/vertex_manager_hash_fixed_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_GRAPH_ENGINE
    "Navigation/graph_engine.h"
    "Navigation/graph_engine_inline.h"
    "Navigation/graph_engine_space.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS
    "Navigation/PathManagers/path_manager.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GAME
    "Navigation/PathManagers/path_manager_game.h"
    "Navigation/PathManagers/path_manager_game_inline.h"
    "Navigation/PathManagers/path_manager_game_level.h"
    "Navigation/PathManagers/path_manager_game_level_inline.h"
    "Navigation/PathManagers/path_manager_game_vertex.h"
    "Navigation/PathManagers/path_manager_game_vertex_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GENERIC
    "Navigation/PathManagers/path_manager_generic.h"
    "Navigation/PathManagers/path_manager_generic_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_LEVEL
    "Navigation/PathManagers/path_manager_level.h"
    "Navigation/PathManagers/path_manager_level_inline.h"
    "Navigation/PathManagers/path_manager_level_flooder.h"
    "Navigation/PathManagers/path_manager_level_flooder_inline.h"
    "Navigation/PathManagers/path_manager_level_nearest_vertex.h"
    "Navigation/PathManagers/path_manager_level_nearest_vertex_inline.h"
    "Navigation/PathManagers/path_manager_level_straight_line.h"
    "Navigation/PathManagers/path_manager_level_straight_line_inline.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_PARAMS
    "Navigation/PathManagers/path_manager_params.h"
    "Navigation/PathManagers/path_manager_params_flooder.h"
    "Navigation/PathManagers/path_manager_params_game_level.h"
    "Navigation/PathManagers/path_manager_params_game_vertex.h"
    "Navigation/PathManagers/path_manager_params_nearest_vertex.h"
    "Navigation/PathManagers/path_manager_params_straight_line.h"
)

set(AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_SOLVER
    "Navigation/PathManagers/path_manager_solver.h"
    "Navigation/PathManagers/path_manager_solver_inline.h"
)

set(AI_NAVIGATION_PATROL_PATH
    "Navigation/PatrolPath/patrol_path.cpp"
    "Navigation/PatrolPath/patrol_path.h"
    "Navigation/PatrolPath/patrol_path_inline.h"
)

set(AI_NAVIGATION_PATROL_PATH_PARAMS
    "Navigation/PatrolPath/patrol_path_params.cpp"
    "Navigation/PatrolPath/patrol_path_params.h"
    "Navigation/PatrolPath/patrol_path_params_inline.h"
    "Navigation/PatrolPath/patrol_path_params_script.cpp"
)

set(AI_NAVIGATION_PATROL_PATH_PATROL_POINT
    "Navigation/PatrolPath/patrol_point.cpp"
    "Navigation/PatrolPath/patrol_point.h"
    "Navigation/PatrolPath/patrol_point_inline.h"
)

set(AI_NAVIGATION_PATROL_PATH_STORAGE
    "Navigation/PatrolPath/patrol_path_storage.cpp"
    "Navigation/PatrolPath/patrol_path_storage.h"
    "Navigation/PatrolPath/patrol_path_storage_inline.h"
)

set(KERNEL_SRC
    "pch.cpp"
    "pch.hpp"
)

source_group("AI" FILES ${AI_SRC})
source_group("AI\\Component\\ProblemSolver" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_SRC})
source_group("AI\\Component\\ProblemSolver\\ConditionState" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SRC})
source_group("AI\\Component\\ProblemSolver\\ConditionState\\ScriptWorldState" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SCRIPT_WORLD_STATE_SRC})
source_group("AI\\Component\\ProblemSolver\\OperatorAbstract" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_ABSTRACT_SRC})
source_group("AI\\Component\\ProblemSolver\\OperatorCondition" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SRC})
source_group("AI\\Component\\ProblemSolver\\OperatorCondition\\ScriptWorldProperty" FILES ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SCRIPT_WORLD_PROPERTY_SRC})
source_group("AI\\Navigation\\GameGraph" FILES ${AI_NAVIGATION_GAME_GRAPH_SRC})
source_group("AI\\Navigation\\GameLevelCrossTable" FILES ${AI_NAVIGATION_GAME_LEVEL_CROSS_TABLE_SRC})
source_group("AI\\Navigation\\GraphAbstract" FILES ${AI_NAVIGATION_GRAPH_ABSTRACT_SRC})
source_group("AI\\Navigation\\LevelGraph" FILES ${AI_NAVIGATION_LEVEL_GRAPH})
source_group("AI\\Navigation\\ObjectLocation" FILES ${AI_NAVIGATION_OBJECT_LOCATION})
source_group("AI\\Navigation\\Pathfinding\\Algorithms\\AStar" FILES ${AI_NAVIGATION_PATHFINDING_ALGORITHMS_ASTAR})
source_group("AI\\Navigation\\Pathfinding\\Algorithms\\Dijkstra" FILES ${AI_NAVIGATION_PATHFINDING_ALGORITHMS_DIJKSTRA})
source_group("AI\\Navigation\\Pathfinding\\DataStorages\\Constructors" FILES ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_CONSTRUCTORS})
source_group("AI\\Navigation\\Pathfinding\\DataStorages\\PathBuilders" FILES ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_PATH_BUILDERS})
source_group("AI\\Navigation\\Pathfinding\\DataStorages\\PriorityQueues" FILES ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_PRIORITY_QUEUES})
source_group("AI\\Navigation\\Pathfinding\\DataStorages\\VertexAllocators" FILES ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_ALLOCATORS})
source_group("AI\\Navigation\\Pathfinding\\DataStorages\\VertexManagers" FILES ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_MANAGERS})
source_group("AI\\Navigation\\Pathfinding\\GraphEngine" FILES ${AI_NAVIGATION_PATHFINDING_GRAPH_ENGINE})
source_group("AI\\Navigation\\Pathfinding\\PathManagers" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS})
source_group("AI\\Navigation\\Pathfinding\\PathManagers\\Game" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GAME})
source_group("AI\\Navigation\\Pathfinding\\PathManagers\\Generic" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GENERIC})
source_group("AI\\Navigation\\Pathfinding\\PathManagers\\Level" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_LEVEL})
source_group("AI\\Navigation\\Pathfinding\\PathManagers\\Params" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_PARAMS})
source_group("AI\\Navigation\\Pathfinding\\PathManagers\\Solver" FILES ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_SOLVER})
source_group("AI\\Navigation\\PatrolPath" FILES ${AI_NAVIGATION_PATROL_PATH})
source_group("AI\\Navigation\\PatrolPath\\Params" FILES ${AI_NAVIGATION_PATROL_PATH_PARAMS})
source_group("AI\\Navigation\\PatrolPath\\PatrolPoint" FILES ${AI_NAVIGATION_PATROL_PATH_PATROL_POINT})
source_group("AI\\Navigation\\PatrolPath\\Storage" FILES ${AI_NAVIGATION_PATROL_PATH_STORAGE})
source_group("Kernel" FILES ${KERNEL_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE}
    ${AI_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_CONDITION_STATE_SCRIPT_WORLD_STATE_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_ABSTRACT_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SRC}
    ${AI_COMPONENTS_PROBLEM_SOLVER_OPERATOR_CONDITION_SCRIPT_WORLD_PROPERTY_SRC}
    ${AI_NAVIGATION_GAME_GRAPH_SRC}
    ${AI_NAVIGATION_GAME_LEVEL_CROSS_TABLE_SRC}
    ${AI_NAVIGATION_GRAPH_ABSTRACT_SRC}
    ${AI_NAVIGATION_LEVEL_GRAPH}
    ${AI_NAVIGATION_OBJECT_LOCATION}
    ${AI_NAVIGATION_PATHFINDING_ALGORITHMS_ASTAR}
    ${AI_NAVIGATION_PATHFINDING_ALGORITHMS_DIJKSTRA}
    ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_CONSTRUCTORS}
    ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_PATH_BUILDERS}
    ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_PRIORITY_QUEUES}
    ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_ALLOCATORS}
    ${AI_NAVIGATION_PATHFINDING_DATASTORAGES_VERTEX_MANAGERS}
    ${AI_NAVIGATION_PATHFINDING_GRAPH_ENGINE}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GAME}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_GENERIC}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_LEVEL}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_PARAMS}
    ${AI_NAVIGATION_PATHFINDING_PATH_MANAGERS_SOLVER}
    ${AI_NAVIGATION_PATROL_PATH}
    ${AI_NAVIGATION_PATROL_PATH_PARAMS}
    ${AI_NAVIGATION_PATROL_PATH_PATROL_POINT}
    ${AI_NAVIGATION_PATROL_PATH_STORAGE}
    ${KERNEL_SRC}
)

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
