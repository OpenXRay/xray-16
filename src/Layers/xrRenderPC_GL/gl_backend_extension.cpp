#include "stdafx.h"
#include "gl_backend_extension.h"

namespace xray::render::RENDER_NAMESPACE
{
void CBackend::RenderInstanced(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC, GLsizei instanceCount)
{
    GLenum Topology = TranslateTopology(T);
    u32 iIndexCount = GetIndexCount(T, PC);

    stat.render.calls++;
    stat.render.verts += countV;
    stat.render.polys += PC;
    stat.r.s_details.add(countV*instanceCount);

    CHK_GL(glDrawElementsInstancedBaseVertex(Topology, iIndexCount, GL_UNSIGNED_SHORT, (void*)(startI * sizeof(GLushort)), instanceCount, baseV));

    PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}
}
