find_program(NUGET nuget)

set(PACKAGE_NAME "openxray.luajit")
set(PACKAGE_VERSION "2.2.2-alpha")

if(NOT NUGET)
    message(FATAL "CMake could not find the nuget command line tool. Please install it!")
else()
    execute_process(COMMAND 
        ${NUGET} install ${PACKAGE_NAME} -Version ${PACKAGE_VERSION}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Externals
    )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_PATH "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(PLATFORM_PATH "x86")
endif()
set(LUA_LIBRARIES ${CMAKE_BINARY_DIR}/Externals/${PACKAGE_NAME}.${PACKAGE_VERSION}/build/native/lib/${PLATFORM_PATH}/linux/xrLuajit.so)

set(LUA_INCLUDE_DIR ${CMAKE_BINARY_DIR}/Externals/${PACKAGE_NAME}.${PACKAGE_VERSION}/build/native/include)
