#ifndef dx10r_constants_cacheH
#define dx10r_constants_cacheH
#pragma once

class	ECORE_API  R_constants
{
	enum	BufferType
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

	void					flush_cache	();
public:
	// fp, non-array versions
	
	template<typename T>
	ICF void				set		(R_constant* C, const T& A)		{
		if (C->destination&RC_dest_pixel)	{ set	(C,C->ps,A, BT_PixelBuffer); }	// a_pixel.b_dirty=TRUE;		}
		if (C->destination&RC_dest_vertex)	{ set	(C,C->vs,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;		}
		if (C->destination&RC_dest_geometry){ set	(C,C->gs,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;		}
#ifdef USE_DX11
		if (C->destination&RC_dest_hull)	{ set	(C,C->hs,A, BT_HullBuffer); }	//  a_vertex.b_dirty=TRUE;		}
		if (C->destination&RC_dest_domain)	{ set	(C,C->ds,A, BT_DomainBuffer); }	//  a_vertex.b_dirty=TRUE;		}
		if (C->destination&RC_dest_compute)	{ set	(C,C->cs,A, BT_Compute); }	//  a_vertex.b_dirty=TRUE;		}
#endif
	}

	template<typename T>
	ICF void				seta	(R_constant* C, u32 e, const T& A)		{
		if (C->destination&RC_dest_pixel)	{ seta	(C,C->ps,e,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;	}
		if (C->destination&RC_dest_vertex)	{ seta	(C,C->vs,e,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;	}
		if (C->destination&RC_dest_geometry){ seta	(C,C->gs,e,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;	}
#ifdef USE_DX11
		if (C->destination&RC_dest_hull)	{ seta	(C,C->hs,e,A, BT_HullBuffer); }	//  a_vertex.b_dirty=TRUE;		}
		if (C->destination&RC_dest_domain)	{ seta	(C,C->ds,e,A, BT_DomainBuffer); }	//  a_vertex.b_dirty=TRUE;		}
		if (C->destination&RC_dest_compute)	{ seta	(C,C->cs,e,A, BT_Compute); }	//  a_vertex.b_dirty=TRUE;		}
#endif
	}
	//ICF void				set		(R_constant* C, const Fmatrix& A)		{
	//	if (C->destination&RC_dest_pixel)	{ set	(C,C->ps,A, BT_PixelBuffer); }	// a_pixel.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_vertex)	{ set	(C,C->vs,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_geometry){ set	(C,C->gs,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_hull)	{ set	(C,C->hs,A, BT_HullBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_domain)	{ set	(C,C->ds,A, BT_DomainBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//}
	//ICF void				set		(R_constant* C, const Fvector4& A)		{
	//	if (C->destination&RC_dest_pixel)	{ set	(C,C->ps,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_vertex)	{ set	(C,C->vs,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_geometry){ set	(C,C->gs,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_hull)	{ set	(C,C->hs,A, BT_HullBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_domain)	{ set	(C,C->ds,A, BT_DomainBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//}
	ICF void				set		(R_constant* C, float x, float y, float z, float w)	{
		Fvector4 data;		data.set(x,y,z,w);
		set					(C,data);
	}

	// scalars, non-array versions
	//ICF	void				set		(R_constant* C, float A)
	//{ 
	//	if (C->destination&RC_dest_pixel)	{ set	(C,C->ps,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_vertex)	{ set	(C,C->vs,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_geometry){ set	(C,C->gs,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_hull)	{ set	(C,C->hs,A, BT_HullBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_domain)	{ set	(C,C->ds,A, BT_DomainBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//}

	//ICF	void				set		(R_constant* C, int A)
	//{
	//	if (C->destination&RC_dest_pixel)	{ set	(C,C->ps,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_vertex)	{ set	(C,C->vs,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//	if (C->destination&RC_dest_geometry){ set	(C,C->gs,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;		}
	//}

	// fp, array versions
	//ICF void				seta	(R_constant* C, u32 e, const Fmatrix& A)		{
	//	if (C->destination&RC_dest_pixel)	{ seta	(C,C->ps,e,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;	}
	//	if (C->destination&RC_dest_vertex)	{ seta	(C,C->vs,e,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;	}
	//	if (C->destination&RC_dest_geometry){ seta	(C,C->gs,e,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;	}
	//}
	//ICF void				seta	(R_constant* C, u32 e, const Fvector4& A)		{
	//	if (C->destination&RC_dest_pixel)	{ seta	(C,C->ps,e,A, BT_PixelBuffer); }	//  a_pixel.b_dirty=TRUE;	}
	//	if (C->destination&RC_dest_vertex)	{ seta	(C,C->vs,e,A, BT_VertexBuffer); }	//  a_vertex.b_dirty=TRUE;	}
	//	if (C->destination&RC_dest_geometry){ seta	(C,C->gs,e,A, BT_GeometryBuffer); }	//  a_vertex.b_dirty=TRUE;	}
	//}
	ICF void				seta	(R_constant* C, u32 e, float x, float y, float z, float w)	{
		Fvector4 data;		data.set(x,y,z,w);
		seta				(C,e,data);
	}

	//
	ICF void				flush	()
	{
		//if (a_pixel.b_dirty || a_vertex.b_dirty)	flush_cache();
		flush_cache();
	}

	ICF void				access_direct(R_constant* C, u32 DataSize, void** ppVData, void** ppGData, void** ppPData)
	{
		if (ppPData)
		{
			if (C->destination&RC_dest_pixel)	{ access_direct(C,C->ps,ppPData,DataSize, BT_PixelBuffer); }
			else *ppPData = 0;
		}

		if (ppVData)
		{
			if (C->destination&RC_dest_vertex)	{ access_direct(C,C->vs,ppVData,DataSize, BT_VertexBuffer); }
			else *ppVData = 0;
		}

		if (ppGData)
		{
			if (C->destination&RC_dest_geometry){ access_direct(C,C->gs,ppGData,DataSize, BT_GeometryBuffer); }
			else *ppGData = 0;
		}
	}

private:


	void					set		(R_constant* C, R_constant_load& L, const Fmatrix& A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.set(C, L, A);
	}

	void					set		(R_constant* C, R_constant_load& L, const Fvector4& A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.set(C, L, A);
	}

	void					set		(R_constant* C, R_constant_load& L, float A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.set(C, L, A);
	}

	void					set		(R_constant* C, R_constant_load& L, int A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.set(C, L, A);
	}

	void					seta	(R_constant* C, R_constant_load& L, u32 e, const Fmatrix& A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.seta(C, L, e, A);
	}

	void					seta	(R_constant* C, R_constant_load& L, u32 e, const Fvector4& A, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		Buffer.seta(C, L, e, A);	
	}

	void					access_direct(R_constant* C, R_constant_load& L, void **ppData, u32 DataSize, BufferType BType)
	{
		dx10ConstantBuffer& Buffer = GetCBuffer(C, BType);
		*ppData = Buffer.AccessDirect( L, DataSize );
	}

	dx10ConstantBuffer&		GetCBuffer(R_constant* C, BufferType BType);
};
#endif	//	dx10r_constants_cacheH