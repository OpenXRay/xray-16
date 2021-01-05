project(xrQSlim)

set(SRC_FILES
    "geom3d.h"
    "mat2.h"
    "mat2.cpp"
    "mat3.h"
    "mat3.cpp"
    "mat4.h"
    "mat4.cpp"
    "mixmops.h"
    "mixmops.cpp"
    "mixvops.h"
    "MxBlock2.h"
    "MxBlock.h"
    "MxBlockModel.h"
    "MxBlockModel.cpp"
    "MxDefines.h"
    "MxDynBlock.h"
    "MxGeom3D.h"
    "MxGeom3D.cpp"
    "MxGeoPrims.h"
    "MxHeap.h"
    "MxHeap.cpp"
    "MxMat2.h"
    "MxMat3.h"
    "MxMat3-jacobi.cpp"
    "MxMat4-jacobi.cpp"
    "MxMat4.h"
    "MxMatrix.h"
    "MxMatrix.cpp"
    "MxPropSlim.h"
    "MxPropSlim.cpp"
    "MxQMetric3.h"
    "MxQMetric3.cpp"
    "MxQMetric.h"
    "MxQMetric.cpp"
    "MxQSlim.h"
    "MxQSlim.cpp"
    "MxStdModel.h"
    "MxStdModel.cpp"
    "MxStdSlim.h"
    "MxStdSlim.cpp"
    "MxVec3.h"
    "MxVec4.h"
    "MxVector.h"
    "stdafx.h"
    "vec2.h"
    "vec3.h"
    "vec4.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    -DXR_QSLIM_EXPORTS
)
