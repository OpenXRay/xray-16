# CMake generated Testfile for 
# Source directory: C:/Users/Пользователь/xray-16/Externals/mimalloc
# Build directory: C:/Users/Пользователь/xray-16/out/build/x64-Debug/Externals/mimalloc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_api, "mimalloc-test-api")
set_tests_properties(test_api, PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Пользователь/xray-16/Externals/mimalloc/CMakeLists.txt;276;add_test;C:/Users/Пользователь/xray-16/Externals/mimalloc/CMakeLists.txt;0;")
add_test(test_stress, "mimalloc-test-stress")
set_tests_properties(test_stress, PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Пользователь/xray-16/Externals/mimalloc/CMakeLists.txt;277;add_test;C:/Users/Пользователь/xray-16/Externals/mimalloc/CMakeLists.txt;0;")
