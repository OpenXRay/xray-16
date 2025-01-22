add_subdirectory(xrAPI)

# TODO test on Windows
if (OS_WINDOWS)
    #add_subdirectory(xrRenderPC_R1)
    #add_subdirectory(xrRenderPC_R2)
    #add_subdirectory(xrRenderPC_R4)
endif()

add_subdirectory(xrRenderPC_GL)
