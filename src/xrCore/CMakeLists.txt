set(SOURCES
  clsid.cpp
  cpuid.cpp
  crc32.cpp
  dump_string.cpp
  file_stream_reader.cpp
  FileSystem.cpp
  FileSystem_borland.cpp
  FMesh.cpp
  FS.cpp
  FTimer.cpp
  LocatorAPI.cpp
  LocatorAPI_auth.cpp
  LocatorAPI_defs.cpp
  log.cpp
  LzHuf.cpp
  lzo_compressor.cpp
  Model.cpp
  ModuleLookup.cpp
  NET_utils.cpp
  os_clipboard.cpp
  ppmd_compressor.cpp
  stdafx.cpp
  stream_reader.cpp
  string_concatenations.cpp
  xr_ini.cpp
  xr_shared.cpp
  xr_token.cpp
  xr_trims.cpp
  xrCore.cpp
  xrDebug.cpp
  xrMemory.cpp
  xrsharedmem.cpp
  xrstring.cpp
  _compressed_normal.cpp
  _math.cpp
  _sphere.cpp
  _std_extensions.cpp
#  clsid.cpp
#  cpuid.cpp
#  crc32.cpp
#  doug_lea_allocator.cpp
#  dump_string.cpp
#  FMesh.cpp
#  FS.cpp
#  FTimer.cpp
#  LocatorAPI.cpp
#  LocatorAPI_auth.cpp
#  LocatorAPI_defs.cpp
#  log.cpp
#  LzHuf.cpp
#  memory_allocation_stats.cpp
#  memory_monitor.cpp
#  memory_usage.cpp
#  Model.cpp
#  NET_utils.cpp
#  os_clipboard.cpp
#  ppmd_compressor.cpp
#  stdafx.cpp
#  string_concatenations.cpp
#  xrCore.cpp
#  ModuleLookup.cpp
#  xrDebug.cpp
#  xrMemory.cpp
#  xrMemory_debug.cpp
#  xrMemory_POOL.cpp
#  xrsharedmem.cpp
#  xrstring.cpp
#  Xr_ini.cpp
#  xr_shared.cpp
#  xr_trims.cpp
#  _compressed_normal.cpp
#  _math.cpp
#  _sphere.cpp
#  _std_extensions.cpp
)

set(HEADERS
  buffer_vector.h
  buffer_vector_inline.h
  cdecl_cast.hpp
  client_id.h
  clsid.h
  Coder.hpp
  compression_ppmd_stream.h
  compression_ppmd_stream_inline.h
  cpuid.h
  dump_string.h
  fastdelegate.h
  file_stream_reader.h
  FileSystem.h
  FixedVector.h
  FMesh.hpp
  FS.h
  FS_impl.h
  FS_internal.h
  FTimer.h
  intrusive_ptr.h
  LocatorAPI.h
  LocatorAPI_defs.h
  log.h
  lzhuf.h
  lzo_compressor.h
  math_constants.h
  ModuleLookup.hpp
  net_utils.h
  os_clipboard.h
  PPMd.h
  ppmd_compressor.h
  PPMdType.h
  resource.h
  stdafx.h
  Stream_Reader.h
  stream_reader_inline.h
  string_concatenations.h
  string_concatenations_inline.h
  SubAlloc.hpp
  vector.h
  xr_ini.h
  xr_resource.h
  xr_shared.h
  xr_shortcut.h
  xr_token.h
  xr_trims.h
  xr_vector_defs.h
  xrCore.h
  xrCore_benchmark_macros.h
  xrDebug.h
  xrDebug_macros.h
  xrMemory.h
  xrPool.h
  xrsharedmem.h
  xrstring.h
  _bitwise.h
  _color.h
  _compressed_normal.h
  _cylinder.h
  _fbox.h
  _fbox2.h
  _flags.h
  _math.h
  _matrix.h
  _matrix33.h
  _obb.h
  _plane.h
  _plane2.h
  _quaternion.h
  _random.h
  _rect.h
  _sphere.h
  _std_extensions.h
  _stl_extensions.h
  _types.h
  _vector2.h
  _vector3d.h
  _vector3d_ext.h
  _vector4.h
)

set(ANIMATION_SOURCES
  Animation/Bone.cpp
  Animation/Envelope.cpp
  Animation/interp.cpp
  Animation/Motion.cpp
  Animation/SkeletonMotions.cpp
)

set(ANIMATION_HEADERS
  Animation/Bone.hpp
  Animation/Envelope.hpp
  Animation/Motion.hpp
  Animation/SkeletonMotionDefs.hpp
  Animation/SkeletonMotions.hpp
)

set(COMPRESSION_SOURCES
  Compression/lzo_compressor.cpp
  Compression/rt_compressor.cpp
  Compression/rt_compressor9.cpp
)

set(COMPRESSION_HEADERS
  Compression/lzo_compressor.h
  Compression/rt_compressor.h
)

set(CONTAINERS_HEADER
  Containers/AssociativeVector.hpp
  Containers/AssociativeVectorComparer.hpp
)

set(CRYPTO_SOURCES
  Crypto/xr_dsa.cpp
  Crypto/xr_dsa_signer.cpp
  Crypto/xr_dsa_verifyer.cpp
  Crypto/xr_sha.cpp
)

set(CRYPTO_HEADERS
  Crypto/xr_dsa.h
  Crypto/xr_dsa_signer.h
  Crypto/xr_dsa_verifyer.h
  Crypto/xr_sha.h
)

set(DEBUG_SOURCES
  #Debug/dxerr.cpp
  #Debug/MiniDump.cpp
  #Debug/StackTrace.cpp
)

set(DEBUG_HEADERS
  #Debug/dxerr.h
  #Debug/DXGetErrorDescription.inl
  #Debug/MiniDump.h
  #Debug/StackTrace.h
  #Debug/SymbolEngine.h
  #Debug/DXGetErrorString.inl
)

set(IO_SOURCES
#  IO/DirectoryLookup_posix.cpp
#  IO/DirectoryLookup_win.cpp
)

set(IO_HEADERS
#  IO/DirectoryLookup.hpp
)

set(MATH_SOURCES
  Math/PLC_SSE.cpp
#  Math/Skin4W_MT.cpp
#  Math/SkinXW_SSE.cpp
  Math/MathUtil.cpp
)

set(MATH_HEADERS
  Math/PLC_SSE.hpp
  Math/Random32.hpp
  Math/Skin4W_MT.hpp
  Math/SkinXW_SSE.hpp
  Math/MathUtil.hpp
)

set(MEDIA_SOURCES
  Media/Image.cpp
)

set(MEDIA_HEADERS
  Media/Image.hpp
)

set(POST_PROCESS_SOURCES
  PostProcess/PostProcess.cpp
  PostProcess/PPInfo.cpp
)

set(POST_PROCESS_HEADERS
  PostProcess/PostProcess.hpp
  PostProcess/PPInfo.hpp
)

#ptmalloc3/malloc.c
#ptmalloc3/sysdeps/win32/win32.c

#ptmalloc3/malloc-2.8.3.h

set(TEXT_SOURCES
  Text/MbHelpers.cpp
)

set(TEXT_HEADERS
  Text/MbHelpers.h
)

set(THREADING_SOURCES
#  Threading/Event.cpp
#  Threading/ttapi.cpp
  Threading/Lock.cpp
)

set(THREADING_HEADERS
  Threading/Event.hpp
  Threading/ttapi.h
  Threading/Lock.hpp
)

set(XML_SOURCES
  XML/XMLDocument.cpp
)

set(XML_HEADERS
  XML/XMLDocument.hpp
)

set(COMMON_HEADERS
  ../Common/GUID.hpp
  ../Common/LevelStructure.hpp
  ../Common/Noncopyable.hpp
  ../Common/object_broker.h
  ../Common/object_cloner.h
  ../Common/object_comparer.h
  ../Common/object_destroyer.h
  ../Common/object_interfaces.h
  ../Common/object_loader.h
  ../Common/object_saver.h
  ../Common/object_type_traits.h
  ../Common/Util.hpp
#  ../Common/_d3d_extensions.h
)

include_directories("." ${CMAKE_SOURCE_DIR} ${SDL2_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${LZO_INCLUDE_DIR})

add_library(xrCore SHARED ${SOURCES} ${HEADERS}
    ${ANIMATION_SOURCES}
    ${CRYPTO_SOURCES}
    ${DEBUG_SOURCES}
    ${COMPRESSION_SOURCES}
    ${MATH_SOURCES}
    ${MEDIA_SOURCES}
    ${POST_PROCESS_SOURCES}
    ${TEXT_SOURCES}
    ${THREADING_SOURCES}
    ${IO_SOURCES}
)

set_target_properties(xrCore PROPERTIES PREFIX "")
target_link_libraries(xrCore ${SDL2_LIBRARY} ${OPENSSL_LIBRARIES} ${LZO_LIBRARIES})
