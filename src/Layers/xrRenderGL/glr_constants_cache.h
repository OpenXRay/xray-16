#ifndef glr_constants_cacheH
#define glr_constants_cacheH
#pragma once

class	ECORE_API  R_constants
{
private:
	// fp, non-array versions
	ICF void				set(R_constant* C, R_constant_load& L, const Fmatrix& A)		{
		VERIFY(RC_float == C->type);
		Fvector4	it[4];
		switch (L.cls)
		{
		case RC_2x4:
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			CHK_GL(glProgramUniformMatrix4x2fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
			break;
		case RC_3x4:
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			CHK_GL(glProgramUniformMatrix4x3fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
			break;
		case RC_4x4:
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			CHK_GL(glProgramUniformMatrix4fv(L.program, L.location, 1, GL_TRUE, (float*)&it));
			break;
		default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
	}
	ICF void				set(R_constant* C, R_constant_load& L, const Fvector4& A)		{
		VERIFY(RC_float == C->type);
		switch (L.cls)
		{
		case RC_1x2:
			CHK_GL(glProgramUniform2fv(L.program, L.location, 1, (float*)&A));
			break;
		case RC_1x3:
			CHK_GL(glProgramUniform3fv(L.program, L.location, 1, (float*)&A));
			break;
		case RC_1x4:
			CHK_GL(glProgramUniform4fv(L.program, L.location, 1, (float*)&A));
			break;
		default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
	}
	ICF void				set(R_constant* C, R_constant_load& L, float x, float y, float z, float w)	{
		VERIFY(RC_float == C->type);
		switch (L.cls)
		{
		case RC_1x2:
			CHK_GL(glProgramUniform2f(L.program, L.location, x, y));
			break;
		case RC_1x3:
			CHK_GL(glProgramUniform3f(L.program, L.location, x, y, z));
			break;
		case RC_1x4:
			CHK_GL(glProgramUniform4f(L.program, L.location, x, y, z, w));
			break;
		default:
#ifdef DEBUG
			xrDebug::Fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
	}

	// scalars, non-array versions
	ICF	void				set(R_constant* C, R_constant_load& L, float A)
	{
		VERIFY(RC_float == C->type);
		VERIFY(RC_1x1 == L.cls);
		CHK_GL(glProgramUniform1f(L.program, L.location, A));
	}
	ICF	void				set(R_constant* C, R_constant_load& L, int A)
	{
		VERIFY(RC_int == C->type);
		VERIFY(RC_1x1 == L.cls);
		CHK_GL(glProgramUniform1i(L.program, L.location, A));
	}

public:
	// fp, non-array versions
	ICF void				set		(R_constant* C, const Fmatrix& A)		{
		if (C->destination&RC_dest_pixel)		{ set(C, C->ps, A); }
		if (C->destination&RC_dest_vertex)		{ set(C, C->vs, A); }
		if (C->destination&RC_dest_geometry)		{ set(C, C->gs, A); }
	}
	ICF void				set		(R_constant* C, const Fvector4& A)		{
		if (C->destination&RC_dest_pixel)		{ set(C, C->ps, A); }
		if (C->destination&RC_dest_vertex)		{ set(C, C->vs, A); }
		if (C->destination&RC_dest_geometry)		{ set(C, C->gs, A); }
	}
	ICF void				set		(R_constant* C, float x, float y, float z, float w)	{
		if (C->destination&RC_dest_pixel)		{ set(C, C->ps, x, y, z, w); }
		if (C->destination&RC_dest_vertex)		{ set(C, C->vs, x, y, z, w); }
		if (C->destination&RC_dest_geometry)		{ set(C, C->gs, x, y, z, w); }
	}

	// scalars, non-array versions
	ICF	void				set(R_constant* C, float A)
	{
		if (C->destination&RC_dest_pixel)		{ set(C, C->ps, A); }
		if (C->destination&RC_dest_vertex)		{ set(C, C->vs, A); }
		if (C->destination&RC_dest_geometry)		{ set(C, C->gs, A); }
	}
	ICF	void				set(R_constant* C, int A)
	{
		if (C->destination&RC_dest_pixel)		{ set(C, C->ps, A); }
		if (C->destination&RC_dest_vertex)		{ set(C, C->vs, A); }
		if (C->destination&RC_dest_geometry)		{ set(C, C->gs, A); }
	}

	// fp, array versions
	ICF void				seta	(R_constant* C, u32 e, const Fmatrix& A)		{
		R_constant_load L;
		if (C->destination&RC_dest_pixel)		{ L = C->ps; }
		if (C->destination&RC_dest_vertex)		{ L = C->vs; }
		if (C->destination&RC_dest_geometry)		{ L = C->gs; }
		L.location += e;
		set(C, L, A);
	}
	ICF void				seta	(R_constant* C, u32 e, const Fvector4& A)		{
		R_constant_load L;
		if (C->destination&RC_dest_pixel)		{ L = C->ps; }
		if (C->destination&RC_dest_vertex)		{ L = C->vs; }
		if (C->destination&RC_dest_geometry)		{ L = C->gs; }
		L.location += e;
		set(C, L, A);
	}
	ICF void				seta	(R_constant* C, u32 e, float x, float y, float z, float w)	{
		R_constant_load L;
		if (C->destination&RC_dest_pixel)		{ L = C->ps; }
		if (C->destination&RC_dest_vertex)		{ L = C->vs; }
		if (C->destination&RC_dest_geometry)		{ L = C->gs; }
		L.location += e;
		set(C, L, x, y, z, w);

	}

	// TODO: OGL: Implement constant caching through UBOs
	ICF void					flush	()
	{
	}
};
#endif	//	glr_constants_cacheH
