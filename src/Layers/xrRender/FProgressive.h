// FProgressive.h: interface for the FProgressive class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "FVisual.h"

struct FSlideWindowItem;

namespace xray::render::fg
{
class FProgressive : public Fvisual
{
protected:
    FSlideWindowItem nSWI;
    FSlideWindowItem* xSWI;
    u32 last_lod;

public:
    FProgressive();
    virtual ~FProgressive();
    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

    // Accessor for slide window data (used by framegraph renderer for LOD selection)
    const FSlideWindowItem& GetSWI() const { return nSWI; }

private:
    FProgressive(const FProgressive& other);
    void operator=(const FProgressive& other);
};
} // namespace xray::render::fg
