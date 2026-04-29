add_subdirectory(xrAPI)

if (WIN32)
    add_subdirectory(xrRender)
    add_subdirectory(xrRenderPC_R4)
endif()
