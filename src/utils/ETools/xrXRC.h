// xrXRC.h: interface for the xrXRC class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCDB/xrCDB.h"
#include "xrCore/_fbox.h"

class ENGINE_API xrXRC
{
    CDB::COLLIDER CL;

public:
    IC CDB::COLLIDER* collider() { return &CL; }
    IC void ray_options(u32 f) { CL.ray_options(f); }
    IC void ray_query(const CDB::MODEL* m_def, const Fvector& r_start, const Fvector& r_dir, float r_range)
    {
#ifndef NO_XRC_STATS
        Device.Statistic->clRAY.Begin();
#endif
        CL.ray_query(m_def, r_start, r_dir, r_range);
#ifndef NO_XRC_STATS
        Device.Statistic->clRAY.End();
#endif
    }
    IC void ray_query(
        const Fmatrix& inv_parent, const CDB::MODEL* m_def, const Fvector& r_start, const Fvector& r_dir, float r_range)
    {
        // transform
        Fvector S, D;
        inv_parent.transform_tiny(S, r_start);
        inv_parent.transform_dir(D, r_dir);
        ray_query(m_def, S, D, r_range);
    }

    IC void box_options(u32 f) { CL.box_options(f); }
    IC void box_query(const CDB::MODEL* m_def, const Fvector& b_center, const Fvector& b_dim)
    {
#ifndef NO_XRC_STATS
        Device.Statistic->clBOX.Begin();
#endif
        CL.box_query(m_def, b_center, b_dim);
#ifndef NO_XRC_STATS
        Device.Statistic->clBOX.End();
#endif
    }
    IC void box_query(const Fmatrix& inv_parent, const CDB::MODEL* m_def, const Fbox& src)
    {
        Fbox dest;
        dest.xform(src, inv_parent);
        Fvector c, d;
        dest.getcenter(c);
        dest.getradius(d);
        box_query(m_def, c, d);
    }

    IC void frustum_options(u32 f) { CL.frustum_options(f); }
    IC void frustum_query(const CDB::MODEL* m_def, const CFrustum& F)
    {
#ifndef NO_XRC_STATS
        Device.Statistic->clFRUSTUM.Begin();
#endif
        CL.frustum_query(m_def, F);
#ifndef NO_XRC_STATS
        Device.Statistic->clFRUSTUM.End();
#endif
    }

    IC CDB::RESULT* r_begin() { return CL.r_begin(); };
    IC void r_free() { CL.r_free(); }
    IC int r_count() { return CL.r_count(); };
    IC void r_clear() { CL.r_clear(); };
};
ENGINE_API extern xrXRC XRC;
