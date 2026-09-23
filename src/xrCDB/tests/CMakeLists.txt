add_executable(xrCDBTests CollisionQueries.cpp)
target_include_directories(xrCDBTests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(xrCDBTests PRIVATE xrCDB xrCore xrMiscMath)
target_compile_definitions(xrCDBTests PRIVATE $<$<BOOL:${XRAY_USE_JOLT_CDB}>:XRAY_USE_JOLT_CDB>)

add_test(NAME xrCDB.queries COMMAND xrCDBTests)
add_test(NAME xrCDB.threaded_queries COMMAND xrCDBTests -mt_cdb)
set_tests_properties(xrCDB.queries xrCDB.threaded_queries PROPERTIES TIMEOUT 60)
