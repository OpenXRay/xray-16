#include "stdafx.h"

#include "xrLight_Implicit.h"
#include "xrLight_ImplicitDeflector.h"
#include "xrLight_ImplicitRun.h"
#include "light_point.h"
#include "xrDeflector.h"
#include "xrLC_GlobalData.h"
#include "xrFace.h"
#include "xrLight_ImplicitCalcGlobs.h"
#include "net_task_callback.h"
#include "xrCDB/xrCDB.h"
#include "xrCore/Threading/Lock.hpp"

extern "C" bool XR_IMPORT __stdcall DXTCompress(
    LPCSTR out_name, u8* raw_data, u8* normal_map, u32 w, u32 h, u32 pitch, STextureParams* fmt, u32 depth);

using Implicit = xr_map<u32, ImplicitDeflector>;

void ImplicitExecute::read(INetReader& r)
{
    y_start = r.r_u32();
    y_end = r.r_u32();
}
void ImplicitExecute::write(IWriter& w) const
{
    R_ASSERT(y_start != (u32(-1)));
    R_ASSERT(y_end != (u32(-1)));
    w.w_u32(y_start);
    w.w_u32(y_end);
}

ImplicitCalcGlobs cl_globs;

void ImplicitExecute::receive_result(INetReader& r)
{
    R_ASSERT(y_start != (u32(-1)));
    R_ASSERT(y_end != (u32(-1)));
    ImplicitDeflector& defl = cl_globs.DATA();
    for (u32 V = y_start; V < y_end; V++)
        for (u32 U = 0; U < defl.Width(); U++)
        {
            r_pod<base_color>(r, defl.Lumel(U, V));
            r_pod<u8>(r, defl.Marker(U, V));
        }
}
void ImplicitExecute::send_result(IWriter& w) const
{
    R_ASSERT(y_start != (u32(-1)));
    R_ASSERT(y_end != (u32(-1)));
    ImplicitDeflector& defl = cl_globs.DATA();
    for (u32 V = y_start; V < y_end; V++)
        for (u32 U = 0; U < defl.Width(); U++)
        {
            w_pod<base_color>(w, defl.Lumel(U, V));
            w_pod<u8>(w, defl.Marker(U, V));
        }
}

void ImplicitExecute::Execute(net_task_callback* net_callback)
{
    R_ASSERT(y_start != (u32(-1)));
    R_ASSERT(y_end != (u32(-1)));
    // R_ASSERT				(DATA);
    ImplicitDeflector& defl = cl_globs.DATA();
    CDB::COLLIDER DB;

    // Setup variables
    Fvector2 dim, half;
    dim.set(float(defl.Width()), float(defl.Height()));
    half.set(.5f / dim.x, .5f / dim.y);

    // Jitter data
    Fvector2 JS;
    JS.set(.499f / dim.x, .499f / dim.y);
    u32 Jcount;
    Fvector2* Jitter;
    Jitter_Select(Jitter, Jcount);

    // Lighting itself
    DB.ray_options(0);
    for (u32 V = y_start; V < y_end; V++)
    {
        for (u32 U = 0; U < defl.Width(); U++)
        {
            if (net_callback && !net_callback->test_connection())
                return;
            base_color_c C;
            u32 Fcount = 0;

            try
            {
                for (u32 J = 0; J < Jcount; J++)
                {
                    // LUMEL space
                    Fvector2 P;
                    P.x = float(U) / dim.x + half.x + Jitter[J].x * JS.x;
                    P.y = float(V) / dim.y + half.y + Jitter[J].y * JS.y;
                    xr_vector<Face*>& space = cl_globs.Hash().query(P.x, P.y);

                    // World space
                    Fvector wP, wN, B;
                    for (auto it = space.begin(); it != space.end(); ++it)
                    {
                        Face* F = *it;
                        _TCF& tc = F->tc[0];
                        if (tc.isInside(P, B))
                        {
                            // We found triangle and have barycentric coords
                            Vertex* V1 = F->v[0];
                            Vertex* V2 = F->v[1];
                            Vertex* V3 = F->v[2];
                            wP.from_bary(V1->P, V2->P, V3->P, B);
                            wN.from_bary(V1->N, V2->N, V3->N, B);
                            wN.normalize();
                            LightPoint(&DB, inlc_global_data()->RCAST_Model(), C, wP, wN,
                                inlc_global_data()->L_static(), (inlc_global_data()->b_nosun() ? LP_dont_sun : 0), F);
                            Fcount++;
                        }
                    }
                }
            }
            catch (...)
            {
                Logger.clMsg("* THREAD #%d: Access violation. Possibly recovered."); //,thID
            }
            if (Fcount)
            {
                // Calculate lighting amount
                C.scale(Fcount);
                C.mul(.5f);
                defl.Lumel(U, V)._set(C);
                defl.Marker(U, V) = 255;
            }
            else
            {
                defl.Marker(U, V) = 0;
            }
        }
        //		thProgress	= float(V - y_start) / float(y_end-y_start);
    }
}

//#pragma optimize( "g", off )

void ImplicitLightingExec(BOOL b_net);
void ImplicitLightingTreadNetExec(void* p);
void ImplicitLighting(BOOL b_net)
{
    if (g_params().m_quality == ebqDraft)
        return;
    if (!b_net)
    {
        ImplicitLightingExec(FALSE);
        return;
    }
    Threading::SpawnThread(ImplicitLightingTreadNetExec, "worker-thread", 1024 * 1024, 0);
}
Lock implicit_net_lock;
void XRLC_LIGHT_API ImplicitNetWait()
{
    implicit_net_lock.Enter();
    implicit_net_lock.Leave();
}
void ImplicitLightingTreadNetExec(void* p)
{
    implicit_net_lock.Enter();
    ImplicitLightingExec(TRUE);
    implicit_net_lock.Leave();
}

static xr_vector<u32> not_clear;
void ImplicitLightingExec(BOOL b_net)
{
    Implicit calculator;

    cl_globs.Allocate();
    not_clear.clear();
    // Sorting
    Logger.Status("Sorting faces...");
    for (auto I = inlc_global_data()->g_faces().begin(); I != inlc_global_data()->g_faces().end(); ++I)
    {
        Face* F = *I;
        if (F->pDeflector)
            continue;
        if (!F->hasImplicitLighting())
            continue;

        Logger.Progress(float(I - inlc_global_data()->g_faces().begin()) / float(inlc_global_data()->g_faces().size()));
        b_material& M = inlc_global_data()->materials()[F->dwMaterial];
        u32 Tid = M.surfidx;
        b_BuildTexture* T = &(inlc_global_data()->textures()[Tid]);

        auto it = calculator.find(Tid);
        if (it == calculator.end())
        {
            ImplicitDeflector ImpD;
            ImpD.texture = T;
            ImpD.faces.push_back(F);
            calculator.insert(std::make_pair(Tid, ImpD));
            not_clear.push_back(Tid);
        }
        else
        {
            ImplicitDeflector& ImpD = it->second;
            ImpD.faces.push_back(F);
        }
    }

    // Lighting
    for (auto imp = calculator.begin(); imp != calculator.end(); ++imp)
    {
        ImplicitDeflector& defl = imp->second;
        Logger.Status("Lighting implicit map '%s'...", defl.texture->name);
        Logger.Progress(0);
        defl.Allocate();

        // Setup cache
        Logger.Progress(0);
        cl_globs.Initialize(defl);
        if (b_net)
            lc_net::RunImplicitnet(defl, not_clear);
        else
            RunImplicitMultithread(defl);

        defl.faces.clear();

        // Expand
        Logger.Status("Processing lightmap...");
        for (u32 ref = 254; ref > 0; ref--)
            if (!ApplyBorders(defl.lmap, ref))
                break;

        Logger.Status("Mixing lighting with texture...");
        {
            b_BuildTexture& TEX = *defl.texture;
            VERIFY(TEX.pSurface);
            u32* color = TEX.pSurface;
            for (u32 V = 0; V < defl.Height(); V++)
            {
                for (u32 U = 0; U < defl.Width(); U++)
                {
                    // Retreive Texel
                    float h = defl.Lumel(U, V).h._r();
                    u32& C = color[V * defl.Width() + U];
                    C = subst_alpha(C, u8_clr(h));
                }
            }
        }

        xr_vector<u32> packed;
        defl.lmap.Pack(packed);
        defl.Deallocate();

        // base
        Logger.Status("Saving base...");
        {
            string_path name, out_name;
            sscanf(strstr(Core.Params, "-f") + 2, "%s", name);
            R_ASSERT(name[0] && defl.texture);
            b_BuildTexture& TEX = *defl.texture;
            strconcat(sizeof(out_name), out_name, name, "\\", TEX.name, ".dds");
            FS.update_path(out_name, "$game_levels$", out_name);
            Logger.clMsg("Saving texture '%s'...", out_name);
            VerifyPath(out_name);
            u8* raw_data = (u8*)(TEX.pSurface);
            u32 w = TEX.dwWidth;
            u32 h = TEX.dwHeight;
            u32 pitch = w * 4;
            STextureParams fmt = TEX.THM;
            fmt.fmt = STextureParams::tfDXT5;
            fmt.flags.set(STextureParams::flDitherColor, FALSE);
            fmt.flags.set(STextureParams::flGenerateMipMaps, FALSE);
            fmt.flags.set(STextureParams::flBinaryAlpha, FALSE);
            DXTCompress(out_name, raw_data, 0, w, h, pitch, &fmt, 4);
        }

        // lmap
        Logger.Status("Saving lmap...");
        {
            // xr_vector<u32>			packed;
            // defl.lmap.Pack			(packed);

            string_path name, out_name;
            sscanf(strstr(GetCommandLine(), "-f") + 2, "%s", name);
            b_BuildTexture& TEX = *defl.texture;
            strconcat(sizeof(out_name), out_name, name, "\\", TEX.name, "_lm.dds");
            FS.update_path(out_name, "$game_levels$", out_name);
            Logger.clMsg("Saving texture '%s'...", out_name);
            VerifyPath(out_name);
            u8* raw_data = (u8*)(&*packed.begin());
            u32 w = TEX.dwWidth;
            u32 h = TEX.dwHeight;
            u32 pitch = w * 4;
            STextureParams fmt;
            fmt.fmt = STextureParams::tfDXT5;
            fmt.flags.set(STextureParams::flDitherColor, FALSE);
            fmt.flags.set(STextureParams::flGenerateMipMaps, FALSE);
            fmt.flags.set(STextureParams::flBinaryAlpha, FALSE);
            DXTCompress(out_name, raw_data, 0, w, h, pitch, &fmt, 4);
        }
        // defl.Deallocate				();
    }
    not_clear.clear();
    cl_globs.Deallocate();
    calculator.clear();
    if (b_net)
        inlc_global_data()->clear_build_textures_surface();
}
