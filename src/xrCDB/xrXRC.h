// xrXRC.h: interface for the xrXRC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XRXRC_H__9AA25268_621F_4FCA_BD75_AF2E9822B8E3__INCLUDED_)
#define AFX_XRXRC_H__9AA25268_621F_4FCA_BD75_AF2E9822B8E3__INCLUDED_
//#pragma once

#include "xrCDB.h"
class IGameFont;
class IPerformanceAlert;

class XRCDB_API xrXRC  
{
	CDB::COLLIDER	CL;
    const char *name;
public:
    struct ColliderStatistics
    {
        CStatTimer RayQuery; // total: ray-testing
        CStatTimer BoxQuery; // total: box query
        CStatTimer FrustumQuery; // total: frustum query
        float RayPs = 0;
        float BoxPs = 0;

        ColliderStatistics() { FrameStart(); }

        void FrameStart()
        {
            RayQuery.FrameStart();
            BoxQuery.FrameStart();
            FrustumQuery.FrameStart();
        }

        void FrameEnd()
        {
            RayQuery.FrameEnd();
            BoxQuery.FrameEnd();
            FrustumQuery.FrameEnd();
            float newRayPs = RayQuery.count/RayQuery.result;
            if (std::isnan(newRayPs))
                newRayPs = 0;
            RayPs = 0.99f*RayPs + 0.01f*newRayPs;
            float newBoxPs = BoxQuery.count/BoxQuery.result;
            if (std::isnan(newBoxPs))
                newBoxPs = 0;
            BoxPs = 0.99f*BoxPs + 0.01f*newBoxPs;
        }
    };
    ColliderStatistics Stats;

	IC void			ray_options		(u32 f)		
	{ 
		CL.ray_options(f); 
	}
	IC void			ray_query		(const CDB::MODEL *m_def, const Fvector& r_start,  const Fvector& r_dir, float r_range = 10000.f)
	{
        Stats.RayQuery.Begin();
		CL.ray_query(m_def,r_start,r_dir,r_range);
        Stats.RayQuery.End();
	}
	
	IC void			box_options		(u32 f)	
	{	
		CL.box_options(f);
	}
	IC void			box_query		(const CDB::MODEL *m_def, const Fvector& b_center, const Fvector& b_dim)
	{
        Stats.BoxQuery.Begin();
		CL.box_query(m_def,b_center,b_dim);
        Stats.BoxQuery.End();
	}
	
	IC void			frustum_options	(u32 f)
	{
		CL.frustum_options(f);
	}
	IC void			frustum_query	(const CDB::MODEL *m_def, const CFrustum& F)
	{
        Stats.FrustumQuery.Begin();
		CL.frustum_query(m_def,F);
        Stats.FrustumQuery.End();
	}
	
	IC CDB::RESULT*	r_begin			()	{	return CL.r_begin();		};
	IC CDB::RESULT*	r_end			()	{	return CL.r_end();			};
	IC void			r_free			()	{	CL.r_free();				}
	IC int			r_count			()	{	return CL.r_count();		};
	IC void			r_clear			()	{	CL.r_clear();				};
	IC void			r_clear_compact	()	{	CL.r_clear_compact();		};
	
    void DumpStatistics(IGameFont &font, IPerformanceAlert *alert);

    xrXRC(const char *name = "<unknown>") : name(name) {}
};
XRCDB_API extern xrXRC XRC;

#endif // !defined(AFX_XRXRC_H__9AA25268_621F_4FCA_BD75_AF2E9822B8E3__INCLUDED_)
