#include "stdafx.h"
#include "global_calculation_data.h"
#include "utils/Shader_xrLC.h"
#include "serialize.h"

global_claculation_data gl_data;

template <class T>
void transfer(const char* name, xr_vector<T>& dest, IReader& F, u32 chunk)
{
    IReader* O = F.open_chunk(chunk);
    size_t count = O ? (O->length() / sizeof(T)) : 0;
    Logger.clMsg("* %16s: %d", name, count);
    if (count)
    {
        dest.reserve(count);
        dest.insert(dest.begin(), (T*)O->pointer(), (T*)O->pointer() + count);
    }
    if (O)
        O->close();
}

extern u32* Surface_Load(char* name, u32& w, u32& h);
extern void Surface_Init();

void global_claculation_data::xrLoad()
{
    string_path N;
    FS.update_path(N, "$game_data$", "shaders_xrlc.xr");
    g_shaders_xrlc = xr_new<Shader_xrLC_LIB>();
    g_shaders_xrlc->Load(N);

    // Load CFORM
    {
        FS.update_path(N, "$level$", "build.cform");
        IReader* fs = FS.r_open("$level$", "build.cform");

        R_ASSERT(fs->find_chunk(0));
        hdrCFORM H;
        fs->r(&H, sizeof(hdrCFORM));
        R_ASSERT(CFORM_CURRENT_VERSION == H.version);

        Fvector* verts = (Fvector*)fs->pointer();
        CDB::TRI* tris = (CDB::TRI*)(verts + H.vertcount);
        RCAST_Model.build(verts, H.vertcount, tris, H.facecount);
        Msg("* Level CFORM: %dK", RCAST_Model.memory() / 1024);

        g_rc_faces.resize(H.facecount);
        R_ASSERT(fs->find_chunk(1));
        fs->r(&*g_rc_faces.begin(), g_rc_faces.size() * sizeof(b_rc_face));

        LevelBB.set(H.aabb);
    }

    {
        slots_data.Load();
    }
    // Lights
    {
        IReader* fs = FS.r_open("$level$", "build.lights");
        IReader* F;
        size_t cnt;
        R_Light* L;

        // rgb
        F = fs->open_chunk(0);
        cnt = F->length() / sizeof(R_Light);
        L = (R_Light*)F->pointer();
        g_lights.rgb.assign(L, L + cnt);
        F->close();

        // hemi
        F = fs->open_chunk(1);
        cnt = F->length() / sizeof(R_Light);
        L = (R_Light*)F->pointer();
        g_lights.hemi.assign(L, L + cnt);
        F->close();

        // sun
        F = fs->open_chunk(2);
        cnt = F->length() / sizeof(R_Light);
        L = (R_Light*)F->pointer();
        g_lights.sun.assign(L, L + cnt);
        F->close();

        FS.r_close(fs);
    }

    // Load level data
    {
        IReader* fs = FS.r_open("$level$", "build.prj");
        IReader* F;

        // Version
        u32 version;
        fs->r_chunk(EB_Version, &version);
        R_ASSERT(XRCL_CURRENT_VERSION == version);

        // Header
        fs->r_chunk(EB_Parameters, &g_params);

        // Load level data
        transfer("materials", g_materials, *fs, EB_Materials);
        transfer("shaders_xrlc", g_shader_compile, *fs, EB_Shaders_Compile);
        post_process_materials(*g_shaders_xrlc, g_shader_compile, g_materials);
// process textures
#ifdef DEBUG
        xr_vector<b_texture> dbg_textures;
#endif
        Logger.Status("Processing textures...");
        {
            Surface_Init();
            F = fs->open_chunk(EB_Textures);
            const size_t tex_count = F->length() / sizeof(b_texture);
            for (size_t t = 0; t < tex_count; t++)
            {
                Logger.Progress(float(t) / float(tex_count));

                b_BuildTexture BT(F);

#ifdef DEBUG
                dbg_textures.push_back(static_cast<b_texture>(BT));
#endif

                // load thumbnail
                pstr N = BT.name;
                if (strchr(N, '.'))
                    *(strchr(N, '.')) = 0;
                xr_strlwr(N);

                if (0 == xr_strcmp(N, "level_lods"))
                {
                    // HACK for merged lod textures
                    BT.dwWidth = 1024;
                    BT.dwHeight = 1024;
                    BT.bHasAlpha = TRUE;
                    BT.pSurface = 0;
                    BT.THM.SetHasSurface(false);
                }
                else
                {
                    xr_strcat(N, sizeof(BT.name), ".thm");
                    IReader* THM = FS.r_open("$game_textures$", N);
                    R_ASSERT2(THM, N);

                    // version
                    u32 version = 0;
                    R_ASSERT(THM->r_chunk(THM_CHUNK_VERSION, &version));
                    // if( version!=THM_CURRENT_VERSION )	FATAL	("Unsupported version of THM file.");

                    // analyze thumbnail information
                    R_ASSERT(THM->find_chunk(THM_CHUNK_TEXTUREPARAM));
                    THM->r(&BT.THM.fmt, sizeof(STextureParams::ETFormat));
                    BT.THM.flags.assign(THM->r_u32());
                    BT.THM.border_color = THM->r_u32();
                    BT.THM.fade_color = THM->r_u32();
                    BT.THM.fade_amount = THM->r_u32();
                    BT.THM.mip_filter = THM->r_u32();
                    BT.THM.width = THM->r_u32();
                    BT.THM.height = THM->r_u32();
                    BOOL bLOD = FALSE;
                    if (N[0] == 'l' && N[1] == 'o' && N[2] == 'd' && N[3] == '\\')
                        bLOD = TRUE;

                    // load surface if it has an alpha channel or has "implicit lighting" flag
                    BT.dwWidth = BT.THM.width;
                    BT.dwHeight = BT.THM.height;
                    BT.bHasAlpha = BT.THM.HasAlphaChannel();
                    BT.pSurface = 0;
                    BT.THM.SetHasSurface(false);
                    if (!bLOD)
                    {
                        if (BT.bHasAlpha || BT.THM.flags.test(STextureParams::flImplicitLighted))
                        {
                            Logger.clMsg("- loading: %s", N);
                            u32 w = 0, h = 0;
                            BT.pSurface = Surface_Load(N, w, h);
                            BT.THM.SetHasSurface(true);
                            R_ASSERT2(BT.pSurface, "Can't load surface");
                            if ((w != BT.dwWidth) || (h != BT.dwHeight))
                                Msg("! THM doesn't correspond to the texture: %dx%d -> %dx%d", BT.dwWidth, BT.dwHeight,
                                    w, h);
                            BT.Vflip();
                        }
                        else
                        {
                            // Free surface memory
                        }
                    }
                }

                // save all the stuff we've created
                g_textures.push_back(BT);
            }
        }
    }
}

void read(INetReader& r, CDB::MODEL& m);

void global_claculation_data::read(INetReader& r)
{
    g_lights.read(r);
    R_ASSERT(!g_shaders_xrlc);
    g_shaders_xrlc = xr_new<Shader_xrLC_LIB>();
    r_pod_vector(r, g_shaders_xrlc->Library());
    r_pod(r, g_params);
    r_pod_vector(r, g_materials);
    r_vector(r, g_textures);
    ::read(r, RCAST_Model);
    r_pod(r, LevelBB);
    slots_data.read(r);
    r_pod_vector(r, g_shader_compile);
    r_pod_vector(r, g_rc_faces);
}

// base_lighting					g_lights; /////////////////////lc
// Shader_xrLC_LIB*				g_shaders_xrlc;////////////////lc
// b_params						g_params;//////////////////////lc
// xr_vector<b_material>			g_materials;///////////////////lc
// xr_vector<b_BuildTexture>		g_textures;////////////////////lc
// CDB::MODEL						RCAST_Model;///////////////////lc

// Fbox							LevelBB;//-----------============
// global_slots_data				slots_data;//-------=============
// xr_vector<b_shader>				g_shader_compile;//-----==========
// xr_vector<b_rc_face>			g_rc_faces;//---------===============

void write(IWriter& w, const CDB::MODEL& m);
void global_claculation_data::write(IWriter& w) const
{
    g_lights.write(w);
    R_ASSERT(g_shaders_xrlc);
    w_pod_vector(w, g_shaders_xrlc->Library());
    w_pod(w, g_params);
    w_pod_vector(w, g_materials);
    w_vector(w, g_textures);
    ::write(w, RCAST_Model);
    w_pod(w, LevelBB);
    slots_data.write(w);
    w_pod_vector(w, g_shader_compile);
    w_pod_vector(w, g_rc_faces);
}
