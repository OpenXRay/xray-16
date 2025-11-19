#include "stdafx.h"
#include "Layers/xrRender/DetailManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "Layers/xrRender/BufferUtils.h"

namespace xray::render::RENDER_NAMESPACE
{
namespace detail_manager
{
extern const int quant;
//extern const int c_hdr;
}

void CDetailManager::hw_Load_Shaders()
{
    // Create shader to access constant storage
    ref_shader S;
    S.create("details" DELIMITER "set");
    R_constant_table& T0 = *S->E[0]->passes[0]->constants;
    R_constant_table& T1 = *S->E[1]->passes[0]->constants;
    hwc_consts = T0.get("consts");
    hwc_wave = T0.get("wave");
    hwc_wind = T0.get("dir2D");
    hwc_array = T0.get("array");
    hwc_s_consts = T1.get("consts");
    hwc_s_xform = T1.get("xform");
    hwc_s_array = T1.get("array");
}

void CDetailManager::hw_Render(CBackend& cmd_list)
{
    using namespace detail_manager;

    if (uniformBufferObject.id == GL_NONE)
        RCache.uniformBufferObjectGenerate(uniformBufferObject);

    // Render-prepare
    //	Update timer
    //	Can't use Device.fTimeDelta since it is smoothed! Don't know why, but smoothed value looks more choppy!
    float fDelta = Device.fTimeGlobal - m_global_time_old;
    if (fDelta < 0 || fDelta > 1)
    {
        fDelta = 0.03f;
    }
    m_global_time_old = Device.fTimeGlobal;

    m_time_rot_1 += PI_MUL_2 * fDelta / swing_current.rot1;
    m_time_rot_2 += PI_MUL_2 * fDelta / swing_current.rot2;
    m_time_pos += fDelta * swing_current.speed;

    glm::vec4 wind1 = normalize(glm::vec4(_sin(m_time_rot_1), 0, _cos(m_time_rot_1), 0)) * swing_current.amp1;
    glm::vec4 wind2 = normalize(glm::vec4(_sin(m_time_rot_2), 0, _cos(m_time_rot_2), 0)) * swing_current.amp2;

    // Setup geometry and DMA
    cmd_list.set_Geometry(hw_Geom);

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;

    // Wave0
    float scale = 1.f / float(quant);

    //environmentDetailUbo[0].xform = Device.mFullTransform;
    memcpy(&environmentDetailData[0].xform, &Device.mFullTransform, sizeof(glm::mat4));
    //environmentDetailUbo[0].xformView;
    environmentDetailData[0].consts = glm::vec4(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    environmentDetailData[0].scale = glm::vec4(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    //environmentDetailUbo[0].bias;
    environmentDetailData[0].wind = wind1;
    environmentDetailData[0].wave = glm::vec4(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos) / PI_MUL_2;
    environmentDetailData[0].sun = glm::vec3(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z) * 0.5f;

    //

    //environmentDetailUbo[1].xform = Device.mFullTransform;
    memcpy(&environmentDetailData[1].xform, &Device.mFullTransform, sizeof(glm::mat4));
    //environmentDetailUbo[1].xformView;
    environmentDetailData[1].consts = glm::vec4(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    environmentDetailData[1].scale = glm::vec4(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    //environmentDetailUbo[1].bias;
    environmentDetailData[1].wind = wind2;
    environmentDetailData[1].wave = glm::vec4(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos) / PI_MUL_2;
    environmentDetailData[1].sun = glm::vec3(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z) * 0.5f;

    //

    //environmentDetailUbo[2].xform = Device.mFullTransform;
    memcpy(&environmentDetailData[2].xform, &Device.mFullTransform, sizeof(glm::mat4));
    //environmentDetailUbo[2].xformView;
    environmentDetailData[2].consts = glm::vec4(scale, scale, scale, 1.f);
    environmentDetailData[2].scale = glm::vec4(scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient);
    //environmentDetailUbo[2].bias;
    environmentDetailData[2].wind = wind2;
    environmentDetailData[2].wave = glm::vec4(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos) / PI_MUL_2;
    environmentDetailData[2].sun = glm::vec3(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z) * 0.5f;

    RCache.uniformBufferObjectPushToDevice(uniformBufferObject, uniformBufferObject.size, &environmentDetailData);

    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, 3, uniformBufferObject.id, 0, sizeof(EnvironmentDetailData)));
    hw_Render_dump(cmd_list, 1, 0);

    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, 3, uniformBufferObject.id, sizeof(EnvironmentDetailData), sizeof(EnvironmentDetailData)));
    hw_Render_dump(cmd_list, 2, 0);

    CHK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, 3, uniformBufferObject.id, sizeof(EnvironmentDetailData)*2, sizeof(EnvironmentDetailData)));
    hw_Render_dump(cmd_list, 0, 1);
}

void CDetailManager::hw_Render_dump(CBackend& cmd_list, u32 var_id, u32 lod_id)
{
    static shared_str strArray("array");

    RImplementation.BasicStats.DetailCount = 0;

    // Matrices and offsets
    u32 vOffset = 0;
    u32 iOffset = 0;

    vis_list& list = m_visibles [var_id];

    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
    Fvector c_sun, c_ambient, c_hemi;
    c_sun.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z);
    c_sun.mul(.5f);
    c_ambient.set(desc.ambient.x, desc.ambient.y, desc.ambient.z);
    c_hemi.set(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z);

    // Iterate
    for (u32 O = 0; O < objects.size(); O++)
    {
        CDetail& Object = *objects [O];
        xr_vector<SlotItemVec*>& vis = list [O];
        if (!vis.empty())
        {
            for (u32 iPass = 0; iPass < Object.shader->E[lod_id]->passes.size(); ++iPass)
            {
                // TODO register only once
                RCache.uniformBufferObjectRegisterWithProgram(Object.shader->E[lod_id]->passes[iPass]->vs, "EnvironmentDetailUBO", 3, uniformBufferObject);

                cmd_list.set_Element(Object.shader->E[lod_id], iPass);
                cmd_list.apply_lmaterial();

                ref_constant constArray = cmd_list.get_c(strArray);
                VERIFY(constArray);

                u32 dwBatch = 0;
                xr_vector<glm::vec4> uniformBuffer;
                uniformBuffer.reserve(hw_BatchSize*4);

                for (auto items : vis)
                {
                    for (auto& instance : *items)
                    {
                        u32 base = dwBatch * 4;

                        // Build matrix ( 3x4 matrix, last row - color )
                        float scale = instance->scale_calculated;
                        Fmatrix& M = instance->mRotY;

                        uniformBuffer.emplace_back(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
                        uniformBuffer.emplace_back(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
                        uniformBuffer.emplace_back(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

                        // Build color
                        // R2 only needs hemisphere
                        uniformBuffer.emplace_back(instance->c_sun, instance->c_sun, instance->c_sun, instance->c_hemi);

                        dwBatch ++;
                        if (dwBatch == hw_BatchSize)
                        {
                            // flush
                            RImplementation.BasicStats.DetailCount += dwBatch;
                            u32 dwCNT_verts = dwBatch * Object.number_vertices;
                            u32 dwCNT_prims = dwBatch * Object.number_indices / 3;

                            cmd_list.set_uniforms(constArray->vs.program, constArray->vs.location, uniformBuffer);
                            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, Object.number_indices, GL_UNSIGNED_SHORT, (void*)(iOffset * sizeof(GLushort)), dwBatch, vOffset);

                            cmd_list.stat.r.s_details.add(dwCNT_verts);
                            uniformBuffer.clear();

                            // restart
                            dwBatch = 0;
                        }
                    }
                }
                // flush if nessecary
                if (dwBatch)
                {
                    RImplementation.BasicStats.DetailCount += dwBatch;
                    u32 dwCNT_verts = dwBatch * Object.number_vertices;
                    u32 dwCNT_prims = dwBatch * Object.number_indices / 3;

                    cmd_list.set_uniforms(constArray->vs.program, constArray->vs.location, uniformBuffer);
                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, Object.number_indices, GL_UNSIGNED_SHORT, (void*)(iOffset * sizeof(GLushort)), dwBatch, vOffset);
                    cmd_list.stat.r.s_details.add(dwCNT_verts);
                }
            }
        }
        vOffset += Object.number_vertices;
        iOffset += Object.number_indices;
    }
}
} // namespace xray::render::RENDER_NAMESPACE
