// xrRender/Shaders/SlangVFSAdapter.h
#pragma once

#include <slang.h>
#include <slang-com-ptr.h>

namespace xray::render
{

// ══════════════════════════════════════════════════════════
//  SLANG VFS ADAPTER FOR X-RAY VIRTUAL FILE SYSTEM
// ══════════════════════════════════════════════════════════
//
//  Bridges Slang's file system interface with X-Ray's VFS
//  - Loads shader includes from loose files AND archives
//  - Supports #pragma once via unique identity
//  - Thread-safe blob creation
//
// ══════════════════════════════════════════════════════════

class SlangVFSAdapter : public ISlangFileSystemExt
{
public:
    SlangVFSAdapter();
    virtual ~SlangVFSAdapter();

    // ═════════════════════════════════════════════════════
    //  ISlangUnknown - COM Interface (Reference Counting)
    // ═════════════════════════════════════════════════════

    SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) SLANG_OVERRIDE;
    SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE;
    SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE;

    // ═════════════════════════════════════════════════════
    //  ISlangCastable
    // ═════════════════════════════════════════════════════

    SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid) SLANG_OVERRIDE;

    // ═════════════════════════════════════════════════════
    //  ISlangFileSystem - Core File Loading
    // ═════════════════════════════════════════════════════

    /// <summary>
    /// Load a shader file from X-Ray VFS
    /// Uses FS.r_open("$game_shaders$", path) to search loose files + archives
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(
        char const* path,
        ISlangBlob** outBlob) SLANG_OVERRIDE;

    // ═════════════════════════════════════════════════════
    //  ISlangFileSystemExt - Extended Operations
    // ═════════════════════════════════════════════════════

    /// <summary>
    /// Get unique identity for a file (used for #pragma once)
    /// Returns the canonical path as the unique identifier
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL getFileUniqueIdentity(
        const char* path,
        ISlangBlob** outUniqueIdentity) SLANG_OVERRIDE;

    /// <summary>
    /// Calculate combined path (resolve relative paths)
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL calcCombinedPath(
        SlangPathType fromPathType,
        const char* fromPath,
        const char* path,
        ISlangBlob** pathOut) SLANG_OVERRIDE;

    /// <summary>
    /// Get path type (file, directory, etc.)
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL getPathType(
        const char* path,
        SlangPathType* pathTypeOut) SLANG_OVERRIDE;

    /// <summary>
    /// Get canonical path
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL getPath(
        PathKind kind,
        const char* path,
        ISlangBlob** outPath) SLANG_OVERRIDE;

    /// <summary>
    /// Clear internal caches (if any)
    /// </summary>
    SLANG_NO_THROW void SLANG_MCALL clearCache() SLANG_OVERRIDE;

    /// <summary>
    /// Enumerate directory contents
    /// </summary>
    SLANG_NO_THROW SlangResult SLANG_MCALL enumeratePathContents(
        const char* path,
        FileSystemContentsCallBack callback,
        void* userData) SLANG_OVERRIDE;

    /// <summary>
    /// Get OS path kind (VFS doesn't map to OS paths)
    /// </summary>
    SLANG_NO_THROW OSPathKind SLANG_MCALL getOSPathKind() SLANG_OVERRIDE;

private:
    /// <summary>
    /// Normalize path separators (/ and \\ to /)
    /// </summary>
    xr_string NormalizePath(const char* path);

    /// <summary>
    /// Create a blob from string data
    /// </summary>
    SlangResult CreateBlob(const char* data, size_t size, ISlangBlob** outBlob);

private:
    uint32_t m_refCount;
};

} // namespace xray::render
