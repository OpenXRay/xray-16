add_executable(xrCDBTests CollisionQueries.cpp CacheFailures.cpp ConstructionLifecycle.cpp ObjectSpace.cpp FrustumPruning.cpp DynamicQueries.cpp)
target_include_directories(xrCDBTests PRIVATE "${XRAY_SOURCE_DIR}/src" "${XRAY_SOURCE_DIR}/Externals/ode/include")
target_link_libraries(xrCDBTests PRIVATE xrCDB xrCore xrMiscMath xrAPI xrPhysics xrEngine xrMaterialSystem xrScriptEngine)
target_compile_definitions(xrCDBTests PRIVATE $<$<BOOL:${XRAY_USE_JOLT_CDB}>:XRAY_USE_JOLT_CDB>)
add_test(NAME xrCDB.queries COMMAND xrCDBTests)
add_test(NAME xrCDB.threaded_queries COMMAND xrCDBTests -mt_cdb)
set_tests_properties(xrCDB.queries xrCDB.threaded_queries PROPERTIES TIMEOUT 60)

# Run explicitly in an optimized build; timings are informational, never a
# CTest threshold. The fixture is synthetic and does not require game assets.
add_executable(xrCDBBenchmark Benchmark.cpp)
target_include_directories(xrCDBBenchmark PRIVATE "${XRAY_SOURCE_DIR}/src")
target_link_libraries(xrCDBBenchmark PRIVATE xrCDB xrCore)
target_compile_definitions(xrCDBBenchmark PRIVATE $<$<BOOL:${XRAY_USE_JOLT_CDB}>:XRAY_USE_JOLT_CDB>)
