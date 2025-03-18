#pragma once

#include "IRenderDetailModel.h"

namespace xray::render::RENDER_NAMESPACE
{
class ECORE_API CDetail : public IRender_DetailModel
{
private:
    void transfer_indices(u16* iDest, u32 iOffset);
public:
    void Load(IReader* S);
    void Optimize();
    virtual void Unload();

    virtual void transfer(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset);
    virtual void transfer(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset, float du, float dv);
    virtual ~CDetail();
};
} // namespace xray::render::RENDER_NAMESPACE
