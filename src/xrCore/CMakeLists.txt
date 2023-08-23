project(xrCore)

set(ANIMATION
    "Animation/Bone.cpp"
    "Animation/Bone.hpp"
    "Animation/BoneEditor.cpp"
    "Animation/Envelope.cpp"
    "Animation/Envelope.hpp"
    "Animation/interp.cpp"
    "Animation/Motion.cpp"
    "Animation/Motion.hpp"
    "Animation/SkeletonMotionDefs.hpp"
    "Animation/SkeletonMotions.cpp"
    "Animation/SkeletonMotions.hpp"
)

set(COMMON
    "../Common/Common.hpp"
    "../Common/Config.hpp"
    "../Common/face_smoth_flags.h"
    "../Common/FSMacros.hpp"
    "../Common/LevelGameDef.h"
    "../Common/Noncopyable.hpp"
    "../Common/Platform.hpp"
    "../Common/OGF_GContainer_Vertices.hpp"
)

set(COMMON_OBJECT
    "../Common/object_broker.h"
    "../Common/object_cloner.h"
    "../Common/object_comparer.h"
    "../Common/object_destroyer.h"
    "../Common/object_interfaces.h"
    "../Common/object_loader.h"
    "../Common/object_saver.h"
    "../Common/object_type_traits.h"
)

set(COMPRESSION_LZ
    "LzHuf.cpp"
    "lzhuf.h"
)

set(COMPRESSION_LZO
    "Compression/lzo_compressor.cpp"
    "Compression/lzo_compressor.h"
    "Compression/rt_compressor9.cpp"
    "Compression/rt_compressor.cpp"
    "Compression/rt_compressor.h"
)

set(COMPRESSION_PPMD
    "Compression/ppmd_compressor.cpp"
    "Compression/ppmd_compressor.h"
)

set(COMPRESSION_PPMD_COMMON
    "Compression/PPMd.h"
    "Compression/PPMdType.h"
)

set(COMPRESSION_PPMD_CORE
    "Compression/Coder.hpp"
    "Compression/Model.cpp"
)

set(COMPRESSION_PPMD_CORE_ALLOCATOR
    "Compression/SubAlloc.hpp"
)

set(COMPRESSION_PPMD_STREAM
    "Compression/compression_ppmd_stream.h"
    "Compression/compression_ppmd_stream_inline.h"
)

set(CONTAINERS
    "Containers/AssociativeVectorComparer.hpp"
    "Containers/AssociativeVector.hpp"
    "Containers/FixedMap.h"
)

set(CRYPTO
    "Crypto/trivial_encryptor.cpp"
    "Crypto/trivial_encryptor.h"
    "Crypto/xr_dsa.cpp"
    "Crypto/xr_dsa.h"
    "Crypto/xr_dsa_signer.cpp"
    "Crypto/xr_dsa_signer.h"
    "Crypto/xr_dsa_verifyer.cpp"
    "Crypto/xr_dsa_verifyer.h"
    "Crypto/xr_sha.h"
)

set(DEBUG_CORE
    #"Debug/MiniDump.cpp"
    #"Debug/MiniDump.h"
    "xrDebug.cpp"
    "xrDebug.h"
    "xrDebug_macros.h"
)

#set(DEBUG_CORE_DXERR
#    "Debug/dxerr.cpp"
#    "Debug/dxerr.h"
#    "Debug/DXGetErrorDescription.inl"
#    "Debug/DXGetErrorString.inl"
#    "Debug/DXTrace.inl"
#)

set(EVENTS
    "Events/Notifier.h"
)

set(FMESH
    "FMesh.cpp"
    "FMesh.hpp"
)

set(FS
    "FileCRC32.cpp"
    "FileCRC32.h"
    "FileSystem.cpp"
    "FileSystem.h"
    "FileSystem_borland.cpp"
    "FS.cpp"
    "FS.h"
    "FS_impl.h"
    "FS_internal.h"
    "LocatorAPI.cpp"
    "LocatorAPI.h"
    "LocatorAPI_auth.cpp"
    "LocatorAPI_defs.cpp"
    "LocatorAPI_defs.h"
    "log.cpp"
    "log.h"
    "NET_utils.cpp"
    "net_utils.h"
    "xr_ini.cpp"
    "xr_ini.h"
)

set(FS_FILE_STREAM_READER
    "file_stream_reader.cpp"
    "file_stream_reader.h"
)

set(FS_STREAM_READER
    "stream_reader.cpp"
    "stream_reader.h"
    "stream_reader_inline.h"
)

set(INTRUSIVE_PTR
    "intrusive_ptr.h"
)

set(KERNEL
    "cdecl_cast.hpp"
    "FTimer.cpp"
    "FTimer.h"
    "ModuleLookup.cpp"
    "ModuleLookup.hpp"
    "resource.h"
    "xr_shortcut.h"
    "xrCore.cpp"
    "xrCore.h"
)

set(KERNEL_COMMON
    "../Common/_d3d_extensions.h"
    "../Common/GUID.hpp"
    "../Common/LevelStructure.hpp"
    "../Common/Util.hpp"
)

set(MATH
    "_bitwise.h"
    "_color.h"
    "_compressed_normal.cpp"
    "_compressed_normal.h"
    "_cylinder.cpp"
    "_cylinder.h"
    "_fbox.h"
    "_fbox2.h"
    "_flags.h"
    "_math.cpp"
    "_math.h"
    "_matrix.h"
    "_matrix33.h"
    "_obb.h"
    "_plane.h"
    "_plane2.h"
    "_quaternion.h"
    "_random.h"
    "_rect.h"
    "_sphere.cpp"
    "_sphere.h"
    "_vector2.h"
    "_vector3d.h"
    "_vector3d_ext.h"
    "_vector4.h"
    "dump_string.cpp"
    "dump_string.h"
    "math_constants.h"
    "vector.h"
    "xr_types.h"
)

set(MATH_EXTENSIONS
    #"ChooseTypes.H"
    #"client_id.h"
    "_std_extensions.cpp"
    "_std_extensions.h"
    "_stl_extensions.h"
    "clsid.cpp"
    "clsid.h"
    "fastdelegate.h"
    "xr_token.cpp"
    "xr_token.h"
    "xr_trims.cpp"
    "xr_trims.h"
)

set(MATH_EXTENSIONS_BUFFER_VECTOR
    "buffer_vector.h"
    "buffer_vector_inline.h"
)

set(MATH_RNG
    "Math/Random32.hpp"
)

set(MATH_STLEXT
    "FixedVector.h"
    "xrPool.h"
)

set(MEDIA
    "Media/Image.cpp"
    "Media/Image.hpp"
    "Media/ImageJPEG.cpp"
)

set(MEMORY
    "Memory/xalloc.h"
    "xrMemory.cpp"
    "xrMemory.h"
    #"Memory/xrMemory_align.cpp"
    #"Memory/xrMemory_align.h"
)

set(OS
    "os_clipboard.cpp"
    "os_clipboard.h"
)

set(PCH
    "stdafx.cpp"
    "stdafx.h"
)

set(POSTPROCESS
    "PostProcess/PostProcess.cpp"
    "PostProcess/PostProcess.hpp"
    "PostProcess/PPInfo.cpp"
    "PostProcess/PPInfo.hpp"
)

set(SHARED_MEMORY_STRING_LIBRARY
    "crc32.cpp"
    "string_concatenations.cpp"
    "string_concatenations.h"
    "string_concatenations_inline.h"
    "xr_resource.h"
    "xr_shared.cpp"
    "xr_shared.h"
    "xrsharedmem.cpp"
    "xrsharedmem.h"
    "xrstring.cpp"
    "xrstring.h"
)

set(TEXT
    "Text/StringConversion.cpp"
    "Text/StringConversion.hpp"
)

set(THREADING
    "Threading/Event.cpp"
    "Threading/Event.hpp"
    "Threading/Lock.cpp"
    "Threading/Lock.hpp"
    "Threading/ParallelFor.hpp"
    "Threading/ParallelForEach.hpp"
    "Threading/ScopeLock.cpp"
    "Threading/ScopeLock.hpp"
    "Threading/Task.cpp"
    "Threading/Task.hpp"
    "Threading/TaskManager.cpp"
    "Threading/TaskManager.hpp"
)

set(THREADING_UTIL
    "Threading/ThreadUtil.cpp"
    "Threading/ThreadUtil.h"
)

set(XML
    "XML/XMLDocument.cpp"
    "XML/XMLDocument.hpp"
)

set(XML_TINY_XML
    "XML/tinystr.cpp"
    "XML/tinystr.h"
    "XML/tinyxml.cpp"
    "XML/tinyxmlerror.cpp"
    "XML/tinyxml.h"
    "XML/tinyxmlparser.cpp"
)

source_group("Animation" FILES ${ANIMATION})
source_group("Common" FILES ${COMMON})
source_group("Common\\Object" FILES ${COMMON_OBJECT})
source_group("Compression\\lz" FILES ${COMPRESSION_LZ})
source_group("Compression\\lzo" FILES ${COMPRESSION_LZO})
source_group("Compression\\ppmd" FILES ${COMPRESSION_PPMD})
source_group("Compression\\ppmd\\common" FILES ${COMPRESSION_PPMD_COMMON})
source_group("Compression\\ppmd\\core" FILES ${COMPRESSION_PPMD_CORE})
source_group("Compression\\ppmd\\core\\allocator" FILES ${COMPRESSION_PPMD_CORE_ALLOCATOR})
source_group("Compression\\ppmd\\stream" FILES ${COMPRESSION_PPMD_STREAM})
source_group("Containers" FILES ${CONTAINERS})
source_group("Crypto" FILES ${CRYPTO})
source_group("Debug core" FILES ${DEBUG_CORE})
#source_group("Debug core\\DxErr" FILES ${DEBUG_CORE_DXERR})
source_group("Events" FILES ${EVENTS})
source_group("FMesh" FILES ${FMESH})
source_group("FS" FILES ${FS})
source_group("FS\\file_stream_reader" FILES ${FS_FILE_STREAM_READER})
source_group("FS\\stream_reader" FILES ${FS_STREAM_READER})
source_group("intrusive_ptr" FILES ${INTRUSIVE_PTR})
source_group("Kernel" FILES ${KERNEL})
source_group("Kernel\\Common" FILES ${KERNEL_COMMON})
source_group("Math" FILES ${MATH})
source_group("Math\\Extensions" FILES ${MATH_EXTENSIONS})
source_group("Math\\Extensions\\buffer_vector" FILES ${MATH_EXTENSIONS_BUFFER_VECTOR})
source_group("Math\\RNG" FILES ${MATH_RNG})
source_group("Math\\STLext" FILES ${MATH_STLEXT})
source_group("Media" FILES ${MEDIA})
source_group("Memory" FILES ${MEMORY})
source_group("OS" FILES ${OS})
source_group("PCH" FILES ${PCH})
source_group("PostProcess" FILES ${POSTPROCESS})
source_group("shared memory/string library" FILES ${SHARED_MEMORY_STRING_LIBRARY})
source_group("Text" FILES ${TEXT})
source_group("Threading" FILES ${THREADING})
source_group("Threading\\Util" FILES ${THREADING_UTIL})
source_group("XML" FILES ${XML})
source_group("XML\\TinyXML" FILES ${XML_TINY_XML})

set(SRC_FILES
    ${ANIMATION}
    ${COMMON}
    ${COMMON_OBJECT}
    ${COMPRESSION_LZ}
    ${COMPRESSION_LZO}
    ${COMPRESSION_PPMD}
    ${COMPRESSION_PPMD_COMMON}
    ${COMPRESSION_PPMD_CORE}
    ${COMPRESSION_PPMD_CORE_ALLOCATOR}
    ${COMPRESSION_PPMD_STREAM}
    ${CONTAINERS}
    ${CRYPTO}
    ${EVENTS}
    ${DEBUG_CORE}
    ${FMESH}
    ${FS}
    ${FS_FILE_STREAM_READER}
    ${FS_STREAM_READER}
    ${INTRUSIVE_PTR}
    ${KERNEL}
    ${KERNEL_COMMON}
    ${MATH}
    ${MATH_EXTENSIONS}
    ${MATH_EXTENSIONS_BUFFER_VECTOR}
    ${MATH_RNG}
    ${MATH_STLEXT}
    ${MEDIA}
    ${MEMORY}
    ${OS}
    ${PCH}
    ${POSTPROCESS}
    ${SHARED_MEMORY_STRING_LIBRARY}
    ${TEXT}
    ${THREADING}
    ${THREADING_UTIL}
    ${XML}
    ${XML_TINY_XML}
)

if (PROJECT_PLATFORM_ARM OR PROJECT_PLATFORM_ARM64)
  list(REMOVE_ITEM SRC_FILES "Math/PLC_SSE.cpp")
  list(REMOVE_ITEM SRC_FILES "Math/PLC_SSE.hpp")
endif()

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${SDL2_INCLUDE_DIRS}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/mimalloc/include
    $<$<BOOL:${JPEG_FOUND}>:${JPEG_INCLUDE_DIRS}>
    ${LZO_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    pthread
    $<$<PLATFORM_ID:FreeBSD,OpenBSD,NetBSD,DragonFlyBSD>:execinfo>
    ${SDL2_LIBRARIES}
    PRIVATE
    xrMiscMath
    dl
    $<$<STREQUAL:${MEMORY_ALLOCATOR},mimalloc>:mimalloc>
    $<$<BOOL:${JPEG_FOUND}>:${JPEG_LIBRARIES}>
    ${LZO_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRCORE_EXPORTS
    -DCI=$ENV{CI}
    -DTRAVIS=$ENV{TRAVIS}
    -DTRAVIS_BUILD_ID=$ENV{TRAVIS_BUILD_ID}
    -DTRAVIS_BUILD_NUMBER=$ENV{TRAVIS_BUILD_NUMBER}
    -DTRAVIS_REPO_SLUG=$ENV{TRAVIS_REPO_SLUG}
    -DGITHUB_ACTIONS=$ENV{GITHUB_ACTIONS}
    -DGITHUB_RUN_ID=$ENV{GITHUB_RUN_ID}
    -DGITHUB_RUN_NUMBER=$ENV{GITHUB_RUN_NUMBER}
    -DGITHUB_REPOSITORY=$ENV{GITHUB_REPOSITORY}
    -DGIT_INFO_CURRENT_COMMIT=${GIT_SHA1}
    -DGIT_INFO_CURRENT_BRANCH=${GIT_BRANCH}
    -DCMAKE_INSTALL_FULL_DATAROOTDIR=${CMAKE_INSTALL_FULL_DATAROOTDIR}
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
