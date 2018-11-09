#ifndef TRI_PRIMITIVE_COLIDE_CLASS_DEF
#define TRI_PRIMITIVE_COLIDE_CLASS_DEF

#define TRI_PRIMITIVE_COLIDE_CLASS_DECLARE(primitive)                                                          \
    \
class primitive##Tri                                                                                           \
    \
{                                                                                                       \
        dcTriListCollider& m_tri_list;                                                                         \
        primitive##Tri& operator=(primitive##Tri& nx_nado) = delete;                                           \
    \
public:                                                                                                        \
        explicit primitive##Tri(dcTriListCollider& tri_list) : m_tri_list(tri_list){};                         \
        IC float Proj(dxGeom* o, const dReal* normal);                                                         \
        IC int Collide(const dReal* v0, const dReal* v1, const dReal* v2, Triangle* T, dxGeom* o1, dxGeom* o2, \
            int flags, dContactGeom* contact, int skip);                                                       \
        IC int CollidePlain(const dReal* triSideAx0, const dReal* triSideAx1, const dReal* triAx, CDB::TRI* T, \
            dReal dist, dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip);                   \
    \
\
};

#define TRI_PRIMITIVE_COLIDE_CLASS_IMPLEMENT(primitive)                                                                \
    IC float dcTriListCollider::primitive##Tri::Proj(dxGeom* o, const dReal* normal)                                   \
    {                                                                                                                  \
        return m_tri_list.d##primitive##Proj(o, normal);                                                               \
    }                                                                                                                  \
    IC int dcTriListCollider::primitive##Tri::Collide(const dReal* v0, const dReal* v1, const dReal* v2, Triangle* T,  \
        dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip)                                            \
    {                                                                                                                  \
        return m_tri_list.dTri##primitive(v0, v1, v2, T, o1, o2, flags, contact, skip);                                \
    }                                                                                                                  \
    IC int dcTriListCollider::primitive##Tri::CollidePlain(const dReal* triSideAx0, const dReal* triSideAx1,           \
        const dReal* triAx, CDB::TRI* T, dReal dist, dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact,         \
        int skip)                                                                                                      \
    {                                                                                                                  \
        return m_tri_list.dSortedTri##primitive(triSideAx0, triSideAx1, triAx, T, dist, o1, o2, flags, contact, skip); \
    }
#endif
