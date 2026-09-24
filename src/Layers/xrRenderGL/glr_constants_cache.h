#pragma once

namespace xray::render::RENDER_NAMESPACE
{
#ifdef XR_PLATFORM_WEB
class UniformShadow
{
public:
    bool Update(GLuint program, GLuint location, const void* data, u32 size)
    {
        if (program >= programs.size())
            programs.resize(program + 1);
        auto& slots = programs[program];
        if (location >= slots.size())
            slots.resize(location + 1);
        Slot& slot = slots[location];
        if (slot.size == size && memcmp(slot.data, data, size) == 0)
            return false;
        slot.size = size;
        memcpy(slot.data, data, size);
        return true;
    }

private:
    struct Slot
    {
        u32 size{};
        u8 data[sizeof(Fmatrix)];
    };
    xr_vector<xr_vector<Slot>> programs;
};

inline UniformShadow g_uniformShadow;
#endif

class ECORE_API R_constants
{
private:
    // fp, non-array versions
    ICF void set(R_constant* C, R_constant_load& L, const Fmatrix& A)
    {
        VERIFY(RC_float == C->type);
#ifdef XR_PLATFORM_WEB
        if (!g_uniformShadow.Update(L.program, L.location, &A, sizeof(A)))
            return;
#endif
        Fvector4 it[4];
        switch (L.cls)
        {
        case RC_2x4:
            it[0].set(A._11, A._21, A._31, A._41);
            it[1].set(A._12, A._22, A._32, A._42);
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniformMatrix4x2fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
            else
                CHK_GL(glUniformMatrix4x2fv(L.location, 1, GL_TRUE, (float*)&it));
            break;

        case RC_3x4:
            it[0].set(A._11, A._21, A._31, A._41);
            it[1].set(A._12, A._22, A._32, A._42);
            it[2].set(A._13, A._23, A._33, A._43);
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniformMatrix4x3fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
            else
                CHK_GL(glUniformMatrix4x3fv(L.location, 1, GL_TRUE, (float*)&it));
            break;

        case RC_4x4:
            it[0].set(A._11, A._21, A._31, A._41);
            it[1].set(A._12, A._22, A._32, A._42);
            it[2].set(A._13, A._23, A._33, A._43);
            it[3].set(A._14, A._24, A._34, A._44);
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniformMatrix4fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
            else
                CHK_GL(glUniformMatrix4fv(L.location, 1, GL_TRUE, (float*)&it));
            break;

        default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", C->name.c_str());
#else
            NODEFAULT;
#endif
        }
    }

    ICF void set(R_constant* C, R_constant_load& L, const Fvector4& A)
    {
        VERIFY(RC_float == C->type);
#ifdef XR_PLATFORM_WEB
        if (!g_uniformShadow.Update(L.program, L.location, &A, sizeof(A)))
            return;
#endif
        switch (L.cls)
        {
        case RC_1x2:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform2fv(L.program, L.location, 1, (float*)&A));
            else
                CHK_GL(glUniform2fv(L.location, 1, (float*)&A));
            break;

        case RC_1x3:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform3fv(L.program, L.location, 1, (float*)&A));
            else
                CHK_GL(glUniform3fv(L.location, 1, (float*)&A));
            break;

        case RC_1x4:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform4fv(L.program, L.location, 1, (float*)&A));
            else
                CHK_GL(glUniform4fv(L.location, 1, (float*)&A));
            break;

        default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", C->name.c_str());
#else
            NODEFAULT;
#endif
        }
    }

    ICF void set(R_constant* C, R_constant_load& L, float x, float y, float z, float w)
    {
        VERIFY(RC_float == C->type);
#ifdef XR_PLATFORM_WEB
        const Fvector4 value{ x, y, z, w };
        if (!g_uniformShadow.Update(L.program, L.location, &value, sizeof(value)))
            return;
#endif
        switch (L.cls)
        {
        case RC_1x2:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform2f(L.program, L.location, x, y));
            else
                CHK_GL(glUniform2f(L.location, x, y));
            break;

        case RC_1x3:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform3f(L.program, L.location, x, y, z));
            else
                CHK_GL(glUniform3f(L.location, x, y, z));
            break;

        case RC_1x4:
            if (GLAD_GL_ARB_separate_shader_objects)
                CHK_GL(glProgramUniform4f(L.program, L.location, x, y, z, w));
            else
                CHK_GL(glUniform4f(L.location, x, y, z, w));
            break;

        default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", C->name.c_str());
#else
            NODEFAULT;
#endif
        }
    }

    // scalars, non-array versions
    ICF void set(R_constant* C, R_constant_load& L, float A)
    {
        VERIFY(RC_float == C->type);
        VERIFY(RC_1x1 == L.cls);
#ifdef XR_PLATFORM_WEB
        if (!g_uniformShadow.Update(L.program, L.location, &A, sizeof(A)))
            return;
#endif
        if (GLAD_GL_ARB_separate_shader_objects)
            CHK_GL(glProgramUniform1f(L.program, L.location, A));
        else
            CHK_GL(glUniform1f(L.location, A));
    }

    ICF void set(R_constant* C, R_constant_load& L, int A)
    {
        VERIFY(RC_int == C->type);
        VERIFY(RC_1x1 == L.cls);
#ifdef XR_PLATFORM_WEB
        if (!g_uniformShadow.Update(L.program, L.location, &A, sizeof(A)))
            return;
#endif
        if (GLAD_GL_ARB_separate_shader_objects)
            CHK_GL(glProgramUniform1i(L.program, L.location, A));
        else
            CHK_GL(glUniform1i(L.location, A));
    }

public:
    // fp, non-array versions
    ICF void set(R_constant* C, const Fmatrix& A)
    {
        if (C->destination & RC_dest_pixel) { set(C, C->ps, A); }
        if (C->destination & RC_dest_vertex) { set(C, C->vs, A); }
        if (C->destination & RC_dest_geometry) { set(C, C->gs, A); }
        if (C->destination & RC_dest_all) { set(C, C->pp, A); }
    }

    ICF void set(R_constant* C, const Fvector4& A)
    {
        if (C->destination & RC_dest_pixel) { set(C, C->ps, A); }
        if (C->destination & RC_dest_vertex) { set(C, C->vs, A); }
        if (C->destination & RC_dest_geometry) { set(C, C->gs, A); }
        if (C->destination & RC_dest_all) { set(C, C->pp, A); }
    }

    ICF void set(R_constant* C, float x, float y, float z, float w)
    {
        if (C->destination & RC_dest_pixel) { set(C, C->ps, x, y, z, w); }
        if (C->destination & RC_dest_vertex) { set(C, C->vs, x, y, z, w); }
        if (C->destination & RC_dest_geometry) { set(C, C->gs, x, y, z, w); }
        if (C->destination & RC_dest_all) { set(C, C->pp, x, y, z, w); }
    }

    // scalars, non-array versions
    ICF void set(R_constant* C, float A)
    {
        if (C->destination & RC_dest_pixel) { set(C, C->ps, A); }
        if (C->destination & RC_dest_vertex) { set(C, C->vs, A); }
        if (C->destination & RC_dest_geometry) { set(C, C->gs, A); }
        if (C->destination & RC_dest_all) { set(C, C->pp, A); }
    }

    ICF void set(R_constant* C, int A)
    {
        if (C->destination & RC_dest_pixel) { set(C, C->ps, A); }
        if (C->destination & RC_dest_vertex) { set(C, C->vs, A); }
        if (C->destination & RC_dest_geometry) { set(C, C->gs, A); }
        if (C->destination & RC_dest_all) { set(C, C->pp, A); }
    }

    // fp, array versions
    ICF void seta(R_constant* C, u32 e, const Fmatrix& A)
    {
        R_constant_load L;
        if (C->destination & RC_dest_pixel) { L = C->ps; }
        if (C->destination & RC_dest_vertex) { L = C->vs; }
        if (C->destination & RC_dest_geometry) { L = C->gs; }
        if (C->destination & RC_dest_all) { L = C->pp; }
        L.location += e;
        set(C, L, A);
    }

    ICF void seta(R_constant* C, u32 e, const Fvector4& A)
    {
        R_constant_load L;
        if (C->destination & RC_dest_pixel) { L = C->ps; }
        if (C->destination & RC_dest_vertex) { L = C->vs; }
        if (C->destination & RC_dest_geometry) { L = C->gs; }
        if (C->destination & RC_dest_all) { L = C->pp; }
        L.location += e;
        set(C, L, A);
    }

    ICF void seta(R_constant* C, u32 e, float x, float y, float z, float w)
    {
        R_constant_load L;
        if (C->destination & RC_dest_pixel) { L = C->ps; }
        if (C->destination & RC_dest_vertex) { L = C->vs; }
        if (C->destination & RC_dest_geometry) { L = C->gs; }
        if (C->destination & RC_dest_all) { L = C->pp; }
        L.location += e;
        set(C, L, x, y, z, w);
    }

#ifdef XR_PLATFORM_WEB
    ICF void seta(R_constant* C, u32 e, const Fvector4* values, u32 count)
    {
        VERIFY(RC_float == C->type);
        R_constant_load L;
        if (C->destination & RC_dest_pixel) { L = C->ps; }
        if (C->destination & RC_dest_vertex) { L = C->vs; }
        if (C->destination & RC_dest_geometry) { L = C->gs; }
        if (C->destination & RC_dest_all) { L = C->pp; }
        L.location += e;
        for (u32 i = 0; i < count; ++i)
            g_uniformShadow.Update(L.program, L.location + i, &values[i], sizeof(Fvector4));
        CHK_GL(glUniform4fv(L.location, count, reinterpret_cast<const float*>(values)));
    }
#endif

    // TODO: OGL: Implement constant caching through UBOs
    ICF void flush() { }
};
} // namespace xray::render::RENDER_NAMESPACE
