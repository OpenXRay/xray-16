#pragma once

#include <slang.h>
#include <slang-com-ptr.h>

namespace xray::render
{

class SlangVFSAdapter : public ISlangFileSystemExt
{
public:
    SlangVFSAdapter();
    virtual ~SlangVFSAdapter();

    SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) SLANG_OVERRIDE;
    SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE;
    SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE;

    SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid) SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(
        char const* path,
        ISlangBlob** outBlob) SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL getFileUniqueIdentity(
        const char* path,
        ISlangBlob** outUniqueIdentity) SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL calcCombinedPath(
        SlangPathType fromPathType,
        const char* fromPath,
        const char* path,
        ISlangBlob** pathOut) SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL getPathType(
        const char* path,
        SlangPathType* pathTypeOut) SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL getPath(
        PathKind kind,
        const char* path,
        ISlangBlob** outPath) SLANG_OVERRIDE;

    SLANG_NO_THROW void SLANG_MCALL clearCache() SLANG_OVERRIDE;

    SLANG_NO_THROW SlangResult SLANG_MCALL enumeratePathContents(
        const char* path,
        FileSystemContentsCallBack callback,
        void* userData) SLANG_OVERRIDE;

    SLANG_NO_THROW OSPathKind SLANG_MCALL getOSPathKind() SLANG_OVERRIDE;

private:
    xr_string NormalizePath(const char* path);

    SlangResult CreateBlob(const char* data, size_t size, ISlangBlob** outBlob);

private:
    uint32_t m_refCount;
};

}
