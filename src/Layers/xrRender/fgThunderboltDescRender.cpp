#include "stdafx.h"
#include "Layers/xrRender/fgThunderboltDescRender.h"

namespace xray::render::fg
{
void fgThunderboltDescRender::Copy(IThunderboltDescRender& _in) { *this = *((fgThunderboltDescRender*)&_in); }
void fgThunderboltDescRender::CreateModel(LPCSTR m_name)
{
    IReader* F = nullptr;
    F = FS.r_open("$game_meshes$", m_name);
    R_ASSERT2(F, "Empty 'lightning_model'.");
    l_model = RImplementation.model_CreateDM(F);
    FS.r_close(F);
}

void fgThunderboltDescRender::DestroyModel() { RImplementation.model_Delete(l_model); }
} // namespace xray::render::fg
