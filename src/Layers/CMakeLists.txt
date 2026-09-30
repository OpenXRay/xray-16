add_subdirectory(xrAPI)

if (WIN32)
    add_subdirectory(xrRenderPC_R4)
endif()

add_subdirectory(xrRenderPC_GL)
# Compile the SDL-backed Vulkan layer on desktop too so platform WSI code is
# covered by the normal Windows, Linux and macOS build matrix.
add_subdirectory(xrRenderVK)
