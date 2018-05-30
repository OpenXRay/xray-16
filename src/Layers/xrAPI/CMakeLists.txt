include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include)

add_library(xrAPI SHARED "xrAPI.cpp" "stdafx.h")

set_target_properties(xrAPI PROPERTIES PREFIX "")