include_guard()

set(CMAKE_CONFIGURATION_TYPES
  Debug
  Mixed
  Release
  ReleaseMasterGold
)

set(XRAY_DEFAULT_BUILD_TYPE ReleaseMasterGold)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (is_multi_config)
    if (CMAKE_BUILD_TYPE)
        message(WARNING "CMAKE_BUILD_TYPE is ignored with multi-config generator.")
        unset(CMAKE_BUILD_TYPE)
    endif()
else()
    message(DEBUG "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
    if (NOT CMAKE_BUILD_TYPE)
        message(WARNING "CMAKE_BUILD_TYPE isn't defined, setting it to ${XRAY_DEFAULT_BUILD_TYPE}.")
        set(CMAKE_BUILD_TYPE ${XRAY_DEFAULT_BUILD_TYPE})
    elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        message(WARNING "MinSizeRel is unsupported, CMAKE_BUILD_TYPE is set to ${XRAY_DEFAULT_BUILD_TYPE}.")
        set(CMAKE_BUILD_TYPE ${XRAY_DEFAULT_BUILD_TYPE})
    elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(WARNING "RelWithDebInfo equals Release in OpenXRay. Please, use Release build type directly.")
        set(CMAKE_BUILD_TYPE Release)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif()
unset(is_multi_config)
