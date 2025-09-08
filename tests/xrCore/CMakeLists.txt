add_executable(
    xrCoreTests
    main.cpp
    xr_ini_test.cpp)

target_link_libraries(xrCoreTests xrCore doctest::doctest)
target_include_directories(xrCoreTests PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xrCoreTests COMMAND xrCoreTests)
# https://github.com/doctest/doctest/blob/master/doc/markdown/configuration.md
target_compile_definitions(xrCoreTests PRIVATE
    DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
    DOCTEST_CONFIG_SUPER_FAST_ASSERTS
)
