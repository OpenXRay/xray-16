#include "StdAfx.h"
#include "wallmark_manager.h"
#include "Level.h"
#include "xrEngine/GameMtlLib.h"
#include "xrPhysics/CalculateTriangle.h"
#include "xrEngine/profiler.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

CWalmarkManager::CWalmarkManager() {}
CWalmarkManager::~CWalmarkManager() { Clear(); }
void CWalmarkManager::Clear() { m_wallmarks->clear(); }
void CWalmarkManager::AddWallmark(const Fvector& dir, const Fvector& start_pos, float range, float wallmark_size,
    IWallMarkArray& wallmarks_vector, int t)
{
    CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + t; // result.element;
    SGameMtl* pMaterial = GMLib.GetMaterialByIdx(pTri->material);

    if (pMaterial->Flags.is(SGameMtl::flBloodmark))
    {
        //вычислить нормаль к пораженной поверхности
        Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();

        //вычислить точку попадания
        Fvector end_point;
        end_point.set(0, 0, 0);
        end_point.mad(start_pos, dir, range);

        if (!wallmarks_vector.empty())
        {
            GEnv.Render->add_StaticWallmark(&wallmarks_vector, end_point, wallmark_size, pTri, pVerts);
        }

        /*
        ref_shader* pWallmarkShader = wallmarks_vector.empty()?NULL:
        &wallmarks_vector[::Random.randI(0,wallmarks_vector.size())];

        if (pWallmarkShader)
        {
            //добавить отметку на материале
            GlobalEnv.Render->add_StaticWallmark(*pWallmarkShader, end_point, wallmark_size, pTri, pVerts);
        }
        */
    }
}

/*
void CWalmarkManager::PlaceWallmark(const Fvector& dir, const Fvector& start_pos,
                                      float trace_dist, float wallmark_size,
                                      SHADER_VECTOR& wallmarks_vector,IGameObject* ignore_obj)
{
    collide::rq_result	result;
    BOOL				reach_wall =
        Level().ObjectSpace.RayPick(
        start_pos,
        dir,
        trace_dist,
        collide::rqtBoth,
        result,
        ignore_obj
        )
        &&
        !result.O;

    //если кровь долетела до статического объекта
    if(reach_wall)
    {
        AddWallmark(dir,start_pos,result.range,wallmark_size,wallmarks_vector,result.element);
    }
}
*/

void CWalmarkManager::PlaceWallmarks(const Fvector& start_pos)
{
    m_pos = start_pos;
    //.	LPCSTR				sect				= pSettings->r_string(m_owner->cNameSect(), "wallmark_section");
    Load("explosion_marks");

    //.	Device.seqParallel.push_back	(fastdelegate::FastDelegate0<>(this,&CWalmarkManager::StartWorkflow));

    StartWorkflow();
}

float Distance(
    const Fvector& rkPoint, const Fvector rkTri[3], float& pfSParam, float& pfTParam, Fvector& closest, Fvector& dir);

void CWalmarkManager::StartWorkflow()
{
    LPCSTR sect = "explosion_marks";
    float m_trace_dist = pSettings->r_float(sect, "dist");
    float m_wallmark_size = pSettings->r_float(sect, "size");
    u32 max_wallmarks_count = pSettings->r_u32(sect, "max_count");

    XRC.box_options(0);
    XRC.box_query(Level().ObjectSpace.GetStaticModel(), m_pos, Fvector().set(m_trace_dist, m_trace_dist, m_trace_dist));

    CDB::TRI* T_array = Level().ObjectSpace.GetStaticTris();
    Fvector* V_array = Level().ObjectSpace.GetStaticVerts();
    //.	Triangle		ntri;
    //.	float			ndist					= phInfinity;
    //.	Fvector			npoint;
    u32 wm_count = 0;

    u32 _ray_test = 0;
    //	u32 _tri_behind		= 0;
    u32 _tri_not_plane = 0;
    u32 _not_dist = 0;
    /*
        DBG_OpenCashedDraw		();
        DBG_DrawAABB			(m_pos,Fvector().set(m_trace_dist,m_trace_dist,m_trace_dist),color_xrgb(255,0,0));
        DBG_DrawAABB			(m_pos,Fvector().set(0.05f,0.05f,0.05f),color_xrgb(0,255,0));

        CTimer T; T.Start();
    */
    for (auto &Res : *XRC.r_get())
    {
        //.		DBG_DrawTri(Res, color_xrgb(0,255,0) );

        if (wm_count >= max_wallmarks_count)
            break;

        //.		Triangle					tri;
        Fvector end_point;
        //.		ETriDist					c;
        Fvector pdir;
        float pfSParam;
        float pfTParam;

        //.		CalculateTriangle			(T_array+Res->id,cast_fp(m_pos),tri);

        //.		float dist					= DistToTri(&tri,cast_fp(m_pos),cast_fp(pdir),cast_fp(end_point),c,V_array);
        Fvector _tri[3];

        CDB::TRI* _t = T_array + Res.id;

        _tri[0] = V_array[_t->verts[0]];
        _tri[1] = V_array[_t->verts[1]];
        _tri[2] = V_array[_t->verts[2]];

        float dist = Distance(m_pos, _tri, pfSParam, pfTParam, end_point, pdir);

        /*
                if (c==tdBehind){
                    ++_tri_behind;
                    continue;
                }
        */
        float test = dist - EPS_L;

        if (test > 0.f)
        {
            if (Level().ObjectSpace.RayTest(m_pos, pdir, test, collide::rqtStatic, NULL, m_owner))
            {
                ++_ray_test;
                continue;
            }
        }
        if (fis_zero(pfSParam) || fis_zero(pfTParam) || fsimilar(pfSParam, 1.0f) || fsimilar(pfTParam, 1.0f))
        {
            ++_tri_not_plane;
            continue;
        }

        if (dist <= m_trace_dist)
        {
            GEnv.Render->add_StaticWallmark(&*m_wallmarks, end_point, m_wallmark_size, _t, V_array);
            ++wm_count;
        }
        else
            ++_not_dist;
    }
    /*
        Msg("----------------------------------");
        Msg("tri count=%d",						XRC.r_count());
        Msg("far_dist=%d",						_not_dist);
        Msg("RayTest = %d",						_ray_test);
        Msg("c==tdBehind = %d",					_tri_behind);
        Msg	("c!=tdPlane && dist>ndist = %d",	_tri_not_plane);
        Msg("Wallmarks added = %d",				wm_count);
        Msg("Time: %d",							T.GetElapsed_ms());

        DBG_ClosedCashedDraw	(10000);
    */
}

void CWalmarkManager::Load(LPCSTR section)
{
    //кровавые отметки на стенах
    string256 tmp;
    pcstr wallmarks_name = pSettings->r_string(section, "wallmarks");
    const int cnt = _GetItemCount(wallmarks_name);
    VERIFY(cnt);

    for (int k = 0; k < cnt; ++k)
        m_wallmarks->AppendMark(_GetItem(wallmarks_name, k, tmp));
}

float Distance(
    const Fvector& rkPoint, const Fvector rkTri[3], float& pfSParam, float& pfTParam, Fvector& closest, Fvector& dir)
{
    //.    Fvector kDiff = rkTri.Origin() - rkPoint;
    Fvector kDiff;
    kDiff.sub(rkTri[0], rkPoint); //

    Fvector Edge0;
    Edge0.sub(rkTri[1], rkTri[0]); //
    Fvector Edge1;
    Edge1.sub(rkTri[2], rkTri[0]); //

    //.    float fA00 = rkTri.Edge0().SquaredLength();
    float fA00 = Edge0.square_magnitude();

    //.    float fA01 = rkTri.Edge0().Dot(rkTri.Edge1());
    float fA01 = Edge0.dotproduct(Edge1);

    //.    float fA11 = rkTri.Edge1().SquaredLength();
    float fA11 = Edge1.square_magnitude();

    //.    float fB0 = kDiff.Dot(rkTri.Edge0());
    float fB0 = kDiff.dotproduct(Edge0);

    //.	float fB1 = kDiff.Dot(rkTri.Edge1());
    float fB1 = kDiff.dotproduct(Edge1);

    //.    float fC = kDiff.SquaredLength();
    float fC = kDiff.square_magnitude();

    float fDet = _abs(fA00 * fA11 - fA01 * fA01);

    float fS = fA01 * fB1 - fA11 * fB0;
    float fT = fA01 * fB0 - fA00 * fB1;
    float fSqrDist;

    if (fS + fT <= fDet)
    {
        if (fS < 0.0f)
        {
            if (fT < 0.0f) // region 4
            {
                if (fB0 < 0.0f)
                {
                    fT = 0.0f;
                    if (-fB0 >= fA00)
                    {
                        fS = 1.0f;
                        fSqrDist = fA00 + 2.0f * fB0 + fC;
                    }
                    else
                    {
                        fS = -fB0 / fA00;
                        fSqrDist = fB0 * fS + fC;
                    }
                }
                else
                {
                    fS = 0.0f;
                    if (fB1 >= 0.0f)
                    {
                        fT = 0.0f;
                        fSqrDist = fC;
                    }
                    else if (-fB1 >= fA11)
                    {
                        fT = 1.0f;
                        fSqrDist = fA11 + 2.0f * fB1 + fC;
                    }
                    else
                    {
                        fT = -fB1 / fA11;
                        fSqrDist = fB1 * fT + fC;
                    }
                }
            }
            else // region 3
            {
                fS = 0.0f;
                if (fB1 >= 0.0f)
                {
                    fT = 0.0f;
                    fSqrDist = fC;
                }
                else if (-fB1 >= fA11)
                {
                    fT = 1.0f;
                    fSqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else
                {
                    fT = -fB1 / fA11;
                    fSqrDist = fB1 * fT + fC;
                }
            }
        }
        else if (fT < 0.0f) // region 5
        {
            fT = 0.0f;
            if (fB0 >= 0.0f)
            {
                fS = 0.0f;
                fSqrDist = fC;
            }
            else if (-fB0 >= fA00)
            {
                fS = 1.0f;
                fSqrDist = fA00 + 2.0f * fB0 + fC;
            }
            else
            {
                fS = -fB0 / fA00;
                fSqrDist = fB0 * fS + fC;
            }
        }
        else // region 0
        {
            // minimum at interior point
            float fInvDet = 1.0f / fDet;
            fS *= fInvDet;
            fT *= fInvDet;
            fSqrDist = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
        }
    }
    else
    {
        float fTmp0, fTmp1, fNumer, fDenom;

        if (fS < 0.0f) // region 2
        {
            fTmp0 = fA01 + fB0;
            fTmp1 = fA11 + fB1;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    fS = 1.0f;
                    fT = 0.0f;
                    fSqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else
                {
                    fS = fNumer / fDenom;
                    fT = 1.0f - fS;
                    fSqrDist =
                        fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
                }
            }
            else
            {
                fS = 0.0f;
                if (fTmp1 <= 0.0f)
                {
                    fT = 1.0f;
                    fSqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else if (fB1 >= 0.0f)
                {
                    fT = 0.0f;
                    fSqrDist = fC;
                }
                else
                {
                    fT = -fB1 / fA11;
                    fSqrDist = fB1 * fT + fC;
                }
            }
        }
        else if (fT < 0.0f) // region 6
        {
            fTmp0 = fA01 + fB1;
            fTmp1 = fA00 + fB0;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    fT = 1.0f;
                    fS = 0.0f;
                    fSqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else
                {
                    fT = fNumer / fDenom;
                    fS = 1.0f - fT;
                    fSqrDist =
                        fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
                }
            }
            else
            {
                fT = 0.0f;
                if (fTmp1 <= 0.0f)
                {
                    fS = 1.0f;
                    fSqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else if (fB0 >= 0.0f)
                {
                    fS = 0.0f;
                    fSqrDist = fC;
                }
                else
                {
                    fS = -fB0 / fA00;
                    fSqrDist = fB0 * fS + fC;
                }
            }
        }
        else // region 1
        {
            fNumer = fA11 + fB1 - fA01 - fB0;
            if (fNumer <= 0.0f)
            {
                fS = 0.0f;
                fT = 1.0f;
                fSqrDist = fA11 + 2.0f * fB1 + fC;
            }
            else
            {
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    fS = 1.0f;
                    fT = 0.0f;
                    fSqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else
                {
                    fS = fNumer / fDenom;
                    fT = 1.0f - fS;
                    fSqrDist =
                        fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
                }
            }
        }
    }

    pfSParam = fS;
    pfTParam = fT;

    closest.mad(rkTri[0], Edge0, fS).mad(Edge1, fT);

    dir.sub(closest, rkPoint);
    dir.normalize_safe();
    return _sqrt(_abs(fSqrDist));
}
