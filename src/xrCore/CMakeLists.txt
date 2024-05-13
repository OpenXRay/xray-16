add_library(xrCore SHARED)

target_sources_grouped(
    TARGET xrCore
    NAME "Animation"
    FILES
    Animation/Bone.cpp
    Animation/Bone.hpp
    Animation/BoneEditor.cpp
    Animation/Envelope.cpp
    Animation/Envelope.hpp
    Animation/interp.cpp
    Animation/Motion.cpp
    Animation/Motion.hpp
    Animation/SkeletonMotionDefs.hpp
    Animation/SkeletonMotions.cpp
    Animation/SkeletonMotions.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Common"
    FILES
    ../Common/Common.hpp
    ../Common/Config.hpp
    ../Common/face_smoth_flags.h
    ../Common/FSMacros.hpp
    ../Common/LevelGameDef.h
    ../Common/Noncopyable.hpp
    ../Common/Platform.hpp
    ../Common/OGF_GContainer_Vertices.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Common\\Object"
    FILES
    ../Common/object_broker.h
    ../Common/object_cloner.h
    ../Common/object_comparer.h
    ../Common/object_destroyer.h
    ../Common/object_interfaces.h
    ../Common/object_loader.h
    ../Common/object_saver.h
    ../Common/object_type_traits.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\lz"
    FILES
    LzHuf.cpp
    lzhuf.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\lzo"
    FILES
    Compression/lzo_compressor.cpp
    Compression/lzo_compressor.h
    Compression/rt_compressor9.cpp
    Compression/rt_compressor.cpp
    Compression/rt_compressor.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\ppmd"
    FILES
    Compression/ppmd_compressor.cpp
    Compression/ppmd_compressor.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\ppmd\\common"
    FILES
    Compression/PPMd.h
    Compression/PPMdType.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\ppmd\\core"
    FILES
    Compression/Coder.hpp
    Compression/Model.cpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\ppmd\\core\\allocator"
    FILES
    Compression/SubAlloc.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Compression\\ppmd\\stream"
    FILES
    Compression/compression_ppmd_stream.h
    Compression/compression_ppmd_stream_inline.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Containers"
    FILES
    Containers/AssociativeVectorComparer.hpp
    Containers/AssociativeVector.hpp
    Containers/FixedMap.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Crypto"
    FILES
    Crypto/trivial_encryptor.cpp
    Crypto/trivial_encryptor.h
    Crypto/xr_dsa.cpp
    Crypto/xr_dsa.h
    Crypto/xr_dsa_signer.cpp
    Crypto/xr_dsa_signer.h
    Crypto/xr_dsa_verifyer.cpp
    Crypto/xr_dsa_verifyer.h
    Crypto/xr_sha.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Debug core"
    FILES
    #Debug/MiniDump.cpp
    #Debug/MiniDump.h
    xrDebug.cpp
    xrDebug.h
    xrDebug_macros.h
)

#target_sources_grouped(
#    TARGET xrCore
#    NAME "Debug core\\DxErr"
#    FILES
#    Debug/dxerr.cpp
#    Debug/dxerr.h
#    Debug/DXGetErrorDescription.inl
#    Debug/DXGetErrorString.inl
#    Debug/DXTrace.inl
#)

target_sources_grouped(
    TARGET xrCore
    NAME "Events"
    FILES
    Events/Notifier.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "FMesh"
    FILES
    FMesh.cpp
    FMesh.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "FS"
    FILES
    FileCRC32.cpp
    FileCRC32.h
    FileSystem.cpp
    FileSystem.h
    FileSystem_borland.cpp
    FS.cpp
    FS.h
    FS_impl.h
    FS_internal.h
    LocatorAPI.cpp
    LocatorAPI.h
    LocatorAPI_auth.cpp
    LocatorAPI_defs.cpp
    LocatorAPI_defs.h
    log.cpp
    log.h
    NET_utils.cpp
    net_utils.h
    xr_ini.cpp
    xr_ini.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "FS\\file_stream_reader"
    FILES
    file_stream_reader.cpp
    file_stream_reader.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "FS\\stream_reader"
    FILES
    stream_reader.cpp
    stream_reader.h
    stream_reader_inline.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "intrusive_ptr"
    FILES
    intrusive_ptr.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Kernel"
    FILES
    cdecl_cast.hpp
    FTimer.cpp
    FTimer.h
    ModuleLookup.cpp
    ModuleLookup.hpp
    resource.h
    xr_shortcut.h
    xrCore.cpp
    xrCore.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Kernel\\Common"
    FILES
    ../Common/_d3d_extensions.h
    ../Common/GUID.hpp
    ../Common/LevelStructure.hpp
    ../Common/Util.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Math"
    FILES
    _bitwise.h
    _color.h
    _compressed_normal.cpp
    _compressed_normal.h
    _cylinder.cpp
    _cylinder.h
    _fbox.h
    _fbox2.h
    _flags.h
    _math.cpp
    _math.h
    _matrix.h
    _matrix33.h
    _obb.h
    _plane.h
    _plane2.h
    _quaternion.h
    _random.h
    _rect.h
    _sphere.cpp
    _sphere.h
    _vector2.h
    _vector3d.h
    _vector3d_ext.h
    _vector4.h
    dump_string.cpp
    dump_string.h
    math_constants.h
    vector.h
    xr_types.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Math\\Extensions"
    FILES
    #ChooseTypes.H
    #client_id.h
    _std_extensions.cpp
    _std_extensions.h
    _stl_extensions.h
    clsid.cpp
    clsid.h
    fastdelegate.h
    xr_token.cpp
    xr_token.h
    xr_trims.cpp
    xr_trims.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Math\\Extensions\\buffer_vector"
    FILES
    buffer_vector.h
    buffer_vector_inline.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Math\\RNG"
    FILES
    Math/fast_lc16.cpp
    Math/fast_lc16.hpp
    Math/Random32.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Math\\STLext"
    FILES
    FixedVector.h
    xrPool.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Media"
    FILES
    Media/Image.cpp
    Media/Image.hpp
    Media/ImageJPEG.cpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Memory"
    FILES
    Memory/xalloc.h
    xrMemory.cpp
    xrMemory.h
    #Memory/xrMemory_align.cpp
    #Memory/xrMemory_align.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "OS"
    FILES
    os_clipboard.cpp
    os_clipboard.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "PCH"
    FILES
    stdafx.cpp
    stdafx.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "PostProcess"
    FILES
    PostProcess/PostProcess.cpp
    PostProcess/PostProcess.hpp
    PostProcess/PPInfo.cpp
    PostProcess/PPInfo.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "shared memory/string library"
    FILES
    crc32.cpp
    string_concatenations.cpp
    string_concatenations.h
    string_concatenations_inline.h
    xr_resource.h
    xr_shared.cpp
    xr_shared.h
    xrsharedmem.cpp
    xrsharedmem.h
    xrstring.cpp
    xrstring.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "Text"
    FILES
    Text/StringConversion.cpp
    Text/StringConversion.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Threading"
    FILES
    Threading/Event.cpp
    Threading/Event.hpp
    Threading/Lock.cpp
    Threading/Lock.hpp
    Threading/ParallelFor.hpp
    Threading/ParallelForEach.hpp
    Threading/ScopeLock.cpp
    Threading/ScopeLock.hpp
    Threading/Task.cpp
    Threading/Task.hpp
    Threading/TaskManager.cpp
    Threading/TaskManager.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "Threading\\Util"
    FILES
    Threading/ThreadUtil.cpp
    Threading/ThreadUtil.h
)

target_sources_grouped(
    TARGET xrCore
    NAME "XML"
    FILES
    XML/XMLDocument.cpp
    XML/XMLDocument.hpp
)

target_sources_grouped(
    TARGET xrCore
    NAME "XML\\TinyXML"
    FILES
    XML/tinystr.cpp
    XML/tinystr.h
    XML/tinyxml.cpp
    XML/tinyxmlerror.cpp
    XML/tinyxml.h
    XML/tinyxmlparser.cpp
)

target_include_directories(xrCore
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/tracy/public"

    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals"
    "${CMAKE_SOURCE_DIR}/Externals/mimalloc/include"
)

target_link_libraries(xrCore
    PUBLIC
    pthread
    $<$<PLATFORM_ID:FreeBSD,OpenBSD,NetBSD,DragonFlyBSD>:execinfo>
    SDL3::SDL3

    PRIVATE
    xrMiscMath
    $<$<STREQUAL:${MEMORY_ALLOCATOR},mimalloc>:mimalloc>
    $<$<BOOL:${JPEG_FOUND}>:JPEG::JPEG>
    LZO::LZO
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" AND NOT CMAKE_SYSTEM_NAME STREQUAL "NetBSD" AND NOT WIN32)
    target_link_libraries(xrCore
        PRIVATE
        dl
    )
endif()

target_compile_definitions(xrCore
    PRIVATE
    XRCORE_EXPORTS
    CMAKE_INSTALL_FULL_LIBDIR=\"${CMAKE_INSTALL_FULL_LIBDIR}\"
    CMAKE_INSTALL_FULL_DATAROOTDIR=\"${CMAKE_INSTALL_FULL_DATAROOTDIR}\"
    CI=$ENV{CI}
    GITHUB_ACTIONS=$ENV{GITHUB_ACTIONS}
    GITHUB_RUN_ID=\"$ENV{GITHUB_RUN_ID}\"
    GITHUB_RUN_NUMBER=\"$ENV{GITHUB_RUN_NUMBER}\"
    GITHUB_REPOSITORY=\"$ENV{GITHUB_REPOSITORY}\"
    GIT_INFO_CURRENT_COMMIT=\"${GIT_SHA1}\"
    GIT_INFO_CURRENT_BRANCH=\"${GIT_BRANCH}\"
)

if (MEMORY_ALLOCATOR STREQUAL "mimalloc")
    target_compile_definitions(xrCore PRIVATE USE_MIMALLOC)
else()
    target_compile_definitions(xrCore PRIVATE USE_PURE_ALLOC)
endif()

set_target_properties(xrCore PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrCore
    PRIVATE
    stdafx.h
)

install(TARGETS xrCore LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
