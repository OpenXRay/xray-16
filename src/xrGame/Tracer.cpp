#include "StdAfx.h"
#include "Tracer.h"
#include "xrEngine/Render.h"

#include "Include/xrRender/UIShader.h"
#include "Include/xrRender/UIRender.h"
#include "xrCore/_fbox2.h"

const u32 MAX_TRACERS = (1024 * 5);
const float TRACER_SIZE = 0.13f;

CTracer::CTracer()
{
    LPCSTR sh_name = pSettings->r_string("bullet_manager", "tracer_shader");
    LPCSTR tx_name = pSettings->r_string("bullet_manager", "tracer_texture");
    m_circle_size_k = pSettings->r_float("bullet_manager", "fire_circle_k");

    sh_Tracer->create(sh_name, tx_name);

    m_aColors.clear();
    string64 LineName;

    for (u8 i = 0; i < 255; i++)
    {
        xr_sprintf(LineName, "color_%d", i);
        if (!pSettings->line_exist("tracers_color_table", LineName))
            break;
        u32 clr = pSettings->r_color("tracers_color_table", LineName);

        m_aColors.push_back(clr);
    };
}

IC void FillSprite_Circle(const Fvector& pos, const float width, const float length, u32 color)
{
    const Fvector& T = Device.vCameraTop;
    const Fvector& R = Device.vCameraRight;
    Fvector Vr, Vt;
    Vr.x = R.x * width;
    Vr.y = R.y * width;
    Vr.z = R.z * width;
    Vt.x = T.x * length;
    Vt.y = T.y * length;
    Vt.z = T.z * length;

    Fvector a, b, c, d;
    a.sub(Vt, Vr);
    b.add(Vt, Vr);
    c.invert(a);
    d.invert(b);

    Fbox2 t_crcl;
    t_crcl.min.set(32.0f / 64.0f, 0.0f);
    t_crcl.max.set(1.0f, 32.0f / 512.0f);

    //	TODO: return code back to indexed rendering since we use quads
    //	Tri 1
    GEnv.UIRender->PushPoint(
        d.x + pos.x, d.y + pos.y, d.z + pos.z, color, t_crcl.min.x, t_crcl.max.y); // 0.f,1.f);
    GEnv.UIRender->PushPoint(
        a.x + pos.x, a.y + pos.y, a.z + pos.z, color, t_crcl.min.x, t_crcl.min.y); // 0.f,0.f);
    GEnv.UIRender->PushPoint(
        c.x + pos.x, c.y + pos.y, c.z + pos.z, color, t_crcl.max.x, t_crcl.max.y); // 1.f,1.f);
    //	Tri 2
    GEnv.UIRender->PushPoint(
        c.x + pos.x, c.y + pos.y, c.z + pos.z, color, t_crcl.max.x, t_crcl.max.y); // 1.f,1.f);
    GEnv.UIRender->PushPoint(
        a.x + pos.x, a.y + pos.y, a.z + pos.z, color, t_crcl.min.x, t_crcl.min.y); // 0.f,0.f);
    GEnv.UIRender->PushPoint(
        b.x + pos.x, b.y + pos.y, b.z + pos.z, color, t_crcl.max.x, t_crcl.min.y); // 1.f,0.f);

    // pv->set         (d.x+pos.x,d.y+pos.y,d.z+pos.z, color, 0.f,1.f);        pv++;
    // pv->set         (a.x+pos.x,a.y+pos.y,a.z+pos.z, color, 0.f,0.f);        pv++;
    // pv->set         (c.x+pos.x,c.y+pos.y,c.z+pos.z, color, 1.f,1.f);        pv++;
    // pv->set         (b.x+pos.x,b.y+pos.y,b.z+pos.z, color, 1.f,0.f);        pv++;
}

IC void FillSprite_Line(const Fvector& pos, const Fvector& dir, const float width, const float length, u32 color)
{
    const Fvector& T = dir;
    Fvector R;
    R.crossproduct(T, Device.vCameraDirection).normalize_safe();

    Fvector Vr, Vt;
    Vr.x = R.x * width;
    Vr.y = R.y * width;
    Vr.z = R.z * width;

    Vt.x = T.x * length;
    Vt.y = T.y * length;
    Vt.z = T.z * length;

    Fvector a, b, c, d;
    a.sub(Vt, Vr);
    b.add(Vt, Vr);
    c.invert(a);
    d.invert(b);

    Fbox2 t_tracer;
    t_tracer.min.set(0.0f, 1.0f);
    t_tracer.max.set(16.0f / 64.0f, 0.0f);

    //	TODO: return code back to indexed rendering since we use quads
    //	Tri 1
    GEnv.UIRender->PushPoint(d.x + pos.x, d.y + pos.y, d.z + pos.z, color, t_tracer.min.x, t_tracer.max.y);
    GEnv.UIRender->PushPoint(a.x + pos.x, a.y + pos.y, a.z + pos.z, color, t_tracer.min.x, t_tracer.min.y);
    GEnv.UIRender->PushPoint(c.x + pos.x, c.y + pos.y, c.z + pos.z, color, t_tracer.max.x, t_tracer.max.y);
    //	Tri 2
    GEnv.UIRender->PushPoint(c.x + pos.x, c.y + pos.y, c.z + pos.z, color, t_tracer.max.x, t_tracer.max.y);
    GEnv.UIRender->PushPoint(a.x + pos.x, a.y + pos.y, a.z + pos.z, color, t_tracer.min.x, t_tracer.min.y);
    GEnv.UIRender->PushPoint(b.x + pos.x, b.y + pos.y, b.z + pos.z, color, t_tracer.max.x, t_tracer.min.y);

    // pv->set         (d.x+pos.x,d.y+pos.y,d.z+pos.z, color, 0.f,1.f);        pv++;
    // pv->set         (a.x+pos.x,a.y+pos.y,a.z+pos.z, color, 0.f,0.5f);        pv++;
    // pv->set         (c.x+pos.x,c.y+pos.y,c.z+pos.z, color, 1.f,1.f);        pv++;
    // pv->set         (b.x+pos.x,b.y+pos.y,b.z+pos.z, color, 1.f,0.5f);        pv++;
}

void CTracer::Render(const Fvector& pos, const Fvector& center, const Fvector& dir, float length, float width,
    u8 colorID, float speed, bool bActor)
{
    if (GEnv.Render->ViewBase.testSphere_dirty((Fvector&)center, length * .5f))
    {
        R_ASSERT(colorID < m_aColors.size());

        if (bActor)
        {
            float k_speed = speed / 1000.0f;
            //			float f_distance	= Device.vCameraPosition.distance_to(pos);

            FillSprite_Circle(
                pos, k_speed * width * m_circle_size_k, k_speed * width * m_circle_size_k, m_aColors[colorID]);
        }

        FillSprite_Line(center, dir, width * .5f, length * .5f, m_aColors[colorID]);
    }
}
