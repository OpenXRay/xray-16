////////////////////////////////////////////////////////////////////////////
//	Created		: 21.05.2009
//	Author		: Mykhailo Parfeniuk
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef COMPUTESHADER_H_INCLUDED
#define COMPUTESHADER_H_INCLUDED

class ComputeShader
{
	friend class CSCompiler;
public:
	~ComputeShader();

	ComputeShader& set_c(shared_str name, const Fvector4& value);
	ComputeShader& set_c(shared_str name, float x, float y, float z, float w);

	void Dispatch(u32 dimx, u32 dimy, u32 dimz);

private:
	void Construct(
		ID3D11ComputeShader*	cs,
		ref_ctable				ctable,
		xr_vector<ID3D11SamplerState*>&			Samplers,
		xr_vector<ID3D11ShaderResourceView*>&	Textures,
		xr_vector<ID3D11UnorderedAccessView*>&	Outputs
	);

private:
	ID3D11ComputeShader*	m_cs;
	ref_ctable				m_ctable;
	xr_vector<ID3D11SamplerState*>			m_Samplers;
	xr_vector<ID3D11ShaderResourceView*>	m_Textures;
	xr_vector<ID3D11UnorderedAccessView*>	m_Outputs;
}; // class ComputeShader

#endif // #ifndef COMPUTESHADER_H_INCLUDED