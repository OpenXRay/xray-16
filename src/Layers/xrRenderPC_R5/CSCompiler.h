////////////////////////////////////////////////////////////////////////////
//	Created		: 22.05.2009
//	Author		: Mykhailo Parfeniuk
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef CSCOMPILER_H_INCLUDED
#define CSCOMPILER_H_INCLUDED

class ComputeShader;

class CSCompiler
{
public:
    CSCompiler(ComputeShader& target);

    CSCompiler& begin(const char* name);
    CSCompiler& defSampler(LPCSTR ResourceName);
    CSCompiler& defSampler(LPCSTR ResourceName, const D3D_SAMPLER_DESC& def);
    CSCompiler& defOutput(LPCSTR ResourceName, const ref_rt& rt);
    CSCompiler& defTexture(LPCSTR ResourceName, ref_texture texture);
    void end();

private:
    // suppress warning
    CSCompiler& operator=(const CSCompiler& other);

    void compile(const char* name);

private:
    ComputeShader& m_Target;
    ID3D11ComputeShader* m_cs;
    R_constant_table m_constants;
    xr_vector<ID3D11SamplerState*> m_Samplers;
    xr_vector<ID3D11ShaderResourceView*> m_Textures;
    xr_vector<ID3D11UnorderedAccessView*> m_Outputs;
}; // class CSCompiler

#endif // #ifndef CSCOMPILER_H_INCLUDED
