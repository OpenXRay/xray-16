project(xrGameSpy)

set(KERNEL_SRC
    "stdafx.cpp"
    "stdafx.h"
    "xrGameSpy.cpp"
    "xrGameSpy.h"
    "xrGameSpy_MainDefs.h"
)

set(WRAPPER_SRC
    "GameSpy_ATLAS.cpp"
    "GameSpy_ATLAS.h"
    "GameSpy_Available.cpp"
    "GameSpy_Available.h"
    "GameSpy_Browser.cpp"
    "GameSpy_Browser.h"
    "GameSpy_BrowsersWrapper.cpp"
    "GameSpy_BrowsersWrapper.h"
    "GameSpy_Full.cpp"
    "GameSpy_Full.h"
    "GameSpy_GCD_Client.cpp"
    "GameSpy_GCD_Client.h"
    "GameSpy_GCD_Server.cpp"
    "GameSpy_GCD_Server.h"
    "GameSpy_GP.cpp"
    "GameSpy_GP.h"
    "GameSpy_HTTP.cpp"
    "GameSpy_HTTP.h"
    "GameSpy_Keys.h"
    "GameSpy_Patching.cpp"
    "GameSpy_Patching.h"
    "GameSpy_QR2.cpp"
    "GameSpy_QR2.h"
    "GameSpy_SAKE.cpp"
    "GameSpy_SAKE.h"
)

source_group("Kernel" FILES ${KERNEL_SRC})
source_group("Wrapper" FILES ${WRAPPER_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE} ${KERNEL_SRC} ${WRAPPER_SRC})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/xrServerEntities
    ${CMAKE_SOURCE_DIR}/Externals/GameSpy/src
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${CMAKE_SOURCE_DIR}/Externals/ode/include
    ${CMAKE_SOURCE_DIR}/Externals/OpenAutomate/inc
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    GameSpy-oxr
    ${SDL2_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRGAMESPY_EXPORTS
    -DXRAY_DISABLE_GAMESPY_WARNINGS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

if (NOT STATIC_BUILD)
    install(TARGETS ${PROJECT_NAME} LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()
