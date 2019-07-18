#pragma once
#ifndef SHADER_XRLC_H
#define SHADER_XRLC_H

struct Shader_xrLC
{
    enum
    {
        flCollision = 1 << 0,
        flRendering = 1 << 1,
        flOptimizeUV = 1 << 2,
        flLIGHT_Vertex = 1 << 3,
        flLIGHT_CastShadow = 1 << 4,
        flLIGHT_Sharp = 1 << 5,
    };
    struct Flags
    {
        u32 bCollision : 1;
        u32 bRendering : 1;
        u32 bOptimizeUV : 1;
        u32 bLIGHT_Vertex : 1;
        u32 bLIGHT_CastShadow : 1;
        u32 bLIGHT_Sharp : 1;
    };

    char Name[128];
    union
    {
        Flags32 m_Flags;
        Flags flags;
    };
    float vert_translucency;
    float vert_ambient;
    float lm_density;

    Shader_xrLC()
    {
        xr_strcpy(Name, "unknown");
        m_Flags.assign(0);
        flags.bCollision = TRUE;
        flags.bRendering = TRUE;
        flags.bOptimizeUV = TRUE;
        flags.bLIGHT_Vertex = FALSE;
        flags.bLIGHT_CastShadow = TRUE;
        flags.bLIGHT_Sharp = TRUE;
        vert_translucency = .5f;
        vert_ambient = .0f;
        lm_density = 1.f;
    }
};

using Shader_xrLCVec = xr_vector<Shader_xrLC>;

class Shader_xrLC_LIB
{
    Shader_xrLCVec library;

public:
    void Load(LPCSTR name)
    {
        auto fs = FS.r_open(name);
        if (NULL == fs)
        {
            string256 inf;
            extern HWND logWindow;
            xr_sprintf(inf, sizeof(inf), "Build failed!\nCan't load shaders library: '%s'", name);
            //          clMsg           (inf);
            //          MessageBox      (logWindow,inf,"Error!",MB_OK|MB_ICONERROR);
            FATAL(inf);
            return;
        };

        const size_t count = fs->length() / sizeof(Shader_xrLC);
        R_ASSERT(fs->length() == count * sizeof(Shader_xrLC));
        library.resize(count);
        fs->r(&library.front(), fs->length());
        FS.r_close(fs);
    }
    bool Save(LPCSTR name)
    {
        auto F = FS.w_open(name);
        if (F)
        {
            F->w(&library.front(), library.size() * sizeof(Shader_xrLC));
            FS.w_close(F);
            return true;
        }
        return false;
    }
    void Unload() { library.clear(); }

    size_t GetID(LPCSTR name) const
    {
        for (auto it = library.begin(); it != library.end(); ++it)
            if (0 == xr_stricmp(name, it->Name))
                return size_t(it - library.begin());
        return size_t(-1);
    }

    Shader_xrLC* Get(LPCSTR name)
    {
        for (auto& shader : library)
            if (0 == xr_stricmp(name, shader.Name))
                return &shader;
        return nullptr;
    }

    Shader_xrLC* Get(size_t id) { return &library[id]; }

    const Shader_xrLC* Get(size_t id) const { return &library[id]; }

    Shader_xrLC* Append(Shader_xrLC* parent = 0)
    {
        library.push_back(parent ? Shader_xrLC(*parent) : Shader_xrLC());
        return &library.back();
    }

    void Remove(LPCSTR name)
    {
        for (auto it = library.begin(); it != library.end(); ++it)
            if (0 == xr_stricmp(name, it->Name))
            {
                library.erase(it);
                break;
            }
    }

    void Remove(int id) { library.erase(library.begin() + id); }

    Shader_xrLCVec& Library() { return library; }

    const Shader_xrLCVec& Library() const { return library; }
};

#ifdef LEVEL_COMPILER

IC void post_process_materials(
    const Shader_xrLC_LIB& shaders, const xr_vector<b_shader>& shader_compile, xr_vector<b_material>& materials)
{
    for (size_t m = 0; m < materials.size(); m++)
    {
        b_material& M = materials[m];

        if (65535 == M.shader_xrlc)
        {
            // No compiler shader
            M.reserved = u16(-1);
            // clMsg    (" *  %20s",shader_render[M.shader].name);
        }
        else
        {
            // clMsg    (" *  %20s / %-20s",shader_render[M.shader].name, shader_compile[M.shader_xrlc].name);
            int id = shaders.GetID(shader_compile[M.shader_xrlc].name);
            if (id < 0)
            {
                Logger.clMsg("ERROR: Shader '%s' not found in library", shader_compile[M.shader].name);
                R_ASSERT(id >= 0);
            }
            M.reserved = u16(id);
        }
    }
}

IC const Shader_xrLC& shader(u16 dwMaterial, const Shader_xrLC_LIB& shaders, const xr_vector<b_material>& materials)
{
    u32 shader_id = materials[dwMaterial].reserved;
    return *(shaders.Get(shader_id));
}

#endif

#endif
