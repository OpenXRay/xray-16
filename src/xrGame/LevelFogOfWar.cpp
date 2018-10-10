// LevelFogOfWar.cpp:  для карты уровня в одиночном режиме игры
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LevelFogOfWar.h"
#include "Level.h"
#include "alife_registry_wrappers.h"
#include "ui/UIMap.h"
#include "game_base_space.h"

#define FOG_CELL_SZ (50.0f)

struct FindFogByLevelName
{
    shared_str level_name;
    FindFogByLevelName(const shared_str& name) { level_name = name; }
    bool operator()(const CLevelFogOfWar& fog) { return (level_name == fog.m_level_name); }
};

CFogOfWarMngr::CFogOfWarMngr()
{
    m_fogOfWarRegistry = xr_new<CFogOfWarWrapper>();
    m_fogOfWarRegistry->registry().init(0);
}

CFogOfWarMngr::~CFogOfWarMngr() { xr_delete(m_fogOfWarRegistry); }
CLevelFogOfWar* CFogOfWarMngr::GetFogOfWar(const shared_str& level_name)
{
    if (GameID() != GAME_SINGLE)
        return NULL;
    FOG_STORAGE_IT it = std::find_if(GetFogStorage().begin(), GetFogStorage().end(), FindFogByLevelName(level_name));
    if (it != GetFogStorage().end())
        return &(*it);
    else
    { // create new or load...
        GetFogStorage().resize(GetFogStorage().size() + 1);
        CLevelFogOfWar& F = GetFogStorage().back();
        F.Init(level_name);
        return &F;
    };
}

FOG_STORAGE_T& CFogOfWarMngr::GetFogStorage()
{
    VERIFY(m_fogOfWarRegistry);
    return m_fogOfWarRegistry->registry().objects();
};

CLevelFogOfWar::CLevelFogOfWar()
    : m_rowNum(0), m_colNum(0){
                       //	m_levelRect.set	(0.0f,0.0f,0.0f,0.0f);
                       //	hShader.create	("hud" DELIMITER "default","ui" DELIMITER "ui_fog_of_war");
                       //	hGeom.create	(FVF::F_TL, RCache.Vertex.Buffer(), 0);
                   };

void CLevelFogOfWar::Init(const shared_str& level)
{
    m_level_name = level;

    CInifile& gameLtx = *pGameIni;

    Fvector4 tmp;
    if (gameLtx.line_exist(m_level_name, "bound_rect"))
        tmp = gameLtx.r_fvector4(m_level_name, "bound_rect");
    else
        tmp.set(-10000.0f, -10000.0f, 10000.0f, 10000.0f); //. hack

    m_levelRect.set(tmp.x, tmp.y, tmp.z, tmp.w);

    m_rowNum = iFloor((m_levelRect.height() + FOG_CELL_SZ * 0.5f) / FOG_CELL_SZ);
    m_colNum = iFloor((m_levelRect.width() + FOG_CELL_SZ * 0.5f) / FOG_CELL_SZ);

    m_cells.resize(m_rowNum * m_colNum, false);

    hShader.create("hud" DELIMITER "fog_of_war", "ui" DELIMITER "ui_fog_of_war");
    hGeom.create(FVF::F_TL, RCache.Vertex.Buffer(), 0);
}

#define FOG_OPEN_RADIUS (FOG_CELL_SZ / 4)

void CLevelFogOfWar::Open(Fvector2 pos)
{
    if (!m_rowNum || !m_rowNum)
        return; // invalid map
    if (!(pos.x >= m_levelRect.lt.x && pos.y >= m_levelRect.lt.y && pos.x <= m_levelRect.rb.x &&
            pos.y <= m_levelRect.rb.y))
        return; // invalid position
    VERIFY2((pos.x >= m_levelRect.lt.x && pos.y >= m_levelRect.lt.y && pos.x <= m_levelRect.rb.x &&
                pos.y <= m_levelRect.rb.y),
        "invalid position for opening FogOfWar map cell");

    int col = iFloor((pos.x - m_levelRect.lt.x) / FOG_CELL_SZ);
    int row = iFloor((m_levelRect.height() - (pos.y - m_levelRect.lt.y)) / FOG_CELL_SZ);

    int cell_sz = iCeil(FOG_OPEN_RADIUS / FOG_CELL_SZ);

    Frect tgt, cell;
    tgt.lt.x = pos.x - FOG_OPEN_RADIUS;
    tgt.lt.y = pos.y - FOG_OPEN_RADIUS;
    tgt.rb.x = pos.x + FOG_OPEN_RADIUS;
    tgt.rb.y = pos.y + FOG_OPEN_RADIUS;

    for (int rr = row - cell_sz; rr <= row + cell_sz; ++rr)
    {
        if (rr < 0)
            continue;
        for (int cc = col - cell_sz; cc <= col + cell_sz; ++cc)
        {
            if (cc < 0)
                continue;
            cell.lt.x = m_levelRect.lt.x + cc * FOG_CELL_SZ;
            cell.rb.y = m_levelRect.lt.y + m_levelRect.height() - rr * FOG_CELL_SZ;
            cell.rb.x = m_levelRect.lt.x + cc * FOG_CELL_SZ + FOG_CELL_SZ;
            cell.lt.y = m_levelRect.lt.y + m_levelRect.height() - rr * FOG_CELL_SZ - FOG_CELL_SZ;
            if (tgt.intersected(cell))
                Open(rr, cc, true);
        }
    }
}

void CLevelFogOfWar::Open(u32 row, u32 col, bool b)
{
    if (row >= m_rowNum || col >= m_colNum)
        return;

    m_cells.at(row * m_colNum + col) = b;
}

Ivector2 CLevelFogOfWar::ConvertRealToLocal(const Fvector2& src)
{
    Ivector2 res;
    res.x = iFloor((src.x - m_levelRect.lt.x) / FOG_CELL_SZ);
    res.y = iFloor((src.y - m_levelRect.lt.y) / FOG_CELL_SZ);

    return res;
}

Irect CLevelFogOfWar::ConvertRealToLocal(const Frect& src)
{
    return Irect().set(ConvertRealToLocal(src.lt), ConvertRealToLocal(src.rb));
}

Fvector2 CLevelFogOfWar::ConvertLocalToReal(const Ivector2& src)
{
    return Fvector2().set(m_levelRect.lt.x + src.x * FOG_CELL_SZ, m_levelRect.lt.y + src.y * FOG_CELL_SZ);
}

enum
{
    tfLT = 0,
    tfRT,
    tfLB,
    tfRB
};

void CLevelFogOfWar::GetTexUVLT(Fvector2& uv, u32 col, u32 row)
{
    if (row >= m_rowNum || col >= m_colNum)
        uv.set(0.5f, 0.0f);
    else
    {
        bool cell_mask = m_cells[row * m_colNum + col];
        if (cell_mask == true)
            uv.set(0.0f, 0.0f);
        else
            uv.set(0.5f, 0.0f);
    }
}

void CLevelFogOfWar::Draw()
{
    CUICustomMap* m = ((CUICustomMap*)(GetParent()));

    Frect tgt;
    Frect clip_rect = m->GetClipperRect();
    Frect _r;
    m->GetAbsoluteRect(_r);
    BOOL intersected = clip_rect.intersection(clip_rect, _r);
    VERIFY(intersected);
    Fvector2 map_abs_pos;
    m->GetAbsolutePos(map_abs_pos);

    Frect vis_rect;
    vis_rect.set(clip_rect.lt.x - map_abs_pos.x, clip_rect.lt.y - map_abs_pos.y, clip_rect.rb.x - map_abs_pos.x,
        clip_rect.rb.y - map_abs_pos.y); // vis_rect now in pixels

    tgt.set(float(vis_rect.x1), float(vis_rect.y1), float(vis_rect.x2), float(vis_rect.y2));
    tgt.div(m->GetCurrentZoom(), m->GetCurrentZoom());
    tgt.add(m_levelRect.lt.x, m_levelRect.lt.y);
    tgt.rb.add(Fvector2().set(FOG_CELL_SZ - EPS_L, FOG_CELL_SZ - EPS_L));
    Irect cells = ConvertRealToLocal(tgt);

    Fvector2 realCellsPosLT = ConvertLocalToReal(cells.lt);

    realCellsPosLT.sub(m_levelRect.lt).mul(m->GetCurrentZoom());

    Ivector2 drawLT;
    drawLT.set(
        (realCellsPosLT.x + map_abs_pos.x) * UI()->GetScaleX(), (realCellsPosLT.y + map_abs_pos.y) * UI()->GetScaleY());

    const Fvector2 pts[6] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    const Fvector2 uvs[6] = {{0.0f, 0.0f}, {0.5f, 0.0f}, {0.5f, 1.0f}, {0.0f, 0.0f}, {0.5f, 1.0f}, {0.0f, 1.0f}};

    // calculate cell size in screen pixels
    float fw = FOG_CELL_SZ * m->GetCurrentZoom() * UI()->GetScaleX();
    float fh = FOG_CELL_SZ * m->GetCurrentZoom() * UI()->GetScaleY();

    // fill cell buffer
    u32 vOffset = 0;
    FVF::TL* start_pv = (FVF::TL*)RCache.Vertex.Lock(cells.width() * cells.height() * 6, hGeom.stride(), vOffset);
    FVF::TL* pv = start_pv;
    for (int x = 0; x < cells.width(); ++x)
    {
        for (int y = 0; y < cells.height(); ++y)
        {
            Fvector2 tp;
            GetTexUVLT(tp, cells.x1 + x, cells.y1 + y);
            for (u32 k = 0; k < 6; ++k, ++pv)
            {
                const Fvector2& p = pts[k];
                const Fvector2& uv = uvs[k];
                pv->set(iFloor(drawLT.x + p.x * (fw) + fw * x) - 0.5f, iFloor(drawLT.y + p.y * (fh) + fh * y) - 0.5f,
                    0xFFFFFFFF, tp.x + uv.x, tp.y + uv.y);
            }
        }
    }
    std::ptrdiff_t p_cnt = (pv - start_pv) / 3;
    RCache.Vertex.Unlock(u32(pv - start_pv), hGeom.stride());

    // set scissor
    UI()->PushScissor(clip_rect);
    if (p_cnt != 0)
    {
        // draw
        RCache.set_Shader(hShader);
        RCache.set_Geometry(hGeom);
        RCache.Render(D3DPT_TRIANGLELIST, vOffset, u32(p_cnt));
    }
    UI()->PopScissor();
}

void CLevelFogOfWar::save(IWriter& stream)
{
    save_data(m_level_name, stream);
    save_data(m_levelRect, stream);
    save_data(m_rowNum, stream);
    save_data(m_colNum, stream);
    save_data(m_cells, stream);
}

void CLevelFogOfWar::load(IReader& stream)
{
    load_data(m_level_name, stream);
    Init(m_level_name);

    Frect levelRect;
    load_data(levelRect, stream);
    VERIFY(m_levelRect.cmp(levelRect));

    u32 rowNum, colNum;
    load_data(rowNum, stream);
    load_data(colNum, stream);
    VERIFY(rowNum == m_rowNum);
    VERIFY(colNum == m_colNum);
    load_data(m_cells, stream);
}
