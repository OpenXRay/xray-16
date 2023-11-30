project(xrAPI)

set(KERNEL_SRC
    "stdafx.h"
    "stdafx.cpp"
    "xrAPI.cpp"
)

source_group("Kernel" FILES ${KERNEL_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE} ${KERNEL_SRC})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRAPI_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
