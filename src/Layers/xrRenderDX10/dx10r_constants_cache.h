#ifndef dx10r_constants_cacheH
#define dx10r_constants_cacheH
#pragma once

class ECORE_API R_constants
{
    enum BufferType
    {
        BT_PixelBuffer,
        BT_VertexBuffer,
        BT_GeometryBuffer,
        BT_HullBuffer,
        BT_DomainBuffer,
        BT_Compute
    };

public:
    //	ALIGN(16)	R_constant_array	a_pixel;
    //	ALIGN(16)	R_constant_array	a_vertex;

    void flush_cache();

public:
    // fp, non-array versions
    template <typename... Args>
    ICF void set(R_constant* C, Args&&... args)
    {
        if (C->destination & RC_dest_pixel)
        {
            set<BT_PixelBuffer>(C, C->ps, std::forward<Args>(args)...);
        } // a_pixel.b_dirty=TRUE;		}
        if (C->destination & RC_dest_vertex)
        {
            set<BT_VertexBuffer>(C, C->vs, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
        if (C->destination & RC_dest_geometry)
        {
            set<BT_GeometryBuffer>(C, C->gs, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
#ifdef USE_DX11
        if (C->destination & RC_dest_hull)
        {
            set<BT_HullBuffer>(C, C->hs, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
        if (C->destination & RC_dest_domain)
        {
            set<BT_DomainBuffer>(C, C->ds, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
        if (C->destination & RC_dest_compute)
        {
            set<BT_Compute>(C, C->cs, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
#endif
    }

    // scalars, non-array versions
    ICF void set(R_constant* C, float x, float y, float z, float w)
    {
        Fvector4 data;
        data.set(x, y, z, w);
        set(C, data);
    }

    // fp, array versions
    template <typename... Args>
    ICF void seta(R_constant* C, u32 e, Args&& ... args)
    {
        if (C->destination & RC_dest_pixel)
        {
            seta<BT_PixelBuffer>(C, C->ps, e, std::forward<Args>(args)...);
        } //  a_pixel.b_dirty=TRUE;	}
        if (C->destination & RC_dest_vertex)
        {
            seta<BT_VertexBuffer>(C, C->vs, e, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;	}
        if (C->destination & RC_dest_geometry)
        {
            seta<BT_GeometryBuffer>(C, C->gs, e, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;	}
#ifdef USE_DX11
        if (C->destination & RC_dest_hull)
        {
            seta<BT_HullBuffer>(C, C->hs, e, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
        if (C->destination & RC_dest_domain)
        {
            seta<BT_DomainBuffer>(C, C->ds, e, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
        if (C->destination & RC_dest_compute)
        {
            seta<BT_Compute>(C, C->cs, e, std::forward<Args>(args)...);
        } //  a_vertex.b_dirty=TRUE;		}
#endif
    }

    // scalars, array versions
    ICF void seta(R_constant* C, u32 e, float x, float y, float z, float w)
    {
        Fvector4 data;
        data.set(x, y, z, w);
        seta(C, e, data);
    }

    //
    ICF void flush()
    {
        // if (a_pixel.b_dirty || a_vertex.b_dirty)	flush_cache();
        flush_cache();
    }

    ICF void access_direct(R_constant* C, u32 DataSize, void** ppVData, void** ppGData, void** ppPData)
    {
        if (ppPData)
        {
            if (C->destination & RC_dest_pixel)
            {
                access_direct(C, C->ps, ppPData, DataSize, BT_PixelBuffer);
            }
            else
                *ppPData = 0;
        }

        if (ppVData)
        {
            if (C->destination & RC_dest_vertex)
            {
                access_direct(C, C->vs, ppVData, DataSize, BT_VertexBuffer);
            }
            else
                *ppVData = 0;
        }

        if (ppGData)
        {
            if (C->destination & RC_dest_geometry)
            {
                access_direct(C, C->gs, ppGData, DataSize, BT_GeometryBuffer);
            }
            else
                *ppGData = 0;
        }
    }

private:
    template<BufferType BType, typename... Args>
    void set(R_constant* C, R_constant_load& L, Args&&... args)
    {
        dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
        Buffer.set(C, L, std::forward<Args>(args)...);
    }

    template<BufferType BType, typename... Args>
    void seta(R_constant* C, R_constant_load& L, u32 e, Args&&... args)
    {
        dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
        Buffer.seta(C, L, e, std::forward<Args>(args)...);
    }

    void access_direct(R_constant* C, R_constant_load& L, void** ppData, u32 DataSize, BufferType BType)
    {
        dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
        *ppData = Buffer.AccessDirect(L, DataSize);
    }

    dx10ConstantBuffer& GetCBuffer(R_constant* C, BufferType BType);
};
#endif //	dx10r_constants_cacheH
