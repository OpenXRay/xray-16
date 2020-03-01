#ifndef __MESHSTRUCTURE_H__
#define __MESHSTRUCTURE_H__

//#ifdef MESHSTRUCTURE_EXSPORTS_IMPORTS
#define MESHSTRUCTURE_API XRLC_LIGHT_API
//#else
//#	define MESHSTRUCTURE_API
//#endif

// typedef	xr_vector<_vertex*>		v_vertices;
// typedef	v_vertices::iterator	v_vertices_it;

// typedef v_faces::iterator		v_faces_it;
// typedef xr_vector<_subdiv>		v_subdivs;
// typedef v_subdivs::iterator		v_subdivs_it;
// extern	volatile	u32		dwInvalidFaces;
class MESHSTRUCTURE_API vector_item
{
protected:
    vector_item() : m_self_index(u32(-1)) {}
private:
    u32 m_self_index;

public:
    IC void set_index(u32 self_index) { m_self_index = self_index; }
    IC u32 self_index() const { return m_self_index; }
};
template <typename DataVertexType>
struct Tvertex;
class CDeflector;
template <typename DataVertexType>
struct MESHSTRUCTURE_API Tface : public DataVertexType::DataFaceType, public vector_item
{
    typedef Tvertex<DataVertexType> type_vertex;
    typedef Tface<DataVertexType> type_face;
    // private:
    type_vertex* v[3];
    //	u32				m_self_index;
public:
    // IC	void			set_index	( u32 idx )		{ m_self_index = idx; }
    // IC	u32				self_index	( )	const		{return m_self_index ;}
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    Tface();
    virtual ~Tface();
    static Tface* read_create();
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void Verify();
    void Failure();
    void OA_Unwarp(CDeflector* d);

    virtual void read(INetReader& r);
    virtual void write(IWriter& w) const;
    virtual void read_vertices(INetReader& r);
    virtual void write_vertices(IWriter& w) const;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    IC void raw_set_vertex(u8 index, type_vertex* _v)
    {
        R_ASSERT(index < 3);
        v[index] = _v;
    }
    IC type_vertex* vertex(u8 index)
    {
        R_ASSERT(index < 3);
        return v[index];
    }

    /////////////////////////////////////////////////////////
    // Does the face contains this vertex?
    IC bool VContains(const type_vertex* pV) { return VIndex(pV) >= 0; };
    // Replace ONE vertex by ANOTHER
    IC void VReplace(type_vertex* what, type_vertex* to)
    {
        if (v[0] == what)
        {
            v[0] = to;
            what->prep_remove(this);
            to->prep_add(this);
        }
        if (v[1] == what)
        {
            v[1] = to;
            what->prep_remove(this);
            to->prep_add(this);
        }
        if (v[2] == what)
        {
            v[2] = to;
            what->prep_remove(this);
            to->prep_add(this);
        }
    };
    IC void VReplace_not_remove(type_vertex* what, type_vertex* to)
    {
        if (v[0] == what)
        {
            v[0] = to;
            to->prep_add(this);
        }
        if (v[1] == what)
        {
            v[1] = to;
            to->prep_add(this);
        }
        if (v[2] == what)
        {
            v[2] = to;
            to->prep_add(this);
        }
    };
    IC int VIndex(const type_vertex* pV)
    {
        if (v[0] == pV)
            return 0;
        if (v[1] == pV)
            return 1;
        if (v[2] == pV)
            return 2;
        return -1;
    };

    IC void SetVertex(size_t idx, type_vertex* V)
    {
        v[idx] = V;
        V->prep_add(this);
    };
    IC void SetVertices(type_vertex* V1, type_vertex* V2, type_vertex* V3)
    {
        SetVertex(0, V1);
        SetVertex(1, V2);
        SetVertex(2, V3);
    };
    IC BOOL isDegenerated() { return (v[0] == v[1] || v[0] == v[2] || v[1] == v[2]); };
    IC float EdgeLen(size_t edge)
    {
        type_vertex* V1 = v[edge2idx[edge][0]];
        type_vertex* V2 = v[edge2idx[edge][1]];
        return V1->P.distance_to(V2->P);
    };
    IC void EdgeVerts(size_t e, type_vertex** A, type_vertex** B) const
    {
        *A = v[edge2idx[e][0]];
        *B = v[edge2idx[e][1]];
    }

    BOOL isEqual(type_face& F)
    {
        // Test for 6 variations
        if ((v[0] == F.v[0]) && (v[1] == F.v[1]) && (v[2] == F.v[2]))
            return true;
        if ((v[0] == F.v[0]) && (v[2] == F.v[1]) && (v[1] == F.v[2]))
            return true;
        if ((v[2] == F.v[0]) && (v[0] == F.v[1]) && (v[1] == F.v[2]))
            return true;
        if ((v[2] == F.v[0]) && (v[1] == F.v[1]) && (v[0] == F.v[2]))
            return true;
        if ((v[1] == F.v[0]) && (v[0] == F.v[1]) && (v[2] == F.v[2]))
            return true;
        if ((v[1] == F.v[0]) && (v[2] == F.v[1]) && (v[0] == F.v[2]))
            return true;
        return false;
    }

    void CalcNormal()
    {
        Fvector t1, t2;

        Fvector* v0 = &(v[0]->P);
        Fvector* v1 = &(v[1]->P);
        Fvector* v2 = &(v[2]->P);
        t1.sub(*v1, *v0);
        t2.sub(*v2, *v1);
        N.crossproduct(t1, t2);
        float mag = N.magnitude();
        if (mag < EPS_S)
        {
            Fvector3 save_N = N;
            if (exact_normalize(save_N))
                N = save_N;
            else
                CalcNormal2();
        }
        else
        {
            N.div(mag);
            N.normalize();
        }
    }

    void CalcNormal2()
    {
        FPU::m64r();
        Dvector v0, v1, v2, t1, t2, dN;
        v0.set(v[0]->P);
        v1.set(v[1]->P);
        v2.set(v[2]->P);
        t1.sub(v1, v0);
        t2.sub(v2, v1);
        dN.crossproduct(t1, t2);
        double mag = dN.magnitude();
        if (mag < dbl_zero)
        {
            Failure();
            Dvector Nabs;
            Nabs.abs(dN);

#define SIGN(a) ((a >= 0.f) ? 1.f : -1.f)
            if (Nabs.x > Nabs.y && Nabs.x > Nabs.z)
                N.set(SIGN(N.x), 0.f, 0.f);
            else if (Nabs.y > Nabs.x && Nabs.y > Nabs.z)
                N.set(0.f, SIGN(N.y), 0.f);
            else if (Nabs.z > Nabs.x && Nabs.z > Nabs.y)
                N.set(0.f, 0.f, SIGN(N.z));
            else
            {
                N.set(0, 1, 0);
            }
#undef SIGN
        }
        else
        {
            dN.div(mag);
            N.set(dN);
        }
    }

    float CalcArea() const
    {
        float e1 = v[0]->P.distance_to(v[1]->P);
        float e2 = v[0]->P.distance_to(v[2]->P);
        float e3 = v[1]->P.distance_to(v[2]->P);

        float p = (e1 + e2 + e3) / 2.f;
        return _sqrt(p * (p - e1) * (p - e2) * (p - e3));
    }
    float CalcMaxEdge()
    {
        float e1 = v[0]->P.distance_to(v[1]->P);
        float e2 = v[0]->P.distance_to(v[2]->P);
        float e3 = v[1]->P.distance_to(v[2]->P);

        if (e1 > e2 && e1 > e3)
            return e1;
        if (e2 > e1 && e2 > e3)
            return e2;
        return e3;
    }
    void CalcCenter(Fvector& C)
    {
        C.set(v[0]->P);
        C.add(v[1]->P);
        C.add(v[2]->P);
        C.div(3);
    };
};

template <typename DataVertexType>
struct MESHSTRUCTURE_API Tvertex : public DataVertexType, public vector_item
{
    typedef Tface<DataVertexType> type_face;
    typedef Tvertex<DataVertexType> type_vertex;

    typedef xr_vector<type_face*> v_faces;
    typedef typename v_faces::iterator v_faces_it;

    // typedef typename xr_vector<type_vertex>::iterator v_dummy;
    typedef xr_vector<type_vertex*> v_vertices;

    typedef typename v_vertices::iterator v_vertices_it;
    //////////////////////////////////////////////////////////////
    Tvertex();
    virtual ~Tvertex();
    Tvertex* CreateCopy_NOADJ(v_vertices& vertises_storage) const;
    static Tvertex* read_create();

    virtual void read(INetReader& r);
    virtual void write(IWriter& w) const;

    //////////////////////////////////////////////////////////////
    void isolate_pool_clear_read(INetReader& r);
    void isolate_pool_clear_write(IWriter& w) const;

    void read_adjacents(INetReader& r);
    void write_adjacents(IWriter& w) const;
    ///////////////////////////////////////////////////////////////
    v_faces m_adjacents;

    IC type_vertex* CreateCopy(v_vertices& vertises_storage)
    {
        type_vertex* V = CreateCopy_NOADJ(vertises_storage);
        V->m_adjacents = m_adjacents;
        return V;
    }
    IC void prep_add(type_face* F)
    {
        v_faces_it I = std::find(m_adjacents.begin(), m_adjacents.end(), F);
        if (I == m_adjacents.end())
            m_adjacents.push_back(F);
    }

    IC void prep_remove(type_face* F)
    {
        v_faces_it I = std::find(m_adjacents.begin(), m_adjacents.end(), F);
        if (I != m_adjacents.end())
            m_adjacents.erase(I);
    }

    IC void normalFromAdj()
    {
        N.set(0, 0, 0);
        for (v_faces_it ad = m_adjacents.begin(); ad != m_adjacents.end(); ++ad)
            N.add((*ad)->N);
        exact_normalize(N);
    }
};

template <typename typeVertex>
IC void _destroy_vertex(typeVertex*& v, bool unregister)
{
    destroy_vertex(v, unregister);
}

template <typename typeVertex>
struct remove_pred
{
    bool operator()(typeVertex*& v)
    {
        if (v && v->m_adjacents.empty())
        {
            _destroy_vertex(v, false);
            return true;
        }
        return false;
    }
};

template <typename typeVertex>
IC void isolate_vertices(BOOL bProgress, xr_vector<typeVertex*>& vertices)
{
    if (bProgress)
        Logger.Status("Isolating vertices...");
    // g_bUnregister		= false;
    const size_t verts_old = vertices.size();

    for (size_t it = 0; it < verts_old; ++it)
    {
        if (bProgress)
            Logger.Progress(float(it) / float(verts_old));

        if (vertices[it] && vertices[it]->m_adjacents.empty())
            _destroy_vertex(vertices[it], false);
    }
    VERIFY(verts_old == vertices.size());

    auto _end = std::remove(vertices.begin(), vertices.end(), (typeVertex*)0);

    /*
        remove_pred<typeVertex> rp;
        xr_vector<typeVertex*>::iterator	_end	= std::remove_if	(vertices.begin(),vertices.end(),rp);

    */
    vertices.erase(_end, vertices.end());
    // g_bUnregister		= true;
    Memory.mem_compact();

    if (bProgress)
        Logger.Progress(1.f);

    size_t verts_new = vertices.size();
    size_t _count = verts_old - verts_new;

    if (_count)
        Logger.clMsg("::compact:: %d verts removed", _count);
}

#endif //__MESHSTRUCTURE_H__
