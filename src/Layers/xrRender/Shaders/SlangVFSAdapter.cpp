#include "stdafx.h"
#include "SlangVFSAdapter.h"
#include "xrCore/xrMemory.h"

namespace xray::render
{

// ══════════════════════════════════════════════════════════
//  Simple Blob Implementation for File Contents
// ══════════════════════════════════════════════════════════

class FileBlob : public ISlangBlob
{
public:
    FileBlob(const void* data, size_t size)
        : m_refCount(1)
    {
        m_data.resize(size);
        std::memcpy(m_data.data(), data, size);
    }

    // ISlangUnknown
    SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) SLANG_OVERRIDE
    {
        if (uuid == ISlangUnknown::getTypeGuid() ||
            uuid == ISlangBlob::getTypeGuid())
        {
            *outObject = static_cast<ISlangBlob*>(this);
            addRef();
            return SLANG_OK;
        }
        return SLANG_E_NO_INTERFACE;
    }

    SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE
    {
        return ++m_refCount;
    }

    SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE
    {
        uint32_t newCount = --m_refCount;
        if (newCount == 0)
            xr_delete(this);
        return newCount;
    }

    // ISlangBlob
    SLANG_NO_THROW void const* SLANG_MCALL getBufferPointer() SLANG_OVERRIDE
    {
        return m_data.data();
    }

    SLANG_NO_THROW size_t SLANG_MCALL getBufferSize() SLANG_OVERRIDE
    {
        return m_data.size();
    }

private:
    xr_vector<u8> m_data;
    uint32_t m_refCount;
};

// ══════════════════════════════════════════════════════════
//  SlangVFSAdapter Implementation
// ══════════════════════════════════════════════════════════

SlangVFSAdapter::SlangVFSAdapter()
    : m_refCount(1)
{
    // Force VFS to scan shader directories to populate the file index
    xr_vector<char*>* folder = FS.file_list_open("$game_shaders$", "", FS_ListFiles);
    if (folder)
    {
        // Resolve paths through VFS
        for (u32 it = 0; it < folder->size(); it++)
        {
            const char* filename = (*folder)[it];
            string_path fn;
            xr_strcpy(fn, filename);
            FS.update_path(fn, "$game_shaders$", fn);
        }
        FS.file_list_close(folder);
    }
}

SlangVFSAdapter::~SlangVFSAdapter()
{
    // Cleanup handled by reference counting
}

// ═════════════════════════════════════════════════════
//  ISlangUnknown - COM Reference Counting
// ═════════════════════════════════════════════════════

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::queryInterface(
    SlangUUID const& uuid, void** outObject)
{
    if (uuid == ISlangUnknown::getTypeGuid() ||
        uuid == ISlangFileSystem::getTypeGuid() ||
        uuid == ISlangFileSystemExt::getTypeGuid())
    {
        *outObject = static_cast<ISlangFileSystemExt*>(this);
        addRef();
        return SLANG_OK;
    }
    return SLANG_E_NO_INTERFACE;
}

SLANG_NO_THROW uint32_t SLANG_MCALL SlangVFSAdapter::addRef()
{
    return ++m_refCount;
}

SLANG_NO_THROW uint32_t SLANG_MCALL SlangVFSAdapter::release()
{
    uint32_t newCount = --m_refCount;
    if (newCount == 0)
        xr_delete(this);
    return newCount;
}

// ═════════════════════════════════════════════════════
//  ISlangCastable
// ═════════════════════════════════════════════════════

SLANG_NO_THROW void* SLANG_MCALL SlangVFSAdapter::castAs(const SlangUUID& guid)
{
    if (guid == ISlangUnknown::getTypeGuid() ||
        guid == ISlangFileSystem::getTypeGuid() ||
        guid == ISlangFileSystemExt::getTypeGuid())
        return static_cast<ISlangFileSystemExt*>(this);
    return nullptr;
}

// ═════════════════════════════════════════════════════
//  ISlangFileSystem - Core File Loading
// ═════════════════════════════════════════════════════

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::loadFile(
    char const* path,
    ISlangBlob** outBlob)
{
    if (!path || !outBlob)
        return SLANG_E_INVALID_ARG;

    *outBlob = nullptr;

    // Convert forward slashes to backslashes for VFS (Windows path format)
    xr_string vfsPath = path;
    for (char& c : vfsPath)
    {
        if (c == '/')
            c = '\\';
    }

    // Try to open from VFS (searches loose files + archives)
    IReader* reader = FS.r_open("$game_shaders$", vfsPath.c_str());
    if (!reader)
    {
        // Fallback: Try without directory prefix (e.g., "common_policies.h" instead of "r5\common_policies.h")
        // This matches vanilla D3DCompile behavior for shared includes
        size_t lastSlash = vfsPath.find_last_of("/\\");
        if (lastSlash != xr_string::npos)
        {
            xr_string fileNameOnly = vfsPath.substr(lastSlash + 1);
            reader = FS.r_open("$game_shaders$", fileNameOnly.c_str());
        }

        if (!reader)
        {
            return SLANG_E_NOT_FOUND;
        }
    }

    // Read file contents
    size_t fileSize = reader->length();
    const void* fileData = reader->pointer();

    // Create blob
    SlangResult result = CreateBlob((const char*)fileData, fileSize, outBlob);

    // Close reader
    FS.r_close(reader);

    if (!SLANG_SUCCEEDED(result))
    {
        Msg("! [SlangVFSAdapter] Failed to create blob for: %s", vfsPath.c_str());
    }

    return result;
}

// ═════════════════════════════════════════════════════
//  ISlangFileSystemExt - Extended Operations
// ═════════════════════════════════════════════════════

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::getFileUniqueIdentity(
    const char* path,
    ISlangBlob** outUniqueIdentity)
{
    if (!path || !outUniqueIdentity)
        return SLANG_E_INVALID_ARG;

    // Use normalized path as unique identity for #pragma once
    xr_string normalizedPath = NormalizePath(path);

    return CreateBlob(normalizedPath.c_str(), normalizedPath.length() + 1, outUniqueIdentity);
}

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::calcCombinedPath(
    SlangPathType fromPathType,
    const char* fromPath,
    const char* path,
    ISlangBlob** pathOut)
{
    if (!path || !pathOut)
        return SLANG_E_INVALID_ARG;

    // If path is absolute, just return it
    if (path[0] == '/' || path[0] == '\\' || (path[1] == ':'))
    {
        xr_string normalized = NormalizePath(path);
        return CreateBlob(normalized.c_str(), normalized.length() + 1, pathOut);
    }

    // Otherwise, combine with fromPath
    // X-Ray convention: All #includes are relative to shader root (r5\)
    // So we extract the root directory (r5\) from fromPath and combine with path
    xr_string combined;
    if (fromPath && fromPath[0])
    {
        // Extract shader root (everything up to and including "r5\")
        xr_string fromPathStr = fromPath;
        size_t r5Pos = fromPathStr.find("r5");
        if (r5Pos != xr_string::npos)
        {
            // Find the slash after "r5"
            size_t slashAfterR5 = fromPathStr.find_first_of("/\\", r5Pos);
            if (slashAfterR5 != xr_string::npos)
            {
                xr_string shaderRoot = fromPathStr.substr(0, slashAfterR5 + 1);
                combined = shaderRoot + path;
            }
            else
            {
                // No slash after r5, just use path as-is
                combined = path;
            }
        }
        else
        {
            // No r5 in path, fall back to relative to current file's directory
            xr_string fromDir = fromPath;
            size_t lastSlash = fromDir.find_last_of("/\\");
            if (lastSlash != xr_string::npos)
                fromDir = fromDir.substr(0, lastSlash + 1);
            else
                fromDir = "";

            combined = fromDir + path;
        }
    }
    else
    {
        combined = path;
    }

    xr_string normalized = NormalizePath(combined.c_str());
    return CreateBlob(normalized.c_str(), normalized.length() + 1, pathOut);
}

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::getPathType(
    const char* path,
    SlangPathType* pathTypeOut)
{
    if (!path || !pathTypeOut)
        return SLANG_E_INVALID_ARG;

    // Convert forward slashes to backslashes for VFS (Windows path format)
    xr_string vfsPath = path;
    for (char& c : vfsPath)
    {
        if (c == '/')
            c = '\\';
    }

    IReader* reader = FS.r_open("$game_shaders$", vfsPath.c_str());
    if (!reader)
    {
        // Fallback: Try without directory prefix (matches vanilla behavior)
        size_t lastSlash = vfsPath.find_last_of("/\\");
        if (lastSlash != xr_string::npos)
        {
            xr_string fileNameOnly = vfsPath.substr(lastSlash + 1);
            reader = FS.r_open("$game_shaders$", fileNameOnly.c_str());
        }
    }

    if (reader)
    {
        FS.r_close(reader);
        *pathTypeOut = SLANG_PATH_TYPE_FILE;
        return SLANG_OK;
    }

    // VFS doesn't support directory queries easily
    *pathTypeOut = SLANG_PATH_TYPE_FILE;
    return SLANG_E_NOT_FOUND;
}

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::getPath(
    PathKind kind,
    const char* path,
    ISlangBlob** outPath)
{
    if (!path || !outPath)
        return SLANG_E_INVALID_ARG;

    // For VFS, canonical path is just the normalized path
    xr_string normalized = NormalizePath(path);
    return CreateBlob(normalized.c_str(), normalized.length() + 1, outPath);
}

SLANG_NO_THROW void SLANG_MCALL SlangVFSAdapter::clearCache()
{
    // No caching in this implementation
}

SLANG_NO_THROW SlangResult SLANG_MCALL SlangVFSAdapter::enumeratePathContents(
    const char* path,
    FileSystemContentsCallBack callback,
    void* userData)
{
    // VFS doesn't support directory enumeration easily
    // This is not critical for shader compilation
    return SLANG_E_NOT_IMPLEMENTED;
}

SLANG_NO_THROW OSPathKind SLANG_MCALL SlangVFSAdapter::getOSPathKind()
{
    // VFS doesn't map to OS paths
    return OSPathKind::None;
}

// ═════════════════════════════════════════════════════
//  Helper Methods
// ═════════════════════════════════════════════════════

xr_string SlangVFSAdapter::NormalizePath(const char* path)
{
    xr_string normalized = path;

    // Replace backslashes with forward slashes
    for (char& c : normalized)
    {
        if (c == '\\')
            c = '/';
    }

    // Remove redundant slashes
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != xr_string::npos)
    {
        normalized.erase(pos, 1);
    }

    return normalized;
}

SlangResult SlangVFSAdapter::CreateBlob(const char* data, size_t size, ISlangBlob** outBlob)
{
    if (!data || !outBlob)
        return SLANG_E_INVALID_ARG;

    FileBlob* blob = xr_new<FileBlob>(data, size);
    if (!blob)
        return SLANG_E_OUT_OF_MEMORY;

    *outBlob = blob;
    return SLANG_OK;
}

} // namespace xray::render
